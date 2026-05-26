%{
#include "lexer.h"
#include "parser.hh"
#include <stdio.h>

extern int yyerror(void* root, const char * err);
%}

%define parse.trace
%define parse.error verbose

%parse-param { void* program }

%union {
    double fVal;
    long long iVal;
    char const* s;
    int op;
}

%token kwASSIGN
%token kwEND kwIF kwTHEN kwELSE kwELSIF kwFOR kwWHILE kwIN kwTO kwBREAK kwCONTINUE kwFUNC kwRETURN kwCLASS kwEXTENDS
%left <op> OP_ADD '-' 
%left <op> OP_MUL OP_DIV OP_LESS OP_GREATER OP_LEQUAL OP_GEQUAL OP_EQUAL OP_NEQUAL OP_AND OP_MOD OP_OR

%left OP_ACCESS

%token <fVal> FLT_CONST
%token <iVal> INT_CONST
%token <s> IDENT STR_CONST

%start PROGRAM

%%

PROGRAM: STMTLIST {}

STMTLIST
    : STMT STMTLIST {}
    |               {}
    ;

STMT
    : IFBLOCK       {}
    | WHILEBLOCK    {}
    | FORBLOCK      {}
    | RETURN        {}
    | FLOWNODE      {}
    | EXPRSTMT      {}
    ;

FLOWNODE
    : kwBREAK       {}
    | kwCONTINUE    {}
    ;

IFBLOCK
    : kwIF EXPR kwTHEN ELSIFBLOCK kwEND                 {}
    | kwIF EXPR kwTHEN ELSIFBLOCK kwELSE STMTLIST kwEND {}
    ;

ELSIFBLOCK
    : STMTLIST                                  {}
    | STMTLIST kwELSIF EXPR kwTHEN ELSIFBLOCK   {}
    ;

WHILEBLOCK
    : kwWHILE EXPR STMTLIST kwEND               {}
    ;

FORBLOCK
    : kwFOR IDENT kwIN EXPR STMTLIST kwEND      {}
    | kwFOR IDENTASN kwTO EXPR STMTLIST kwEND   {}
    ;

FIELDSET
    : IDENTASN FIELDSET             {}
    |                               {}
    ;

IDENTASN
    : IDENT kwASSIGN EXPR           {}
    ;

PARAM
    : IDENT                         {}
    | IDENTASN                      {}
    ;

PARAMLIST
    : PARAM PARAMLIST               {}
    |                               {}
    ;

CALL
    : IDENT '(' EXPRLIST ')'        {}
    ;

RETURN
    : kwRETURN EXPR                 {}
    | kwRETURN                      {}
    ;

EXPRLIST
    : EXPR ',' EXPRLIST             {}
    | EXPR                          {}
    |                               {}
    ;

EXPRSTMT
    : CALL                          {}
    | EXPR OP_ACCESS CALL           {}
    ;

CLASSEXPR
    : kwCLASS FIELDSET kwEND        {}
    | kwCLASS kwEXTENDS IDENT FIELDSET kwEND {}
    ;

LIST_CONST
    : '[' EXPRLIST ']'              {}
    ;

EXPR
    : EXPRSTMT                      {}
    | '(' EXPR ')'                  {}
    | kwFUNC '(' PARAMLIST ')' STMTLIST kwEND {}
    | CLASSEXPR                     {}
    | LIST_CONST                    {}
    | INT_CONST                     {}
    | FLT_CONST                     {}
    | STR_CONST                     {}
    ;

%%

int yyerror(void* root, const char * err) {
    root = nullptr;
    fprintf(stderr, "Error @ line %d : %s\n", yylineno, err);
    return 0;
}
