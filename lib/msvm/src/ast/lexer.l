%{
#include "alloc.hpp"
#include "parser.hh"
//extern int yylval;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Find the non-interactive flag
#ifdef _WIN32
#include <io.h>
#define fileno _fileno
#define isatty _isatty
#define YY_NO_UNISTD_H
#else
#define register
#endif

extern "C" int yylex(void);
%}

%option yylineno

INTATOM [0-9]+
FLTATOM {INTATOM}*[.]{INTATOM}+
VARATOM [_A-Za-z][_A-Za-z0-9]*
STRATOM ["][^\n"]*[^\\]["]

%%

[ \t\n\r]+   ; /* whitespace */
\/\/[^\n\r]* ; /* single line comments */
{STRATOM}    {   
                int nt = strlen(yytext) - 2;
                yytext[nt+1] = '\0';    // overwrite delimiter
                char* buf = vm::ast::Alloc::strdup(yytext+1, true);
                yylval.s = buf;
                return(STR_CONST);
             }
"=="         { return(yylval.op = OP_EQUAL); }
"!="         { return(yylval.op = OP_NEQUAL); }
">="         { return(yylval.op = OP_GEQUAL); }
"<="         { return(yylval.op = OP_LEQUAL); }
"and"        { return(yylval.op = OP_AND); }
"or"         { return(yylval.op = OP_OR); }
"--"         { return(yylval.op = OP_INC); }
"++"         { return(yylval.op = OP_DEC); }
"<<"         { return(yylval.op = OP_SHL); }
">>"         { return(yylval.op = OP_SHR); }
"+="         |
"-="         |
"*="         |
"/="         |
"%="         |
"&="         |
"^="         |
"|="         |
"<<="        |
">>="        { return(yylval.op = yytext[0]); }
"+"          { return(yylval.op = OP_ADD); }
"-"          { return(yylval.op = OP_SUB); }
"*"          { return(yylval.op = OP_MUL); }
"/"          { return(yylval.op = OP_DIV); }
"<"          { return(yylval.op = OP_LESS); }
">"          { return(yylval.op = OP_GREATER); }
"|"          { return(yylval.op = OP_BITOR); }
"."          { return(yylval.op = OP_ACCESS); }
"!"          { return(yylval.op = OP_NOT); }
"&"          { return(yylval.op = OP_BITAND); }
"%"          { return(yylval.op = OP_MOD); }
"^"          { return(yylval.op = OP_XOR); }
"~"          { return(yylval.op = OP_ONESC); }
"="          { return(yylval.op = OP_ASSIGN); }
"local"      { return(kwLOCAL); }
"end"        { return(kwEND); }
"if"         { return(kwIF); }
"then"       { return(kwTHEN); }
"else"       { return(kwELSE); }
"elsif"      { return(kwELSIF); }
"for"        { return(kwFOR); }
"while"      { return(kwWHILE); }
"in"         { return(kwIN); }
"to"         { return(kwTO); }
"by"         { return(kwBY); }
"break"      { return(kwBREAK); }
"continue"   { return(kwCONTINUE); }
"func"       { return(kwFUNC); }
"return"     { return(kwRETURN); }
"class"      { return(kwCLASS); }
"extends"    { return(kwEXTENDS); }
"true"       { yylval.iVal = 1; return(INT_CONST); }
"false"      { yylval.iVal = 0; return(INT_CONST); }
{VARATOM}    { yylval.s = vm::ast::Alloc::strdup(yytext); return(IDENT); }
{INTATOM}    { yylval.iVal = atoll(yytext); return(INT_CONST); }
{FLTATOM}    { yylval.fVal = atof(yytext); return(FLT_CONST); }

.           { return *yytext; }

%%

int yywrap(void)
{
    return 1;
}
