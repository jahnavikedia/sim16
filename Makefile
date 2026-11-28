# Makefile -- build, run and test SIM16
#
#   make         build sim16 and test_cpu
#   make run     build, then run ./sim16 (this assignment's programs)
#   make test    build, then run the unit tests
#   make clean   remove built files
#
# Without make:
#   cc -std=c99 -Wall -o sim16 src/main.c src/text.c src/cpu.c src/loader.c \
#      src/assembler.c src/programs.c src/printer.c
#   cc -std=c99 -Wall -o test_cpu tests/test_cpu.c src/text.c src/cpu.c src/loader.c \
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

clean:
	rm -f sim16 test_cpu

.PHONY: all run test clean

# ==== BEGIN REPO ONLY (make submit removes this part from the zips) ====
#
#   make outputs     save each program's output in output/
#   make spec-check  prove the output matches the SPEC 7 examples
#   make submit      build the two upload packages in submission/

outputs: sim16
	mkdir -p output
	./sim16 factorial > output/factorial_trace.txt
	./sim16 power > output/power_trace.txt
	./sim16 recursive > output/recursive_calls.txt

# Cut the SPEC 7.2 / 7.3 / 7.4 example blocks out of the real output and
# compare them with tests/spec/*.txt (copied from docs/SPEC.md). No diff = match.
spec-check: outputs
	awk '/^STEP 27 /{p=1} p{n++} p && n>=5 && n<=11' output/factorial_trace.txt \
		| diff tests/spec/spec_7_2_grid_step27.txt -
	awk '/^STEP 3 /{p=1; print prev} p && n<11 {print; n++} {prev=$$0}' \
		output/factorial_trace.txt | diff tests/spec/spec_7_3_step3.txt -
	awk '/AFTER CALL #2:/{p=1; print prev} p{print} p && /\(free\)/{exit} {prev=$$0}' \
		output/recursive_calls.txt | diff tests/spec/spec_7_4_call2.txt -
	@echo "SPEC 7.2, 7.3, 7.4: output matches character for character"

# Two upload packages (one per assignment), each with its own zip + PDF:
#   submission/Assignment1_CPU_Design/sim16_assignment1.zip (+ PDF)
#   submission/Assignment2_Program_Layout/sim16_assignment2.zip (+ PDF)
STAGE = submission/stage

submit: outputs
	rm -rf submission
	$(MAKE) package N=1 PKG=Assignment1_CPU_Design DIR=sim16_assignment1 \
		PROGS="power.c" OUTS="factorial_trace.txt power_trace.txt"
	$(MAKE) package N=2 PKG=Assignment2_Program_Layout DIR=sim16_assignment2 \
		PROGS="fact_recursive.c" OUTS="recursive_calls.txt"
	rm -rf $(STAGE)
	@echo "Packages:" && ls -R submission

# One package: copy the files into $(STAGE)/$(DIR), set ASSIGNMENT = N, zip it.
package:
	mkdir -p $(STAGE)/$(DIR)/src $(STAGE)/$(DIR)/tests $(STAGE)/$(DIR)/programs \
		$(STAGE)/$(DIR)/output submission/$(PKG)
	cp README_A$(N).md $(STAGE)/$(DIR)/README.md
	sed '/^# ==== BEGIN REPO ONLY/,/^# ==== END REPO ONLY/d' Makefile > $(STAGE)/$(DIR)/Makefile
	cp src/*.c src/*.h $(STAGE)/$(DIR)/src/
	sed 's/^#define ASSIGNMENT 0$$/#define ASSIGNMENT $(N)/' src/assignment.h \
		> $(STAGE)/$(DIR)/src/assignment.h
	cp tests/test_cpu.c $(STAGE)/$(DIR)/tests/
	cd programs && cp $(PROGS) ../$(STAGE)/$(DIR)/programs/
	cd output && cp $(OUTS) ../$(STAGE)/$(DIR)/output/
	cd $(STAGE) && zip -qrX ../$(PKG)/$(DIR).zip $(DIR)
	if [ -f report/$(PKG).pdf ]; then cp report/$(PKG).pdf submission/$(PKG)/; fi

.PHONY: outputs spec-check submit package
# ==== END REPO ONLY ====
