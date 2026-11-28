/*
 * loader.h -- place an assembled program into CPU memory (4 segments)
 *
 * Purpose : Assignment 2 (how the executable is loaded into memory).
 */
#ifndef LOADER_H
#define LOADER_H

#include "cpu.h"

int load_program(const Program *prog, CPU *cpu, SimError *err);

#endif
