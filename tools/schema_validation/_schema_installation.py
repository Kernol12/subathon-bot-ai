# SPDX-License-Identifier: GPL-3.0-or-later
"""Trusted process admission and same-artifact SDK metadata admission."""
import errno
import hashlib
import json
import os
import stat
import sys

from _schema_common import refuse, scalar_string, utf8_size


def metadata_error(error):
    if isinstance(error, OSError) and error.errno in {errno.ENOMEM, errno.EMFILE, errno.ENFILE}:
        refuse("resource_limit")


def bounded_path(value):
    return (type(value) is str and scalar_string(value) and "\0" not in value
            and 1 <= utf8_size(value) <= 4096 and os.path.isabs(value))


def regular_bytes(path, ceiling, directory=None):
    fd = os.open(path, os.O_RDONLY | os.O_CLOEXEC | os.O_NOFOLLOW | os.O_NONBLOCK,
                 dir_fd=directory)
    try:
        before = os.fstat(fd)
        if not stat.S_ISREG(before.st_mode) or not 0 <= before.st_size <= ceiling:
            raise ValueError("trusted regular file")
        raw = os.read(fd, before.st_size + 1)
        after = os.fstat(fd)
        if (len(raw) != before.st_size or os.read(fd, 1)
                or (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns)
                != (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns)):
            raise ValueError("unstable trusted file")
        return raw
    finally:
        os.close(fd)


def environment(sdk):
    # sysconfig itself observes these even under -I: check BEFORE importing it.
    if any(k in os.environ for k in ("_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH", "_PYTHON_PROJECT_BASE")):
        refuse("environment_unavailable")
    if (not bounded_path(sdk) or not bounded_path(sys.executable)
            or sys.implementation.name != "cpython" or sys.platform != "linux"
            or os.uname().machine != "x86_64" or sys.version_info[:2] not in {(3, 10), (3, 12), (3, 14)}
            or not sys.flags.isolated or not sys.flags.no_site):
        refuse("environment_unavailable")
    import sysconfig
    minor = sys.version_info.minor
    if (sysconfig.get_config_var("SOABI") != "cpython-3%d-x86_64-linux-gnu" % minor
            or bool(sysconfig.get_config_var("Py_DEBUG"))
            or bool(sysconfig.get_config_var("Py_GIL_DISABLED"))):
        refuse("environment_unavailable")
    executable = sys.executable
    bindir = os.path.dirname(executable)
    root = os.path.dirname(bindir)
    if os.path.basename(bindir) != "bin":
        refuse("environment_unavailable")
    try:
        regular_bytes(os.path.join(root, "pyvenv.cfg"), 4096)
        paths = sysconfig.get_paths(scheme="posix_prefix", vars={"base": root, "platbase": root})
        pure, plat = os.path.realpath(paths["purelib"]), os.path.realpath(paths["platlib"])
        received = os.path.realpath(sdk)
        if (not all(bounded_path(x) for x in (pure, plat, received)) or pure != plat or pure != received
                or not os.path.isdir(received) or not os.path.samefile(pure, sdk)):
            refuse("environment_unavailable")
    except (OSError, ValueError, KeyError):
        refuse("environment_unavailable")
    return "cp3%d" % minor, received


def dependencies(sdk, target):
    """Read fixed metadata pairs, without discovery, imports, or .pth processing."""
    directory = None
    try:
        manifest = json.loads(regular_bytes(os.path.join(os.path.dirname(__file__), "wheels.json"), 1048576))
        if len(manifest["packages"]) != 20:
            refuse("dependency_unavailable")
        directory = os.open(sdk, os.O_RDONLY | os.O_CLOEXEC | os.O_DIRECTORY)
        for package in manifest["packages"]:
            artifacts = [a for a in package["artifacts"] if target in a["candidate_targets"]]
            matched = False
            for artifact in artifacts:
                fd = None
                try:
                    fd = os.open(artifact["dist_info_directory"], os.O_RDONLY | os.O_CLOEXEC | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=directory)
                    wheel = regular_bytes("WHEEL", 4096, fd)
                    metadata = regular_bytes("METADATA", 262144, fd)
                    if (hashlib.sha256(wheel).hexdigest() == artifact["wheel_metadata_sha256"]
                            and hashlib.sha256(metadata).hexdigest() == artifact["distribution_metadata_sha256"]):
                        matched = True
                        break
                except (OSError, ValueError) as error:
                    metadata_error(error)
                finally:
                    if fd is not None:
                        os.close(fd)
            if not matched:
                refuse("dependency_unavailable")
    except (OSError, ValueError, KeyError, TypeError) as error:
        metadata_error(error)
        refuse("dependency_unavailable")
    finally:
        if directory is not None:
            os.close(directory)
    # -I -S supplies stdlib only; this is the sole candidate import directory.
    sys.path.append(sdk)


class ForbiddenIO(BaseException):
    pass


class Offline:
    """Persistent backstop: swallowed library IO exceptions can never yield ok."""
    blocked = False

    def audit(self, event, _args):
        if (event.startswith(("socket.", "http.client.", "urllib.", "subprocess.", "os.exec", "os.spawn"))
                or event in {"os.system", "os.posix_spawn", "os.fork", "os.forkpty"}):
            self.blocked = True
            raise ForbiddenIO()
