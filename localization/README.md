# Shared translation source

Edit messages.json. Stable IDs, English/German text, named parameter declarations
and plural forms follow the reviewed catalog contract in docs/contracts/.
The generated/ Qt TS and browser JSON files are generated artifacts, not a second
source. Do not hand-edit them or use them as test expectations.

With the selected verified Qt6.12.0 prefix discoverable, configure
translation-catalog-debug. To update tracked text after editing the source:

```sh
cmake --build --preset translation-catalog-debug --target translation_catalog_update
```

A normal build generates actual QM and temporary text under the selected preset's build directory,
then compares the tracked generated text without overwriting it. Stale tracked
outputs fail the build. Commit the source and all four generated text files together;
QM stays in the build directory. The build obtains lrelease from Qt6::lrelease,
not PATH. Tests retain separate independent literal fixtures/goldens.

This build catalog does not implement runtime named formatting, language
preferences, live retranslation, browser/native presentation or packaging.
