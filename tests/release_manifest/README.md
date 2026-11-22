# RELEASE-001 Stage 1

The opt-in test file calls `tools.release_manifest.assemble` with modeled
owner declarations and independently observed fixture frames. Frozen contract
r3 pins source bytes to `f63a418b9bb9248a9cc9b0e438139ef2c4b47466`. The M1
and M10 goldens are canonical UTF-8 with one LF. The frame, inventory, manifest
and finding expectations apply to the selected fixture scope. The owner
declarations apply only to the modeled artifacts.

After the Stage 1 review and applicable input qualification, select the suite
explicitly with `python3 tests/release_manifest/test_release_manifest.py`.
The individual selectors are the `ReleaseManifestTests.test_*` methods. A
required fixture input failure is a setup error. The assembly seam raises
`NotImplementedError` when reached.

## Inputs supplied by the fixture owner

All external inputs are existing readable data. The fixture performs no
download, install, binary execution or release packaging.

- M3, M4, M7 and M9 use `RELEASE_001_SDK` for the exact successor SDK
  directory and `RELEASE_001_SDK_SEAL` plus its `_SHA256` for the existing
  builder's seal. M4, M7 and M9 also use `RELEASE_001_SDK_LINKS` plus its
  `_SHA256` for the separately sealed passive closure with exact DSO roots,
  resolved edges, lengths and hashes. This fixture describes the local
  successor projection with four file aliases and the copied
  `share/licenses/local/` notice set. The builder seal follows links. The
  fixture separately compares the actual raw link values and binds them to
  the modeled artifact inventory.
- M5 requires `RELEASE_001_PASSWORD_RECEIPT` and its independent
  `_SHA256`. The owner must seal the exact receipt for
  `server_password_hashing_acceptance`, including `target`,
  `complete_link_closure`, `output`, `recipe`, `project_sources`,
  `static` and `external`. Identities use canonical decimal `bytes` and
  lowercase `sha256`. The `static` entries `sodium` and `gtest`
  provide `archives` by filename, `notice`, `source`, `license` and
  `dependencies`. The GTest entry also provides `version`, `origin`
  and `revision` from the selected build. The `external` entries provide
  `revision`, `files` and `dependencies` for every resolved external
  prerequisite. Place the observed output, notice files, linked archives and
  sodium source tarball at these paths relative to the sealed receipt.

  | Data | Path |
  | --- | --- |
  | Output binary | `bin/password-test` |
  | Copied notices | `legal/sodium.LICENSE`, `legal/gtest.LICENSE` |
  | Static archives | `archives/sodium/libsodium.a`, `archives/gtest/<receipt archive name>` |
  | Exact source archive | `archives/sodium/libsodium-1.0.22.tar.gz` |

- M6 requires `RELEASE_001_NOTO_OFL` and its independent `_SHA256` for the
  exact `OFL.txt` from Noto Sans v2.015 revision
  `c4a321e123e4d4ff315f57f4e0adf294fe3a95be`. That text is source/notice
  evidence. No font payload belongs in this case.
- The M10 wheel variant requires `RELEASE_001_WHEEL` for the separately
  admitted `annotated_types-0.8.0-py3-none-any.whl`, 13,427 bytes and
  SHA-256 `f072f4d804ea359e4eaf198b1af7a8b0943881a87f31bb764f8bf219bb9419e0`.
  The owner must qualify the exact wheel as an acquisition input before the
  variant runs. The tracked lock and wheel metadata establish its acquisition
  expectation. The actual file supplies the newly observed member.
