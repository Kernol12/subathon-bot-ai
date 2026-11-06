#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read generated ELF/disassembly before any native foundation program executes.

This is a deliberately narrow build-harness audit for the reviewed inert stage,
not a general binary verifier. Unknown generated control flow fails closed and
needs different review. It never loads or executes an input artifact.
"""

import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess


HEADER = re.compile(r"^([0-9a-f]+) <(.*)>:$")
INSTRUCTION = re.compile(r"^\s*([0-9a-f]+):\s+([a-z][a-z0-9]*)\s*(.*)$")
TARGET = re.compile(r"^([0-9a-f]+)\s+<.*>$")
PLAIN = {
    "mov", "movabs", "movzbl", "movzwl", "movsbl", "movswl", "movslq",
    "movb", "movw", "movl", "movq", "lea", "push", "pop", "leave", "ret",
    "nop", "nopl", "nopw", "add", "sub", "cmp", "test", "xor", "and", "or",
    "shl", "shr", "sar", "sal", "imul", "mul", "idiv", "div", "neg", "not",
    "cltq", "cqto", "xchg", "endbr64", "syscall",
}
SIZED = re.compile(r"^(?:add|sub|cmp|test|xor|and|or|shl|shr|sar|sal|imul|mul|idiv|div|neg|not|inc|dec)[bwlq]?$")


def raw_helper(name):
    return name.startswith("schema_foundation::") or name.startswith("long schema_foundation::address<")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(*argv):
    result = subprocess.run(argv, check=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=30)
    require(len(result.stdout) <= 64 * 1024 * 1024, "binutils output exceeds audit bound")
    return result.stdout.decode("utf-8", errors="strict")


def elf(path):
    path = Path(path).resolve(strict=True)
    require(path.is_file() and path.stat().st_size <= 64 * 1024 * 1024,
            "unexpected ELF file/size")
    with path.open("rb") as source:
        header = source.read(64)
    require(len(header) == 64 and header[:6] == b"\x7fELF\x02\x01"
            and struct.unpack_from("<H", header, 18)[0] == 62,
            "audit requires ELF64 little-endian x86-64")
    return path


def disassembly(path):
    functions = {}
    current = None
    for line in command("objdump", "-d", "-C", "--no-show-raw-insn", str(path)).splitlines():
        header = HEADER.fullmatch(line.strip())
        if header:
            address, name = header.groups()
            current = (int(address, 16), name, [])
            require(current[0] not in functions, "ambiguous disassembly address")
            functions[current[0]] = current
            continue
        instruction = INSTRUCTION.fullmatch(line)
        if instruction and current is not None:
            address, mnemonic, operands = instruction.groups()
            current[2].append((int(address, 16), mnemonic, operands.strip()))
    require(functions, "empty disassembly")
    return functions


def reachable(functions, root_name, allowed):
    roots = [address for address, (_, name, _) in functions.items() if root_name(name)]
    require(len(roots) == 1, "generated child root missing/ambiguous")
    pending = roots[:]
    seen = set()
    while pending:
        address = pending.pop()
        if address in seen:
            continue
        seen.add(address)
        _, name, instructions = functions[address]
        require(allowed(name) and instructions, "unreviewed generated function: " + name)
        first, last = instructions[0][0], instructions[-1][0]
        for _, mnemonic, operands in instructions:
            require("%fs:" not in operands and "%gs:" not in operands,
                    "TLS/segment access in generated child function " + name)
            require(mnemonic in PLAIN or SIZED.fullmatch(mnemonic)
                    or mnemonic.startswith(("call", "j", "loop", "set", "cmov")),
                    "unreviewed generated instruction in " + name + ": " + mnemonic)
            if mnemonic.startswith("call") or mnemonic.startswith("j") or mnemonic.startswith("loop"):
                target = TARGET.fullmatch(operands)
                require(target is not None, "indirect/unresolved child transfer in " + name)
                destination = int(target.group(1), 16)
                if not mnemonic.startswith("call") and first <= destination <= last:
                    continue
                require(destination in functions, "unknown generated child target in " + name)
                pending.append(destination)
            require(mnemonic not in {"int", "int3", "sysenter"},
                    "unreviewed generated trap in " + name)
    return sorted(functions[address][1] for address in seen)


def libc_path():
    # The audit itself runs in the same minimal system environment as the probes.
    # Reading maps binds its actual libc; no foreign artifact is dlopened.
    with Path("/proc/self/maps").open("rb") as source:
        data = source.read(131073)
    require(len(data) <= 131072, "process maps exceeds audit bound")
    paths = set()
    for line in data.decode("utf-8", errors="strict").splitlines():
        fields = line.split(maxsplit=5)
        if len(fields) == 6 and Path(fields[5]).name == "libc.so.6":
            paths.add(Path(fields[5]).resolve(strict=True))
    require(len(paths) == 1, "actual libc mapping missing/ambiguous")
    return elf(paths.pop())


def syscall_return(path):
    functions = disassembly(path)
    found = [item for item in functions.values()
             if item[1] == "syscall" or item[1].startswith("syscall@@")]
    require(len(found) == 1, "actual libc syscall symbol missing/ambiguous")
    instructions = found[0][2]
    prefix = []
    for item in instructions:
        prefix.append(item)
        if item[1].startswith("ret"):
            break
    require(prefix and prefix[-1][1].startswith("ret"), "libc syscall has no bounded success return")
    require(sum(item[1] == "syscall" for item in prefix) == 1,
            "libc syscall success path changed")
    require(not any(item[1].startswith("call") for item in prefix),
            "libc syscall success path calls another function")
    branches = [item for item in prefix if item[1].startswith("j")]
    require(len(branches) == 1 and branches[0][1] in {"jae", "jnb"},
            "unreviewed libc syscall error branch")
    branch = branches[0]
    target = TARGET.fullmatch(branch[2])
    require(target is not None and int(target.group(1), 16) > prefix[-1][0],
            "libc syscall success does not fall through to ret")
    compares = [item for item in prefix if item[1].startswith("cmp")]
    require(len(compares) == 1 and compares[0][0] < branch[0]
            and "0xfffffffffffff001" in compares[0][2] and "%rax" in compares[0][2],
            "unreviewed libc negative-errno threshold")
    moves = {item[2].replace(" ", "").replace("\t", "")
             for item in prefix if item[1] == "mov"}
    require({"%rdi,%rax", "%rsi,%rdi", "%rdx,%rsi", "%rcx,%rdx",
             "%r8,%r10", "%r9,%r8", "0x8(%rsp),%r9"} <= moves,
            "libc x86-64 syscall argument register mapping changed")
    return found[0][1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--foundation", required=True)
    parser.add_argument("--child", required=True)
    parser.add_argument("--transport", required=True)
    options = parser.parse_args()
    foundation, child, transport = map(elf, (options.foundation, options.child, options.transport))
    graphs = {}
    graphs["copied_child"] = reachable(
        disassembly(foundation), lambda name: name.startswith("schema_foundation::child("),
        raw_helper)
    graphs["exec_reporter"] = reachable(
        disassembly(child), lambda name: name == "main",
        lambda name: name == "main" or raw_helper(name)
        or name.startswith("(anonymous namespace)::literal(")
        or name.startswith("(anonymous namespace)::descriptor("))
    graphs["inert_transport"] = reachable(
        disassembly(transport), lambda name: name == "subathon_schema_transport_v1",
        lambda name: name == "subathon_schema_transport_v1")
    exports = command("nm", "-D", "--defined-only", str(transport)).splitlines()
    require(len(exports) == 1 and exports[0].split()[-1] == "subathon_schema_transport_v1",
            "production native exports differ")
    dynamic = command("readelf", "--dynamic", "--wide", str(transport))
    require("BIND_NOW" in dynamic or re.search(r"FLAGS_1.*\bNOW\b", dynamic),
            "native artifact is not eagerly bound")
    symbols = command("nm", "--defined-only", "--demangle", str(transport))
    require("_GLOBAL__sub_I" not in symbols and "__static_initialization_and_destruction" not in symbols,
            "unexpected C++ resource initialization in inert transport")
    libc = libc_path()
    graphs["libc_success_return"] = syscall_return(libc)
    for label, names in graphs.items():
        print(label + ":", names)
    for path in (foundation, child, transport, libc):
        digest = hashlib.sha256()
        with path.open("rb") as source:
            for chunk in iter(lambda: source.read(65536), b""):
                digest.update(chunk)
        print("sha256", digest.hexdigest(), path)
    print("Static inert-stage child call graphs admitted; no input artifact executed")


if __name__ == "__main__":
    main()
