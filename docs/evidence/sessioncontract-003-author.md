# SESSIONCONTRACT-003 — lifecycle author record

Author `/root/session_lifecycle_contract_author`, 7 November2026, isolated worktree
`/tmp/subathon-sessioncontract003-author`, branch
`task/SESSIONCONTRACT-003-author`, supplied claim `db6fbee` from actual main
`ebb4368ab5af3b0c39314607896a6213fc061bb5`.
Own only [new lifecycle proposal](../contracts/session-lifecycle-v1.md) and this
record. Root owns shared state/HTML/handoff/schemas/build/CI/pins and publication.
[Coordinator handoff](sessioncontract-003-handoff.md) is the task entry point.

Read root AGENTS, FIRST_AGENT_PROMPT, architecture, agent workflow, test strategy,
task/state, ADR-0010, full SESSION-002/ACCESS-004, relevant v0.45 §10/16/AUTH rows,
and accepted supplier/transport/password/writer/effective-permission contracts.
No scoped docs AGENTS file was present. Existing initial environment/behavioral
baselines are retained; this assignment is document-only, not a replacement first
implementation/test assignment.

Exact unchanged imported document blobs on the supplied branch:

| Path under docs/contracts | Git blob |
| --- | --- |
| session-resolution-v1.md | c5f5a6e71664042e18475068d3caf577b0c8d009 |
| session-transport-v1.md | 2c33c2bbe9052455107c3056783d6bb605a5fef2 |
| password-hashing-v1.md | bc9ccbf66a41c9ae69abde4bd905401900e17461 |
| password-hashing-api-v1.md | 7ad1e8d3f3311673dd368f2ef4d297de31dbafcd |
| writer-transactions-v1.md | 4ba20506db9a858450099b82888a22c8f6dbd761 |
| effective-permissions-v1.md | 3d82bfa418bb14099ee130fca7a400adca66500b |
| effective-permissions-api-v1.md | dda2793c71299dbe36a321ce3dd5620fecc8b214 |

The proposal makes six internal commands, current source/conditional-write ports,
finite input/source/result/page ownership, exact staged failure priority, actual
accepted-sign-in/account/credential/install binding, self/current-admin disclosure,
writer/CAS/cleanup/unknown/post-commit truth and normal-completion-only secret
publication reviewable. Absolute positive-U64 lifetimes/full-i64 expiry, default
24h/30d, inclusive5m freshness, epochs, collision refusal, no overlap/sliding renewal,
per-session independence and unrelated-session preservation import supplier §4.
Local accepted-attempt authority/reservation remains its existing upstream task;
actual verified Twitch remains downstream ACCESS-004 with no claimed fake support.

Review-sensitive internal choices: same-session identity for accepted-fresh reauth;
fresh accepted attempts bounded by the existing five-minute definition and reserved
through actual outcome; exact-account bounded keyset inspection with one probe,
no lifetime session-count quota; source-before-effect current permission and
complete result projection; one nonreentrant queue scope through real completion;
private fixed secret owner with at-most-once synchronous emitter handoff. All are
proposals for separate review, not accepted requirements/runtime evidence.

No header/schema/migration, test/stub/source, crypto/library/protocol/dependency
choice or runtime/provider action is added. No new substantial prerequisite is
identified beyond the already split store/local-auth/clock/ingress work. Actual
producer designs must expose new conflicts/prerequisites before implementation.
All original test-first/platform/provider/release gates remain. Independent active
SESSIONTEST-008 baseline/review files were not touched or used as lifecycle approval.

Independent document audit actually run from this worktree with `python3 - <<'PY'`:

```python
from pathlib import Path
import re
I64_MIN = -(1 << 63)
I64_MAX = (1 << 63) - 1
U64_MAX = (1 << 64) - 1
assert 1000 + 86400000 == 86401000
assert 1000 + 2592000000 == 2592001000
assert I64_MIN + U64_MAX == I64_MAX
assert I64_MIN + (U64_MAX - 1) == I64_MAX - 1
assert I64_MAX - 86400000 == 9223372036768375807
assert 9223372036768375807 + 86400000 == I64_MAX
assert 9223372036768375808 + 86400000 > I64_MAX
assert 301001 + 86400000 == 86701001
assert 301000 - 1000 == 300000
assert 301001 - 1000 == 300001
assert (1 + 999) // 1000 == 1
assert (1001 + 999) // 1000 == 2
assert I64_MAX - I64_MIN == U64_MAX
assert (U64_MAX // 1000) + (U64_MAX % 1000 != 0) == 18446744073709552
assert 1 + 2 + 1 + 1 == 5
assert 1 + 2 + 32 == 35
assert 1 + 4 + 1 + 24 == 30
assert 1 + 4 == 5
assert 2 + 1 + 1 + 24 == 28
assert 1 + 3 + 8 + 28 + 1 == 41
assert 1 + 3 + 8 + 56 + 2 == 70
assert 1 + (3 + 1) + 2 + 1 + 65 * (6 + 32 + 1 + 24) == 4103
ordered = [b'a', b'b', b'c']
assert ordered[:2] == [b'a', b'b']
assert [x for x in ordered if x > b'b'] == [b'c']
assert [x for x in ordered if x > b'z'] == []
assert sorted([b'\x80', b'a\x00', b'a', b'\x00']) == [b'\x00', b'a', b'a\x00', b'\x80']
text = Path('docs/contracts/session-lifecycle-v1.md').read_text()
assert re.findall(r'^\| LC(\d\d) \|', text, re.M) == [f'{n:02}' for n in range(1, 33)]
print('PASS: independent full-domain arithmetic, DTO budgets, byte ordering and 32 literal-row IDs; no project service/native code invoked')
```

Exit0, all assertions PASS. This audit checks literal arithmetic/count/order facts,
not lifecycle behavior. Literal LC01–32 outcome derivations remain independently
reviewable proposed oracles. No product implementation generates their expected
results. No crypto, password, session service or provider function was invoked.

Actual document checks on 7 November2026, Python3.14.9, git2.56.0,
Linux7.2.14-arch1-1 x86-64:

| Exact command | Observed outcome |
| --- | --- |
| `python3 tools/check_project.py` | Exit0, PASS443 task records/122 decisions/local links/presets/current HTML. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit0, all19 tooling tests PASS. These are tooling, not product behavior. |
| `python3 tools/progress.py render --check` | Exit0, progress snapshot current; shared files were not regenerated/edited here. |
| `git diff --cached --check` | Exit0, staged two-file document packet has no whitespace errors. |

No native build/test applies to this document-only delta; none was run. Existing
document/tooling CI is appropriate, and no executable lane or dependency is added.
Root owns actual PR/publication, latest-head CI and shared state/HTML evidence.
The actual author commit and different review are recorded by the coordinator
handoff; this author supplies no independent approval. Remaining gates are actual
document review, later minimal-source/harness/test review and intended red, real
lifecycle/producer/store implementation and separate code/configuration reviews,
selected CI, operational and supported-platform/live-provider verification.
