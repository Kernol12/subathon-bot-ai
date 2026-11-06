# Read-only schema-validator library preparation — 6 November 2026

Investigator canonical session_supplier_review; no versions selected/installed,
no validation execution, code, tests, pins or capability proof. Actual selection,
full transitive lock/hashes/licenses/compatibility and offline foundations require
SCHEMATEST-001's own reviewed contract stage after plan005 integrates.

Primary verified candidates: openapi-spec-validator0.9.0 (20May2026,Python>=3.10<4,
Apache2), jsonschema4.26.0 (7Jan2026,>=3.10,MIT), openapi-schema-validator0.9.0
(BSD3), jsonschema-path0.5.0 (19May2026,Apache2), referencing0.37.0 (MIT).
OSV transitives include jsonschema>=4.26<5, schema-validator>=0.9<0.10,
schema-path>=0.5<0.6, lazy-object-proxy, Pydantic/settings; other dependencies
attrs/specifications/referencing/rpds etc still need complete actual lock review.

Sources:
https://pypi.org/project/openapi-spec-validator/
https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/pyproject.toml
https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/pyproject.toml
https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/validators.py
https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/keywords.py
https://raw.githubusercontent.com/p1c2u/jsonschema-path/0.5.0/jsonschema_path/handlers/__init__.py
https://raw.githubusercontent.com/python-openapi/openapi-schema-validator/0.9.0/openapi_schema_validator/shortcuts.py
https://python-jsonschema.readthedocs.io/en/stable/referencing/
https://python-jsonschema.readthedocs.io/en/stable/validate/#validating-formats

Source-supported traps: explicitly use OpenAPIV31SpecValidator/OAS31Validator,
not defaultlatest3.2 aliases; spec validation checks schemas but not all example
instances. Explicitly enumerate/validate admitted examples. SchemaPath default
handlers can fetchhttp/https/file and allURLs, so bounded admitted handlers required.
referencing.Registry can contain explicit local resources with denied retrieval.
Unknown formats pass default FormatChecker; require closed explicit format policy.
Complete ref closure must be schema/context aware (not interpret literal example/
const data $ref as schema references). No custom OpenAPI/JSONSchema engine.

No actual dependency foundation/CLI contract/fixture oracle/harness/test review/red
exists yet. Do not treat read-only source findings as actual offline library proof.
