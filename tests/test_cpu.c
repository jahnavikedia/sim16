/*
 * test_cpu.c -- unit tests and golden runs for SIM16
 *
 * Purpose : proves the CPU, loader and assembler follow the spec:
 *           one test per instruction (with flags), every error, and the
 *           three reference programs (SPEC 9) with their exact results.
 * Run     : make test      (or ./test_cpu)
 *
 * CPU tests use raw hex words, so they run in every build.
 */
#include <stdio.h>
#include "../src/assignment.h"
#include "../src/cpu.h"
#include "../src/loader.h"
#include "../src/text.h"
#include "../src/assembler.h"
#include "../src/programs.h"

/* ---- Tiny test framework ---------------------------------------------- */

static int passed = 0;
static int failed = 0;

/*
 * check: record one test result; print the message if it failed.
 */
static void check(int cond, const char *message)
{
    if (cond) {
        passed++;
    } else {
        failed++;
        printf("  FAIL: %s\n", message);
    }
}

/* ---- Helpers ----------------------------------------------------------- */

/*
 * make_cpu: reset cpu, put words at 0x00.. (text) and 0x60.. (data).
 * SP = FP = 0x100, PC = 0.
 */
static void make_cpu(CPU *cpu, const unsigned short *text, int n_text,
                     const unsigned short *data, int n_data)
{
    int i;
    cpu_reset(cpu);
    for (i = 0; i < n_text; i++) {
        cpu->mem[TEXT_BASE + i] = text[i];
    }
    for (i = 0; i < n_data; i++) {
        cpu->mem[DATA_BASE + i] = data[i];
    }
}

/*
 * step_n: run n steps; returns 1 if all succeeded.
 */
static int step_n(CPU *cpu, int n)
{
    SimError err;
    int i;
    for (i = 0; i < n; i++) {
        if (cpu_step(cpu, &err) != 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * same_cpu: are two CPU states identical? (used to prove errors change nothing)
 */
static int same_cpu(const CPU *a, const CPU *b)
{
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (a->mem[i] != b->mem[i]) {
            return 0;
        }
    }
    for (i = 0; i < NUM_REGS; i++) {
        if (a->regs[i] != b->regs[i]) {
            return 0;
        }
    }
    for (i = 0; i < MAX_OUTPUT; i++) {
        if (a->output[i] != b->output[i]) {
            return 0;
        }
    }
    return a->pc == b->pc && a->ir == b->ir && a->exec_addr == b->exec_addr
        && a->flag_z == b->flag_z && a->flag_n == b->flag_n
        && a->halted == b->halted && a->steps == b->steps
        && a->n_output == b->n_output && a->last_event.type == b->last_event.type;
}

/*
 * expect_error: step once, expect error `kind`, and expect no change.
 */
static void expect_error(CPU *cpu, int kind, const char *message)
{
    SimError err;
    CPU before = *cpu;                  /* snapshot = struct copy */
    int status = cpu_step(cpu, &err);
    check(status != 0 && err.kind == kind, message);
    check(same_cpu(&before, cpu), "failed instruction changed nothing");
}

/* ---- Reset and snapshot ------------------------------------------------ */

static void test_reset(void)
{
    CPU cpu;
    int i, zero = 1;
    cpu_reset(&cpu);
    for (i = 0; i < MEM_SIZE; i++) {
        if (cpu.mem[i] != 0) {
            zero = 0;
        }
    }
    check(zero, "reset: memory all zero");
    check(cpu.regs[0] == 0 && cpu.regs[5] == 0, "reset: R0-R5 = 0");
    check(cpu.regs[REG_FP] == 0x100 && cpu.regs[REG_SP] == 0x100, "reset: FP = SP = 0x100");
    check(cpu.pc == 0 && cpu.ir == 0 && cpu.exec_addr == -1, "reset: PC 0, IR 0, no exec_addr");
    check(cpu.flag_z == 0 && cpu.flag_n == 0, "reset: flags 0");
    check(!cpu.halted && cpu.steps == 0 && cpu.n_output == 0, "reset: not halted, 0 steps");
    check(cpu.last_event.type == EV_NONE, "reset: no event");
}

static void test_snapshot_is_copy(void)
{
    static const unsigned short prog[] = { 0x1005, 0xB800, 0x0000 };  /* LDI R0,5; OUT; HALT */
    CPU cpu, snap;
    SimError err;
    make_cpu(&cpu, prog, 3, 0, 0);
    step_n(&cpu, 1);
    snap = cpu;                         /* snapshot */
    cpu_run(&cpu, DEFAULT_MAX_STEPS, &err);
    check(snap.steps == 1 && snap.exec_addr == 0 && snap.ir == 0x1005, "snapshot: step 1 state");
    check(snap.n_output == 0 && !snap.halted, "snapshot: later steps do not change it");
    snap.mem[0] = 0xFFFF;
    check(cpu.mem[0] == 0x1005, "snapshot: has its own memory");
}

/* ---- One test per instruction ------------------------------------------ */

static void test_halt(void)
{
    static const unsigned short prog[] = { 0x0000 };
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.halted && cpu.pc == 1 && cpu.steps == 1, "HALT stops the CPU");
    check(cpu.last_event.type == EV_HALT, "HALT event");
}

static void test_nop(void)
{
    static const unsigned short prog[] = { 0x0800 };
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.pc == 1 && cpu.regs[0] == 0 && cpu.flag_z == 0, "NOP does nothing");
    check(cpu.last_event.type == EV_NONE, "NOP: no event");
}

static void test_ldi(void)
{
    static const unsigned short prog[] = { 0x1001, 0x13FF };   /* LDI R0,1; LDI R3,255 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    step_n(&cpu, 2);
    check(cpu.regs[0] == 1 && cpu.regs[3] == 255, "LDI loads 0..255");
}

static void test_ld(void)
{
    static const unsigned short prog[] = { 0x1960 };           /* LD R1, 0x60 */
    static const unsigned short data[] = { 0x1234 };
    CPU cpu;
    make_cpu(&cpu, prog, 1, data, 1);
    step_n(&cpu, 1);
    check(cpu.regs[1] == 0x1234, "LD reads memory");
}

static void test_st(void)
{
    static const unsigned short prog[] = { 0x1007, 0x2061 };   /* LDI R0,7; ST R0, 0x61 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    step_n(&cpu, 2);
    check(cpu.mem[0x61] == 7, "ST writes memory");
}

static void test_ldr(void)
{
    static const unsigned short prog[] = { 0x29C2, 0x2ADF };   /* LDR R1,[FP+2]; LDR R2,[FP-1] */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[REG_FP] = 0xFA;
    cpu.mem[0xFC] = 4;
    cpu.mem[0xF9] = 0x42;
    step_n(&cpu, 2);
    check(cpu.regs[1] == 4, "LDR [FP+2]");
    check(cpu.regs[2] == 0x42, "LDR [FP-1] (negative offset)");
}

static void test_str(void)
{
    static const unsigned short prog[] = { 0x1209, 0x32FF };   /* LDI R2,9; STR R2,[SP-1] */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    step_n(&cpu, 2);
    check(cpu.mem[0xFF] == 9 && cpu.regs[REG_SP] == 0x100, "STR writes, SP unchanged");
}

static void test_mov(void)
{
    static const unsigned short prog[] = { 0x1207, 0x3940 };   /* LDI R2,7; MOV R1,R2 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    step_n(&cpu, 2);
    check(cpu.regs[1] == 7, "MOV copies a register");
}

static void test_add(void)
{
    static const unsigned short prog[] = { 0x4140, 0x4140 };   /* ADD R1, R2 (twice) */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[1] = 3;
    cpu.regs[2] = 4;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 7 && !cpu.flag_z && !cpu.flag_n, "ADD 3+4 = 7, Z=0 N=0");
    cpu.regs[1] = 0xFFFF;
    cpu.regs[2] = 1;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 0 && cpu.flag_z && !cpu.flag_n, "ADD wraps to 0, Z=1");
}

static void test_sub(void)
{
    static const unsigned short prog[] = { 0x4940 };           /* SUB R1, R2 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.regs[1] = 3;
    cpu.regs[2] = 5;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 0xFFFE && !cpu.flag_z && cpu.flag_n, "SUB 3-5 = 0xFFFE, N=1");
}

static void test_mul(void)
{
    static const unsigned short prog[] = { 0x5020, 0x5020 };   /* MUL R0, R1 (twice) */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[0] = 5040;
    cpu.regs[1] = 8;
    step_n(&cpu, 1);
    check(cpu.regs[0] == 0x9D80 && !cpu.flag_z && cpu.flag_n, "MUL 5040*8 = 0x9D80, N=1");
    cpu.regs[0] = 0x100;
    cpu.regs[1] = 0x100;
    step_n(&cpu, 1);
    check(cpu.regs[0] == 0 && cpu.flag_z, "MUL 0x100*0x100 wraps to 0, Z=1");
}

static void test_addi(void)
{
    static const unsigned short prog[] = { 0x5F01 };           /* ADDI SP, 1 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.regs[REG_SP] = 0xFE;
    step_n(&cpu, 1);
    check(cpu.regs[REG_SP] == 0xFF && !cpu.flag_z && !cpu.flag_n, "ADDI SP, 1");
}

static void test_subi(void)
{
    static const unsigned short prog[] = { 0x6101, 0x6101 };   /* SUBI R1, 1 (twice) */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[1] = 1;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 0 && cpu.flag_z && !cpu.flag_n, "SUBI 1-1 = 0, Z=1");
    step_n(&cpu, 1);
    check(cpu.regs[1] == 0xFFFF && !cpu.flag_z && cpu.flag_n, "SUBI 0-1 = 0xFFFF, N=1");
}

static void test_cmp(void)
{
    static const unsigned short prog[] = { 0x6940, 0x6940 };   /* CMP R1, R2 (twice) */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[1] = 1;
    cpu.regs[2] = 2;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 1 && cpu.regs[2] == 2, "CMP discards the result");
    check(!cpu.flag_z && cpu.flag_n, "CMP 1,2: N=1 (less than)");
    cpu.regs[1] = 2;
    step_n(&cpu, 1);
    check(cpu.flag_z && !cpu.flag_n, "CMP 2,2: Z=1 (equal)");
}

static void test_cmpi(void)
{
    static const unsigned short prog[] = { 0x7102 };           /* CMPI R1, 2 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.regs[1] = 5;
    step_n(&cpu, 1);
    check(cpu.regs[1] == 5 && !cpu.flag_z && !cpu.flag_n, "CMPI 5,2: Z=0 N=0");
}

static void test_flags_unchanged(void)
{
    /* LDI R0,0; MOV R1,R2; PUSH R1; POP R2 : none of these touch flags */
    static const unsigned short prog[] = { 0x1000, 0x3940, 0x9900, 0xA200 };
    CPU cpu;
    make_cpu(&cpu, prog, 4, 0, 0);
    cpu.flag_z = 1;
    cpu.flag_n = 1;
    step_n(&cpu, 4);
    check(cpu.flag_z && cpu.flag_n, "non-ALU instructions keep the flags");
}

static void test_jmp(void)
{
    static const unsigned short prog[] = { 0x7804 };           /* JMP 0x04 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.pc == 0x04, "JMP jumps");
}

static void test_jz(void)
{
    static const unsigned short prog[] = { 0x8010 };           /* JZ 0x10 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.pc == 1, "JZ not taken when Z=0");
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.flag_z = 1;
    step_n(&cpu, 1);
    check(cpu.pc == 0x10, "JZ taken when Z=1");
}

static void test_jnz(void)
{
    static const unsigned short prog[] = { 0x8802 };           /* JNZ 0x02 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.pc == 0x02, "JNZ taken when Z=0");
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.flag_z = 1;
    step_n(&cpu, 1);
    check(cpu.pc == 1, "JNZ not taken when Z=1");
}

static void test_jn(void)
{
    static const unsigned short prog[] = { 0x9011 };           /* JN 0x11 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    step_n(&cpu, 1);
    check(cpu.pc == 1, "JN not taken when N=0");
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.flag_n = 1;
    step_n(&cpu, 1);
    check(cpu.pc == 0x11, "JN taken when N=1");
}

static void test_push(void)
{
    static const unsigned short prog[] = { 0x9900 };           /* PUSH R1 */
    CPU cpu;
    make_cpu(&cpu, prog, 1, 0, 0);
    cpu.regs[1] = 0xBEEF;
    step_n(&cpu, 1);
    check(cpu.regs[REG_SP] == 0xFF && cpu.mem[0xFF] == 0xBEEF, "PUSH: SP-1, then store");
}

static void test_pop(void)
{
    static const unsigned short prog[] = { 0x9900, 0xA200 };   /* PUSH R1; POP R2 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[1] = 42;
    step_n(&cpu, 2);
    check(cpu.regs[2] == 42 && cpu.regs[REG_SP] == 0x100, "POP: load, then SP+1");
    check(cpu.mem[0xFF] == 42, "POP leaves the old value in memory");
}

static void test_call(void)
{
    static const unsigned short prog[] = { 0x9900, 0xA805 };   /* PUSH R1; CALL 0x05 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    cpu.regs[1] = 3;
    step_n(&cpu, 2);
    check(cpu.pc == 0x05, "CALL jumps to the target");
    check(cpu.regs[REG_SP] == 0xFD && cpu.regs[REG_FP] == 0xFD, "CALL: FP = SP = 0xFD");
    check(cpu.mem[0xFD] == 0x100 && cpu.mem[0xFE] == 0x02 && cpu.mem[0xFF] == 3,
          "CALL frame: saved FP, return address, argument");
    check(cpu.last_event.type == EV_CALL && cpu.last_event.target == 5
          && cpu.last_event.ret_addr == 2 && cpu.last_event.arg == 3, "CALL event");
}

static void test_ret(void)
{
    /* 00 PUSH R1; 01 CALL 0x04; 02 HALT; 03 HALT; 04 LDI R0,9; 05 SUBI SP,2; 06 RET */
    static const unsigned short prog[] = { 0x9900, 0xA804, 0x0000, 0x0000,
                                           0x1009, 0x6702, 0xB000 };
    CPU cpu;
    make_cpu(&cpu, prog, 7, 0, 0);
    step_n(&cpu, 4);
    check(cpu.regs[REG_SP] == 0xFB, "two locals below the frame");
    step_n(&cpu, 1);
    check(cpu.pc == 0x02, "RET returns to the caller");
    check(cpu.regs[REG_FP] == 0x100 && cpu.regs[REG_SP] == 0xFF, "RET: locals + frame removed");
    check(cpu.last_event.type == EV_RET && cpu.last_event.ret_addr == 2
          && cpu.last_event.value == 9, "RET event");
}

static void test_out(void)
{
    static const unsigned short prog[] = { 0x1005, 0xB800 };   /* LDI R0,5; OUT R0 */
    CPU cpu;
    make_cpu(&cpu, prog, 2, 0, 0);
    step_n(&cpu, 2);
    check(cpu.n_output == 1 && cpu.output[0] == 5, "OUT appends to output");
    check(cpu.last_event.type == EV_OUT && cpu.last_event.value == 5, "OUT event");
}

/* ---- Errors (SPEC 3.8): each must change nothing ------------------------ */

static void test_errors(void)
{
    static const unsigned short push[] = { 0x9900 };           /* PUSH R1 */
    static const unsigned short call[] = { 0xA805 };           /* CALL 0x05 */
    static const unsigned short pop[] = { 0xA200 };            /* POP R2 */
    static const unsigned short ret[] = { 0xB000 };            /* RET */
    static const unsigned short bad[] = { 0x0800, 0x0800, 0xD000 };  /* opcode 0x1A at 0x02 */
    static const unsigned short ldr[] = { 0x29C5 };            /* LDR R1, [FP+5] */
    static const unsigned short str[] = { 0x31A1 };            /* STR R1, [R5+1] */
    static const unsigned short halt[] = { 0x0000 };
    static const unsigned short nop[] = { 0x0800 };
    CPU cpu;
    SimError err;
    int op;

    make_cpu(&cpu, push, 1, 0, 0);
    cpu.regs[REG_SP] = 0xA0;
    expect_error(&cpu, ERR_STACK_OVERFLOW, "PUSH on a full stack: stack overflow");
    check(cpu.mem[0x9F] == 0, "stack overflow: heap untouched");

    make_cpu(&cpu, call, 1, 0, 0);
    cpu.regs[REG_SP] = 0xA1;
    expect_error(&cpu, ERR_STACK_OVERFLOW, "CALL with 1 free word: stack overflow");

    make_cpu(&cpu, pop, 1, 0, 0);
    expect_error(&cpu, ERR_STACK_UNDERFLOW, "POP on an empty stack: stack underflow");

    make_cpu(&cpu, ret, 1, 0, 0);
    expect_error(&cpu, ERR_STACK_UNDERFLOW, "RET outside a function: stack underflow");

    make_cpu(&cpu, bad, 3, 0, 0);
    step_n(&cpu, 2);
    expect_error(&cpu, ERR_INVALID_OPCODE, "reserved opcode: invalid opcode");
    cpu_step(&cpu, &err);
    check(err.value == 0x1A && err.addr == 0x02, "invalid opcode 0x1A at 0x02");
    for (op = 0x18; op <= 0x1F; op++) {
        unsigned short word[1];
        word[0] = (unsigned short)(op << OP_SHIFT);
        make_cpu(&cpu, word, 1, 0, 0);
        expect_error(&cpu, ERR_INVALID_OPCODE, "every reserved opcode 0x18-0x1F");
    }

    make_cpu(&cpu, ldr, 1, 0, 0);
    expect_error(&cpu, ERR_ADDR_RANGE, "LDR outside memory: address out of range");
    cpu_step(&cpu, &err);
    check(err.addr == 0x105, "memory address out of range: 0x105");

    make_cpu(&cpu, str, 1, 0, 0);
    cpu.regs[5] = 0xFF;
    expect_error(&cpu, ERR_ADDR_RANGE, "STR outside memory: address out of range");

    cpu_reset(&cpu);
    cpu.mem[0xFF] = nop[0];
    cpu.pc = 0xFF;
    step_n(&cpu, 1);
    expect_error(&cpu, ERR_PC_RANGE, "PC past 0xFF: PC out of range");

    make_cpu(&cpu, halt, 1, 0, 0);
    step_n(&cpu, 1);
    expect_error(&cpu, ERR_HALTED, "stepping a halted CPU");
}

static void test_max_steps(void)
{
    static const unsigned short loop[] = { 0x7800 };           /* 00: JMP 0x00 forever */
    static const unsigned short recurse[] = { 0xA800 };        /* 00: CALL 0x00 forever */
    CPU cpu;
    SimError err;

    make_cpu(&cpu, loop, 1, 0, 0);
    check(cpu_run(&cpu, 100, &err) != 0 && err.kind == ERR_MAX_STEPS && err.value == 100,
          "exceeded max steps (100)");
    check(cpu.steps == 100, "stopped after exactly 100 steps");

    make_cpu(&cpu, recurse, 1, 0, 0);
    check(cpu_run(&cpu, DEFAULT_MAX_STEPS, &err) != 0 && err.kind == ERR_STACK_OVERFLOW,
          "endless recursion ends in stack overflow");
    check(cpu.steps == 48, "48 CALLs fill the 96-word stack");
}

/* ---- Loader (SPEC 4.3) -------------------------------------------------- */

static void test_loader(void)
{
    Program prog;
    CPU cpu;
    SimError err;
    int i;

    for (i = 0; i < TEXT_SIZE; i++) {
        prog.text[i] = 0;
    }
    prog.text[0] = 0x1001;
    prog.text[1] = 0x0000;
    prog.text_len = 2;
    prog.data[0] = 0x0008;
    prog.data_len = 1;
    prog.entry = 1;
    prog.n_symbols = 0;

    cpu_reset(&cpu);
    cpu.mem[0x90] = 0x5555;                 /* leftover from an earlier run */
    check(load_program(&prog, &cpu, &err) == 0, "loader: loads a valid program");
    check(cpu.mem[0x00] == 0x1001 && cpu.mem[0x60] == 0x0008, "loader: TEXT at 0x00, DATA at 0x60");
    check(cpu.mem[0x90] == 0, "loader: HEAP cleared");
    check(cpu.pc == 1 && cpu.regs[REG_SP] == 0x100 && cpu.regs[REG_FP] == 0x100,
          "loader: PC = entry, SP = FP = 0x100");

    prog.entry = 5;
    check(load_program(&prog, &cpu, &err) != 0 && err.kind == ERR_ENTRY_RANGE && err.addr == 5,
          "loader: entry point outside text");
    prog.entry = 0;
    prog.text_len = 97;
    check(load_program(&prog, &cpu, &err) != 0 && err.kind == ERR_TEXT_TOO_LARGE
          && err.value == 97, "loader: text segment too large");
    prog.text_len = 2;
    prog.data_len = 33;
    check(load_program(&prog, &cpu, &err) != 0 && err.kind == ERR_DATA_TOO_LARGE,
          "loader: data segment too large");
    prog.data_len = -1;
    check(load_program(&prog, &cpu, &err) != 0 && err.kind == ERR_BAD_OBJECT,
          "loader: bad object format");
}

/* ---- String helpers (text.c) -------------------------------------------- */

static void test_text(void)
{
    char buf[8];
    int v = 0;
    check(text_equal("ADD", "ADD") && !text_equal("ADD", "ADDI"), "text_equal");
    check(text_equal_ignore_case("add", "ADD") && !text_equal_ignore_case("ad", "ADD"),
          "text_equal_ignore_case");
    check(text_length("") == 0 && text_length("main") == 4, "text_length");
    text_copy(buf, "fact_recursive", 8);
    check(text_equal(buf, "fact_re"), "text_copy cuts long text");
    check(parse_number("12", &v) == 0 && v == 12, "parse_number decimal");
    check(parse_number("08", &v) == 0 && v == 8, "parse_number 08 (leading zero)");
    check(parse_number("-3", &v) == 0 && v == -3, "parse_number negative");
    check(parse_number("0x1F", &v) == 0 && v == 31, "parse_number hex");
    check(parse_number("-0x1", &v) == 0 && v == -1, "parse_number negative hex");
    check(parse_number("0x", &v) != 0 && parse_number("1a", &v) != 0
          && parse_number("", &v) != 0, "parse_number rejects non-numbers");
}

/* ---- Golden runs from raw hex (SPEC 9) ---------------------------------- */

/*
 * load_hex: build a Program from hex words and load it (entry 0).
 */
static void load_hex(CPU *cpu, const unsigned short *text, int n_text,
                     const unsigned short *data, int n_data)
{
    Program prog;
    SimError err;
    int i;
    for (i = 0; i < n_text; i++) {
        prog.text[i] = text[i];
    }
    for (i = 0; i < n_data; i++) {
        prog.data[i] = data[i];
    }
    prog.text_len = n_text;
    prog.data_len = n_data;
    prog.entry = 0;
    prog.n_symbols = 0;
    load_program(&prog, cpu, &err);
}

static const unsigned short FACTORIAL_HEX[] = {
    0x1960, 0x1001, 0x5020, 0x6101, 0x8802, 0x2061, 0xB800, 0x0000 };
static const unsigned short FACTORIAL_DATA[] = { 0x0008, 0x0000 };

static const unsigned short POWER_HEX[] = {
    0x1001, 0x1100, 0x1A61, 0x1B60, 0x6940, 0x9007, 0x780A,
    0x5060, 0x5901, 0x7804, 0x2062, 0xB800, 0x0000 };
static const unsigned short POWER_DATA[] = { 0x0008, 0x0002, 0x0000 };

static const unsigned short RECURSIVE_HEX[] = {
    0x1960, 0x9900, 0xA807, 0x5F01, 0x2061, 0xB800, 0x0000,
    0x29C2, 0x7102, 0x9011, 0x6101, 0x9900, 0xA807, 0x5F01,
    0x29C2, 0x5020, 0xB000, 0x1001, 0xB000 };
static const unsigned short RECURSIVE_DATA[] = { 0x0005, 0x0000 };

/*
 * check_factorial: SPEC 9.1 expected final state.
 */
static void check_factorial(const CPU *cpu, const char *label)
{
    printf("  golden %s: factorial\n", label);
    check(cpu->halted && cpu->steps == 29, "factorial: 29 steps");
    check(cpu->n_output == 1 && cpu->output[0] == 40320, "factorial: output 40320");
    check(cpu->mem[0x61] == 0x9D80, "factorial: M[0x61] = 9D80");
    check(cpu->regs[0] == 0x9D80 && cpu->regs[1] == 0, "factorial: R0 = 9D80, R1 = 0");
    check(cpu->regs[REG_FP] == 0x100 && cpu->regs[REG_SP] == 0x100, "factorial: FP = SP = 0100");
    check(cpu->flag_z == 1 && cpu->flag_n == 0, "factorial: Z=1 N=0");
}

/*
 * check_power: SPEC 9.2 expected final state.
 */
static void check_power(const CPU *cpu, const char *label)
{
    printf("  golden %s: power\n", label);
    check(cpu->halted && cpu->steps == 20, "power: 20 steps");
    check(cpu->n_output == 1 && cpu->output[0] == 64, "power: output 64");
    check(cpu->mem[0x62] == 0x0040, "power: M[0x62] = 0040");
    check(cpu->regs[0] == 0x40 && cpu->regs[1] == 2 && cpu->regs[2] == 2 && cpu->regs[3] == 8,
          "power: R0=0040 R1=0002 R2=0002 R3=0008");
    check(cpu->regs[REG_FP] == 0x100 && cpu->regs[REG_SP] == 0x100, "power: FP = SP = 0100");
    check(cpu->flag_z == 1 && cpu->flag_n == 0, "power: Z=1 N=0");
}

/*
 * run_recursive: step the loaded recursive program to HALT, checking the
 * stack after each of the 5 CALLs against the SPEC 9.3 table.
 */
static void run_recursive(CPU *cpu, const char *label)
{
    /* SPEC 9.3: step, SP = FP, and the new frame (saved FP, return, arg) */
    static const int call_step[5] = { 3, 9, 15, 21, 27 };
    static const int call_sp[5] = { 0xFD, 0xFA, 0xF7, 0xF4, 0xF1 };
    static const int frame[5][3] = {
        { 0x0100, 0x0003, 0x0005 },     /* fact(5): called from main, returns to 0x03 */
        { 0x00FD, 0x000D, 0x0004 },
        { 0x00FA, 0x000D, 0x0003 },
        { 0x00F7, 0x000D, 0x0002 },
        { 0x00F4, 0x000D, 0x0001 } };
    SimError err;
    int calls = 0, ok = 1, k, a;

    printf("  golden %s: recursive\n", label);
    while (!cpu->halted && cpu->steps < DEFAULT_MAX_STEPS) {
        if (cpu_step(cpu, &err) != 0) {
            ok = 0;
            break;
        }
        if (cpu->last_event.type != EV_CALL) {
            continue;
        }
        if (calls < 5) {
            int sp = call_sp[calls];
            check(cpu->steps == call_step[calls], "recursive: CALL at the expected step");
            check(cpu->regs[REG_SP] == sp && cpu->regs[REG_FP] == sp, "recursive: SP = FP");
            check(cpu->mem[sp] == frame[calls][0], "recursive: saved FP");
            check(cpu->mem[sp + 1] == frame[calls][1], "recursive: return address");
            check(cpu->mem[sp + 2] == frame[calls][2], "recursive: argument");
            check(cpu->last_event.arg == frame[calls][2], "recursive: CALL event arg");
            /* every older frame is still there, unchanged */
            for (k = 0; k < calls; k++) {
                a = call_sp[k];
                if (cpu->mem[a] != frame[k][0] || cpu->mem[a + 2] != frame[k][2]) {
                    ok = 0;
                }
            }
        }
        calls++;
    }
    check(ok, "recursive: runs without error, caller frames intact");
    check(calls == 5, "recursive: exactly 5 CALLs");
    check(cpu->halted && cpu->steps == 52, "recursive: 52 steps");
    check(cpu->n_output == 1 && cpu->output[0] == 120, "recursive: output 120");
    check(cpu->mem[0x61] == 0x0078, "recursive: M[0x61] = 0078");
    check(cpu->regs[0] == 0x78 && cpu->regs[1] == 5, "recursive: R0 = 0078, R1 = 0005");
    check(cpu->regs[REG_FP] == 0x100 && cpu->regs[REG_SP] == 0x100,
          "recursive: FP = SP = 0100 (stack empty)");
    check(cpu->flag_z == 0 && cpu->flag_n == 0, "recursive: Z=0 N=0");
    check(cpu->mem[0xF1] == 0xF4 && cpu->mem[0xF3] == 1, "recursive: stale values stay at F1-F3");
}

static void test_golden_hex(void)
{
    CPU cpu;
    SimError err;

    load_hex(&cpu, FACTORIAL_HEX, 8, FACTORIAL_DATA, 2);
    check(cpu_run(&cpu, DEFAULT_MAX_STEPS, &err) == 0, "factorial (hex) runs");
    check_factorial(&cpu, "(hex)");

    load_hex(&cpu, POWER_HEX, 13, POWER_DATA, 3);
    check(cpu_run(&cpu, DEFAULT_MAX_STEPS, &err) == 0, "power (hex) runs");
    check_power(&cpu, "(hex)");

    load_hex(&cpu, RECURSIVE_HEX, 19, RECURSIVE_DATA, 2);
    run_recursive(&cpu, "(hex)");
}

/* ---- Assembler: SPEC 9.4 rows, both ways -------------------------------- */

static void test_encoding_table(void)
{
    static const char *const ASM[24] = {
        "HALT", "NOP", "LDI R0, 1", "LD R1, 0x60", "ST R0, 0x61", "LDR R1, [FP+2]",
        "STR R2, [SP-1]", "MOV R1, R2", "ADD R1, R2", "SUB R1, R2", "MUL R0, R1",
        "ADDI SP, 1", "SUBI R1, 1", "CMP R1, R2", "CMPI R1, 2", "JMP 0x04", "JZ 0x10",
        "JNZ 0x02", "JN 0x11", "PUSH R1", "POP R2", "CALL 0x07", "RET", "OUT R0" };
    static const unsigned short HEX[24] = {
        0x0000, 0x0800, 0x1001, 0x1960, 0x2061, 0x29C2, 0x32FF, 0x3940,
        0x4140, 0x4940, 0x5020, 0x5F01, 0x6101, 0x6940, 0x7102, 0x7804,
        0x8010, 0x8802, 0x9011, 0x9900, 0xA200, 0xA807, 0xB000, 0xB800 };
    Program prog;
    SimError err;
    char text[DISASM_LEN];
    int i, ok_asm = 1, ok_dis = 1;

    for (i = 0; i < 24; i++) {
        const char *line[1];
        line[0] = ASM[i];
        if (assemble(line, 1, &prog, &err) != 0 || prog.text_len != 1 || prog.text[0] != HEX[i]) {
            ok_asm = 0;
            printf("  assemble \"%s\" != %04X\n", ASM[i], HEX[i]);
        }
        disassemble(HEX[i], 0, text);
        if (!text_equal(text, ASM[i])) {
            ok_dis = 0;
            printf("  disassemble %04X = \"%s\", expected \"%s\"\n", HEX[i], text, ASM[i]);
        }
    }
    check(ok_asm, "SPEC 9.4: all 24 rows assemble to the right hex");
    check(ok_dis, "SPEC 9.4: all 24 rows disassemble back to the same text");
    disassemble(0xC000, 0, text);
    check(text_equal(text, ".word 0xC000  ; invalid"), "disassemble a reserved opcode");
    disassemble(0x2BE0, 0, text);
    check(text_equal(text, "LDR R3, [SP+0]"), "disassemble shows +0");
}

/* ---- Assembler: syntax rules (SPEC 3.9) --------------------------------- */

static void test_syntax(void)
{
    static const char *const SRC[] = {
        "; comment-only line",
        "",
        "start:  add r1,r2          ; lower case, no space after comma",
        "        LDR R1, [ FP + 2 ] ; spaces inside brackets",
        "        LDR R2, [SP]       ; no offset",
        "        JMP done           ; forward reference",
        "        LDI R3, 0x1F       ; hex",
        "        LDI R4, 08         ; leading zero is decimal",
        "        LD  R5, v",
        "done:",
        "        HALT",
        ".data",
        "v:      .word -1           ; stored as two's complement",
        "w:      .word done         ; a label as a value",
        "        .word 0xFFFF" };
    static const unsigned short WANT[] = {
        0x4140, 0x29C2, 0x2AE0, 0x7807, 0x131F, 0x1408, 0x1D60, 0x0000 };
    Program prog;
    SimError err;
    int i, ok = 1;

    check(assemble(SRC, 15, &prog, &err) == 0, "syntax: program assembles");
    check(prog.text_len == 8, "syntax: 8 instructions");
    for (i = 0; i < 8; i++) {
        if (prog.text[i] != WANT[i]) {
            ok = 0;
            printf("  word %d = %04X, expected %04X\n", i, prog.text[i], WANT[i]);
        }
    }
    check(ok, "syntax: case, spaces, forward label, hex, 08");
    check(prog.data_len == 3 && prog.data[0] == 0xFFFF && prog.data[1] == 7
          && prog.data[2] == 0xFFFF, "syntax: .word -1, .word label, .word 0xFFFF");
    check(prog.entry == 0, "syntax: no main -> entry 0");
    check(prog.n_symbols == 4 && prog.symbols[1].addr == 7 && prog.symbols[2].addr == 0x60,
          "syntax: label addresses (text and data)");
    check(prog.source_line[0] == 3 && prog.source_line[7] == 11, "syntax: source line numbers");
}

/* ---- Assembler errors (SPEC 3.8) ----------------------------------------- */

/*
 * expect_asm_error: assembling lines must fail with this kind on this line.
 */
static void expect_asm_error(const char *const *lines, int n, int kind, int line,
                             const char *message)
{
    Program prog;
    SimError err;
    check(assemble(lines, n, &prog, &err) != 0 && err.kind == kind && err.line == line, message);
}

static void test_asm_errors(void)
{
    static const char *const MNEM[] = { "MOVE R1, R2" };
    static const char *const UNDEF[] = { "loop: NOP", "JMP lop" };
    static const char *const IMM[] = { "LDI R1, 300" };
    static const char *const OFF[] = { "LDR R1, [FP+20]" };
    static const char *const DUP[] = { "loop: NOP", "loop: NOP" };
    static const char *const COUNT[] = { "ADD R1" };
    static const char *const REG[] = { "PUSH R9" };
    static const char *const WORD[] = { ".text", ".word 5" };
    static const char *const INSTR[] = { ".data", "NOP" };
    static const char *const RES1[] = { "add: NOP" };
    static const char *const RES2[] = { "sp: NOP" };
    static const char *big_text[97];
    static const char *big_data[34];
    Program prog;
    SimError err;
    int i;

    expect_asm_error(MNEM, 1, ERR_UNKNOWN_MNEMONIC, 1, "line 1: unknown mnemonic 'MOVE'");
    assemble(MNEM, 1, &prog, &err);
    check(text_equal(err.name, "MOVE"), "unknown mnemonic keeps the name");
    expect_asm_error(UNDEF, 2, ERR_UNDEFINED_LABEL, 2, "line 2: undefined label 'lop'");
    expect_asm_error(IMM, 1, ERR_IMM_RANGE, 1, "line 1: immediate out of range 0..255: 300");
    assemble(IMM, 1, &prog, &err);
    check(err.value == 300, "immediate error keeps the value");
    expect_asm_error(OFF, 1, ERR_OFFSET_RANGE, 1, "line 1: offset out of range -16..15: 20");
    expect_asm_error(DUP, 2, ERR_DUPLICATE_LABEL, 2, "line 2: duplicate label 'loop'");
    expect_asm_error(COUNT, 1, ERR_OPERAND_COUNT, 1, "line 1: expected 2 operands, got 1");
    assemble(COUNT, 1, &prog, &err);
    check(err.value == 2 && err.addr == 1, "operand count error keeps 2 and 1");
    expect_asm_error(REG, 1, ERR_BAD_REGISTER, 1, "line 1: bad register 'R9'");
    expect_asm_error(WORD, 2, ERR_WORD_IN_TEXT, 2, "line 2: .word not allowed in .text");
    expect_asm_error(INSTR, 2, ERR_INSTR_IN_DATA, 2, "line 2: instruction not allowed in .data");
    expect_asm_error(RES1, 1, ERR_RESERVED_LABEL, 1, "line 1: label 'add' is a reserved word");
    expect_asm_error(RES2, 1, ERR_RESERVED_LABEL, 1, "line 1: label 'sp' is a reserved word");

    for (i = 0; i < 97; i++) {
        big_text[i] = "NOP";
    }
    expect_asm_error(big_text, 97, ERR_ASM_TEXT_TOO_LARGE, 0,
                     "line 0: text segment too large: 97 words (max 96)");
    big_data[0] = ".data";
    for (i = 1; i < 34; i++) {
        big_data[i] = ".word 1";
    }
    expect_asm_error(big_data, 34, ERR_ASM_DATA_TOO_LARGE, 0,
                     "line 0: data segment too large: 33 words (max 32)");
}

/* ---- Assembled programs: machine code + golden runs (SPEC 9) -------------- */

/*
 * assemble_builtin: assemble a program from programs.c; 1 if ok.
 */
static int assemble_builtin(const char *name, Program *prog)
{
    const ProgramSource *src = find_program(name);
    SimError err;
    return src != 0 && assemble(src->lines, src->n_lines, prog, &err) == 0;
}

/*
 * same_words: do the first n words match?
 */
static int same_words(const unsigned short *a, const unsigned short *b, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static void test_programs_in_build(void)
{
    int want = (ASSIGNMENT == 0) ? 3 : (ASSIGNMENT == 1 ? 2 : 1);
    check(NUM_PROGRAMS == want, "build holds the right number of programs");
    check((find_program("factorial") != 0) == (ASSIGNMENT != 2), "factorial only in A1 builds");
    check((find_program("power") != 0) == (ASSIGNMENT != 2), "power only in A1 builds");
    check((find_program("recursive") != 0) == (ASSIGNMENT != 1), "recursive only in A2 builds");
    check(find_program("nope") == 0, "unknown program not found");
}

#if ASSIGNMENT == 0 || ASSIGNMENT == 1
static void test_assembled_a1(void)
{
    Program prog;
    CPU cpu;
    SimError err;
    char text[DISASM_LEN];

    check(assemble_builtin("factorial", &prog), "factorial assembles");
    check(prog.text_len == 8 && same_words(prog.text, FACTORIAL_HEX, 8),
          "factorial: machine code = SPEC 9.1");
    check(prog.data_len == 2 && same_words(prog.data, FACTORIAL_DATA, 2), "factorial: data");
    check(prog.n_symbols == 4 && text_equal(prog.symbols[1].name, "loop")
          && prog.symbols[1].addr == 2 && prog.symbols[3].addr == 0x61, "factorial: labels");
    check(prog.source_line[0] == 5 && prog.source_line[7] == 13, "factorial: source lines");
    disassemble(prog.text[4], &prog, text);
    check(text_equal(text, "JNZ loop"), "factorial: JNZ loop (jump shows TEXT label)");
    disassemble(prog.text[0], &prog, text);
    check(text_equal(text, "LD R1, n"), "factorial: LD R1, n (address shows DATA label)");
    disassemble(0x1900, &prog, text);
    check(text_equal(text, "LD R1, 0x00"), "LD R1, 0x00 is not shown as main");
    load_program(&prog, &cpu, &err);
    cpu_run(&cpu, DEFAULT_MAX_STEPS, &err);
    check_factorial(&cpu, "(assembled)");

    check(assemble_builtin("power", &prog), "power assembles");
    check(prog.text_len == 13 && same_words(prog.text, POWER_HEX, 13),
          "power: machine code = SPEC 9.2");
    check(prog.data_len == 3 && same_words(prog.data, POWER_DATA, 3), "power: data");
    load_program(&prog, &cpu, &err);
    cpu_run(&cpu, DEFAULT_MAX_STEPS, &err);
    check_power(&cpu, "(assembled)");
}
#endif

#if ASSIGNMENT == 0 || ASSIGNMENT == 2
static void test_assembled_a2(void)
{
    /* SPEC 7.4 INSTRUCTION column */
    static const char *const LISTING[19] = {
        "LD R1, n", "PUSH R1", "CALL fact", "ADDI SP, 1", "ST R0, result", "OUT R0",
        "HALT", "LDR R1, [FP+2]", "CMPI R1, 2", "JN base", "SUBI R1, 1", "PUSH R1",
        "CALL fact", "ADDI SP, 1", "LDR R1, [FP+2]", "MUL R0, R1", "RET", "LDI R0, 1", "RET" };
    static const char *const NAMES[5] = { "main", "fact", "base", "n", "result" };
    static const int ADDRS[5] = { 0x00, 0x07, 0x11, 0x60, 0x61 };
    Program prog;
    CPU cpu;
    SimError err;
    char text[DISASM_LEN];
    int i, ok = 1;

    check(assemble_builtin("recursive", &prog), "recursive assembles");
    check(prog.text_len == 19 && same_words(prog.text, RECURSIVE_HEX, 19),
          "recursive: machine code = SPEC 9.3");
    check(prog.data_len == 2 && same_words(prog.data, RECURSIVE_DATA, 2), "recursive: data");
    for (i = 0; i < 5; i++) {
        if (!text_equal(prog.symbols[i].name, NAMES[i]) || prog.symbols[i].addr != ADDRS[i]) {
            ok = 0;
        }
    }
    check(ok && prog.n_symbols == 5, "recursive: symbols main=00 fact=07 base=11 n=60 result=61");
    ok = 1;
    for (i = 0; i < 19; i++) {
        disassemble(prog.text[i], &prog, text);
        if (!text_equal(text, LISTING[i])) {
            ok = 0;
            printf("  %02X: \"%s\", expected \"%s\"\n", i, text, LISTING[i]);
        }
    }
    check(ok, "recursive: disassembly = SPEC 7.4 listing");
    load_program(&prog, &cpu, &err);
    run_recursive(&cpu, "(assembled)");
}
#endif

/* ---- Main --------------------------------------------------------------- */

int main(void)
{
    printf("SIM16 tests (build: ASSIGNMENT %d)\n", ASSIGNMENT);

    printf("CPU: reset, snapshot\n");
    test_reset();
    test_snapshot_is_copy();

    printf("CPU: one test per instruction\n");
    test_halt();
    test_nop();
    test_ldi();
    test_ld();
    test_st();
    test_ldr();
    test_str();
    test_mov();
    test_add();
    test_sub();
    test_mul();
    test_addi();
    test_subi();
    test_cmp();
    test_cmpi();
    test_flags_unchanged();
    test_jmp();
    test_jz();
    test_jnz();
    test_jn();
    test_push();
    test_pop();
    test_call();
    test_ret();
    test_out();

    printf("CPU: errors\n");
    test_errors();
    test_max_steps();

    printf("Loader\n");
    test_loader();

    printf("String helpers\n");
    test_text();

    printf("Assembler\n");
    test_encoding_table();
    test_syntax();
    test_asm_errors();
    test_programs_in_build();

    printf("Golden runs (SPEC 9)\n");
    test_golden_hex();
#if ASSIGNMENT == 0 || ASSIGNMENT == 1
    test_assembled_a1();
#endif
#if ASSIGNMENT == 0 || ASSIGNMENT == 2
    test_assembled_a2();
#endif

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
