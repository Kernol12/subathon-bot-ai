# ACCESSTEST-002 selected environment first run

Root executed the actual separately approved environment on 2026-11-04 after
review15460f62/importc3a925f, candidate source8d88fd0e/harnessdab476e. Authenticated
isolated Boost1.83, GCC16.2.1, CMake4.4.5, Ninja1.13.2; no fake SDK or host1.92.

```sh
ulimit -c 0
ctest --preset effective-permissions-debug -R '^scaffold.effective_permissions_environment$' --no-tests=error --verbose
ctest --preset effective-permissions-debug -LE '^intentional-red$' -E '^scaffold.effective_permissions_environment$' --no-tests=error
```

First command: exit0, 1/1 passed, 21 std/Boost primitive checks and zero failures.
[Exact retained output](accesstest-002-environment-first.txt), SHA256
`611ea8a5318ed461851d4bb29ea531258832fcca05d67bf38dc84bed5eb3fd9e`.
Second command: exit0, all36 existing numeric/infrastructure cases passed.
This is SDK/host foundation evidence only. No permission evaluator exists or has
run, no behavioral baseline/red/implementation or Ubuntu CI result is claimed.
Actual API/source/test reviews and intended red remain required before production.
