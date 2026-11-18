# Account capture ABI — stage-one proposal 1

SESSION-011 #591. Author session SESSION011#591, gpt-6.1-sol through Codex/T3.
This declaration proposal supplements, and does not replace or weaken, the FULL
[account ingress R2](account-ingress-v1.md) SESSIONCONTRACT-005 and its frozen
imports. That document's budgets, policies, priorities, erasure and pre-log
requirements remain normative, including clauses absent from the examples below.
No runtime/artifact admission, review, test execution or behavior is claimed.

The concrete header is `server/include/subathon/server/account_capture.hpp`.
It imports the admitted API-016 foundation. The NI source uses no SDK additions.
The later separately reviewed artifact must implement this first-party boundary;
it must never define competing first-party symbols in a vendor namespace.

1. `install` runs on the actual foundation's creating thread, cold, exactly once,
   before listener registration or any reads. Null owner/callback is invalid_config;
   wrong thread is wrong_thread; repeated/late/conflicting RawHooks installation is
   invalid_state. Unknown/incompatible actual artifact, ABI or profile is unavailable.
   Errors have no partial installation or listener/configuration side effects.
   Success requires actual selected-source binding, no user-populated fact setter.
2. The native issuer creates exactly one move-only lease per complete admitted
   account request. Private state is precharged before accepting input, with no
   per-request general heap fallback. The implementation/source inventory must
   prove its placement in existing metadata BEFORE allocation; no extra quota is
   granted here. A public request, index, connection value or identical hash cannot
   manufacture a lease. Sequence/incarnation exhaustion closes; no wrap/reuse.
3. The callback runs on IO0 with the actual request's native listener/connection,
   sequence, numeric peer endpoint and artifact/profile digest. These observations
   establish neither trusted proxy isolation nor TLS, identity, policy or permission.
   This plain-backend ABI deliberately has no inferred direct-TLS boolean. Unadmitted
   direct TLS remains unavailable. SESSION-010 owns the actual policy/source binding.
4. `with_capture` visits once synchronously/nonreentrantly. Nested access is reentrant;
   off-thread access is wrong_thread; moved-from, already visited or completed scope
   is revoked; null visitor is invalid_config. A visitor exception is callback_failed
   and closes with erasure after unwinding. Invalid calls invoke no visitor. Bytes
   remain immutable and valid THROUGH the enclosing capture callback return, even
   after with_capture returns. No retained lease extends that scope. Destruction on
   another ordinary thread only uses the foundation's bounded drop path.
5. Every original application-listener field occurrence is visible, including empty,
   unrelated, mixed-case and repeated fields, with only outer SP/HTAB stripped.
   Same-name relative order is preserved; no maps/cookie jar/comma merging. Method,
   escaped target and exact body remain borrowed, bounded native-owned bytes.
   No stock header/path/query/JSON/Expect/OPTIONS logging or serializers can receive
   them. Public reference envelopes/routes are later owning integration, not supplied
   by this capability. Native Upgrade projection is NEVER a capture lease.
6. Install selected libsodium secure erasure through the existing RawHooks seam;
   never treat generic memset/clear/free as proof. Erase fields/consumed receive only
   after both route scan and synchronous transport finish; method/target/body as soon
   as their owner finishes and always before scope release. Error/EOF/timeout/stop,
   cancellation/exception, compaction and owner teardown erase actual attributed
   regions before reuse. Preserve bounded later pipelined bytes until their own
   admission; erase them on close. Wipe failure quarantines/closes, never successful
   reuse. Lease revocation precedes release and cannot be undone by retaining a handle.
7. All §3 native bounds remain: receive65536+prefix8, pre-read free8192, raw head40960,
   fields64, individual name+value8192, H<=32768, method16, target2048; reference
   semantic header/total/body4096. The 104472 native payload sum excludes exactly64
   fixed records and lease facts. API-016's metadata/control/native/global inventory
   remains charged additionally as already documented, not replaced by this sum.
   No new body/head clone, map, pool, route, launcher, process, TLS slab or SDK recipe
   is authorized by this proposal. Reference pre-body priority remains header431,
   body413, aggregate431, syntax400. F05P remains a budget probe, not account support.

## Independent examples and owning tests

| ID | Input/condition | Account-capture oracle | Later owning oracle |
| --- | --- | --- | --- |
| C01 | Native A, two identical Authorization occurrences (237 bytes) | Both occurrences, exact values, no crypto | SESSION-010 transport401 |
| C02 | Native B, Origin/oRiGiN (241 bytes) | Both occurrences/case retained | SESSION-010403 |
| C03 | Native C, marker/x-marker (199 bytes) | Both occurrences/case retained | SESSION-010403 |
| C04 | Native D, two Cookie occurrences (277 bytes) | Both full cookie values, no coalescing | SESSION-010401 |
| C05 | A without second Authorization (149 bytes) | One live actual native lease, same immutable bytes through callback return | SESSION-010 digestZ and original generation; no current authentication claim |
| C06 | Empty X-Empty; Via1.0 edge-a, vIa1.1 edge-b | Empty retained; both Via values in order | Deployment007 adds third Via, no Via trust |
| C07 | Field65/raw40961/individual8193 | Owning431/close before excess copy | Foundation baseline remains unchanged |
| C08 | Reference header4097 + CL4097 + malformed name |431 before body413 before syntax400 | Exact reference envelope later API010 |
| C09 | Nested/off-thread/completed/moved lease | Refusal; zero visitor calls | No policy or supplier lookup |
| C10 | Callback throws or peer closes/timeout/stop, with later bytes pending | Actual secure wipe/revocation, no raw reuse | No Qt raw-token retention |
| C11 | Canaries in Authorization/Cookie/Expect/target/query/other/body/forwarding | No raw/fragment/encoded bytes in reachable sinks from launch to shutdown | Proxy sinks separately DEPLOYMENT007/008 |
| C12 | Ordinary or native Upgrade with synthetic Cookie | No account capture on native projection; account profile refuses Upgrade | Unchanged native44 compatibility |

`tests/account_capture/capture_assertions.hpp` authors literal capture assertions
for actual callback use, not a fake source or an executable harness. `oracles.hpp`
contains independently literal heads, no production parser. `cold_tests.cpp` uses
GoogleTest and actual cold Foundation creation to require installation success;
NI must fail an ordinary assertion, never be an expected-success outcome.
There is no admitted registration or native driver for these additions yet.
Review/source-only assertion code does not establish any row as observed red/pass.
The author packet identifies the exact fixture/log/erasure binding gaps and options.
Independent tier A test review, admission, reviewed executable release and intended
behavior red precede implementation. Existing frozen assertions remain untouched.

## Stage-one continuation 02: exact identity and approved private observations

The digest fields have these concrete meanings: artifact_sha256 is SHA-256 of
exact selected SDK dependency-build.json bytes; profile_sha256 is SHA-256 of its
installed share/subathon-bounded-http-foundation/manifest.json bytes and MUST equal
that seal's profile_manifest_sha256 field. This identifies the actual native SDK
profile, not a trusted proxy/current-policy generation or a credential. The native
issuer must bind its actual installation/connection to those identities. Independent
expected TEST DATA hashes those read-only selected files at root-owned configure,
checks their agreement and configures selected_identity.hpp.in. No SUT output is
used as an expected value. A changed selected successor gets newly bound expected
data and fresh admission/review; retained c4d is not account support or a reusable
receipt. Existing listener/route/limits/native thread bindings remain mandatory.

The approved E2 declaration is PRIVATE server/src/account_capture_observer.hpp,
not a public or installed SDK header. NI installs nothing. Future account erasure
code calls it only around its OWN actual selected sodium_memzero invocation, with
before_sodium and after_sodium notifications sharing one checked call ordinal1..8192
and actual RawTag/region/length. Region distinguishes fields records from arena.
Before may borrow nonzero bytes; after borrows the SAME region after real sodium
returns. No retained payload/byte digest, pointer-to-payload, callback queue or
user-supplied fact/zeroization setter. Callback is noexcept/IO0/synchronous and may
not reenter. Registration is cold/creator/once; null invalid_config, late/repeated
invalid_state, wrong thread wrong_thread. No install replacement/RawHooks chaining.
Before overflow/false must NOT skip the actual secure wipe; false at either phase
quarantines/closes with wipe_failed and never grants reuse/clean finish. Call ordinal
overflow similarly fails closed, with actual erasure still attempted. Callbacks must
not throw; violating noexcept is process-terminal, no fabricated successful cleanup.
Uninstalled observer adds no payload scan/callback/allocation or changed erasure;
only a disabled branch. Registration lifetime covers actual stop/drain/IO joins;
no callbacks after finish. Exact placement/layout and all phase/exit tests require
tier A review before any behavior implementation. Existing production quotas do
not grow; metadata placement/pins must be proved before allocation in stage two.

The ordinary fixture binds the EXISTING Trantor default output callback at TRACE
before profile construction and scans with a <=2MiB stream ceiling, no raw payload
retention. An actual Logger canary positive precedes the clean actual-case target;
response wire is independently scanned. Indexed/file/plugin/direct fd/stdout/stderr
and pre-main coverage require the written sink audit; no launcher/interceptor or
claim of unseen bytes is authorized. Whole-account log suppression still requires
source review of actual branches, not a Fatal filter or scanner absence alone.
Standing unchanged admission now covers authorized Qt/GTest binaries and their
OWN actual closure with Session-symbol applicability; no new helper/mode is added.
