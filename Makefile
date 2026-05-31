# Disable all built-in implicit rules to avoid conflicts
MAKEFLAGS += -r

# Prevent automatic deletion of intermediate files (.s and .o)
.SECONDARY:

# Check that necessary tools exist before compiling
NASM_EXISTS := $(shell which nasm 2>/dev/null)
GCC_EXISTS := $(shell which gcc 2>/dev/null)

ifeq ($(NASM_EXISTS),)
$(error "Error: nasm is not installed. Please install nasm to compile this template.")
endif

ifeq ($(GCC_EXISTS),)
$(error "Error: gcc is not installed. Please install gcc to compile this template.")
endif

# Compiler & Tool Settings
CC      := gcc
CFLAGS  := -Wall -Wextra -Isrc -Isrc/backend -Ibuild
LEX     := flex
LFLAGS  := -s
YACC    := bison
YFLAGS  := -v

.PHONY: default clean test

default: ccpel

# Create build directory if it doesn't exist
build:
	mkdir -p build

# Link the final ccpel executable
ccpel: build/parser.tab.c build/symtab.o build/codegen.o build/ir.o
	$(CC) $(CFLAGS) -o ccpel build/parser.tab.c build/symtab.o build/codegen.o build/ir.o

# Flex compilation
build/lexer.lex.c: src/lexer.l | build
	$(LEX) $(LFLAGS) -obuild/lexer.lex.c src/lexer.l

# Bison compilation
build/parser.tab.c: src/parser.y build/lexer.lex.c src/backend/codegen.h src/backend/symtab.h | build
	$(YACC) $(YFLAGS) -o build/parser.tab.c src/parser.y

# C Objects compilation
build/symtab.o: src/backend/symtab.c src/backend/symtab.h src/backend/codegen.h | build
	$(CC) $(CFLAGS) -c -o build/symtab.o src/backend/symtab.c

build/codegen.o: src/backend/codegen.c src/backend/codegen.h | build
	$(CC) $(CFLAGS) -c -o build/codegen.o src/backend/codegen.c

build/ir.o: src/backend/ir.c src/backend/ir.h | build
	$(CC) $(CFLAGS) -c -o build/ir.o src/backend/ir.c

# Test Target Configuration - Dynamically discovered and sorted alphabetically
TESTS := $(sort $(basename $(notdir $(shell find tests -maxdepth 1 -type f -name "*.cpel" ! -name "*error*"))))
TEST_BINS := $(patsubst %,tests/bin/%,$(TESTS))

# The main test entry
test: ccpel $(TEST_BINS)
	@-clear
	@echo "                [ Running Test Suite ]                "
	@echo ""
	@for t in $(TESTS); do \
		echo "Running test: $$t"; \
		./tests/bin/$$t; \
		echo ""; \
	done
	@echo "All tests executed successfully."

# Ensure test subdirectories exist
tests/asm tests/obj tests/bin:
	mkdir -p $@

# Translate prefix source to x86 assembly
tests/asm/%.s: tests/%.cpel ccpel | tests/asm
	./ccpel $< $@

# Assemble x86 assembly to object files
tests/obj/%.o: tests/asm/%.s | tests/obj
	nasm -f elf64 -o $@ $<

# Link object files to executable binaries
tests/bin/%: tests/obj/%.o | tests/bin
	$(CC) -no-pie -o $@ $<

# Merged clean target
clean:
	rm -rf build
	rm -f ccpel
	rm -rf tests/asm tests/obj tests/bin
