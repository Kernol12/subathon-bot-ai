"""Opt-in RELEASE-001 Stage 1 cases, with expectations from contract r3."""

from copy import deepcopy
from hashlib import sha256
import json
from pathlib import Path
import sys
import unittest

from fixture import OLD_CMAKE, ROOT, WHEEL_PATH, blob, f0, identity, inventory_bytes, m6, m10, pin
from receipt_fixtures import (
    GTEST_NOTICE, PASSWORD_BINARY, SODIUM_NOTICE, SUMMARY, archive_missing, linked_password, native,
)

sys.path.insert(0, str(ROOT))
from tools.release_manifest import Blocked, Complete, Finding, assemble


class ReleaseManifestTests(unittest.TestCase):
    def assert_blocked(self, request, *findings, artifact="A"):
        self.assertEqual(assemble(request), Blocked(tuple(Finding(artifact, code, subject)
                                                         for code, subject in findings)))

    def assert_golden(self, request, case, length, digest):
        expected = Path(__file__).with_name(case + ".golden").read_bytes()
        self.assertEqual(identity(expected), {"bytes": str(length), "sha256": digest})
        parsed = json.loads(expected)
        inventory = inventory_bytes(parsed["members"])
        self.assertEqual(sha256(inventory).hexdigest(), parsed["artifact"]["inventory_sha256"])
        self.assertEqual(request["observed"]["artifact"], parsed["artifact"])
        self.assertCountEqual(request["observed"]["members"], parsed["members"])
        self.assertEqual(assemble(request), Complete(expected))

    def assert_m1(self, request):
        self.assert_golden(request, "m1", 1665,
                           "2e80301f066f7919f08486263daa4785c3cefce6967561d34da667428e686ebf")

    def test_m1_exact_source_manifest(self):
        self.assert_m1(f0())

    def test_m2_historical_pin_stops_before_rights_and_notices(self):
        request = f0()
        selected = next(record for record in request["evidence"] if record["id"] == "selected-CMakeLists.txt")
        selected["facts"] = deepcopy(OLD_CMAKE)
        request["components"][0]["rights"].clear()
        request["rights"].clear()
        request["notices"][0]["paths"].remove("LICENSE.md")
        self.assertEqual(request["observed"]["members"][0]["bytes"], "7374")
        self.assert_blocked(request, ("identity_conflict", "member:CMakeLists.txt"))

    def test_m3_developer_sdk_evidence_cannot_seal_archive(self):
        self.assert_blocked(archive_missing(), ("inventory_missing", "artifact"))

    def test_m4_selected_native_binding_missing_fifth_patch(self):
        self.assert_blocked(native(4, missing_patch=True), ("source_missing", "component:native-binding"))

    def test_m5_stale_notice_summary_cannot_erase_static_linkage(self):
        self.assert_blocked(linked_password(), ("notice_conflict", "component:sodium"))

    def test_m5_current_summary_preserves_the_embedded_membership(self):
        request = linked_password(current_summary=True)
        result = assemble(request)
        self.assertIsInstance(result, Complete)
        manifest = json.loads(result.manifest_bytes)
        self.assertEqual([row["path"] for row in manifest["members"]],
                         ["CMakeLists.txt", "COPYING", "LICENSE.md", SUMMARY,
                          PASSWORD_BINARY, GTEST_NOTICE, SODIUM_NOTICE])
        self.assertEqual(manifest["artifact"], request["observed"]["artifact"])
        self.assertEqual(sha256(inventory_bytes(manifest["members"])).hexdigest(),
                         manifest["artifact"]["inventory_sha256"])
        binary = next(row for row in manifest["members"] if row["path"] == PASSWORD_BINARY)
        self.assertEqual(binary["components"], ["gtest", "project", "sodium"])
        expected_roles = {"project": "carried", "sodium": "embedded", "gtest": "embedded"}
        for row in request["components"]:
            if row["role"] == "external":
                expected_roles[row["id"]] = "external"
        self.assertEqual({row["id"]: row["role"] for row in manifest["components"]}, expected_roles)
        for row in manifest["components"]:
            if row["role"] == "external":
                self.assertEqual(row["members"], [])

    def test_m6_font_metadata_cannot_replace_either_required_role(self):
        self.assert_blocked(m6(), ("font_unverified", "font:bold"), ("font_unverified", "font:regular"))

    def test_m7_resealed_missing_trantor_notice(self):
        self.assert_blocked(native(7, omit_trantor=True), ("notice_missing", "component:trantor"))

    def test_m7_old_whole_artifact_seal_stops_before_notice(self):
        self.assert_blocked(native(7, omit_trantor=True, stale=True), ("identity_conflict", "artifact"))

    def test_m8_reordered_member_and_path_sets_have_identical_bytes(self):
        self.assert_m1(self.reordered_f0())

    def test_m8_different_copying_pin_conflicts_with_actual_member(self):
        request = self.reordered_f0()
        selected = next(record for record in request["evidence"] if record["id"] == "selected-COPYING")
        selected["facts"]["sha256"] = identity(blob("LICENSE.md"))["sha256"]
        self.assert_blocked(request, ("identity_conflict", "member:COPYING"))

    def reordered_f0(self):
        request = f0()
        request["observed"]["members"].reverse()
        request["notices"][0]["paths"].reverse()
        request["components"][0]["members"].reverse()
        return request

    def test_m9_scope_rights_stop_before_notice_omission(self):
        self.assert_blocked(native(9, limited_rights=True),
                            ("redistribution_prohibited", "component:drogon"),
                            ("rights_unknown", "component:trantor"))

    def test_m10_acquisition_evidence_does_not_expand_membership(self):
        self.assert_golden(m10(), "m10", 2440,
                           "8927ec92e4f9c2103216f9bc977c72a84b81a636efcd808115695ef59b2ae79e")

    def test_m10_observed_wheel_with_acquisition_pin_is_unmapped(self):
        self.assert_blocked(m10(wheel=True), ("unmapped_content", f"member:{WHEEL_PATH}"))

    def test_stage1_duplicate_selected_role_beats_identity_comparison(self):
        request = f0()
        pin(request, "member:CMakeLists.txt", "source", "selected-historical-cmake", **OLD_CMAKE)
        self.assert_blocked(request, ("invalid_input", "member:CMakeLists.txt"))

    def test_stage2_compares_distinct_roles_and_sorts_all_qualified_subjects(self):
        request = f0()
        for row in request["evidence"]:
            if row["id"] == "selected-LICENSE.md":
                row["facts"]["sha256"] = identity(blob("COPYING"))["sha256"]
            elif row["id"] == "selected-CMakeLists.txt":
                row["facts"] = deepcopy(OLD_CMAKE)
        request["selected_pins"].reverse()
        self.assert_blocked(request, ("identity_conflict", "member:CMakeLists.txt"),
                            ("identity_conflict", "member:LICENSE.md"))


if __name__ == "__main__":
    unittest.main()
