# Honest starting state

This is a packaged development foundation. Product server/desktop/browser applications,
all integrations, real credentials, production SQL, exact accounting engine, scene
editors/renderers, deployment/backup code, localization assets and release binaries are
not implemented. Empty module scopes are intentional assignments, not fake integrations.

Initial time-credit API and six GoogleTest examples are DRAFT. No separate test review,
observed red GoogleTest run, CMake/CTest run or Qt probe run has happened in preparation.
Direct GCC compilation of the contract/stub/scaffold and developer-tool checks are recorded
in docs/evidence/baseline-validation.md. They do not prove the acceptance tests compile.

Subsequent local scaffold and Qt probe runs passed; see the
[initial publication review](evidence/initial-publication.md). The acceptance review
and observed red baseline remain open.

The first CLion agent validates this environment and prepares tests, then hands off to a
separate reviewer and later implementer. The full specification's remaining engineering
choices and external capability/packaging/performance gates remain in force.

Progress/state checks verify record consistency, not whether a claimed test result or
review is honest. Actual source/evidence/review remains necessary. The progress page is
static and updates only when regenerated and reloaded. No automatic browser write-back.
