%{
/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpsl.l - Minimum Profit Scripting Language [F]lexer

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

#include <stdlib.h>
#include "y.tab.h"

void yyerror(char * s);
%}

DIGIT		[0-9]
LETTER		[a-zA-Z_]
COMMENT		#.*\n
WSPACE		[ \t\n]+
OPERATOR	[-()<>=+*/;{}?:.\[\],]

INTEGER		{DIGIT}+
REAL		{DIGIT}*[\.]?{DIGIT}+
SYMBOL		{LETTER}({LETTER}|{DIGIT})*

%x REM

%%

{INTEGER}	{
			/* integer */
			yylval.v=MPDM_MBS(yytext);
			return(INTEGER);
		}

{REAL}		{
			/* real numbers */
			yylval.v=MPDM_MBS(yytext);
			return(REAL);
		}

\"[^"]*\"	{
			/* string includes quotes; strip them */
			yytext[yyleng - 1]='\0';
			yylval.v=MPDM_MBS(yytext + 1);
			return(STRING);
		}

{OPERATOR}	return(*yytext);

"NULL"		return NULLV;
"while"		return WHILE;
"if"		return IF;
"else"		return ELSE;
"sub"		return SUB;
"foreach"	return FOREACH;
"dump"		return DUMP;

"=="		return NUMEQ;
"!="		return NUMNE;
"eq"		return STREQ;
"ne"		return STRNE;
"=>"		return HASHPAIR;
".."		return RANGE;

{SYMBOL}	{
			/* symbol name */
			yylval.v=MPDM_MBS(yytext);
			return(SYMBOL);
		}

{COMMENT}	;	/* ignore comments */

{WSPACE}	;	/* ignore spaces */

.		yyerror("LEX Unknown character");

\/\*		{ BEGIN REM; /* C-like comments */ }
<REM>\*\/	{ BEGIN 0; }
<REM>.		;

%%

int yywrap(void) { return(1); }
