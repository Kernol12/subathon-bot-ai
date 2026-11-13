# SPDX-License-Identifier: GPL-3.0-or-later
add_test(NAME schema.repository
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -S -B "${CMAKE_CURRENT_SOURCE_DIR}/test_project_schemas.py")
set_tests_properties(schema.repository PROPERTIES
    LABELS "schema-validation;acceptance;tooling" TIMEOUT 120)

add_test(NAME schema.publication
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -S -B "${CMAKE_CURRENT_SOURCE_DIR}/test_publication_integration.py")
set_tests_properties(schema.publication PROPERTIES
    LABELS "schema-validation;acceptance;tooling" TIMEOUT 120)

add_test(NAME schema.proofs
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -S -B "${CMAKE_CURRENT_SOURCE_DIR}/test_proof_schemas.py")
set_tests_properties(schema.proofs PROPERTIES
    LABELS "schema-validation;acceptance;tooling" TIMEOUT 120)

add_test(NAME schema.proof_publication
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -S -B "${CMAKE_CURRENT_SOURCE_DIR}/test_proof_publication_integration.py")
set_tests_properties(schema.proof_publication PROPERTIES
    LABELS "schema-validation;acceptance;tooling" TIMEOUT 120)
