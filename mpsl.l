%{

/* lex file */

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
