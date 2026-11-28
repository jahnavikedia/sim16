/*
 * main.c -- SIM16 command line
 *
 * Purpose : the one command the grader runs:  ./sim16
 *           It runs this build's assignment (see assignment.h):
 *             Assignment 1: factorial, then power (memory after every step)
 *             Assignment 2: recursive (4 segments after every call)
 *
 *   ./sim16                       run this assignment's program(s)
 *   ./sim16 <program>             one program, in its default mode
 *   ./sim16 <program> <mode>      force a mode: trace | calls | final
 *   ./sim16 list [program]        assembly listing
 *   ./sim16 help                  usage table
 *
 * Errors are returned up to main, which prints "ERROR: ..." and returns 1.
 */
#include <stdio.h>
#include "assignment.h"
#include "cpu.h"
#include "assembler.h"
#include "loader.h"
#include "printer.h"
#include "programs.h"
#include "text.h"

#define TITLE_LEN 64

/*
 * print_assignment_line: which assignment this build is for.
 */
static void print_assignment_line(void)
{
#if ASSIGNMENT == 1
    printf("SIM16 -- Assignment 1: CPU Design (programs: factorial, power)\n");
#elif ASSIGNMENT == 2
    printf("SIM16 -- Assignment 2: Program Layout & Execution (program: recursive)\n");
#else
    printf("SIM16 -- development build (all programs)\n");
#endif
}

/*
 * print_usage: the command table, listing only this build's programs.
 */
static void print_usage(void)
{
    int i;
    print_assignment_line();
    printf("\n");
    printf("Usage:\n");
    printf("  ./sim16                      run this assignment's program(s)\n");
    printf("  ./sim16 <program>            run one program\n");
    printf("  ./sim16 <program> <mode>     force a mode: trace | calls | final\n");
    printf("  ./sim16 list [program]       assembly listing: address, machine code, source\n");
    printf("  ./sim16 help                 this table\n");
    printf("  ./test_cpu                   unit tests + golden runs\n");
    printf("\n");
    printf("Programs in this build:\n");
    for (i = 0; i < NUM_PROGRAMS; i++) {
        printf("  %-12s %s\n", PROGRAMS[i].name, PROGRAMS[i].about);
    }
    printf("\n");
    printf("Modes:\n");
    printf("  trace   memory after EVERY instruction (default: factorial, power)\n");
    printf("  calls   4 segments after EVERY CALL (default: recursive)\n");
    printf("  final   4 segments at load and at the end only\n");
}

/*
 * is_mode: is this one of the 3 mode names?
 */
static int is_mode(const char *s)
{
    return text_equal(s, "trace") || text_equal(s, "calls") || text_equal(s, "final");
}

/*
 * run_program: assemble, load and run one program, printing in the mode:
 *   trace : load view, a step block after EVERY instruction, final view
 *   calls : load view, 4-segment view after EVERY CALL, final view
 *   final : load view, final view
 * Returns 0 on success, 1 on error (message already printed).
 */
static int run_program(const ProgramSource *src, const char *mode)
{
    static Program prog;            /* static: large struct, not on the stack */
    CPU cpu, start, last, prev;     /* snapshots are plain struct copies */
    SimError err;
    char title[TITLE_LEN];
    const char *name;
    int calls = 0;

    if (assemble(src->lines, src->n_lines, &prog, &err) != 0
        || load_program(&prog, &cpu, &err) != 0) {
        print_error(&err);
        return 1;
    }

    printf("SIM16 CPU Simulator -- %s  (mode: %s)\n", src->name, mode);
    start = cpu;                                /* step 0: program loaded */
    print_segments(&start, 0, &prog, "PROGRAM LOADED INTO MEMORY");
    printf("\n");
    last = start;                               /* last snapshot printed */

    while (!cpu.halted) {
        if (cpu.steps >= DEFAULT_MAX_STEPS) {
            err.kind = ERR_MAX_STEPS;
            err.value = DEFAULT_MAX_STEPS;
            print_error(&err);
            return 1;
        }
        prev = cpu;                             /* state before this step */
        if (cpu_step(&cpu, &err) != 0) {
            print_error(&err);
            return 1;
        }

        if (text_equal(mode, "trace")) {
            print_step(&prev, &cpu, &prog, 0);  /* memory after EVERY step */
            printf("\n");
        } else if (text_equal(mode, "calls") && cpu.last_event.type == EV_CALL) {
            /* title: "AFTER CALL #k: name(arg)" */
            calls++;
            name = text_label_at(&prog, cpu.last_event.target);
            text_copy(title, "AFTER CALL #", TITLE_LEN);
            text_append_int(title, calls, TITLE_LEN);
            text_append(title, ": ", TITLE_LEN);
            text_append(title, name != 0 ? name : "?", TITLE_LEN);
            text_append(title, "(", TITLE_LEN);
            text_append_int(title, cpu.last_event.arg, TITLE_LEN);
            text_append(title, ")", TITLE_LEN);
            print_segments(&cpu, &last, &prog, title);
            printf("\n");
            last = cpu;
        } else if (text_equal(mode, "calls") && cpu.last_event.type == EV_OUT) {
            printf("OUTPUT> %d\n\n", cpu.last_event.value);
        }
    }

    /* compare the end with step 0 (trace, final) or the last view (calls) */
    print_segments(&cpu, text_equal(mode, "calls") ? &last : &start, &prog,
                   "FINAL STATE (after HALT)");
    printf("\n");
    print_summary(&cpu);
    return 0;
}

/*
 * list_program: print the assembly listing of one program.
 */
static int list_program(const ProgramSource *src)
{
    static Program prog;
    SimError err;
    if (assemble(src->lines, src->n_lines, &prog, &err) != 0) {
        print_error(&err);
        return 1;
    }
    print_listing(&prog, src->lines, src->n_lines, src->name);
    return 0;
}

/*
 * main: read the command line and run.
 * Returns 0 on success, 1 on any error or bad argument.
 */
int main(int argc, char *argv[])
{
    const ProgramSource *src;
    int i, status = 0;

    /* ./sim16  -> this assignment's programs, in their default modes */
    if (argc == 1) {
        print_assignment_line();
        for (i = 0; i < NUM_PROGRAMS && status == 0; i++) {
            printf("\n");
            status = run_program(&PROGRAMS[i], PROGRAMS[i].default_mode);
        }
        return status;
    }

    /* ./sim16 help */
    if (argc == 2 && text_equal(argv[1], "help")) {
        print_usage();
        return 0;
    }

    /* ./sim16 list [program] */
    if (text_equal(argv[1], "list") && argc <= 3) {
        if (argc == 3) {
            src = find_program(argv[2]);
            if (src == 0) {
                print_usage();
                return 1;
            }
            return list_program(src);
        }
        for (i = 0; i < NUM_PROGRAMS && status == 0; i++) {
            if (i > 0) {
                printf("\n");
            }
            status = list_program(&PROGRAMS[i]);
        }
        return status;
    }

    /* ./sim16 <program> [mode] */
    src = find_program(argv[1]);
    if (src == 0 || argc > 3 || (argc == 3 && !is_mode(argv[2]))) {
        print_usage();                          /* unknown program or argument */
        return 1;
    }
    return run_program(src, argc == 3 ? argv[2] : src->default_mode);
}
