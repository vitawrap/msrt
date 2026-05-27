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

%token <fVal> FLT_CONST
%token <iVal> INT_CONST
%token <s> IDENT STR_CONST

%left '['
%token kwLOCAL kwEND kwIF kwTHEN kwELSE kwELSIF kwFOR kwWHILE kwIN kwTO kwBY kwBREAK kwCONTINUE kwFUNC kwRETURN kwCLASS kwEXTENDS
%right <op> OP_MODASN OP_ANDASN OP_ORASN OP_XORASN OP_ADDASN OP_SUBASN OP_MULASN OP_DIVASN OP_SHLASN OP_SHRASN OP_ASSIGN
%left <op> OP_AND OP_OR
%left <op> OP_BITOR OP_XOR
%left <op> OP_BITAND
%left <op> OP_EQUAL OP_NEQUAL
%left <op> OP_LESS OP_GREATER OP_LEQUAL OP_GEQUAL
%left <op> OP_SHL OP_SHR
%left <op> OP_ADD OP_SUB
%left <op> OP_MUL OP_DIV OP_MOD
%right <op> OP_NOT OP_ONESC OP_INC OP_DEC UNARY
%left OP_ACCESS

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
    | LOCALVARDECL  {}
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
    : kwFOR IDENT kwIN EXPR STMTLIST kwEND              {}
    | kwFOR IDENTASN kwTO EXPR STMTLIST kwEND           {}
    | kwFOR IDENTASN kwTO EXPR kwBY EXPR STMTLIST kwEND {}
    ;

FIELDSET
    : IDENTASN FIELDSET             {}
    |                               {}
    ;

ANY_ASN
    : OP_ASSIGN                     {}
    | OP_ADDASN                     {}
    | OP_SUBASN                     {}
    | OP_MULASN                     {}
    | OP_DIVASN                     {}
    | OP_MODASN                     {}
    | OP_ANDASN                     {}
    | OP_ORASN                      {}
    | OP_XORASN                     {}
    | OP_SHLASN                     {}
    | OP_SHRASN                     {}
    ;

INCDEC
    : OP_INC                        {}
    | OP_DEC                        {}
    ;

VARASN
    : IDENT ANY_ASN EXPR                {}
    | EXPR OP_ACCESS IDENT ANY_ASN EXPR {}
    | EXPR '[' EXPR ']' ANY_ASN EXPR    {}
    | INCDEC IDENT                      {}
    | IDENT INCDEC                      {}
    | INCDEC EXPR OP_ACCESS IDENT       {}
    | EXPR OP_ACCESS IDENT INCDEC       {}
    | INCDEC EXPR '[' EXPR ']'          {}
    | EXPR '[' EXPR ']' INCDEC          {}
    ;

LOCALVARDECL
    : kwLOCAL IDENT OP_ASSIGN EXPR      {}
    ;

IDENTASN
    : IDENT OP_ASSIGN EXPR          {}
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
    | VARASN                        {}
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
    | EXPR OP_XOR EXPR              {}
    | EXPR OP_MOD EXPR              {}
    | EXPR OP_BITAND EXPR           {}
    | EXPR OP_BITOR EXPR            {}
    | EXPR OP_ADD EXPR              {}
    | EXPR OP_SUB EXPR              {}
    | EXPR OP_MUL EXPR              {}
    | EXPR OP_DIV EXPR              {}
    | OP_SUB EXPR %prec UNARY       {}
    | EXPR OP_LESS EXPR             {}
    | EXPR OP_GREATER EXPR          {}
    | EXPR OP_GEQUAL EXPR           {}
    | EXPR OP_LEQUAL EXPR           {}
    | EXPR OP_EQUAL EXPR            {}
    | EXPR OP_NEQUAL EXPR           {}
    | EXPR OP_OR EXPR               {}
    | EXPR OP_AND EXPR              {}
    | EXPR OP_SHL EXPR              {}
    | EXPR OP_SHR EXPR              {}
    | OP_NOT EXPR                   {}
    | OP_ONESC EXPR                 {}
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
