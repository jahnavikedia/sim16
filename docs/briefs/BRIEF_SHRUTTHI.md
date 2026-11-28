# Brief: Shrutthi

**Your role:** assembler, command-line runner, README, and two of the programs.
**Read first:** `TEAM_GUIDE.md`, then `SPEC.md` sections 3.9, 3.10, 4 and 8.

---

## Your files

| File | What |
|---|---|
| `assembler.py` | `.asm` text → object dict; `disassemble()`; `listing()`; small CLI |
| `main.py` | The command the grader runs: `python3 main.py ...` |
| `README.md` | How to run it (the grader reads this first) |
| `programs/factorial.asm` | Assignment 1: 8! written directly in assembly |
| `programs/fact_recursive.c` + `.asm` | Assignment 2: recursive factorial in C + its translation |

Do **not** edit `isa.py` or anyone else's files.

---

## Steps

1. Give your AI `SPEC.md`, `isa.py` and the prompt below.
2. Build `assembler.py` first, including `disassemble()`, and **push it early**: Snehil's printer uses `disassemble()`.
3. Create the program files exactly as in SPEC section 9.
4. Build `main.py`. It needs everyone's code, so it can only be fully tested at integration.
5. Write `README.md`.

---

## AI prompt (copy everything in the box)

```
I am attaching SPEC.md and isa.py for a CPU simulator called SIM16 (a class project).
Please write: assembler.py, main.py, README.md, programs/factorial.asm,
programs/fact_recursive.c and programs/fact_recursive.asm.

assembler.py:
- Import everything from isa.py. Do not redefine constants. Do not edit isa.py.
- Provide assemble, assemble_file, disassemble and listing exactly as in SPEC 6.2.
- Accept the syntax in 3.9 (two-pass; numbers parsed as described there, NOT
  with int(s, 0); labels may not be reserved words in any case).
- Produce the object dict in section 4 with all keys, including source_map.
- Raise AssemblerError with the messages in 3.8.
- disassemble() uses the canonical format and the symbol-lookup rule in 3.10,
  and never raises.
- CLI: python3 assembler.py file.asm [-o out.json] prints the listing.

main.py:
- Implement the CLI in SPEC section 8 with argparse, using loader.load_file,
  CPU.run(on_step=...) and the printer.format_* functions (section 6.4).
- Print exactly the sequence listed for each mode (trace / calls / final).
- --all runs the three programs and writes output/*.txt; paths resolved relative
  to the folder containing main.py.
- Catch isa.SimError -> print "ERROR: <message>" to stderr, exit code 1.
  Missing file -> "ERROR: file not found: <path>", exit 1. No tracebacks.

Programs: create the three program files EXACTLY as written in SPEC section 9.1
and 9.3 (same instructions; comments may be improved).

README.md: 2-line summary; requirements (Python 3.8+, nothing to install); the
5 commands from section 8; the file/owner table from section 2; expected outputs
(40320, 64, 120); the Windows note (use `python` instead of `python3`).

General: Python 3.8+, standard library only, ASCII-only output, module docstring
(owner: Shrutthi), a docstring on every function, comments on non-obvious lines.
```

---

## Check you're done

```
python3 assembler.py programs/factorial.asm
python3 assembler.py programs/fact_recursive.asm
```
- [ ] The hex matches SPEC 9.1 and 9.3 exactly
- [ ] Every row of the SPEC 9.4 table converts both ways (assembly → hex and hex → assembly)
- [ ] A typo in an `.asm` file gives `line N: ...` (no traceback)

After integration:
```
python3 main.py programs/factorial.asm
python3 main.py programs/fact_recursive.asm --mode calls
python3 main.py --all
python3 main.py nofile.asm        # must print ERROR: file not found
```

---

## Your report pieces

| Piece | PDF | What to make | Caption example |
|---|---|---|---|
| C ↔ assembly (Assignment 1) | Assignment 1 | 2-column table: each line of `power.c` next to its assembly lines, with the register mapping (r→R0, i→R1, exponent→R2, base→R3). Plus the `factorial.asm` listing (`python3 assembler.py programs/factorial.asm`) | *Table 3: power.c translated to SIM16 assembly* |
| C ↔ assembly (Assignment 2) | Assignment 2 | Same 2-column table for `fact_recursive.c` | *Table 1: fact_recursive.c translated to SIM16 assembly* |
| Calling convention / stack frame | Assignment 2 | Diagram of one frame (arg / return addr / saved FP, FP and SP arrows) + the CALL and RET steps from SPEC 3.6–3.7 | *Figure 2: Stack frame and calling convention* |
| Segment layout | Assignment 2 | Memory map 0x00–0xFF with TEXT/DATA/HEAP/STACK, filled in for the recursive program (code 0x00–0x12, n at 0x60, result at 0x61, stack growing down from 0xFF) | *Figure 1: Program layout in memory* |
| How to run | Both | The README commands | *Table N: How to run* |

Give your finished pieces to Snehil (he assembles the PDFs).
