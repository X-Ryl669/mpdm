%{
/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpsl.y - Minimum Profit Scripting Language YACC parser

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    http://www.triptico.com

*/

#include <stdio.h>
#include "mpdm.h"

/* the script being compiled */
static mpdm_v _bytecode=NULL;

int yylex(void);
void yyerror(char * s);
void _mpsl_store_code(mpdm_v code);

/* shortcut macros to insert instructions */

#define INS0(o)			_ins(o, 0, NULL, NULL, NULL)
#define INS1(o,a1)		_ins(o, 1, a1, NULL, NULL)
#define INS2(o,a1,a2)		_ins(o, 2, a1, a2, NULL)
#define INS3(o,a1,a2,a3)	_ins(o, 3, a1, a2, a3)

mpdm_v _ins(mpdm_v opcode, int args, mpdm_v a1, mpdm_v a2, mpdm_v a3);

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

%type <ins> stmt expr sym_list stmt_list list hash compsym

%%

program:
	function		{ ; }
	;

function:
	function stmt		{ mpdm_apush(_bytecode, $2); }
	| /* NULL */
	;

stmt:
	';'			{ $$ = INS0(MPDM_LS(L";")); }
	| expr ';'		{ $$ = $1; }
	| compsym '=' expr ';'	{ $$ = INS2(MPDM_LS(L"="), $1, $3); }
	| DUMP expr ';'		{ $$ = INS1(MPDM_LS(L"DUMP"), $2); }
	| WHILE '(' expr ')' stmt
				{ $$ = INS2(MPDM_LS(L"WHILE"), $3, $5); }
	| IF '(' expr ')' stmt %prec IFI
				{ $$ = INS2(MPDM_LS(L"IF"), $3, $5); }
	| IF '(' expr ')' stmt ELSE stmt
				{ $$ = INS3(MPDM_LS(L"IFELSE"), $3, $5, $7); }

	| SUB compsym '{' stmt_list '}'
				{ mpdm_v w=MPDM_A(3);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_PREFIX")), 0);
				mpdm_aset(w, $4, 1);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_POSTFIX")), 2);
				$$ = INS2(MPDM_LS(L"SUB"), $2, w); }

	| SUB compsym '(' ')' '{' stmt_list '}'
				{ mpdm_v w=MPDM_A(3);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_PREFIX")), 0);
				mpdm_aset(w, $6, 1);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_POSTFIX")), 2);
				$$ = INS2(MPDM_LS(L"SUB"), $2, w); }

	| SUB compsym '(' sym_list ')' '{' stmt_list '}'
				{ mpdm_v w=MPDM_A(4);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_PREFIX")), 0);
				mpdm_aset(w, INS1(MPDM_LS(L"ARGS"), $4), 1);
				mpdm_aset(w, $7, 2);
				mpdm_aset(w, INS0(MPDM_LS(L"SUB_POSTFIX")), 3);
				$$ = INS2(MPDM_LS(L"SUB"), $2, w); }

	| FOREACH compsym expr stmt
				{ $$ = INS3(MPDM_LS(L"FOREACH"), $2, $3, $4); }

	| '{' stmt_list '}'	{ mpdm_v w=MPDM_A(3);
				mpdm_aset(w, INS0(MPDM_LS(L"BLK_PREFIX")), 0);
				mpdm_aset(w, $2, 1);
				mpdm_aset(w, INS0(MPDM_LS(L"BLK_POSTFIX")), 2);
				$$ = w; }
	;

stmt_list:
	stmt			{ $$ = $1; }
	| stmt_list stmt	{ $$ = INS2(MPDM_LS(L";"), $1, $2); }
	;

list:
	expr			{ $$ = INS1(MPDM_LS(L"LIST"), $1); }
	| list ',' expr		{ mpdm_apush($1, $3); $$ = $1; }
	;

sym_list:
	SYMBOL			{ $$ = INS1(MPDM_LS(L"SYMLIST"), $1); }
	| sym_list ',' SYMBOL	{ mpdm_apush($1, $3); $$ = $1; }
	;

hash:
	expr HASHPAIR expr	{ $$ = INS2(MPDM_LS(L"HASH"), $1, $3); }
	| hash ',' expr HASHPAIR expr
				{ mpdm_apush($1, $3); mpdm_apush($1, $5); $$ = $1; }
	;

compsym:
	SYMBOL			{ $$ = INS1(MPDM_LS(L"SYMBOL"),
					INS1(MPDM_LS(L"LITERAL"), $1)); }
	| compsym '.' INTEGER	{ mpdm_apush($1,
				  INS1(MPDM_LS(L"LITERAL"), $3));
				  $$ = $1; }
	| compsym '.' SYMBOL	{ mpdm_apush($1,
				  INS1(MPDM_LS(L"LITERAL"), $3));
				  $$ = $1; }
	| compsym '[' expr ']'	{ mpdm_apush($1, $3); $$ = $1; }
	;

expr:
	INTEGER			{ $$ = INS1(MPDM_LS(L"LITERAL"), $1); }
	| STRING		{ $$ = INS1(MPDM_LS(L"LITERAL"), $1); }
	| REAL			{ $$ = INS1(MPDM_LS(L"LITERAL"), $1); }
	| compsym		{ mpdm_aset($1, MPDM_LS(L"SYMVAL"), 0); $$ = $1; }
	| NULLV			{ $$ = INS0(MPDM_LS(L"NULL")); }

	| '-' expr %prec UMINUS	{ $$ = INS1(MPDM_LS(L"UMINUS"), $2); }

	| expr '+' expr		{ $$ = INS2(MPDM_LS(L"+"), $1, $3); }
	| expr '-' expr		{ $$ = INS2(MPDM_LS(L"-"), $1, $3); }
	| expr '*' expr		{ $$ = INS2(MPDM_LS(L"*"), $1, $3); }
	| expr '/' expr		{ $$ = INS2(MPDM_LS(L"/"), $1, $3); }

	| expr '<' expr		{ $$ = INS2(MPDM_LS(L"<"), $1, $3); }
	| expr '>' expr		{ $$ = INS2(MPDM_LS(L">"), $1, $3); }
	| expr NUMEQ expr       { $$ = INS2(MPDM_LS(L"NUMEQ"), $1, $3); }
	| expr NUMNE expr       { $$ = INS2(MPDM_LS(L"NUMNE"), $1, $3); }
	| expr STREQ expr       { $$ = INS2(MPDM_LS(L"STREQ"), $1, $3); }
	| expr STRNE expr       { $$ = INS2(MPDM_LS(L"STRNE"), $1, $3); }
 
	| '(' expr ')'		{ $$ = $2; }

	| '[' ']'		{ $$ = INS0(MPDM_LS(L"LIST")); }
	| '[' list ']'		{ $$ = $2; }
	| '[' expr RANGE expr ']' { $$ = INS2(MPDM_LS(L"RANGE"), $2, $4); }

	| '{' '}'		{ $$ = INS0(MPDM_LS(L"HASH")); }
	| '{' hash '}'		{ $$ = $2; }

	| compsym '(' ')'	{ $$ = INS1(MPDM_LS(L"CALL"), $1); }
	| compsym '(' list ')'	{ $$ = INS2(MPDM_LS(L"CALL"), $1, $3); }

	;

%%

void yyerror(char * s)
{
	printf("yyerror: %s\n", s);
}


mpdm_v _ins(mpdm_v opcode, int args, mpdm_v a1, mpdm_v a2, mpdm_v a3)
{
	mpdm_v v;

	v=MPDM_A(args + 1);

	/* inserts the opcode */
	mpdm_aset(v, opcode, 0);
	if(args > 0) mpdm_aset(v, a1, 1);
	if(args > 1) mpdm_aset(v, a2, 2);
	if(args > 2) mpdm_aset(v, a3, 3);

	return(v);
}


static mpdm_v _mpsl_machine(mpdm_v c, mpdm_v args)
{
	printf("Executing mpsl!!!\n");
	mpdm_dump(c);

	return(args);
}


mpdm_v mpsl_compile(mpdm_v code)
{
	mpdm_v x;

	/* create a new holder for the bytecode */
	_bytecode=MPDM_A(0);
	mpdm_apush(_bytecode, MPDM_LS(L"PROG"));

	/* creates the new executable value */
	x=MPDM_A(2);
	x->flags |= MPDM_EXEC;

	/* first argument is the interpreter, and second the bytecode */
	mpdm_aset(x, MPDM_X(_mpsl_machine), 0);
	mpdm_aset(x, _bytecode, 1);

	/* stores the code to be compiled */
	_mpsl_store_code(code);

	mpdm_ref(x);
	mpdm_ref(code);

	/* compile! */
	yyparse();

	mpdm_unref(code);
	mpdm_unref(x);

	return(x);
}
