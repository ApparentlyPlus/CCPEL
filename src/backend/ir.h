/*
 * CCPEL - Intermediate Representation Header
 * Author: ApparentlyPlus
 */

#ifndef _IR_H
#define _IR_H

#include <stdio.h>

#define NO_LABEL 0

typedef struct Instr {
    int num;
    char *text;
    int lbl;
    struct Instr *next;
} Instr;

typedef Instr *InstrTable;

void addI(char *text, int lbl);
void prtI(FILE *out);

#define addInstruction(txt, lbl) addI(txt, lbl)
#define printIntCode(str) prtI(str)

#endif
