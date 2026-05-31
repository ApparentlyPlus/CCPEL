#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sglib.h"
#include "ir.h"

InstrTable INTERCODE = NULL;
static int curN = 10;

void addI(char *text, int lbl)
{
    Instr *i = malloc(sizeof(Instr));
    i->num = curN;
    i->text = strdup(text);
    i->lbl = lbl;
    curN++;
    SGLIB_LIST_ADD(Instr, INTERCODE, i, next);
}

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
