# Brief: Snehil

**Your role:** loader, all printed output (what the grader actually looks at), the 8² program, and putting the two PDFs together.
**Read first:** `TEAM_GUIDE.md`, then `SPEC.md` sections 4.3, 5 and 7.

---

## Your files

| File | What |
|---|---|
| `loader.py` | Puts an assembled program into CPU memory (the 4 segments) |
| `printer.py` | Every formatted output: memory grid, step blocks, four-segment view, stack frames |
| `programs/power.c` + `.asm` | Assignment 1: 8² in C + its translation |

Do **not** edit `isa.py` or anyone else's files.

---

## Steps

1. Give your AI `SPEC.md`, `isa.py` and the prompt below.
2. Build `loader.py` (small).
3. Build `printer.py` against **fake snapshots** (dicts built from SPEC 5.4 and 9.3), so you don't have to wait for Jahnavi's CPU. Until Shrutthi pushes `disassemble()`, use the stub described in SPEC 6.4.
4. Create `programs/power.c` and `programs/power.asm` exactly as in SPEC 9.2.
5. At integration, run the real thing and confirm your output matches the examples.

---

## AI prompt (copy everything in the box)

```
I am attaching SPEC.md and isa.py for a CPU simulator called SIM16 (a class project).
Please write: loader.py, printer.py, programs/power.c and programs/power.asm.

loader.py:
- Import everything from isa.py. Do not redefine constants. Do not edit isa.py.
- Provide load(obj, cpu) and load_file(path, cpu) exactly as in SPEC 6.3,
  following the loader rules in 4.3 and raising LoaderError with the messages in 3.8.
- load() must work with any object that has the CPU attributes in 5.1
  (do not import cpu.py). load_file() may import assembler.

printer.py:
- Provide the six format_* functions with the exact signatures in SPEC 6.4
  (note format_stack takes prev=None). They take snapshot dicts (5.2) and the
  object dict (4.1), RETURN strings and never print.
- Produce exactly the layouts in section 7, using the exact format strings in 7.6:
  ASCII only, at most 80 columns, trailing spaces stripped, "*" change markers
  found by comparing prev and snap, compact grid rows as defined in 7.2, and the
  frame-chain walk with the safety guard from 3.7 for the stack view.
- Use assembler.disassemble(word, obj["symbols"]) for instruction text. If
  assembler.py is not ready yet, fall back to the stub in 6.4.
- Also give me a small throwaway script that builds fake snapshots from the
  golden data in SPEC 5.4 and 9.3, so I can test printer.py before the real CPU
  exists.

Programs: create programs/power.c and programs/power.asm EXACTLY as written in
SPEC section 9.2 (same instructions; comments may be improved).

General: Python 3.8+, standard library only, module docstring (owner: Snehil),
a docstring on every function, comments on non-obvious lines.
```

---

## Check you're done

These must match the SPEC examples **character for character** (fully checkable at integration, when real snapshots exist):
- [ ] `format_memory_grid(step27, step26, obj)` for factorial = the SPEC 7.2 example
- [ ] `format_step(step2, step3, obj)` for factorial = the SPEC 7.3 example
- [ ] `format_segments(call2, obj, "AFTER CALL #2: fact(4)", prev=call1)` = the SPEC 7.4 example
- [ ] Empty stack prints `SP=100  FP=100` and `(empty)`
- [ ] No line longer than 80 characters; no non-ASCII characters

---

## Your report pieces

### Assignment 2 per-call memory pages
After integration, `python3 main.py --all` creates `output/fact_recursive_calls.txt`. Copy into the doc (Consolas 8–9 pt), **one figure per page**:

| Figure | Content | Caption |
|---|---|---|
| 3 | Load view | *Figure 3: Program loaded into memory (step 0)* |
| 4–8 | After CALL #1 … #5 | *Figure 4: Memory after CALL #1, fact(5)* … *Figure 8: Memory after CALL #5, fact(1)* |
| 9 | Final state | *Figure 9: Memory at end of program (after HALT)* |

### Assembling both PDFs (you own the final look)
Use the page plans in `TEAM_GUIDE.md` section 7. For **both** PDFs:
- [ ] Title page: course (CMPE 220), assignment name, **all three names**
- [ ] Contributions page (TEAM_GUIDE section 8)
- [ ] Page numbers in `##/##` format on every page
- [ ] Every figure and table captioned and numbered in order
- [ ] Monospace font for all memory dumps / code, same size throughout
- [ ] No long paragraphs anywhere
- [ ] Export to PDF, open it, check page numbers and that nothing is cut off
