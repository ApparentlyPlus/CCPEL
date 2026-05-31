/* SYMBOL TABLE CODE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"
#include "codegen.h"

/* Adding a Variable entry to the symbol table.
 Returns TRUE (1) if the variable is successfully added
 and FALSE (0) if the variable was already in the symbol table. */
int addvar(ST_TABLE_TYPE *symbol_table, char *VariableName,ParType TypeDecl)
{
  // Start numbering Variable from 1
  // Static means it value survives successive calls.
  static int current_stack_value = 1;

	ST_ENTRY_TYPE *newVar;
	if (!lookup(*symbol_table,VariableName))
		{
		newVar = malloc(sizeof(ST_ENTRY_TYPE));
		newVar->varname = VariableName;
		newVar->vartype = TypeDecl;
		newVar->position = current_stack_value;
		SGLIB_LIST_ADD(ST_ENTRY_TYPE, *symbol_table, newVar, next_st_var);
		current_stack_value++;
		return 1;
		}
	else return 0; /* error variable already in Table. */
}

/* Looking up a symbol (variable) in the symbol table. Returns 0 if symbol was not found. */
int lookup(ST_TABLE_TYPE symbol_table, char *VariableName){
	ST_ENTRY_TYPE *var, *result;
	var = malloc(sizeof(ST_ENTRY_TYPE));
	var->varname = strdup(VariableName);
	SGLIB_LIST_FIND_MEMBER(ST_ENTRY_TYPE,symbol_table,var,ST_COMPARATOR,next_st_var, result);
	free(var);
   if (result == NULL) {return 0;}
   else {return 1;}
}

/* Looking up a symbol type in the symbol table. Returns 0 if symbol was not found.
type_error is 0 since it is the first symbol in the num type.*/

ParType lookup_type(ST_TABLE_TYPE symbol_table, char *VariableName)
{
	ST_ENTRY_TYPE *var, *result;
	var = malloc(sizeof(ST_ENTRY_TYPE));
	var->varname = strdup(VariableName);
	SGLIB_LIST_FIND_MEMBER(ST_ENTRY_TYPE,symbol_table,var,ST_COMPARATOR,next_st_var, result);
	free(var);
   if (result == NULL) {return type_error;}
   else {return result->vartype;}
}

/* Looking up a poisition of a variable in the Frame in the symbol table.
   Returns 0 if the variable was not found, otherwise returns the position. */
int lookup_position(ST_TABLE_TYPE symbol_table, char *VariableName)
{
	ST_ENTRY_TYPE *var, *result;
	var = malloc(sizeof(ST_ENTRY_TYPE));
	var->varname = strdup(VariableName);
	SGLIB_LIST_FIND_MEMBER(ST_ENTRY_TYPE,symbol_table,var,ST_COMPARATOR,next_st_var, result);
	free(var);
   	if (result == NULL) {return 0;}
   	else {return result->position;}
}

/* end of function declarations for variable management */
