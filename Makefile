# Makefile -- build and test SIM16
#
#   make         build test_cpu
#   make test    build and run the tests
#   make clean   remove built files

CC     = cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic

CORE = src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c

all: test_cpu

test_cpu: tests/test_cpu.c $(CORE) src/*.h
	$(CC) $(CFLAGS) -o test_cpu tests/test_cpu.c $(CORE)

test: test_cpu
	./test_cpu

clean:
	rm -f test_cpu sim16

.PHONY: all test clean
