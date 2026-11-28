# Makefile -- build, run and test SIM16
#
#   make             build sim16 and test_cpu
#   make run         build, then run ./sim16 (this assignment's programs)
#   make test        build, then run the unit tests
#   make outputs     save each program's output in output/   (repo build)
#   make spec-check  prove the output matches the SPEC 7 examples (repo build)
#   make clean       remove built files
#
# Without make:
#   cc -std=c99 -Wall -o sim16 src/main.c src/text.c src/cpu.c src/loader.c \
#      src/assembler.c src/programs.c src/printer.c

CC     = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic

# Every source file except the two main() files
SRC = src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c src/printer.c

all: sim16 test_cpu

sim16: src/main.c $(SRC) src/*.h
	$(CC) $(CFLAGS) -o sim16 src/main.c $(SRC)

test_cpu: tests/test_cpu.c $(SRC) src/*.h
	$(CC) $(CFLAGS) -o test_cpu tests/test_cpu.c $(SRC)

run: sim16
	./sim16

test: test_cpu
	./test_cpu

outputs: sim16
	mkdir -p output
	./sim16 factorial > output/factorial_trace.txt
	./sim16 power > output/power_trace.txt
	./sim16 recursive > output/recursive_calls.txt

# Cut the SPEC 7.2 / 7.3 / 7.4 example blocks out of the real output and
# compare them with tests/spec/*.txt (copied from SPEC.md). No diff = match.
spec-check: outputs
	awk '/^STEP 27 /{p=1} p{n++} p && n>=5 && n<=11' output/factorial_trace.txt \
		| diff tests/spec/spec_7_2_grid_step27.txt -
	awk '/^STEP 3 /{p=1; print prev} p && n<11 {print; n++} {prev=$$0}' \
		output/factorial_trace.txt | diff tests/spec/spec_7_3_step3.txt -
	awk '/AFTER CALL #2:/{p=1; print prev} p{print} p && /\(free\)/{exit} {prev=$$0}' \
		output/recursive_calls.txt | diff tests/spec/spec_7_4_call2.txt -
	@echo "SPEC 7.2, 7.3, 7.4: output matches character for character"

clean:
	rm -f sim16 test_cpu

.PHONY: all run test outputs spec-check clean
