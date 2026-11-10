# Third-party inventory

No dependency binaries or complete upstream source trees are bundled. The selected
native profile's exact correction patches are tracked. System tools are not redistributed.

| Component | Intended use | State |
| --- | --- | --- |
| Qt >=6.12 | Server/desktop, Qt Test/Quick Test | Required for product; optional environment probe and Qt SQL writer acceptance |
| GoogleTest >=1.17 | Native acceptance tests | System/local source, or opt-in v1.18.0 commit fetch |
| Drogon / Trantor | HTTP/webhooks/WebSockets | Reviewed corrected native profile and distinct SDK/consumer build; native acceptance and production host remain pending |
| Boost.Multiprecision | Exact arithmetic | Bounded numeric primitives use installed headers; local1.92.0 and pinned Ubuntu1.83.0 CI tested; Boost Software License1.0; no sources/binaries bundled |
| libsodium | Passwords/credentials/backups | Selected; not integrated |
| QtKeychain | Linux remembered login | Selected; not integrated |
| SQLite via Qt SQL | Durable state | Bounded Qt SQL writer/reader tested with disposable files; application schema/recovery remain pending |
| React/TypeScript/Vite | Browser management/viewer | Selected; manifests await frontend foundation task |
| Vitest/Playwright | Browser tests | Selected; not installed/bundled |
| Noto Sans | Default fonts | Selected; no font files bundled yet |
| Caddy | Default deployment HTTPS | Selected; no deployment configuration yet |

GoogleTest optional source pin: 063de7e9578f82b369302001269680b4b1553359 (v1.18.0).
Source: https://github.com/google/googletest/releases/tag/v1.18.0
A release license/source manifest is still required; this table is an initial inventory.

The historical PR108 native WebSocket baseline used clean Drogon1.9.14 source
`3aae769cc405cf5abb9d477675c6430924f02c7c` and Trantor
`78e480f5e138acdc05f76c55a2716718cd846364`, without patches. The existing
[builder](tools/setup_drogon.py) selects shared Release libraries, OpenSSL and a
C++20 consumer/Drogon build; pinned upstream Trantor explicitly compiles as C++14.
That baseline prefix retained Drogon's MIT `LICENSE`, Trantor's BSD three-clause
`License`, and compiled ORM utility/libpqxx `orm_lib/COPYING`, even with database
backends disabled. No source or binary is committed or redistributed.

API-012's [corrected profile](dependencies/native-websocket-profile/manifest.json)
keeps those source pins and build options with the exact ordered patches in that
directory. Corrected SDKs retain the full Drogon MIT, Trantor BSD three-clause and
compiled ORM MIT license files, project GPL-3.0-or-later text and correction/source
provenance. Earlier review receipts that called Trantor MIT mislabel that license;
the actual selected `trantor/License` bytes and the BSD notice above govern. A
corrected prefix is distinct from the pristine SDK and has independently reviewed
source identity; its configuration manifest is not ABI, native or distribution proof.

[Ubuntu baseline inputs](tools/native_websocket_ubuntu.json) record the exact eight
JsonCpp/OpenSSL/UUID/zlib development/runtime archives, hashes, declared dependencies
and full installed copyright-file hashes. The workflow verifies archives before
installation and retains the package copyright files. The runner's libc/compiler
runtime remains a recorded host dependency. Local Arch loaded-library evidence and
Ubuntu CI are different proof levels; neither establishes release packaging,
all provider modules, complete ABI compatibility or license/source obligations for
future distribution.

Qt writer baseline CI selects official Qtbase 6.12.0 Linux x86-64 and its ICU73
runtime archives, with SHA-256 verification in
[the workflow](.github/workflows/sqlite-writer.yml). No Qt/ICU binary is committed or
redistributed by this repository. The downloaded archives retain their license
material; release packaging must provide the complete applicable Qt/ICU/SQLite
license and source/offer obligations before any distribution. The baseline uses
only Core, Sql and Test, without changing the product minimum of Qt6.12.

## Admitted isolated schema-validation developer dependencies

The existing artifact/license review and installed-library receipts remain historical
evidence; TESTCLEAN-001 reuses those exact dependencies without selecting new versions.
No wheel is committed or redistributed, and these are not server/client runtime
dependencies. Full admitted Linux x86-64 CPython3.10/3.12/3.14 wheel/license-file hashes
are in [the manifest](tools/schema_validation/wheels.json) and
[hash lock](tools/schema_validation/requirements.lock). Versions/licenses below are
wheel metadata declarations, not a release-compliance or security certification.

| Package | Pinned version | Declared license |
| --- | --- | --- |
| annotated-types | 0.8.0 | MIT |
| attrs | 26.1.0 | MIT |
| jsonschema | 4.26.0 | MIT |
| jsonschema-path | 0.5.0 | Apache-2.0 |
| jsonschema-specifications | 2025.9.1 | MIT |
| lazy-object-proxy | 1.12.0 | BSD-2-Clause |
| openapi-schema-validator | 0.9.0 | BSD-3-Clause |
| openapi-spec-validator | 0.9.0 | Apache-2.0 |
| pathable | 0.6.0 | Apache-2.0 |
| pydantic | 2.14.3 | MIT |
| pydantic-core | 2.50.4 | MIT |
| pydantic-settings | 2.15.0 | MIT |
| python-dotenv | 1.2.4 | BSD-3-Clause |
| PyYAML | 6.0.3 | MIT |
| referencing | 0.37.0 | MIT |
| rfc3339-validator | 0.1.4 | MIT |
| rpds-py | 0.30.0 | MIT |
| six | 1.17.0 | MIT |
| typing-extensions | 4.16.0 | PSF-2.0 |
| typing-inspection | 0.4.5 | MIT |

Future dependency redistribution must retain the actual bundled license material
and satisfy the full release manifest/source obligations. Existing standard-library
progress/repository tools keep their current independent dependency environment.
