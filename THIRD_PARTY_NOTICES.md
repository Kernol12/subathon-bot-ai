# Third-party inventory

No dependency binaries or sources are bundled. System tools are not redistributed.

| Component | Intended use | State |
| --- | --- | --- |
| Qt >=6.12 | Server/desktop, Qt Test/Quick Test | Required for product; optional environment probe only in scaffold |
| GoogleTest >=1.17 | Native acceptance tests | System/local source, or opt-in v1.18.0 commit fetch |
| Drogon | HTTP/webhooks/WebSockets | Selected architecture; not integrated |
| Boost.Multiprecision | Exact arithmetic | Selected; not integrated |
| libsodium | Passwords/credentials/backups | Selected; not integrated |
| QtKeychain | Linux remembered login | Selected; not integrated |
| SQLite via Qt SQL | Durable state | Selected; not integrated |
| React/TypeScript/Vite | Browser management/viewer | Selected; manifests await frontend foundation task |
| Vitest/Playwright | Browser tests | Selected; not installed/bundled |
| Noto Sans | Default fonts | Selected; no font files bundled yet |
| Caddy | Default deployment HTTPS | Selected; no deployment configuration yet |

GoogleTest optional source pin: 063de7e9578f82b369302001269680b4b1553359 (v1.18.0).
Source: https://github.com/google/googletest/releases/tag/v1.18.0
A release license/source manifest is still required; this table is an initial inventory.
