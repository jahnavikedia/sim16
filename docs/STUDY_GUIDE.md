# SIM16 Study Guide (team only, not in the zips)

The professor may ask anyone about any part. Know **your own files** in depth (your section), and the **"Everyone must know"** section for the rest.

| Member | Must explain |
|---|---|
| Jahnavi | `src/cpu.c`, `src/cpu.h`, `tests/test_cpu.c` |
| Shrutthi | `src/assembler.c`, `src/text.c`, `src/main.c`, `Makefile`, `src/programs.c` (factorial + recursive), `programs/fact_recursive.c` |
| Snehil | `src/loader.c`, `src/printer.c`, `programs/power.c` + its assembly in `src/programs.c` |
| Everyone | `src/isa.h`, `src/assignment.h`, memory map, instruction format, calling convention |

---

## Everyone must know

### The machine (`src/isa.h`)

| Item | Value |
|---|---|
| Word | 16 bits, every result `& 0xFFFF` |
| Memory | 256 words, word-addressed, 0x00–0xFF |
| Registers | R0–R5, FP = R6, SP = R7; plus PC, IR, flags Z and N |
| R0 | Return value of a function |
| Flags | Only ADD SUB MUL ADDI SUBI CMP CMPI set them. Z = result 0, N = bit 15 |
| Stack | Full-descending: SP points at the last pushed word; empty = 0x100 |

| Segment | Addresses | Holds |
|---|---|---|
| TEXT | 0x00–0x5F (96) | machine code |
| DATA | 0x60–0x7F (32) | globals (`n`, `result`) |
| HEAP | 0x80–0x9F (32) | unused, shown empty |
| STACK | 0xA0–0xFF (96) | frames, grows down from 0xFF |

### Instruction word

```
 15      11 10    8 7     5 4      0
+----------+-------+-------+--------+
|  opcode  |  rd   |  rs   |  off5  |   R, M, S, N formats
+----------+-------+-------+--------+
|  opcode  |  rd   |      imm8      |   I, J formats
```

| Format | Shape | Instructions |
|---|---|---|
| N | `HALT` | HALT, NOP, RET |
| R | `ADD rd, rs` | MOV ADD SUB MUL CMP |
| I | `LDI rd, imm8` | LDI LD ST ADDI SUBI CMPI |
| M | `LDR rd, [rs+off5]` | LDR STR (off5 = −16…+15) |
| J | `JMP addr8` | JMP JZ JNZ JN CALL |
| S | `PUSH rd` | PUSH POP OUT |

Opcodes 0x00–0x17 = 24 instructions; 0x18–0x1F = "invalid opcode".

### Calling convention

| Who | Does |
|---|---|
| Caller | `PUSH arg` → `CALL f` → `ADDI SP, 1` (removes its argument) |
| CALL (hardware) | push return address, push FP, FP ← SP, PC ← f |
| Callee | argument at `[FP+2]`; result in R0; ends with `RET` |
| RET (hardware) | SP ← FP, FP ← pop, PC ← pop |

```
 FP+2  argument        <- pushed by caller
 FP+1  return address  <- pushed by CALL
 FP+0  saved FP        <- pushed by CALL, FP points here
```

### `assignment.h`
One line: `#define ASSIGNMENT 0 | 1 | 2`. `programs.c` uses `#if` to include only that assignment's programs; `./sim16` runs them. Repo = 0 (all), A1 zip = 1, A2 zip = 2. CPU, assembler, loader, printer are identical in all builds.

### Why snapshots are struct copies
`CPU snap = cpu;` C copies every field of a struct, **including arrays inside it** (mem[256], regs[8]). So `snap` is a full, independent picture: later steps change `cpu`, never `snap`. The printer compares `prev` and `snap` to put `*` on changed values.

### Flow of one run
`programs.c` (source lines) → `assemble()` → `Program` → `load_program()` → `CPU` → `cpu_step()` loop in `main.c` → `print_step()` / `print_segments()`.

---

## Jahnavi: `cpu.c`, `cpu.h`, `tests/test_cpu.c`

### `cpu.c`

| Function | Does |
|---|---|
| `cpu_reset` | memory and R0–R5 = 0, FP = SP = 0x100, PC = 0, exec_addr = −1 |
| `cpu_step` | one instruction: checks → fetch → decode → `switch` execute → record |
| `cpu_run` | `cpu_step` until HALT, error after `max_steps` |
| `set_flags` | Z = (v == 0), N = bit 15 of the 16-bit result |
| `word16` | `x & 0xFFFF` (wrap-around) |

**One instruction, real numbers** (factorial step 3, `MUL R0, R1` at 0x02):

| Stage | What happens |
|---|---|
| Check | not halted, PC = 0x02 is in range |
| Fetch | word = M[0x02] = 0x5020; next_pc = 0x03 |
| Decode | 0x5020 = `01010 000 001 00000` → opcode 0x0A (MUL), rd = 0 (R0), rs = 1 (R1) |
| Execute | R0 = (1 × 8) & 0xFFFF = 8; Z = 0, N = 0 |
| Record | exec_addr = 0x02, IR = 0x5020, PC = 0x03, steps = 3 |

**CALL / RET, fact(5) → fact(4)** (CALL at 0x0C, step 9):

| Before CALL | CALL does | After |
|---|---|---|
| SP = 0xFC (arg 4 just pushed), FP = 0xFD, next_pc = 0x0D | M[0xFB] = 0x000D (return), M[0xFA] = 0x00FD (old FP), SP = FP = 0xFA, PC = 0x07 | frame FA:00FD FB:000D FC:0004 |

| RET of fact(4) | Result |
|---|---|
| SP ← FP (0xFA); FP ← M[0xFA] = 0xFD; PC ← M[0xFB] = 0x0D; SP = 0xFC | back in fact(5) at 0x0D (`ADDI SP, 1` then removes the argument) |

**Error checks happen before any change** (so a failed step leaves the CPU exactly as it was):

| Check | Error |
|---|---|
| PUSH: SP − 1 < 0xA0 | stack overflow (heap never written) |
| CALL: SP − 2 < 0xA0 | stack overflow |
| POP: SP ≥ 0x100 | stack underflow |
| RET: FP + 1 ≥ 0x100 | stack underflow |
| LDR/STR: rs + off5 outside 0x00–0xFF | memory address out of range |
| opcode 0x18–0x1F | invalid opcode |

| Likely question | Short answer |
|---|---|
| Why is PC incremented before execute? | So CALL pushes the address of the **next** instruction; jumps just overwrite it |
| How is a negative offset decoded? | off5 is 5 bits; if bit 4 is set, subtract 32 (0x1F → −1) |
| How does MUL avoid overflow in C? | Multiply as `long`, keep `& 0xFFFF` |
| Why does 8! set N = 1? | 40320 = 0x9D80, bit 15 is 1. Values are shown unsigned |
| Why no callback in `cpu_run`? | No function pointers allowed; `main.c` loops over `cpu_step` itself |

### `tests/test_cpu.c`

| Part | Proves |
|---|---|
| `check(cond, msg)` | tiny framework: counts passes/fails, prints failures, exit 1 if any fail |
| One test per instruction | result + flags of all 24 instructions |
| Error tests | each SPEC 3.8 CPU/loader error, and that a failed step changes nothing (`same_cpu`) |
| Assembler tests | SPEC 9.4 rows both ways, syntax rules, every assembler error |
| Golden runs | 8!: 29 steps, 40320 · 8²: 20 steps, 64 · recursive: 52 steps, 120, stack after each of the 5 CALLs |

| Likely question | Short answer |
|---|---|
| How do you test "nothing changed"? | Copy the CPU struct before the step, compare every field after |
| Why raw hex tests? | They test the CPU without depending on the assembler |
| Why do counts differ per build (265 / 221 / 242)? | Tests of programs not in the build are left out with `#if` |

---

## Shrutthi: `assembler.c`, `text.c`, `main.c`, `Makefile`, programs

### `assembler.c` (two passes)

| Pass | Does | Factorial example |
|---|---|---|
| 1 | read every line, give each label its address | main = 0x00, loop = 0x02, n = 0x60, result = 0x61 |
| 2 | read again, encode instructions and `.word` | `JNZ loop` → (0x11 << 11) \| 0x02 = 0x8802 |

Two passes are needed for **forward references** (`JMP done` before `done:`).

| Function | Does |
|---|---|
| `parse_line` | splits a line into label / mnemonic / operands; drops the `;` comment |
| `encode` | operands → `opcode<<11 \| rd<<8 \| rs<<5 \| off5` or `\| imm8` |
| `read_memory` | `[FP+2]` → register 6, offset +2 |
| `disassemble` | word → text, e.g. `0x29C2` → `LDR R1, [FP+2]`; labels for jump targets (TEXT) and LD/ST addresses (DATA) |

| Likely question | Short answer |
|---|---|
| How are errors reported? | Fill a `SimError` (kind, line, value, name), return −1; `main` prints `ERROR: line N: ...` |
| Why can't a label be `add` or `sp`? | It would be ambiguous with a mnemonic or register |
| Why `LD R1, 0x00` not `LD R1, main`? | LD/ST only use DATA labels (≥ 0x60), jumps only TEXT labels |
| How is `-1` stored by `.word`? | `& 0xFFFF` → 0xFFFF (two's complement) |

### `text.c`
Own `text_equal`, `text_equal_ignore_case`, `text_length`, `text_copy`, `text_append`, `text_append_int`, `parse_number`, `is_digit`… because only `printf`/`putchar` are allowed.

| Likely question | Short answer |
|---|---|
| How does `parse_number` read `0x1F`? | sees `0x` → base 16, then `result = result * 16 + digit` |
| Why not `int(s, 0)`-style parsing? | `08` must be decimal 8, not an octal error |

### `main.c`, `Makefile`, programs

| Item | Key point |
|---|---|
| `./sim16` | runs this build's programs in default modes (trace / calls) |
| Modes | trace: step block after every instruction · calls: 4 segments after every CALL · final: start + end |
| Errors | returned up to `main`, which prints `ERROR:` and returns 1 (no `exit()`) |
| `make submit` | copies files, sets `ASSIGNMENT` 1 / 2 with `sed`, removes repo-only Makefile part, zips |
| `fact_recursive.c` → assembly | `if (k <= 1)` → `CMPI R1, 2` + `JN base` (k − 2 < 0 ⇔ k ≤ 1) |

| Likely question | Short answer |
|---|---|
| Why are programs strings in C? | The simulator may not read files (`fopen` not allowed) |
| Why reload `[FP+2]` after the recursive CALL? | R1 is caller-saved; the call overwrote it |
| Who removes the argument? | The caller: `ADDI SP, 1` after CALL |

---

## Snehil: `loader.c`, `printer.c`, power program

### `loader.c`

| Step | Does |
|---|---|
| 1 | check sizes (text ≤ 96, data ≤ 32) and entry inside text |
| 2 | `cpu_reset`: all memory 0, SP = FP = 0x100 |
| 3 | copy text → 0x00…, data → 0x60…; HEAP and STACK stay 0 |
| 4 | PC = entry (address of `main`) |

Recursive program: code 0x00–0x12, `n` = 5 at 0x60, `result` at 0x61, heap empty, stack empty.

### `printer.c`

| Function | Prints |
|---|---|
| `print_step` | `STEP n @addr word instr`, registers, memory grid (Assignment 1) |
| `print_memory_grid` | rows of 8 words: TEXT/DATA rows in use, HEAP 0x80, STACK F0, F8 |
| `print_segments` | title + 4 segments: TEXT decoded, DATA names, HEAP, STACK (Assignment 2) |
| `print_stack` | SP…0xFF with `saved FP` / `return addr` / `arg` / `local` and frame brackets |
| `walk_frames` | follows saved FPs from FP up to 0x100; stops if FP doesn't increase (safety) |
| `print_error` | the exact SPEC 3.8 messages |

| Likely question | Short answer |
|---|---|
| How do you know what changed? | compare `prev` snapshot with current: different → `*` |
| How is the frame's name found? | the word before the return address is the CALL; its addr8 is the target, the label there is `fact` |
| Why no trailing spaces? | the last column is printed without padding |
| Why only some memory rows? | small block that holds code, data, heap start and stack top, same rows every step |

### power program (8²)

| C (`power.c`) | Assembly |
|---|---|
| `r = 1; i = 0;` | `LDI R0, 1` / `LDI R1, 0` |
| `i < exponent` | `CMP R1, R2` / `JN body` / `JMP done` |
| `r = r * base; i++;` | `MUL R0, R3` / `ADDI R1, 1` / `JMP loop` |
| `result = r; printf` | `ST R0, result` / `OUT R0` |

Register mapping: r → R0, i → R1, exponent → R2, base → R3. Result: 20 steps, 64.
