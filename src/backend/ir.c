/*
 * CCPEL - Intermediate Representation Implementation
 * Author: ApparentlyPlus
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sglib.h"
#include "ir.h"

// INTERCODE is the head of the linked list representing the IR instruction stream
InstrTable INTERCODE = NULL;

// curN is the next available instruction number for IR generation
static int curN = 10;

// Adds a new instruction to the IR stream with the given text and label
void addI(char *text, int lbl)
{
    Instr *i = malloc(sizeof(Instr));
    i->num = curN;
    i->text = strdup(text);
    i->lbl = lbl;
    curN++;
    SGLIB_LIST_ADD(Instr, INTERCODE, i, next);
}

// Prints the IR instruction stream to the given output file, reversing the list to maintain original order
void prtI(FILE *out)
{
    Instr *p __attribute__((unused));

    SGLIB_LIST_REVERSE(Instr, INTERCODE, next);
    SGLIB_LIST_MAP_ON_ELEMENTS(Instr, INTERCODE, p, next, {
        (void)p;
        fprintf(out, "%s", p->text);
        if (p->lbl != 0) {
            fprintf(out, "%d", p->lbl);
        }
        fprintf(out, "\n");
    });
}
