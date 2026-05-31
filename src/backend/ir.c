/* File: ir.c
   Source file implementing the instruction storage and serialization.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sglib.h"
#include "ir.h"

// There is a single instruction table.
INSTRUCTION_TABLE_TYPE INTERCODE = NULL;
int current_instruction_number = 10;

void add_i(char *text, int label)
{
    INSTRUCTION_TYPE *new_instruction = malloc(sizeof(INSTRUCTION_TYPE));
    new_instruction->number = current_instruction_number;
    new_instruction->text = strdup(text);
    new_instruction->label = label;
    current_instruction_number++;
    // SGLIB function that adds the new instruction at the end of the linked list.
    SGLIB_LIST_ADD(INSTRUCTION_TYPE, INTERCODE, new_instruction, next_i);
}

void prt_i(FILE *outputStream)
{
    INSTRUCTION_TYPE *instr __attribute__((unused));

    SGLIB_LIST_REVERSE(INSTRUCTION_TYPE, INTERCODE, next_i);
    SGLIB_LIST_MAP_ON_ELEMENTS(INSTRUCTION_TYPE, INTERCODE, instr, next_i, {
        (void)instr;
        fprintf(outputStream, "%s", instr->text);
        if (instr->label != 0) {
            fprintf(outputStream, "%d", instr->label);
        }
        fprintf(outputStream, "\n");
    });
}
