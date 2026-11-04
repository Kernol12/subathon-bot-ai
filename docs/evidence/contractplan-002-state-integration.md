# CONTRACTPLAN-002 integrated dependency proposal

Coordinator /root,2026-11-04. Branch task/CONTRACTPLAN-002-scene-prerequisites,
claim0fdbc5e based on actual main009632386564f3a00fb413580ac1cee3ef43cc11 after
PR58/all12 final-head checks. Actual planning author85be9bb8/import2c76307 changed
seven assigned Markdown files. Root now applies only the exact ADR-0005 table.

The pre-proposal402-node state (including this active planning claim) has1473 edges.
The actual406-node graph has1497 edges: parent008 adds PLAN002 and009–012(5), while
new009 contributes2,010 contributes11,011 contributes2 and012 contributes4.
All1473 oldedges are retained, including319 whose prerequisite is a test baseline.
Only one original dependency list changes:008 appends gates. Every other prior list,
HTTP006→008/C003→008/SCENE/RENDER/epic/release prerequisite remains unchanged.
The original008 brief is a byte-identical prefix; author adds23 integration lines.

010's domain gates deliberately use actual existing baseline/model owners; no
HTTP/WS/viewer/renderer/parent dependency creates a cycle. A completed baseline
still needs suitable concrete model evidence. Newchildren remain planned/unverified;
PLAN002 is in_review until a different reviewer approves these actual documents and
state. No source/assertion/schema/codec/validator/pin/renderer/provider changes.

Inline stdlib comparison of before/after actual state and original008prefix: PASS.
`python3 tools/check_project.py`, currentHTML/render check and whitespace checks
accompany this commit. Independent review must verify exact graph and requirement
allocation itself, not merely reuse author/root counts. Existing consistency/
component CI still applies to the final PR head; no new executable suite here.
This planning artifact approves no scene feature, typed domain or platform support.
