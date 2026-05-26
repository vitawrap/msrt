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
"true"       { yylval.iVal = 1; return(INT_CONST); }
"false"      { yylval.iVal = 0; return(INT_CONST); }
{VARATOM}    { yylval.s = vm::ast::Alloc::strdup(yytext); return(IDENT); }
{STRATOM}    {   
                int nt = strlen(yytext) - 2;
                yytext[nt+1] = '\0';    // overwrite delimiter
                char* buf = vm::ast::Alloc::strdup(yytext+1, true);
                yylval.s = buf;
                return(STR_CONST);
             }

.           { return *yytext; }

%%

int yywrap(void)
{
    return 1;
}
