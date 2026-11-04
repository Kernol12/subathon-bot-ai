# LOCALETEST-001 — selected tool environment preflight

Coordinator root and read-only investigator codec_author,2026-11-04. This is
existing third-party tool/archive evidence only. No project catalog/generator,
fixture, feature test, compiled QM, translation lookup or intended red was created
or executed. These environment observations do not satisfy the future baseline.

## Installed host tools

Read-only version checks observed /usr/lib/qt6/bin/lrelease and lupdate6.12.0,
Qt Core/qtpaths6.12.0, Arch qt6-tools6.12.0-1 and imported CMake targets
Qt6::lrelease/lupdate/lconvert. The tools are not assumed to be on PATH.
Root independently ran /usr/lib/qt6/bin/lrelease -version and pkg-config Qt6Core:
both report6.12.0. This host tool is not substituted for an official CI artifact.

## Actual verified official artifacts

The official repository component is qt.qt6.6120.linux_gcc_64, build
6.12.0-0-202609280346. The investigator read the actual
[Updates.xml](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6120/qt6_6120/Updates.xml)
and checksum/Metalink metadata. Root then downloaded and independently hashed the
archives below into disposable /tmp/subathon-locale-qttools-6.12.0. Each SHA256
matches the selected/public checksum; archive paths were inspected before extraction
(no absolute or parent-traversal paths). Actual sizes and entries:

| Archive | Bytes / entries | Verified SHA256 |
| --- | --- | --- |
| qtbase-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z |23729678 /5183|943023b929d0c08eafe7341700b5ed7601284410b7afd6093f950be291d1d7cd|
| icu-linux-Rhel8.6-x86_64.7z |9660660 /12|1fdbcd67b1760f3a3f031c64637a9f19992aed2f35aa615e12a468f8c655d4da|
| qttools-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z |29144192 /589|5d6f90961df3a1e5a57fac69bbbe6983860bad11c419a49736b84914588b4334|
| qtdeclarative-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z |153654100 /5771|0c719e17ea9fbd37aa615595149dca2578985656433f27bf2846cffe4b687d3e|

Base and ICU are the existing SQL workflow pins. The QtTools and Declarative
published checksums were independently read from the respective
[QtTools sidecar](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6120/qt6_6120/qt.qt6.6120.linux_gcc_64/6.12.0-0-202609280346qttools-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z.sha256)
and [Declarative sidecar](https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_6120/qt6_6120/qt.qt6.6120.linux_gcc_64/6.12.0-0-202609280346qtdeclarative-Linux-RHEL_9_6-GCC-Linux-RHEL_9_6-X86_64.7z.sha256).
All four filenames use that same version/build prefix. Their SHA256 matches were
observed against actual downloaded bytes, not invented from filenames.

QtTools actually contains bin/lrelease and the LinguistTools CMake configuration.
readelf on that exact binary reports Qt6Qml, Qt6Network and Qt6Core dependencies;
therefore the existing qtbase/ICU-only prefix cannot establish lrelease availability.
Matching Declarative is required by the selected official binary even though the
project's baseline runtime probe can otherwise use Qt Core.

The first scratch prefix probe failed because root extracted ICU at the prefix
root instead of prefix/lib. This is a setup error, NOT feature red/acceptance.
Root corrected the extraction location to match the existing CI workflow and
reran the checks with checked process results. The actual selected official
bin/lrelease -version then exited0 and reported6.12.0. ldd reported no missing
libraries; ALL Qt6Qml/Network/Core and ICU73 dependencies resolved inside this
verified prefix, not host Qt. Other OS libraries resolved normally from the host.
No translation source or compiled QM was produced by that version probe.

## Verified source facts and remaining gates

Primary Qt6.12 source supports empty-context stable-ID QM keys; -idbased is now a
deprecated no-op. Direct QTranslator returns the raw selected text, while the
QCoreApplication/qtTrId wrapper separately substitutes %n. English/German witness
values1/2 select the corresponding two forms. These facts guide the proposed common
named-template mapping; they are not actual compiled-QM runtime proof.

- [QM writer](https://github.com/qt/qttools/blob/v6.12.0/src/linguist/shared/qm.cpp)
- [QTranslator](https://github.com/qt/qtbase/blob/v6.12.0/src/corelib/kernel/qtranslator.cpp)
- [QCoreApplication](https://github.com/qt/qtbase/blob/v6.12.0/src/corelib/kernel/qcoreapplication.cpp)
- [Plural table](https://github.com/qt/qttools/blob/v6.12.0/src/linguist/shared/numerus.cpp)

Later reviewed harness/CI must use the selected exact tool, real independently
specified TS fixtures, actual en/de QM compilation/load/raw lookup, escaped Unicode/
percent/CR cases, actual English fallback and two fresh-directory reproducibility.
The tool help has no deterministic-output switch and this preflight proves no
cross-build byte identity. Missing libraries, compile errors or skips cannot count
as the intended project missing-feature baseline. Root owns future CMake/workflow/
pin integration; no new workflow or feature suite was introduced in this preflight.
