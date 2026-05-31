/*
Grammar with semantics and type checking.
Lexical Analyser
*/

%{
#include <stdlib.h>
#include <string.h>
int code_line = 1;
%}

%option yylineno
%option noyywrap
%option noinput
%option nounput

DIGITS [0-9]
LETTER [a-zA-Z]
VARIABLE {LETTER}({LETTER}|{DIGITS}|_)*
FLOAT {DIGITS}+\.{DIGITS}+
INTEGER {DIGITS}+
newline \n|\x0A|\x0D\x0A

%%

"start"         { return T_start; }
"end"           { return T_end; }
"int"           { return T_int_type; }
"float"         { return T_float_type; }
"print"         { return T_print; }

"++"            { return T_inc; }
"=="            { return T_eq; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"="             { return '='; }
"?"             { return '?'; }
":"             { return ':'; }
">"             { return '>'; }
"<"             { return '<'; }

"("             { return '('; }
")"             { return ')'; }

{VARIABLE}      { yylval.lexical = strdup(yytext); return T_id; }
{FLOAT}         { yylval.lexical = strdup(yytext); return T_float; }
{INTEGER}       { yylval.lexical = strdup(yytext); return T_integer; }

{newline}       { code_line++; }
[ \t]           { /* nothing */ }
.               { printf("Lexical Analysis: Unexpected String! :: (%s) in line %d. \n", yytext, code_line); }

%%
