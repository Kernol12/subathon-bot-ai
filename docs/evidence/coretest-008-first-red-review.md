# CORETEST-008 independent FIRST-red attribution

**APPROVED — actual intended missing-behavior red only.** Reviewer
`service_reviewer`, different from test author `service_author`, 2026-11-04.
Inspected actual `ee291be4c7a17d3b5693c722c7ac79cb9f7387bd` in isolated
`task/CORETEST-008-first-red-review`. Only this note changes.

The [native receipt](coretest-008-first-red.json) and original return-code file
retain FIRST on clean `2dce71d193e4945efe8b43a5ca91276675b2c778`,
`20:46:32.706042Z`–`20:46:32.752604Z`:

```sh
ctest --preset timer-editorial-debug -L '^timer-editorial$' --no-tests=error --output-on-failure --output-junit /tmp/subathon-coretest008-first-red/first-red.xml
```

**Direct native CTest exit 8; 27 executed / 27 failed / 0 disabled / 0 skipped.**
[JUnit](coretest-008-first-red.xml) parses directly, timestamp 21:46:32 CET
(20:46:32Z). Its exact unique names are the frozen 27 source/JSON names with the
reviewed `timer-editorial.` prefix. Every case has fail status, one JUnit failure,
the exact four labels, complete filtered GTest execution and zero passing tests.
This is missing-behavior baseline evidence, not passing timer acceptance.

## All actual failure events and row multiplicities

Inspected every raw/JUnit GTest failure block and its source context:

| Actual source location | Count | Attribution |
| --- | ---: | --- |
| Line 179 | 88 | Proposal helper catches the exact module marker and reports Expected proposal |
| Line 191 | 76 | Error helper reports the exact marker as a wrong exception, never successful TimerEditorialFailure |
| Line 795 | 1 | Ownership call catches the same primary marker before retaining a result |
| Line 798 | 1 | Consequential retained.has_value() ASSERT safety gate, not another marker |
| Total | 166 | 165 primary missing-behavior markers plus one expected consequence |

All 27 groups contain primary marker evidence. All 165 primary blocks contain
exactly `Timer editorial commands are intentionally unimplemented; review tests first`; the ownership consequence contains no marker and correctly reports false
retained presence. There is no other failure category. The sole frozen feature
body immediately throws TimerEditorialCommandsNotImplemented, so these primary
records attribute 165 observed projector invocations to explicit missing behavior.

Primary counts, in the frozen JSON/source group order, are:

```text
4,2,8,2,4,4,6,4,6,9,6,4,4,1,3,3,2,1,16,15,10,4,3,9,9,13,13
```

Every group matches its independently inspected source row/loop multiplicity.
Repeated-label traces are honest: first-Start ED28 has 2 and ED29 has 2, plus one
Active and one Finished check; deadline-edit ED30 has 2 and ED31 has 4, plus Grace,
wait and future-deadline rows once each. These are repetitions of the label in
known source order, not additional unique CTest names or proof of serialized raw
input values. Structural groups reach 16 snapshot, 15 command/access and 9 preview
refusals; observation/refusal/resource/option groups likewise match their source.
Nonfatal helpers permit later independent rows to run despite earlier markers.

No unrelated setup, dependency, SDK, fixture-construction, timeout, crash, skip or
inverted-result failure is present. Native stderr is only `Errors while running
CTest`. Proposal comparisons after a throw have not executed. In the ownership
case, emplace throws before input mutation, then the retained-value ASSERT ends
the test safely; later getters/copy/move checks did not run. Those consequences
neither invalidate red nor establish lifetime or other successful implementation.

## Artifact integrity and prerequisite order

| Independently rehashed native artifact | Bytes | SHA256 |
| --- | ---: | --- |
| [stdout bytes](coretest-008-first-red.raw.log) | 112892 | c0ba154cd8a17110cf873ea1a56d0fa4e019015e9e6335f0d0f741263ee5692c |
| [stderr bytes](coretest-008-first-red.stderr.log) | 27 | 18e00877dc1071c85cd4f479d451bebeef8309022e46cf8c63b0bed5151984a4 |
| Native JUnit | 113816 | 1f9ea9c5b28e013cb5686a388f3c57385b110a3a210e3e64c2ed7f2498fb7f8c |
| Original returncode.txt (`8` plus LF) | 2 | aa67a169b0bba217aa0aa88a65346920c84c42447c36ba5f7ea65f422c1fe5d8 |

Original `/tmp/subathon-coretest008-first-red` artifacts match every receipt
size/hash and committed native copy. The [readable projection](coretest-008-first-red.txt)
is exactly raw UTF-8 decoding with backslashreplace. No collector loss, replay or
supplemental execution is needed. Receipt blob is
`7643da97dac461a5f582e33e9d7dd9e20d967b3e`.

Actual full-test approval `bd7f5e3e` and current-main/harness approval `166c3a7a`,
imported as `2dce71d`, precede FIRST. Test
`3b0c80411e73736925ed2491e12741cbb03aedeb`, inventory
`1a4fb38d1a28b768e138590dca46e8b3dc5e0abc`, header
`92281931bd09e9130b3266b5465f6cff6d5f81de`, stub TU
`651c68b3db599849c3e539c908e6dd57f633c0bf` and logical/API/spec blobs
`f4a4c3ce`/`a2ac5a2c`/`81406806` are unchanged at test review, selected CI, FIRST
and current evidence. All six relevant config/label/workflow blobs are identical
between selected CI, FIRST and this candidate. Only native evidence was added
post-FIRST; no baseline assertion/oracle/input or implementation changed.

Read retained selected Ubuntu [41908880067](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41908880067)
at `732f73e2b0f4e660b7592ec81699ebca682bd390`: exact 27/74/101 guard and all 74 old
green checks, completing 20:41:09Z before FIRST. The pinned GTest-fetch harness
was already separately approved; its green lane did not invoke editorial bodies.
Log SHA256 `576c0dcc2bcd63e468b4c3bf1bcb6c686a38405efe6480a991b1cfc0bd82d5c7`.
No SDK or already-completed source/test/foundation gate was repeated here.

Binary-safe Python XML/hash/name/trace/source comparisons,
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` pass. No build, CTest, feature/marker, test/native, SDK or
foundation body ran in this review. Approval is actual attribution only, not
future metadata, implementation, CI or merge. Host rights/UI warning/queue/clock/
CAS, first Start/reopen, mixed/global review and all release gates remain required.
