#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent literal controls for the owner build audit; never touch an ELF."""

import copy
import importlib.util
from pathlib import Path
import unittest


CI = '''graph: { title: "fixture.cpp"
node: { title: "entry" label: "entry()\\nfixture.cpp:1:1\\n16 bytes (static)" }
node: { title: "fixture.cpp:_ZN1AC1Ev" label: "A::A()\\nfixture.cpp:2:1" shape : triangle }
node: { title: "fixture.cpp:_ZN1AC2Ev" label: "A::A()\\nfixture.cpp:2:1\\n24 bytes (static)" }
edge: { sourcename: "entry" targetname: "fixture.cpp:_ZN1AC1Ev" label: "fixture.cpp:1:2" }
edge: { sourcename: "fixture.cpp:_ZN1AC1Ev" targetname: "fixture.cpp:_ZN1AC2Ev" label: "fixture.cpp:2:1" }
}
'''
SU = "fixture.cpp:1:1:entry()\t16\tstatic\nfixture.cpp:2:1:A::A()\t24\tstatic\n"
RAW_POLICY = "subathon::schema_validation::native::RawOps"
FAULT_POLICY = "subathon::schema_validation::native::fault::FaultOps"
RAW_CLONE = ("long subathon::schema_validation::native::RawOps::syscall<(subathon::schema_validation::native::KernelCall)8>"
             "(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)")
FAULT_CLONE = ("long subathon::schema_validation::native::fault::FaultOps::syscall<(subathon::schema_validation::native::KernelCall)8>"
               "(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)")
RAW_FUNCTION = "schema_foundation::raw(long, long, long, long, long, long, long)"
ENTERED = "subathon::schema_validation::native::fault::FaultOps::entered_child()"


def load_tool():
    path = Path(__file__).parents[2] / "tools/schema_validation/audit_native_owner.py"
    spec = importlib.util.spec_from_file_location("owner_audit_literal_target", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class NativeOwnerAuditControls(unittest.TestCase):
    def test_stack_pointer_comparison_is_read_only_and_writes_still_refuse(self):
        audit = load_tool()
        # Literal hosted GCC11/13 stack-clash comparison; CMP writes only flags.
        audit.fixed_rsp([(0x100, "cmp", "%r11,%rsp")])
        audit.fixed_rsp([(0x100, "cmp", "%r11,%rbp")])
        for mnemonic, operands in (("mov", "%r11,%rsp"),
                                   ("sub", "%r11,%rsp"),
                                   ("xchg", "%r11,%rsp"),
                                   ("cmpxchg", "%r11,%rsp"),
                                   ("mov", "%r11,%rbp")):
            with self.subTest(mnemonic=mnemonic, operands=operands), self.assertRaises(RuntimeError):
                audit.fixed_rsp([(0x100, mnemonic, operands)])

    def test_literal_mangled_identity_alias_requires_same_elf_frame(self):
        audit = load_tool()
        symbols = {"entry": 0x100, "_ZN1AC1Ev": 0x200, "_ZN1AC2Ev": 0x200}
        frames, graph = audit.compiler_graph([(CI, SU)], symbols)
        self.assertEqual(frames, {0x100: (16, "static"), 0x200: (24, "static")})
        self.assertEqual(graph, {0x100: {0x200}})
        self.assertEqual(audit.stack_bound(frames, graph, 0x100), 48)
        with self.assertRaises(RuntimeError):
            audit.compiler_graph([(CI, SU)], {"entry": 0x100, "_ZN1AC1Ev": 0x201, "_ZN1AC2Ev": 0x200})
        with self.assertRaises(RuntimeError):
            audit.compiler_graph([(CI, SU.replace("\t24\t", "\t25\t"))], symbols)
        with self.assertRaises(RuntimeError):
            audit.parse_ci(CI.replace('shape : triangle', 'unknown : triangle'))

    def test_missing_dynamic_external_cycle_and_variable_rsp_refuse(self):
        audit = load_tool()
        rows = (({1: (16, "static")}, {1: {2}}),
                ({1: (16, "static"), 2: (24, "dynamic")}, {1: {2}}),
                ({1: (16, "static")}, {1: {"malloc"}}),
                ({1: (16, "static"), 2: (24, "static")}, {1: {2}, 2: {1}}))
        for frames, graph in rows:
            with self.subTest(graph=graph), self.assertRaises(RuntimeError):
                audit.stack_bound(frames, graph, 1)
        audit.fixed_rsp([(1, "push", "%rbp"), (2, "mov", "%rsp,%rbp"),
                         (3, "sub", "$0x20,%rsp"), (4, "add", "$0x10,%rsp"), (5, "leave", ""), (6, "ret", "")])
        for instruction in ((1, "sub", "%rax,%rsp"), (1, "mov", "%rax,%rsp"),
                            (1, "xchg", "%rsp,%rax"), (1, "mov", "%rax,%rbp"),
                            (1, "sub", "$0x10000,%rsp")):
            with self.subTest(instruction=instruction), self.assertRaises(RuntimeError):
                audit.fixed_rsp([instruction])
        self.assertNotIn("malloc@plt", audit.allowed_names(RAW_POLICY))
        self.assertNotIn("subathon::schema_validation::native::detail::unreviewed()", audit.allowed_names(RAW_POLICY))
        for policy in (RAW_POLICY, FAULT_POLICY):
            for number, member in ((15, "rt_sigaction"), (16, "rt_sigprocmask")):
                # Literal enum positions, independently read from the frozen header.
                expected = ("long " + policy + "::syscall<(subathon::schema_validation::native::KernelCall)"
                            + str(number) + ">(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)")
                with self.subTest(policy=policy, child_call=member):
                    self.assertIn(expected, audit.child_names(policy))
            for number in (0, 7, 8, 17, 18):
                forbidden = ("long " + policy + "::syscall<(subathon::schema_validation::native::KernelCall)"
                             + str(number) + ">(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long)")
                with self.subTest(policy=policy, parent_call=number):
                    self.assertNotIn(forbidden, audit.child_names(policy))
        monotonic = "subathon::schema_validation::native::fault::monotonic_ns()"
        checkpoint = "subathon::schema_validation::native::fault::checkpoint(subathon::schema_validation::native::Phase, long)"
        self.assertTrue(audit.owner_atomic(monotonic, "lock", "cmpxchg %rcx,(%rsi)"))
        self.assertTrue(audit.owner_atomic(checkpoint, "lock", "cmpxchg %ecx,(%rsi)"))
        for context, mnemonic, operand in (
                ("subathon::schema_validation::native::RawOps::monotonic_ns()", "lock", "cmpxchg %rcx,(%rsi)"),
                (monotonic, "lock", "cmpxchg %ecx,(%rsi)"),
                (checkpoint, "lock", "cmpxchg %rcx,(%rsi)"),
                (monotonic, "lock", "cmpxchg %rdx,(%rsi)"),
                (monotonic, "lock", "cmpxchg %rcx,(%rdi)"),
                (checkpoint, "lock", "cmpxchg %ecx,0x8(%rsi)"),
                (monotonic, "cmpxchg", "%rcx,(%rsi)"),
                (monotonic, "lock", "cmpxchg %rcx,%fs:(%rsi)")):
            with self.subTest(context=context, mnemonic=mnemonic, operand=operand):
                self.assertFalse(audit.owner_atomic(context, mnemonic, operand))
        self.assertTrue(audit.owner_instruction(monotonic, "lock", "cmpxchg %rcx,(%rsi)"))
        self.assertTrue(audit.owner_instruction(checkpoint, "lock", "cmpxchg %ecx,(%rsi)"))
        parents = (
            "subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::RawOps>::receive()",
            "subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::RawOps>::pinned_exit(int)",
            "subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::fault::FaultOps>::receive()",
            "subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::fault::FaultOps>::pinned_exit(int)",
        )
        for parent in parents:
            with self.subTest(parent_scalar=parent):
                self.assertTrue(audit.owner_parent_scalar(parent, "cwtl", ""))
                self.assertTrue(audit.owner_instruction(parent, "cwtl", ""))
                self.assertNotIn(parent, audit.child_names(RAW_POLICY))
                self.assertNotIn(parent, audit.child_names(FAULT_POLICY))
        for context, mnemonic, operand in (
                ("subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::RawOps>::child()", "cwtl", ""),
                ("subathon::schema_validation::native::detail::Owner<subathon::schema_validation::native::fault::FaultOps>::child()", "cwtl", ""),
                (monotonic, "cwtl", ""),
                (parents[0], "cltq", ""),
                (parents[0], "cwtl", "%eax"),
                (parents[1], "cwtl", "%fs:0x28"),
                (parents[2], "cwtl", " ")):
            with self.subTest(context=context, mnemonic=mnemonic, operand=operand):
                self.assertFalse(audit.owner_parent_scalar(context, mnemonic, operand))
                self.assertFalse(audit.owner_instruction(context, mnemonic, operand))
        # Literal fixed GNU allocations and RBP-relative epilogues, not dynamic RSP.
        audit.fixed_rsp([(1, "add", "$0xffffffffffffff80,%rsp"),
                         (2, "sub", "$0xffffffffffffff80,%rsp"),
                         (3, "lea", "-0x10(%rbp),%rsp"), (4, "lea", "-0x18(%rbp),%rsp")])
        for instruction in ((1, "add", "$0x10000000000000000,%rsp"),
                            (1, "sub", "$0xffffffffffff7ff8,%rsp"),
                            (1, "add", "$0xffffffffffffff81,%rsp"),
                            (1, "lea", "-0x8008(%rbp),%rsp"),
                            (1, "lea", "-0x11(%rbp),%rsp"),
                            (1, "lea", "-0x10(%rbp,%rax,1),%rsp"),
                            (1, "lea", "-0x10(%rax),%rsp"),
                            (1, "lea", "0x10(%rbp),%rsp"),
                            (1, "lea", "-0x0(%rbp),%rsp")):
            with self.subTest(fixed_stack_refusal=instruction), self.assertRaises(RuntimeError):
                audit.fixed_rsp([instruction])
        for instruction in ((1, "add", "$0x080,%rsp"),
                            (1, "sub", "$0x00000000000000000080,%rsp"),
                            (1, "lea", "-0x010(%rbp),%rsp")):
            with self.subTest(noncanonical_stack=instruction), self.assertRaises(RuntimeError):
                audit.fixed_rsp([instruction])

    def test_whole_32768_boundary_keeps_inherited_parent_frames_at_child(self):
        audit = load_tool()
        # ABI entry16 + parent32000 + birth128 + child600 +3 return slots8.
        frames = {1: (16, "static"), 2: (32000, "static"),
                  3: (128, "dynamic,bounded"), 4: (600, "static")}
        graph = {1: {2}, 2: {3}, 3: {4}}
        self.assertEqual(audit.stack_bound(frames, graph, 1), 32768)
        with self.assertRaises(RuntimeError):
            audit.stack_bound({**frames, 4: (601, "static")}, graph, 1)
        # A child-only 601-byte result must not replace the inherited-root proof.
        self.assertEqual(audit.stack_bound({4: (601, "static")}, {}, 4), 601)

    def test_literal_common_zero_edge_and_unchanged_raw_return(self):
        audit = load_tool()
        functions = {
            0x100: (0x100, "birth", [(0x100, "call", "200 <policy>"),
                (0x105, "add", "$0x10,%rsp"), (0x109, "mov", "%rax,-0x18(%rbp)"),
                (0x10d, "cmpq", "$0x0,-0x18(%rbp)"), (0x112, "jne", "122 <birth+0x22>"),
                (0x114, "mov", "-0x28(%rbp),%rax"), (0x118, "mov", "%rax,%rdi"),
                (0x11b, "call", "300 <child>"), (0x122, "ret", "")]),
            0x200: (0x200, "policy", [(0x200, "syscall", ""),
                (0x202, "mov", "%rax,-0x8(%rbp)"), (0x206, "mov", "-0x8(%rbp),%rax"),
                (0x20a, "pop", "%rbp"), (0x20b, "ret", "")]),
            0x300: (0x300, "child", [(0x300, "syscall", ""), (0x302, "jmp", "300 <child>")]),
        }
        self.assertEqual(audit.zero_edge(functions, "birth", "policy", "child"), (0x200, 0x300))
        audit.raw_return(functions[0x200][2], 1)
        for index, replacement in ((2, (0x109, "mov", "%rdx,-0x18(%rbp)")),
                                   (3, (0x10d, "cmpq", "$0x0,-0x20(%rbp)")),
                                   (4, (0x112, "jne", "123 <birth+0x23>"))):
            corrupted = copy.deepcopy(functions)
            corrupted[0x100][2][index] = replacement
            with self.subTest(index=index), self.assertRaises(RuntimeError):
                audit.zero_edge(corrupted, "birth", "policy", "child")
        with self.assertRaises(RuntimeError):
            audit.raw_return([(1, "mov", "%rax,-0x8(%rbp)"), (2, "mov", "$0x0,%rax"), (3, "ret", "")], 0)
        corrupted = copy.deepcopy(functions)
        corrupted[0x300][2][-1] = (0x302, "ret", "")
        with self.assertRaises(RuntimeError):
            audit.zero_edge(corrupted, "birth", "policy", "child")

    def test_literal_policy_clone_edges_keep_real_zero_and_skip_parent_adoption(self):
        audit = load_tool()
        raw = [(0x200, "mov", "$0x1b3,%eax"), (0x205, "syscall", ""),
               (0x207, "mov", "%rax,-0x8(%rbp)"), (0x20b, "mov", "-0x8(%rbp),%rax"),
               (0x20f, "pop", "%rbp"), (0x210, "ret", "")]
        audit.policy_zero_edge({0x200: (0x200, RAW_CLONE, raw)}, RAW_POLICY)
        with self.assertRaises(RuntimeError):
            audit.policy_zero_edge({0x200: (0x200, RAW_CLONE, [(0x200, "mov", "$0x0,%eax"), *raw[1:]])}, RAW_POLICY)
        fault = [(0x400, "mov", "$0x1b3,%edi"), (0x405, "call", "200 <raw>"),
            (0x40a, "add", "$0x10,%rsp"), (0x40e, "mov", "%rax,-0x20(%rbp)"),
            (0x412, "mov", "-0x28(%rbp),%rax"), (0x416, "movl", "$0x8,0x1b0(%rax)"),
            (0x420, "mov", "-0x28(%rbp),%rax"), (0x424, "mov", "-0x38(%rbp),%rdx"),
            (0x428, "mov", "%rdx,0x1b8(%rax)"), (0x42f, "mov", "-0x28(%rbp),%rax"),
            (0x433, "mov", "-0x20(%rbp),%rdx"), (0x437, "mov", "%rdx,0x1c0(%rax)"),
            (0x43e, "cmpq", "$0x0,-0x20(%rbp)"), (0x443, "jne", "453 <parent>"),
            (0x445, "mov", "-0x28(%rbp),%rax"), (0x449, "mov", "%rax,%rdi"),
            (0x44c, "call", "600 <entered_child>"), (0x451, "jmp", "460 <return_zero>"),
            (0x453, "cmpq", "$0x0,-0x20(%rbp)"), (0x458, "jmp", "460 <return_zero>"),
            (0x460, "mov", "-0x20(%rbp),%rax"), (0x464, "leave", ""), (0x465, "ret", "")]
        wrapper = [(0x200, "syscall", ""), (0x202, "mov", "%rax,-0x8(%rbp)"),
                   (0x206, "mov", "-0x8(%rbp),%rax"), (0x20a, "pop", "%rbp"), (0x20b, "ret", "")]
        functions = {0x400: (0x400, FAULT_CLONE, fault), 0x200: (0x200, RAW_FUNCTION, wrapper),
                     0x600: (0x600, ENTERED, [(0x600, "ret", "")])}
        audit.policy_zero_edge(functions, FAULT_POLICY)
        for index, replacement in ((0, (0x400, "mov", "$0x0,%edi")),
                                   (12, (0x43e, "cmpq", "$0x0,-0x18(%rbp)")),
                                   (17, (0x451, "jmp", "453 <parent>"))):
            corrupted = copy.deepcopy(functions)
            corrupted[0x400][2][index] = replacement
            with self.subTest(index=index), self.assertRaises(RuntimeError):
                audit.policy_zero_edge(corrupted, FAULT_POLICY)


if __name__ == "__main__":
    unittest.main(verbosity=2)
