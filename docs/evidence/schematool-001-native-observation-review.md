# SCHEMATOOL-001 native final observation — independent delta review

Reviewer C (`session_plan_review`), 6 November 2026, existing PR77 only. I reviewed
root's actual `34b4618f5c2b44a85fe3371089f914912bffcff7` on a fresh isolated
branch. Root authored this clarification after B found the instrumentation
conflict. I did not author its normative/type changes. Only this review note is
owned here.

**APPROVED: this actual private document/type clarification only.** It preserves
the already-reviewed actual native18 observation and does not release a helper,
owner implementation, unreviewed test body, compilation/execution, host capability
or native behavior red. Dependent helper/negative SIGINT test source still needs
different actual review before execution, and the established foundation/test/red
sequence remains mandatory.

Reviewed identities:

| Path | Git blob |
| --- | --- |
| `docs/contracts/schema-validation-native-process-v1.md` revision2 | `34bf0137c7b59d065c420cf74cb40b718e1977a3` |
| `tools/schema_validation/native/transport_core_v1.h` | `17781526228ce84536558fc9ab9fd5f59f447e7c` |
| Written conflict reason `schematool-001-native-observation-clarification.md` | `386213f412c98751dc8ead1aaaca69ba690f4e36` |

Authority remains the accepted logical/public/context contracts, native contract
semantic revisionaec, C44/C48 scope, unchanged
[74-oracle map](schematool-001-native-test-mapping.md) and actual
[negative matrix](schematool-001-native-owner-test-matrix.md). Matrix case25 already
requires the actual committed interrupted18 from a native final observation when
real pending SIGINT prevents ctypes from exposing the C return. The previous
ledger cell wording said only syscall result, while final-commit kind8 was already
defined. The private header also used a syscall-only alias for that field. Leaving
that conflict implicit would make a required observation unspecified; dropping
the18 assertion or fabricating a Python-received integer would weaken the oracle.

The actual revision gives existing cell5 a kind-specific meaning. Kind2 keeps the
actual raw kernel return. Kind8 records only the common owner's **actual** closed
ABI integer after cleanup/quiescence and final decision, before C return/Python
conversion, with errno unavailable-1. It neither supplies nor overrides the
decision. Other undefined fields remain-1. Independent actual FD/reap observations
remain mandatory; a final scalar is not proof that a child/FD disappeared.

Private18 remains an interrupted native transport value, not a public enum. Real
pending Python SIGINT may propagate through the real CDLL boundary after cleanup;
without that exception,18 maps to validation_unavailable. No integer manufactures
KeyboardInterrupt or an accepted validation result. The unchanged inert31 stub
emits no observation/factory event. Future adapter source must record only actual
observer data; this note does not approve any proposed future event-producing body.

Observation.result changes from alias RawResult to its exact existing underlying
type std::int64_t, so representation, alignment, field order and C++ member type
are identical. RawResult remains syscall-only in Ops/checkpoint declarations. No
owner body, C ABI/export, journal kind/count/layout, phase, map/scratch/FD budget,
public value/limit/priority or existing assertion changes. This is an explicit
private instrumentation reconciliation within the accepted scope, with no owner
decision or added dependency required.

Actually performed: full source/contract/matrix/reason review; stdlib source-only
comparison proving the header differs only by alias spelling/comments and that
exactly the three expected paths changed; existing ABI/stub inspection;
`python3 tools/check_project.py` PASS426/122;
`python3 tools/progress.py render --check` current; `git diff --check` PASS.
No candidate import, compilation, helper/test body, binutils, native artifact,
SIGINT, child/probe or SDK operation was executed. Reported local foundation
results await their separate immutable raw attribution; they are not certified
by this document/type review.
