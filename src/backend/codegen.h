/* File: codegen.h
   Header file that provides definitions and functions for proper 64-bit x86_64 register code generation.
*/
#ifndef _CODEGEN_H
#define _CODEGEN_H

#include <stdio.h>

/* Definition of the supported types */
typedef enum {
    type_error, 
    type_integer, 
    type_real, 
    type_boolean, 
    type_void
} ParType;
              
// Relational operators
typedef enum {OP_GT, OP_LT, OP_EQ} RelationType;

// Core functions
void gen_pre(char *progName);
void gen_fin();
void gen_rst();
int gen_nxt();
char *gen_tmp(int temp_idx);

// x86 Proper Instruction Generation Helpers
char *gen_ldc(char *value, ParType *out_type);
char *gen_lod(char *name, ParType *out_type);
void gen_sav(ParType expType, int tempPosition);
void gen_asn(char *name, char *place, ParType expr_type);
void gen_prt(char *place, ParType expr_type);

char *gen_op(char *op, char *place1, ParType type1, char *place2, ParType type2, ParType *out_type);
char *gen_inc(char *name, int is_pre, ParType *out_type);
char *gen_cnd(RelationType rel, char *place1, ParType type1, char *place2, ParType type2,
              char *place3, ParType type3, char *place4, ParType type4, ParType *out_type);

void gen_ins(char *instruction);

#endif
