# BOOT-002 — Validate CLion and the local CMake toolchain

Module: Foundation. Kind: environment.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §2, §17.

## Goal

Configure/build/test the scaffold preset on Arch/KDE; record tool versions and resolve normal setup issues.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Configure/build/test scaffold-debug in CLion's selected native toolchain.
- Run Python progress/tooling consistency checks; record compiler/CMake/Ninja/Python versions.
- Set up GoogleTest without silent downloads, and record availability of Qt >=6.12 separately.
- Preserve stub/draft test status. Record actual commands/results and remaining gates.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
