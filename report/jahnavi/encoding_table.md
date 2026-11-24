# Instruction Encoding (Assignment 1, page 4)

Every instruction is one 16-bit word. Unused fields are 0.

```
 bit: 15 14 13 12 11 | 10  9  8 |  7  6  5 |  4  3  2  1  0
     +---------------+----------+----------+----------------+
     |    opcode     |    rd    |    rs    |      off5      |   R, M, S, N formats
     |    (5 bits)   | (3 bits) | (3 bits) | (5 bits signed)|
     +---------------+----------+----------+----------------+
     |    opcode     |    rd    |       imm8 / addr8        |   I, J formats
     |    (5 bits)   | (3 bits) |         (8 bits)          |
     +---------------+----------+---------------------------+
```

*Figure: SIM16 instruction word layout*

| Op | Mnemonic | Fmt | Syntax | Meaning | Flags | Example | Hex |
|---|---|---|---|---|---|---|---|
| 0x00 | HALT | N | `HALT` | stop execution | – | `HALT` | `0000` |
| 0x01 | NOP | N | `NOP` | do nothing | – | `NOP` | `0800` |
| 0x02 | LDI | I | `LDI rd, imm8` | rd ← imm8 | – | `LDI R0, 1` | `1001` |
| 0x03 | LD | I | `LD rd, addr8` | rd ← M[addr8] | – | `LD R1, 0x60` | `1960` |
| 0x04 | ST | I | `ST rd, addr8` | M[addr8] ← rd | – | `ST R0, 0x61` | `2061` |
| 0x05 | LDR | M | `LDR rd, [rs+off5]` | rd ← M[rs + off5] | – | `LDR R1, [FP+2]` | `29C2` |
| 0x06 | STR | M | `STR rd, [rs+off5]` | M[rs + off5] ← rd | – | `STR R2, [SP-1]` | `32FF` |
| 0x07 | MOV | R | `MOV rd, rs` | rd ← rs | – | `MOV R1, R2` | `3940` |
| 0x08 | ADD | R | `ADD rd, rs` | rd ← rd + rs | Z,N | `ADD R1, R2` | `4140` |
| 0x09 | SUB | R | `SUB rd, rs` | rd ← rd − rs | Z,N | `SUB R1, R2` | `4940` |
| 0x0A | MUL | R | `MUL rd, rs` | rd ← rd × rs | Z,N | `MUL R0, R1` | `5020` |
| 0x0B | ADDI | I | `ADDI rd, imm8` | rd ← rd + imm8 | Z,N | `ADDI SP, 1` | `5F01` |
| 0x0C | SUBI | I | `SUBI rd, imm8` | rd ← rd − imm8 | Z,N | `SUBI R1, 1` | `6101` |
| 0x0D | CMP | R | `CMP rd, rs` | flags ← rd − rs | Z,N | `CMP R1, R2` | `6940` |
| 0x0E | CMPI | I | `CMPI rd, imm8` | flags ← rd − imm8 | Z,N | `CMPI R1, 2` | `7102` |
| 0x0F | JMP | J | `JMP addr8` | PC ← addr8 | – | `JMP 0x04` | `7804` |
| 0x10 | JZ | J | `JZ addr8` | if Z: PC ← addr8 | – | `JZ 0x10` | `8010` |
| 0x11 | JNZ | J | `JNZ addr8` | if not Z: PC ← addr8 | – | `JNZ 0x02` | `8802` |
| 0x12 | JN | J | `JN addr8` | if N: PC ← addr8 | – | `JN 0x11` | `9011` |
| 0x13 | PUSH | S | `PUSH rd` | SP ← SP−1; M[SP] ← rd | – | `PUSH R1` | `9900` |
| 0x14 | POP | S | `POP rd` | rd ← M[SP]; SP ← SP+1 | – | `POP R2` | `A200` |
| 0x15 | CALL | J | `CALL addr8` | push PC; push FP; FP ← SP; PC ← addr8 | – | `CALL 0x07` | `A807` |
| 0x16 | RET | N | `RET` | SP ← FP; FP ← pop; PC ← pop | – | `RET` | `B000` |
| 0x17 | OUT | S | `OUT rd` | print rd | – | `OUT R0` | `B800` |
| 0x18–0x1F | reserved | – | – | invalid opcode error | – | – | – |

*Table 1: SIM16 instruction encoding*

All results are 16-bit (`& 0xFFFF`). Z = result is 0; N = bit 15 of result.
