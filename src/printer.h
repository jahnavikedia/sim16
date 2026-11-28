/*
 * printer.h -- every printed view of the simulator (SPEC section 7)
 *
 * Purpose : Assignment 1: memory grid after every instruction (step blocks).
 *           Assignment 2: the four-segment view at load, after every CALL,
 *           and at the end, with the stack frames labelled.
 * All functions print directly with printf. "prev" is the snapshot to
 * compare with: changed values get a '*'. prev = 0 means no marks.
 */
#ifndef PRINTER_H
#define PRINTER_H

#include "cpu.h"

void print_error(const SimError *e);
void print_registers(const CPU *snap, const CPU *prev);
void print_memory_grid(const CPU *snap, const CPU *prev, const Program *prog, int full);
void print_step(const CPU *prev, const CPU *snap, const Program *prog, int full);
void print_segments(const CPU *snap, const CPU *prev, const Program *prog, const char *title);
void print_stack(const CPU *snap, const CPU *prev, const Program *prog);
void print_summary(const CPU *snap);
void print_listing(const Program *prog, const char *const *lines, int n_lines,
                   const char *name);
const char *text_label_at(const Program *prog, int addr);

#endif
