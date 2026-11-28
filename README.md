# SIM16 CPU Simulator (CMPE 220 System Software)

Team: Jahnavi Kedia · Shrutthi Khatta · Snehil Raut

| Assignment | README | Upload package |
|---|---|---|
| 1: CPU Design | [README_A1.md](README_A1.md) | `submission/Assignment1_CPU_Design/` |
| 2: Program Layout & Execution | [README_A2.md](README_A2.md) | `submission/Assignment2_Program_Layout/` |

## Repo commands

| Command | Does |
|---|---|
| `make` | Build `sim16` and `test_cpu` (repo build: all three programs) |
| `./sim16` | Run factorial, power and recursive |
| `make test` | Unit tests + golden runs |
| `make outputs` | Regenerate `output/*.txt` |
| `make spec-check` | Prove the output matches the SPEC 7 examples |
| `make submit` | Build the two upload packages in `submission/` |

| Folder | Contents |
|---|---|
| `src/`, `tests/` | Simulator and tests (C, `printf`/`putchar` only) |
| `programs/` | C sources that were translated to assembly |
| `output/` | Saved program output |
| `report/` | Report sources and PDFs |
| `docs/` | Spec, team guide, study guide (not in the zips) |
