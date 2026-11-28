/*
 * programs.c -- the assembly programs built into the simulator (SPEC 9)
 *
 * Purpose : Assignment 1: factorial (8! written in assembly) and
 *                         power (8^2, hand-translated from programs/power.c)
 *           Assignment 2: recursive (factorial, hand-translated from
 *                         programs/fact_recursive.c)
 * Each program is its source lines exactly as written (comments included).
 * Assembler error line numbers = index in the array + 1.
 */
#include "assignment.h"
#include "programs.h"
#include "text.h"

#if ASSIGNMENT == 0 || ASSIGNMENT == 1

/* Assignment 1: 8! written directly in SIM16 assembly (SPEC 9.1) */
static const char *const FACTORIAL_SRC[] = {
    "; factorial.asm -- Assignment 1, written directly in SIM16 assembly",
    "; Computes 8! = 40320 iteratively and stores it in `result`.",
    ".text",
    "main:",
    "        LD    R1, n          ; R1 = counter = n (8)",
    "        LDI   R0, 1          ; R0 = running product = 1",
    "loop:",
    "        MUL   R0, R1         ; product = product * counter",
    "        SUBI  R1, 1          ; counter = counter - 1   (sets Z when it hits 0)",
    "        JNZ   loop           ; repeat while counter != 0",
    "        ST    R0, result     ; result = product",
    "        OUT   R0             ; print result (I/O)",
    "        HALT",
    ".data",
    "n:        .word 8",
    "result:   .word 0"
};

/* Assignment 1: 8^2, hand-translated from programs/power.c (SPEC 9.2) */
static const char *const POWER_SRC[] = {
    "; power.asm -- Assignment 1, hand-translated from power.c",
    "; Computes base^exponent = 8^2 = 64",
    ".text",
    "main:",
    "        LDI   R0, 1          ; r = 1",
    "        LDI   R1, 0          ; i = 0",
    "        LD    R2, exponent   ; R2 = exponent",
    "        LD    R3, base       ; R3 = base",
    "loop:",
    "        CMP   R1, R2         ; i - exponent",
    "        JN    body           ; if (i < exponent) run the body",
    "        JMP   done           ; else exit the loop",
    "body:",
    "        MUL   R0, R3         ; r = r * base",
    "        ADDI  R1, 1          ; i++",
    "        JMP   loop",
    "done:",
    "        ST    R0, result     ; result = r",
    "        OUT   R0             ; printf (I/O)",
    "        HALT",
    ".data",
    "base:     .word 8",
    "exponent: .word 2",
    "result:   .word 0"
};

#endif

#if ASSIGNMENT == 0 || ASSIGNMENT == 2

/* Assignment 2: recursive factorial, hand-translated from
   programs/fact_recursive.c (SPEC 9.3) */
static const char *const RECURSIVE_SRC[] = {
    "; fact_recursive.asm -- Assignment 2, hand-translated from fact_recursive.c",
    ".text",
    "main:",
    "        LD    R1, n          ; R1 = n",
    "        PUSH  R1             ; push argument",
    "        CALL  fact           ; R0 = fact(n)",
    "        ADDI  SP, 1          ; caller pops the argument",
    "        ST    R0, result     ; result = fact(n)",
    "        OUT   R0             ; printf (I/O)",
    "        HALT",
    "fact:",
    "        LDR   R1, [FP+2]     ; R1 = k (argument)",
    "        CMPI  R1, 2          ; k - 2",
    "        JN    base           ; if (k <= 1) goto base",
    "        SUBI  R1, 1          ; R1 = k - 1",
    "        PUSH  R1             ; push argument k-1",
    "        CALL  fact           ; R0 = fact(k-1)",
    "        ADDI  SP, 1          ; pop argument",
    "        LDR   R1, [FP+2]     ; reload k (R1 was clobbered)",
    "        MUL   R0, R1         ; R0 = k * fact(k-1)",
    "        RET",
    "base:",
    "        LDI   R0, 1          ; return 1",
    "        RET",
    ".data",
    "n:        .word 5",
    "result:   .word 0"
};

#endif

/* This build's programs, in run order.
   n_lines = sizeof(whole array) / sizeof(one line) */
const ProgramSource PROGRAMS[] = {
#if ASSIGNMENT == 0 || ASSIGNMENT == 1
    { "factorial", "A1: 8! in assembly, memory after EVERY instruction",
      FACTORIAL_SRC, (int)(sizeof(FACTORIAL_SRC) / sizeof(FACTORIAL_SRC[0])), "trace" },
    { "power", "A1: 8^2 (C -> assembly), memory after EVERY instruction",
      POWER_SRC, (int)(sizeof(POWER_SRC) / sizeof(POWER_SRC[0])), "trace" },
#endif
#if ASSIGNMENT == 0 || ASSIGNMENT == 2
    { "recursive", "A2: load view, 4 segments after EVERY recursive call, final view",
      RECURSIVE_SRC, (int)(sizeof(RECURSIVE_SRC) / sizeof(RECURSIVE_SRC[0])), "calls" },
#endif
};
const int NUM_PROGRAMS = (int)(sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

/*
 * find_program: name -> program in this build, or 0 if not here.
 */
const ProgramSource *find_program(const char *name)
{
    int i;
    for (i = 0; i < NUM_PROGRAMS; i++) {
        if (text_equal(name, PROGRAMS[i].name)) {
            return &PROGRAMS[i];
        }
    }
    return 0;
}
