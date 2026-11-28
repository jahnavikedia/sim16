# SIM16 Team Guide: start here

**Team:** Jahnavi · Shrutthi · Snehil
**Deliverables:** one GitHub repo (code) + two PDFs (Assignment 1: CPU Design, Assignment 2: Program Layout & Execution)

---

## 1. What's in this folder

| File | What it is | Who uses it |
|---|---|---|
| `TEAM_GUIDE.md` | This file: the plan, the order of work, the report plan | Everyone, read first |
| `SPEC.md` | The full technical contract (ISA, file formats, function signatures, exact output layouts, expected results) | Everyone + everyone's AI |
| `isa.py` | The shared constants file. **Never edit it** without telling the other two | Everyone's code imports it |
| `briefs/BRIEF_JAHNAVI.md` | Jahnavi's tasks, AI prompt, checks, report pieces | Jahnavi |
| `briefs/BRIEF_SHRUTTHI.md` | Shrutthi's tasks, AI prompt, checks, report pieces | Shrutthi |
| `briefs/BRIEF_SNEHIL.md` | Snehil's tasks, AI prompt, checks, report pieces | Snehil |
| `cpu.py`, `assembler.py`, … | Empty stubs with the owner's name at the top | Each owner replaces their own |

---

## 2. Who owns what

| | **Jahnavi** | **Shrutthi** | **Snehil** |
|---|---|---|---|
| **Code** | `cpu.py`, `tests/test_cpu.py` | `assembler.py`, `main.py`, `README.md`, `programs/factorial.asm`, `programs/fact_recursive.c` + `.asm` | `loader.py`, `printer.py`, `programs/power.c` + `.asm` |
| **Report** | CPU block diagram · instruction encoding table · ISA summary · Assignment 1 execution-trace pages | C ↔ assembly pages · calling-convention / stack-frame diagram · segment layout diagram | Assignment 2 per-call memory pages · **assembles both PDFs** |
| **Final check** | Fresh-machine test on own laptop | Fresh-machine test on own laptop | Fresh-machine test on own laptop |

**Golden rule:** only edit your own files. Need something changed in someone else's file? Message them.

---

## 3. Setup (one person, ~10 min, do first)

1. One person creates a **private GitHub repo** named `sim16` and adds the other two as collaborators.
2. Unzip this starter folder, then from inside `sim16/`:
   ```
   git init
   git add .
   git commit -m "Starter: spec, isa.py, stubs"
   git branch -M main
   git remote add origin <repo-url>
   git push -u origin main
   ```
3. The other two: `git clone <repo-url>`.
4. Everyone checks Python: `python3 --version` must say **3.8 or newer** (Windows: `python --version`).

**Day-to-day git** (files never overlap, so this stays simple):
```
git pull                         # before you start
git add <only your files>
git commit -m "cpu: add CALL/RET"
git pull && git push             # share your work
```

---

## 4. Order of work

```
Day 1      KICKOFF (all 3, ~1 hr)
           - everyone reads SPEC.md sections 1-5 together
           - confirm: nobody changes isa.py or the formats without telling the group
           - agree on the submission date and an integration date

Days 2-4   EVERYONE IN PARALLEL  (nobody waits for anybody)
           Jahnavi : cpu.py + tests        (tests use raw hex, no assembler needed)
           Shrutthi: assembler.py + main.py (check hex against SPEC section 9)
           Snehil  : loader.py + printer.py (use fake snapshots built from SPEC 5.4 / 9.3)
           + start the report pieces that DON'T need real output
             (diagrams, encoding table, C <-> assembly pages)

Day 5      INTEGRATION (all 3, ~1 hr)
           - pull everything, run SPEC section 11 checklist together
           - run: python3 main.py --all   -> creates output/*.txt
           - commit output/ files

Day 6      REPORTS + FINAL CHECK
           - Jahnavi & Snehil paste real output into their report pages
           - Snehil assembles both PDFs
           - EVERYONE: fresh-machine test (below)
           - submit
```

**Things you can share early to unblock others:**
- Jahnavi: push a basic `cpu.py` as soon as LOAD/ADD/HALT work, so others can try real runs.
- Shrutthi: push `assembler.disassemble()` early, because Snehil's printer uses it.

---

## 5. How to use your AI

Give your AI these **three things**, in one message:
1. `SPEC.md`
2. `isa.py`
3. The "AI prompt" box from **your** brief in `briefs/`

Then:
- Ask it to **write your files only**.
- Run the "Check you're done" commands from your brief. If something fails, paste the error back to your AI.
- If your AI wants to change the spec or `isa.py`: **don't.** Ask the group first.
- Review what it wrote: you must be able to explain your own code.

---

## 6. Fresh-machine test (everyone, before submitting)

This protects the biggest rubric item ("ran the first time I tried it", 50 pts):
```
cd /tmp            (or any empty folder; Windows: cd %TEMP%)
git clone <repo-url> test-sim16
cd test-sim16
```
Then run **exactly** the commands in `README.md`, without editing anything. All five must work and show 40320, 64 and 120. If anything fails, tell the owner of that file.

---

## 7. Report plan (both PDFs)

### General rules (from the assignment; we lose points if we miss these)
- **Title page** with all three names clearly.
- **Page numbers in `##/##` format** (e.g. `03/14`). In Word: footer → field `PAGE` + "/" + field `NUMPAGES`. In Google Docs: Insert → Page numbers, then type "/" and Insert → Page count.
- **Every table and figure has a caption** ("Figure 3: Stack after CALL #2, fact(4)").
- **No long prose.** Diagrams and tables only, plus at most one or two lines per page.
- Memory dumps in a **monospace font** (Consolas / Courier New, 8–9 pt) so the columns line up.
- A **Team contributions** page (text below).

### Assignment 1 PDF: CPU Design (Snehil assembles)

| # | Page | Content | From |
|---|---|---|---|
| 1 | Title | Course, assignment, team names | Snehil |
| 2 | Contributions | Section 8 text | Snehil |
| 3 | Figure 1: CPU block diagram | Registers, PC, IR, flags, ALU, control unit, memory, buses | Jahnavi |
| 4 | Table 1: Instruction encoding | Bit layout + all 24 instructions | Jahnavi |
| 5 | ISA summary | Registers, memory map, formats | Jahnavi |
| 6 | How to run | The README commands | Shrutthi |
| 7 | Program 1: 8! in assembly | `factorial.asm` listing + Figure: execution trace | Shrutthi (listing), Jahnavi (trace) |
| 8 | Program 2: 8² from C | `power.c` ↔ `power.asm` side by side + Figure: execution trace | Shrutthi (C ↔ asm), Jahnavi (trace) |

### Assignment 2 PDF: Program Layout & Execution (Snehil assembles)

| # | Page | Content | From |
|---|---|---|---|
| 1 | Title | Course, assignment, team names | Snehil |
| 2 | Contributions | Section 8 text | Snehil |
| 3 | Figure 1: Program layout | The 4 segments with addresses | Shrutthi |
| 4 | Figure 2: Stack frame & calling convention | Frame layout + CALL/RET steps | Shrutthi |
| 5 | Recursive program | `fact_recursive.c` ↔ `.asm` side by side | Shrutthi |
| 6 | Figure 3: Program loaded into memory | Load view (4 segments) | Snehil |
| 7–11 | Figures 4–8: After CALL #1 … #5 | One per recursive call | Snehil |
| 12 | Figure 9: Final state | After HALT | Snehil |
| 13 | How to run | README commands | Shrutthi |

---

## 8. Team contributions (paste into both PDFs)

> **Jahnavi:** Code: implemented the CPU core (`cpu.py`), including fetch/decode/execute, all instructions, stack operations and CALL/RET, plus unit tests. Report: CPU block diagram, instruction encoding table, ISA summary, Assignment 1 execution traces.
>
> **Shrutthi:** Code: implemented the assembler (`assembler.py`) and runner (`main.py`); wrote the factorial assembly program and translated the recursive C program to assembly; wrote the README. Report: C ↔ assembly pages, calling-convention diagram, segment layout diagram.
>
> **Snehil:** Code: implemented the segment loader (`loader.py`) and memory visualizer (`printer.py`), including per-call stack snapshots; wrote the 8² C program and its assembly translation. Report: Assignment 2 per-call memory pages; assembled and formatted both PDFs.

(Edit this if the real split changes. It must match who actually did what.)

---

## 9. Submission checklist

- [ ] SPEC.md section 11 integration checklist all ticked
- [ ] `output/` committed with the three generated `.txt` files
- [ ] All three fresh-machine tests passed
- [ ] Both PDFs: title page, `##/##` page numbers, every figure/table captioned, contributions page
- [ ] Repo link (or zip) + both PDFs submitted before the deadline
