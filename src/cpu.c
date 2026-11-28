/*
 * cpu.c -- SIM16 CPU: reset, fetch/decode/execute, stack, CALL/RET
 *
 * Purpose : Assignment 1 (the CPU we designed) and Assignment 2 (the stack
 *           and CALL/RET used by the recursive program).
 * Spec    : SPEC 3.4 (instructions + flags), 3.5 (step order),
 *           3.6 (stack, CALL, RET), 3.8 (error messages), 5 (state, events).
 *
 * Rule: every check is made BEFORE anything changes, so an instruction that
 * fails leaves the CPU exactly as it was.
 */
#include "cpu.h"

/*
 * set_error: fill in an error record.
 */
static void set_error(SimError *err, int kind, int value, int addr)
{
    err->kind = kind;
    err->line = 0;
    err->value = value;
    err->addr = addr;
    err->name[0] = '\0';
}

/*
 * word16: keep the low 16 bits (all SIM16 arithmetic wraps around).
 */
static unsigned short word16(long x)
{
    return (unsigned short)(x & WORD_MASK);
}

/*
 * set_flags: Z and N from the 16-bit value of a result (SPEC 3.4).
 */
static void set_flags(CPU *cpu, long result)
{
    int v = word16(result);
    cpu->flag_z = (v == 0) ? 1 : 0;     /* Z: result is zero */
    cpu->flag_n = (v >> 15) & 1;        /* N: bit 15 (negative if signed) */
}

/*
 * addr_ok: is this a valid memory address (0x00..0xFF)?
 */
static int addr_ok(int addr)
{
    return addr >= 0 && addr < MEM_SIZE;
}

/*
 * cpu_reset: power-on state (SPEC 4.3 step 2).
 * Memory and R0-R5 are 0, FP = SP = 0x100 (empty stack), PC = 0.
 */
void cpu_reset(CPU *cpu)
{
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        cpu->mem[i] = 0;
    }
    for (i = 0; i < NUM_REGS; i++) {
        cpu->regs[i] = 0;
    }
    cpu->regs[REG_FP] = STACK_TOP;
    cpu->regs[REG_SP] = STACK_TOP;
    cpu->pc = 0;
    cpu->ir = 0;
    cpu->exec_addr = -1;                /* nothing executed yet */
    cpu->flag_z = 0;
    cpu->flag_n = 0;
    cpu->halted = 0;
    cpu->steps = 0;
    for (i = 0; i < MAX_OUTPUT; i++) {
        cpu->output[i] = 0;
    }
    cpu->n_output = 0;
    cpu->last_event.type = EV_NONE;
    cpu->last_event.target = 0;
    cpu->last_event.ret_addr = 0;
    cpu->last_event.arg = 0;
    cpu->last_event.value = 0;
}

/*
 * cpu_step: execute ONE instruction (SPEC 3.5).
 *   1. check: not halted, PC in range
 *   2. FETCH  the word at PC
 *   3. DECODE its fields
 *   4. EXECUTE (switch on the opcode)
 *   5. update PC, IR, step count, event
 * Returns 0 on success, -1 on error (err filled in, CPU unchanged).
 */
int cpu_step(CPU *cpu, SimError *err)
{
    int pc, next_pc, word;
    int opcode, rd, rs, off5, imm8;
    int sp, fp, addr, value;
    Event ev = { EV_NONE, 0, 0, 0, 0 };

    if (cpu->halted) {
        set_error(err, ERR_HALTED, 0, 0);
        return -1;
    }
    if (cpu->pc < 0 || cpu->pc >= MEM_SIZE) {
        set_error(err, ERR_PC_RANGE, 0, cpu->pc);
        return -1;
    }

    /* FETCH */
    pc = cpu->pc;
    word = cpu->mem[pc];
    next_pc = pc + 1;                   /* PC now points at the next instruction */

    /* DECODE (SPEC 3.3) */
    opcode = (word >> OP_SHIFT) & OP_MASK;
    rd = (word >> RD_SHIFT) & REG_MASK;
    rs = (word >> RS_SHIFT) & REG_MASK;
    off5 = word & OFF5_MASK;
    if (off5 & 0x10) {
        off5 -= 32;                     /* sign-extend: -16 .. +15 */
    }
    imm8 = word & IMM8_MASK;            /* 0 .. 255 */
    sp = cpu->regs[REG_SP];
    fp = cpu->regs[REG_FP];

    /* EXECUTE */
    switch (opcode) {
    case OP_HALT:   /* HALT : stop execution */
        cpu->halted = 1;
        ev.type = EV_HALT;
        break;

    case OP_NOP:    /* NOP : do nothing */
        break;

    case OP_LDI:    /* LDI rd, imm8 : rd <- imm8 */
        cpu->regs[rd] = (unsigned short)imm8;
        break;

    case OP_LD:     /* LD rd, addr8 : rd <- M[addr8] */
        cpu->regs[rd] = cpu->mem[imm8];
        break;

    case OP_ST:     /* ST rd, addr8 : M[addr8] <- rd */
        cpu->mem[imm8] = cpu->regs[rd];
        break;

    case OP_LDR:    /* LDR rd, [rs+off5] : rd <- M[rs + off5] */
        addr = cpu->regs[rs] + off5;
        if (!addr_ok(addr)) {
            set_error(err, ERR_ADDR_RANGE, 0, addr);
            return -1;
        }
        cpu->regs[rd] = cpu->mem[addr];
        break;

    case OP_STR:    /* STR rd, [rs+off5] : M[rs + off5] <- rd */
        addr = cpu->regs[rs] + off5;
        if (!addr_ok(addr)) {
            set_error(err, ERR_ADDR_RANGE, 0, addr);
            return -1;
        }
        cpu->mem[addr] = cpu->regs[rd];
        break;

    case OP_MOV:    /* MOV rd, rs : rd <- rs */
        cpu->regs[rd] = cpu->regs[rs];
        break;

    case OP_ADD:    /* ADD rd, rs : rd <- rd + rs, sets Z,N */
        cpu->regs[rd] = word16((long)cpu->regs[rd] + cpu->regs[rs]);
        set_flags(cpu, cpu->regs[rd]);
        break;

    case OP_SUB:    /* SUB rd, rs : rd <- rd - rs, sets Z,N */
        cpu->regs[rd] = word16((long)cpu->regs[rd] - cpu->regs[rs]);
        set_flags(cpu, cpu->regs[rd]);
        break;

    case OP_MUL:    /* MUL rd, rs : rd <- (rd * rs) & 0xFFFF, sets Z,N */
        cpu->regs[rd] = word16((long)cpu->regs[rd] * (long)cpu->regs[rs]);
        set_flags(cpu, cpu->regs[rd]);
        break;

    case OP_ADDI:   /* ADDI rd, imm8 : rd <- rd + imm8, sets Z,N */
        cpu->regs[rd] = word16((long)cpu->regs[rd] + imm8);
        set_flags(cpu, cpu->regs[rd]);
        break;

    case OP_SUBI:   /* SUBI rd, imm8 : rd <- rd - imm8, sets Z,N */
        cpu->regs[rd] = word16((long)cpu->regs[rd] - imm8);
        set_flags(cpu, cpu->regs[rd]);
        break;

    case OP_CMP:    /* CMP rd, rs : flags from rd - rs, result discarded */
        set_flags(cpu, (long)cpu->regs[rd] - cpu->regs[rs]);
        break;

    case OP_CMPI:   /* CMPI rd, imm8 : flags from rd - imm8, result discarded */
        set_flags(cpu, (long)cpu->regs[rd] - imm8);
        break;

    case OP_JMP:    /* JMP addr8 : PC <- addr8 */
        next_pc = imm8;
        break;

    case OP_JZ:     /* JZ addr8 : if Z = 1, PC <- addr8 */
        if (cpu->flag_z) {
            next_pc = imm8;
        }
        break;

    case OP_JNZ:    /* JNZ addr8 : if Z = 0, PC <- addr8 */
        if (!cpu->flag_z) {
            next_pc = imm8;
        }
        break;

    case OP_JN:     /* JN addr8 : if N = 1, PC <- addr8 */
        if (cpu->flag_n) {
            next_pc = imm8;
        }
        break;

    case OP_PUSH:   /* PUSH rd : SP <- SP - 1; M[SP] <- rd */
        if (sp - 1 < STACK_BASE) {
            set_error(err, ERR_STACK_OVERFLOW, 0, 0);   /* heap never written */
            return -1;
        }
        if (!addr_ok(sp - 1)) {
            set_error(err, ERR_ADDR_RANGE, 0, sp - 1);
            return -1;
        }
        cpu->mem[sp - 1] = cpu->regs[rd];
        cpu->regs[REG_SP] = (unsigned short)(sp - 1);
        break;

    case OP_POP:    /* POP rd : rd <- M[SP]; SP <- SP + 1 */
        if (sp >= STACK_TOP) {
            set_error(err, ERR_STACK_UNDERFLOW, 0, 0);
            return -1;
        }
        value = cpu->mem[sp];
        cpu->regs[REG_SP] = (unsigned short)(sp + 1);
        cpu->regs[rd] = (unsigned short)value;          /* after SP, so POP SP works */
        break;

    case OP_CALL:   /* CALL addr8 : push PC; push FP; FP <- SP; PC <- addr8 */
        if (sp - 2 < STACK_BASE) {
            set_error(err, ERR_STACK_OVERFLOW, 0, 0);   /* room for both words? */
            return -1;
        }
        if (!addr_ok(sp - 1)) {
            set_error(err, ERR_ADDR_RANGE, 0, sp - 1);
            return -1;
        }
        cpu->mem[sp - 1] = (unsigned short)next_pc;     /* 1. return address */
        cpu->mem[sp - 2] = (unsigned short)fp;          /* 2. caller's FP */
        cpu->regs[REG_SP] = (unsigned short)(sp - 2);
        cpu->regs[REG_FP] = (unsigned short)(sp - 2);   /* 3. FP <- SP */
        ev.type = EV_CALL;
        ev.target = imm8;
        ev.ret_addr = next_pc;
        ev.arg = addr_ok(sp) ? cpu->mem[sp] : -1;       /* argument = M[FP+2] */
        next_pc = imm8;                                 /* 4. jump */
        break;

    case OP_RET:    /* RET : SP <- FP; FP <- pop; PC <- pop */
        if (fp + 1 >= STACK_TOP) {
            set_error(err, ERR_STACK_UNDERFLOW, 0, 0);   /* no frame to leave */
            return -1;
        }
        next_pc = cpu->mem[fp + 1];                     /* return address */
        cpu->regs[REG_FP] = cpu->mem[fp];               /* caller's FP */
        cpu->regs[REG_SP] = (unsigned short)(fp + 2);   /* frame removed */
        ev.type = EV_RET;
        ev.ret_addr = next_pc;                          /* where we return to */
        ev.value = cpu->regs[0];                        /* return value in R0 */
        break;

    case OP_OUT:    /* OUT rd : print rd (append to output) */
        if (cpu->n_output < MAX_OUTPUT) {
            cpu->output[cpu->n_output] = cpu->regs[rd];
            cpu->n_output++;
        }
        ev.type = EV_OUT;
        ev.value = cpu->regs[rd];
        break;

    default:        /* 0x18..0x1F : reserved */
        set_error(err, ERR_INVALID_OPCODE, opcode, pc);
        return -1;
    }

    /* Instruction done: record it */
    cpu->exec_addr = pc;
    cpu->ir = (unsigned short)word;
    cpu->pc = next_pc;
    cpu->steps++;
    cpu->last_event = ev;
    return 0;
}

/*
 * cpu_run: step until HALT.
 *   max_steps : stop with an error after this many instructions
 * Returns 0 on success, -1 on error (err filled in).
 */
int cpu_run(CPU *cpu, int max_steps, SimError *err)
{
    int count = 0;
    while (!cpu->halted) {
        if (count >= max_steps) {
            set_error(err, ERR_MAX_STEPS, max_steps, 0);
            return -1;
        }
        if (cpu_step(cpu, err) != 0) {
            return -1;
        }
        count++;
    }
    return 0;
}
