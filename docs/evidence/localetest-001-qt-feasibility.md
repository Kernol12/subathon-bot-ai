# LOCALETEST-001 — actual existing-Qt feasibility findings

Investigator codec_author, root integration,2026-11-04. This is a scratch probe of
existing third-party Qt tools and public QTranslator, not project generator code,
a reviewed baseline test suite, feature acceptance or an intended missing-feature
red. No project catalog/schema/API/stub/test/production behavior was introduced.
Expected strings and TS were manually written from contract TC01/02/06/07/14.

## Actual rejected mapping

The document approved in534d4d0, contract3a249a58, inferred that top-level TS
message elements were usable from the XSD and empty-context QM ID rule. The actual
selected official lrelease6.12.0 REJECTED that literal TS with exit1:
`Unexpected tag <message>` at messages_en.ts line3. This was a genuine third-party
format finding, not missing dependency/compilation failure or a valid project red.
Original TS, commands and failure log remain preserved; failure-log SHA256
2f920117 is pinned in the [separate reopened review](localetest-001-contract-review.md).

The independently inspected Qt6.12 reader
[ts.cpp](https://github.com/qt/qttools/blob/v6.12.0/src/linguist/shared/ts.cpp)
accepts messages only within a context; its root reader handles context/dependencies/
extras. The XSD's broader allowance is not actual reader support. The QM writer's
empty-context ID rule remains correct; it requires an EMPTY context name, not the
absence of the XML context wrapper. No source-catalog semantics or product scope
must change to correct this serialization.

## Actual supported scratch probe

Only the scratch TS structure was changed to one explicit
`<context><name></name>...messages...</context>`. The actual verified official tool
then compiled en/de in TWO fresh directories with -fail-on-unfinished. Both fresh
pairs were byte-identical:

- en QM SHA2567922fd96e3294688cc22d102ea12859b5c0d2622ff9d93bf1b41497c625a8543
- de QM SHA256918a90bd8f71dc2b4391cfd7c89fdd895f769f699fc2074da4c9f71ba5271442

A tiny scratch C++ program linked to the same official Qt Core/ICU prefix produced
56 literal checks PASS: stable IDs/empty context, direct raw lookup for ordinary
and witnesses1/2, literal percent/braces, CR/LF/TAB/umlauts/combining text/emoji,
selector-only forms, missing-message null and an explicit English fallback probe.
This observes raw third-party lookup/fallback operations, not a production named
formatter, locale preference/runtime UI or project catalog generator.
The wrapper-only scratch change was NOT a check of the future canonical indentation
or independent project goldens; the author/reviewer must fix and review those.

Selected lrelease binary SHA256
2defbad7e991c031635a812cdbc4054e1406f3a8a20fac5386ed89102f1e061c;
actual version6.12.0 and ldd verify the pinned Qt/ICU prefix. The four official archive
hashes/runtime setup remain in [tool preflight](localetest-001-tool-preflight.md).
Scratch success-log SHA256
a113de3ebb640718cc2748f3371b5be1c65e9fb709ba6a5878d8cf5fc8de092f.
Full commands, TS/C++ sources, logs, binary/QM hashes and the evidence manifest are
preserved in ignored local-evidence/localetest001-qt-feasibility, copied from
/tmp/subathon-locale-tool-probe-ihwtzipp. Manifest SHA256
6a91c05ce6725a2b282165d7529280fe2872675a7a69608769c51dadc6f26d7c.
The different reviewer inspected the real files/hashes/source without replaying it.

## Gate consequence

R4 in [the document review](localetest-001-contract-review.md) reopens ONLY the
unsupported TS serialization approval. The original verdict and rejection history
remain, rather than being rewritten as if the first mapping passed. Author is
correcting the canonical empty-context wrapper/indentation and complete TC01 XML;
that actual new revision needs renewed independent document approval before the
API/test baseline can rely on it. There are no existing project assertions/fixtures
to weaken. All later interface/harness/independent-fixture/test-review/selected-CI
foundation/intended-red/implementation/code-review gates remain unfinished.
