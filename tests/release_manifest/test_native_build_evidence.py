"""PACKAGE-001 synthetic examples. No native toolchain or runtime is invoked."""

from copy import deepcopy
import unittest

from fixture import identity as bytes_identity
from native_build_evidence_candidate import Qualified, Unqualified, qualify_native_build


def identity(data):
    return bytes_identity(data.encode("utf-8"))


def component(name, version, role, license_name, *, source=True):
    declaration = {"value": license_name, "source": name + "-upstream-notice"}
    return {
        "revision": version,
        "role": role,
        "material": identity(name + version),
        "license": declaration,
        **(
            {"source": "https://example.invalid/src/" + name}
            if source
            else {"vendor": "https://example.invalid/download/" + name}
        ),
    }


def fixture(profile="desktop-linux-archive"):
    windows = profile == "desktop-win-zip"
    arch = profile == "desktop-arch-recipe"
    server = profile == "server-native"
    executable = "bin/subathon.exe" if windows else "bin/subathon"
    platform = "windows-11-x86_64" if windows else "ubuntu-24.04-x86_64"
    if arch:
        platform = "arch-linux-x86_64"
    selected = {
        "profile": profile,
        "revision": "synthetic-release-r1",
        "scope": "synthetic-release-r1/" + profile,
        "platform": platform,
        "recipe": "native-release",
        "presets": ("release-configure", "release-build", "release-acceptance"),
        "release_qt": "6.12.3",
        "environment": {"name": platform, "material": identity("clean-" + platform)},
        "inputs": {
            "compiler": (
                component("msvc-2022", "14.40.1", "build-only", "LicenseRef-MSVC", source=False)
                if windows
                else component("gcc", "14.2.0", "build-only", "GPL-3.0-or-later")
            ),
            "linker": (
                component("msvc-link", "14.40.1", "build-only", "LicenseRef-MSVC", source=False)
                if windows
                else component("ld", "2.44", "build-only", "GPL-3.0-or-later")
            ),
            "cmake": component("cmake", "3.30.1", "build-only", "BSD-3-Clause"),
            "ninja": component("ninja", "1.12.1", "build-only", "Apache-2.0"),
            "qt": component("qt", "6.12.3", "external" if arch else "carried", "LGPL-3.0-only"),
            "parser": component("parser", "2.1.0", "embedded", "MIT"),
        },
    }
    selected["inputs"]["parser"]["patches"] = {"portability.patch": identity("public patch")}
    linked = {"qt-core": "qt", "parser": "parser"}
    runtime_loaded = {} if server else {"qt-platform": "qt", "qt-qml-controls": "qt"}
    files = {executable: identity("application bytes"), "notices/Qt.txt": identity("Qt notice")}
    if not arch:
        files["runtime/qt-core"] = identity("Qt Core bytes")
        for name in runtime_loaded:
            files["runtime/" + name] = identity(name + " bytes")
    if windows:
        selected["inputs"]["msvc-runtime"] = component(
            "msvc-runtime", "14.40.1", "carried", "LicenseRef-MSVC-Redist", source=False
        )
        linked["msvc-runtime"] = "msvc-runtime"
        files["runtime/msvc-runtime.dll"] = identity("MSVC runtime bytes")
    selected["runtime"] = {**linked, **runtime_loaded}
    selected["members"] = files
    observed = {
        "profile": profile,
        "run": "first-clean-build",
        "revision": selected["revision"],
        "scope": selected["scope"],
        "platform": platform,
        "recipe": selected["recipe"],
        "presets": selected["presets"],
        "environment": deepcopy(selected["environment"]),
        "inputs": deepcopy(selected["inputs"]),
        "linked": linked,
        "runtime_loaded": runtime_loaded,
        "members": deepcopy(files),
        "output": identity("reproducible synthetic artifact"),
    }
    repeated = deepcopy(observed)
    repeated["run"] = "second-clean-build"
    return selected, observed, repeated


class PackageNativeEvidenceTest(unittest.TestCase):
    def qualified(self, selected, observed, repeated):
        result = qualify_native_build(selected, observed, repeated)
        self.assertIsInstance(result, Qualified)
        return result.evidence

    def refused(self, selected, observed, repeated, code, subject):
        result = qualify_native_build(selected, observed, repeated)
        self.assertIsInstance(result, Unqualified)
        self.assertIn((code, subject), result.findings)

    def test_matching_linux_profile_emits_owned_build_evidence(self):
        for profile in ("server-native", "desktop-linux-archive"):
            with self.subTest(profile=profile):
                selected, observed, repeated = fixture(profile)
                evidence = self.qualified(selected, observed, repeated)
                build = next(row for row in evidence if row["kind"] == "observed-build")
                self.assertEqual(build["facts"]["output"], observed["output"])
                self.assertEqual(build["facts"]["recipe"], selected["recipe"])
                self.assertEqual(build["scope"], selected["scope"])
                self.assertTrue(any(row["kind"] == "observed-build-recipe" for row in evidence))
                self.assertFalse(
                    any(row["kind"] in {"rights-review", "source-review"} for row in evidence)
                )

    def test_missing_or_pre_minimum_qt_patch_refuses(self):
        for version in (None, "6.12", "6.11.9"):
            with self.subTest(version=version):
                selected, observed, repeated = fixture()
                selected["release_qt"] = version
                self.refused(selected, observed, repeated, "qt_release_pin", "qt")

    def test_locally_matching_windows_patch_must_match_release_pin(self):
        selected, observed, repeated = fixture("desktop-win-zip")
        for row in (selected, observed, repeated):
            row["inputs"]["qt"]["revision"] = "6.12.4"
            row["inputs"]["qt"]["material"] = identity("Qt 6.12.4 Windows")
        self.refused(selected, observed, repeated, "qt_release_pin", "qt")

    def test_exact_msvc_vendor_material_qualifies_without_compiler_source(self):
        selected, observed, repeated = fixture("desktop-win-zip")
        self.assertNotIn("source", selected["inputs"]["compiler"])
        self.qualified(selected, observed, repeated)

    def test_selected_input_identity_cannot_be_replaced_by_observed_material(self):
        for name, field, value in (
            ("compiler", "revision", "different compiler"),
            ("qt", "material", identity("another build of Qt")),
            ("parser", "patches", {"portability.patch": identity("another patch")}),
        ):
            with self.subTest(name=name, field=field):
                selected, observed, repeated = fixture()
                observed["inputs"][name][field] = value
                self.refused(selected, observed, repeated, "selected_input_mismatch", name)

    def test_mutable_environment_name_is_not_an_exact_pin(self):
        selected, observed, repeated = fixture()
        del selected["environment"]["material"]
        self.refused(selected, observed, repeated, "environment_pin", "environment")

    def test_missing_linked_dependency_blocks_the_profile(self):
        selected, observed, repeated = fixture()
        del observed["linked"]["qt-core"]
        self.refused(selected, observed, repeated, "runtime_missing", "qt-core")

    def test_dynamic_requirements_cannot_be_satisfied_by_link_listing_alone(self):
        for missing in ("qt-platform", "qt-qml-controls"):
            with self.subTest(missing=missing):
                selected, observed, repeated = fixture()
                del observed["runtime_loaded"][missing]
                self.refused(selected, observed, repeated, "runtime_missing", missing)

    def test_arch_recipe_keeps_resolved_distribution_qt_external(self):
        selected, observed, repeated = fixture("desktop-arch-recipe")
        evidence = self.qualified(selected, observed, repeated)
        qt = next(row for row in evidence if row["subject"] == "component:qt")
        self.assertEqual(qt["facts"]["role"], "external")
        self.assertFalse(any(name.startswith("runtime/") for name in observed["members"]))

    def test_linux_observation_cannot_qualify_windows_profile(self):
        selected, _, _ = fixture("desktop-win-zip")
        _, observed, repeated = fixture("desktop-linux-archive")
        self.refused(selected, observed, repeated, "profile_mismatch", "desktop-win-zip")

    def test_matching_member_path_cannot_hide_replaced_bundled_bytes(self):
        selected, observed, repeated = fixture()
        path = "runtime/qt-core"
        observed["members"][path] = identity("different library")
        self.refused(selected, observed, repeated, "member_mismatch", path)

    def test_unrecorded_runtime_library_blocks_the_profile(self):
        selected, observed, repeated = fixture()
        observed["linked"]["unrecorded-decoder"] = "decoder"
        self.refused(selected, observed, repeated, "runtime_unrecorded", "unrecorded-decoder")

    def test_reused_build_observation_is_not_an_independent_repeat(self):
        selected, observed, _ = fixture()
        self.refused(selected, observed, deepcopy(observed), "rebuild_missing", selected["profile"])

    def test_clean_rebuild_must_match_output_bytes(self):
        selected, observed, repeated = fixture()
        repeated["output"] = identity("different build output")
        self.refused(selected, observed, repeated, "rebuild_mismatch", selected["profile"])

    def test_unavailable_upstream_license_stays_unresolved(self):
        selected, observed, repeated = fixture()
        for row in (selected, observed, repeated):
            row["inputs"]["parser"]["license"] = {"value": None, "source": None}
        evidence = self.qualified(selected, observed, repeated)
        parser = next(row for row in evidence if row["subject"] == "component:parser")
        self.assertEqual(parser["facts"]["license"], {"value": None, "source": None})


if __name__ == "__main__":
    unittest.main()
