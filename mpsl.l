%{

/* lex file */

#include <stdio.h>
#include "fdm.h"

#include <stdlib.h>
#include "y.tab.h"

void yyerror(char * s);
%}

%%

[0-9]+		{
			/* integer */
			yylval.v=FDM_S(yytext);
			return(INTEGER);
		}

\"[^"]*\"	{
			/* string includes quotes; strip them */
			yytext[yyleng - 1]='\0';
			yylval.v=FDM_S(yytext + 1);
			return(STRING);
		}

[-()<>=+*/;{}?:.\[\],]	return(*yytext);

"while"		return WHILE;
"if"		return IF;
"else"		return ELSE;
"sub"		return SUB;
"dump"		return DUMP;

"=="		return NUMEQ;
"!="		return NUMNE;
"eq"		return STREQ;
"ne"		return STRNE;
"=>"		return HASHPAIR;

[A-Za-z_]+	{
			/* symbol name */
			yylval.v=FDM_S(yytext);
			return(SYMBOL);
		}

#.*\n		;	/* ignore comments */

[ \t\n]+	;	/* ignore spaces */

.		yyerror("LEX Unknown character");

%%

int yywrap(void) { return(1); }
