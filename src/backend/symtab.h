/*
 * CCPEL - Symbol Table Header
 * Author: ApparentlyPlus
 */

#ifndef _symbol_table_h
#define _symbol_table_h

#include "codegen.h"
#include "sglib.h"

typedef struct Entry {
	char *name;
	ParType type;
	int pos;
	struct Entry *next;
} Entry;

typedef Entry *Table;

#define ST_COMPARATOR(e1,e2) (strcmp(e1->name,e2->name))

int add(Table *t, char *name, ParType type);
void rst();
int nextPos();
int has(Table t, char *name);
ParType typeOf(Table t, char *name);
int posOf(Table t, char *name);

#endif
