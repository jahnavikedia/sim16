/*
 * assembler.h -- SIM16 assembler (text -> machine code) and disassembler
 *
 * Purpose : both assignments: turns our assembly language into machine code
 *           (two passes), and machine code back into readable assembly.
 */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "isa.h"

#define DISASM_LEN 40   /* room for the longest disassembly text */

int  assemble(const char *const *lines, int n_lines, Program *prog, SimError *err);
void disassemble(unsigned short word, const Program *prog, char *out);

#endif
