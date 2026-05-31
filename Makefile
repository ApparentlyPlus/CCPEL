# Disable all builtin implicit rules to avoid conflicts
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
CC := gcc
CFLAGS := -Wall -Wextra -Isrc -Isrc/backend -Ibuild -O3
LEX := flex
LFLAGS := -s
YACC := bison
YFLAGS := -v

.PHONY: default clean test

default: ccpel

build:
	mkdir -p build

ccpel: build/parser.tab.c build/symtab.o build/codegen.o build/ir.o
	$(CC) $(CFLAGS) -o ccpel build/parser.tab.c build/symtab.o build/codegen.o build/ir.o

build/lexer.lex.c: src/lexer.l | build
	$(LEX) $(LFLAGS) -obuild/lexer.lex.c src/lexer.l

build/parser.tab.c: src/parser.y build/lexer.lex.c src/backend/codegen.h src/backend/symtab.h | build
	$(YACC) $(YFLAGS) -o build/parser.tab.c src/parser.y

build/symtab.o: src/backend/symtab.c src/backend/symtab.h src/backend/codegen.h | build
	$(CC) $(CFLAGS) -c -o build/symtab.o src/backend/symtab.c

build/codegen.o: src/backend/codegen.c src/backend/codegen.h | build
	$(CC) $(CFLAGS) -c -o build/codegen.o src/backend/codegen.c

build/ir.o: src/backend/ir.c src/backend/ir.h | build
	$(CC) $(CFLAGS) -c -o build/ir.o src/backend/ir.c

TESTS := $(sort $(basename $(notdir $(shell find tests -maxdepth 1 -type f -name "*.cpel" ! -name "*error*"))))
TEST_BINS := $(patsubst %,tests/bin/%,$(TESTS))

test: ccpel $(TEST_BINS)
	@-clear
	@echo "                [ Running Test Suite ]                "
	@echo ""
	@for t in $(TESTS); do \
		echo "Running test: $$t"; \
		./tests/bin/$$t || exit 1; \
		echo ""; \
	done
	@echo "All tests executed successfully."

tests/asm tests/obj tests/bin:
	mkdir -p $@

tests/asm/%.s: tests/%.cpel ccpel | tests/asm
	./ccpel $< $@

tests/obj/%.o: tests/asm/%.s | tests/obj
	nasm -f elf64 -o $@ $<

tests/bin/%: tests/obj/%.o | tests/bin
	$(CC) -no-pie -o $@ $<

clean:
	rm -rf build
	rm -f ccpel
	rm -rf tests/asm tests/obj tests/bin
