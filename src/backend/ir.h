/* File: ir.h
   Header file that provides definitions and functions for storing and printing the generated x86 instructions.
*/
#ifndef _IR_H
#define _IR_H

#include <stdio.h>

#define NO_LABEL 0

typedef struct instruction {
    int number;
    char *text;
    int label;
    struct instruction *next_i;
} INSTRUCTION_TYPE;

typedef INSTRUCTION_TYPE *INSTRUCTION_TABLE_TYPE;

// Minimal function names
void add_i(char *text, int label);
void prt_i(FILE *outputStream);

// Compatibility macros
#define addInstruction(txt, lbl) add_i(txt, lbl)
#define print_int_code(str) prt_i(str)

#endif
