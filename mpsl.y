%{

/* yacc file */

#include <stdio.h>
#include "fdm.h"

/* the script being compiled */
static fdm_v _pcode=NULL;

int yylex(void);
void yyerror(char * s);

fdm_v _ins(fdm_v opcode, fdm_v a1, fdm_v a2, fdm_v a3);

%}

%union {
	fdm_v v;	/* a simple value */
	fdm_v ins;	/* an 'instruction': [ opcode, args ] */
};

%token <v> INTEGER
%token <v> STRING
%token <v> SYMBOL
%token <v> LITERAL
%token WHILE IF SUB DUMP
%nonassoc IFI
%nonassoc ELSE

%left STREQ NUMEQ STRNE NUMNE HASHPAIR '>''<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%type <ins> stmt expr stmt_list list hash

%%

program:
	function		{ fdm_dump(_pcode, 0); }
	;

function:
	function stmt		{ fdm_apush(_pcode, $2); }
	| /* NULL */
	;

stmt:
	';'			{ $$ = _ins(FDM_LS(";"), NULL, NULL, NULL); }
	| expr ';'		{ $$ = $1; }
	| SYMBOL '=' expr ';'	{ $$ = _ins(FDM_LS("="),
					_ins(FDM_LS("SYMBOL"), $1, NULL, NULL),
					$3, NULL); }
	| DUMP expr ';'		{ $$ = _ins(FDM_LS("DUMP"), $2, NULL, NULL); }
	| WHILE '(' expr ')' stmt
				{ $$ = _ins(FDM_LS("WHILE"), $3, $5, NULL); }
	| IF '(' expr ')' stmt %prec IFI
				{ $$ = _ins(FDM_LS("IF"), $3, $5, NULL); }
	| IF '(' expr ')' stmt ELSE stmt
				{ $$ = _ins(FDM_LS("IFELSE"), $3, $5, $7); }
	| SUB SYMBOL '{' stmt_list '}'
				{ $$ = _ins(FDM_LS("SUB"),
					_ins(FDM_LS("SYMBOL"), $2, NULL, NULL),
					$4, NULL); }
	| '{' stmt_list '}'	{ $$ = $2; }
	;

stmt_list:
	stmt			{ $$ = $1; }
	| stmt_list stmt	{ $$ = _ins(FDM_LS(";"), $1, $2, NULL); }
	;

list:
	expr			{ $$ = _ins(FDM_LS("LIST"), $1, NULL, NULL); }
	| list ',' expr		{ fdm_apush($1, $3); $$ = $1; }
	;

hash:
	expr HASHPAIR expr	{ $$ = _ins(FDM_LS("HASH"), $1, $3, NULL); }
	| hash ',' expr HASHPAIR expr
				{ fdm_apush($1, $3); fdm_apush($1, $5); $$ = $1; }
	;

expr:
	INTEGER			{ $$ = _ins(FDM_LS("LITERAL"), $1, NULL, NULL); }
	| STRING		{ $$ = _ins(FDM_LS("LITERAL"), $1, NULL, NULL); }
	| SYMBOL		{ $$ = _ins(FDM_LS("SYMBOL"), $1, NULL, NULL); }

	| expr '+' expr		{ $$ = _ins(FDM_LS("+"), $1, $3, NULL); }
	| expr '*' expr		{ $$ = _ins(FDM_LS("*"), $1, $3, NULL); }
	| '(' expr ')'		{ $$ = $2; }
	| '[' list ']'		{ $$ = $2; }
	| '{' hash '}'		{ $$ = $2; }

	| SYMBOL '(' ')'	{ $$ = _ins(FDM_LS("CALL"),
					_ins(FDM_LS("SYMBOL"), $1, NULL, NULL),
					NULL, NULL); }
	| SYMBOL '(' list ')'	{ $$ = _ins(FDM_LS("CALL"),
					_ins(FDM_LS("SYMBOL"), $1, NULL, NULL),
					$3, NULL); }

	;

%%

void yyerror(char * s)
{
	printf("yyerror: %s\n", s);
}


fdm_v _ins(fdm_v opcode, fdm_v a1, fdm_v a2, fdm_v a3)
{
	fdm_v v;

	v=FDM_A(1);

	/* inserts the opcode */
	fdm_aset(v, opcode, 0);
	if(a1 != NULL) fdm_apush(v, a1);
	if(a2 != NULL) fdm_apush(v, a2);
	if(a3 != NULL) fdm_apush(v, a3);

	return(v);
}


int main(void)
{
	/* create a new pcode */
	_pcode=FDM_A(0);
	fdm_ref(_pcode);

	yyparse();

	printf("Exiting main...\n");
	exit(0);
}
