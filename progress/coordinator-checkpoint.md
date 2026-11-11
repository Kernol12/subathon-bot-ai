# Coordinator checkpoint

Owner explicitly authorized CI runner changes after PR118's nine failed-only reruns
(attempt3 on7a7d821) all failed Ubuntu mirror DNS before compile/test bodies.
Selected CI-001, brief docs/tasks/CI-001.md. Isolated coordinator checkout
/tmp/subathon-ci-001, branch fix/CI-001-ubuntu-apt, base main38b482d.
Original PR118 checkout/branch and LOCAL uncommitted checkpoint remain untouched.

Coordinator /root owns contract/test baseline and shared metadata. Fresh implementation
session and full independent contract/tests and source/workflow reviews remain pending.
Known provider capability: GitHub Linux runners support passwordless sudo; changes
last only for that disposable job. Recovery contract uses official mirror list,
bounded strict APT update and diagnostic DNS; no resolver change assumed necessary.
All dependency pins/trust/SDK hashes/bodies/job names and PR118 approvals preserved.
No product task, API implementation, session-clock or valuation work authorized.

Finish only CI repair and PR118 normal publication gates, then STOP. Do not bypass
protection or merge red checks. PR118 is linked but unwatched after all reruns finished.

Test stage: initial Claude0495679b reviewed80de648 B1–6; correction15e4343,
separate Codex ci_001_test_verification C1/C2 fixedb24ad5a and verified READY;
closing Claude02fcd8e0 approvedb24ad5a. Reports under /tmp/ci-001-*. No tests run.
New current incidents15296/15297 and Canonical RSS DNS major outage show mirror-only
ubuntu.com endpoints insufficient. Contract revision2 adds preferred kernel.org
Ubuntu mirror (ordinary existing Ubuntu signatures/pins unchanged), seventh diagnostic
and all-Canonical-DNS-failed example. Fresh Codex and Claude closing pending before red.
