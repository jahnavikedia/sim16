/*
 * printer.c -- every printed view of the simulator (SPEC section 7)
 *
 * Purpose : Assignment 1: step blocks (registers + memory grid after every
 *           instruction).  Assignment 2: four-segment views (TEXT decoded,
 *           DATA, HEAP, STACK with frames) at load, after each CALL, at end.
 * Rules   : print directly with printf; widths from SPEC 7.6; no trailing
 *           spaces (the last column is printed without padding).
 */
#include <stdio.h>
#include "printer.h"
#include "assembler.h"
#include "text.h"

#define RULE_WIDTH 78   /* separator lines are 78 characters */

/* Register display names (R6 = FP, R7 = SP) */
static const char *REG_NAMES[NUM_REGS] = { "R0", "R1", "R2", "R3", "R4", "R5", "FP", "SP" };

/* ---- Small helpers ----------------------------------------------------- */

/*
 * rule: print a separator line of 78 '=' or '-'.
 */
static void rule(char c)
{
    int i;
    for (i = 0; i < RULE_WIDTH; i++) {
        putchar(c);
    }
    putchar('\n');
}

/*
 * mem_mark / reg_mark: '*' if the value changed since prev, else ' '.
 */
static char mem_mark(const CPU *snap, const CPU *prev, int addr)
{
    return (prev != 0 && prev->mem[addr] != snap->mem[addr]) ? '*' : ' ';
}

static char reg_mark(const CPU *snap, const CPU *prev, int r)
{
    return (prev != 0 && prev->regs[r] != snap->regs[r]) ? '*' : ' ';
}

/*
 * segment_name: which of the 4 segments an address is in.
 */
static const char *segment_name(int addr)
{
    if (addr < TEXT_END) {
        return "TEXT";
    }
    if (addr < DATA_END) {
        return "DATA";
    }
    if (addr < HEAP_END) {
        return "HEAP";
    }
    return "STACK";
}

/*
 * text_label_at: first label in TEXT at this address, or 0.
 */
const char *text_label_at(const Program *prog, int addr)
{
    int i;
    for (i = 0; i < prog->n_symbols; i++) {
        if (prog->symbols[i].addr == addr && addr < TEXT_END) {
            return prog->symbols[i].name;
        }
    }
    return 0;
}

/*
 * data_label_at: first label in DATA/HEAP/STACK at this address, or "".
 */
static const char *data_label_at(const Program *prog, int addr)
{
    int i;
    for (i = 0; i < prog->n_symbols; i++) {
        if (prog->symbols[i].addr == addr && addr >= DATA_BASE) {
            return prog->symbols[i].name;
        }
    }
    return "";
}

/* ---- Errors (SPEC 3.8) -------------------------------------------------- */

/*
 * print_hex_addr: "0x1F"; a negative address prints as "-0x10".
 */
static void print_hex_addr(int addr)
{
    if (addr < 0) {
        printf("-0x%02X", -addr);
    } else {
        printf("0x%02X", addr);
    }
}

/*
 * print_error: "ERROR: <message>" with the exact SPEC 3.8 wording.
 */
void print_error(const SimError *e)
{
    printf("ERROR: ");
    if (e->kind >= ERR_UNKNOWN_MNEMONIC) {
        printf("line %d: ", e->line);       /* every assembler error */
    }
    switch (e->kind) {
    /* cpu */
    case ERR_HALTED:          printf("CPU is halted"); break;
    case ERR_PC_RANGE:        printf("PC out of range: "); print_hex_addr(e->addr); break;
    case ERR_INVALID_OPCODE:  printf("invalid opcode 0x%02X at 0x%02X", e->value, e->addr); break;
    case ERR_STACK_OVERFLOW:  printf("stack overflow"); break;
    case ERR_STACK_UNDERFLOW: printf("stack underflow"); break;
    case ERR_ADDR_RANGE:
        printf("memory address out of range: ");
        print_hex_addr(e->addr);
        break;
    case ERR_MAX_STEPS:
        printf("exceeded max steps (%d), infinite loop?", e->value);
        break;
    /* loader */
    case ERR_BAD_OBJECT:      printf("bad object format"); break;
    case ERR_TEXT_TOO_LARGE:  printf("text segment too large: %d words (max 96)", e->value); break;
    case ERR_DATA_TOO_LARGE:  printf("data segment too large: %d words (max 32)", e->value); break;
    case ERR_ENTRY_RANGE:     printf("entry point 0x%02X outside text", e->addr); break;
    /* assembler */
    case ERR_UNKNOWN_MNEMONIC: printf("unknown mnemonic '%s'", e->name); break;
    case ERR_UNDEFINED_LABEL:  printf("undefined label '%s'", e->name); break;
    case ERR_IMM_RANGE:        printf("immediate out of range 0..255: %d", e->value); break;
    case ERR_OFFSET_RANGE:     printf("offset out of range -16..15: %d", e->value); break;
    case ERR_DUPLICATE_LABEL:  printf("duplicate label '%s'", e->name); break;
    case ERR_OPERAND_COUNT:    printf("expected %d operands, got %d", e->value, e->addr); break;
    case ERR_BAD_REGISTER:     printf("bad register '%s'", e->name); break;
    case ERR_WORD_IN_TEXT:     printf(".word not allowed in .text"); break;
    case ERR_INSTR_IN_DATA:    printf("instruction not allowed in .data"); break;
    case ERR_ASM_TEXT_TOO_LARGE:
        printf("text segment too large: %d words (max 96)", e->value);
        break;
    case ERR_ASM_DATA_TOO_LARGE:
        printf("data segment too large: %d words (max 32)", e->value);
        break;
    case ERR_RESERVED_LABEL:   printf("label '%s' is a reserved word", e->name); break;
    case ERR_BAD_NUMBER:       printf("bad number '%s'", e->name); break;
    case ERR_BAD_OPERAND:      printf("bad operand '%s'", e->name); break;
    case ERR_BAD_DIRECTIVE:    printf("unknown directive '%s'", e->name); break;
    case ERR_WORD_RANGE:       printf(".word value out of range: %d", e->value); break;
    case ERR_TOO_MANY_LABELS:  printf("too many labels (max 64)"); break;
    default:                   printf("unknown error"); break;
    }
    printf("\n");
}

/* ---- Registers (SPEC 7.1) ----------------------------------------------- */

/*
 * print_registers:
 *   PC=03  IR=5020  Z=0 N=0
 *   R0=0008* R1=0008  R2=0000  R3=0000  R4=0000  R5=0000  FP=0100  SP=0100
 */
void print_registers(const CPU *snap, const CPU *prev)
{
    int r;
    printf("PC=%02X  IR=%04X  Z=%d N=%d\n", snap->pc, snap->ir, snap->flag_z, snap->flag_n);
    for (r = 0; r < NUM_REGS; r++) {
        char mark = reg_mark(snap, prev, r);
        printf("%s=%04X", REG_NAMES[r], snap->regs[r]);
        if (r < NUM_REGS - 1) {
            printf("%c ", mark);                /* mark + column gap */
        } else if (mark == '*') {
            putchar('*');                       /* last one: no trailing space */
        }
    }
    putchar('\n');
}

/* ---- Memory grid (SPEC 7.2) --------------------------------------------- */

/*
 * print_grid_row: one row of 8 words, e.g.
 *   DATA   60   0008  9D80* 0000  0000  0000  0000  0000  0000
 */
static void print_grid_row(const CPU *snap, const CPU *prev, int row)
{
    int a;
    printf("%-6s %02X   ", segment_name(row), row);
    for (a = row; a < row + 8; a++) {
        char mark = mem_mark(snap, prev, a);
        printf("%04X", snap->mem[a]);
        if (a < row + 7) {
            printf("%c ", mark);
        } else if (mark == '*') {
            putchar('*');
        }
    }
    putchar('\n');
}

/*
 * print_memory_grid: the small block of memory shown after every step.
 *   compact (full = 0): TEXT rows in use, DATA rows in use, HEAP row 0x80,
 *                       STACK rows F0 and F8 (+ rows down to SP if deeper)
 *   full = 1          : all 32 rows
 */
void print_memory_grid(const CPU *snap, const CPU *prev, const Program *prog, int full)
{
    int row, sp_row;

    printf("MEMORY  (* = changed by this instruction)\n");
    printf("            +0    +1    +2    +3    +4    +5    +6    +7\n");
    if (full) {
        for (row = 0; row < MEM_SIZE; row += 8) {
            print_grid_row(snap, prev, row);
        }
        return;
    }
    row = TEXT_BASE;
    do {                                        /* TEXT rows in use (at least 1) */
        print_grid_row(snap, prev, row);
        row += 8;
    } while (row < TEXT_BASE + prog->text_len);
    row = DATA_BASE;
    do {                                        /* DATA rows in use (at least 1) */
        print_grid_row(snap, prev, row);
        row += 8;
    } while (row < DATA_BASE + prog->data_len);
    print_grid_row(snap, prev, HEAP_BASE);      /* HEAP: one row (empty) */

    sp_row = snap->regs[REG_SP] - (snap->regs[REG_SP] % 8);   /* row holding SP */
    if (sp_row < STACK_BASE) {
        sp_row = STACK_BASE;
    }
    for (row = sp_row; row < 0xF0; row += 8) {  /* deeper stack rows, if any */
        print_grid_row(snap, prev, row);
    }
    print_grid_row(snap, prev, 0xF0);           /* top 16 stack words */
    print_grid_row(snap, prev, 0xF8);
}

/* ---- Step block (SPEC 7.3) ---------------------------------------------- */

/*
 * print_step: one instruction of an Assignment 1 trace:
 *   header (step, address, word, instruction), registers, memory grid.
 */
void print_step(const CPU *prev, const CPU *snap, const Program *prog, int full)
{
    char text[DISASM_LEN];
    disassemble(snap->ir, prog, text);
    rule('=');
    printf("STEP %-3d @%02X  %04X  %s\n", snap->steps, snap->exec_addr, snap->ir, text);
    rule('-');
    if (snap->last_event.type == EV_OUT) {
        printf("OUTPUT> %d\n", snap->last_event.value);
    }
    print_registers(snap, prev);
    print_memory_grid(snap, prev, prog, full);
}

/* ---- Four-segment view (SPEC 7.4) --------------------------------------- */

/* Role of a stack word in the frame chain */
#define ROLE_LOCAL  0       /* not inside a frame */
#define ROLE_SAVED  1       /* FP + 0: saved FP (first line of a frame) */
#define ROLE_RETURN 2       /* FP + 1: return address */
#define ROLE_ARG    3       /* FP + 2: argument */
#define MAX_FRAMES  32      /* safety guard for a corrupted stack */

/*
 * walk_frames: follow the saved-FP chain from FP up to 0x100 (SPEC 3.7)
 * and mark each word's role. Stops if the chain does not go up.
 */
static void walk_frames(const CPU *snap, int role[MEM_SIZE])
{
    int a, fp, last = -1, frames = 0;

    for (a = 0; a < MEM_SIZE; a++) {
        role[a] = ROLE_LOCAL;
    }
    fp = snap->regs[REG_FP];
    while (fp != STACK_TOP && frames < MAX_FRAMES) {
        if (fp < STACK_BASE || fp + 2 > 0xFF || fp <= last) {
            break;                              /* not a sensible frame */
        }
        role[fp] = ROLE_SAVED;
        role[fp + 1] = ROLE_RETURN;
        role[fp + 2] = ROLE_ARG;
        last = fp;
        fp = snap->mem[fp];                     /* caller's frame (higher up) */
        frames++;
    }
}

/*
 * frame_column: text of the FRAME column for one stack word, e.g.
 *   "+-- fact(4)   <- SP, FP",  "|",  "+--",  ""
 */
static void frame_column(const CPU *snap, const Program *prog, const int role[MEM_SIZE],
                         int addr, char *out, int size)
{
    char frame[NAME_LEN + 16];
    int sp = snap->regs[REG_SP];
    int fp = snap->regs[REG_FP];
    int i, len;

    frame[0] = '\0';
    if (role[addr] == ROLE_SAVED) {
        /* name = label at the CALL target; the CALL is the word before the return address */
        int ret = snap->mem[addr + 1];
        int target = (ret >= 1 && ret <= 0xFF) ? (snap->mem[ret - 1] & IMM8_MASK) : -1;
        const char *name = (target >= 0) ? text_label_at(prog, target) : 0;
        text_copy(frame, "+-- ", (int)sizeof(frame));
        text_append(frame, name != 0 ? name : "?", (int)sizeof(frame));
        text_append(frame, "(", (int)sizeof(frame));
        text_append_int(frame, snap->mem[addr + 2], (int)sizeof(frame));
        text_append(frame, ")", (int)sizeof(frame));
    } else if (role[addr] == ROLE_RETURN) {
        text_copy(frame, "|", (int)sizeof(frame));
    } else if (role[addr] == ROLE_ARG) {
        text_copy(frame, "+--", (int)sizeof(frame));
    }

    text_copy(out, frame, size);
    if (addr == sp || addr == fp) {
        len = text_length(out);
        for (i = len; i < 13; i++) {            /* pad to 13: "%-13s <- SP" */
            out[i] = ' ';
        }
        out[i] = '\0';
        text_append(out, " <- ", size);
        text_append(out, (addr == sp && addr == fp) ? "SP, FP" : (addr == sp ? "SP" : "FP"),
                    size);
    }
}

/*
 * print_stack: the STACK part of the four-segment view: SP .. 0xFF,
 * each word labelled saved FP / return addr / arg / local, frames drawn.
 */
void print_stack(const CPU *snap, const CPU *prev, const Program *prog)
{
    static const char *CONTENTS[4] = { "local", "saved FP", "return addr", "arg" };
    int role[MEM_SIZE];
    char frame[64];
    int sp = snap->regs[REG_SP];
    int fp = snap->regs[REG_FP];
    int a;

    printf("[STACK] 0xA0-0xFF   %d of 96 words used   SP=%02X  FP=%02X   (grows down)\n",
           STACK_TOP - sp, sp, fp);
    if (sp >= STACK_TOP) {
        printf("  (empty)\n");
    } else {
        walk_frames(snap, role);
        printf("  ADDR  HEX    DEC   CONTENTS              FRAME\n");
        for (a = (sp < STACK_BASE ? STACK_BASE : sp); a < STACK_TOP; a++) {
            frame_column(snap, prog, role, a, frame, (int)sizeof(frame));
            printf("  %02X    %04X%c  %3d   ", a, snap->mem[a], mem_mark(snap, prev, a),
                   snap->mem[a]);
            if (frame[0] == '\0') {
                printf("%s\n", CONTENTS[role[a]]);          /* no trailing spaces */
            } else {
                printf("%-22s%s\n", CONTENTS[role[a]], frame);
            }
        }
    }
    if (sp > STACK_BASE) {
        printf("  A0-%02X (free)\n", (sp > STACK_TOP ? STACK_TOP : sp) - 1);
    }
}

/*
 * print_segments: Assignment 2 view of all 4 segments (SPEC 7.4):
 *   title + step, registers, TEXT (decoded), DATA, HEAP, STACK.
 */
void print_segments(const CPU *snap, const CPU *prev, const Program *prog, const char *title)
{
    char text[DISASM_LEN];
    char label[NAME_LEN + 1];
    int a, i, heap_used = 0;
    int width = text_length(title);
    int step = snap->steps;

    /* Title line: " AFTER CALL #2: fact(4)          ...          step 9" */
    width += 5;                                 /* "step " */
    do {                                        /* + digits of the step number */
        width++;
        step /= 10;
    } while (step > 0);
    rule('=');
    printf(" %s", title);
    for (i = 0; i < RULE_WIDTH - 1 - width; i++) {
        putchar(' ');
    }
    printf("step %d\n", snap->steps);
    rule('=');
    print_registers(snap, prev);

    /* TEXT: one line per instruction, decoded */
    printf("\n[TEXT]  0x00-0x5F   %d of 96 words used\n", prog->text_len);
    printf("  ADDR  HEX    LABEL    INSTRUCTION\n");
    for (a = TEXT_BASE; a < TEXT_BASE + prog->text_len; a++) {
        const char *name = text_label_at(prog, a);
        label[0] = '\0';
        if (name != 0) {
            text_copy(label, name, NAME_LEN);
            text_append(label, ":", (int)sizeof(label));
        }
        disassemble(snap->mem[a], prog, text);
        printf("  %02X    %04X%c  %-9s", a, snap->mem[a], mem_mark(snap, prev, a), label);
        if (a == snap->pc) {
            printf("%-20s<- PC\n", text);
        } else {
            printf("%s\n", text);
        }
    }
    if (prog->text_len < TEXT_SIZE) {
        printf("  %02X-5F (unused)\n", TEXT_BASE + prog->text_len);
    }

    /* DATA: one line per global, with its name and decimal value */
    printf("\n[DATA]  0x60-0x7F   %d of 32 words used\n", prog->data_len);
    printf("  ADDR  HEX    NAME     VALUE\n");
    for (a = DATA_BASE; a < DATA_BASE + prog->data_len; a++) {
        printf("  %02X    %04X%c  %-9s%d\n", a, snap->mem[a], mem_mark(snap, prev, a),
               data_label_at(prog, a), snap->mem[a]);
    }
    if (prog->data_len < DATA_SIZE) {
        printf("  %02X-7F (unused)\n", DATA_BASE + prog->data_len);
    }

    /* HEAP: empty in our programs; non-zero words are listed if any */
    printf("\n[HEAP]  0x80-0x9F\n");
    for (a = HEAP_BASE; a < HEAP_END; a++) {
        if (snap->mem[a] != 0) {
            heap_used = 1;
        }
    }
    if (!heap_used) {
        printf("  (empty -- program uses no dynamic memory)\n");
    } else {
        printf("  ADDR  HEX    NAME     VALUE\n");
        for (a = HEAP_BASE; a < HEAP_END; a++) {
            if (snap->mem[a] != 0) {
                printf("  %02X    %04X%c  %-9s%d\n", a, snap->mem[a],
                       mem_mark(snap, prev, a), data_label_at(prog, a), snap->mem[a]);
            }
        }
    }

    /* STACK */
    printf("\n");
    print_stack(snap, prev, prog);
}

/* ---- Summary (SPEC 7.5) ------------------------------------------------- */

/*
 * print_summary: "Program halted after 52 steps.  Output: 120"
 */
void print_summary(const CPU *snap)
{
    int i;
    printf("Program halted after %d steps.  Output:", snap->steps);
    for (i = 0; i < snap->n_output; i++) {
        if (i > 0) {
            printf(",");                        /* "120" or "1, 2, 3" */
        }
        printf(" %d", snap->output[i]);
    }
    printf("\n");
}

/* ---- Assembly listing (./sim16 list) ------------------------------------ */

/*
 * print_listing: every source line; instruction and .word lines get their
 * address and machine code in front.
 *   ADR HEX   SOURCE
 *   00  1960  LD    R1, n          ; R1 = counter = n (8)
 */
void print_listing(const Program *prog, const char *const *lines, int n_lines,
                   const char *name)
{
    int i, k, j, addr, word;

    printf("SIM16 assembly listing -- %s\n", name);
    printf("ADR HEX   SOURCE\n");
    for (i = 1; i <= n_lines; i++) {
        const char *s = lines[i - 1];
        j = 0;
        while (is_space(s[j])) {
            j++;                                /* drop the indent */
        }
        addr = -1;
        word = 0;
        for (k = 0; k < prog->text_len; k++) {  /* an instruction on this line? */
            if (prog->source_line[k] == i) {
                addr = TEXT_BASE + k;
                word = prog->text[k];
            }
        }
        for (k = 0; k < prog->data_len; k++) {  /* a .word on this line? */
            if (prog->data_line[k] == i) {
                addr = DATA_BASE + k;
                word = prog->data[k];
            }
        }
        if (addr >= 0) {
            printf("%02X  %04X  %s\n", addr, word, s + j);
        } else if (s[j] == ';' || s[j] == '\0') {
            printf("%s\n", s + j);              /* comment line */
        } else {
            printf("          %s\n", s + j);    /* label or directive */
        }
    }
}
