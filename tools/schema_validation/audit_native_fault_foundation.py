#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Closed static checks for the reviewed nonvalidating native fault foundations."""
import argparse
import hashlib
from pathlib import Path
import re
import runpy

BASE = runpy.run_path(str(Path(__file__).with_name('audit_native_foundation.py')),
                     run_name='native_fault_static_base')
require, command, disassembly = (BASE[name] for name in ('require', 'command', 'disassembly'))
PRIMITIVE = '(anonymous namespace)::primitive_child(unsigned int, int const*)'
DRIVER = '(anonymous namespace)::Driver::start(unsigned int, int const*)'
WAKE = '(anonymous namespace)::wake((anonymous namespace)::Shared*)'
COMMON = {
    'schema_foundation::raw(long, long, long, long, long, long, long)',
    'schema_foundation::finish(int)',
    'long schema_foundation::address<schema_foundation::KernelAction>(schema_foundation::KernelAction*)',
    'long schema_foundation::address<unsigned long>(unsigned long*)',
    'long schema_foundation::address<char const>(char const*)',
}
PRIMITIVE_HELPERS = COMMON | {
    PRIMITIVE, WAKE,
    'long schema_foundation::address<char>(char*)',
    'long schema_foundation::address<timespec>(timespec*)',
    'long schema_foundation::address<unsigned int>(unsigned int*)',
    '(anonymous namespace)::state()',
    '(anonymous namespace)::poison((anonymous namespace)::Shared*)',
    '(anonymous namespace)::real_clock()',
    '(anonymous namespace)::encoded(subathon::schema_validation::native::Phase)',
    '(anonymous namespace)::append((anonymous namespace)::Shared*, subathon::schema_validation::native::fault::Writer, subathon::schema_validation::native::ObservationKind, unsigned int, long, long, long)',
    'subathon::schema_validation::native::fault::record(subathon::schema_validation::native::fault::Writer, subathon::schema_validation::native::Observation const&)',
    'subathon::schema_validation::native::fault::child_ready_and_hold()',
    '(anonymous namespace)::Shared* (anonymous namespace)::load<(anonymous namespace)::Shared*>((anonymous namespace)::Shared* const*)',
    'void (anonymous namespace)::store<unsigned int>(unsigned int*, unsigned int)',
    'unsigned int (anonymous namespace)::load<unsigned int>(unsigned int const*)',
}
NEGATIVE_HELPERS = COMMON | {
    'main', 'long schema_foundation::address<unsigned char>(unsigned char*)',
    '(anonymous namespace)::literal(char const*, char const*)',
    '(anonymous namespace)::positive_decimal(char const*)',
    '(anonymous namespace)::send(int, char const*, unsigned int)',
    '(anonymous namespace)::close_checked(int)',
    '(anonymous namespace)::put32(unsigned char*, unsigned int&, long)',
    '(anonymous namespace)::put64(unsigned char*, unsigned int&, unsigned long)',
    '(anonymous namespace)::dot_name(unsigned char const*, unsigned int)',
    '(anonymous namespace)::fd_name(unsigned char const*, unsigned int)',
    '(anonymous namespace)::report()', '(anonymous namespace)::protocol(unsigned int)',
} | {f'void (anonymous namespace)::send<{n}ul>(int, char const (&) [{n}ul])'
     for n in (2, 3, 14, 15, 20, 24, 25, 35, 36, 37, 41, 44)}
EXPORTS = {
    'subathon_schema_transport_v1', 'subathon_schema_fault_arm_v1',
    'subathon_schema_fault_snapshot_v1', 'subathon_schema_fault_report_v1',
    'subathon_schema_fault_fallback_v1',
}


def driver_entry(functions):
    """Check the native helper's exact successful zero-PID path into raw code."""
    drivers = [item for item in functions.values() if item[1] == DRIVER]
    require(len(drivers) == 1, 'primitive driver missing/ambiguous')
    instructions = drivers[0][2]
    calls = [i for i, item in enumerate(instructions)
             if item[1].startswith('call') and item[2].endswith('<syscall@plt>')]
    require(len(calls) == 1, 'primitive clone syscall missing/ambiguous')
    i = calls[0]
    require(i >= 2 and i + 12 < len(instructions), 'truncated primitive clone edge')
    require(instructions[i-2][1:] == ('mov', '$0x1b3,%edi')
            and instructions[i-1][1:] == ('mov', '$0x0,%eax'), 'primitive clone prefix changed')
    edge = instructions[i+1:i+12]
    saved = re.fullmatch(r'(-0x[0-9a-f]+\(%rbp\)),%rdx', edge[0][2])
    require(edge[0][1] == 'mov' and saved is not None, 'primitive this pointer changed')
    require(edge[1][1:] == ('mov', '%rax,(%rdx)')
            and edge[2][1:] == ('mov', saved.group(1) + ',%rax')
            and edge[3][1:] == ('mov', '(%rax),%rax')
            and edge[4][1:] == ('test', '%rax,%rax'), 'primitive saved-PID comparison changed')
    branch = BASE['TARGET'].fullmatch(edge[5][2])
    require(edge[5][1] == 'jne' and branch is not None
            and int(branch.group(1), 16) == instructions[i+12][0],
            'primitive nonzero branch does not skip exact raw child entry')
    require(edge[6][1] == 'mov'
            and re.fullmatch(r'-0x[0-9a-f]+\(%rbp\),%rdx', edge[6][2])
            and edge[7][1] == 'mov'
            and re.fullmatch(r'-0x[0-9a-f]+\(%rbp\),%eax', edge[7][2])
            and edge[8][1:] == ('mov', '%rdx,%rsi')
            and edge[9][1:] == ('mov', '%eax,%edi')
            and edge[10][1].startswith('call')
            and edge[10][2].endswith('<' + PRIMITIVE + '>'),
            'primitive child has an unreviewed instruction before raw entry')
    target = BASE['TARGET'].fullmatch(edge[10][2])
    require(target is not None and int(target.group(1), 16) in functions
            and functions[int(target.group(1), 16)][1] == PRIMITIVE,
            'primitive entry is not an audited function boundary')
    probe_name = '(anonymous namespace)::probe(bool, (anonymous namespace)::Controller*)'
    probes = [item for item in functions.values() if item[1] == probe_name]
    require(len(probes) == 1, 'binding-admitted probe missing/ambiguous')
    old_calls = [item[2] for item in probes[0][2]
                 if item[1].startswith('call') and item[2].endswith('<syscall@plt>')]
    require(old_calls == [instructions[i][2]], 'primitive and binding probe use different syscall stubs')
    return {'driver': drivers[0][0], 'child': int(target.group(1), 16)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--foundation', required=True)
    parser.add_argument('--child', required=True)
    parser.add_argument('--fault', required=True)
    options = parser.parse_args()
    foundation, child, fault = map(BASE['elf'], (options.foundation, options.child, options.fault))
    functions = disassembly(foundation)
    # Keep the existing entry/GOT proof mandatory even when invoked separately.
    print('clone_entry_binding:', BASE['probe_entry_binding'](
        functions, command('readelf', '-Wr', str(foundation))))
    print('primitive_entry:', driver_entry(functions))
    print('primitive_child:', BASE['reachable'](
        functions, lambda n: n == PRIMITIVE, lambda n: n in PRIMITIVE_HELPERS))
    print('negative_exec:', BASE['reachable'](
        disassembly(child), lambda n: n == 'main', lambda n: n in NEGATIVE_HELPERS))
    names = command('nm', '-D', '--defined-only', str(fault)).splitlines()
    require(len(names) == 5 and {line.split()[-1] for line in names} == EXPORTS,
            'fault artifact does not have exactly the five reviewed exports')
    dynamic = command('readelf', '-dW', str(fault))
    require('BIND_NOW' in dynamic or re.search(r'FLAGS_1.*\bNOW\b', dynamic), 'fault artifact is not eager')
    symbols = command('nm', '--defined-only', '--demangle', str(fault))
    require('_GLOBAL__sub_I' not in symbols and '__static_initialization_and_destruction' not in symbols,
            'unexpected fault C++ initialization')
    print('fault_owner:', BASE['owner_proof'](fault, 'fault'))
    for path in (foundation, child, fault):
        print('sha256', hashlib.sha256(path.read_bytes()).hexdigest(), path)
    print('Static fault-foundation graphs checked; no input artifact executed')


if __name__ == '__main__':
    main()
