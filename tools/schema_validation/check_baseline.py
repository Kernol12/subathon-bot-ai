#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify SDK identities and reviewed acceptance registration without running cases."""
import ast
import hashlib
import json
import os
import importlib.util
from pathlib import Path
import sys
import unittest


# Registration inventory for actual test-source review; these are names, never
# expected validation results derived from production behavior.
EXPECTED_METHODS = (
    'test_all_289_unique_files_are_admitted',
    'test_all_documents_finish_dialect_admission_before_native_checks',
    'test_api_path_encoding_and_shape',
    'test_api_path_types_never_convert',
    'test_complete_unused_reference_closure_and_wrong_target_role',
    'test_context_phase_priority_across_independent_defects',
    'test_context_resource_and_reference_edge_boundaries',
    'test_cross_document_scope_nested_ids_anchors_and_ref_siblings',
    'test_dialect_admission_is_explicit_and_precedes_format',
    'test_document_and_binding_count_limits_precede_missing_reads',
    'test_duplicate_declarations_bindings_and_file_aliases',
    'test_duplicate_resource_ids_anchors_and_outside_ids',
    'test_exact_fraction_multipleof_and_neighbor',
    'test_exact_numeric_integer_boolean_and_keyword_semantics',
    'test_external_example_shared_across_actual_schemas',
    'test_extra_misdirected_and_resolved_alias_inventory',
    'test_first_failure_json_preflight_and_complete_file_order',
    'test_free_component_example_requires_explicit_actual_schema_binding',
    'test_fresh_calls_do_not_cache_prior_success',
    'test_graph_keys_include_encoded_controls_and_complete_owned_spellings',
    'test_help_static_without_worker_or_package_io',
    'test_inventory_null_example_and_no_associated_schema',
    'test_json_depth_values_and_raw_string_boundaries',
    'test_lexical_numeric_bounds_before_exact_construction',
    'test_literal_data_ref_id_format_and_reserved_property_names',
    'test_main_closed_argv_never_converts',
    'test_main_closed_codes_and_real_script_redaction',
    'test_main_tuple_none_and_option_looking_path',
    'test_manifest_and_individual_file_byte_boundaries',
    'test_manifest_closed_shape_and_types',
    'test_manifest_path_grammar_and_exact_256_boundary',
    'test_manifest_pointer_and_inline_source_shape',
    'test_manifest_pointer_escaping_percent_hash_and_unicode',
    'test_minimal_draft_package_and_openapi_patches',
    'test_missing_manifest_and_declared_member',
    'test_native_applicators_booleans_unevaluated_and_property_names',
    'test_native_invalid_schema_in_unused_children_and_ref_siblings',
    'test_nonregular_files_are_refused_without_blocking',
    'test_oas_closed_roles_include_unused_components_trace_and_content_forms',
    'test_oas_default_literal_ref_preserved_and_raw_sibling_checked',
    'test_oas_schema_scope_with_and_without_root_id',
    'test_package_root_is_manifest_parent_not_cwd',
    'test_parameter_header_encoding_and_schema_annotation_inventory',
    'test_parent_pipe_enosys_refuses_without_worker_or_logs',
    'test_productive_recursive_schema_and_inventory_only_edges',
    'test_profile_features_are_context_specific',
    'test_real_sdk_clone_control_then_missing_distribution_is_dependency_unavailable',
    'test_reference_example_physical_identity_complete_forced_schema_pairs',
    'test_required_inventory_growth_refuses_instead_of_truncating',
    'test_same_instance_cycles_even_unused_or_conditional',
    'test_schema_example_inventory_missing_then_invalid_instance',
    'test_schema_format_only_not_literal_or_metadata',
    'test_standalone_inline_examples_are_complete_literal_data',
    'test_strict_json_rejects_encoding_duplicates_and_grammar',
    'test_symlink_manifest_parent_and_members',
    'test_sysconfig_environment_presence_including_empty_refuses_unchanged',
    'test_total_raw_file_byte_boundary_includes_manifest',
    'test_unicode_pointer_identity_is_not_normalized',
    'test_uri_reference_fragments_are_not_reencoded_as_raw_pointers',
    'test_worker_activation_missing_extra_and_malformed_refuse_forged_ok',
    'test_worker_outcome_bounded_exact_closed_protocol',
    'test_worker_setup_refusal_before_activation',
    'test_worker_startup_deadline_kills_and_reaps',
    'test_worker_status_mismatch_and_unclassified_signal',
)
EXPECTED_OUTCOMES = (
    "ok", "usage_error", "invalid_path", "io_unavailable",
    "environment_unavailable", "resource_limit", "invalid_json",
    "invalid_manifest", "unsupported_dialect", "unsupported_format",
    "dependency_unavailable", "invalid_schema", "unresolved_reference",
    "reference_cycle", "invalid_inventory", "unsupported_feature",
    "invalid_instance", "validation_unavailable",
)
MARKER = "Schema package validation is intentionally unimplemented; review tests first"


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def load_definitions(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    require(spec is not None and spec.loader is not None, "Missing baseline module loader")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def cases(suite):
    for child in suite:
        if isinstance(child, unittest.TestSuite):
            yield from cases(child)
        else:
            yield child


def check_sdk_metadata(root):
    overrides = {"_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH",
                 "_PYTHON_PROJECT_BASE"}
    require(not overrides.intersection(os.environ), "Uncontrolled sysconfig override")
    import sysconfig

    minor = sys.version_info[:2]
    require(sys.implementation.name == "cpython" and minor in {(3, 10), (3, 12), (3, 14)}
            and sys.platform == "linux" and os.uname().machine == "x86_64",
            "This SDK foundation requires its admitted Linux CPython target")
    target = f"cp{minor[0]}{minor[1]}"
    require(sysconfig.get_config_var("SOABI") == f"cpython-{minor[0]}{minor[1]}-x86_64-linux-gnu"
            and not bool(sysconfig.get_config_var("Py_DEBUG"))
            and not bool(sysconfig.get_config_var("Py_GIL_DISABLED")),
            "Actual interpreter ABI differs from the admitted target")
    # Observe the CHILD no-site projection, not pre3.14's unreliable sys.prefix.
    executable = Path(os.path.abspath(sys.executable))
    venv_root = executable.parent.parent
    config = venv_root / "pyvenv.cfg"
    require(config.is_file() and config.stat().st_size <= 4096,
            "Missing bounded selected-venv configuration")
    paths = sysconfig.get_paths(scheme="posix_prefix",
                               vars={"base": str(venv_root), "platbase": str(venv_root)})
    sdk = Path(paths["purelib"]).resolve(strict=True)
    require(sdk == Path(paths["platlib"]).resolve(strict=True),
            "Selected SDK does not have one installation root")
    packages = json.loads((root / "tools/schema_validation/wheels.json").read_text())["packages"]
    require(len(packages) == 20 and len({p["name"] for p in packages}) == 20,
            "Selected twenty-package metadata inventory differs")
    for package in packages:
        artifacts = [a for a in package["artifacts"] if target in a["candidate_targets"]]
        require(bool(artifacts), "No admitted artifact for actual interpreter")
        directories = {a["dist_info_directory"] for a in artifacts}
        require(len(directories) == 1, "Ambiguous distribution metadata identity")
        directory = sdk / next(iter(directories))
        observed = []
        for filename, ceiling in (("WHEEL", 4096), ("METADATA", 262144)):
            with (directory / filename).open("rb") as stream:
                payload = stream.read(ceiling + 1)
            require(len(payload) <= ceiling, "Installed metadata exceeds admitted ceiling")
            observed.append(hashlib.sha256(payload).hexdigest())
        require(any(observed == [a["wheel_metadata_sha256"], a["distribution_metadata_sha256"]]
                    for a in artifacts), "Installed metadata does not match one admitted artifact")
    print(f"Verified actual {target} no-site SDK projection and twenty metadata identities")


def main():
    root = Path(__file__).resolve().parents[2]
    check_sdk_metadata(root)
    wrapper = root / "tools" / "schema_validate.py"
    acceptance = root / "tests" / "schema_validation" / "wrapper_acceptance.py"
    wrapper_tree = ast.parse(wrapper.read_text(), feature_version=(3, 10))
    acceptance_tree = ast.parse(acceptance.read_text(), feature_version=(3, 10))
    public_calls = {node.name: node for node in wrapper_tree.body
                    if isinstance(node, ast.FunctionDef) and not node.name.startswith("_")}
    require(set(public_calls) == {"validate_package", "main"}, "Public interface inventory differs")
    require(not any(node.decorator_list for node in public_calls.values()),
            "Public interface cannot replace the reviewed raw calls with decorators")

    classes = [node for node in acceptance_tree.body
               if isinstance(node, ast.ClassDef) and node.name == "WrapperAcceptance"]
    require(len(classes) == 1 and not classes[0].decorator_list,
            "Acceptance class inventory or decoration differs")
    methods = [node for node in classes[0].body
               if isinstance(node, ast.FunctionDef) and node.name.startswith("test_")]
    require(len(methods) == len(EXPECTED_METHODS)
            and {node.name for node in methods} == set(EXPECTED_METHODS)
            and not any(node.decorator_list for node in methods),
            "Acceptance registration differs or a case is decorated")
    require(not any(isinstance(node, ast.FunctionDef) and node.name == "load_tests"
                    for node in acceptance_tree.body), "Custom test discovery is not admitted")
    require(not any(isinstance(node, ast.FunctionDef)
                    and node.name in {"__init__", "__call__", "id", "run"}
                    for node in classes[0].body), "Custom acceptance lifecycle is not admitted")

    # This step loads definitions only. Feature behavior is checked by the separate
    # unconditional suite; setup, cases and suite.run are never called here.
    api = load_definitions(wrapper, "schema_wrapper_baseline_declarations")
    require(tuple(api.SchemaValidationOutcome.__members__) == EXPECTED_OUTCOMES,
            "Outcome names/order differ")
    require(tuple(member.value for member in api.SchemaValidationOutcome) == EXPECTED_OUTCOMES,
            "Outcome values differ")
    error = api.SchemaValidationNotImplemented()
    require(type(error).__bases__ == (RuntimeError,) and error.args == (MARKER,),
            "Named marker declaration differs")
    module = load_definitions(acceptance, "schema_wrapper_acceptance_definitions")
    suite = unittest.defaultTestLoader.loadTestsFromModule(module)
    registered = list(cases(suite))
    require(len(registered) == len(EXPECTED_METHODS), "Loaded acceptance count differs")
    require(sorted(case._testMethodName for case in registered) == list(EXPECTED_METHODS),
            "Loaded acceptance names differ")
    for case in registered:
        require(type(case).__name__ == "WrapperAcceptance", "Unexpected acceptance case class")
        method = getattr(type(case), case._testMethodName)
        require(not getattr(type(case), "__unittest_skip__", False)
                and not getattr(method, "__unittest_skip__", False)
                and not getattr(method, "__unittest_expecting_failure__", False),
                "Skipped or failure-inverted acceptance is not admitted")
    candidate_roots = {
        "annotated_types", "attr", "attrs", "jsonschema", "jsonschema_path",
        "jsonschema_specifications", "lazy_object_proxy", "openapi_schema_validator",
        "openapi_spec_validator", "pathable", "pydantic", "pydantic_core",
        "pydantic_settings", "dotenv", "yaml", "referencing", "rfc3339_validator",
        "rpds", "six", "typing_extensions", "typing_inspection",
    }
    require(not candidate_roots.intersection(name.split(".")[0] for name in sys.modules),
            "Definition-only harness imported a candidate SDK")
    print(f"Registered {len(registered)} reviewed wrapper acceptance methods; cases run in the separate suite")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
