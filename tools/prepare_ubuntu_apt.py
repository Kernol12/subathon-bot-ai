#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare authenticated package acquisition on GitHub's Ubuntu 24.04 runners."""
import os
from pathlib import Path
import platform
import subprocess
import sys

MIRROR_URI = "mirror+file:/etc/apt/apt-mirrors.txt"
MIRRORS = ("https://mirrors.edge.kernel.org/ubuntu/\tpriority:1\n"
           "https://archive.ubuntu.com/ubuntu/\tpriority:2\n"
           "https://security.ubuntu.com/ubuntu/\tpriority:3\n")
NETWORK_CONFIG = ('Acquire::Retries "1";\nAcquire::http::Timeout "15";\n'
                  'Acquire::https::Timeout "15";\nAPT::Update::Error-Mode "any";\n')


def validate_sources(source: str) -> None:
    """Refuse unfamiliar deb822 layouts before touching the runner configuration."""
    error = "unsupported Ubuntu APT source layout"
    allowed = {"Types", "URIs", "Suites", "Components", "Signed-By", "Architectures", "Enabled"}
    stanzas, fields = [], {}
    for line in [*source.splitlines(), ""]:
        if line.lstrip().startswith("#"):
            continue
        if not line.strip():
            if fields:
                stanzas.append(fields)
                fields = {}
            continue
        key, separator, value = line.partition(":")
        if line[0].isspace() or not separator or key not in allowed or key in fields:
            raise RuntimeError(error)
        fields[key] = value.strip()
    if not stanzas or any(stanza.get("URIs") != MIRROR_URI
                          or stanza.get("Enabled", "yes") != "yes" for stanza in stanzas):
        raise RuntimeError(error)


def diagnostic(command: list[str]) -> None:
    """Log best-effort evidence, including any output captured before a timeout."""
    label = " ".join(command)
    try:
        result = subprocess.run(command, check=False, capture_output=True, text=True, timeout=10)
        print(f"{label}: exit {result.returncode}")
    except (OSError, subprocess.SubprocessError) as error:
        print(f"{label}: unavailable {error}")
        result = error
    for name, stream in (("stdout", sys.stdout), ("stderr", sys.stderr)):
        output = getattr(result, name, None)
        if output:
            print(output.decode(errors="replace") if isinstance(output, bytes) else output,
                  end="", file=stream)


def prepare_apt() -> None:
    """Configure the disposable runner and require a successful index refresh."""
    if os.environ.get("GITHUB_ACTIONS") != "true" or os.environ.get("RUNNER_ENVIRONMENT") != "github-hosted":
        raise RuntimeError("requires GitHub-hosted Actions")
    release = platform.freedesktop_os_release()
    if release.get("ID") != "ubuntu" or release.get("VERSION_ID") != "24.04":
        raise RuntimeError("requires Ubuntu 24.04")
    validate_sources(Path("/etc/apt/sources.list.d/ubuntu.sources").read_text(encoding="utf-8"))

    print("Diagnostic only; not dependency admission")
    for command in (["cat", "/etc/resolv.conf"], ["resolvectl", "status"],
                    ["cat", "/etc/apt/apt-mirrors.txt"]):
        diagnostic(command)
    for host in ("azure.archive.ubuntu.com", "archive.ubuntu.com", "security.ubuntu.com",
                 "mirrors.edge.kernel.org"):
        diagnostic(["getent", "ahostsv4", host])

    for path, content in (("/etc/apt/apt-mirrors.txt", MIRRORS),
                          ("/etc/apt/apt.conf.d/99-subathon-network", NETWORK_CONFIG)):
        subprocess.run(["sudo", "tee", path], input=content, check=True, text=True,
                       stdout=subprocess.DEVNULL, timeout=10)
    subprocess.run(["sudo", "apt-get", "update"], check=True, timeout=300)


def main() -> int:
    try:
        prepare_apt()
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f"Ubuntu APT setup failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
