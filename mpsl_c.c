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

/* local symbol table */
mpdm_v _mpsl_local=NULL;

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


mpdm_v mpsl_local_add_subframe(void)
{
	/* if local symbol table don't exist, create */
	if(_mpsl_local == NULL)
		_mpsl_local=mpdm_ref(MPDM_A(0));

	/* creates a new array for holding the hashes */
	return(mpdm_apush(_mpsl_local, MPDM_A(0)));
}


void mpsl_local_del_subframe(void)
{
	/* simply pops the subframe */
	mpdm_apop(_mpsl_local);
}


mpdm_v mpsl_local_add_blkframe(void)
{
	/* pushes a new hash onto the last subframe */
	return(mpdm_apush(mpdm_aget(_mpsl_local, -1), MPDM_H(0)));
}


void mpsl_local_del_blkframe(void)
{
	/* simply pops the blkframe */
	mpdm_apop(mpdm_aget(_mpsl_local, -1));
}


mpdm_v mpsl_local_find_symbol(mpdm_v s)
{
	int n;
	mpdm_v l;
	mpdm_v v = NULL;

	l=mpdm_aget(_mpsl_local, -1);

	/* travel the local symbol table trying to find it */
	for(n=mpdm_size(l) - 1;n >=0;n--)
	{
		mpdm_v h = mpdm_aget(l, n);

		if(mpdm_hexists(h, s))
		{
			v=h;
			break;
		}
	}

	return(v);
}


mpdm_v mpsl_set_symbol(mpdm_v s, mpdm_v v)
{
	mpdm_v l;

	if((l=mpsl_local_find_symbol(s)) != NULL)
	{
		mpdm_hset(l, s, v);
		return(v);
	}

	mpdm_sset(NULL, s, v);
	return(v);
}


mpdm_v mpsl_get_symbol(mpdm_v s)
{
	mpdm_v l;

	if((l=mpsl_local_find_symbol(s)) != NULL)
		return(mpdm_hget(l, s));

	return(mpdm_sget(NULL, s));
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


static mpdm_v _mpsl_op_symval(mpdm_v c, mpdm_v args)
/* returns the value stored in a symbol */
{
	return(mpsl_get_symbol(_mpsl_machine(mpdm_aget(c, 1), args)));
}


static mpdm_v _mpsl_op_assign(mpdm_v c, mpdm_v args)
/* assigns value to a symbol */
{
	return(mpsl_set_symbol(_mpsl_machine(mpdm_aget(c, 1), args),
		_mpsl_machine(mpdm_aget(c, 2), args)));
}


static mpdm_v _mpsl_op_exec(mpdm_v c, mpdm_v args)
/* executes an executable value */
{
	mpdm_v v;

	if((v=mpdm_aget(c, 2)) != NULL)
		v=_mpsl_machine(v, args);

	return(mpdm_exec(_mpsl_machine(mpdm_aget(c, 1), args), v));
}


static mpdm_v _mpsl_op_subframe(mpdm_v c, mpdm_v args)
/* runs an instruction under a subroutine frame */
{
	mpdm_v ret=NULL;
	mpdm_v v;
	mpdm_v l;
	int n;

	/* creates subroutine and block frames */
	mpsl_local_add_subframe();
	l=mpsl_local_add_blkframe();

	/* if the instruction has 3 elements, 3rd is the argument list */
	if(mpdm_size(c) > 2)
	{
		v=_mpsl_machine(mpdm_aget(c, 2), NULL);

		/* transfer all arguments with the keys as the
		   symbol names and args as the values */
		for(n=0;n < mpdm_size(args) && n < mpdm_size(v);n++)
			mpdm_hset(l, mpdm_aget(v, n), mpdm_aget(args, n));
	}

	/* execute instruction */
	ret=_mpsl_machine(mpdm_aget(c, 1), args);

	/* destroys the frames */
	mpsl_local_del_subframe();

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
/* boolean 'not' */
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
	mpdm_v v1, v2;
	double r1, r2;
	mpsl_op op;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	v1=_mpsl_machine(mpdm_aget(c, 1), args);
	v2=_mpsl_machine(mpdm_aget(c, 2), args);

	/* special case: NULL equality test */
	if(op == MPSL_OP_NUMEQ && v1 == NULL && v2 == NULL)
		return(mpsl_true_or_false(1));

	r1=mpdm_rval(v1);
	r2=mpdm_rval(v2);

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
	case MPSL_OP_SYMVAL: ret=_mpsl_op_symval(c, args); break;
	case MPSL_OP_ASSIGN: ret=_mpsl_op_assign(c, args); break;
	case MPSL_OP_EXEC: ret=_mpsl_op_exec(c, args); break;
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
