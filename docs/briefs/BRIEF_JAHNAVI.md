# Brief: Jahnavi

**Your role:** CPU core. Every program runs through your code, so you are the hub.
**Read first:** `TEAM_GUIDE.md`, then `SPEC.md` sections 3 and 5.

---

## Your files

| File | What |
|---|---|
| `cpu.py` | The `CPU` class: registers, memory, fetch/decode/execute, flags, stack, CALL/RET, snapshots |
| `tests/test_cpu.py` | `unittest` tests: every instruction, every error, three golden runs |

Do **not** edit `isa.py` or anyone else's files.

---

## Steps

1. Give your AI `SPEC.md`, `isa.py` and the prompt below.
2. Get `step()` working for LDI / ADD / HALT first, then **push it** so the others can try real runs.
3. Finish all 24 instructions, then CALL/RET and the error checks.
4. Write the tests and run them until everything passes.
5. Push `cpu.py` and `tests/test_cpu.py`.

---

## AI prompt (copy everything in the box)

```
I am attaching SPEC.md and isa.py for a CPU simulator called SIM16 (a class project).
Please write two files: cpu.py and tests/test_cpu.py.

cpu.py:
- Import all constants and CPUError from isa.py. Do not redefine them. Do not edit isa.py.
- Implement the CPU class exactly as in SPEC section 6.1, with:
  - the public attributes in 5.1
  - the reset state in 4.3 step 2
  - snapshot() returning a deep copy in the shape of 5.2
  - events as in 5.3
  - the fetch/decode/execute order in 3.5
  - instruction semantics and flag rules in 3.4
  - the stack / CALL / RET micro-operations and error checks in 3.6 (all checks
    happen BEFORE anything is changed)
  - error messages exactly as in 3.8
- Do not import assembler.py. The CPU works on raw 16-bit words.
- Python 3.8+, standard library only, no 3.9+ syntax.
- Add a module docstring (owner: Jahnavi), a docstring on every method,
  and a short comment block for every instruction.

tests/test_cpu.py:
- unittest only, using raw hex words (no assembler).
- One test per instruction, including flag behaviour.
- Error tests: stack overflow, stack underflow, invalid opcode, memory address
  out of range, exceeded max steps, stepping a halted CPU.
- Three golden tests that place the hex from SPEC section 9 into memory by hand
  (text at 0x00, data at 0x60, SP = FP = 0x100, PC = 0), run, and assert the
  expected steps, output, memory and registers listed in section 9.
- The fact_recursive golden test must also assert the stack contents after each
  of the 5 CALL events (table in 9.3), captured with run(on_step=...).
- Must run with: python3 -m unittest discover -s tests -v  (from the repo root)
```

---

## Check you're done

```
python3 -m unittest discover -s tests -v
```
- [x] All tests pass (46 tests)
- [x] Factorial: 29 steps, output 40320, memory 0x61 = 9D80
- [x] Power: 20 steps, output 64, memory 0x62 = 0040
- [x] Recursive: 52 steps, output 120, and the 5 stack snapshots match SPEC 9.3

**Status:** `cpu.py` and `tests/test_cpu.py` are done and pushed to `main`.

---

## Your report pieces

| Piece | PDF | What to make | Caption example |
|---|---|---|---|
| CPU block diagram | Assignment 1 | Draw in draw.io / Excalidraw, export PNG. Show: R0–R5, FP (R6), SP (R7), PC, IR, Z/N flags, ALU, control unit / decoder, 256-word memory split into TEXT/DATA/HEAP/STACK, and the address / data / control buses with arrows. Short labels only. | *Figure 1: SIM16 CPU block diagram* |
| Instruction encoding table | Assignment 1 | The bit-layout picture from SPEC 3.3 + the table from SPEC 3.4 (opcode, mnemonic, format, syntax, meaning, example, hex) | *Table 1: SIM16 instruction encoding* |
| ISA summary | Assignment 1 | One page: registers table (3.1), memory map (3.2), the 6 formats (3.3) | *Table 2: Registers* · *Figure 2: Memory map* |
| Execution-trace pages | Assignment 1 | After integration, copy step blocks from `output/factorial_trace.txt` and `output/power_trace.txt` into the doc in Consolas 8 pt | *Figure 3: Execution trace, 8! (steps 1–29)* |

Give your finished pieces to Snehil (he assembles the PDFs).

**Status (all files in `report/jahnavi/`):**
- [x] Instruction encoding table: `encoding_table.md`
- [x] ISA summary: `isa_summary.md`
- [x] CPU block diagram: `cpu_block_diagram.png` (source: `cpu_block_diagram.drawio`)
- [ ] Execution-trace pages: waiting for integration (`output/factorial_trace.txt`, `output/power_trace.txt`)
