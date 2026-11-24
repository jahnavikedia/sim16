# ISA Summary (Assignment 1, page 5)

16-bit words · 256-word memory, word-addressed · 8 registers · Z/N flags · stack grows down.

| Register | Name | Role |
|---|---|---|
| R0 | `R0` | Return value; general purpose |
| R1–R5 | `R1`–`R5` | General purpose (caller-saved) |
| R6 | `FP` | Frame pointer: base of current stack frame |
| R7 | `SP` | Stack pointer: last pushed item (empty = 0x100) |
| — | `PC` | Address of next instruction (not addressable) |
| — | `IR` | Last fetched instruction word |
| — | `Z`, `N` | Zero flag, Negative flag (bit 15 of result) |

*Table 2: Registers*

```
 Address    Segment   Size       Contents
+--------+                                         low addresses
| 0x00   |  TEXT     96 words   machine code
|  ...   |
| 0x5F   |
+--------+
| 0x60   |  DATA     32 words   global variables
|  ...   |
| 0x7F   |
+--------+
| 0x80   |  HEAP     32 words   dynamic memory (unused)
|  ...   |
| 0x9F   |
+--------+
| 0xA0   |  STACK    96 words   call stack, grows DOWN
|  ...   |                      SP = FP = 0x100 when empty
| 0xFF   |
+--------+                                         high addresses
```

*Figure 2: Memory map*

| Format | Fields | Assembly shape | Instructions |
|---|---|---|---|
| N | opcode | `MNEM` | HALT, NOP, RET |
| R | opcode, rd, rs | `MNEM rd, rs` | MOV, ADD, SUB, MUL, CMP |
| I | opcode, rd, imm8 | `MNEM rd, imm8` | LDI, LD, ST, ADDI, SUBI, CMPI |
| M | opcode, rd, rs, off5 | `MNEM rd, [rs+off5]` | LDR, STR |
| J | opcode, addr8 | `MNEM addr8` | JMP, JZ, JNZ, JN, CALL |
| S | opcode, rd | `MNEM rd` | PUSH, POP, OUT |

*Table 3: Instruction formats*
