/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpsl.h - Minimum Profit Scripting Language

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

/* the opcodes */

typedef enum
{
	MPSL_OP_MULTI,		/* ; */
	MPSL_OP_LITERAL,	/* literal values */
	MPSL_OP_LIST,		/* build list from instructions */
	MPSL_OP_HASH,		/* build hash from instructions */
	MPSL_OP_RANGE,		/* build range from instructions */

	MPSL_OP_SYMVAL,		/* symbol value */
	MPSL_OP_ASSIGN,		/* assign to symbol */
	MPSL_OP_EXEC,		/* execute executable value */

	MPSL_OP_WHILE,		/* while */
	MPSL_OP_IF,		/* if (or ifelse) */
	MPSL_OP_FOREACH,	/* foreach */
	MPSL_OP_SUBFRAME,	/* subroutine frame */
	MPSL_OP_BLKFRAME,	/* block frame */
	MPSL_OP_LOCAL,		/* create local variables */

	MPSL_OP_UMINUS,		/* unary minus */
	MPSL_OP_ADD,		/* math add */
	MPSL_OP_SUB,		/* math substract */
	MPSL_OP_MUL,		/* math multiply */
	MPSL_OP_DIV,		/* math divide */
	MPSL_OP_MOD,		/* math modulo */

	MPSL_OP_PINC,		/* prefix increment */
	MPSL_OP_SINC,		/* suffix increment */
	MPSL_OP_PDEC,		/* prefix decrement */
	MPSL_OP_SDEC,		/* suffix decrement */
	MPSL_OP_IADD,		/* immediate add */
	MPSL_OP_ISUB,		/* immediate sub */
	MPSL_OP_IMUL,		/* immediate mul */
	MPSL_OP_IDIV,		/* immediate div */

	MPSL_OP_NOT,		/* boolean negation */
	MPSL_OP_AND,		/* boolean and */
	MPSL_OP_OR,		/* boolean or */
	MPSL_OP_NUMEQ,		/* numerical equal */
	MPSL_OP_NUMLT,		/* numerical less than */
	MPSL_OP_NUMLE,		/* numerical less or equal than */
	MPSL_OP_NUMGT,		/* numerical greater than */
	MPSL_OP_NUMGE,		/* numerical greater or equal than */
	MPSL_OP_STREQ		/* string equal */
} mpsl_op;

int mpsl_is_true(mpdm_v v);
mpdm_v mpsl_true_or_false(int b);
mpdm_v _mpsl_machine(mpdm_v c, mpdm_v args);
mpdm_v mpsl_compile(mpdm_v code);
