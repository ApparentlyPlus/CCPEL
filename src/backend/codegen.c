#include "codegen.h"
#include "ir.h"
#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyout;
extern int yyerror(const char *);
extern ST_TABLE_TYPE symbolTable;

static char **float_constants = NULL;
static int float_constant_count = 0;
static int float_constant_capacity = 0;

static int current_label = 1;

char *gen_tmp(int temp_idx)
{
    char buf[64];
    sprintf(buf, "[rbp - %d]", temp_idx * 8);
    return strdup(buf);
}

char *gen_push(ParType type, char *place)
{
    char buf[256];
    if (type == type_integer) {
        if (strcmp(place, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", place);
            addInstruction(buf, NO_LABEL);
        }
        addInstruction("    push rax", NO_LABEL);
    } else if (type == type_real) {
        if (strcmp(place, "xmm0") != 0) {
            sprintf(buf, "    movsd xmm0, %s", place);
            addInstruction(buf, NO_LABEL);
        }
        addInstruction("    sub rsp, 8", NO_LABEL);
        addInstruction("    movsd [rsp], xmm0", NO_LABEL);
    }
    return "%stack%";
}

static void gen_cvtsi2sd(char *dest_xmm, char *src_place)
{
    char buf[256];
    if (src_place[0] != '[' && strcmp(src_place, "rax") != 0) {
        sprintf(buf, "    mov rax, %s", src_place);
        addInstruction(buf, NO_LABEL);
        sprintf(buf, "    cvtsi2sd %s, rax", dest_xmm);
        addInstruction(buf, NO_LABEL);
    } else {
        sprintf(buf, "    cvtsi2sd %s, %s", dest_xmm, src_place);
        addInstruction(buf, NO_LABEL);
    }
}

int Label()
{
    return current_label++;
}

void insertLabel(int l)
{
    char instruction[256];
    sprintf(instruction, "_lbl_%d:", l);
    addInstruction(instruction, NO_LABEL);
}

/* Preamble generation for 64-bit x86_64 */
void gen_pre(char *progName)
{
    reset_symtab();
    current_label = 1;

    if (float_constants != NULL) {
        for (int i = 0; i < float_constant_count; i++) {
            free(float_constants[i]);
        }
        free(float_constants);
        float_constants = NULL;
    }
    float_constant_count = 0;
    float_constant_capacity = 0;

    char comment[256];
    sprintf(comment, "; Assembly transpiled for program %s (Transpiler written by u/ApparentlyPlus)", progName);
    addInstruction(comment, NO_LABEL);
    
    addInstruction("global main", NO_LABEL);
    addInstruction("extern printf", NO_LABEL);
    addInstruction("", NO_LABEL);
    
    addInstruction("section .data", NO_LABEL);
    addInstruction("    fmt_int: db \"%d\", 10, 0", NO_LABEL);
    addInstruction("    fmt_float: db \"%.2f\", 10, 0", NO_LABEL);
    addInstruction("", NO_LABEL);
    
    addInstruction("section .text", NO_LABEL);
    addInstruction("main:", NO_LABEL);
    addInstruction("    push rbp", NO_LABEL);
    addInstruction("    mov rbp, rsp", NO_LABEL);
    addInstruction("    sub rsp, %STACK_SIZE%", NO_LABEL);
}

void gen_fin()
{
    // Return path
    addInstruction("    mov rsp, rbp", NO_LABEL);
    addInstruction("    pop rbp", NO_LABEL);
    addInstruction("    xor rax, rax", NO_LABEL);
    addInstruction("    ret\n", NO_LABEL);
    
    // Dynamic data segment for floating point constants
    if (float_constant_count > 0) {
        addInstruction("section .data", NO_LABEL);
        for (int i = 0; i < float_constant_count; i++) {
            char buf[256];
            sprintf(buf, "flt_const_%d: dq %s", i, float_constants[i]);
            addInstruction(strdup(buf), NO_LABEL);
        }
        addInstruction("", NO_LABEL);
    }
    
    addInstruction("section .note.GNU-stack noalloc noexec nowrite progbits", NO_LABEL);

    // Resolve stack size and patch %STACK_SIZE%
    int num_vars = get_current_stack_value() - 1;
    int stack_bytes = num_vars * 8;
    int aligned_size = ((stack_bytes + 15) / 16) * 16;
    if (aligned_size < 16) {
        aligned_size = 16; // Minimum 16-byte stack frame
    }
    
    extern INSTRUCTION_TABLE_TYPE INTERCODE;
    INSTRUCTION_TYPE *curr = INTERCODE;
    while (curr != NULL) {
        if (curr->text && strstr(curr->text, "%STACK_SIZE%") != NULL) {
            char new_text[128];
            sprintf(new_text, "    sub rsp, %d", aligned_size);
            free(curr->text);
            curr->text = strdup(new_text);
        }
        curr = curr->next_i;
    }
}

/* Load Literal - Lazy evaluation */
char *gen_ldc(char *value, ParType *out_type)
{
    if (strchr(value, '.') != NULL) {
        *out_type = type_real;
        char buf[64];
        if (float_constant_count >= float_constant_capacity) {
            float_constant_capacity = float_constant_capacity == 0 ? 16 : float_constant_capacity * 2;
            float_constants = realloc(float_constants, float_constant_capacity * sizeof(char *));
        }
        float_constants[float_constant_count] = strdup(value);
        sprintf(buf, "[flt_const_%d]", float_constant_count);
        float_constant_count++;
        return strdup(buf);
    } else {
        *out_type = type_integer;
        return strdup(value);
    }
}

/* Load Local Variable - Lazy evaluation */
char *gen_lod(char *name, ParType *out_type)
{
    if (!lookup(symbolTable, name)) {
        char err_msg[128];
        sprintf(err_msg, "Variable %s NOT declared", name);
        yyerror(err_msg);
        exit(1);
    }
    
    *out_type = lookup_type(symbolTable, name);
    int pos = lookup_position(symbolTable, name);
    return gen_tmp(pos);
}

/* Store expression into variable with coercion & redundancy filtering */
void gen_asn(char *name, char *place, ParType expr_type)
{
    if (!lookup(symbolTable, name)) {
        char err_msg[128];
        sprintf(err_msg, "Variable %s NOT declared", name);
        yyerror(err_msg);
        exit(1);
    }
    
    ParType var_type = lookup_type(symbolTable, name);
    int pos = lookup_position(symbolTable, name);
    char instruction[256];
    
    if (var_type == type_integer) {
        if (expr_type == type_integer) {
            if (strcmp(place, "rax") != 0) {
                sprintf(instruction, "    mov rax, %s", place);
                addInstruction(instruction, NO_LABEL);
            }
            sprintf(instruction, "    mov [rbp - %d], rax", pos * 8);
            addInstruction(instruction, NO_LABEL);
        } else if (expr_type == type_real) {
            // Coerce Real -> Integer
            sprintf(instruction, "    cvttsd2si rax, %s", place);
            addInstruction(instruction, NO_LABEL);
            sprintf(instruction, "    mov [rbp - %d], rax", pos * 8);
            addInstruction(instruction, NO_LABEL);
        }
    } else if (var_type == type_real) {
        if (expr_type == type_real) {
            if (strcmp(place, "xmm0") != 0) {
                sprintf(instruction, "    movsd xmm0, %s", place);
                addInstruction(instruction, NO_LABEL);
            }
            sprintf(instruction, "    movsd [rbp - %d], xmm0", pos * 8);
            addInstruction(instruction, NO_LABEL);
        } else if (expr_type == type_integer) {
            // Coerce Integer -> Real
            gen_cvtsi2sd("xmm0", place);
            sprintf(instruction, "    movsd [rbp - %d], xmm0", pos * 8);
            addInstruction(instruction, NO_LABEL);
        }
    }
}

/* Output values to terminal using printf */
void gen_prt(char *place, ParType expr_type)
{
    char instruction[256];
    if (expr_type == type_integer) {
        if (strcmp(place, "rax") != 0) {
            sprintf(instruction, "    mov rsi, %s", place);
            addInstruction(instruction, NO_LABEL);
        } else {
            addInstruction("    mov rsi, rax", NO_LABEL);
        }
        addInstruction("    mov rdi, fmt_int", NO_LABEL);
        addInstruction("    xor al, al", NO_LABEL);
        addInstruction("    call printf", NO_LABEL);
    } else if (expr_type == type_real) {
        if (strcmp(place, "xmm0") != 0) {
            sprintf(instruction, "    movsd xmm0, %s", place);
            addInstruction(instruction, NO_LABEL);
        }
        addInstruction("    mov rdi, fmt_float", NO_LABEL);
        addInstruction("    mov al, 1", NO_LABEL);
        addInstruction("    call printf", NO_LABEL);
    }
}

/* Binary Operations with Type Coercion & Redundancy Filtering */
char *gen_op(char *op, char *place1, ParType type1, char *place2, ParType type2, ParType *out_type)
{
    char buf[256];
    
    if (type1 == type_integer && type2 == type_integer) {
        *out_type = type_integer;
        
        if (strcmp(place1, "%stack%") == 0) {
            addInstruction("    pop rbx", NO_LABEL);
            place1 = "rbx";
        }
        
        // If RHS is in rax, copy it to rdx first to avoid being overwritten by LHS load
        if (strcmp(place2, "rax") == 0) {
            addInstruction("    mov rdx, rax", NO_LABEL);
            place2 = "rdx";
        }
        
        if (strcmp(place1, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", place1);
            addInstruction(buf, NO_LABEL);
        }
        
        if (strcmp(op, "+") == 0) {
            sprintf(buf, "    add rax, %s", place2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(op, "-") == 0) {
            sprintf(buf, "    sub rax, %s", place2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(op, "*") == 0) {
            sprintf(buf, "    imul rax, %s", place2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(op, "/") == 0) {
            sprintf(buf, "    mov rbx, %s", place2);
            addInstruction(buf, NO_LABEL);
            addInstruction("    cqo", NO_LABEL);
            addInstruction("    idiv rbx", NO_LABEL);
        }
        return "rax";
        
    } else {
        *out_type = type_real;
        
        if (strcmp(place1, "%stack%") == 0) {
            addInstruction("    movsd xmm1, [rsp]", NO_LABEL);
            addInstruction("    add rsp, 8", NO_LABEL);
            place1 = "xmm1";
        }
        
        // If RHS is in xmm0, copy it to xmm2 first to avoid being overwritten by LHS load
        if (strcmp(place2, "xmm0") == 0) {
            addInstruction("    movsd xmm2, xmm0", NO_LABEL);
            place2 = "xmm2";
        }
        
        // Load LHS
        if (type1 == type_real) {
            if (strcmp(place1, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", place1);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            gen_cvtsi2sd("xmm0", place1);
        }
        
        // Load RHS
        if (strcmp(place2, "xmm2") == 0) {
            addInstruction("    movsd xmm1, xmm2", NO_LABEL);
            place2 = "xmm1";
        } else {
            if (type2 == type_real) {
                sprintf(buf, "    movsd xmm1, %s", place2);
                addInstruction(buf, NO_LABEL);
            } else {
                gen_cvtsi2sd("xmm1", place2);
            }
        }
        
        // Execute operation
        if (strcmp(op, "+") == 0) {
            addInstruction("    addsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(op, "-") == 0) {
            addInstruction("    subsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(op, "*") == 0) {
            addInstruction("    mulsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(op, "/") == 0) {
            addInstruction("    divsd xmm0, xmm1", NO_LABEL);
        }
        
        return "xmm0";
    }
}

/* Integer Pre/Post-Increment */
char *gen_inc(char *name, int is_pre, ParType *out_type)
{
    if (!lookup(symbolTable, name)) {
        char err_msg[128];
        sprintf(err_msg, "Variable %s NOT declared", name);
        yyerror(err_msg);
        exit(1);
    }
    
    ParType type = lookup_type(symbolTable, name);
    int pos = lookup_position(symbolTable, name);
    char buf[256];
    
    if (type != type_integer) {
        yyerror("Semantic Error: Increment operator is supported for integers only.");
        exit(1);
    }
    
    *out_type = type_integer;
    
    if (is_pre) {
        sprintf(buf, "    add qword [rbp - %d], 1", pos * 8);
        addInstruction(buf, NO_LABEL);
        sprintf(buf, "    mov rax, [rbp - %d]", pos * 8);
        addInstruction(buf, NO_LABEL);
    } else {
        sprintf(buf, "    mov rax, [rbp - %d]", pos * 8);
        addInstruction(buf, NO_LABEL);
        sprintf(buf, "    add qword [rbp - %d], 1", pos * 8);
        addInstruction(buf, NO_LABEL);
    }
    
    return "rax";
}

/* Ternary Conditional Expressions */
char *gen_cnd(RelationType rel, char *place1, ParType type1, char *place2, ParType type2,
              char *place3, ParType type3, char *place4, ParType type4, ParType *out_type)
{
    int true_lbl = Label();
    int false_lbl = Label();
    int end_lbl = Label();
    char buf[256];
    
    if (type1 != type_integer || type2 != type_integer) {
        yyerror("Semantic Error: Comparisons are supported for integers only.");
        exit(1);
    }
    
    if (strcmp(place1, "%stack%") == 0) {
        addInstruction("    pop rbx", NO_LABEL);
        place1 = "rbx";
    }
    
    // Perform comparison
    if (strcmp(place1, "rax") != 0) {
        sprintf(buf, "    mov rax, %s", place1);
        addInstruction(buf, NO_LABEL);
    }
    sprintf(buf, "    cmp rax, %s", place2);
    addInstruction(buf, NO_LABEL);
    
    // Jump based on relation
    switch (rel) {
        case OP_GT: sprintf(buf, "    jg _lbl_%d", true_lbl); break;
        case OP_LT: sprintf(buf, "    jl _lbl_%d", true_lbl); break;
        case OP_EQ: sprintf(buf, "    je _lbl_%d", true_lbl); break;
    }
    addInstruction(buf, NO_LABEL);
    sprintf(buf, "    jmp _lbl_%d", false_lbl);
    addInstruction(buf, NO_LABEL);
    
    // Coercion check
    if (type3 == type_integer && type4 == type_integer) {
        *out_type = type_integer;
    } else {
        *out_type = type_real;
    }
    
    // True block
    insertLabel(true_lbl);
    if (*out_type == type_integer) {
        if (strcmp(place3, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", place3);
            addInstruction(buf, NO_LABEL);
        }
    } else {
        if (type3 == type_real) {
            if (strcmp(place3, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", place3);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            gen_cvtsi2sd("xmm0", place3);
        }
    }
    sprintf(buf, "    jmp _lbl_%d", end_lbl);
    addInstruction(buf, NO_LABEL);
    
    // False block
    insertLabel(false_lbl);
    if (*out_type == type_integer) {
        if (strcmp(place4, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", place4);
            addInstruction(buf, NO_LABEL);
        }
    } else {
        if (type4 == type_real) {
            if (strcmp(place4, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", place4);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            gen_cvtsi2sd("xmm0", place4);
        }
    }
    
    // End block
    insertLabel(end_lbl);
    
    if (*out_type == type_integer) {
        return "rax";
    } else {
        return "xmm0";
    }
}

void gen_ins(char *instruction)
{
    addInstruction(instruction, NO_LABEL);
}
