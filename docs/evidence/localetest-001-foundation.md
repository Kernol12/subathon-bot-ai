# LOCALETEST-001 selected Qt foundation

Coordinator /root, 2026-11-04. This records only the reviewed independent existing-Qt
foundation, not generator acceptance or the first project failure.

## Reviewed revision and command

Actual narrow fixture/probe/harness review2968f33/importacfb4af approved the seven
literal messages, complete four golden files, probe blobf033074d and Qt registration
9ff75621 at e1779a3. The full Python test review remains separate. Earlier findings,
additive plural-percent fixture and fresh missing-English check are retained in
[test review](localetest-001-test-review.md); all six original messages/forms remain.

The already configured/built translation-catalog-debug uses verified official
Qt6.12.0 at /tmp/subathon-locale-qttools-6.12.0/prefix, not the host Qt tool.
[Tool preflight](localetest-001-tool-preflight.md) records all four archive pins and
runtime closure. Actual probe ldd resolves QtCore/ICU73 inside that selected prefix.
No dependency/compiler/setup failure was used as red.

```sh
ctest --preset translation-catalog-debug -R '^scaffold.translation_catalog_environment$' --verbose
```

Actual exit0,1/1 CTest passed,0.02s. The probe completed100 assertions, including
literal raw ordinary/plural witness forms, literal percent sequences at witnesses
1/2, braces, Unicode/CR/LF/TAB, missing selection/English load, actual language
identities, process/file checks and exact byte comparison. This count includes
infrastructure assertions; it is not100 different raw lookup scenarios. Real
lrelease compiled independent TS in TWO fresh temporary directories. Both repeated
QM byte comparisons passed with the same exact tool/options:

- en SHA256 c791a34309302ff7c157c6d347bc872fc99e98eb583ae69a182a2d0cb3cc4d17
- de SHA256 bee9ef8e5810b97e4bfc86ac6c9d533f4ecb84c78013d287239801e3c61706ab

Ignored raw log local-evidence/localetest001-ci/foundation-e1779a3.log has SHA256
bd8f98f621c5f152f6fab5b9754dca83beaff2d6e90d048d2c901c1bc0a33224.
This proves same-tool local reproduction only, not cross-version/platform identity.
No new generator, renderer, main, script or missing-feature marker was invoked.

## Automatic CI and next gate

Root e1779a3 always registers all43 authored feature methods. Actual AST/inventory/
CTest discovery agrees on43 unique names/48 total CTests without feature execution.
CI guard6fdd985 additionally rejects duplicate definitions, disabled/failure-inverted
cases and missing baseline labels; the independent reviewer inspected this actual
revision. Ordinary workflow builds the probe, verifies exact discovery and runs the
mandatory Qt foundation plus existing checks with explicit -LE intentional-red.
Intentional feature red is separate and never inverted, skipped or treated as green.

Actual [CI41797849656](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41797849656)
at cafe4adc093d1d39a51e0db97effcfd8381207ec completed SUCCESS. Fresh Ubuntu24.04,
Python3.12.3 and the four verified official archives passed version/runtime closure,
build/syntax/inert imports, exact43 enabled/uninverted discovery and all five
non-intentional-red CTests. The Qt foundation completed100 assertions and BOTH
same-tool binary hashes above also matched the root observations. Ignored log
local-evidence/localetest001-ci/foundation-41797849656.log SHA256
 dae3c4196ee47e1675a02e3540aa631c489d59e6b1261b22383bd20b91a25134.

This actual CI ran before two additive Python-only semantic oracle corrections
(T1 global cross-message stage priority, T2 Unicode separators). It is a real
foundation/CI check, not approval/execution of a future corrected feature suite. Full actual test review and same-environment green setup must
precede FIRST intended project marker red. Missing setup is not red. Production
LOCALE-001 remains unimplemented; formatting/preferences/runtime/package/platform
and full release gates remain separate. The earlier scratch56-check experiment is
neither substituted for this foundation nor promoted into a test oracle.
