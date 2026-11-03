# ACCTTEST-004 minimal header and stub author evidence

Author bundle_implementer,2026-11-03. This is the explicitly released MINIMAL
INTERFACE/STUB phase, not service/codec/view behavior, fixtures, tests, intended red,
implementation, API approval or product acceptance.

Root recorded both actual DOCUMENT gates on final contract
8fa0c76ad90ad046712463dfc95206e2a99b4e40 / document-author note
608acc129c48f8ee5ed97fd09db1ade541511c56:
full reviewer b098c7e→root99d3ef6 and independent bounded7b501fd→950db76.
Root release f476706 preceded these edits. The same isolated
task/ACCTTEST-004-service-api-author worktree continued from ee056945, preserving
all previous document history and old storage source/fixture/review worktrees.

Actual API/source checkpoint2b96ae14ee89fedfc74ae84979acd4602f1ddb3c:

| New owned path | Git blob |
| --- | --- |
| server/include/subathon/server/application_service.hpp | 03a747a70ab7decd49227396466ff12a32c31864 |
| server/src/application_service.cpp | ebbea2388b7f4fb6474940b53cc14b95853969cb |

This note is the only other owned path. Approved contract and old author note
remain BYTE-EXACT at8fa/608. Root owns CMake/presets/CI/progress/HTML/shared schema
and integration; no shared configuration or test/fixture was changed here.

## Finite surface and inert state

Header contains owned command/principal/limits and all agreed error accessors;
finite access/policy/admission/clock/token-free calculation/action/reference values;
pure virtual trusted access/disclosure/policy/clock/action/issuer ports; aggregate
borrowed port references; typed response payload and failure reason variants;
actual writer outcome/error completion and accessible body/matching-completion pair.
The exact owned ApplicationServiceResponse has Q_DECLARE_METATYPE, without any
coercion/registration/extraction implementation.

ApplicationReferenceReadView exposes ONLY find_retained and an inert default
constructor. Private weak callback state is forward-declared, with no public
Session/SQL/writer/adoption or capability-construction escape. Copy/destruction
does not extend callback access. Default/expired/foreign validation is NOT
implemented by this phase.

OriginalApplicationService's constructor merely stores writer/ports references
and limits. The host lifetime requirement includes outstanding unsubmitted work
pairs as specified in the approved document. No constructor validates inputs,
adopts a capability, binds authority or reads/writes/queues/publishes anything.
DTO enum/group/payload validity is likewise not silently implemented.

Public codecs are ONLY the approved complete-command encode/decode entries.
State/result/audit/hold-observation/synthetic-job retained byte meanings remain
the approved document's semantics, without unnecessary new public codec methods
or an implementation. No third-party dependency was added.

## Stable missing-feature attribution

All FIVE callable feature definitions are immediate unconditional single throws:

| Entry | Stable exact exception class |
| --- | --- |
| encode_original_application_command | OriginalApplicationCommandEncodeNotImplemented |
| decode_original_application_command | OriginalApplicationCommandDecodeNotImplemented |
| ApplicationReferenceReadView::find_retained | ApplicationReferenceReadNotImplemented |
| OriginalApplicationService::make_work | OriginalApplicationServiceNotImplemented |
| OriginalApplicationService::enqueue | OriginalApplicationServiceNotImplemented |

Each class derives std::logic_error. Exact messages are respectively:
"Original application command encoding is intentionally unimplemented; review tests first";
"Original application command decoding is intentionally unimplemented; review tests first";
"Application reference reading is intentionally unimplemented; review tests first";
"Original application service is intentionally unimplemented; review tests first".
The two service entries deliberately share their named service feature marker;
codec/view entry markers distinguish those missing features from generic failure.

There is NO pure validation/parser/encoding/counting/equality algorithm, current
access/policy/clock/numeric/action logic, reference issuance, session adoption,
SQL/BEGIN/COMMIT/cleanup, queue admission, result bridge or publication behavior.
No new method, marker, feature body or stub factory was invoked to produce this
evidence. Compiling explicit throws is not an observed intended red.

## Actual checks and environment

Linux x86-64; GNU16.2.1 (20260810), C++20, QtCore6.12.0,
clang-format23.1.4. Existing system dependencies suffice.

- clang-format -i server/include/subathon/server/application_service.hpp
  server/src/application_service.cpp — PASS, only new owned files.
- c++ -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only
  $(pkg-config --cflags Qt6Core) -Iserver/include -Icore/include
  server/src/application_service.cpp — PASS, zero diagnostics. This performs syntax/
  type checking only; no library/test/service executable was run.
- Static Python source inspection found EXACTLY five complete single-throw bodies
  in expected order with the table's marker classes; no execute/enqueue_work/begin/
  commit/cleanup call. It did not load/execute the new C++ methods.
- python3 tools/check_project.py — PASS396 task records/122 decisions/local links/
  presets/current HTML; no service baseline is compiled/approved by this tool.
- python3 tools/progress.py render --check — PASS, no shared render regenerated.
- git diff --check — PASS. Changed-path comparison from ee056 listed only the
  two new interface/source files before this note.
- git hash-object of approved contract and previous document-author evidence
  confirms unchanged8fa0c76/608acc1.

Root will wire the actual target/profile and compile the latest integrated archive.
Actual different API/configuration review and OWN selected foundation CI must precede
independent fixtures, their full review and selected-green gate. Feature assertions/
new entry invocation and FIRST same-clean service/codec/view marker red remain
later explicit phases. ACCT-004 production remains planned behind that real baseline.
No provider/network/live/user-state operation or routine approval request occurred.
