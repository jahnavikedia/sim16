# SIM16: Assignment 2, Program Layout & Execution

A recursive factorial in C, translated to our SIM16 assembly, loaded into the four memory segments (text, data, heap, stack) and shown after every recursive call.

## Requirements

| Need | Notes |
|---|---|
| A C compiler (`gcc` or `clang`) | Nothing else to install |
| `make` (optional) | Without `make`, use the `cc` commands below |
| Windows | Use WSL or MinGW |

## Compile

```
make
```

Without `make`:

```
cc -std=c99 -Wall -o sim16 src/main.c src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c src/printer.c
cc -std=c99 -Wall -o test_cpu tests/test_cpu.c src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c src/printer.c
```

## Run

```
make
./sim16
```

**You will see:** the program loaded into memory (4 segments, code decoded), then the 4 segments after **each** of the 5 recursive calls fact(5) … fact(1), then the final state. It ends with `Output: 120`.

| Command | Shows | Expected result |
|---|---|---|
| `./sim16 recursive` | Load view, 4 segments after every recursive call, final view | 5 calls, 52 steps, `Output: 120` |
| `./sim16 list recursive` | Assembly listing: address, machine code, source line | 19 instructions, 2 data words |
| `./test_cpu` | Unit tests (every instruction, every error) + golden runs | `0 failed` |
| `./sim16 help` | All commands | |
| `./sim16 > run.txt` | Save the full output to a file | |

| In the output | Meaning |
|---|---|
| `[TEXT]` | Machine code, decoded; `<- PC` = next instruction |
| `[DATA]` | Globals `n` and `result` |
| `[HEAP]` | Dynamic memory (unused: empty) |
| `[STACK]` | From SP up to 0xFF; each word labelled `saved FP`, `return addr`, `arg` |
| `+-- fact(4)` … `+--` | One stack frame (3 words); `<- SP, FP` marks the newest |
| `*` after a value | Changed since the previous view |

## Files

| File | Purpose |
|---|---|
| `README.md` | This file |
| `Makefile` | `make`, `make run`, `make test`, `make clean` |
| `src/assignment.h` | One line: this copy is Assignment 2 |
| `src/isa.h` | Instruction set: memory map, registers, opcodes, formats |
| `src/cpu.h`, `src/cpu.c` | CPU: registers, fetch / decode / execute, all 24 instructions, stack, CALL/RET |
| `src/assembler.h`, `src/assembler.c` | Assembler (two passes) and disassembler |
| `src/loader.h`, `src/loader.c` | Loads a program into the 4 memory segments |
| `src/printer.h`, `src/printer.c` | Every printed view (registers, segments, stack frames) |
| `src/programs.h`, `src/programs.c` | The assembly program (recursive factorial) |
| `src/text.h`, `src/text.c` | Own string helpers (only `printf` / `putchar` allowed) |
| `src/main.c` | Command line |
| `tests/test_cpu.c` | Unit tests and golden runs |
| `programs/fact_recursive.c` | Recursive factorial in C (translated by hand to the `recursive` assembly) |
| `output/recursive_calls.txt` | Saved output of `./sim16 recursive` |

## Where things are

| What | Where |
|---|---|
| Assembly program (recursive factorial) | `src/programs.c` (or `./sim16 list`) |
| C source | `programs/fact_recursive.c` |
| Saved output | `output/` |
