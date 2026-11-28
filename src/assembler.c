/*
 * assembler.c -- SIM16 assembler (two passes) and disassembler
 *
 * Purpose : both assignments: our assembly language -> machine code, and
 *           machine code -> readable assembly (for every printed view).
 * Spec    : SPEC 3.9 (syntax), 3.10 (disassembly text), 3.8 (error messages).
 *
 *   Pass 1: read every line, give each label its address.
 *   Pass 2: read every line again, encode instructions and .word values
 *           (labels are all known now, so "JMP done" can jump forward).
 */
#include "assembler.h"
#include "text.h"

#define LINE_LEN     128    /* longest source line we keep */
#define MAX_OPERANDS 3      /* we store up to 3 to report "got 3" */

/* Mnemonic and format of each opcode (index = opcode, SPEC 3.4) */
static const char *MNEMONICS[NUM_OPCODES] = {
    "HALT", "NOP", "LDI", "LD", "ST", "LDR", "STR", "MOV",
    "ADD", "SUB", "MUL", "ADDI", "SUBI", "CMP", "CMPI", "JMP",
    "JZ", "JNZ", "JN", "PUSH", "POP", "CALL", "RET", "OUT"
};
static const int FORMATS[NUM_OPCODES] = {
    FMT_N, FMT_N, FMT_I, FMT_I, FMT_I, FMT_M, FMT_M, FMT_R,
    FMT_R, FMT_R, FMT_R, FMT_I, FMT_I, FMT_R, FMT_I, FMT_J,
    FMT_J, FMT_J, FMT_J, FMT_S, FMT_S, FMT_J, FMT_N, FMT_S
};
/* Operands each format needs: N R I M J S */
static const int OPERAND_COUNT[6] = { 0, 2, 2, 2, 1, 1 };

/* Register display names (R6 = FP, R7 = SP) */
static const char *REG_NAMES[NUM_REGS] = { "R0", "R1", "R2", "R3", "R4", "R5", "FP", "SP" };

/* What a source line contains */
#define LINE_EMPTY 0        /* blank, comment only, or label only */
#define LINE_TEXT  1        /* .text */
#define LINE_DATA  2        /* .data */
#define LINE_WORD  3        /* .word value */
#define LINE_INSTR 4        /* an instruction */

/* One source line, split into its parts */
typedef struct {
    int kind;                               /* LINE_... */
    char label[NAME_LEN];                   /* "" if no label */
    char word[NAME_LEN];                    /* mnemonic or directive, as written */
    int opcode;                             /* LINE_INSTR only */
    char ops[MAX_OPERANDS][NAME_LEN];       /* operand texts */
    int n_ops;
} Line;

/* ---- Errors ------------------------------------------------------------ */

/*
 * asm_error: fill in an assembler error and return -1.
 */
static int asm_error(SimError *err, int kind, int line, int value, int addr, const char *name)
{
    err->kind = kind;
    err->line = line;
    err->value = value;
    err->addr = addr;
    text_copy(err->name, name, NAME_LEN);
    return -1;
}

/* ---- Small parsing helpers -------------------------------------------- */

/*
 * skip_spaces: index of the first non-space character at or after i.
 */
static int skip_spaces(const char *s, int i)
{
    while (is_space(s[i])) {
        i++;
    }
    return i;
}

/*
 * copy_range: copy s[from..to) into dst (NAME_LEN chars).
 * Returns 0, or -1 if it does not fit.
 */
static int copy_range(char *dst, const char *s, int from, int to)
{
    int i;
    if (to - from >= NAME_LEN) {
        return -1;
    }
    for (i = from; i < to; i++) {
        dst[i - from] = s[i];
    }
    dst[to - from] = '\0';
    return 0;
}

/*
 * find_opcode: mnemonic (any case) -> opcode, or -1.
 */
static int find_opcode(const char *name)
{
    int op;
    for (op = 0; op < NUM_OPCODES; op++) {
        if (text_equal_ignore_case(name, MNEMONICS[op])) {
            return op;
        }
    }
    return -1;
}

/*
 * find_register: "R0".."R7", "FP", "SP" (any case) -> 0..7, or -1.
 */
static int find_register(const char *name)
{
    static const char *ALL[10] = { "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "FP", "SP" };
    static const int NUM[10] = { 0, 1, 2, 3, 4, 5, 6, 7, REG_FP, REG_SP };
    int i;
    for (i = 0; i < 10; i++) {
        if (text_equal_ignore_case(name, ALL[i])) {
            return NUM[i];
        }
    }
    return -1;
}

/*
 * find_symbol: label (exact case) -> index in prog->symbols, or -1.
 */
static int find_symbol(const Program *prog, const char *name)
{
    int i;
    for (i = 0; i < prog->n_symbols; i++) {
        if (text_equal(prog->symbols[i].name, name)) {
            return i;
        }
    }
    return -1;
}

/* ---- Reading one line -------------------------------------------------- */

/*
 * parse_line: split one source line into label / word / operands.
 *   "loop:  MUL R0, R1   ; comment"  ->  label "loop", word "MUL", ops "R0" "R1"
 * Returns 0, or -1 on error (err filled in).
 */
static int parse_line(const char *src, int line_no, Line *ln, SimError *err)
{
    char buf[LINE_LEN];
    int i, n, start, end;

    ln->kind = LINE_EMPTY;
    ln->label[0] = '\0';
    ln->word[0] = '\0';
    ln->opcode = -1;
    ln->n_ops = 0;

    text_copy(buf, src, LINE_LEN);
    for (i = 0; buf[i] != '\0'; i++) {          /* drop the comment */
        if (buf[i] == ';') {
            buf[i] = '\0';
            break;
        }
    }
    i = skip_spaces(buf, 0);
    if (buf[i] == '\0') {
        return 0;                               /* blank line */
    }

    /* Label: a name followed by ':' */
    if (is_alpha(buf[i])) {
        n = i;
        while (is_alpha(buf[n]) || is_digit(buf[n])) {
            n++;
        }
        if (buf[n] == ':') {
            if (copy_range(ln->label, buf, i, n) != 0) {
                return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, "label too long");
            }
            i = skip_spaces(buf, n + 1);
        }
    }
    if (buf[i] == '\0') {
        return 0;                               /* label alone on the line */
    }

    /* Mnemonic or directive */
    start = i;
    while (buf[i] != '\0' && !is_space(buf[i])) {
        i++;
    }
    if (copy_range(ln->word, buf, start, i) != 0) {
        return asm_error(err, ERR_UNKNOWN_MNEMONIC, line_no, 0, 0, "(too long)");
    }

    /* Operands: the rest, split at commas */
    i = skip_spaces(buf, i);
    while (buf[i] != '\0') {
        start = i;
        while (buf[i] != '\0' && buf[i] != ',') {
            i++;
        }
        end = i;
        while (end > start && is_space(buf[end - 1])) {
            end--;                              /* trim trailing spaces */
        }
        if (end == start) {
            return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, "");
        }
        if (ln->n_ops < MAX_OPERANDS
            && copy_range(ln->ops[ln->n_ops], buf, start, end) != 0) {
            return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, "operand too long");
        }
        ln->n_ops++;
        if (buf[i] == ',') {
            i = skip_spaces(buf, i + 1);        /* next operand */
            if (buf[i] == '\0') {
                return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, "");
            }
        }
    }

    /* Directive? */
    if (ln->word[0] == '.') {
        if (text_equal_ignore_case(ln->word, ".text")) {
            ln->kind = LINE_TEXT;
        } else if (text_equal_ignore_case(ln->word, ".data")) {
            ln->kind = LINE_DATA;
        } else if (text_equal_ignore_case(ln->word, ".word")) {
            ln->kind = LINE_WORD;
            if (ln->n_ops != 1) {
                return asm_error(err, ERR_OPERAND_COUNT, line_no, 1, ln->n_ops, "");
            }
            return 0;
        } else {
            return asm_error(err, ERR_BAD_DIRECTIVE, line_no, 0, 0, ln->word);
        }
        if (ln->n_ops != 0) {
            return asm_error(err, ERR_OPERAND_COUNT, line_no, 0, ln->n_ops, "");
        }
        return 0;
    }

    /* Instruction */
    ln->opcode = find_opcode(ln->word);
    if (ln->opcode < 0) {
        return asm_error(err, ERR_UNKNOWN_MNEMONIC, line_no, 0, 0, ln->word);
    }
    n = OPERAND_COUNT[FORMATS[ln->opcode]];
    if (ln->n_ops != n) {
        return asm_error(err, ERR_OPERAND_COUNT, line_no, n, ln->n_ops, "");
    }
    ln->kind = LINE_INSTR;
    return 0;
}

/* ---- Reading operands -------------------------------------------------- */

/*
 * read_register: operand text -> register number.
 */
static int read_register(const char *op, int line_no, int *reg, SimError *err)
{
    *reg = find_register(op);
    if (*reg < 0) {
        return asm_error(err, ERR_BAD_REGISTER, line_no, 0, 0, op);
    }
    return 0;
}

/*
 * read_value: operand text -> number. A number ("12", "-3", "0x1F")
 * or a label name (its address).
 */
static int read_value(const char *op, int line_no, const Program *prog, int *value,
                      SimError *err)
{
    int s;
    if (is_digit(op[0]) || op[0] == '-') {
        if (parse_number(op, value) != 0) {
            return asm_error(err, ERR_BAD_NUMBER, line_no, 0, 0, op);
        }
        return 0;
    }
    if (is_alpha(op[0])) {
        s = find_symbol(prog, op);
        if (s < 0) {
            return asm_error(err, ERR_UNDEFINED_LABEL, line_no, 0, 0, op);
        }
        *value = prog->symbols[s].addr;
        return 0;
    }
    return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, op);
}

/*
 * read_memory: "[REG]", "[REG+n]" or "[REG-n]" -> register and offset.
 * Spaces inside the brackets are allowed.
 */
static int read_memory(const char *op, int line_no, int *reg, int *off, SimError *err)
{
    char t[NAME_LEN];
    char reg_text[NAME_LEN];
    int i, n = 0, sign_at = -1;

    for (i = 0; op[i] != '\0'; i++) {           /* copy without spaces */
        if (!is_space(op[i])) {
            t[n] = op[i];
            n++;
        }
    }
    t[n] = '\0';
    if (n < 3 || t[0] != '[' || t[n - 1] != ']') {
        return asm_error(err, ERR_BAD_OPERAND, line_no, 0, 0, op);
    }
    t[n - 1] = '\0';                            /* drop ']' */

    for (i = 1; t[i] != '\0'; i++) {            /* find '+' or '-' */
        if (t[i] == '+' || t[i] == '-') {
            sign_at = i;
            break;
        }
    }
    if (sign_at < 0) {                          /* [REG] */
        *off = 0;
        copy_range(reg_text, t, 1, n - 1);
    } else {                                    /* [REG+n] or [REG-n] */
        copy_range(reg_text, t, 1, sign_at);
        /* '+n' -> parse "n";  '-n' -> parse "-n" */
        if (parse_number(t + sign_at + (t[sign_at] == '+' ? 1 : 0), off) != 0) {
            return asm_error(err, ERR_BAD_NUMBER, line_no, 0, 0, op);
        }
    }
    if (read_register(reg_text, line_no, reg, err) != 0) {
        return -1;
    }
    if (*off < -16 || *off > 15) {
        return asm_error(err, ERR_OFFSET_RANGE, line_no, *off, 0, "");
    }
    return 0;
}

/*
 * encode: one instruction line -> one 16-bit word (pass 2).
 */
static int encode(const Line *ln, int line_no, const Program *prog, unsigned short *word,
                  SimError *err)
{
    int rd = 0, rs = 0, off = 0, imm = 0;

    switch (FORMATS[ln->opcode]) {
    case FMT_N:                                 /* HALT */
        break;
    case FMT_R:                                 /* ADD rd, rs */
        if (read_register(ln->ops[0], line_no, &rd, err) != 0
            || read_register(ln->ops[1], line_no, &rs, err) != 0) {
            return -1;
        }
        break;
    case FMT_I:                                 /* LDI rd, imm8 */
        if (read_register(ln->ops[0], line_no, &rd, err) != 0
            || read_value(ln->ops[1], line_no, prog, &imm, err) != 0) {
            return -1;
        }
        break;
    case FMT_M:                                 /* LDR rd, [rs+off5] */
        if (read_register(ln->ops[0], line_no, &rd, err) != 0
            || read_memory(ln->ops[1], line_no, &rs, &off, err) != 0) {
            return -1;
        }
        break;
    case FMT_J:                                 /* JMP addr8 */
        if (read_value(ln->ops[0], line_no, prog, &imm, err) != 0) {
            return -1;
        }
        break;
    case FMT_S:                                 /* PUSH rd */
        if (read_register(ln->ops[0], line_no, &rd, err) != 0) {
            return -1;
        }
        break;
    }
    if (imm < 0 || imm > 255) {
        return asm_error(err, ERR_IMM_RANGE, line_no, imm, 0, "");
    }

    /* opcode | rd | rs | off5   (imm8 shares bits 7..0; unused fields are 0) */
    *word = (unsigned short)((ln->opcode << OP_SHIFT) | (rd << RD_SHIFT) | (rs << RS_SHIFT)
                             | (off & OFF5_MASK) | imm);
    return 0;
}

/*
 * is_reserved: may this name NOT be used as a label? (mnemonic or register)
 */
static int is_reserved(const char *name)
{
    return find_opcode(name) >= 0 || find_register(name) >= 0;
}

/* ---- The assembler ----------------------------------------------------- */

/*
 * assemble: source lines -> Program (machine code, data, labels).
 *   lines   : the program, one string per line (line 1 = lines[0])
 * Returns 0, or -1 on error (err has the line number and message).
 */
int assemble(const char *const *lines, int n_lines, Program *prog, SimError *err)
{
    Line ln;
    int i, s, value, in_data = 0;
    int text_count = 0, data_count = 0;
    unsigned short word;

    prog->text_len = 0;
    prog->data_len = 0;
    prog->n_symbols = 0;
    prog->entry = TEXT_BASE;
    for (i = 0; i < TEXT_SIZE; i++) {
        prog->text[i] = 0;
        prog->source_line[i] = 0;
    }
    for (i = 0; i < DATA_SIZE; i++) {
        prog->data[i] = 0;
    }

    /* PASS 1: give every label its address */
    for (i = 0; i < n_lines; i++) {
        if (parse_line(lines[i], i + 1, &ln, err) != 0) {
            return -1;
        }
        if (ln.label[0] != '\0') {
            if (is_reserved(ln.label)) {
                return asm_error(err, ERR_RESERVED_LABEL, i + 1, 0, 0, ln.label);
            }
            if (find_symbol(prog, ln.label) >= 0) {
                return asm_error(err, ERR_DUPLICATE_LABEL, i + 1, 0, 0, ln.label);
            }
            if (prog->n_symbols == MAX_SYMBOLS) {
                return asm_error(err, ERR_TOO_MANY_LABELS, i + 1, 0, 0, "");
            }
            /* label = address of the next instruction (.text) or .word (.data) */
            text_copy(prog->symbols[prog->n_symbols].name, ln.label, NAME_LEN);
            prog->symbols[prog->n_symbols].addr =
                in_data ? DATA_BASE + data_count : TEXT_BASE + text_count;
            prog->n_symbols++;
        }
        if (ln.kind == LINE_TEXT) {
            in_data = 0;
        } else if (ln.kind == LINE_DATA) {
            in_data = 1;
        } else if (ln.kind == LINE_WORD) {
            if (!in_data) {
                return asm_error(err, ERR_WORD_IN_TEXT, i + 1, 0, 0, "");
            }
            data_count++;
        } else if (ln.kind == LINE_INSTR) {
            if (in_data) {
                return asm_error(err, ERR_INSTR_IN_DATA, i + 1, 0, 0, "");
            }
            text_count++;
        }
    }
    if (text_count > TEXT_SIZE) {
        return asm_error(err, ERR_ASM_TEXT_TOO_LARGE, 0, text_count, 0, "");
    }
    if (data_count > DATA_SIZE) {
        return asm_error(err, ERR_ASM_DATA_TOO_LARGE, 0, data_count, 0, "");
    }

    /* PASS 2: encode instructions and data words */
    for (i = 0; i < n_lines; i++) {
        parse_line(lines[i], i + 1, &ln, err);  /* same result as pass 1 */
        if (ln.kind == LINE_INSTR) {
            if (encode(&ln, i + 1, prog, &word, err) != 0) {
                return -1;
            }
            prog->text[prog->text_len] = word;
            prog->source_line[prog->text_len] = i + 1;
            prog->text_len++;
        } else if (ln.kind == LINE_WORD) {
            if (read_value(ln.ops[0], i + 1, prog, &value, err) != 0) {
                return -1;
            }
            if (value < -32768 || value > 65535) {
                return asm_error(err, ERR_WORD_RANGE, i + 1, value, 0, "");
            }
            prog->data[prog->data_len] = (unsigned short)(value & WORD_MASK);
            prog->data_len++;
        }
    }

    /* Entry point: label main, else address 0 */
    s = find_symbol(prog, "main");
    if (s >= 0) {
        prog->entry = prog->symbols[s].addr;
    }
    return 0;
}

/* ---- The disassembler -------------------------------------------------- */

/*
 * put_text / put_hex / put_int: add text to out at position pos.
 * Each returns the new position. (Our own sprintf, which is not allowed.)
 */
static int put_text(char *out, int pos, const char *s)
{
    int i;
    for (i = 0; s[i] != '\0' && pos < DISASM_LEN - 1; i++) {
        out[pos] = s[i];
        pos++;
    }
    out[pos] = '\0';
    return pos;
}

static int put_hex(char *out, int pos, int value, int digits)
{
    static const char HEX[] = "0123456789ABCDEF";
    char tmp[8];
    int i;
    for (i = digits - 1; i >= 0; i--) {         /* lowest digit last */
        tmp[i] = HEX[value & 0xF];
        value >>= 4;
    }
    tmp[digits] = '\0';
    return put_text(out, pos, tmp);
}

static int put_int(char *out, int pos, int value)
{
    char tmp[12];
    int n = 0, i;
    char digits[12];
    if (value < 0) {
        pos = put_text(out, pos, "-");
        value = -value;
    }
    do {                                        /* digits come out reversed */
        digits[n] = (char)('0' + value % 10);
        n++;
        value /= 10;
    } while (value > 0);
    for (i = 0; i < n; i++) {
        tmp[i] = digits[n - 1 - i];
    }
    tmp[n] = '\0';
    return put_text(out, pos, tmp);
}

/*
 * put_address: a label name if one fits the rule (SPEC 3.10), else "0x%02X".
 *   jump : 1 = jump target (TEXT labels only), 0 = LD/ST address (>= 0x60)
 */
static int put_address(char *out, int pos, int addr, int jump, const Program *prog)
{
    int i, a;
    if (prog != 0) {
        for (i = 0; i < prog->n_symbols; i++) {         /* first match wins */
            a = prog->symbols[i].addr;
            if (a == addr && (jump ? a < TEXT_END : a >= DATA_BASE)) {
                return put_text(out, pos, prog->symbols[i].name);
            }
        }
    }
    pos = put_text(out, pos, "0x");
    return put_hex(out, pos, addr, 2);
}

/*
 * disassemble: one word -> canonical assembly text (SPEC 3.10).
 *   prog : labels to show instead of addresses, or 0 for plain hex
 *   out  : at least DISASM_LEN chars
 * Examples: "LDR R1, [FP+2]", "JNZ loop", ".word 0xC000  ; invalid"
 */
void disassemble(unsigned short word, const Program *prog, char *out)
{
    int opcode = (word >> OP_SHIFT) & OP_MASK;
    int rd = (word >> RD_SHIFT) & REG_MASK;
    int rs = (word >> RS_SHIFT) & REG_MASK;
    int off = word & OFF5_MASK;
    int imm = word & IMM8_MASK;
    int pos;

    if (off & 0x10) {
        off -= 32;                              /* sign-extend */
    }
    out[0] = '\0';
    if (opcode >= NUM_OPCODES) {                /* reserved opcode */
        pos = put_text(out, 0, ".word 0x");
        pos = put_hex(out, pos, word, 4);
        put_text(out, pos, "  ; invalid");
        return;
    }

    pos = put_text(out, 0, MNEMONICS[opcode]);
    switch (FORMATS[opcode]) {
    case FMT_N:                                 /* HALT */
        break;
    case FMT_R:                                 /* ADD R1, R2 */
        pos = put_text(out, pos, " ");
        pos = put_text(out, pos, REG_NAMES[rd]);
        pos = put_text(out, pos, ", ");
        put_text(out, pos, REG_NAMES[rs]);
        break;
    case FMT_I:                                 /* LDI R0, 1  /  LD R1, n */
        pos = put_text(out, pos, " ");
        pos = put_text(out, pos, REG_NAMES[rd]);
        pos = put_text(out, pos, ", ");
        if (opcode == OP_LD || opcode == OP_ST) {
            put_address(out, pos, imm, 0, prog);
        } else {
            put_int(out, pos, imm);
        }
        break;
    case FMT_M:                                 /* LDR R1, [FP+2] (sign always shown) */
        pos = put_text(out, pos, " ");
        pos = put_text(out, pos, REG_NAMES[rd]);
        pos = put_text(out, pos, ", [");
        pos = put_text(out, pos, REG_NAMES[rs]);
        pos = put_text(out, pos, off < 0 ? "" : "+");
        pos = put_int(out, pos, off);
        put_text(out, pos, "]");
        break;
    case FMT_J:                                 /* JNZ loop */
        pos = put_text(out, pos, " ");
        put_address(out, pos, imm, 1, prog);
        break;
    case FMT_S:                                 /* PUSH R1 */
        pos = put_text(out, pos, " ");
        put_text(out, pos, REG_NAMES[rd]);
        break;
    }
}
