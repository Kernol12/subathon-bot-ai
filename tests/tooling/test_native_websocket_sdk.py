# SPDX-License-Identifier: GPL-3.0-or-later
"""Configuration admission only: synthetic files, no compiler, DSO or native body."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "cmake/DrogonDependency.cmake"
PROFILE = ROOT / "dependencies/native-websocket-profile/manifest.json"
REQUIRED = (
    "include/drogon/HttpAppFramework.h",
    "include/drogon/WebSocketConnection.h",
    "include/trantor/net/TcpConnection.h",
    "include/trantor/utils/MsgBuffer.h",
    "lib/libdrogon.so.1.9.14",
    "lib/libtrantor.so.1.5.29",
    "share/licenses/drogon/LICENSE",
    "share/licenses/drogon/orm_lib-COPYING",
    "share/licenses/trantor/License",
    "share/subathon-native-profile/manifest.json",
)


class NativeWebSocketSdkTests(unittest.TestCase):
    def fixture(self, directory):
        sdk = directory / "sdk"
        for name in REQUIRED:
            path = sdk / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("synthetic configuration input; never loaded\n", encoding="utf-8")
        (sdk / REQUIRED[-1]).write_bytes(PROFILE.read_bytes())
        for package, version, library in (
                ("Trantor", "1.5.29", "libtrantor.so.1.5.29"),
                ("Drogon", "1.9.14", "libdrogon.so.1.9.14")):
            config = sdk / "lib/cmake" / package
            config.mkdir(parents=True)
            (config / f"{package}Config.cmake").write_text(
                'get_filename_component(prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)\n'
                f'add_library({package}::{package} SHARED IMPORTED)\n'
                f'set_target_properties({package}::{package} PROPERTIES\n'
                f' IMPORTED_LOCATION_RELEASE "${{prefix}}/lib/{library}"\n'
                ' INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")\n', encoding="utf-8")
            (config / f"{package}ConfigVersion.cmake").write_text(
                f'set(PACKAGE_VERSION "{version}")\n'
                'if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)\n'
                ' set(PACKAGE_VERSION_EXACT TRUE)\n'
                ' set(PACKAGE_VERSION_COMPATIBLE TRUE)\n'
                'endif()\n', encoding="utf-8")
        return sdk

    def seal(self, sdk):
        identity = {
            "format": 1,
            "profile_manifest_sha256": hashlib.sha256(PROFILE.read_bytes()).hexdigest(),
            "artifacts": {
                p.relative_to(sdk).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted(sdk.rglob("*"))
                if p.is_file() and p.name != "dependency-build.json"
            },
            "toolchain": {"scope": "synthetic configuration only, not ABI proof"},
        }
        self.write_identity(sdk, identity)
        return identity

    def write_identity(self, sdk, identity):
        (sdk / "dependency-build.json").write_text(json.dumps(identity), encoding="utf-8")

    def configure(self, directory, sdk):
        cmake = shutil.which("cmake")
        self.assertIsNotNone(cmake, "CMake is required by the repository tooling contract")
        project = directory / "project"
        project.mkdir()
        (project / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.25)\n'
            'project(SyntheticSdkAdmission LANGUAGES NONE)\n'
            '# Model the admitted architecture; do not probe a compiler or load the dummy DSOs.\n'
            'set(CMAKE_SYSTEM_NAME Linux)\n'
            'set(CMAKE_SYSTEM_PROCESSOR x86_64)\n'
            'set(CMAKE_SIZEOF_VOID_P 8)\n'
            'include("${ADMISSION_MODULE}")\n', encoding="utf-8")
        return subprocess.run(
            [cmake, "-S", str(project), "-B", str(directory / "build"),
             f"-DADMISSION_MODULE:FILEPATH={MODULE}", f"-DSUBATHON_DROGON_ROOT:PATH={sdk}"],
            capture_output=True, text=True, encoding="utf-8", timeout=15)

    def test_exact_corrected_identity_and_explicit_targets_are_admitted(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            sdk = self.fixture(directory)
            self.seal(sdk)
            result = self.configure(directory, sdk)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_pristine_identity_is_not_a_corrected_sdk(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            sdk = self.fixture(directory)
            profile = json.loads(PROFILE.read_text(encoding="utf-8"))
            self.write_identity(sdk, {
                "drogon": profile["upstreams"]["drogon"],
                "trantor": profile["upstreams"]["trantor"],
                "options": profile["build_options"],
            })
            result = self.configure(directory, sdk)
            self.assertNotEqual(result.returncode, 0, "Stock identity cannot admit patched-only ABI")

    def test_profile_digest_and_copied_manifest_must_both_match(self):
        for change in ("digest", "copied_manifest"):
            with self.subTest(change=change), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                sdk = self.fixture(directory)
                identity = self.seal(sdk)
                if change == "digest":
                    identity["profile_manifest_sha256"] = "0" * 64
                    self.write_identity(sdk, identity)
                else:
                    (sdk / REQUIRED[-1]).write_text("{}\n", encoding="utf-8")
                    self.seal(sdk)  # An internally consistent inventory is still the wrong profile.
                result = self.configure(directory, sdk)
                self.assertNotEqual(result.returncode, 0)

    def test_missing_changed_or_unlisted_installed_files_are_refused(self):
        for change in ("missing_header", "missing_notice", "changed_dso", "extra_header"):
            with self.subTest(change=change), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                sdk = self.fixture(directory)
                self.seal(sdk)
                if change == "missing_header":
                    (sdk / REQUIRED[0]).unlink()
                elif change == "missing_notice":
                    (sdk / "share/licenses/drogon/orm_lib-COPYING").unlink()
                elif change == "changed_dso":
                    (sdk / "lib/libdrogon.so.1.9.14").write_bytes(b"different synthetic library")
                else:
                    (sdk / "include/drogon/extra.h").write_text("unlisted header", encoding="utf-8")
                result = self.configure(directory, sdk)
                self.assertNotEqual(result.returncode, 0)

    def test_artifact_symlink_cannot_escape_explicit_prefix(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            sdk = self.fixture(directory)
            library = sdk / "lib/libdrogon.so.1.9.14"
            outside = directory / "outside.so"
            outside.write_bytes(library.read_bytes())
            library.unlink()
            library.symlink_to(outside)
            self.seal(sdk)  # Matching bytes do not grant ownership of an outside DSO.
            result = self.configure(directory, sdk)
            self.assertNotEqual(result.returncode, 0)

    def test_package_target_cannot_import_a_different_prefix(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            sdk = self.fixture(directory)
            outside = directory / "outside.so"
            outside.write_bytes(b"synthetic wrong target")
            config = sdk / "lib/cmake/Drogon/DrogonConfig.cmake"
            text = config.read_text(encoding="utf-8")
            config.write_text(text.replace("${prefix}/lib/libdrogon.so.1.9.14", outside.as_posix()),
                              encoding="utf-8")
            self.seal(sdk)
            result = self.configure(directory, sdk)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
