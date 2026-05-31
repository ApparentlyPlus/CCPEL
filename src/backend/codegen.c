/*
 * CCPEL - Code Generation Implementation
 * Author: ApparentlyPlus
 */

#include "codegen.h"
#include "ir.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Externs for bison integration
extern FILE *yyout;
extern int yyerror(const char *);
extern Table symbolTable;

// float constants array
static char **fc = NULL; 

// count of float constants
static int fcc = 0; 

// capacity of float constants array
static int fcap = 0;

// jump label counter
static int lbl = 1;

// Returns format string for variable stack offset
char *tmp(int idx)
{
    char buf[64];
    sprintf(buf, "[rbp - %d]", idx * 8);
    return strdup(buf);
}

// Pushes volatile register values onto stack
char *push(ParType type, char *place)
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

// Emits conversion sequence for integer to double
static void cvt(char *dest, char *src)
{
    char buf[256];
    if (src[0] != '[' && strcmp(src, "rax") != 0) {
        sprintf(buf, "    mov rax, %s", src);
        addInstruction(buf, NO_LABEL);
        sprintf(buf, "    cvtsi2sd %s, rax", dest);
        addInstruction(buf, NO_LABEL);
    } else {
        sprintf(buf, "    cvtsi2sd %s, %s", dest, src);
        addInstruction(buf, NO_LABEL);
    }
}

// Allocates new unique label number
int Label()
{
    return lbl++;
}

// Inserts assembly label string into instruction stream
void insertLabel(int l)
{
    char buf[256];
    sprintf(buf, "_lbl_%d:", l);
    addInstruction(buf, NO_LABEL);
}

// Emits boilerplate prologue and resets compilation state
void pre(char *prog)
{
    rst();
    lbl = 1;

    if (fc != NULL) {
        for (int i = 0; i < fcc; i++) free(fc[i]);
        free(fc);
        fc = NULL;
    }
    fcc = fcap = 0;

    char comment[256];
    sprintf(comment, "; Assembly transpiled for program %s (Transpiler written by u/ApparentlyPlus)", prog);
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

// Resolves/patches dynamic stack size, prints dynamic float data, and emits epilogue
void fin()
{
    addInstruction("    mov rsp, rbp", NO_LABEL);
    addInstruction("    pop rbp", NO_LABEL);
    addInstruction("    xor rax, rax", NO_LABEL);
    addInstruction("    ret\n", NO_LABEL);
    
    if (fcc > 0) {
        addInstruction("section .data", NO_LABEL);
        for (int i = 0; i < fcc; i++) {
            char buf[256];
            sprintf(buf, "flt_const_%d: dq %s", i, fc[i]);
            addInstruction(strdup(buf), NO_LABEL);
        }
        addInstruction("", NO_LABEL);
    }
    
    addInstruction("section .note.GNU-stack noalloc noexec nowrite progbits", NO_LABEL);

    int n = nextPos() - 1;
    int bytes = n * 8;
    int sz = ((bytes + 15) / 16) * 16;
    if (sz < 16) sz = 16;
    
    // Patch the stack size placeholder in the instruction stream with the actual calculated size
    extern InstrTable INTERCODE;
    Instr *curr = INTERCODE;
    while (curr != NULL) {
        if (curr->text && strstr(curr->text, "%STACK_SIZE%") != NULL) {
            char buf[128];
            sprintf(buf, "    sub rsp, %d", sz);
            free(curr->text);
            curr->text = strdup(buf);
        }
        curr = curr->next;
    }
}

// Allocates double literal dynamically or returns integer immediate text
char *ldc(char *val, ParType *t)
{
    if (strchr(val, '.') != NULL) {
        *t = type_real;
        char buf[64];
        if (fcc >= fcap) {
            fcap = fcap == 0 ? 16 : fcap * 2;
            fc = realloc(fc, fcap * sizeof(char *));
        }
        fc[fcc] = strdup(val);
        sprintf(buf, "[flt_const_%d]", fcc);
        fcc++;
        return strdup(buf);
    } else {
        *t = type_integer;
        return strdup(val);
    }
}

// Resolves local variable types and stack offsets
char *lod(char *name, ParType *t)
{
    if (!has(symbolTable, name)) {
        char err[128];
        sprintf(err, "Variable %s NOT declared", name);
        yyerror(err);
        exit(1);
    }
    *t = typeOf(symbolTable, name);
    return tmp(posOf(symbolTable, name));
}

// Stores parsed expressions into local variables with automatic coercion
void asn(char *name, char *place, ParType t)
{
    if (!has(symbolTable, name)) {
        char err[128];
        sprintf(err, "Variable %s NOT declared", name);
        yyerror(err);
        exit(1);
    }
    ParType vt = typeOf(symbolTable, name);
    int pos = posOf(symbolTable, name);
    char buf[256];
    
    if (vt == type_integer) {
        if (t == type_integer) {
            if (strcmp(place, "rax") != 0) {
                sprintf(buf, "    mov rax, %s", place);
                addInstruction(buf, NO_LABEL);
            }
            sprintf(buf, "    mov [rbp - %d], rax", pos * 8);
            addInstruction(buf, NO_LABEL);
        } else if (t == type_real) {
            sprintf(buf, "    cvttsd2si rax, %s", place);
            addInstruction(buf, NO_LABEL);
            sprintf(buf, "    mov [rbp - %d], rax", pos * 8);
            addInstruction(buf, NO_LABEL);
        }
    } else if (vt == type_real) {
        if (t == type_real) {
            if (strcmp(place, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", place);
                addInstruction(buf, NO_LABEL);
            }
            sprintf(buf, "    movsd [rbp - %d], xmm0", pos * 8);
            addInstruction(buf, NO_LABEL);
        } else if (t == type_integer) {
            cvt("xmm0", place);
            sprintf(buf, "    movsd [rbp - %d], xmm0", pos * 8);
            addInstruction(buf, NO_LABEL);
        }
    }
}

// Calls external printf function with correct type formatting
void print(char *place, ParType t)
{
    char buf[256];
    if (t == type_integer) {
        if (strcmp(place, "rax") != 0) {
            sprintf(buf, "    mov rsi, %s", place);
            addInstruction(buf, NO_LABEL);
        } else {
            addInstruction("    mov rsi, rax", NO_LABEL);
        }
        addInstruction("    mov rdi, fmt_int", NO_LABEL);
        addInstruction("    xor al, al", NO_LABEL);
        addInstruction("    call printf", NO_LABEL);
    } else if (t == type_real) {
        if (strcmp(place, "xmm0") != 0) {
            sprintf(buf, "    movsd xmm0, %s", place);
            addInstruction(buf, NO_LABEL);
        }
        addInstruction("    mov rdi, fmt_float", NO_LABEL);
        addInstruction("    mov al, 1", NO_LABEL);
        addInstruction("    call printf", NO_LABEL);
    }
}

// Executes basic binary operations (+, -, *, /) with implicit type promotions
char *op(char *o, char *p1, ParType t1, char *p2, ParType t2, ParType *out_t)
{
    char buf[256];
    
    if (t1 == type_integer && t2 == type_integer) {
        *out_t = type_integer;
        
        if (strcmp(p1, "%stack%") == 0) {
            addInstruction("    pop rbx", NO_LABEL);
            p1 = "rbx";
        }
        
        if (strcmp(p2, "rax") == 0) {
            addInstruction("    mov rdx, rax", NO_LABEL);
            p2 = "rdx";
        }
        
        if (strcmp(p1, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", p1);
            addInstruction(buf, NO_LABEL);
        }
        
        if (strcmp(o, "+") == 0) {
            sprintf(buf, "    add rax, %s", p2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(o, "-") == 0) {
            sprintf(buf, "    sub rax, %s", p2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(o, "*") == 0) {
            sprintf(buf, "    imul rax, %s", p2);
            addInstruction(buf, NO_LABEL);
        } else if (strcmp(o, "/") == 0) {
            sprintf(buf, "    mov rbx, %s", p2);
            addInstruction(buf, NO_LABEL);
            addInstruction("    cqo", NO_LABEL);
            addInstruction("    idiv rbx", NO_LABEL);
        }
        return "rax";
        
    } else {
        *out_t = type_real;
        
        if (strcmp(p1, "%stack%") == 0) {
            addInstruction("    movsd xmm1, [rsp]", NO_LABEL);
            addInstruction("    add rsp, 8", NO_LABEL);
            p1 = "xmm1";
        }
        
        if (strcmp(p2, "xmm0") == 0) {
            addInstruction("    movsd xmm2, xmm0", NO_LABEL);
            p2 = "xmm2";
        }
        
        if (t1 == type_real) {
            if (strcmp(p1, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", p1);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            cvt("xmm0", p1);
        }
        
        if (strcmp(p2, "xmm2") == 0) {
            addInstruction("    movsd xmm1, xmm2", NO_LABEL);
            p2 = "xmm1";
        } else {
            if (t2 == type_real) {
                sprintf(buf, "    movsd xmm1, %s", p2);
                addInstruction(buf, NO_LABEL);
            } else {
                cvt("xmm1", p2);
            }
        }
        
        if (strcmp(o, "+") == 0) {
            addInstruction("    addsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(o, "-") == 0) {
            addInstruction("    subsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(o, "*") == 0) {
            addInstruction("    mulsd xmm0, xmm1", NO_LABEL);
        } else if (strcmp(o, "/") == 0) {
            addInstruction("    divsd xmm0, xmm1", NO_LABEL);
        }
        
        return "xmm0";
    }
}

// Evaluates integer pre- and post-increment expressions directly in memory
char *inc(char *name, int pre_inc, ParType *out_t)
{
    if (!has(symbolTable, name)) {
        char err[128];
        sprintf(err, "Variable %s NOT declared", name);
        yyerror(err);
        exit(1);
    }
    
    ParType t = typeOf(symbolTable, name);
    int pos = posOf(symbolTable, name);
    char buf[256];
    
    if (t != type_integer) {
        yyerror("Semantic Error: Increment operator is supported for integers only.");
        exit(1);
    }
    
    *out_t = type_integer;
    
    if (pre_inc) {
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

// Evaluates ternary conditional branches with dynamic coercion directly in registers
char *cnd(RelationType rel, char *p1, ParType t1, char *p2, ParType t2,
          char *p3, ParType t3, char *p4, ParType t4, ParType *out_t)
{
    int t_lbl = Label();
    int f_lbl = Label();
    int e_lbl = Label();
    char buf[256];
    
    if (t1 != type_integer || t2 != type_integer) {
        yyerror("Semantic Error: Comparisons are supported for integers only.");
        exit(1);
    }
    
    if (strcmp(p1, "%stack%") == 0) {
        addInstruction("    pop rbx", NO_LABEL);
        p1 = "rbx";
    }
    
    if (strcmp(p1, "rax") != 0) {
        sprintf(buf, "    mov rax, %s", p1);
        addInstruction(buf, NO_LABEL);
    }
    sprintf(buf, "    cmp rax, %s", p2);
    addInstruction(buf, NO_LABEL);
    
    switch (rel) {
        case OP_GT: sprintf(buf, "    jg _lbl_%d", t_lbl); break;
        case OP_LT: sprintf(buf, "    jl _lbl_%d", t_lbl); break;
        case OP_EQ: sprintf(buf, "    je _lbl_%d", t_lbl); break;
    }
    addInstruction(buf, NO_LABEL);
    sprintf(buf, "    jmp _lbl_%d", f_lbl);
    addInstruction(buf, NO_LABEL);
    
    if (t3 == type_integer && t4 == type_integer) {
        *out_t = type_integer;
    } else {
        *out_t = type_real;
    }
    
    insertLabel(t_lbl);
    if (*out_t == type_integer) {
        if (strcmp(p3, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", p3);
            addInstruction(buf, NO_LABEL);
        }
    } else {
        if (t3 == type_real) {
            if (strcmp(p3, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", p3);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            cvt("xmm0", p3);
        }
    }
    sprintf(buf, "    jmp _lbl_%d", e_lbl);
    addInstruction(buf, NO_LABEL);
    
    insertLabel(f_lbl);
    if (*out_t == type_integer) {
        if (strcmp(p4, "rax") != 0) {
            sprintf(buf, "    mov rax, %s", p4);
            addInstruction(buf, NO_LABEL);
        }
    } else {
        if (t4 == type_real) {
            if (strcmp(p4, "xmm0") != 0) {
                sprintf(buf, "    movsd xmm0, %s", p4);
                addInstruction(buf, NO_LABEL);
            }
        } else {
            cvt("xmm0", p4);
        }
    }
    
    insertLabel(e_lbl);
    
    return *out_t == type_integer ? "rax" : "xmm0";
}

// Writes raw custom instruction string into the IR stream
void ins(char *s)
{
    addInstruction(s, NO_LABEL);
}
