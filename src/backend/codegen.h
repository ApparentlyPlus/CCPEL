/*
 * CCPEL - Code Generation Header
 * Author: ApparentlyPlus
 */

#ifndef _CODEGEN_H
#define _CODEGEN_H

#include <stdio.h>

typedef enum {
    type_error, 
    type_integer, 
    type_real, 
    type_boolean, 
    type_void
} ParType;
              
typedef enum {OP_GT, OP_LT, OP_EQ} RelationType;

// Core functions
void pre(char *prog);
void fin();
char *tmp(int idx);
char *push(ParType type, char *place);

// x86 Proper Instruction Generation Helpers
char *ldc(char *val, ParType *t);
char *lod(char *name, ParType *t);
void asn(char *name, char *place, ParType t);
void print(char *place, ParType t);

char *op(char *o, char *p1, ParType t1, char *p2, ParType t2, ParType *out_t);
char *inc(char *name, int pre_inc, ParType *out_t);
char *cnd(RelationType rel, char *p1, ParType t1, char *p2, ParType t2, char *p3, ParType t3, char *p4, ParType t4, ParType *out_t);

void ins(char *s);

#endif
