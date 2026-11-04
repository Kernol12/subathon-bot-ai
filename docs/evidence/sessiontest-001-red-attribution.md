# SESSIONTEST-001 independent FIRST local red attribution

2026-11-04, reviewer `/root/codec_author`, separate from the test author and
coordinator who executed the run. Reviewed actual retained evidence at
`bf0a805c6af4b15394608cc867d7319174361088`; this note owns no source, fixture,
harness, workflow or progress change.

**VALID intended local red: all 37 executed reviewed cases fail because the
password feature still throws `PasswordHashingNotImplemented`.** No unrelated
failure invalidates this run. This approves attribution of this particular failing
baseline, not passing acceptance, remote CI, task integration or production.

## Actual run and prerequisites

The [original receipt and corrected mechanical inspection](sessiontest-001-red-results.json)
record clean execution revision `d560cbda44cb180bab8d12fc79d3da5fefded6ba`,
started `2026-11-04T16:06:50.483521Z`, core limit 0, CTest exit 8. The ordinary
command was:

```sh
ctest --preset password-hashing-debug -L '^password-hashing$' --verbose --output-junit /home/archnoob/ProgrammingProjects/SubathonProject/subathon-bot-ai/local-evidence/worktrees/sessiontest001-integrator/local-evidence/sessiontest001-red/first-red.xml
```

Actual XML: 37 tests, 37 failures, 0 disabled, 0 skipped. Each case contains a real
GTest RUN and FAILED entry, rather than a discovery-only result. The exact 37
unique enabled source names equal the XML and both receipt inventories. Suite
grouping changes their order relative to source declarations; nothing is missing
or added. The original command selected only the feature label, excluding the
separately proven foundation. The receipt's process wall time is 0.062666 seconds;
CTest reports 0.05 seconds. XML's 17:06:50 local timestamp agrees with 16:06:50 UTC.

Actual corrected full-test approval `093b528e743a0b0190e3570cacbd8cdaa0c8105f`
was integrated as execution revision `d560cbda`; the
[review](sessiontest-001-test-review.md) blob is
`d4f7213bec3014a7eab4bde11992ea195e60b92a`. The earlier R1/R2 findings are
historical, followed by approval of the actual corrected bytes. Independent
[environment review](sessiontest-001-environment-review.md) `d0a3819b` and actual
[adoption](sessiontest-001-local-red-sequencing.md) `e83c704` preceded execution.
Their conditional local ordering applies to this exact host SDK. It does not
assert Ubuntu CI passed or transfer to another selected dependency.

Read-only hashes confirm that the present 1,814,688-byte test executable matches
the execution receipt: SHA256
`d7427c9cce123c2c38cacdc19b68f446458b1cc4a77746348f9115ed4836f241`.
The selected static SDK's 836,744 bytes still hash to
`959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935`, matching
the independently reviewed [actual host foundation](sessiontest-001-native-results.json).
That earlier real foundation was not rerun. The clean-head assertion is retained
execution evidence; later read-only checks corroborate its frozen source/binary
identity, rather than claiming to observe the past worktree afresh.

All eight execution source blobs equal both their `d560cbda` versions and the
previously approved candidate:

| Artifact | Git blob |
| --- | --- |
| Feature tests | `1d0d37753cd54cad189bdf9d08481838cac9880e` |
| Literal fixture | `025d4de904c4c07a08cc192fc6cf94318bd5c0c8` |
| Private observer header | `7e87bbe9c328a8791244902c7e9446ceef000996` |
| Private observer source | `4e1f049a2dcd0ff018dc3c728daad39f0773a82f` |
| Test CMake | `9b10f68ce44ea5bcc0f279e5576e5c579613ae96` |
| Workflow source | `cb61b3ec028eddee805f7b6c9928d834789b54c7` |
| Production header | `ec3c5cf87afe406db8501053321598599325a239` |
| Production source | `eaf56f41bafd502b8e4cb38c0de3082180bffcd5` |

## Causal findings

I inspected all XML case outputs and their failure locations, not merely the
coordinator's booleans. There are 209 assertion failures: 158 are the specific
typed-marker catch paths in the test helpers (36 at source line 49, 122 at line
67). Each case reaches at least one of these paths. Their diagnostic is:

```text
PasswordHashingNotImplemented: Password hashing is intentionally unimplemented; review tests first
```

The helpers print this only after catching the distinct exception type from the
actual feature call. Generic or wrong typed exceptions take different failing
paths; none occurs. All three actual definitions—`create_password_hash`,
`verify_password`, `propose_password_upgrade`—unconditionally throw that marker as
their first statement. Input assembly and observer Controls are inert, and success
oracles are behind the absent result. The recorded markers therefore correspond
to missing behavior, not a successful native fake or a deliberate passing catch.

The remaining 51 failures follow those marker failures and are consistent
consequences of returning before any requested work:

| Source assertion lines | Additional failures | Actual missing work |
| --- | ---: | --- |
| 317, 318 | 8 | Hash passes/memory remain 0 |
| 426 | 2 | Verification calls remain 0 |
| 584, 637, 674 | 18 | Source reads remain 0 |
| 820 | 8 | Login checks remain 0 |
| 1075, 1095, 1097, 1114 | 15 | Initialization/allocation/lock/hash calls remain 0 |

These account for every failure location. No setup, load/link, missing executable,
unexpected exception, timeout, crash, skip or disabled-case failure appears.
Stderr contains only `Errors while running CTest`. All 37 raw process commands
invoke the actual reviewed test executable. Reviewed literal inputs/assertions,
real-default/error-only wrappers and ordinary uninverted CTest properties are
unchanged; no expected output was generated by the SUT or revised for this run.

## Preserved evidence and checks

Raw files remain in the integrator's ignored
`local-evidence/sessiontest001-red/`. Independently recomputed SHA256:

| File | SHA256 |
| --- | --- |
| first-red.stdout | `262ea46e9ab9ecdaef9db07dee8f38e7460d43e5f625a4968a3f4614b123d9ab` |
| first-red.stderr | `18e00877dc1071c85cd4f479d451bebeef8309022e46cf8c63b0bed5151984a4` |
| first-red.xml | `d733828a8c53d21a395df38a8e77b6cc7ea3f2388e8e9e21ee359cacaec59607` |
| first-red.json | `cb26303241ebc8e4d877f45b179d96dadd58f22b5c0b924a21ace8e4a75b99ac` |
| mechanical-inspection.json | `b4f6af10c1293abfcf36213a05ad586c82c300c2583cefc1a113d80f254e4971` |

The durable JSON embeds byte-equivalent parsed original/corrected receipts. The
original inspector falsely reported marker presence false because it expected
`status=run` and an obsolete diagnostic. Actual failed CTest records use
`status=fail`; the reviewed exact diagnostic is present in all 37. Both original
false result and raw evidence remain intact. The corrected inspection did not
rerun tests, amend logs or itself supply independent causal approval. My finding
above derives from the raw outputs and actual typed catches/throw bodies.

Actual checks were source/receipt reads, `git status/log/diff/hash-object/rev-parse`
and Python stdlib XML/JSON/hash comparisons. The final independent inspection is
retained in `/tmp/subathon-sessiontest001-red-attribution/audit.py` and
`audit.stdout`; `python3 /tmp/subathon-sessiontest001-red-attribution/audit.py`
passed all artifact, inventory and complete failure-location assertions. No build,
discovery, feature, marker, SDK, foundation, generator or injection body was run
by this reviewer. Inspection script SHA256 is
`24dacf93cab9fdc9cba346f4ceb698b4ecc85df17ba5d9dccb09d4f9a657d59d`;
its output SHA256 is
`a9a74ff05a29fe072349234143e6fee2f451d7be429785119da2f9a5262fc493`.
`python3 tools/check_project.py`, `python3 tools/progress.py render --check`
and `git diff --check` passed: 406 tasks, 122 decisions, links/presets/current HTML
and whitespace. No shared files changed.

Remote selected Ubuntu CI, workflow access/publication, current PR checks/reviews/
discussions/rules and normal merge remain OPEN and mandatory. This local red does
not finish SESSIONTEST-001's integration gate, make SESSION-001 dependency-ready,
release production implementation or verify account/login/store/admission, complete
secret cleanup, cost benchmarks, supported platforms or packaging.
