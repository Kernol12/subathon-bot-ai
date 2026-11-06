# SPDX-License-Identifier: GPL-3.0-or-later
"""Private fresh-process worker. Only fixed parent-owned controls are accepted."""
import os
import sys


def emit(fd, code):
    raw = ('{"code":"' + code + '"}\n').encode("ascii")
    while raw:
        count = os.write(fd, raw)
        if count <= 0:
            raise OSError("closed outcome")
        raw = raw[count:]


def main():
    activated = False
    outcome = None
    offline = None
    code = "environment_unavailable"
    exits = {"ok": 0}
    try:
        # No package/candidate imports or reads precede the fixed address limit.
        if (len(sys.argv) != 9 or sys.argv[1] != "--manifest" or sys.argv[3] != "--sdk-dir"
                or sys.argv[5] != "--activation-fd" or sys.argv[7] != "--outcome-fd"):
            return 3
        activation, outcome = int(sys.argv[6]), int(sys.argv[8])
        if activation <= 2 or outcome <= 2 or activation == outcome:
            return 3
        import resource
        limit = 512 * 1024 * 1024
        resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
        if resource.getrlimit(resource.RLIMIT_AS) != (limit, limit):
            raise ValueError("address limit")
        # -I excludes the script directory. This single trusted source directory
        # is activated only after limits; candidate SDK activation happens later.
        sys.path.append(os.path.dirname(os.path.abspath(__file__)))
        from _schema_common import Budget, EXIT, Refusal
        from _schema_installation import Offline, environment
        exits = EXIT
        target, sdk = environment(sys.argv[4])
        os.write(activation, b"A")
        os.close(activation)
        activated = True
        offline = Offline()
        sys.addaudithook(offline.audit)
        from _schema_package import load_package
        from _schema_index import Index
        from _schema_installation import dependencies
        budget = Budget()
        manifest, specs, bindings, snapshots, values, aliases = load_package(sys.argv[2], budget)
        index = Index(specs, values, budget)
        index.admit_dialects_formats()
        dependencies(sdk, target)
        try:
            from _schema_backend import Backend
        except ImportError:
            raise Refusal("dependency_unavailable")
        Backend(index, sdk).validate(bindings, values, aliases)
        code = "ok"
    except MemoryError:
        code = "resource_limit" if activated else "environment_unavailable"
    except BaseException as error:
        # Keep even SDK traceback/value/URI text private. Unclassified inability
        # cannot be mislabeled invalid data or a successful validation.
        if activated:
            from _schema_common import Refusal
            code = error.code if isinstance(error, Refusal) else "validation_unavailable"
        else:
            code = "environment_unavailable"
    finally:
        if offline is not None and offline.blocked and code != "resource_limit":
            code = "validation_unavailable"
        if outcome is not None:
            try:
                emit(outcome, code)
                os.close(outcome)
            except OSError:
                pass
    return exits.get(code, 2 if activated else 3)


if __name__ == "__main__":
    raise SystemExit(main())
