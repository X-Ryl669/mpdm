%{

/* yacc file */

#include <stdio.h>
#include "mpdm.h"

/* the script being compiled */
static mpdm_v _pcode=NULL;

int yylex(void);
void yyerror(char * s);

mpdm_v _ins(mpdm_v opcode, mpdm_v a1, mpdm_v a2, mpdm_v a3);

%}

%union {
	mpdm_v v;	/* a simple value */
	mpdm_v ins;	/* an 'instruction': [ opcode, args ] */
};

%token <v> NULLV
%token <v> INTEGER
%token <v> REAL
%token <v> STRING
%token <v> SYMBOL
%token <v> LITERAL
%token WHILE IF SUB FOREACH DUMP
%nonassoc IFI
%nonassoc ELSE

%left STREQ NUMEQ STRNE NUMNE HASHPAIR RANGE '>''<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%type <ins> stmt expr sym_list stmt_list block list hash compsym

%%

program:
	function		{ ; }
	;

function:
	function stmt		{ mpdm_apush(_pcode, $2); }
	| /* NULL */
	;

stmt:
	';'			{ $$ = _ins(MPDM_LS(L";"), NULL, NULL, NULL); }
	| expr ';'		{ $$ = $1; }
	| compsym '=' expr ';'	{ $$ = _ins(MPDM_LS(L"="), $1, $3, NULL); }
	| DUMP expr ';'		{ $$ = _ins(MPDM_LS(L"DUMP"), $2, NULL, NULL); }
	| WHILE '(' expr ')' stmt
				{ $$ = _ins(MPDM_LS(L"WHILE"), $3, $5, NULL); }
	| IF '(' expr ')' stmt %prec IFI
				{ $$ = _ins(MPDM_LS(L"IF"), $3, $5, NULL); }
	| IF '(' expr ')' stmt ELSE stmt
				{ $$ = _ins(MPDM_LS(L"IFELSE"), $3, $5, $7); }
	| SUB compsym block
				{ $$ = _ins(MPDM_LS(L"SUB"), $2, NULL, $3); }
	| SUB compsym '(' ')' block
				{ $$ = _ins(MPDM_LS(L"SUB"), $2, NULL, $5); }
	| SUB compsym '(' sym_list ')' block
				{ $$ = _ins(MPDM_LS(L"SUB"), $2, $4, $6); }
	| FOREACH compsym '(' list ')' stmt
				{ $$ = _ins(MPDM_LS(L"FOREACH"), $2, $4, $6); }
	| block			{ $$ = $1; }
	;

block:
	'{' stmt_list '}'	{ mpdm_v w=MPDM_A(3);
				mpdm_aset(w, _ins(MPDM_LS(L"PREFIX"), NULL, NULL, NULL), 0);
				mpdm_aset(w, $2, 1);
				mpdm_aset(w, _ins(MPDM_LS(L"POSTFIX"), NULL, NULL, NULL), 2);
				$$ = w; }

stmt_list:
	stmt			{ $$ = $1; }
	| stmt_list stmt	{ $$ = _ins(MPDM_LS(L";"), $1, $2, NULL); }
	;

list:
	expr			{ $$ = _ins(MPDM_LS(L"LIST"), $1, NULL, NULL); }
	| list ',' expr		{ mpdm_apush($1, $3); $$ = $1; }
	;

sym_list:
	SYMBOL			{ $$ = _ins(MPDM_LS(L"SYMLIST"), $1, NULL, NULL); }
	| sym_list ',' SYMBOL	{ mpdm_apush($1, $3); $$ = $1; }
	;

hash:
	expr HASHPAIR expr	{ $$ = _ins(MPDM_LS(L"HASH"), $1, $3, NULL); }
	| hash ',' expr HASHPAIR expr
				{ mpdm_apush($1, $3); mpdm_apush($1, $5); $$ = $1; }
	;

compsym:
	SYMBOL			{ $$ = _ins(MPDM_LS(L"SYMBOL"),
					_ins(MPDM_LS(L"LITERAL"), $1, NULL, NULL),
					NULL, NULL); }
	| compsym '.' INTEGER	{ mpdm_apush($1,
				  _ins(MPDM_LS(L"LITERAL"), $3, NULL, NULL));
				  $$ = $1; }
	| compsym '.' SYMBOL	{ mpdm_apush($1,
				  _ins(MPDM_LS(L"LITERAL"), $3, NULL, NULL));
				  $$ = $1; }
	| compsym '[' expr ']'	{ mpdm_apush($1, $3); $$ = $1; }
	;

expr:
	INTEGER			{ $$ = _ins(MPDM_LS(L"LITERAL"), $1, NULL, NULL); }
	| STRING		{ $$ = _ins(MPDM_LS(L"LITERAL"), $1, NULL, NULL); }
	| REAL			{ $$ = _ins(MPDM_LS(L"LITERAL"), $1, NULL, NULL); }
/*	| compsym		{ $$ = _ins(MPDM_LS("SYMVAL"), $1, NULL, NULL); } */
	| compsym		{ mpdm_aset($1, MPDM_LS(L"SYMVAL"), 0); $$ = $1; }
	| NULLV			{ $$ = _ins(MPDM_LS(L"NULL"), NULL, NULL, NULL); }

	| '-' expr %prec UMINUS	{ $$ = _ins(MPDM_LS(L"UMINUS"), $2, NULL, NULL); }

	| expr '+' expr		{ $$ = _ins(MPDM_LS(L"+"), $1, $3, NULL); }
	| expr '-' expr		{ $$ = _ins(MPDM_LS(L"-"), $1, $3, NULL); }
	| expr '*' expr		{ $$ = _ins(MPDM_LS(L"*"), $1, $3, NULL); }
	| expr '/' expr		{ $$ = _ins(MPDM_LS(L"/"), $1, $3, NULL); }

	| expr '<' expr		{ $$ = _ins(MPDM_LS(L"<"), $1, $3, NULL); }
	| expr '>' expr		{ $$ = _ins(MPDM_LS(L">"), $1, $3, NULL); }
	| expr NUMEQ expr       { $$ = _ins(MPDM_LS(L"NUMEQ"), $1, $3, NULL); }
	| expr NUMNE expr       { $$ = _ins(MPDM_LS(L"NUMNE"), $1, $3, NULL); }
	| expr STREQ expr       { $$ = _ins(MPDM_LS(L"STREQ"), $1, $3, NULL); }
	| expr STRNE expr       { $$ = _ins(MPDM_LS(L"STRNE"), $1, $3, NULL); }
 
	| '(' expr ')'		{ $$ = $2; }

	| '[' ']'		{ $$ = _ins(MPDM_LS(L"LIST"), NULL, NULL, NULL); }
	| '[' list ']'		{ $$ = $2; }
	| '[' expr RANGE expr ']' { $$ = _ins(MPDM_LS(L"RANGE"), $2, $4, NULL); }

	| '{' '}'		{ $$ = _ins(MPDM_LS(L"HASH"), NULL, NULL, NULL); }
	| '{' hash '}'		{ $$ = $2; }

	| compsym '(' ')'	{ $$ = _ins(MPDM_LS(L"CALL"), $1, NULL, NULL); }
	| compsym '(' list ')'	{ $$ = _ins(MPDM_LS(L"CALL"), $1, $3, NULL); }

	;

%%

void yyerror(char * s)
{
	printf("yyerror: %s\n", s);
}


mpdm_v _ins(mpdm_v opcode, mpdm_v a1, mpdm_v a2, mpdm_v a3)
{
	mpdm_v v;

	v=MPDM_A(1);

	/* inserts the opcode */
	mpdm_aset(v, opcode, 0);
	if(a1 != NULL) mpdm_apush(v, a1);
	if(a2 != NULL) mpdm_apush(v, a2);
	if(a3 != NULL) mpdm_apush(v, a3);

	return(v);
}


int main(void)
{
	/* create a new pcode */
	_pcode=MPDM_A(0);
	mpdm_ref(_pcode);
	mpdm_apush(_pcode, MPDM_LS(L"PROG"));

	yyparse();

	mpdm_dump(_pcode);

	printf("Exiting main...\n");
	exit(0);
}
