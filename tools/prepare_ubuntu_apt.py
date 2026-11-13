#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare authenticated package acquisition on GitHub's Ubuntu 24.04 runners."""
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

MIRROR_URI = "mirror+file:/etc/apt/apt-mirrors.txt"
# Admission is independent of the diagnostic-only URI labels below.
_SECURITY_URIS = {
    "http://security.ubuntu.com/ubuntu",
    "http://security.ubuntu.com/ubuntu/",
    "https://security.ubuntu.com/ubuntu",
    "https://security.ubuntu.com/ubuntu/",
}
MIRRORS = ("https://mirrors.edge.kernel.org/ubuntu/\tpriority:1\n"
           "https://archive.ubuntu.com/ubuntu/\tpriority:2\n"
           "https://security.ubuntu.com/ubuntu/\tpriority:3\n")
NETWORK_CONFIG = ('Acquire::Retries "1";\nAcquire::http::Timeout "15";\n'
                  'Acquire::https::Timeout "15";\nAPT::Update::Error-Mode "any";\n')
_ALLOWED_SOURCE_FIELDS = {"Types", "URIs", "Suites", "Components", "Signed-By",
                          "Architectures", "Enabled"}
# These additional labels are evidence only; they never admit a source field.
_DIAGNOSTIC_SOURCE_FIELDS = _ALLOWED_SOURCE_FIELDS | {
    "Architectures-Add", "Architectures-Remove", "Languages", "Targets", "PDiffs",
    "By-Hash", "Allow-Insecure", "Allow-Weak", "Allow-Downgrade-To-Insecure",
    "Trusted", "Check-Valid-Until", "Valid-Until-Min", "Valid-Until-Max",
    "Check-Date", "Date-Max-Future", "InRelease-Path", "Snapshot",
}
_PUBLIC_URI_LABELS = {
    "http://azure.archive.ubuntu.com/ubuntu/": "azure_archive",
    "http://azure.archive.ubuntu.com/ubuntu": "azure_archive",
    "https://azure.archive.ubuntu.com/ubuntu/": "azure_archive",
    "https://azure.archive.ubuntu.com/ubuntu": "azure_archive",
    "http://archive.ubuntu.com/ubuntu/": "ubuntu_archive",
    "http://archive.ubuntu.com/ubuntu": "ubuntu_archive",
    "https://archive.ubuntu.com/ubuntu/": "ubuntu_archive",
    "https://archive.ubuntu.com/ubuntu": "ubuntu_archive",
    "http://security.ubuntu.com/ubuntu/": "ubuntu_security",
    "http://security.ubuntu.com/ubuntu": "ubuntu_security",
    "https://security.ubuntu.com/ubuntu/": "ubuntu_security",
    "https://security.ubuntu.com/ubuntu": "ubuntu_security",
}


class _DiagnosticUnavailable(RuntimeError):
    """Stop setup when its required diagnostic cannot be emitted completely."""


def _emit_record(record: dict) -> None:
    """Write one bounded ASCII record and flush before any further setup work."""
    line = "CI_APT_V1 " + json.dumps(record, sort_keys=True, separators=(",", ":"),
                                    ensure_ascii=True, allow_nan=False) + "\n"
    if len(line) > 4096:
        raise _DiagnosticUnavailable()
    try:
        written = sys.stderr.write(line)
        sys.stderr.flush()
    except (OSError, ValueError):
        raise _DiagnosticUnavailable() from None
    if type(written) is not int or written != len(line):
        raise _DiagnosticUnavailable()


def _runner_record(release: dict[str, str]) -> None:
    """Use only the already admitted OS mapping and two named image facts."""
    record = {"event": "runner", "os_id": "ubuntu", "os_version": "24.04", "v": 1}
    redacted, unknown = [], []
    for key, value, pattern in (
            ("image_os", os.environ.get("ImageOS"), r"(ubuntu24)"),
            ("image_version", os.environ.get("ImageVersion"),
             r"([0-9]{8}(?:\.[0-9]{1,6}){1,2})"),
            ("os_patch", release.get("VERSION"),
             r"(24\.04(?:\.[0-9]{1,3})?)(?: LTS \(Noble Numbat\))?")):
        match = re.fullmatch(pattern, value, flags=re.ASCII) if value else None
        record[key] = match.group(1) if match else None
        if not value:
            unknown.append(key)
        elif not match:
            redacted.append(key)
    record["redacted_fields"] = sorted(redacted)
    record["unknown_fields"] = sorted(unknown)
    _emit_record(record)


def _matches_split_security_profile(source: str, parsed: list[dict]) -> bool:
    """Check the decoded raw grammar before using structurally checked fields."""
    # Only LF/CRLF delimiters are eligible, including within UTF-8 comments.
    # The unchanged text read cannot reveal the original on-disk newline form.
    if re.search(r"\r(?!\n)|[\v\f\x1c-\x1e\x85\u2028\u2029]", source):
        return False
    for line in re.split(r"\r?\n", source):
        if line == "" or line.startswith("#"):
            continue
        # No indented comments, whitespace-only separators or hidden field bytes.
        if not re.fullmatch(r"[!-~][\t -~]*", line):
            return False

    required = {"Types", "URIs", "Suites", "Components", "Signed-By"}
    if len(parsed) != 2 or any(fields.keys() != required for fields in parsed):
        return False
    if parsed[0]["URIs"][1] != MIRROR_URI or parsed[1]["URIs"][1] not in _SECURITY_URIS:
        return False

    components = {"main", "universe", "restricted", "multiverse"}
    suites = ({"noble", "noble-updates", "noble-backports"}, {"noble-security"})
    for fields, expected_suites in zip(parsed, suites):
        if (fields["Types"][1] != "deb" or fields["Signed-By"][1] !=
                "/usr/share/keyrings/ubuntu-archive-keyring.gpg"):
            return False
        for key, expected in (("Suites", expected_suites), ("Components", components)):
            tokens = fields[key][1].split()
            if len(tokens) != len(expected) or set(tokens) != expected:
                return False
    return True


def validate_sources(source: str) -> None:
    """Refuse unfamiliar deb822 layouts before touching the runner configuration."""
    # Count complete evidence stanzas before selecting the first refusal. Comments
    # do not create stanzas; positions retain the original splitlines numbering.
    stanzas, lines = [], []
    for position, line in enumerate(source.splitlines(), start=1):
        if line.lstrip().startswith("#"):
            continue
        if not line.strip():
            if lines:
                stanzas.append(lines)
                lines = []
            continue
        lines.append((position, line))
    if lines:
        stanzas.append(lines)

    def refuse(reason, stanza=None, line=None, field=None, value_class=None):
        _emit_record({"event": "source_refused", "reason": reason, "stanza": stanza,
                      "line": line, "field": field, "value_class": value_class,
                      "fields_omitted": (len(stanzas[stanza - 1]) - int(line is not None)
                                         if stanza is not None else 0),
                      "stanzas_omitted": len(stanzas) - 1 if stanza is not None else 0,
                      "v": 1})
        raise RuntimeError("unsupported Ubuntu APT source layout")

    if not stanzas:
        refuse("empty_source")

    # Structural refusals anywhere in the file precede all semantic checks.
    parsed = []
    for stanza, lines in enumerate(stanzas, start=1):
        fields = {}
        for position, line in lines:
            key, separator, value = line.partition(":")
            if line[0].isspace():
                refuse("continuation", stanza, position)
            if not separator:
                refuse("malformed_line", stanza, position)
            if key not in _ALLOWED_SOURCE_FIELDS:
                label = key if key in _DIAGNOSTIC_SOURCE_FIELDS else "<unknown>"
                refuse("unknown_field", stanza, position, label)
            if key in fields:
                refuse("duplicate_field", stanza, position, key)
            fields[key] = (position, value.strip())
        parsed.append(fields)

    if _matches_split_security_profile(source, parsed):
        _emit_record({"event": "source_admitted", "profile": "noble_split_security_v1", "v": 1})
        return

    for stanza, fields in enumerate(parsed, start=1):
        if "URIs" not in fields:
            refuse("missing_uri", stanza, field="URIs", value_class="missing")
        position, value = fields["URIs"]
        if value != MIRROR_URI:
            if not value:
                value_class = "empty"
            elif len(value.split()) > 1:
                value_class = "multiple"
            else:
                value_class = _PUBLIC_URI_LABELS.get(value, "other")
            refuse("uri_not_admitted", stanza, position, "URIs", value_class)
        if "Enabled" in fields:
            position, value = fields["Enabled"]
            if value != "yes":
                refuse("enabled_not_admitted", stanza, position, "Enabled",
                       "disabled" if value == "no" else "other")


def diagnostic(command: list[str]) -> None:
    """Log best-effort evidence, including any output captured before a timeout."""
    label = " ".join(command)
    try:
        result = subprocess.run(command, check=False, capture_output=True, text=True, timeout=10)
        print(f"{label}: exit {result.returncode}", flush=True)
    except (OSError, subprocess.SubprocessError, UnicodeError) as error:
        print(f"{label}: unavailable {error}", flush=True)
        result = error
    for name, stream in (("stdout", sys.stdout), ("stderr", sys.stderr)):
        output = getattr(result, name, None)
        if output:
            print(output.decode(errors="replace") if isinstance(output, bytes) else output,
                  end="", file=stream, flush=True)


def prepare_apt() -> None:
    """Configure the disposable runner and require a successful index refresh."""
    if os.environ.get("GITHUB_ACTIONS") != "true" or os.environ.get("RUNNER_ENVIRONMENT") != "github-hosted":
        raise RuntimeError("requires GitHub-hosted Actions")
    release = platform.freedesktop_os_release()
    if release.get("ID") != "ubuntu" or release.get("VERSION_ID") != "24.04":
        raise RuntimeError("requires Ubuntu 24.04")
    try:
        source = Path("/etc/apt/sources.list.d/ubuntu.sources").read_text(encoding="utf-8")
    except UnicodeError:
        raise RuntimeError("source text decoding failed") from None
    _runner_record(release)
    validate_sources(source)

    print("Diagnostic only; not dependency admission", flush=True)
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
    except _DiagnosticUnavailable:
        try:
            sys.stderr.write("Ubuntu APT setup failed: CI APT diagnostic unavailable\n")
            sys.stderr.flush()
        except (OSError, ValueError):
            pass
        return 1
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f"Ubuntu APT setup failed: {error}", file=sys.stderr, flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
