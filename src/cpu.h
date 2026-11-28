/*
 * cpu.h -- SIM16 CPU state and the fetch/decode/execute interface
 *
 * Purpose : Assignment 1 (the CPU design) and Assignment 2 (CALL/RET and
 *           the stack that the recursive program uses). SPEC 3.4-3.6, 5.
 */
#ifndef CPU_H
#define CPU_H

#include "isa.h"

/* Event types: what the last instruction did that the printer cares about. */
#define EV_NONE 0
#define EV_CALL 1
#define EV_RET  2
#define EV_OUT  3
#define EV_HALT 4

/* Event of one step (SPEC 5.3). Unused fields are 0. */
typedef struct {
    int type;           /* EV_... */
    int target;         /* CALL: address jumped to */
    int ret_addr;       /* CALL: return address pushed */
    int arg;            /* CALL: argument at M[FP+2] after the call (-1 if none) */
    int value;          /* RET: R0 (return value); OUT: value printed */
} Event;

/*
 * The whole machine state (SPEC 5.1).
 * A snapshot is a plain struct copy:  CPU snap = cpu;
 * C copies arrays inside a struct, so the snapshot keeps its own memory and
 * registers, and later steps cannot change it.
 */
typedef struct {
    unsigned short mem[MEM_SIZE];   /* 256 words of memory */
    unsigned short regs[NUM_REGS];  /* R0..R7 (R6 = FP, R7 = SP) */
    int pc;                         /* address of the NEXT instruction */
    unsigned short ir;              /* last fetched instruction word */
    int exec_addr;                  /* where ir came from; -1 = none yet */
    int flag_z, flag_n;             /* Z = result was 0, N = bit 15 of result */
    int halted;                     /* 1 after HALT */
    int steps;                      /* instructions executed so far */
    int output[MAX_OUTPUT];         /* values printed by OUT */
    int n_output;
    Event last_event;               /* event of the last step */
} CPU;

void cpu_reset(CPU *cpu);
int  cpu_step(CPU *cpu, SimError *err);
int  cpu_run(CPU *cpu, int max_steps, SimError *err);

#endif
