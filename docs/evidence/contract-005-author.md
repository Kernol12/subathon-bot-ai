# CONTRACT-005 common wire document author handoff

Author: codec_author, 2026-11-04. Branch `task/CONTRACT-005-document-author`, isolated
worktree `local-evidence/worktrees/contract005-author`, from integrated main
`dd6a141107e4e950f3c6c3f6bd3a74b5ba1f7f09` plus actual claim
`c1e985930a87e1e8a5cd1deb16276f7456a8d97d`. CONTRACT-002 and reviewed planning
CONTRACTPLAN-001 are done; this is ONLY CONTRACT-005's first document stage.

Owns only [wire-primitives-v1.md](../contracts/wire-primitives-v1.md) and this note.
Root owns state/HTML/schema/imports/build/CI/pins/maps/checkpoint/publication.
Read root/schema instructions, actual child brief and accepted ADR-0004, workflow/
architecture, specification §§2/3.5/10 and API-01/02/03/NUM-04, reviewed event/
exact-decimal/application-plan/storage/application-service boundaries. No prior
artifact, assertion, source or parallel worktree is changed.

## Proposed bounded contract

Document Git blob `11eeb5f422a174e042143df6b476ae1d717eeacf`, SHA256
`5df6ff027369f84429f0c3510948798bb4319d9c2a994ce92ee30a120212e13d`.
Its proposed identity is `subathon.wire-primitives.v1`, not an available API version.
It defines JSON value semantics and 27 manually authored valid/invalid WP examples,
not schemas, functions, generated fixtures or a passing executable baseline.

Signed/unsigned64, arbitrary-precision integer/rational components and exact
decimal/accounting data are strings with explicit canonical/domain distinctions.
DecimalSource preserves the source lexeme, including signed zero/trailing zeros/
exponent spelling; FiniteDecimal and Rational represent canonical derived VALUES.
Recurring 5/6 remains rational and cannot be approximated into a decimal field.
Raw evidence outside a selected numeric grammar remains separately retainable.

An important actual existing boundary informed the proposal: STORAGE-015 retains
Exact values above 4096 bits with configurable component_bytes (default 16384
DECIMAL BYTES), and references default to 4096 bytes. CORE-001's 512-byte source
parser and 4096-bit finalizer are local contracts. The common layer imposes neither
as a generic wire limit. Finite owning field/message profiles must be pinned before
implementation and must preserve the actual domain. Byte-only numeric policies
remain valid. The −10^−511 boundary illustrates why canonical expansion can require
514 output bytes from a valid 263-byte source.

For opaque bytes, proposed lowercase hex has a single simple spelling, preserves
NUL/non-UTF-8 and costs exactly twice the raw length. The profile explicitly counts
that expansion; this is a documented engineering tradeoff, not a Unicode/ISO/UUID
restriction or encryption. Required identities apply their own nonempty/bounds;
Text preserves Unicode scalar sequences separately. Unknown/null/omission/zero/
present-empty are kept distinct, with domain knowledge states left to their actual
owning schema rather than fabricated from missing provider metadata.

Correlation is separate bounded control metadata (at most 256 decoded bytes).
Operation and revision references keep their domain's actual bounds. Counter
revisions use full Unsigned64; opaque revisions are never parsed as counters.
The document deliberately leaves payload identity, replay access, conflict ordering
and atomicity with the existing owning operation. It does not make a stale-current-
revision check override legitimate protected stored-result retry behavior, or turn
a timeout into a confirmed rollback/new award intent.

Version/capability labels have bounded ASCII vocabulary and explicit exact-identifier
selection semantics. Capability names are synthetic; they establish neither
authority nor actual provider/platform support. Concrete handshake envelopes,
offer limits, matching web assets and disclosure profiles remain HTTP006/API-004;
persistent state synchronization remains WS007. A bounded closed CommonFailure
record contains public code and safe correlation only, with at most 1024 JSON
bytes. No private input/diagnostics/credentials are reflected. Cookie/header/token
validation and current authorization retain their SESSION/ACCESS/owning API gates.

No actual schema/OpenAPI/TS/C++/stub/client/fixture/test/parser/encoder was authored
or generated. No new feature, wire codec, provider or network operation was invoked.
Schemas/imports still need integrator-assigned paths and actual subsequent review.

## Actual checks and their limits

An inline standard-Python command imported only Fraction/gcd and asserted 20
independent arithmetic facts. No wire formatter/parser, JSON/hex encoder or project
feature supplied expectations. Checks cover signed64/U64 endpoints and distance,
2^64, equal source-decimal values, signed mathematical zero, gcd/canonical rational
facts, source mantissa/byte/scale counts 256/263/511, expanded output 514, the
8192-bit decimal-digit bound 2467, reference hex expansion, error/large component
byte sums and unsigned mixed-byte ordering. All assertions passed. The key literal
bound arithmetic is reproducible without generating a schema or encoded artifact:

```sh
python3 - <<'PY'
from fractions import Fraction as F
from math import gcd
assert -(2**63) == -9223372036854775808
assert 2**63-1 == 9223372036854775807
assert 2**64-1 == 18446744073709551615
assert (2**63-1)-(-(2**63)) == 18446744073709551615
assert 2**64 == 18446744073709551616
assert (F(120,100),F(12000,10000),F(12,10)) == (F(6,5),F(6,5),F(6,5))
assert -0*10**3 == 0
assert (gcd(5,6),gcd(10,12),gcd(0,2)) == (1,2,2)
assert F(-1,3) == -F(1,3)
assert 1+254+1 == 256
assert 3+254+6 == 263
assert 255+256 == 511
assert 3+510+1 == 514
assert 263 <= 512 < 514
assert len(str(2**8192-1)) == 2467
assert 2*4096 == 8192
assert 64+2*256+31 == 607
assert 2*16384+33 == 32801
assert 0x4f < 0x80 < 0xff
assert 2*3 == 6
PY
```

Also ran `python3 tools/check_project.py`: PASS, 401 task records, 122 decisions,
local links/presets/current HTML. Its old draft-test notice is not wire test
coverage. `python3 tools/progress.py render --check` passes; `git diff --check`
passes. Final staged-path/whitespace verification accompanies the commit. No native
build/test or wire parser/feature execution occurred.

Existing scaffold CI covers this document/link/progress change; no executable
behavior or dependency justifies a new workflow here. Root records actual final
PR checks. Later validators/codecs need focused reviewed test/build/CI work and
actual selected results; this note claims none.

## Remaining gates

Actual different document review is next. The owning concrete schema/API/limits/
failure-order and independent literal-test reviews still precede minimal interfaces,
selected foundation, first valid missing-behavior red and later implementation/
source acceptance. Field profiles are mandatory future gates, not an authorization
to implement an unbounded generic decoder. HTTP/WS/scene endpoint families remain
their focused contracts and must split further if an actual claim is too large.

CONTRACT-005 remains in progress and CONTRACT-003 remains planned. No policy answer,
auth shortcut, provider metadata, supported capability, domain migration or release/
platform/renderer verification is invented. All seven integrations and three FX
providers remain required. Prior rate/milestone/password work is preserved; root
owns subsequent stage releases and shared publication.
