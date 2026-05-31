%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Flex Declarations
extern int code_line;
extern FILE *yyout;
int yylex();
int yyerror(const char *);
int no_errors = 0;

/* Error Messages Macros */
#define ERR_VAR_DECL(VAR, LINE) { fprintf(stderr, "Variable :: %s in code_line %d. ", VAR, LINE); yyerror("Var already defined"); YYERROR; }

// Code Generation and Symbol Table Headers
#include "codegen.h"
#include "symtab.h"

/* Defining the Symbol table. A simple linked list. */
ST_TABLE_TYPE symbolTable;
#include "ir.h"

%}

/* Output informative error messages (bison Option) */
%define parse.error verbose

%union {
   char *lexical;
   struct {
       ParType type;
       char *place;
   } se;
   RelationType relopIndex;
}

/* Token declarations and their respective types */
%token T_start "start"
%token T_end "end"
%token T_int_type "int"
%token T_float_type "float"
%token T_print "print"
%token T_inc "++"
%token T_eq "=="

%token <lexical> T_id
%token <lexical> T_integer
%token <lexical> T_float

%type <se> expr
%type <relopIndex> relop

%%

program: "start" T_id { gen_pre($2); symbolTable = NULL; }
         stmts "end" { gen_fin(); }
       ;

stmts: /* empty */
     | stmts stmt
     ;

stmt: '(' T_int_type T_id ')' {
         if (!addvar(&symbolTable, $3, type_integer)) {
             ERR_VAR_DECL($3, code_line);
         }
      }
    | '(' T_float_type T_id ')' {
         if (!addvar(&symbolTable, $3, type_real)) {
             ERR_VAR_DECL($3, code_line);
         }
      }
    | '(' '=' T_id expr ')' {
         gen_asn($3, $4.place, $4.type);
      }
    | '(' T_print expr ')' {
         gen_prt($3.place, $3.type);
      }
    ;

expr: T_integer {
         $$.place = gen_ldc($1, &$$.type);
      }
    | T_float {
         $$.place = gen_ldc($1, &$$.type);
      }
    | T_id {
         $$.place = gen_lod($1, &$$.type);
      }
    | '+' expr {
         if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
             $2.place = gen_push($2.type, $2.place);
         }
         $<se>$ = $2;
      } expr {
         $$.place = gen_op("+", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
      }
    | '-' expr {
         if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
             $2.place = gen_push($2.type, $2.place);
         }
         $<se>$ = $2;
      } expr {
         $$.place = gen_op("-", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
      }
    | '*' expr {
         if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
             $2.place = gen_push($2.type, $2.place);
         }
         $<se>$ = $2;
      } expr {
         $$.place = gen_op("*", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
      }
    | '/' expr {
         if (strcmp($2.place, "rax") == 0 || strcmp($2.place, "xmm0") == 0) {
             $2.place = gen_push($2.type, $2.place);
         }
         $<se>$ = $2;
      } expr {
         $$.place = gen_op("/", $<se>3.place, $<se>3.type, $4.place, $4.type, &$$.type);
      }
    | '(' expr ')' {
         $$ = $2;
      }
    | '(' T_id T_inc ')' {
         $$.place = gen_inc($2, 0, &$$.type);
      }
    | '(' T_inc T_id ')' {
         $$.place = gen_inc($3, 1, &$$.type);
      }
    | '(' relop expr {
         if (strcmp($3.place, "rax") == 0 || strcmp($3.place, "xmm0") == 0) {
             $3.place = gen_push($3.type, $3.place);
         }
         $<se>$ = $3;
      } expr '?' expr ':' expr ')' {
         $$.place = gen_cnd($2, $<se>4.place, $<se>4.type, $5.place, $5.type, $7.place, $7.type, $9.place, $9.type, &$$.type);
      }
    ;

relop: '>' { $$ = OP_GT; }
     | '<' { $$ = OP_LT; }
     | T_eq { $$ = OP_EQ; }
     ;

%%

/* The usual yyerror */
int yyerror (const char *msg)
{
    fprintf(stderr, "ERROR: %s (line %d).\n", msg, code_line);
    no_errors++;
    return 0;
}

/* The lexer... */
#include "lexer.lex.c"

/* Main */
int main(int argc, char **argv) {
    FILE *asm_file;
    ++argv, --argc;  /* skip over program name */
    if (argc > 0 && (yyin = fopen(argv[0], "r")) == NULL) {
        fprintf(stderr, "File %s NOT FOUND in current directory.\n Using stdin.\n", argv[0]);
        yyin = stdin;
    }

    // Calling the parser
    int result = yyparse();

    if (no_errors > 0) {
        fprintf(stderr, "Compilation finished with %d error(s).\n", no_errors);
    } else {
        fprintf(stderr, "Compilation finished successfully with no errors.\n");
    }
    
    // If no errors are found, output assembly
    if (no_errors == 0) {
        if (argc > 1) {
            asm_file = fopen(argv[1], "w");
        } else {
            fprintf(stderr, "No second argument defined. Output to screen.\n\n");
            asm_file = stdout;
        }
        print_int_code(asm_file);
        fclose(asm_file);
    } else {
        fprintf(stderr, "No Code Generated.\n");
    }

    return result;
}
