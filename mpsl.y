%{

/* yacc file */

#include <stdio.h>
#include "fdm.h"

int yylex(void);
void yyerror(char * s);

%}

%union {
	fdm_v v;
};

%token <v> INTEGER
%token <v> SYMBOL
%token <v> STRING
%token WHILE IF SUB DUMP
%nonassoc ELSE

%left STREQ NUMEQ STRNE NUMNE HASHPAIR '>''<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%type <v> stmt expr list hash compsym

%%

program:
	function		{ exit(0); }
	;

function:
	function stmt		{ printf("execute!!!\n"); }
	| /* NULL */
	;

stmt:
	';'			{ printf (";\n"); }
	| expr ';'		{ $$ = $1; }
	| compsym '=' expr ';'	{ fdm_sset(NULL, $1, $3); }
	| DUMP expr ';'		{ fdm_dump($2, 0); }
	;

list:
	expr			{ $$=FDM_A(1); fdm_aset($$, $1, 0); }
	| list ',' expr		{ fdm_apush($1, $3); $$ = $1; }
	;

hash:
	expr HASHPAIR expr		{ $$=FDM_H(0); fdm_hset($$, $1, $3); }
	| hash ',' expr HASHPAIR expr	{ fdm_hset($1, $3, $5); }

compsym:
	SYMBOL			{ $$=FDM_A(1); fdm_aset($$, $1, 0); }
	| compsym '.' INTEGER	{ fdm_apush($1, $3); $$ = $1; }
	| compsym '.' SYMBOL	{ fdm_apush($1, $3); $$ = $1; }
	| compsym '[' expr ']' { fdm_apush($1, $3); $$ = $1; }

expr:
	INTEGER			{ $$ = $1; }
	| STRING		{ $$ = $1; }
	| compsym		{ $$ = fdm_sget(NULL, $1); }
	| compsym '(' list ')'	{ printf("call!!!\n"); fdm_dump($3, 0); }

	| expr '+' expr		{ $$ = FDM_I(fdm_ival($1) + fdm_ival($3)); }
	| expr '-' expr		{ $$ = FDM_I(fdm_ival($1) - fdm_ival($3)); }
	| expr '*' expr		{ $$ = FDM_I(fdm_ival($1) * fdm_ival($3)); }
	| expr '/' expr		{ $$ = FDM_I(fdm_ival($1) / fdm_ival($3)); }
	| expr '<' expr		{ $$ = FDM_I(fdm_ival($1) < fdm_ival($3)); }
	| expr '>' expr		{ $$ = FDM_I(fdm_ival($1) > fdm_ival($3)); }
	| expr NUMEQ expr	{ $$ = FDM_I(fdm_ival($1) == fdm_ival($3)); }
	| expr NUMNE expr	{ $$ = FDM_I(fdm_ival($1) != fdm_ival($3)); }
	| expr STREQ expr	{ $$ = FDM_I(fdm_cmp($1, $3) == 0); }
	| expr STRNE expr	{ $$ = FDM_I(fdm_cmp($1, $3) != 0); }
	| expr '?' expr ':' expr { $$ = fdm_ival($1) ? $3 : $5; }
	| '(' expr ')'		{ $$ = $2; }

	| '[' ']'		{ $$ = FDM_A(0); }
	| '[' list ']'		{ $$ = $2; }

	| '{' '}'		{ $$ = FDM_H(0); }
	| '{' hash '}'		{ $$ = $2; }
	;

%%

void yyerror(char * s)
{
	printf("yyerror: %s\n", s);
}


int main(void)
{
	yyparse();
	exit(0);
}
