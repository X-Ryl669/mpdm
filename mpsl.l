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
#include <wchar.h>
#include "mpdm.h"

#include <stdlib.h>
#include "y.tab.h"

void yyerror(char * s);
int yy_input_for_flex(char * buf, int max);

/* redefinition of input function for GNU Flex */
#undef YY_INPUT
#define YY_INPUT(b,r,m) (r=yy_input_for_flex(b,m))

/* internal pointer to next character in code */
wchar_t * _mpsl_next_char=NULL;

/* string being parsed */
static wchar_t * _mpsl_string=NULL;
static int _mpsl_string_i=0;

static void _mpsl_add_string(wchar_t c)
/* dynamic string */
{
	_mpsl_string=realloc(_mpsl_string,
		(_mpsl_string_i + 1) * sizeof(wchar_t));
	_mpsl_string[_mpsl_string_i++]=c;
}


%}

DIGIT		[0-9]
LETTER		[a-zA-Z_]
COMMENT		#.*\n
WSPACE		[ \t\n]+

INTEGER		{DIGIT}+
REAL		{DIGIT}*[\.]?{DIGIT}+
SYMBOL		{LETTER}({LETTER}|{DIGIT})*

%x REM
%x STR

%%

{INTEGER}	{
			/* integers */
			yylval.v=MPDM_MBS(yytext);
			return(INTEGER);
		}

{REAL}		{
			/* real numbers */
			yylval.v=MPDM_MBS(yytext);
			return(REAL);
		}

\'[^']*\'	{
			/* single quoted string; return as is */
			yytext[yyleng - 1]='\0';
			yylval.v=MPDM_MBS(yytext + 1);
			return(STRING);
		}

"NULL"		return NULLV;
"while"		return WHILE;
"if"		return IF;
"else"		return ELSE;
"sub"		return SUB;
"foreach"	return FOREACH;
"local"		return LOCAL;
"break"		return BREAK;
"return"	return RETURN;

"=="		return NUMEQ;
"!="		return NUMNE;
">="		return NUMGE;
"<="		return NUMLE;
"&&"		return BOOLAND;
"||"		return BOOLOR;
"=>"		return HASHPAIR;
".."		return RANGE;

"eq"		return STREQ;
"ne"		return STRNE;
"~"		return STRCAT;

"++"		return INC;
"--"		return DEC;
"+="		return IADD;
"-="		return ISUB;
"*="		return IMUL;
"/="		return IDIV;
"%"		return MODULO;

{SYMBOL}	{
			/* symbol name */
			yylval.v=MPDM_MBS(yytext);
			return(SYMBOL);
		}

{COMMENT}	;	/* ignore comments */

{WSPACE}	;	/* ignore spaces */

\/\*		{ BEGIN REM; /* C-like comments */ }
<REM>\*\/	{ BEGIN 0; }
<REM>.		;

\"		{ BEGIN STR; _mpsl_string=NULL; _mpsl_string_i=0; }
<STR>\\n	{ _mpsl_add_string(L'\n'); }
<STR>\\t	{ _mpsl_add_string(L'\t'); }
<STR>\\r	{ _mpsl_add_string(L'\r'); }
<STR>\\e	{ _mpsl_add_string((wchar_t) 27); }
<STR>\\\"	{ _mpsl_add_string(L'\"'); }
<STR>\"		{
			_mpsl_add_string(L'\0');
			yylval.v=_mpdm_new_wcs(MPDM_STRING, _mpsl_string,
				_mpsl_string_i - 1, _mpdm_tie_fre());
			BEGIN 0;
			return(STRING);
		}
<STR>.		{
			wchar_t wc;
			mbtowc(&wc, yytext, 1);
			_mpsl_add_string(wc);
		}

.		{ return(*yytext); }

%%

int yywrap(void) { return(1); }

int yy_input_for_flex(char * buf, int max)
{
	char tmp[MB_CUR_MAX + 1];

	if(_mpsl_next_char == NULL || *_mpsl_next_char == L'\0')
		return(0);

	if(wctomb(tmp, *_mpsl_next_char) != 1)
	{
		_mpsl_next_char=NULL;
		return(0);
	}

	_mpsl_next_char ++;

	buf[0]=tmp[0];

	return(1);
}
