#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fixed static-gate controls; review source before first execution.

These literal graphs/text are not native programs or validation substitutes.
Import defines helpers/tests only; no candidate import, binutils or ELF discovery.
"""

import importlib.util
from pathlib import Path
import unittest
from unittest import mock


def load_audit():
    path = Path(__file__).parents[2] / "tools/schema_validation/audit_native_foundation.py"
    spec = importlib.util.spec_from_file_location("schema_native_audit_controls_target", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)  # Defines audit functions, never calls main.
    return module


def libc_literal():
    # Independently transcribed x86-64 generic-syscall success prefix, not output
    # of the audit implementation. The TLS errno branch is beyond the first ret.
    return {
        0x2000: (0x2000, "syscall@@GLIBC_2.2.5", [
            (0x2000, "mov", "%rdi,%rax"),
            (0x2003, "mov", "%rsi,%rdi"),
            (0x2006, "mov", "%rdx,%rsi"),
            (0x2009, "mov", "%rcx,%rdx"),
            (0x200c, "mov", "%r8,%r10"),
            (0x200f, "mov", "%r9,%r8"),
            (0x2012, "mov", "0x8(%rsp),%r9"),
            (0x2017, "syscall", ""),
            (0x2019, "cmp", "$0xfffffffffffff001,%rax"),
            (0x201f, "jae", "2022 <syscall@@GLIBC_2.2.5+0x22>"),
            (0x2021, "ret", ""),
            (0x2022, "mov", "%fs:0x0,%rcx"),
            (0x202b, "ret", ""),
        ]),
    }


class NativeAuditControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.audit = load_audit()

    def graph(self, instructions, extra=None):
        graph = {
            0x1000: (0x1000, "schema_foundation::child(schema_foundation::ChildPlan const&)", instructions),
        }
        if extra:
            graph.update(extra)
        return graph

    def reachable(self, graph):
        return self.audit.reachable(
            graph, lambda name: name == "schema_foundation::child(schema_foundation::ChildPlan const&)",
            self.audit.raw_helper,
        )

    def test_internal_raw_graph_has_exact_reachable_names(self):
        graph = self.graph([
            (0x1000, "call", "1100 <schema_foundation::raw(long, long, long, long, long, long, long)>"),
            (0x1005, "jne", "1007 <schema_foundation::child(schema_foundation::ChildPlan const&)+0x7>"),
            (0x1007, "ret", ""),
        ], {0x1100: (0x1100, "schema_foundation::raw(long, long, long, long, long, long, long)", [
            (0x1100, "mov", "%rdi,%rax"), (0x1103, "syscall", ""),
            (0x1105, "ret", ""),
        ])})
        self.assertEqual(self.reachable(graph), [
            "schema_foundation::child(schema_foundation::ChildPlan const&)", "schema_foundation::raw(long, long, long, long, long, long, long)",
        ])

    def test_plt_and_unknown_external_transfers_refuse(self):
        rows = (
            ("call", "2000 <malloc@plt>", {0x2000: (0x2000, "malloc@plt", [(0x2000, "ret", "")])}),
            ("call", "2000 <unresolved_external>", {}),
            ("jmp", "2000 <memcpy@plt>", {0x2000: (0x2000, "memcpy@plt", [(0x2000, "ret", "")])}),
            ("call", "2000 <schema_foundation::unreviewed()>", {
                0x2000: (0x2000, "schema_foundation::unreviewed()", [(0x2000, "ret", "")]),
            }),
        )
        for index, (mnemonic, operands, extra) in enumerate(rows):
            with self.subTest(row=index):
                with self.assertRaises(RuntimeError):
                    self.reachable(self.graph([(0x1000, mnemonic, operands)], extra))

    def test_indirect_or_unresolved_transfers_refuse(self):
        rows = (("callq", "*%rax"), ("jmpq", "*0x8(%rax)"), ("call", "2000"))
        for index, (mnemonic, operands) in enumerate(rows):
            with self.subTest(row=index):
                with self.assertRaises(RuntimeError):
                    self.reachable(self.graph([(0x1000, mnemonic, operands)]))
        # 1001 lies INSIDE the three-byte mov, rather than at an admitted
        # instruction boundary. A range-only local-branch test is insufficient.
        middle = self.graph([
            (0x1000, "mov", "%rdi,%rax"),
            (0x1003, "jmp", "1001 <schema_foundation::child(schema_foundation::ChildPlan const&)+0x1>"),
            (0x1005, "ret", ""),
        ])
        with self.assertRaises(RuntimeError):
            self.reachable(middle)

    def test_tls_unreviewed_instructions_and_traps_refuse(self):
        rows = (
            ("mov", "%fs:0x28,%rax"), ("mov", "%gs:0x0,%rax"),
            ("int3", ""), ("sysenter", ""), ("ud2", ""), ("xsave", "(%rax)"),
        )
        for index, (mnemonic, operands) in enumerate(rows):
            with self.subTest(row=index):
                with self.assertRaises(RuntimeError):
                    self.reachable(self.graph([(0x1000, mnemonic, operands)]))

    def test_malformed_text_and_missing_or_ambiguous_roots_refuse(self):
        texts = (
            "not an objdump function or instruction\n",
            "1000 <first>:\n 1000: ret\n1000 <second>:\n 1000: ret\n",
            "1000 <schema_foundation::child(schema_foundation::ChildPlan const&)>:\n 1000: ret\n 1001: (bad)\n",
            "1000 <schema_foundation::child(schema_foundation::ChildPlan const&)>:\n 1000: ret\n 1001: .byte 0xff\n",
            "1000 <schema_foundation::child(schema_foundation::ChildPlan const&)>:\n 1000: ret\n 1001: ???\n",
        )
        for index, text in enumerate(texts):
            with self.subTest(text=index):
                with mock.patch.object(self.audit, "command", return_value=text):
                    with self.assertRaises(RuntimeError):
                        self.audit.disassembly("literal-unused-path")
        root = "schema_foundation::child(schema_foundation::ChildPlan const&)"
        graphs = (
            {},
            {0x1000: (0x1000, root, [])},
            {0x1000: (0x1000, root, [(0x1000, "ret", "")]),
             0x2000: (0x2000, root, [(0x2000, "ret", "")])},
        )
        for index, graph in enumerate(graphs):
            with self.subTest(graph=index):
                with self.assertRaises(RuntimeError):
                    self.reachable(graph)

    def test_literal_objdump_text_preserves_graph(self):
        text = (
            "literal.elf: file format elf64-x86-64\n\n"
            "0000000000001000 <schema_foundation::child(schema_foundation::ChildPlan const&)>:\n"
            "    1000: call 1100 <schema_foundation::raw(long, long, long, long, long, long, long)>\n"
            "    1005: ret\n\n"
            "0000000000001100 <schema_foundation::raw(long, long, long, long, long, long, long)>:\n"
            "    1100: syscall\n"
            "    1102: ret\n"
        )
        with mock.patch.object(self.audit, "command", return_value=text) as command:
            graph = self.audit.disassembly("literal-unused-path")
        command.assert_called_once_with(
            "objdump", "-d", "-C", "--no-show-raw-insn", "literal-unused-path",
        )
        self.assertEqual(graph, {
            0x1000: (0x1000, "schema_foundation::child(schema_foundation::ChildPlan const&)", [
                (0x1000, "call", "1100 <schema_foundation::raw(long, long, long, long, long, long, long)>"),
                (0x1005, "ret", ""),
            ]),
            0x1100: (0x1100, "schema_foundation::raw(long, long, long, long, long, long, long)", [
                (0x1100, "syscall", ""), (0x1102, "ret", ""),
            ]),
        })
        self.assertEqual(self.reachable(graph), [
            "schema_foundation::child(schema_foundation::ChildPlan const&)", "schema_foundation::raw(long, long, long, long, long, long, long)",
        ])

    def test_literal_libc_success_prefix(self):
        with mock.patch.object(self.audit, "disassembly", return_value=libc_literal()) as disassembly:
            self.assertEqual(self.audit.syscall_return("literal-libc"), "syscall@@GLIBC_2.2.5")
        disassembly.assert_called_once_with("literal-libc")

    def test_corrupted_libc_prefixes_refuse(self):
        # Each row changes one independent ABI/control-flow premise. Expected
        # refusal is fixed here; no value is obtained from the audit's output.
        rows = (
            (4, (0x200c, "mov", "%r8,%r9")),
            (8, (0x2019, "cmp", "$0xfffffffffffff000,%rax")),
            (9, (0x201f, "jbe", "2022 <syscall@@GLIBC_2.2.5+0x22>")),
            (9, (0x201f, "jae", "2000 <syscall@@GLIBC_2.2.5>")),
            (3, (0x2009, "call", "3000 <external>")),
            (7, (0x2017, "nop", "")),
        )
        for index, (position, replacement) in enumerate(rows):
            graph = libc_literal()
            graph[0x2000][2][position] = replacement
            with self.subTest(row=index):
                with mock.patch.object(self.audit, "disassembly", return_value=graph):
                    with self.assertRaises(RuntimeError):
                        self.audit.syscall_return("literal-libc")
        no_return = libc_literal()
        no_return[0x2000][2][:] = no_return[0x2000][2][:10]
        duplicate = libc_literal()
        duplicate[0x3000] = (0x3000, "syscall", [(0x3000, "ret", "")])
        reordered = libc_literal()
        reordered[0x2000][2][0] = (0x2000, "mov", "%rsi,%rdi")
        reordered[0x2000][2][1] = (0x2003, "mov", "%rdi,%rax")

        def insert_literal(position, mnemonic, operands, width):
            # Rebase fixed fixture addresses after one fixed-width instruction.
            # No audit output or candidate code supplies data or expectations.
            graph = libc_literal()
            instructions = graph[0x2000][2]
            boundary = instructions[position][0]
            for index, (address, operation, values) in enumerate(instructions):
                if address >= boundary:
                    address += width
                if operation == "jae":
                    values = f"{0x2022 + width:x} <syscall@@GLIBC_2.2.5+0x{0x22 + width:x}>"
                instructions[index] = (address, operation, values)
            instructions.insert(position, (boundary, mnemonic, operands))
            return graph

        extra = insert_literal(7, "xor", "%eax,%eax", 2)
        tls = insert_literal(7, "mov", "%fs:0x28,%rax", 9)
        stack = insert_literal(7, "sub", "$0x8,%rsp", 4)
        delayed_compare = insert_literal(8, "nop", "", 1)
        for label, graph in (
            ("no-return", no_return), ("ambiguous-symbol", duplicate),
            ("reordered-register-moves", reordered), ("extra-rax-clobber", extra),
            ("tls-before-syscall", tls), ("modified-stack", stack),
            ("compare-not-immediate-after-syscall", delayed_compare),
        ):
            with self.subTest(row=label):
                with mock.patch.object(self.audit, "disassembly", return_value=graph):
                    with self.assertRaises(RuntimeError):
                        self.audit.syscall_return("literal-libc")


if __name__ == "__main__":
    unittest.main(verbosity=2)
