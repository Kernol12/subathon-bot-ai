# SPIKE-001 disposable framework experiments

## Synthetic queue subset

This standalone probe is not linked into the server or core. Its transport is four
synthetic C++ threads, not Drogon. No sockets, credentials or production data are used.
The authoritative queue and SQLite writer share the QCoreApplication thread.

Bounded experiment criteria (specification v0.45 §2/18, SERVER-01, DB-01):

- Discover real Drogon separately; missing discovery does not validate its lifecycle.
- Admit 64 distinct synthetic commands concurrently from four non-writer threads.
- Execute every admitted command only in the owning Qt thread and create, use and
  close the one Qt SQL connection in that same thread.
- Confirm the disposable file database actually uses WAL and synchronous=FULL.
- Close admission, reject a late command, drain admitted commands, verify all 64
  distinct rows and their sum, and join producers before destroying queue/database.
- Preserve a failing exit if a command, owner check, SQL operation or drain fails.

Reproduce from the repository root:

```sh
cmake -S experiments/spike-001 -B build/spike-001-dependency -G Ninja -DSPIKE_REQUIRE_DROGON=ON
# Only when real Drogon is unavailable: the following validates the synthetic subset.
cmake -S experiments/spike-001 -B build/spike-001 -G Ninja
cmake --build build/spike-001
ctest --test-dir build/spike-001 --output-on-failure -V
```

The first command is a dependency check, not an intended failing acceptance test.
A green synthetic run does not validate HTTP/WebSocket callbacks, Drogon run/quit,
startup failure, connection cancellation, response-loop affinity, signal handling,
backpressure, reader isolation, crash recovery, backup, or release performance.
The real bridge suite below supplies separate evidence; this synthetic subset alone
does not complete SPIKE-001. Review this
spike independently or discard it before reusing any of its code in production.


## Pinned real-framework experiment

The [real bridge contract](bridge_contract.md) and [external six-case harness](test_bridge.py)
extend this probe with actual loopback Drogon HTTP/WebSocket traffic. It remains
outside the product build. See [dependency evidence](../../docs/evidence/spike-001-dependencies.md)
and [review/red/final results](../../docs/evidence/spike-001-bridge-validation.md).

From the persistent primary checkout, explicitly prepare the dependency once:

```sh
python3 tools/setup_drogon.py
cmake -S experiments/spike-001 -B build/spike-001-real -G Ninja -DSPIKE_BUILD_REAL_BRIDGE=ON -DCMAKE_PREFIX_PATH="$PWD/build/deps/drogon-1.9.14"
cmake --build build/spike-001-real
ctest --test-dir build/spike-001-real --output-on-failure -V
```

When running from a temporary worktree, pass the primary checkout's build/deps as
setup `--root` and its installed drogon-1.9.14 directory as CMAKE_PREFIX_PATH. Builds
and temporary databases stay local/ignored; ordinary configuration never downloads
sources. The opt-in real probe currently targets Linux and requires local loopback
socket permission. It binds 127.0.0.1 only and uses disposable synthetic command IDs.
The CTest run contains the retained synthetic queue case and the real six-case suite.

The probe opens one command per WebSocket connection and initiates a normal close
after each reply. This explicitly bounds the experiment after discovering a close-
frame limitation in the pinned framework's received-Close path. General persistent
WebSocket/client-initiated close behavior remains a later investigation; the spike
is not evidence of full protocol conformance or production reuse approval. Further
untested cases include uncooperative/idle clients, partial-frame shutdown, startup-
time signals, Caddy/TLS, backpressure, crash recovery and release/platform behavior.
