/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpsl_c.c - Minimum Profit Scripting Language Core

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

/* array containing the opcodes */
mpdm_v _mpsl_ops=NULL;

/*******************
	Code
********************/

#define OP(o) { mpdm_v v = MPDM_MBS(#o + 8); v->ival=o; \
		v->flags |= MPDM_IVAL ; mpdm_aset(_mpsl_ops, v, o); }

mpdm_v _mpsl_op(mpsl_op opcode)
/* returns an mpdm_v containing the opcode */
{
	if(_mpsl_ops == NULL)
	{
		_mpsl_ops=MPDM_A(0);

		OP(MPSL_OP_MULTI);
		OP(MPSL_OP_LITERAL);
		OP(MPSL_OP_LIST);
		OP(MPSL_OP_HASH);
		OP(MPSL_OP_RANGE);
		OP(MPSL_OP_SYMVAL);
		OP(MPSL_OP_ASSIGN);
		OP(MPSL_OP_EXEC);
		OP(MPSL_OP_WHILE);
		OP(MPSL_OP_IF);
		OP(MPSL_OP_FOREACH);
		OP(MPSL_OP_SUBFRAME);
		OP(MPSL_OP_BLKFRAME);
		OP(MPSL_OP_ARGS);
		OP(MPSL_OP_LOCAL);
		OP(MPSL_OP_UMINUS);
		OP(MPSL_OP_ADD);
		OP(MPSL_OP_SUB);
		OP(MPSL_OP_MUL);
		OP(MPSL_OP_DIV);
		OP(MPSL_OP_MOD);
		OP(MPSL_OP_PINC);
		OP(MPSL_OP_SINC);
		OP(MPSL_OP_PDEC);
		OP(MPSL_OP_SDEC);
		OP(MPSL_OP_IMMADD);
		OP(MPSL_OP_IMMSUB);
		OP(MPSL_OP_IMMMUL);
		OP(MPSL_OP_IMMDIV);
		OP(MPSL_OP_NOT);
		OP(MPSL_OP_AND);
		OP(MPSL_OP_OR);
		OP(MPSL_OP_NUMEQ);
		OP(MPSL_OP_NUMLT);
		OP(MPSL_OP_NUMLE);
		OP(MPSL_OP_NUMGT);
		OP(MPSL_OP_NUMGE);
		OP(MPSL_OP_STREQ);
	}

	return(mpdm_aget(_mpsl_ops, opcode));
}


/**
 * mpsl_is_true - Tests if a value is true
 * @v: the value
 *
 * If @v is a valid mpsl 'false' value (NULL or the "0" string), return
 * zero, or nonzero otherwise.
 */
int mpsl_is_true(mpdm_v v)
{
	/* if value or its data is NULL, it's false */
	if(v == NULL || v->data == NULL)
		return(0);

	/* if it's a printable string... */
	if(v->flags & MPDM_STRING)
	{
		wchar_t * ptr=(wchar_t *)v->data;

		/* ... and it's the "0" string, it's false */
		if(*ptr == L'0' && *(ptr + 1) == L'\0')
			return(0);
	}

	/* any other case is true */
	return(1);
}


/**
 * mpsl_true_or_false - Returns mpsl values true or false
 * @b: boolean selector
 *
 * Returns mpsl's 'false' or 'true' values depending on the value in @b.
 */
mpdm_v mpsl_true_or_false(int b)
{
	static mpdm_v _true=NULL;

	if(_true == NULL)
		_true=MPDM_I(1);

	return(b ? _true : NULL);
}


/** opcodes **/

static mpdm_v _mpsl_op_multi(mpdm_v c, mpdm_v args)
/* multi-instruction */
{
	int n;
	mpdm_v ret=NULL;

	/* executes all following instructions */
	for(n=1;n < mpdm_size(c);n++)
		ret=_mpsl_machine(mpdm_aget(c, n), args);

	return(ret);
}


static mpdm_v _mpsl_op_literal(mpdm_v c)
/* literal value */
{
	return(mpdm_clone(mpdm_aget(c, 1)));
}


static mpdm_v _mpsl_op_list(mpdm_v c, mpdm_v args)
/* build list from instructions */
{
	int n;
	mpdm_v ret=MPDM_A(mpdm_size(c) - 1);

	for(n=1;n < mpdm_size(c);n++)
	{
		mpdm_v v = mpdm_aget(c, n);
		mpdm_aset(ret, _mpsl_machine(v, args), n - 1);
	}

	return(ret);
}


static mpdm_v _mpsl_op_hash(mpdm_v c, mpdm_v args)
/* build hash from instructions */
{
	int n;
	mpdm_v ret=MPDM_H(0);

	for(n=1;n < mpdm_size(c);n += 2)
	{
		mpdm_v k = mpdm_aget(c, n);
		mpdm_v v = mpdm_aget(c, n + 1);

		mpdm_hset(ret, _mpsl_machine(k, NULL),
			_mpsl_machine(v, NULL));
	}

	return(ret);
}


static mpdm_v _mpsl_op_subframe(mpdm_v c, mpdm_v args)
{
	int n;
	mpdm_v ret=NULL;

	/* creates a subroutine frame */
	/* ... */

	/* executes all following instructions */
	for(n=1;n < mpdm_size(c);n++)
		ret=_mpsl_machine(mpdm_aget(c, n), args);

	/* destroys the subroutine frame */
	/* ... */

	return(ret);
}


static mpdm_v _mpsl_op_brmath(mpdm_v c, mpdm_v args)
/* binary, real math operations */
{
	mpsl_op op;
	double r, r1, r2;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	r1=mpdm_rval(_mpsl_machine(mpdm_aget(c, 1), args));
	r2=mpdm_rval(_mpsl_machine(mpdm_aget(c, 2), args));

	switch(op)
	{
	case MPSL_OP_ADD: r = r1 + r2; break;
	case MPSL_OP_SUB: r = r1 - r2; break;
	case MPSL_OP_MUL: r = r1 * r2; break;
	case MPSL_OP_DIV: r = r1 / r2; break;
	default: r=0; break;
	}

	return(MPDM_R(r));
}


static mpdm_v _mpsl_op_bimath(mpdm_v c, mpdm_v args)
/* binary, integer math operations */
{
	mpsl_op op;
	int i, i1, i2;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	i1=mpdm_ival(_mpsl_machine(mpdm_aget(c, 1), args));
	i2=mpdm_ival(_mpsl_machine(mpdm_aget(c, 2), args));

	switch(op)
	{
	case MPSL_OP_MOD: i = i1 % i2; break;
	default: i=0; break;
	}

	return(MPDM_I(i));
}


static mpdm_v _mpsl_op_not(mpdm_v c, mpdm_v args)
/* boolean not */
{
	mpdm_v v;

	v=_mpsl_machine(mpdm_aget(c, 1), args);
	return(mpsl_true_or_false(! mpsl_is_true(v)));
}


static mpdm_v _mpsl_op_and(mpdm_v c, mpdm_v args)
/* boolean 'and' */
{
	mpdm_v v;
	mpdm_v w;
	mpdm_v ret=NULL;

	v=_mpsl_machine(mpdm_aget(c, 1), args);

	if(mpsl_is_true(v))
	{
		w=_mpsl_machine(mpdm_aget(c, 2), args);

		if(mpsl_is_true(w))
			ret=w;
	}

	return(ret);
}


static mpdm_v _mpsl_op_or(mpdm_v c, mpdm_v args)
/* boolean 'or' */
{
	mpdm_v v;
	mpdm_v w;
	mpdm_v ret=NULL;

	v=_mpsl_machine(mpdm_aget(c, 1), args);

	if(mpsl_is_true(v))
		ret=v;
	else
	{
		w=_mpsl_machine(mpdm_aget(c, 2), args);

		if(mpsl_is_true(w))
			ret=w;
	}

	return(ret);
}


static mpdm_v _mpsl_op_nbool(mpdm_v c, mpdm_v args)
/* boolean numeric comparisons */
{
	int i;
	double r1, r2;
	mpsl_op op;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	r1=mpdm_rval(_mpsl_machine(mpdm_aget(c, 1), args));
	r2=mpdm_rval(_mpsl_machine(mpdm_aget(c, 2), args));

	switch(op)
	{
	case MPSL_OP_NUMEQ: i = (r1 == r2); break;
	case MPSL_OP_NUMLT: i = (r1 < r2); break;
	case MPSL_OP_NUMLE: i = (r1 <= r2); break;
	case MPSL_OP_NUMGT: i = (r1 > r2); break;
	case MPSL_OP_NUMGE: i = (r1 >= r2); break;
	default: i = 0; break;
	}

	return(mpsl_true_or_false(i));
}


/**
 * _mpsl_machine - The mpsl virtual machine
 * @c: Multiple value containing the bytecode
 * @args: Optional arguments for the bytecode
 *
 * Executes an instruction (or group of instructions) in the
 * mpsl virtual machine. Usually not called directly, but from an
 * executable value returned by mpsl_compile() and executed by
 * mpdm_exec().
 */
mpdm_v _mpsl_machine(mpdm_v c, mpdm_v args)
{
	mpsl_op op;
	mpdm_v ret=NULL;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	/* the very big switch */
	switch(op)
	{
	case MPSL_OP_MULTI: ret=_mpsl_op_multi(c, args); break;
	case MPSL_OP_LITERAL: ret=_mpsl_op_literal(c); break;
	case MPSL_OP_LIST: ret=_mpsl_op_list(c, args); break;
	case MPSL_OP_HASH: ret=_mpsl_op_hash(c, args); break;
	case MPSL_OP_SUBFRAME: ret=_mpsl_op_subframe(c, args); break;
	case MPSL_OP_ADD:
	case MPSL_OP_SUB:
	case MPSL_OP_MUL:
	case MPSL_OP_DIV: ret=_mpsl_op_brmath(c, args); break;
	case MPSL_OP_MOD: ret=_mpsl_op_bimath(c, args); break;
	case MPSL_OP_NOT: ret=_mpsl_op_not(c, args); break;
	case MPSL_OP_AND: ret=_mpsl_op_and(c, args); break;
	case MPSL_OP_OR: ret=_mpsl_op_or(c, args); break;
	case MPSL_OP_NUMEQ:
	case MPSL_OP_NUMLT:
	case MPSL_OP_NUMLE:
	case MPSL_OP_NUMGT:
	case MPSL_OP_NUMGE: ret=_mpsl_op_nbool(c, args); break;
	}

	return(ret);
}
