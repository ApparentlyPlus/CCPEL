/*
 * CPEL - Custom Prefix Expression Language 
 * Parser Implementation using Bison
 * Author: ApparentlyPlus
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Flex Declarations
extern int ll;
extern FILE *yyout;

// Bison Declarations
int yylex();
int yyerror(const char *);
int nerr = 0;

// Error Message Macro
#define ERR_VAR_DECL(VAR, LINE) { fprintf(stderr, "Variable :: %s in line %d. ", VAR, LINE); yyerror("Var already defined"); YYERROR; }

// Code Generation and Symbol Table Headers
#include "codegen.h"
#include "symtab.h"

// Define the symbol table
Table symbolTable;
#include "ir.h"

%}

// Output informative error messages
%define parse.error verbose

// Define the types of semantic values
%union {
   char *lexical;
   struct {
       ParType type;
       char *place;
   } se;
   RelationType relopIndex;
}

// Token declarations and their respective types
%token T_START "start"
%token T_END "end"
%token T_INT "int"
%token T_FLT "float"
%token T_PRINT "print"
%token T_INC "++"
%token T_EQ "=="

%token <lexical> T_ID
%token <lexical> T_INTVAL
%token <lexical> T_FLTVAL

%type <se> expr
%type <relopIndex> relop

%%

// A program starts with "start", followed by statements, and ends with "end"
program: "start" T_ID { pre($2); symbolTable = NULL; } stmts "end" { fin(); }
    ;

// One or more statements
stmts: /* e */
    | stmts stmt
    ;

// A statement can be a variable declaration, assignment, or print statement
stmt: '(' T_INT T_ID ')' {
        // Declare an integer variable and add it to the symbol table
        if (!add(&symbolTable, $3, type_integer)) {
            ERR_VAR_DECL($3, ll);
        }
    }
    | '(' T_FLT T_ID ')' {
        // Declare a float variable and add it to the symbol table
        if (!add(&symbolTable, $3, type_real)) {
            ERR_VAR_DECL($3, ll);
        }
    }
    | '(' '=' T_ID expr ')' {
        // Assign the value and type of an expression to a variable
        asn($3, $4.place, $4.type);
    }
    | '(' T_PRINT expr ')' {
        // Print the value of the evaluated expression
        print($3.place, $3.type);
    }
    ;

// Expressions include constants, identifiers, prefix operators, increments, and conditionals
expr: T_INTVAL {
        // load an integer literal constant
        $$.place = ldc($1, &$$.type);
    }
    | T_FLTVAL {
        // load a float literal constant
        $$.place = ldc($1, &$$.type);
    }
    | T_ID {
        // load the value of a variable identifier
        $$.place = lod($1, &$$.type);
    }
    | '+' expr {
        // prefix Addition: push left-hand side onto stack if in a primary register to prevent overwrite
        if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
            $2.place = push($2.type, $2.place);
        }
        $<se>$ = $2;
    } expr {
        // generate addition code for both operands
        $$.place = op("+", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
    }
    | '-' expr {
        // push left hand side onto stack if in a primary register to prevent overwrite
        if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
            $2.place = push($2.type, $2.place);
        }
        $<se>$ = $2;
    } expr {
        // generate subtraction code for both operands
        $$.place = op("-", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
    }
    | '*' expr {
        // push left hand side onto stack if in a primary register to prevent overwrite
        if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
            $2.place = push($2.type, $2.place);
        }
        $<se>$ = $2;
    } expr {
        // generate multiplication code for both operands
        $$.place = op("*", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
    }
    | '/' expr {
        // push left hand side onto stack if in a primary register to prevent overwrite
        if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
            $2.place = push($2.type, $2.place);
        }
        $<se>$ = $2;
    } expr {
        // generate division code for both operands
        $$.place = op("/", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
    }
    | '(' expr ')' {
        // propagate inside expression
        $$ = $2;
    }
    | '(' T_ID T_INC ')' {
        // Post increment variable
        $$.place = inc($2, 0, &$$.type);
    }
    | '(' T_INC T_ID ')' {
        // Pre-increment variable
        $$.place = inc($3, 1, &$$.type);
    }
    | '(' relop expr {
        // Ternary: push condition left hand side onto stack if in a primary register to prevent overwrite
        if (strcmp($3.place, "rax") == 0 || strcmp($3.place, "xmm0") == 0) {
            $3.place = push($3.type, $3.place);
        }
        $<se>$ = $3;
    } expr '?' expr ':' expr ')' {
        // Generate code for condition evaluation and branch selection
        $$.place = cnd($2, $<se>4.place, $<se>4.type, $5.place, $5.type, $7.place, $7.type, $9.place, $9.type, &$$.type);
    }
    ;

// Relational comparison operators
relop: '>' { $$ = OP_GT; }
    | '<' { $$ = OP_LT; }
    | T_EQ { $$ = OP_EQ; }
    ;

%%

// The usual yyerror
int yyerror (const char *msg)
{
    fprintf(stderr, "ERROR: %s (line %d).\n", msg, ll);
    nerr++;
    return 0;
}

// The lexer goes here
#include "lexer.lex.c"

// Entry point of CCPEL
int main(int argc, char **argv) {
    FILE *asm_file;
    ++argv, --argc;  // skip over program name
    if (argc > 0 && (yyin = fopen(argv[0], "r")) == NULL) {
        fprintf(stderr, "File %s NOT FOUND in current directory.\n Using stdin.\n", argv[0]);
        yyin = stdin;
    }

    // Calling the parser
    int result = yyparse();

    if (nerr > 0) {
        fprintf(stderr, "Compilation finished with %d error(s).\n", nerr);
    } else {
        fprintf(stderr, "Compilation finished successfully with no errors.\n");
    }
    
    // If no errors are found, output assembly
    if (nerr == 0) {
        if (argc > 1) {
            asm_file = fopen(argv[1], "w");
        } else {
            fprintf(stderr, "No second argument defined. Output to screen.\n\n");
            asm_file = stdout;
        }
        printIntCode(asm_file);
        fclose(asm_file);
    } else {
        fprintf(stderr, "No Code Generated.\n");
    }

    return result;
}
