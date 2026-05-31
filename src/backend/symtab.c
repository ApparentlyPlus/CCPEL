/*
 * CCPEL - Symbol Table Implementation
 * Author: ApparentlyPlus
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"
#include "codegen.h"

// sv is the next available stack position for variable storage
static int sv = 1;

// Resets the symbol table state for a new compilation unit
void rst() {
    sv = 1;
}

// Returns the next available stack position for variable storage
int nextPos() {
    return sv;
}

// Adds a new variable entry to the symbol table if it doesn't already exist
int add(Table *t, char *name, ParType type)
{
	Entry *e;
	if (!has(*t, name))
	{
		e = malloc(sizeof(Entry));
		e->name = name;
		e->type = type;
		e->pos = sv;
		SGLIB_LIST_ADD(Entry, *t, e, next);
		sv++;
		return 1;
	}
	else return 0;
}

// Checks if a variable name exists in the symbol table
int has(Table t, char *name) {
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res != NULL;
}

// Retrieves the type of a variable from the symbol table, or type_error if not found
ParType typeOf(Table t, char *name)
{
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res == NULL ? type_error : res->type;
}

// Retrieves the stack position of a variable from the symbol table, or 0 if not found
int posOf(Table t, char *name)
{
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res == NULL ? 0 : res->pos;
}
