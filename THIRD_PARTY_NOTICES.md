# Third-party inventory

No dependency binaries or sources are bundled. System tools are not redistributed.

| Component | Intended use | State |
| --- | --- | --- |
| Qt >=6.12 | Server/desktop, Qt Test/Quick Test | Required for product; optional environment probe and Qt SQL writer baseline |
| GoogleTest >=1.17 | Native acceptance tests | System/local source, or opt-in v1.18.0 commit fetch |
| Drogon | HTTP/webhooks/WebSockets | Selected architecture; not integrated |
| Boost.Multiprecision | Exact arithmetic | Bounded numeric primitives use installed headers; local1.92.0 and pinned Ubuntu1.83.0 CI tested; Boost Software License1.0; no sources/binaries bundled |
| libsodium | Passwords/credentials/backups | Selected; not integrated |
| QtKeychain | Linux remembered login | Selected; not integrated |
| SQLite via Qt SQL | Durable state | Real disposable-file writer baseline; production writer not yet implemented |
| React/TypeScript/Vite | Browser management/viewer | Selected; manifests await frontend foundation task |
| Vitest/Playwright | Browser tests | Selected; not installed/bundled |
| Noto Sans | Default fonts | Selected; no font files bundled yet |
| Caddy | Default deployment HTTPS | Selected; no deployment configuration yet |

GoogleTest optional source pin: 063de7e9578f82b369302001269680b4b1553359 (v1.18.0).
Source: https://github.com/google/googletest/releases/tag/v1.18.0
A release license/source manifest is still required; this table is an initial inventory.

Qt writer baseline CI selects official Qtbase 6.12.0 Linux x86-64 and its ICU73
runtime archives, with SHA-256 verification in
[the workflow](.github/workflows/sqlite-writer.yml). No Qt/ICU binary is committed or
redistributed by this repository. The downloaded archives retain their license
material; release packaging must provide the complete applicable Qt/ICU/SQLite
license and source/offer obligations before any distribution. The baseline uses
only Core, Sql and Test, without changing the product minimum of Qt6.12.
