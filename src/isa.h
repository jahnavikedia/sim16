/*
 * isa.h -- SIM16 instruction set: shared constants and types
 *
 * Purpose : the team's agreed design (SPEC 3.1-3.4): word size, memory map,
 *           registers, instruction fields, opcodes, error kinds, and the
 *           Program struct passed from the assembler to the loader.
 * Used by : every other file (both assignments).
 */
#ifndef ISA_H
#define ISA_H

/* ---- Word and memory --------------------------------------------------- */
#define WORD_MASK   0xFFFF      /* every register and memory cell is 16 bits */
#define MEM_SIZE    256         /* word-addressed: 0x00 .. 0xFF */

/* ---- Memory map: 4 segments (start inclusive, end exclusive) ----------- */
#define TEXT_BASE   0x00        /* 96 words: machine code */
#define TEXT_END    0x60
#define DATA_BASE   0x60        /* 32 words: global variables */
#define DATA_END    0x80
#define HEAP_BASE   0x80        /* 32 words: dynamic memory (unused) */
#define HEAP_END    0xA0
#define STACK_BASE  0xA0        /* 96 words: call stack, grows DOWN */
#define STACK_END   0x100
#define STACK_TOP   0x100       /* initial SP and FP = empty stack */

#define TEXT_SIZE   (TEXT_END - TEXT_BASE)     /* 96 */
#define DATA_SIZE   (DATA_END - DATA_BASE)     /* 32 */

/* ---- Registers --------------------------------------------------------- */
#define NUM_REGS    8           /* R0..R7 */
#define REG_FP      6           /* R6 = frame pointer */
#define REG_SP      7           /* R7 = stack pointer */

/* ---- Instruction word fields (SPEC 3.3) --------------------------------
 *  15      11 10   8 7    5 4      0
 * +----------+------+------+--------+
 * |  opcode  |  rd  |  rs  |  off5  |
 * +----------+------+------+--------+
 *                   |      imm8     |   (I and J formats)
 */
#define OP_SHIFT    11
#define RD_SHIFT    8
#define RS_SHIFT    5
#define OP_MASK     0x1F
#define REG_MASK    0x7
#define IMM8_MASK   0xFF
#define OFF5_MASK   0x1F

/* ---- Opcodes (SPEC 3.4); 0x18..0x1F are reserved -> invalid opcode ----- */
#define OP_HALT     0x00
#define OP_NOP      0x01
#define OP_LDI      0x02
#define OP_LD       0x03
#define OP_ST       0x04
#define OP_LDR      0x05
#define OP_STR      0x06
#define OP_MOV      0x07
#define OP_ADD      0x08
#define OP_SUB      0x09
#define OP_MUL      0x0A
#define OP_ADDI     0x0B
#define OP_SUBI     0x0C
#define OP_CMP      0x0D
#define OP_CMPI     0x0E
#define OP_JMP      0x0F
#define OP_JZ       0x10
#define OP_JNZ      0x11
#define OP_JN       0x12
#define OP_PUSH     0x13
#define OP_POP      0x14
#define OP_CALL     0x15
#define OP_RET      0x16
#define OP_OUT      0x17
#define NUM_OPCODES 24          /* valid opcodes are 0 .. NUM_OPCODES-1 */

/* ---- Instruction formats (SPEC 3.3) ------------------------------------ */
#define FMT_N       0           /* no operands        HALT */
#define FMT_R       1           /* rd, rs             ADD R1, R2 */
#define FMT_I       2           /* rd, imm8           LDI R1, 5 */
#define FMT_M       3           /* rd, [rs+off5]      LDR R1, [FP+2] */
#define FMT_J       4           /* addr8              JMP loop */
#define FMT_S       5           /* rd only            PUSH R1 */

/* ---- Limits ------------------------------------------------------------ */
#define DEFAULT_MAX_STEPS 10000 /* stop runaway programs */
#define MAX_OUTPUT  16          /* values kept from OUT */
#define NAME_LEN    32          /* max label length + 1 */
#define MAX_SYMBOLS 64          /* labels per program */

/* ---- Error kinds (messages in SPEC 3.8, printed by print_error) -------- */
enum {
    ERR_NONE = 0,
    /* cpu */
    ERR_HALTED,             /* CPU is halted */
    ERR_PC_RANGE,           /* PC out of range: 0x<addr> */
    ERR_INVALID_OPCODE,     /* invalid opcode 0x<value> at 0x<addr> */
    ERR_STACK_OVERFLOW,     /* stack overflow */
    ERR_STACK_UNDERFLOW,    /* stack underflow */
    ERR_ADDR_RANGE,         /* memory address out of range: 0x<addr> */
    ERR_MAX_STEPS,          /* exceeded max steps (<value>), infinite loop? */
    /* loader */
    ERR_BAD_OBJECT,         /* bad object format */
    ERR_TEXT_TOO_LARGE,     /* text segment too large: <value> words (max 96) */
    ERR_DATA_TOO_LARGE,     /* data segment too large: <value> words (max 32) */
    ERR_ENTRY_RANGE,        /* entry point 0x<addr> outside text */
    /* assembler (all start with "line <line>: ") */
    ERR_UNKNOWN_MNEMONIC,   /* unknown mnemonic '<name>' */
    ERR_UNDEFINED_LABEL,    /* undefined label '<name>' */
    ERR_IMM_RANGE,          /* immediate out of range 0..255: <value> */
    ERR_OFFSET_RANGE,       /* offset out of range -16..15: <value> */
    ERR_DUPLICATE_LABEL,    /* duplicate label '<name>' */
    ERR_OPERAND_COUNT,      /* expected <value> operands, got <addr> */
    ERR_BAD_REGISTER,       /* bad register '<name>' */
    ERR_WORD_IN_TEXT,       /* .word not allowed in .text */
    ERR_INSTR_IN_DATA,      /* instruction not allowed in .data */
    ERR_ASM_TEXT_TOO_LARGE, /* text segment too large: <value> words (max 96) */
    ERR_ASM_DATA_TOO_LARGE, /* data segment too large: <value> words (max 32) */
    ERR_RESERVED_LABEL,     /* label '<name>' is a reserved word */
    ERR_BAD_NUMBER,         /* bad number '<name>' */
    ERR_BAD_OPERAND,        /* bad operand '<name>' */
    ERR_BAD_DIRECTIVE,      /* unknown directive '<name>' */
    ERR_WORD_RANGE,         /* .word value out of range: <value> */
    ERR_TOO_MANY_LABELS     /* too many labels (max 64) */
};

/* ---- Shared types ------------------------------------------------------ */

/* One error, filled in by the function that found it. */
typedef struct {
    int kind;               /* ERR_... */
    int line;               /* assembler: source line number */
    int value;              /* number shown in the message */
    int addr;               /* address shown in the message */
    char name[NAME_LEN];    /* label / mnemonic / register text */
} SimError;

/* One label and its address. */
typedef struct {
    char name[NAME_LEN];
    int addr;
} Symbol;

/* The assembled program ("object"): assembler -> loader (SPEC 4.1). */
typedef struct {
    unsigned short text[TEXT_SIZE];     /* machine code, goes to 0x00.. */
    int text_len;
    unsigned short data[DATA_SIZE];     /* initial globals, go to 0x60.. */
    int data_len;
    int entry;                          /* start address (label main) */
    Symbol symbols[MAX_SYMBOLS];        /* every label, in definition order */
    int n_symbols;
    int source_line[TEXT_SIZE];         /* source line of each text word */
} Program;

#endif
