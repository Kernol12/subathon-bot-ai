# ACCT-004 independent internal service codec review

Reviewer: codec_author, 2026-11-04. This reviews OTHER AUTHOR service_author's
server/src/application_service.cpp at source commit722e11b, integrated unchanged
in **e4089bd1a823889fa94f67285a1257c91d4af375**. It does NOT review my command
codec implementation. Separate review worktree task/ACCT-004-service-codec-review;
only this evidence file is authored here.

**Verdict: request changes.** The unchanged35-slot service baseline passes on the
coordinator's reported integration, but the actual source and independent bounded
probes below establish missing contract behavior outside those assertions.

Authority: specification v0.45 sections3–5 and the independently approved
[concrete API](../contracts/application-service-api-v1.md), blob
bbd6cd834b8a959ccff16d63a638e0b77d9c6e4c, especially B/E/F. The completed
[original baseline](accttest-004-handoff.md) is preserved and was not replayed.

## Required corrections

1. **Bound complete intermediate values before materialization.** Read::str and
   reference allocate strings immediately after per-field bounds; read_clock and
   check_state push into vectors before complete framing or aggregate projection.
   StateMeaning's actual copies of basis/clock/policy/configuration/head and parsed
   balance/clock collections never receive their complete intermediate B/I check.
   read_policy, read_exact and audit/job parsing have the same allocation-first
   pattern. A two-pass borrowed scan/projection must check each complete semantic
   DTO's actual copies and combined vector elements before constructing strings,
   vectors or large integers. Component-specific ceilings and malformed framing
   remain separate. Current per-vector min(intermediate_items) is insufficient.

   Independently observed check_state on unchanged frozen historyA before-state:
   intermediate_items2 accepts one clock segment plus two parsed balances (3 actual
   vector items); intermediate_owned_bytes1 still accepts its nonempty string/
   clock data. This is an actual helper observation, not a process allocation
   measurement. The source establishes which allocations precede checks.

2. **Check all SAS1 observation/policy duplicates.** check_record checks audit
   policy revision against after.policy only. Its clock_doc comparisons serialize
   lineage/observation/UTC/segments, excluding the derived configuration/policy/head
   bindings. An advanced-before SAS1 policy can therefore disagree with the same
   command's after-state/audit policy. Independently changed ONLY the six policy
   bytes in the frozen historyA advanced-before annotation, policy→policX; all
   other fields remained unchanged and check_record accepted the contradiction.
   A newly advanced SAS1 must bind the same actual command policy/observation;
   retained-head reuse still follows its explicit complete-equality contract.
   Also audit source for clock group validity, elapsed checked sum and final UTC
   as known semantic constraints, independent of today's trusted port checks.

3. **Validate known retained job semantics, not only raw SAR1 duplication.**
   check_record scans opaque job fields, then reconstructs SAR1 from the same raw
   envelopes. It does not apply historical SAJ1 grammar/occurrence/component
   validation. Independently changed the frozen historyA job magic SAJ1→XAJ1 and
   changed the identical one duplicated job document in SAR1. check_record still
   accepted the malformed known job. Existing typed job identities and every other
   byte were preserved. A matching duplicated opaque document does not establish
   valid known semantics. Validate the retained recognized representation and
   ordinal/unit/request/components/captured action relationships without invoking
   current revaluation/actions or treating unavailable historical format as a
   current authority grant.

4. **Separate retained command field bounds from incoming public codec policy.**
   retained_command changes public aggregate limits only. It never explicitly
   bounds its retained envelope by storage_document_bytes and selected value_bytes,
   and keeps public command_bytes; public invalid_body is then indiscriminately
   mapped to invalid_known_document. Internal retained parsing requires the
   contract's storage/intermediate profile and typed configured-bound result_limit
   distinction. Later surrounding record encodings may incidentally reject an
   oversize record, after decoded allocation has already occurred; that does not
   satisfy the before-materialization boundary. This finding is a source trace,
   not a separate runtime fixture result.

## Actual bounded verification

Read the actual complete service TU and concrete contract sections, independently
from the service author. No production/test/fixture/build files were edited.
A scratch source `/tmp/acct004-internal-codec-probe.cpp` includes the ACTUAL reviewed
service TU plus the unchanged independent application_service_fixture header. It
loads frozen historyA as DATA, first checks the healthy complete record, then
performs the explicit mutations/limit selections above. Linked against existing
component libraries and compiled codec object, excluding the application-service
archive to avoid duplicate definitions. Existing fixture paths/Qt compiler flags
were taken from the coordinator's generated Ninja commands. GNU16.2.1/Qt6.12.

Compilation/link completed exit0 without diagnostics. Invoking:

```sh
/tmp/acct004-internal-codec-probe
```

Observed exit0 and these distinct outcomes:

```text
healthy record accepted
DEFECT: contradictory before-policy accepted
DEFECT: malformed SAJ1 job +matching duplicate accepted
DEFECT: semantic state has 3 vector items with ceiling2 accepted
DEFECT: semantic state strings/clock accepted with byte ceiling1
```

These are independent internal helper observations on actual source, not public
real-SQL acceptance, test-first regression baselines, a full suite rerun or an
approval. Additive public regression cases need separate specification-linked
review and reached behavior red before the associated production corrections.
Preserve all original assertions/oracles and the completed original red evidence.

## Next gate

Review the actual corrected integrated source and regression evidence before any
approval. Full source reviewer separately owns queue/completion/current binding/
numeric/view/CI review. No verdict here carries over unexamined corrections.
All provider/adapter/platform/full-release gates remain unverified.
