# ACCT-001 receipt intake implementation evidence

Author/implementer: `/root/receipt_implementer`; date: 2026-10-31.
Branch: `task/ACCT-001-implementation`, isolated worktree
`local-evidence/worktrees/acct001-implementation`.
Base: merged main `7900a614214f72da85c31d44643cf32a078729cc` (baseline PR15).
Source commit: `875c1c98a2b12e463528295db7a62f094dcb4d77`.
Source blob: `7a0986bdd3e4b5e38d8b7ac18d76dd56ff002ee8`.

## Behavior and preserved baseline

Implemented only the framework-free `ReceiptIntake::ingest` service in
[`server/src/receipt_intake.cpp`](../../server/src/receipt_intake.cpp), following
[receipt intake revision 1](../contracts/receipt-intake-v1.md), specification v0.45
§§3.2–4,14,18 and the [ACCT-001 task brief](../tasks/EPIC-002/ACCT-001.md).
The [baseline handoff](accttest-001-baseline-handoff.md) records author `28729c7`,
independent review `54cff97`, integration approval `a843e4d` and observed runtime red
at `18d8670`: all 15 cases failed from the explicit missing-behavior stub after a
successful build. Those gates preceded this implementation.

Each call opens one repository transaction and finds the complete source identity.
An absent identity creates the original receipt; an existing identity returns its
stable ID and compares incoming facts with the immutable original facts to classify
retry or conflict. Every successful path appends the incoming facts, arrival/context
and disposition as an attempt, commits, then returns the result. Exceptions propagate;
the transaction object's owned lifetime supplies rollback under the repository port
contract. There is no service cache or automatic retry and no original-fact/context
replacement.

No reviewed assertion, fixture, fake, public interface or contract was changed. Exact
Git blobs rechecked after the passing commands:

| Preserved artifact | Git blob |
| --- | --- |
| Core receipt values | `f11b0d60013d9606ccfc58188f4363814f26c13c` |
| Server service/repository header | `273f6b4f1aa99f510779f13b965906ac17169e9e` |
| 15 acceptance cases | `94317dc4dde4553c8888f78a433e3003c471de1a` |
| Primitive fake repository | `99ff2eec97a10c89dc93f0aec5b8c4cee88906a8` |
| Receipt contract revision 1 | `149d65b5aa0864c6323e4d5ff6d9580cdbcf2886` |

## Actual local verification

Scope: synthetic, disposable, fake-backed service simulation and existing numeric
regressions. Environment: Linux x86-64; GCC 16.2.1; CMake 4.4.5; Python 3.14.8;
GoogleTest 1.18.0; Boost 1.92.0 for the separate numeric lane; clang-format 23.1.4.
Commands ran against source commit `875c1c9` on 2026-10-31. Expected result was passing
reviewed receipt behavior without altering the existing credit/numeric behavior.

| Exact command | Actual result |
| --- | --- |
| `cmake --preset receipt-intake-debug` | exit 0; configured with installed dependencies |
| `cmake --build --preset receipt-intake-debug` | exit 0; ten compile/link/discovery steps |
| `ctest --preset receipt-intake-debug -L receipt-intake` | exit 0; 15/15 reviewed cases passed |
| `cmake --preset acceptance-debug` | exit 0 |
| `cmake --build --preset acceptance-debug` | exit 0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit 0; 19 credit acceptance plus 4 existing regressions passed |
| `cmake --preset exact-numeric-debug` | exit 0; installed Boost 1.92.0 discovered |
| `cmake --build --preset exact-numeric-debug` | exit 0 |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit 0; 32 numeric acceptance plus 1 existing regression passed |
| `clang-format --dry-run --Werror server/src/receipt_intake.cpp` | exit 0 after formatting, before source commit |
| `git diff --check` | exit 0 |

The receipt cases retain coverage for first intake, full identity scope, distinct
events with equal payment facts, reconnect context, missing facts, all-field conflict
classification, service recreation and failed create/attempt/commit rollback. Numeric
and credit regressions were replayed unchanged. No skipped cases, failure inversion or
new tests mirroring the implementation were used. The inherited `intentional-red`
label remains historical harness metadata in this author's branch; coordinator-owned
CI and label activation are separate integration changes.

## Review and remaining gates

Independent code reviewer `/root/baseline_reviewer` received the actual source commit
and blob before author verification finished. This author record supplies no
independent approval; the separate review records its own inspected revision and
replay. Coordinator owns progress/HTML, build labels, automatic receipt CI, acceptance
map, integration checks and publication/merge evidence.

These passing cases establish only the bounded service/repository-port simulation.
They establish no SQL uniqueness, durable commit/restart recovery, concurrency,
acknowledgment durability, Qt writer/thread ownership or provider identifier behavior.
No live provider, credentials, payment, chat or outbound action was used. Contribution
eligibility/correlation and one-contribution/one-application behavior remain downstream
ACCT-002/004; storage and API integration retain their own gates. Full agreed release
scope, including all integrations/rate providers/platforms, is unchanged.
