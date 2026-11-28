/*
 * loader.c -- place an assembled program into CPU memory (SPEC 4.3)
 *
 * Purpose : Assignment 2 (how the executable is loaded into memory).
 *
 *   TEXT  0x00-0x5F  <- prog->text   (machine code)
 *   DATA  0x60-0x7F  <- prog->data   (globals)
 *   HEAP  0x80-0x9F     all zero     (our programs use none)
 *   STACK 0xA0-0xFF     all zero     (SP = FP = 0x100: empty)
 */
#include "loader.h"

/*
 * load_error: fill in a loader error.
 */
static int load_error(SimError *err, int kind, int value, int addr)
{
    err->kind = kind;
    err->line = 0;
    err->value = value;
    err->addr = addr;
    err->name[0] = '\0';
    return -1;
}

/*
 * load_program: check the program, reset the CPU, copy the segments in.
 * Afterwards the CPU is at step 0 ("program loaded into memory").
 * Returns 0 on success, -1 on error (err filled in, CPU unchanged).
 */
int load_program(const Program *prog, CPU *cpu, SimError *err)
{
    int i;

    /* 1. Validate */
    if (prog->text_len < 0 || prog->data_len < 0) {
        return load_error(err, ERR_BAD_OBJECT, 0, 0);
    }
    if (prog->text_len > TEXT_SIZE) {
        return load_error(err, ERR_TEXT_TOO_LARGE, prog->text_len, 0);
    }
    if (prog->data_len > DATA_SIZE) {
        return load_error(err, ERR_DATA_TOO_LARGE, prog->data_len, 0);
    }
    if (prog->entry < TEXT_BASE || prog->entry >= TEXT_BASE + prog->text_len) {
        return load_error(err, ERR_ENTRY_RANGE, 0, prog->entry);
    }

    /* 2. Reset: all memory 0, FP = SP = 0x100 */
    cpu_reset(cpu);

    /* 3. Copy TEXT and DATA; HEAP and STACK stay 0 */
    for (i = 0; i < prog->text_len; i++) {
        cpu->mem[TEXT_BASE + i] = prog->text[i];
    }
    for (i = 0; i < prog->data_len; i++) {
        cpu->mem[DATA_BASE + i] = prog->data[i];
    }

    /* 4. Start at the entry point (label main) */
    cpu->pc = prog->entry;
    cpu->regs[REG_SP] = STACK_TOP;
    cpu->regs[REG_FP] = STACK_TOP;
    return 0;
}
