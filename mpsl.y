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
#include <wchar.h>
#include "mpdm.h"
#include "mpsl.h"

/*******************
	Data
********************/

/* the bytecode */
static mpdm_v _mpsl_bytecode=NULL;

int yylex(void);
void yyerror(char * s);

/* pointer to source code being compiled */
extern wchar_t * _mpsl_next_char;

/* shortcut macros to insert instructions */

#define INS0(o)			_ins(o, 0, NULL, NULL, NULL)
#define INS1(o,a1)		_ins(o, 1, a1, NULL, NULL)
#define INS2(o,a1,a2)		_ins(o, 2, a1, a2, NULL)
#define INS3(o,a1,a2,a3)	_ins(o, 3, a1, a2, a3)

static mpdm_v _ins(mpsl_op opcode, int args, mpdm_v a1, mpdm_v a2, mpdm_v a3);

/* defined in mpsl_c.c */
mpdm_v _mpsl_op(mpsl_op opcode);

/*******************
	Code
********************/

%}

%union {
	mpdm_v v;	/* a simple value */
	mpdm_v ins;	/* an 'instruction': [ opcode, args ] */
};

%token <v> NULLV INTEGER REAL STRING SYMBOL LITERAL
%token WHILE IF SUB FOREACH LOCAL
%nonassoc IFI
%nonassoc ELSE

%left BOOLAND BOOLOR
%left INC DEC IMMADD IMMSUB IMMMUL IMMDIV
%left '!'
%left STREQ NUMEQ STRNE NUMNE NUMGE NUMLE HASHPAIR RANGE '>''<'
%left '+' '-'
%left '*' '/' MODULO
%nonassoc UMINUS

%type <ins> stmt expr sym_list stmt_list list hash compsym

%%

program:
	function		{ ; }
	;

function:
	function stmt		{
					mpdm_apush(_mpsl_bytecode, $2);
				}
	| /* NULL */
	;

stmt:
	';'			{
					/* null instruction */
					$$ = INS0(MPSL_OP_MULTI);
				}
	| expr ';'		{
					/* expression, as is */
					$$ = $1;
				}
	| compsym '=' expr ';'	{
					/* simple assignation */
					$$ = INS2(MPSL_OP_ASSIGN, $1, $3);
				}

	| WHILE '(' expr ')' stmt
				{
					/* while loop */
					$$ = INS2(MPSL_OP_WHILE, $3, $5);
				}
	| IF '(' expr ')' stmt %prec IFI
				{
					/* if - then construction */
					$$ = INS2(MPSL_OP_IF, $3, $5);
				}
	| IF '(' expr ')' stmt ELSE stmt
				{
					/* if - then - else construction */
					$$ = INS3(MPSL_OP_IF, $3, $5, $7);
				}

	| SUB compsym '{' stmt_list '}'
				{
					/* subroutine definition,
					   without arguments */
					$$ = INS2(MPSL_OP_ASSIGN, $2,
						INS1(MPSL_OP_SUBFRAME, $4));
				}

	| SUB compsym '(' ')' '{' stmt_list '}'
				{
					/* subroutine definition,
					   without arguments (second
					   syntax, including parens) */
					$$ = INS2(MPSL_OP_ASSIGN, $2,
						INS1(MPSL_OP_SUBFRAME, $6));
				}

	| SUB compsym '(' sym_list ')' '{' stmt_list '}'
				{
					/* subroutine definition,
					   with arguments */
					$$ = INS2(MPSL_OP_ASSIGN, $2,
						INS1(MPSL_OP_SUBFRAME,
							INS2(MPSL_OP_MULTI,
								INS1(MPSL_OP_ARGS, $4),
								$7)
						)
					);
				}

	| FOREACH '(' compsym ',' expr ')' stmt
				{
					/* foreach construction */
					$$ = INS3(MPSL_OP_FOREACH, $3, $5, $7);
				}

	| '{' stmt_list '}'	{
					/* block of instructions,
					   with local symbol table */
					$$ = INS1(MPSL_OP_BLKFRAME, $2);
				}

	| LOCAL sym_list ';'	{
					/* local symbol creation */
					$$ = INS1(MPSL_OP_LOCAL, $2);
				}
	| LOCAL SYMBOL '=' expr	';'
				{
					/* contraction; local symbol
					   creation and assignation */
					$$ = INS2(MPSL_OP_MULTI,
						INS1(MPSL_OP_LOCAL,
							INS1(MPSL_OP_LITERAL, $2)),
						INS2(MPSL_OP_ASSIGN,
							INS1(MPSL_OP_LITERAL, $2),$4)
						);
				}

	;

stmt_list:
	stmt			{ $$ = $1; }
	| stmt_list stmt	{
					/* sequence of instructions */
					$$ = INS2(MPSL_OP_MULTI, $1, $2);
				}
	;

list:
	expr			{
					$$ = INS1(MPSL_OP_LIST, $1);
				}
	| list ',' expr		{
					/* build list from list of
					   instructions */
					mpdm_apush($1, $3); $$ = $1;
				}
	;

sym_list:
	SYMBOL			{
					$$ = INS1(MPSL_OP_LIST,
						INS1(MPSL_OP_LITERAL, $1));
				}
	| sym_list ',' SYMBOL	{
					/* comma-separated list of symbols */
					mpdm_apush($1,
						INS1(MPSL_OP_LITERAL, $3));
					$$ = $1;
				}
	;

hash:
	expr HASHPAIR expr	{
					$$ = INS2(MPSL_OP_HASH, $1, $3);
				}
	| hash ',' expr HASHPAIR expr
				{
					/* build hash from list of
					   instructions */
					mpdm_apush($1, $3);
					mpdm_apush($1, $5);
					$$ = $1;
				}
	;

compsym:
	SYMBOL			{
					$$ = INS1(MPSL_OP_LIST,
						INS1(MPSL_OP_LITERAL, $1));
				}
	| compsym '.' INTEGER	{
					/* a.5 compound symbol */
					mpdm_apush($1,
				  		INS1(MPSL_OP_LITERAL, $3));
				  	$$ = $1;
				}
	| compsym '.' SYMBOL	{
					/* a.b compound symbol */
					mpdm_apush($1,
				  		INS1(MPSL_OP_LITERAL, $3));
				  	$$ = $1;
				}
	| compsym '[' expr ']'	{
					/* a["b"] or a[5] compound symbol */
					mpdm_apush($1, $3);
					$$ = $1;
				}
	;

expr:
	INTEGER			{
					/* literal integer */
					$$ = INS1(MPSL_OP_LITERAL, $1);
				}
	| STRING		{
					/* literal string */
					$$ = INS1(MPSL_OP_LITERAL, $1);
				}
	| REAL			{
					/* literal real number */
					$$ = INS1(MPSL_OP_LITERAL, $1);
				}
	| compsym		{
					/* compound symbol */
					mpdm_aset($1, _mpsl_op(MPSL_OP_SYMVAL), 0);
					$$ = $1;
				}
	| NULLV			{
					/* NULL value */
					$$ = INS1(MPSL_OP_LITERAL, NULL);
				}

	| '-' expr %prec UMINUS	{
					/* unary minus */
					$$ = INS1(MPSL_OP_UMINUS, $2);
				}

				/* math operations */
	| expr '+' expr		{ $$ = INS2(MPSL_OP_ADD, $1, $3); }
	| expr '-' expr		{ $$ = INS2(MPSL_OP_SUB, $1, $3); }
	| expr '*' expr		{ $$ = INS2(MPSL_OP_MUL, $1, $3); }
	| expr '/' expr		{ $$ = INS2(MPSL_OP_DIV, $1, $3); }
	| expr MODULO expr	{ $$ = INS2(MPSL_OP_MOD, $1, $3); }

				/* immediate math operations */
	| INC compsym		{ $$ = INS1(MPSL_OP_PINC, $2); }
	| compsym INC		{ $$ = INS1(MPSL_OP_SINC, $1); }
	| DEC compsym		{ $$ = INS1(MPSL_OP_PDEC, $2); }
	| compsym DEC		{ $$ = INS1(MPSL_OP_SDEC, $1); }
	| compsym IMMADD expr	{ $$ = INS2(MPSL_OP_IMMADD, $1, $3); }
	| compsym IMMSUB expr	{ $$ = INS2(MPSL_OP_IMMSUB, $1, $3); }
	| compsym IMMMUL expr	{ $$ = INS2(MPSL_OP_IMMMUL, $1, $3); }
	| compsym IMMDIV expr	{ $$ = INS2(MPSL_OP_IMMDIV, $1, $3); }

	| '!' expr		{
					/* boolean not */
					$$ = INS1(MPSL_OP_NOT, $2);
				}
	| expr '<' expr		{
					/* bool less than */
					$$ = INS2(MPSL_OP_NUMLT, $1, $3);
				}
	| expr '>' expr		{
					/* bool greater than */
					$$ = INS2(MPSL_OP_NUMGT, $1, $3);
				}
	| expr NUMLE expr	{
					/* bool less or equal than */
					$$ = INS2(MPSL_OP_NUMLE, $1, $3);
				}
	| expr NUMGE expr	{
					/* bool greater or equal than */
					$$ = INS2(MPSL_OP_NUMGE, $1, $3);
				}
	| expr NUMEQ expr       {
					/* bool numeric equal */
					$$ = INS2(MPSL_OP_NUMEQ, $1, $3);
				}
	| expr NUMNE expr	{
					/* bool numeric non-equal */
					$$ = INS1(MPSL_OP_NOT,
						INS2(MPSL_OP_NUMEQ, $1, $3));
				}
	| expr STREQ expr       {
					/* bool string equal */
					$$ = INS2(MPSL_OP_STREQ, $1, $3);
				}
	| expr STRNE expr	{
					/* bool string non-equal */
					$$ = INS1(MPSL_OP_NOT,
						INS2(MPSL_OP_STREQ, $1, $3));
				}
	| expr BOOLAND expr	{
					/* boolean and */
					$$ = INS2(MPSL_OP_AND, $1, $3);
				}
	| expr BOOLOR expr	{
					/* boolean or */
					$$ = INS2(MPSL_OP_OR, $1, $3);
				}
 
	| '(' expr ')'		{
					/* parenthesized expression */
					$$ = $2;
				}

	| '[' ']'		{
					/* empty list */
					$$ = INS0(MPSL_OP_LIST);
				}
	| '[' list ']'		{
					/* non-empty list */
					$$ = $2;
				}
	| expr RANGE expr	{
					/* build range from expressions */
					$$ = INS2(MPSL_OP_RANGE, $1, $3);
				}

	| '{' '}'		{
					/* empty hash */
					$$ = INS0(MPSL_OP_HASH);
				}
	| '{' hash '}'		{
					/* non-empty hash */
					$$ = $2;
				}

	| compsym '(' ')'	{
					/* function call (without args) */
					$$ = INS1(MPSL_OP_EXEC,
						INS1(MPSL_OP_SYMVAL, $1));
				}
	| compsym '(' list ')'	{
					/* function call (with args) */
					$$ = INS2(MPSL_OP_EXEC,
						INS1(MPSL_OP_SYMVAL, $1), $3);
				}

	;

%%

void yyerror(char * s)
{
	printf("yyerror: %s\n", s);
}


static mpdm_v _ins(mpsl_op opcode, int args, mpdm_v a1, mpdm_v a2, mpdm_v a3)
/* adds an instruction */
{
	mpdm_v v;

	v=MPDM_A(args + 1);

	/* inserts the opcode */
	mpdm_aset(v, _mpsl_op(opcode), 0);
	if(args > 0) mpdm_aset(v, a1, 1);
	if(args > 1) mpdm_aset(v, a2, 2);
	if(args > 2) mpdm_aset(v, a3, 3);

	return(v);
}


mpdm_v mpsl_compile(mpdm_v code)
{
	mpdm_v x=NULL;

	/* create a new holder for the bytecode */
	_mpsl_bytecode=MPDM_A(1);

	/* point to code */
	_mpsl_next_char=(wchar_t *) code->data;

	mpdm_ref(code);

	/* compile! */
	if(yyparse() == 0)
	{
		mpdm_aset(_mpsl_bytecode, _mpsl_op(MPSL_OP_SUBFRAME), 0);
		x=MPDM_X2(_mpsl_machine, _mpsl_bytecode);
	}

	mpdm_unref(code);
	_mpsl_bytecode=NULL;

	return(x);
}
