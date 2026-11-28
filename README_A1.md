# SIM16: Assignment 1, CPU Design

Our own 16-bit CPU (SIM16), running 8! written in assembly and 8² written in C and translated to assembly, with memory shown after every instruction.

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

**You will see:** 8! (factorial) then 8² (power), each loaded into memory and then shown after **every** instruction. They end with `Output: 40320` and `Output: 64`.

| Command | Shows | Expected result |
|---|---|---|
| `./sim16 factorial` | 8! in assembly, memory after every instruction | 29 steps, `Output: 40320` |
| `./sim16 power` | 8² (C → assembly), memory after every instruction | 20 steps, `Output: 64` |
| `./sim16 list factorial` | Assembly listing: address, machine code, source line | 8 instructions, 2 data words |
| `./sim16 list power` | Assembly listing of the 8² program | 13 instructions, 3 data words |
| `./test_cpu` | Unit tests (every instruction, every error) + golden runs | `0 failed` |
| `./sim16 help` | All commands | |
| `./sim16 > run.txt` | Save the full output to a file | |

| In the output | Meaning |
|---|---|
| `STEP 3   @02  5020  MUL R0, R1` | Step number, address, machine code, instruction |
| `PC  IR  Z N` | Next instruction address, instruction register, zero / negative flags |
| `R0 … R5  FP  SP` | Registers (FP = R6 frame pointer, SP = R7 stack pointer) |
| `*` after a value | Changed by this instruction |
| `TEXT / DATA / HEAP / STACK` rows | Memory block: code, globals, heap, top of stack |
| `OUTPUT> 40320` | Value printed by the `OUT` instruction |

## Files

| File | Purpose |
|---|---|
| `README.md` | This file |
| `Makefile` | `make`, `make run`, `make test`, `make clean` |
| `src/assignment.h` | One line: this copy is Assignment 1 |
| `src/isa.h` | Instruction set: memory map, registers, opcodes, formats |
| `src/cpu.h`, `src/cpu.c` | CPU: registers, fetch / decode / execute, all 24 instructions, stack, CALL/RET |
| `src/assembler.h`, `src/assembler.c` | Assembler (two passes) and disassembler |
| `src/loader.h`, `src/loader.c` | Loads a program into the 4 memory segments |
| `src/printer.h`, `src/printer.c` | Every printed view (registers, memory grid, segments) |
| `src/programs.h`, `src/programs.c` | The assembly programs (8!, 8²) |
| `src/text.h`, `src/text.c` | Own string helpers (only `printf` / `putchar` allowed) |
| `src/main.c` | Command line |
| `tests/test_cpu.c` | Unit tests and golden runs |
| `programs/power.c` | 8² in C (translated by hand to the `power` assembly) |
| `output/factorial_trace.txt` | Saved output of `./sim16 factorial` |
| `output/power_trace.txt` | Saved output of `./sim16 power` |

## Where things are

| What | Where |
|---|---|
| Assembly programs (8!, 8²) | `src/programs.c` (or `./sim16 list`) |
| C source of 8² | `programs/power.c` |
| Saved output | `output/` |
