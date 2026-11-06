#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Closed GNU/Linux x86-64 owner build audit; never load/execute an input ELF.

This checks the reviewed two policy instantiations, not arbitrary C++ binaries.
Existing raw-foundation/bootstrap audits remain separate mandatory gates.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy


LIMIT = 32768
NS = "subathon::schema_validation::native::"
RAW = NS + "RawOps"
FAULT = NS + "fault::FaultOps"
ENTRY = "subathon_schema_transport_v1"
Q = r'"(?:[^"\\]|\\.)*"'
NODE = re.compile(r'node:\s*\{\s*title:\s*(' + Q + r')\s+label:\s*(' + Q
                  + r')(?:\s+shape\s*:\s*(triangle|ellipse))?\s*\}')
EDGE = re.compile(r'edge:\s*\{\s*sourcename:\s*(' + Q + r')\s+targetname:\s*('
                  + Q + r')(?:\s+label:\s*' + Q + r')?\s*\}')
TARGET = re.compile(r'^([0-9a-f]+)\s+<.*>$')
FRAME = re.compile(r'([0-9]+) bytes \((static|dynamic,bounded|dynamic)\)')
SLOT = r'-0x[0-9a-f]+\(%rbp\)'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def owner(policy):
    require(policy in (RAW, FAULT), "unreviewed owner policy")
    return NS + "detail::Owner<" + policy + ">"


def syscall_name(policy, call):
    require(policy in (RAW, FAULT) and 0 <= call <= 19, "unreviewed syscall specialization")
    return "long " + policy + "::syscall<(" + NS + "KernelCall)" + str(call) + ">(" + ", ".join(["unsigned long"] * 6) + ")"


def allowed_names(policy):
    """Finite methods/signatures from reviewed source; no namespace admission."""
    o = owner(policy)
    slot = o + "::Slot"
    names = {ENTRY, "int " + NS + "run_transport_v1<" + policy + ">(" + NS
             + "Request const&, " + policy + "&)"}
    suffixes = {
        "refuse(int)", "capacity_error(long)", "syscall_failure(long, int, bool)",
        "checkpoint(" + NS + "PhaseBase, " + NS + "PhaseEdge, long, bool)",
        "adopted(" + slot + ")", "close_bare(" + slot + ", bool)",
        "close_work(" + slot + ", " + NS + "PhaseBase)", "still_working(long*)",
        "config_name()", "read_config()", "make_pipe(" + slot + ", " + slot + ", " + NS + "PhaseBase)",
        "prepare_sources()", "child_exit(int)", "child_refusal(bool)", "child()", "birth()",
        "read_channel(" + slot + ", unsigned char*, unsigned int&, unsigned int, bool&)",
        "reset_status()", "reaped()", "wait_work()", "matching_line(" + NS + "detail::WireOutcome const&)",
        "decode()", "receive()", "terminate()", "pinned_exit(int)", "wait_cleanup()", "cleanup()",
        "Owner(" + policy + "&, " + NS + "detail::Scratch&, long)", "~Owner()", "run()",
    }
    names.update(o + "::" + suffix for suffix in suffixes)
    names.update(NS + "detail::" + suffix for suffix in (
        "word(int)", "shape(" + NS + "PathView)", "copy_path(" + NS + "PathView, unsigned char*)",
        "ascii_alpha(unsigned int)", "uri_scheme(unsigned char const*, unsigned int)", "same_snapshot(stat const&, stat const&)"))
    for kind in ("clone_args", "stat", "pollfd", "siginfo_t", "char const", "char const*",
                 NS + "detail::KernelAction", "unsigned char", "int", "unsigned long"):
        names.add("unsigned long " + NS + "detail::address<" + kind + ">(" + kind + "*)")
    names.update(syscall_name(policy, call) for call in range(20))
    names.update(policy + "::" + suffix for suffix in (
        "monotonic_ns()", "checkpoint(" + NS + "Phase, long)", "observe(" + NS + "Observation const&)"))
    if policy == RAW:
        names.add(RAW + "::RawOps(" + NS + "Request const&)")
        return names
    names.update(FAULT + "::" + suffix for suffix in (
        "FaultOps(" + NS + "Request const&)", "writer() const", "negative_mode() const",
        "emit(" + NS + "ObservationKind, int, long, int)", "fail()", "live(int)",
        "acquired(long, bool, bool)", "closed(int, long)",
        "target(" + NS + "PhaseBase, " + NS + "KernelCall, unsigned long)",
        "entered_child()", "signals_are_default()", "decimal(unsigned int, char*, unsigned int)",
        "path(char const*, " + NS + "PathView)", "normal_exec(unsigned long, unsigned long, unsigned long)",
        "exec(unsigned long, unsigned long, unsigned long)"))
    names.update("bool " + FAULT + "::literal<" + str(n) + "ul>(char const*, char const (&) [" + str(n) + "ul])"
                 for n in (2, 3, 10, 11, 13, 15, 16, 19))
    names.update(NS + "fault::" + suffix for suffix in (
        "selected_plan()", "record(" + NS + "fault::Writer, " + NS + "Observation const&)",
        "monotonic_ns()", "pin_birth(int, int)", "pin_report_read(int)",
        "capture_outcome(unsigned char const*, unsigned long)", "checkpoint(" + NS + "Phase, long)",
        "child_ready_and_hold()"))
    shared = "(anonymous namespace)::Shared"
    names.update("(anonymous namespace)::" + suffix for suffix in (
        "state()", "wake(" + shared + "*)", "poison(" + shared + "*)", "real_clock()",
        "encoded(" + NS + "Phase)", "append(" + shared + "*, " + NS + "fault::Writer, "
        + NS + "ObservationKind, unsigned int, long, long, long)",
        "wait_change(" + shared + "*, unsigned int, long)", "await_flag(" + shared + "*, unsigned int const*)",
        "wait_pinned(int, siginfo_t&, int)"))
    for kind in (shared + "*", "unsigned int", "long"):
        names.add(kind + " (anonymous namespace)::load<" + kind + ">(" + kind + " const*)")
        names.add("void (anonymous namespace)::store<" + kind + ">(" + kind + "*, " + kind + ")")
    names.add("schema_foundation::raw(long, long, long, long, long, long, long)")
    names.add("schema_foundation::finish(int)")
    for kind in ("pollfd", "timespec", "timespec const", "siginfo_t", "char const", "char const*",
                 "schema_foundation::KernelAction", shared, "char", "unsigned char", "int", "unsigned int", "unsigned long"):
        names.add("long schema_foundation::address<" + kind + ">(" + kind + "*)")
    return names


def child_names(policy):
    names = allowed_names(policy)
    o = owner(policy)
    names = {name for name in names if name != ENTRY and not name.startswith("int " + NS + "run_transport_v1<")
             and (not name.startswith(o + "::") or name in {o + "::child()", o + "::child_refusal(bool)", o + "::child_exit(int)"})}
    names.difference_update(syscall_name(policy, call) for call in range(20))
    names.update(syscall_name(policy, call) for call in (4, 5, 9, 10, 11, 12, 15, 16))
    return names


def owner_atomic(name, mnemonic, operands):
    """Only the two reviewed, width/register/address-exact shared CAS sites."""
    return mnemonic == "lock" and (name, operands) in (
        (NS + "fault::monotonic_ns()", "cmpxchg %rcx,(%rsi)"),
        (NS + "fault::checkpoint(" + NS + "Phase, long)", "cmpxchg %ecx,(%rsi)"),
    )


def owner_parent_scalar(name, mnemonic, operands):
    """Operand-free word-to-long sign extension in the four reviewed parents."""
    return mnemonic == "cwtl" and operands == "" and name in (
        NS + "detail::Owner<" + RAW + ">::receive()",
        NS + "detail::Owner<" + RAW + ">::pinned_exit(int)",
        NS + "detail::Owner<" + FAULT + ">::receive()",
        NS + "detail::Owner<" + FAULT + ">::pinned_exit(int)",
    )


def owner_instruction(name, mnemonic, operands):
    return owner_atomic(name, mnemonic, operands) or owner_parent_scalar(name, mnemonic, operands)


def bounded_text(path):
    with Path(path).open("rb") as source:
        value = source.read(8 * 1024 * 1024 + 1)
    require(len(value) <= 8 * 1024 * 1024, "compiler metadata exceeds audit bound")
    return value.decode("utf-8", errors="strict")


def identity(value):
    symbol = value.rsplit(":", 1)[-1]
    require(re.fullmatch(r'[A-Za-z_][A-Za-z0-9_.$]*', symbol) is not None, "unknown compiler symbol identity")
    return symbol


def parse_ci(text):
    """The exact GNU VCG record subset emitted by the reviewed build flags."""
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    require(len(lines) <= 32768 and len(lines) >= 2, "compiler graph record bound")
    require(re.fullmatch(r'graph:\s*\{\s*title:\s*' + Q, lines[0]) is not None and lines[-1] == "}", "malformed compiler graph envelope")
    nodes, edges = {}, []
    for line in lines[1:-1]:
        node, edge = NODE.fullmatch(line), EDGE.fullmatch(line)
        require(node is not None or edge is not None, "unknown compiler graph record")
        if node:
            name, label, shape = node.groups()
            name, label = identity(json.loads(name)), json.loads(label)
            parts = label.split("\n")
            require(len(parts) in (2, 3) and len(label) <= 16384, "unknown compiler node label")
            frame = FRAME.fullmatch(parts[2]) if len(parts) == 3 else None
            require(len(parts) == 2 or frame is not None, "unknown compiler frame label")
            value = (parts[0], parts[1], (int(frame[1]), frame[2]) if frame else None, shape)
            require(name not in nodes or nodes[name] == value, "conflicting compiler node")
            nodes[name] = value
        else:
            edges.append(tuple(identity(json.loads(x)) for x in edge.groups()))
    require(len(nodes) <= 4096 and len(edges) <= 16384, "compiler graph size bound")
    require(all(a in nodes and b in nodes for a, b in edges), "compiler edge has no node")
    return nodes, edges


def parse_su(text):
    records = {}
    for line in text.splitlines():
        match = re.fullmatch(r'(.+:[0-9]+:[0-9]+):(.+)\t([0-9]+)\t(static|dynamic,bounded|dynamic)', line)
        require(match is not None, "unknown GNU stack record")
        location, signature, count, qualifier = match.groups()
        key, value = (signature, location), (int(count), qualifier)
        require(key not in records or records[key] == value, "conflicting GNU stack record")
        records[key] = value
    require(0 < len(records) <= 4096, "GNU stack record count bound")
    return records


def compiler_graph(pairs, symbols):
    """Bind compiler mangled identities/frames to actual ELF addresses."""
    nodes, edges, definitions, origins = {}, [], {}, {}
    for ci, su in pairs:
        local, connections = parse_ci(ci)
        stack = parse_su(su)
        for name, record in local.items():
            signature, location, frame, shape = record
            if frame is not None:
                require(stack.get((signature, location)) == frame, "CI/SU frame disagreement")
                require(name not in definitions or definitions[name] == frame, "conflicting compiled frame")
                definitions[name] = frame
                require(name not in origins or origins[name] == (signature, location), "conflicting frame source identity")
                origins[name] = signature, location
            nodes.setdefault(name, []).append(record)
        edges.extend(connections)
    frames, graph, owners = {}, {}, {}
    for name, frame in definitions.items():
        if name in symbols:
            address = symbols[name]
            require(address not in frames or (frames[address] == frame and owners[address] == origins[name]),
                    "ELF alias frame disagreement")
            frames[address] = frame
            owners[address] = origins[name]
    for name, records in nodes.items():
        for _, _, frame, shape in records:
            if frame is None and shape == "triangle":
                targets = {b for a, b in edges if a == name}
                require(len(targets) == 1, "ambiguous compiler alias")
                target = next(iter(targets))
                require(name in symbols and target in definitions and target in symbols
                        and symbols[name] == symbols[target], "unproven compiler alias frame")
    for a, b in edges:
        if a not in symbols:
            continue  # Unlinked/unused compiler nodes cannot enter the ELF graph.
        address = symbols[a]
        if a != b and b in symbols and symbols[b] == address:
            require(any(record[3] == "triangle" for record in nodes[a]) and b in definitions,
                    "same-address edge is not a proven constructor/destructor alias")
            continue
        graph.setdefault(address, set()).add(symbols.get(b, b))
    return frames, graph


def fixed_rsp(instructions):
    """Allow fixed outgoing-argument pushes, never a register-sized stack."""
    for _, mnemonic, operands in instructions:
        if mnemonic == "push":
            continue  # Reads its explicit operand; its fixed stack use is counted by GNU.
        destination = operands.rsplit(",", 1)[-1]
        require(not (mnemonic == "xchg" and any(register in operands for register in ("%rsp", "%esp", "%sp", "%spl", "%rbp", "%ebp"))),
                "unreviewed stack/frame pointer exchange")
        if destination in ("%rbp", "%ebp", "%bp", "%bpl"):
            require((mnemonic == "mov" and operands == "%rsp,%rbp")
                    or (mnemonic == "pop" and operands == "%rbp"), "unreviewed frame-pointer write")
        if mnemonic.startswith("ret"):
            require(mnemonic == "ret" and not operands, "unreviewed callee stack adjustment")
        if destination not in ("%rsp", "%esp", "%sp", "%spl"):
            continue
        immediate = re.fullmatch(r'\$0x(0|[1-9a-f][0-9a-f]{0,15}),%rsp', operands)
        signed = None
        if immediate is not None:
            encoded = int(immediate[1], 16)
            if encoded < 1 << 64:
                signed = encoded if encoded < 1 << 63 else encoded - (1 << 64)
        epilogue = re.fullmatch(r'-0x([1-9a-f][0-9a-f]{0,15})\(%rbp\),%rsp', operands)
        require((mnemonic in ("add", "sub") and signed is not None
                 and abs(signed) <= LIMIT and signed % 8 == 0)
                or (mnemonic == "lea" and epilogue is not None
                    and 0 < int(epilogue[1], 16) <= LIMIT and int(epilogue[1], 16) % 8 == 0)
                or (mnemonic == "mov" and operands == "%rbp,%rsp"),
                "unbounded/unknown generated stack-pointer write: " + mnemonic + " " + operands)


def stack_bound(frames, graph, root):
    visiting, complete = set(), {}

    def visit(node):
        require(node not in visiting, "recursive owner call graph")
        if node in complete:
            return complete[node]
        require(node in frames, "missing/external owner stack frame")
        count, kind = frames[node]
        require(kind in ("static", "dynamic,bounded") and 0 <= count <= LIMIT,
                "unbounded/unknown owner stack frame")
        visiting.add(node)
        descendants = [visit(child) + 8 for child in graph.get(node, ())]
        total = count + max(descendants, default=0)
        visiting.remove(node)
        require(total <= LIMIT, "whole inherited owner stack exceeds 32768")
        complete[node] = total
        return total

    return visit(root)


def function(functions, name):
    found = [item for item in functions.values() if item[1] == name]
    require(len(found) == 1, "missing/ambiguous reviewed owner function: " + name)
    return found[0]


def called_address(instruction):
    target = TARGET.fullmatch(instruction[2])
    require(instruction[1] in ("call", "callq") and target is not None, "unknown direct owner call")
    return int(target[1], 16)


def zero_edge(functions, birth_name, policy_name, child_name):
    """Known GNU Debug result-save/zero-branch edge; no child parent-return path."""
    birth, policy, child = (function(functions, name) for name in (birth_name, policy_name, child_name))
    instructions = birth[2]
    calls = [i for i, item in enumerate(instructions) if item[1] in ("call", "callq")
             and called_address(item) == policy[0]]
    require(len(calls) == 1 and calls[0] + 8 < len(instructions), "ambiguous/truncated policy clone edge")
    i = calls[0]
    edge = instructions[i + 1:i + 8]
    saved = re.fullmatch(r'%rax,(' + SLOT + r')', edge[1][2])
    branch = TARGET.fullmatch(edge[3][2])
    require(edge[0][1:] == ("add", "$0x10,%rsp") and edge[1][1] == "mov" and saved is not None,
            "clone result not preserved directly")
    require(edge[2][1:] == ("cmpq", "$0x0," + saved[1]) and edge[3][1] == "jne"
            and branch is not None and int(branch[1], 16) == instructions[i + 8][0],
            "clone zero branch does not skip only the raw child entry")
    require(edge[4][1] == "mov" and re.fullmatch(SLOT + r',%rax', edge[4][2])
            and edge[5][1:] == ("mov", "%rax,%rdi") and called_address(edge[6]) == child[0],
            "unreviewed instruction before raw child entry")
    require(not any(item[1].startswith("ret") for item in child[2]), "raw child can return into parent")
    return policy[0], child[0]


def raw_return(instructions, start):
    tail = instructions[start:]
    while tail and tail[-1][1] in ("nop", "nopl", "nopw"):
        tail = tail[:-1]
    require(len(tail) >= 3, "truncated raw syscall return")
    saved = re.fullmatch(r'%rax,(' + SLOT + r')', tail[0][2])
    require(tail[0][1] == "mov" and saved is not None and tail[1][1:] == ("mov", saved[1] + ",%rax"),
            "raw syscall result is not returned unchanged")
    epilogue = tail[2:]
    require(epilogue[-1][1:] == ("ret", "")
            and all((item[1] == "pop" and item[2] in ("%rbp", "%rbx", "%r12", "%r13", "%r14", "%r15"))
                    or item[1:] == ("leave", "") for item in epilogue[:-1]),
            "unreviewed operation after raw syscall return")


def policy_zero_edge(functions, policy):
    instructions = function(functions, syscall_name(policy, 8))[2]
    if policy == RAW:
        positions = [i for i, item in enumerate(instructions) if item[1] == "syscall"]
        require(len(positions) == 1, "production clone has no unique raw syscall")
        i = positions[0]
        writes = [item for item in instructions[:i] if item[2].rsplit(",", 1)[-1] in ("%rax", "%eax", "%ax", "%al", "%ah")]
        require(writes and writes[-1][1:] == ("mov", "$0x1b3,%eax"), "production clone syscall number changed")
        raw_return(instructions, i + 1)
        return
    raw = function(functions, "schema_foundation::raw(long, long, long, long, long, long, long)")
    positions = [i for i, item in enumerate(instructions) if item[1] in ("call", "callq") and called_address(item) == raw[0]]
    require(len(positions) == 1, "fault clone has no unique raw syscall call")
    i = positions[0]
    require(i >= 1 and i + 17 < len(instructions), "truncated fault clone edge")
    require(instructions[i - 1][1:] == ("mov", "$0x1b3,%edi"), "fault clone syscall number changed")
    edge = instructions[i + 1:i + 17]
    require(len(edge) == 16 and edge[0][1:] == ("add", "$0x10,%rsp"), "fault clone return prefix changed")
    saved = re.fullmatch(r'%rax,(' + SLOT + r')', edge[1][2])
    require(edge[1][1] == "mov" and saved is not None, "fault clone lost actual raw result")
    receiver = re.fullmatch(r'(' + SLOT + r'),%rax', edge[2][2])
    require(receiver is not None and receiver[1] != saved[1]
            and [item[1] for item in edge[2:10]] == ["mov", "movl", "mov", "mov", "mov", "mov", "mov", "mov"]
            and re.fullmatch(r'\$0x8,0x[0-9a-f]+\(%rax\)', edge[3][2])
            and edge[4][2] == edge[2][2] and re.fullmatch(SLOT + r',%rdx', edge[5][2])
            and re.fullmatch(r'%rdx,0x[0-9a-f]+\(%rax\)', edge[6][2])
            and edge[7][2] == edge[2][2] and edge[8][2] == saved[1] + ",%rdx"
            and re.fullmatch(r'%rdx,0x[0-9a-f]+\(%rax\)', edge[9][2]),
            "fault clone post-return bookkeeping changed")
    branch = TARGET.fullmatch(edge[11][2])
    entered = function(functions, FAULT + "::entered_child()")
    require(edge[10][1:] == ("cmpq", "$0x0," + saved[1]) and edge[11][1] == "jne"
            and branch is not None and int(branch[1], 16) == instructions[i + 17][0]
            and edge[12][1] == "mov" and re.fullmatch(SLOT + r',%rax', edge[12][2])
            and edge[13][1:] == ("mov", "%rax,%rdi") and called_address(edge[14]) == entered[0],
            "fault zero result does not enter the audited child policy")
    target = TARGET.fullmatch(edge[15][2])
    require(edge[15][1] == "jmp" and target is not None, "fault child does not skip parent-only adoption")
    finish = next((j for j, item in enumerate(instructions) if item[0] == int(target[1], 16)), None)
    require(finish is not None and finish + 2 < len(instructions)
            and instructions[finish][1:] == ("mov", saved[1] + ",%rax")
            and instructions[finish + 1][1:] == ("leave", "")
            and instructions[finish + 2][1:] == ("ret", "")
            and all(item[1] in ("nop", "nopl", "nopw") for item in instructions[finish + 3:]),
            "fault child does not return actual zero unchanged")
    raw_sites = [j for j, item in enumerate(raw[2]) if item[1] == "syscall"]
    require(len(raw_sites) == 1, "fault raw wrapper syscall missing/ambiguous")
    raw_return(raw[2], raw_sites[0] + 1)


def _audit_artifact(path, metadata_paths, policy, base):
    path = base["elf"](path)
    functions = base["disassembly"](path)
    names = allowed_names(policy)
    seen_names = base["reachable"](functions, lambda name: name == ENTRY, lambda name: name in names,
                                   extra_instruction=owner_instruction)
    reached = {address for address, (_, name, _) in functions.items() if name in seen_names}
    binary = {address: set() for address in reached}
    for address in reached:
        instructions = functions[address][2]
        fixed_rsp(instructions)
        first, last = instructions[0][0], instructions[-1][0]
        for item in instructions:
            if item[1].startswith(("call", "j", "loop")):
                target = TARGET.fullmatch(item[2])
                require(target is not None, "unresolved owner transfer")
                destination = int(target[1], 16)
                if item[1].startswith("call") or not first <= destination <= last:
                    binary[address].add(destination)
    symbols = {}
    for line in base["command"]("nm", "-P", "--defined-only", str(path)).splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[1] in ("t", "T", "w", "W"):
            name, address = parts[0], int(parts[2], 16)
            require(name not in symbols or symbols[name] == address, "ambiguous ELF function identity")
            symbols[name] = address
    pairs, metadata = [], []
    for ci in metadata_paths:
        texts = (bounded_text(ci), bounded_text(Path(ci).with_suffix(".su")))
        pairs.append(texts)
        metadata.extend((str(name), hashlib.sha256(text.encode("utf-8")).hexdigest())
                        for name, text in zip((ci, Path(ci).with_suffix(".su")), texts))
    frames, compiler = compiler_graph(pairs, symbols)
    graph = {}
    for address in reached:
        require(binary[address] <= compiler.get(address, set()), "ELF call lacks compiler metadata edge")
        graph[address] = binary[address] | compiler.get(address, set())
        require(graph[address] <= reached, "compiler graph escapes the audited ELF closure")
    maximum = stack_bound(frames, graph, symbols[ENTRY])
    birth, child = owner(policy) + "::birth()", owner(policy) + "::child()"
    zero_edge(functions, birth, syscall_name(policy, 8), child)
    policy_zero_edge(functions, policy)
    raw_names = child_names(policy)
    base["reachable"](functions, lambda name: name == child, lambda name: name in raw_names,
                      extra_instruction=owner_instruction)
    base["reachable"](functions, lambda name: name == syscall_name(policy, 8),
                      lambda name: name in raw_names or name == syscall_name(policy, 8),
                      extra_instruction=owner_instruction)
    export_rows = [line.split() for line in base["command"]("nm", "-D", "--defined-only", str(path)).splitlines()]
    expected = {ENTRY}
    if policy == FAULT:
        expected |= {"subathon_schema_fault_" + suffix + "_v1" for suffix in ("arm", "snapshot", "report", "fallback")}
    require(len(export_rows) == len(expected) and all(len(row) == 3 and row[1] == "T" for row in export_rows)
            and {row[-1] for row in export_rows} == expected, "owner artifact exports differ")
    dynamic = base["command"]("readelf", "-dW", str(path))
    require("BIND_NOW" in dynamic or re.search(r'FLAGS_1.*\bNOW\b', dynamic), "owner artifact is not eager")
    symbols_text = base["command"]("nm", "--defined-only", "--demangle", str(path))
    require("_GLOBAL__sub_I" not in symbols_text and "__static_initialization_and_destruction" not in symbols_text,
            "dynamic C++ owner initialization")
    print("owner", policy, "functions", len(reached), "inherited_stack_upper_bound", maximum)
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    print("sha256", digest, path)
    for name, value in metadata:
        print("metadata_sha256", value, name)
    return {"artifact": str(path), "sha256": digest, "policy": policy, "functions": len(reached),
            "inherited_stack_upper_bound": maximum, "metadata": metadata}


def audit_artifact(artifact_path, policy_role, build_root):
    """Standalone per-DSO gate for the existing two auditors; fixed TU inventory."""
    require(policy_role in ("production", "fault"), "unreviewed artifact role")
    build = Path(build_root).resolve(strict=True)
    expected = build / "lib" / ("libsubathon_schema_transport_v1.so" if policy_role == "production"
                                else "libsubathon_schema_fault_v1.so")
    require(Path(artifact_path).resolve(strict=True) == expected.resolve(strict=True), "artifact does not match the fixed build role")
    if policy_role == "production":
        paths = [build / "tools/schema_validation/native/CMakeFiles/subathon_schema_transport_v1.dir/transport_entry.cpp.ci"]
    else:
        target = build / "tests/schema_validation/CMakeFiles/subathon_schema_fault_v1.dir"
        paths = [target / "__/__/tools/schema_validation/native/transport_entry.cpp.ci",
                 target / "native_fault_fixture.cpp.ci"]
    base = runpy.run_path(str(Path(__file__).with_name("audit_native_foundation.py")), run_name="owner_static_base")
    return _audit_artifact(expected, paths, RAW if policy_role == "production" else FAULT, base)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", required=True)
    parser.add_argument("--role", choices=("production", "fault"))
    args = parser.parse_args()
    for role in (args.role,) if args.role else ("production", "fault"):
        name = "libsubathon_schema_transport_v1.so" if role == "production" else "libsubathon_schema_fault_v1.so"
        audit_artifact(Path(args.build_root) / "lib" / name, role, args.build_root)
    print("Closed owner graphs/stack metadata checked; no input artifact executed")


if __name__ == "__main__":
    main()
