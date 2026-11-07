# CONTRACT-013 author packet

Author: fresh `/root/reference_contract_author` agent/session, 7 November 2026.
Worktree `/tmp/subathon-contract013-author`, branch
`task/CONTRACT-013-contract-author`, base current main
`e0585ca112a3eb52c8f247a97fcc6ba8d964511b`.
The coordinator owns task claim/state/handoff/HTML, shared schema imports, build,
dependency pins and CI. This author owns only
[reference HTTP R1](../contracts/reference-http-v1.md),
[reference package](../../schemas/http/reference-v1/manifest.json) and this record.

Authored, pending actual different contract/schema/example review and admitted
package validation. This is no test baseline, intended red, production/runtime,
platform/browser/provider or advertised-capability acceptance. No implementation,
validator/library change, test weakening or dependency addition was made.

The packet fixes only public POST negotiate and protected GET committed timer,
their exact method/path/header/query/body/representation rules, imported auth/CSRF,
current execution disclosure, status/priority, complete finite ownership and closed
schemas. The smaller4096-byte reference request profile explicitly bounds the
outer request and narrows the imported helper's header limits; it does not silently
inherit32768 header bytes. It retains HTTP006/CONTRACT003/full-release scope.

The package has25 unique files: manifest, oneOpenAPI3.1.1 document and23 external
positive example files, each explicitly bound once to its actual component schema.
There are no inline example/default occurrences, remote references, negative
manifest flags or scene imports. Negative/raw HTTP/duplicate-key, exact numeric
rejections and runtime/current-state cases are the document's H01–26 oracles;
static decoded-instance schema acceptance cannot establish them. Some endpoint/
zero snapshots are explicitly schema-only and do not claim ready producer facts.

Acceptance decisions needing reviewer attention: status409 versus422 version/
capability ordering with a fixed bounded public upgrade header; HTTP envelope
checks before protected auth while preserving the imported auth's internal order;
public negotiation ignores incidental credentials and never resolves them;
ordinary current permission `timer.snapshot.read`, actual `timer` resource slot,
no quantity slots or freshness requirement. Nothing bypasses current access,
changed ingress-policy503/fresh-request handling or committed-read provenance.
No substantial new architectural prerequisite was found or implemented. Real
lossless/log-safe ingress, earlier finite framework/connection budgets, session/
permission/source/queue/readiness adapters and actual read checks remain existing
runtime gates; unknown producer guarantees cannot enable this profile.

Import blobs read unchanged: reference pathR1
`4b705cba2ab999543689b3e181fdd6a84ce1f33b`; common wirev1
`11eeb5f422a174e042143df6b476ae1d717eeacf`; supplier
`c5f5a6e71664042e18475068d3caf577b0c8d009`; transport
`2c33c2bbe9052455107c3056783d6bb605a5fef2`.

Checks actually run in this worktree, Python3.14.9 on Linuxx86-64:

| Exact command | Outcome |
| --- | --- |
| `python3 tools/check_project.py` | exit0;427 task records,122 decisions, local links/presets/currentHTML pass;19 draft GoogleTests are not run or approved |
| `git diff --check` | exit0; whitespace pass |
| `python3 - <<'PY'` with the finite inspection script below | exit0; JSON syntax/inventory path/pointer inspection and literal regex checks pass; package43999 raw bytes |

```python
import json,re
from pathlib import Path
p=Path('schemas/http/reference-v1')
m=json.loads((p/'manifest.json').read_text())
a=json.loads((p/'api.json').read_text())
files=[p/'manifest.json']+[p/d['path'] for d in m['documents']]+[p/e['path'] for e in m['examples']]
for f in files:json.loads(f.read_text())
assert len(files)==len(set(files))==25
assert {str(f.relative_to(p)) for f in p.rglob('*') if f.is_file()}=={str(f.relative_to(p)) for f in files}
for e in m['examples']:
    v=a
    for s in e['schema_pointer'][1:].split('/'):v=v[s.replace('~1','/').replace('~0','~')]
    assert isinstance(v,dict)
s=a['components']['schemas']
pos=re.compile(s['NonnegativeMilliseconds']['pattern'])
neg=re.compile(s['SignedUtcMilliseconds']['anyOf'][1]['pattern'])
for v in ['0','1','9223372036854775806','9223372036854775807']:assert pos.search(v),v
for v in ['-1','-0','00','01','9223372036854775808','18446744073709551615','1\n']:assert not pos.search(v),v
for v in ['-1','-9223372036854775807','-9223372036854775808']:assert neg.search(v),v
for v in ['-0','-01','-9223372036854775809','-1\n']:assert not neg.search(v),v
assert pos.search('0') and not neg.search('0')
print('PASS: strict JSON syntax; 1 document, 23 explicit bindings, 25 unique inventory files; target pointers exist; literal signed64 regex endpoints/rejections; git whitespace.')
print('Package bytes:',sum(f.stat().st_size for f in files))
```

The inspection is deliberately not a JSON Schema/OpenAPI validator and establishes
no full reference closure, native semantic/schema/example acceptance or running
HTTP behavior. No local admitted schema venv/native artifact survived the
foundation work; no unadmitted artifact was built or called. The coordinator will
run the public `tools/schema_validate.py --manifest
schemas/http/reference-v1/manifest.json` on the admitted hosted workflow AFTER
ordered foundation gates, and record the actual result. Different review of the
actual authored files/examples/imports precedes final acceptance and APITEST-010;
all later separate test review/intended red/implementation/code review remain.

Next action: coordinator integrates this focused author commit, arranges different
packet review, resolves findings, then observes actual hosted package validation.
Publication/PR/progress and final check evidence are in the coordinator handoff;
this author record does not invent a PR, review or validation receipt.
