#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Literal new helper-edge/atomic controls; no ELF/native/process operation."""
import copy
from pathlib import Path
import runpy
import unittest

AUDIT = runpy.run_path(str(Path(__file__).resolve().parents[2] /
                          'tools/schema_validation/audit_native_fault_foundation.py'),
                      run_name='literal_fault_audit')
DRIVER, CHILD, WAKE = (AUDIT[name] for name in ('DRIVER', 'PRIMITIVE', 'WAKE'))
PROBE = '(anonymous namespace)::probe(bool, (anonymous namespace)::Controller*)'
LITERAL = {
    0x600: (0x600, DRIVER, [
        (0x600, 'mov', '$0x1b3,%edi'), (0x605, 'mov', '$0x0,%eax'),
        (0x610, 'call', '200 <syscall@plt>'),
        (0x615, 'mov', '-0xa8(%rbp),%rdx'), (0x61c, 'mov', '%rax,(%rdx)'),
        (0x61f, 'mov', '-0xa8(%rbp),%rax'), (0x626, 'mov', '(%rax),%rax'),
        (0x629, 'test', '%rax,%rax'), (0x62c, 'jne', '650 <' + DRIVER + '+0x50>'),
        (0x62e, 'mov', '-0xb8(%rbp),%rdx'), (0x635, 'mov', '-0xac(%rbp),%eax'),
        (0x63b, 'mov', '%rdx,%rsi'), (0x63e, 'mov', '%eax,%edi'),
        (0x640, 'call', '900 <' + CHILD + '>'), (0x650, 'ret', '')]),
    0x900: (0x900, CHILD, [(0x900, 'ret', '')]),
    0x100: (0x100, PROBE, [(0x100, 'call', '200 <syscall@plt>')]),
}


class NativeFaultAuditControls(unittest.TestCase):
    def test_literal_success_path_has_exact_boundary(self):
        self.assertEqual(AUDIT['driver_entry'](copy.deepcopy(LITERAL)),
                         {'driver': 0x600, 'child': 0x900})

    def test_wrong_clone_save_comparison_branch_or_callback_refuses(self):
        for index, mnemonic, operands in [
                (0, 'mov', '$0x39,%edi'), (3, 'mov', '%fs:0x28,%rdx'),
                (4, 'mov', '%eax,(%rdx)'), (5, 'mov', '-0xb0(%rbp),%rax'),
                (7, 'test', '%eax,%eax'), (8, 'je', '650 <' + DRIVER + '+0x50>'),
                (8, 'jne', '649 <' + DRIVER + '+0x49>'),
                (9, 'call', 'a00 <malloc@plt>'), (13, 'call', '901 <' + CHILD + '>')]:
            with self.subTest(index=index, operands=operands):
                graph = copy.deepcopy(LITERAL)
                graph[0x600][2][index] = (graph[0x600][2][index][0], mnemonic, operands)
                with self.assertRaises(RuntimeError):
                    AUDIT['driver_entry'](graph)

    def test_binding_probe_syscall_stub_must_match(self):
        for operations in [[], [(0x100, 'call', '201 <syscall@plt>')],
                           [(0x100, 'call', '200 <syscall@plt>'),
                            (0x105, 'call', '200 <syscall@plt>')]]:
            with self.subTest(operations=operations):
                graph = copy.deepcopy(LITERAL)
                graph[0x100][2][:] = operations
                with self.assertRaises(RuntimeError):
                    AUDIT['driver_entry'](graph)

    def test_atomic_sequence_increment_is_the_only_new_lock_operation(self):
        def check(name, operation):
            graph = {0x100: (0x100, name, [(0x100, *operation), (0x104, 'ret', '')])}
            return AUDIT['BASE']['reachable'](graph, lambda n: n == name, lambda n: n == name)
        self.assertEqual(check(WAKE, ('lock', 'addl $0x1,(%rax)')), [WAKE])
        for name, operation in [
                (WAKE, ('lock', 'addl $0x2,(%rax)')),
                (WAKE, ('lock', 'addl $0x1,(%rdx)')),
                (WAKE, ('lock', 'cmpxchgl %edx,(%rax)')),
                (CHILD, ('lock', 'addl $0x1,(%rax)')),
                (WAKE, ('mov', '%fs:0x28,%rax'))]:
            with self.subTest(name=name, operation=operation):
                with self.assertRaises(RuntimeError):
                    check(name, operation)

    def test_unreviewed_helpers_remain_refused_even_without_external_calls(self):
        for root, allowed, extra in [
                (CHILD, AUDIT['PRIMITIVE_HELPERS'], '(anonymous namespace)::unreviewed()'),
                ('main', AUDIT['NEGATIVE_HELPERS'], 'void (anonymous namespace)::send<26ul>(int, char const (&) [26ul])')]:
            graph = {0x100: (0x100, root, [(0x100, 'call', '200 <' + extra + '>')]),
                     0x200: (0x200, extra, [(0x200, 'ret', '')])}
            with self.subTest(extra=extra):
                with self.assertRaises(RuntimeError):
                    AUDIT['BASE']['reachable'](graph, lambda n: n == root, lambda n: n in allowed)


if __name__ == '__main__':
    unittest.main(verbosity=2)
