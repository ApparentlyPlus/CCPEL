#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"
#include "codegen.h"

static int sv = 1;

void rst() {
    sv = 1;
}

int nextPos() {
    return sv;
}

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

int has(Table t, char *name) {
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res != NULL;
}

ParType typeOf(Table t, char *name)
{
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res == NULL ? type_error : res->type;
}

int posOf(Table t, char *name)
{
	Entry *e, *res;
	e = malloc(sizeof(Entry));
	e->name = strdup(name);
	SGLIB_LIST_FIND_MEMBER(Entry, t, e, ST_COMPARATOR, next, res);
	free(e);
	return res == NULL ? 0 : res->pos;
}
