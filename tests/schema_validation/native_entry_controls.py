#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Literal static clone/GOT controls; no ELF, loader, syscall or subprocess."""
import copy
from pathlib import Path
import runpy
import unittest

AUDIT = runpy.run_path(str(Path(__file__).resolve().parents[2] /
                          'tools/schema_validation/audit_native_foundation.py'),
                      run_name='static_entry_controls')
PROBE = '(anonymous namespace)::probe(bool, (anonymous namespace)::Controller*)'
CHILD = 'schema_foundation::child(schema_foundation::ChildPlan const&)'
# Literal instructions independently transcribed from actual reviewed host22/24
# evidence. Addresses are deliberately small; the oracle is the identity/edge.
BASE = {
    0x100: (0x100, PROBE, [
        (0x100, 'mov', '$0x1b3,%edi'), (0x105, 'mov', '$0x0,%eax'),
        (0x110, 'call', '200 <syscall@plt>'),
        (0x115, 'mov', '%rax,-0x388(%rbp)'),
        (0x11c, 'cmpq', '$0x0,-0x388(%rbp)'),
        (0x124, 'jne', '135 <' + PROBE + '+0x35>'),
        (0x126, 'lea', '-0x370(%rbp),%rax'), (0x12d, 'mov', '%rax,%rdi'),
        (0x130, 'call', '400 <' + CHILD + '>'), (0x135, 'ret', '')]),
    0x200: (0x200, 'syscall@plt', [
        (0x200, 'endbr64', ''),
        (0x204, 'jmp', '*0xae425(%rip)        # bafb0 <syscall@GLIBC_2.2.5>'),
        (0x20a, 'nopw', '0x0(%rax,%rax,1)')]),
    0x300: (0x300, 'SchemaNativeFoundation_SyscallBindingToActualLibc_Test::TestBody()', [
        (0x300, 'mov', '0xa9147(%rip),%rax        # bafb0 <syscall@GLIBC_2.2.5>')]),
    0x400: (0x400, CHILD, [(0x400, 'ret', '')]),
}
RELOCS = '00000000000bafb0  000000cb00000006 R_X86_64_GLOB_DAT  0000000000000000 syscall@GLIBC_2.2.5 + 0\n'


class NativeEntryControls(unittest.TestCase):
    def check(self, functions, relocations=RELOCS):
        return AUDIT['probe_entry_binding'](functions, relocations)

    def rejected(self, mutate):
        functions = copy.deepcopy(BASE)
        mutate(functions)
        with self.assertRaises(RuntimeError):
            self.check(functions)

    def test_both_reviewed_plt_forms(self):
        for mnemonic, operand in [('jmp', '*0xae425(%rip)        # bafb0 <syscall@GLIBC_2.2.5>'),
                                  ('bnd', 'jmp *0xae425(%rip)        # bafb0 <syscall@GLIBC_2.2.5>')]:
            with self.subTest(mnemonic=mnemonic):
                functions = copy.deepcopy(BASE)
                functions[0x200][2][1] = (0x204, mnemonic, operand)
                self.assertEqual(self.check(functions),
                                 {'probe': 0x100, 'syscall_got': 0xbafb0, 'child': 0x400})

    def test_probe_lambda_is_not_another_probe(self):
        functions = copy.deepcopy(BASE)
        functions[0x500] = (0x500, PROBE + '::{lambda(int)#1}::operator()(int) const',
                            [(0x500, 'ret', '')])
        self.assertEqual(self.check(functions)['probe'], 0x100)

    def test_wrong_clone_number_save_or_comparison(self):
        for index, mnemonic, operand in [
                (0, 'mov', '$0x39,%edi'), (3, 'mov', '%eax,-0x388(%rbp)'),
                (4, 'cmpq', '$0x1,-0x388(%rbp)'), (4, 'cmpq', '$0x0,-0x390(%rbp)')]:
            with self.subTest(index=index, operand=operand):
                self.rejected(lambda f: f[0x100][2].__setitem__(index, (f[0x100][2][index][0], mnemonic, operand)))

    def test_wrong_child_branch_or_callback(self):
        for index, mnemonic, operand in [
                (5, 'je', '135 <' + PROBE + '+0x35>'),
                (5, 'jne', '134 <' + PROBE + '+0x34>'),
                (6, 'call', '600 <__errno_location@plt>'),
                (8, 'call', '401 <' + CHILD + '>')]:
            with self.subTest(index=index, operand=operand):
                self.rejected(lambda f: f[0x100][2].__setitem__(index, (f[0x100][2][index][0], mnemonic, operand)))

    def test_plt_has_no_extra_operation_or_alternate_symbol(self):
        for mnemonic, operand in [('mov', '%fs:0x0,%rax'),
                                  ('jmp', '*0x10(%rip) # bafb0 <syscall@GLIBC_9.9>'),
                                  ('call', '600 <resolve@plt>')]:
            with self.subTest(mnemonic=mnemonic, operand=operand):
                self.rejected(lambda f: f[0x200][2].__setitem__(1, (0x204, mnemonic, operand)))

    def test_relocation_must_be_the_same_resolved_slot(self):
        for text in [RELOCS.replace('bafb0', 'bafb8'),
                     RELOCS.replace('R_X86_64_GLOB_DAT', 'R_X86_64_JUMP_SLOT'),
                     RELOCS.replace('GLIBC_2.2.5', 'GLIBC_9.9'), RELOCS + RELOCS, '']:
            with self.subTest(relocations=text):
                with self.assertRaises(RuntimeError):
                    self.check(copy.deepcopy(BASE), text)

    def test_bootstrap_reference_must_match_the_clone_slot(self):
        for mnemonic, operand in [('mov', '0x10(%rip),%rax # bafb8 <syscall@GLIBC_2.2.5>'),
                                  ('lea', '0x10(%rip),%rax # bafb0 <syscall@GLIBC_2.2.5>')]:
            with self.subTest(operand=operand):
                self.rejected(lambda f: f[0x300][2].__setitem__(0, (0x300, mnemonic, operand)))


if __name__ == '__main__':
    unittest.main(verbosity=2)
