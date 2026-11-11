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
