"""isa.py -- SHARED CONTRACT for the SIM16 CPU. Written once at kickoff.
Do NOT change anything here without telling all three team members."""

# ---- Word / memory --------------------------------------------------------
WORD_BITS = 16
WORD_MASK = 0xFFFF          # every register and memory cell is 16 bits
MEM_SIZE  = 256             # word-addressable: addresses 0x00 .. 0xFF

# ---- Memory map (start inclusive, end exclusive) ---------------------------
TEXT_BASE,  TEXT_END  = 0x00, 0x60   # 96 words : machine code
DATA_BASE,  DATA_END  = 0x60, 0x80   # 32 words : globals (.data)
HEAP_BASE,  HEAP_END  = 0x80, 0xA0   # 32 words : dynamic memory (unused, shown empty)
STACK_BASE, STACK_END = 0xA0, 0x100  # 96 words : call stack, grows DOWN from 0xFF
STACK_TOP = 0x100                    # initial SP and FP (= empty stack)

SEGMENTS = [                          # in display order
    ("TEXT",  TEXT_BASE,  TEXT_END),
    ("DATA",  DATA_BASE,  DATA_END),
    ("HEAP",  HEAP_BASE,  HEAP_END),
    ("STACK", STACK_BASE, STACK_END),
]

def segment_of(addr):
    """Return 'TEXT' | 'DATA' | 'HEAP' | 'STACK' for a valid address."""
    for name, lo, hi in SEGMENTS:
        if lo <= addr < hi:
            return name
    raise ValueError("address out of range: 0x%X" % addr)

# ---- Registers ----------------------------------------------------------------
NUM_REGS  = 8
REG_FP    = 6               # R6 is the frame pointer
REG_SP    = 7               # R7 is the stack pointer
REG_NAMES = ["R0", "R1", "R2", "R3", "R4", "R5", "FP", "SP"]   # display names
REG_LOOKUP = {              # names accepted by the assembler (upper-cased)
    "R0": 0, "R1": 1, "R2": 2, "R3": 3, "R4": 4, "R5": 5,
    "R6": 6, "R7": 7, "FP": 6, "SP": 7,
}

# ---- Instruction word layout ---------------------------------------------------
#  15      11 10   8 7    5 4      0
# +----------+------+------+--------+
# |  opcode  |  rd  |  rs  |  off5  |   (fields not used by a format must be 0)
# +----------+------+------+--------+
#                    |      imm8     |  (I and J formats use bits 7..0)
OP_SHIFT, RD_SHIFT, RS_SHIFT = 11, 8, 5
OP_MASK, REG_MASK, IMM8_MASK, OFF5_MASK = 0x1F, 0x7, 0xFF, 0x1F

# ---- Formats -------------------------------------------------------------------
# N : no operands                 HALT
# R : rd, rs                      ADD R1, R2
# I : rd, imm8 (value/address)    LDI R1, 5   /  LD R1, n
# M : rd, [rs + off5]             LDR R1, [FP+2]   (off5 signed -16..+15)
# J : addr8                       JMP loop
# S : rd only                     PUSH R1

# mnemonic -> (opcode, format)
OPCODES = {
    "HALT": (0x00, "N"),
    "NOP":  (0x01, "N"),
    "LDI":  (0x02, "I"),
    "LD":   (0x03, "I"),
    "ST":   (0x04, "I"),
    "LDR":  (0x05, "M"),
    "STR":  (0x06, "M"),
    "MOV":  (0x07, "R"),
    "ADD":  (0x08, "R"),
    "SUB":  (0x09, "R"),
    "MUL":  (0x0A, "R"),
    "ADDI": (0x0B, "I"),
    "SUBI": (0x0C, "I"),
    "CMP":  (0x0D, "R"),
    "CMPI": (0x0E, "I"),
    "JMP":  (0x0F, "J"),
    "JZ":   (0x10, "J"),
    "JNZ":  (0x11, "J"),
    "JN":   (0x12, "J"),
    "PUSH": (0x13, "S"),
    "POP":  (0x14, "S"),
    "CALL": (0x15, "J"),
    "RET":  (0x16, "N"),
    "OUT":  (0x17, "S"),
}                       # opcodes 0x18..0x1F are reserved -> "invalid opcode" error
OPCODE_INFO = {op: (name, fmt) for name, (op, fmt) in OPCODES.items()}

# Instructions that update the Z and N flags (from the 16-bit result)
SETS_FLAGS = {"ADD", "SUB", "MUL", "ADDI", "SUBI", "CMP", "CMPI"}

DEFAULT_MAX_STEPS = 10000

# ---- Errors (every module raises one of these; main.py catches SimError) -------
class SimError(Exception):
    """Base class for all simulator errors."""

class AssemblerError(SimError):
    def __init__(self, line, message):
        super().__init__("line %d: %s" % (line, message))
        self.line = line

class LoaderError(SimError):
    pass

class CPUError(SimError):
    pass
