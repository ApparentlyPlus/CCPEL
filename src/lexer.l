/*
 * CPEL - Custom Prefix Expression Language 
 * Lexer Implementation using Flex
 * Author: ApparentlyPlus
 */

%{
#include <stdlib.h>
#include <string.h>
int ll = 1;
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

"start"         { return T_START; }
"end"           { return T_END; }
"int"           { return T_INT; }
"float"         { return T_FLT; }
"print"         { return T_PRINT; }

"++"            { return T_INC; }
"=="            { return T_EQ; }
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

{VARIABLE}      { yylval.lexical = strdup(yytext); return T_ID; }
{FLOAT}         { yylval.lexical = strdup(yytext); return T_FLTVAL; }
{INTEGER}       { yylval.lexical = strdup(yytext); return T_INTVAL; }

{newline}       { ll++; }
[ \t]           { /* nothing */ }
.               { printf("Lexical Analysis: Unexpected String! :: (%s) in line %d. \n", yytext, ll); }

%%
