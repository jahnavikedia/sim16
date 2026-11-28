/*
 * programs.h -- the assembly programs built into the simulator
 *
 * Purpose : the simulator may not read files, so each program's assembly
 *           source is stored in programs.c as an array of lines.
 *           Only the programs of this build's assignment are included
 *           (see assignment.h).
 */
#ifndef PROGRAMS_H
#define PROGRAMS_H

/* One built-in program */
typedef struct {
    const char *name;               /* command-line name, e.g. "factorial" */
    const char *about;              /* one line for the usage table */
    const char *const *lines;       /* assembly source, one string per line */
    int n_lines;
    const char *default_mode;       /* "trace" or "calls" */
} ProgramSource;

extern const ProgramSource PROGRAMS[];  /* this build's programs */
extern const int NUM_PROGRAMS;

const ProgramSource *find_program(const char *name);

#endif
