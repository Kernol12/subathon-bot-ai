"""PACKAGE-001 synthetic examples. No native toolchain or runtime is invoked."""

from copy import deepcopy
import sys
import unittest

from fixture import ROOT, identity as bytes_identity
from native_build_evidence_candidate import (
    InputDifference, Qualified, Unqualified, qualify_native_build,
)

sys.path.insert(0, str(ROOT))
from tools.release_manifest import Blocked, assemble
from tools.release_manifest_shape import shape


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
        "revision": "0c0447d3ff21496e2465fe203f7d2b621fe382c1",
        "project_material": identity("selected source tree"),
        "scope": "synthetic-release-r1/" + profile,
        "platform": platform,
        "recipe": "native-release",
        "recipe_material": identity("selected release recipe content"),
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
    archive = "parser.lib" if windows else "libparser.a"
    selected["inputs"]["parser"]["archives"] = {archive: identity("selected parser archive")}
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
        "command": ("cmake", "--build", "first-release-build"),
        "exit_status": 0,
        "revision": selected["revision"],
        "project_material": deepcopy(selected["project_material"]),
        "scope": selected["scope"],
        "platform": platform,
        "recipe": selected["recipe"],
        "recipe_material": deepcopy(selected["recipe_material"]),
        "presets": selected["presets"],
        "environment": deepcopy(selected["environment"]),
        "inputs": deepcopy(selected["inputs"]),
        "linked": linked,
        "runtime_loaded": runtime_loaded,
        "members": deepcopy(files),
        "output": identity("reproducible synthetic artifact"),
    }
    observed["inputs"]["parser"]["archives"] = {archive: identity("selected parser archive")}
    repeated = deepcopy(observed)
    repeated["run"] = "second-clean-build"
    repeated["command"] = ("cmake", "--build", "second-release-build")
    return selected, observed, repeated


class PackageNativeEvidenceTest(unittest.TestCase):
    def qualified(self, selected, observed, repeated):
        result = qualify_native_build(selected, observed, repeated)
        self.assertIsInstance(result, Qualified)
        return result

    def refused(self, selected, observed, repeated, code, subject):
        result = qualify_native_build(selected, observed, repeated)
        self.assertIsInstance(result, Unqualified)
        self.assertIn((code, subject), result.findings)
        return result

    def test_matching_linux_profile_emits_owned_build_evidence(self):
        for profile in ("server-native", "desktop-linux-archive"):
            with self.subTest(profile=profile):
                selected, observed, repeated = fixture(profile)
                result = self.qualified(selected, observed, repeated)
                inputs = result.release_inputs
                indexes, invalid = shape(inputs)
                self.assertEqual(invalid, set())
                self.assertEqual(inputs["selection"]["profile"], profile)
                self.assertEqual(inputs["selection"]["revision"], selected["revision"])
                self.assertEqual(inputs["selection"]["scope"], selected["scope"])
                evidence = indexes["evidence"]
                build = next(row for row in evidence.values() if row["kind"] == "observed-build")
                self.assertEqual(build["facts"]["output"], observed["output"])
                recipe = evidence[build["facts"]["recipe"]]
                self.assertEqual(recipe["kind"], "observed-build-recipe")
                self.assertEqual(build["scope"], selected["scope"])
                self.assertEqual(recipe["scope"], selected["scope"])
                self.assertEqual(build["revision"], selected["revision"])
                for row in (build, recipe):
                    self.assertTrue(row["owner"])
                self.assertEqual(recipe["facts"]["recipe_material"], selected["recipe_material"])
                self.assertEqual(recipe["facts"]["project_material"], selected["project_material"])

                components = indexes["components"]
                for name, selected_input in selected["inputs"].items():
                    self.assertEqual(components[name]["role"], selected_input["role"])
                    self.assertEqual(components[name]["revision"], selected_input["revision"])
                    pins = [row for (subject, _), row in indexes["selected_pins"].items()
                            if subject == "component:" + name]
                    self.assertTrue(pins, name)
                    for pin in pins:
                        row = evidence[pin["evidence"]]
                        self.assertEqual(row["kind"], "selected-identity")
                        self.assertEqual(row["subject"], "component:" + name)
                        self.assertEqual(row["scope"], selected["scope"])
                        self.assertEqual(row["revision"], selected_input["revision"])
                        self.assertEqual(
                            {key: row["facts"][key] for key in ("bytes", "sha256")},
                            selected_input["material"],
                        )

                for path, expected in observed["members"].items():
                    self.assertEqual(
                        {key: indexes["members"][path][key] for key in ("bytes", "sha256")},
                        expected,
                    )
                    expected_owners = {"project", "parser"} if path.startswith("bin/") else {"qt"}
                    self.assertEqual(set(indexes["members"][path]["components"]), expected_owners)
                    pin = indexes["selected_pins"]["member:" + path, "acquisition"]
                    acquisition = evidence[pin["evidence"]]
                    self.assertEqual(acquisition["kind"], "acquisition")
                    self.assertEqual(acquisition["subject"], "member:" + path)
                    self.assertEqual(acquisition["scope"], selected["scope"])
                    self.assertEqual(
                        {key: acquisition["facts"][key] for key in ("bytes", "sha256")},
                        expected,
                    )
                notice = indexes["members"]["notices/Qt.txt"]
                self.assertEqual(notice["kind"], "notice")
                self.assertIn("qt", notice["components"])
                self.assertEqual(inputs["rights"], [])
                self.assertFalse(any(row["kind"] in {"rights-review", "source-review"}
                                     for row in evidence.values()))

                for documented, actual in zip(result.build_runs, (observed, repeated), strict=True):
                    for key in ("run", "command", "exit_status", "revision", "environment",
                                "inputs", "output", "project_material", "recipe_material"):
                        self.assertEqual(documented[key], actual[key])

    def test_exact_technical_evidence_reaches_the_separate_assembler_rights_gate(self):
        selected, observed, repeated = fixture()
        result = self.qualified(selected, observed, repeated)
        inputs = result.release_inputs
        indexes, invalid = shape(inputs)
        self.assertEqual(invalid, set())
        evidence = indexes["evidence"]
        pin = indexes["selected_pins"]["component:parser", "build"]
        self.assertEqual(
            evidence[pin["evidence"]]["facts"]["archives"],
            selected["inputs"]["parser"]["archives"],
        )
        parser = indexes["components"]["parser"]
        self.assertEqual(
            evidence[parser["build"]]["facts"]["static"]["parser"],
            observed["inputs"]["parser"]["archives"],
        )
        assembled = assemble(inputs)
        self.assertIsInstance(assembled, Blocked)
        self.assertTrue(assembled.findings)
        self.assertEqual({finding.code for finding in assembled.findings}, {"rights_unknown"})

    def test_missing_embedded_archive_identity_refuses(self):
        selected, observed, repeated = fixture()
        for row in (selected, observed, repeated):
            del row["inputs"]["parser"]["archives"]
        self.refused(selected, observed, repeated, "input_pin", "parser")

    def test_substituted_observed_archive_identity_refuses(self):
        selected, observed, repeated = fixture()
        observed["inputs"]["parser"]["archives"]["libparser.a"] = identity("different parser archive")
        self.refused(selected, observed, repeated, "selected_input_mismatch", "parser")

    def test_observed_source_or_recipe_material_must_match_selection(self):
        for field in ("project_material", "recipe_material"):
            with self.subTest(field=field):
                selected, observed, repeated = fixture()
                observed[field] = identity("different material")
                self.refused(selected, observed, repeated, "selected_build_mismatch", field)

    def test_mutable_revision_or_unbound_recipe_label_refuses(self):
        with self.subTest(field="revision"):
            selected, observed, repeated = fixture()
            for row in (selected, observed, repeated):
                row["revision"] = "main"
            self.refused(selected, observed, repeated, "project_pin", "revision")
        with self.subTest(field="recipe"):
            selected, observed, repeated = fixture()
            for row in (selected, observed, repeated):
                row["recipe"] = "latest"
                del row["recipe_material"]
            self.refused(selected, observed, repeated, "recipe_pin", "recipe")

    def test_unknown_source_locator_cannot_qualify(self):
        selected, observed, repeated = fixture()
        for row in (selected, observed, repeated):
            row["inputs"]["compiler"]["source"] = "unknown"
        self.refused(selected, observed, repeated, "input_pin", "compiler")

    def test_explicit_runtime_list_cannot_hide_required_qml_smoke_evidence(self):
        selected, observed, repeated = fixture()
        selected["dynamic_runtime"] = ("qt-platform",)
        for row in (observed, repeated):
            row["dynamic_runtime"] = selected["dynamic_runtime"]
            row["linked"]["qt-qml-controls"] = row["runtime_loaded"].pop("qt-qml-controls")
        self.refused(selected, observed, repeated, "runtime_missing", "qt-qml-controls")

    def test_missing_or_pre_minimum_qt_patch_refuses(self):
        for version in (None, "6.12", "6.11.9"):
            with self.subTest(version=version):
                selected, observed, repeated = fixture()
                selected["release_qt"] = version
                self.refused(selected, observed, repeated, "qt_release_pin", "qt")

    def test_matching_qt_pins_below_the_release_minimum_refuse(self):
        for version in ("6.11.9", "6.9.9"):
            with self.subTest(version=version):
                selected, observed, repeated = fixture()
                selected["release_qt"] = version
                for row in (selected, observed, repeated):
                    row["inputs"]["qt"]["revision"] = version
                    row["inputs"]["qt"]["material"] = identity("Qt " + version)
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

    def test_post_processed_zip_identity_stays_separate_from_reproducible_members(self):
        selected, observed, repeated = fixture("desktop-win-zip")
        final_zip = identity("separately signed Windows ZIP with changed executable bytes")
        selected["final_artifact"] = final_zip
        observed["final_artifact"] = deepcopy(final_zip)

        result = self.qualified(selected, observed, repeated)
        inputs = result.release_inputs
        for artifact in (inputs["expected"], inputs["observed"]["artifact"]):
            self.assertEqual({key: artifact[key] for key in observed["output"]}, observed["output"])
        for member in inputs["observed"]["members"]:
            expected = observed["members"][member["path"]]
            self.assertEqual({key: member[key] for key in expected}, expected)
        self.assertEqual(result.build_runs[0]["final_artifact"], final_zip)

    def test_selected_input_identity_cannot_be_replaced_by_observed_material(self):
        for name, field, value in (
            ("compiler", "revision", "different compiler"),
            ("qt", "material", identity("another build of Qt")),
            ("parser", "patches", {"portability.patch": identity("another patch")}),
        ):
            with self.subTest(name=name, field=field):
                selected, observed, repeated = fixture()
                observed["inputs"][name][field] = value
                result = self.refused(selected, observed, repeated, "selected_input_mismatch", name)
                self.assertIn(
                    InputDifference(name, field, selected["inputs"][name][field], value),
                    result.differences,
                )

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
                observed["linked"][missing] = observed["runtime_loaded"].pop(missing)
                self.refused(selected, observed, repeated, "runtime_missing", missing)

    def test_missing_transitive_qt_runtime_dependency_blocks_profile(self):
        selected, observed, repeated = fixture()
        selected["runtime"]["qt-helper"] = "qt"
        repeated["linked"]["qt-helper"] = "qt"
        self.refused(selected, observed, repeated, "runtime_missing", "qt-helper")

    def test_known_runtime_name_resolving_to_substitute_blocks_profile(self):
        selected, observed, repeated = fixture()
        observed["runtime_loaded"]["qt-platform"] = "parser"
        self.refused(selected, observed, repeated, "runtime_mismatch", "qt-platform")

    def test_arch_recipe_keeps_resolved_distribution_qt_external(self):
        selected, observed, repeated = fixture("desktop-arch-recipe")
        result = self.qualified(selected, observed, repeated)
        indexes, invalid = shape(result.release_inputs)
        self.assertEqual(invalid, set())
        self.assertEqual(indexes["components"]["qt"]["role"], "external")
        self.assertIn(("component:qt", "external"), indexes["selected_pins"])
        self.assertFalse(any(path.startswith("runtime/") for path in indexes["members"]))

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

    def test_equal_output_cannot_hide_changed_rebuild_inputs(self):
        for field in ("revision", "environment", "inputs"):
            with self.subTest(field=field):
                selected, observed, repeated = fixture()
                if field == "revision":
                    repeated[field] = "different source revision"
                elif field == "environment":
                    repeated[field]["material"] = identity("different build environment")
                else:
                    repeated[field]["compiler"]["material"] = identity("different compiler")
                self.assertEqual(observed["output"], repeated["output"])
                self.refused(selected, observed, repeated, "rebuild_input_mismatch", field)

    def test_unavailable_upstream_license_stays_unresolved(self):
        selected, observed, repeated = fixture()
        for row in (selected, observed, repeated):
            row["inputs"]["parser"]["license"] = {"value": None, "source": None}
        result = self.qualified(selected, observed, repeated)
        evidence = result.release_inputs["evidence"]
        licenses = {
            row["subject"]: row["facts"]["license"]
            for row in evidence if "license" in row["facts"]
        }
        self.assertEqual(licenses["component:parser"], {"value": None, "source": None})
        self.assertEqual(licenses["component:qt"], selected["inputs"]["qt"]["license"])


if __name__ == "__main__":
    unittest.main()
