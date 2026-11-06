# SCHEMATEST-001 final minimal harness and admission-source review

Reviewer `/root/session_supplier_review`, DIFFERENT from harness/configuration
author `/root` and test author `/root/session_supplier_author`,6 November2026.
**APPROVED exact harness/CI + THREE additive admission tests/helper ONLY.**
Initial isolated review tree from21c71eb is preserved. Final NEW isolated branch
`task/SCHEMATEST-001-final-harness-admission-review`, worktree
`/tmp/subathon-schematest001-final-harness-admission-review`, exact root candidate
`0aa6d8dadf0b95bc6f28a35109ad4d04dc9400a8`. Own ONLY this review note.

| Actual artifact | Exact Git blob |
| --- | --- |
| [Minimal harness](../../tools/schema_validation/check_baseline.py) | `97d95c29d1f7f33dfc06e65ecd7cc38e4d74dd61` |
| [Workflow](../../.github/workflows/schema-library-foundation.yml) | `842790e2bd87039c646a1589cd4982463461b778` |
| [Actual source](../../tests/schema_validation/wrapper_acceptance.py), authorfc5180bc/import395a0b1 | `a80b16cd745f054f9fceba7a1f33966229a8310e` |
| [Root author description](schematest-001-baseline-harness.md) | `a84a6d6885f00b6b69589f26df2d63d243ee24e5` |

Read root/tools/tests instructions, accepted logical/public/context/private-process
documents, installation document86d6d4 and metadata manifest6f4ffec with DIFFERENT
actual approval52229d7. Prior source reviews A ff9476a/C525b0c4 remain valid for
sourceebb. C's actual525d31d separately approves the changed dialect method at
sourcea80 and preservation of the prior middle32. This review does NOT self-certify
SDK14 source or replace C's dialect/schema/oracle verdict. It supplies the other
three source-delta cases, new helper and actual root harness/config verdict.

## Core harness and final inventory delta

Full231-line original core21c71eb was reviewed. Final0aa adds ONLY three literal
EXPECTED_METHODS names; all other harness logic is identical. Static inventory
matches all63 actual source methods exactly, uniquely and in sorted order. No
skip/decorator/inversion/dynamic registration or custom TestCase constructor/run/
id/call/load_tests is admitted. Both raw interface ASTs remain sole immediate
named-marker raises. Runtime checks construct only the approved18 enum members and
zero-argument RuntimeError marker with the exact approved message.

Definition loading uses trusted local source under names other than __main__.
Actual wrapper4a124 and sourcea80 module definitions are inert: no fixture creation,
SDK discovery, raw feature call, setup, test body or subprocess occurs on import.
Actual unittest loading constructs standard TestCase instances and nested suites;
it does NOT invoke setUpClass/setUp/run/tests here. The inherited constructors are
ordinary stdlib implementations, not user-supplied callbacks in this actual source.
The harness never invokes suite.run or either raw feature. Registered case names,
class identity and skip/expected-failure attributes are checked again dynamically.
Known SDK root modules are rejected from sys.modules, and no SDK directory is added
to sys.path. This is an audit of these exact trusted definitions, not permission for
arbitrary future top-level code merely because a registration count happens to fit.

Selected primary [3.10.12 loader](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/unittest/loader.py)
and [3.12.3 loader](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/unittest/loader.py)
confirm the ordinary loading/constructor path and separate load_tests hook. That
hook is absent in actual source and explicitly refused by the harness. No definition
loader, constructor, harness entrypoint or unittest discovery was run by this review.

## Actual no-site SDK observation and CI scope

Before importing/accessing sysconfig, the core rejects PRESENCE of all three named
override keys. It checks actual CPython/Linux/x86-64/minor/SOABI and bool-normalized
debug/GIL flags. Older missing/None flags are false. It preserves executable spelling,
finds the bounded selected-venv config and explicitly projects posix_prefix with
base/platbase from that executable's bin parent, requiring one existing SDK identity.
It does not infer the child SDK from pre3.14 -S prefixes or append/import that SDK.
This is an environmental observation, not the production parent's prefix admission
or a worker RLIMIT/secure-open implementation.

Selected [3.10.12 sysconfig](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/sysconfig.py)
and [3.12.3 sysconfig](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/sysconfig.py)
support the exact get_paths(scheme,vars) API and purelib/platlib substitutions.
Actual distributor layout/no-site projection still must be observed after review;
source feasibility is not a claimed native3.10/3.12 harness result.

For each of20 trusted manifest distributions, target selection and fixed dist-info
identity precede bounded4096/+1 WHEEL and262144/+1 METADATA reads. BOTH raw hashes
must equal fields of the SAME target artifact; combining hashes from two artifacts
cannot pass. Static manifest inspection confirms one selected artifact/package/
target for all three admitted targets. Existing different archive/hash/license/data
review remains authoritative; this harness does not authenticate all installed code
against a hostile administrator or implement production changed-read/nonregular
protection. No candidate library is imported to discover a version.

Workflow delta is exactly ONE unconditional step AFTER the existing14 SDK body.
Every old workflow byte, pin/install/setup/minimal environment and all14 source
methods remain unchanged. The added selected-venv command uses env-i/minimal PATH/
locale and -I -S -B, so it observes metadata/definition registration only. No path
filter, optional success, failure inversion or ignored step is added. Ordinary green
CI still does not execute the intentional-red tests or advertise product behavior.

## Three actual additive tests and real-script helper

First24/last4 and all old helpers are EXACT AST matches to their prior approved
sourceebb;59 old test ASTs match, with only C's separately reviewed dialect change.
Only errno is newly imported at module scope. The new helper/three methods do not
change import-time safety or add a validating fake/public injection path.

- Parent ENOSYS uses ONLY the approved module os.pipe fault, verifies it was reached,
  forbids any Popen attempt, and expects environment_unavailable/no public output.
  This proves only that injected parent failure, not unavailable real host/child
  facilities or resource exhaustion. Its raw marker remains first under inert code.
- Each override key has empty/nonempty cases, with other override keys removed for
  isolation; exact caller environment is compared before/after and patch.dict restores
  it on exceptions. No worker may start. The test does not pretend this outcome alone
  proves every before-import/config-access ordering detail; source audit remains.
- Clone test FIRST calls unmodified raw validate_package on a valid literal package.
  ONLY afterward it imports stdlib clone/config tools, discovers actual single SDK
  root and creates a fresh with_pip=False/no-system-site venv. Copying that actual
  trusted SDK does not relocate a whole venv or download/install dependencies. A REAL
  fresh public script must first pass untouched, then exact code+dist-info deletion
  must give dependency_unavailable/exit3, rather than an early environment refusal.
  Separate child invocations prevent a prior successful process/module cache from
  supplying the negative result. No SDK/clone work can manufacture FIRST marker red.

The helper launches the actual absolute script/interpreter with isolated startup,
fixed minimal environment, shell=False/closed inherited descriptors/DEVNULL input.
Temporary capture files close on every path; reads are capped257bytes, fixed exact
line/exit and empty stderr required. Immediate script waits are bounded15seconds
and failed waits kill/reap the owned child. The copied tree is disposable and comes
from the already admitted trusted installation; this is not arbitrary package input
or an assertion that copytree supplies the worker's memory/file bounds.
[3.10.12 venv source](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/venv/__init__.py)
supports the no-pip/fresh POSIX layout recipe. Actual cloning/scripts have NOT run.

## Actual checks and allowed next observations

No blocking actual source/config finding. Root's exact0aa author note already
preserves the initial60 stage AND appends actual63/a80 attribution; my initial
staleness concern from reading only21 was resolved by reading that exact append.
The whole315 raw-observation count is author inventory, not a discovery/runtime
result independently certified by this harness review.

Actual static checks: Python3.10-feature AST parse/compile WITHOUT exec/import for
harness/tests; exact63 sorted names,18 enum literals,59 old test/all-helper preservation;
five extracted workflow shell blocks pass bash-n; old14/setup prefix byte-identical;
target/metadata pairing and unchanged pin/source comparisons PASS.
`python3 tools/check_project.py` PASS426/122; `python3 tools/progress.py render --check`
PASS; `git diff --check` PASS. No harness/definition/candidate/SDK/test/helper/feature
import or execution, child/venv creation, package install or GitHub operation occurred.

With complementary actual source approvals retained, root may now observe this EXACT
minimal harness in the admitted environments. Its native/hosted results remain new
evidence. Real selected-environment admission must pass before the reviewed complete
FIRST intended wrapper-red; dependency/setup/skip failures cannot supply it. That red,
full baseline integration, later implementation/source/acceptance/ordinary CI and all
product/runtime/platform/live/release gates remain. No production validator is released.
