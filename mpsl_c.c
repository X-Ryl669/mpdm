/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

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

#include "config.h"

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
		OP(MPSL_OP_IADD);
		OP(MPSL_OP_ISUB);
		OP(MPSL_OP_IMUL);
		OP(MPSL_OP_IDIV);
		OP(MPSL_OP_NOT);
		OP(MPSL_OP_AND);
		OP(MPSL_OP_OR);
		OP(MPSL_OP_NUMEQ);
		OP(MPSL_OP_NUMLT);
		OP(MPSL_OP_NUMLE);
		OP(MPSL_OP_NUMGT);
		OP(MPSL_OP_NUMGE);
		OP(MPSL_OP_STRCAT);
		OP(MPSL_OP_STREQ);
	}

	return(mpdm_aget(_mpsl_ops, opcode));
}


/**
 * mpsl_is_true - Tests if a value is true
 * @v: the value
 *
 * If @v is a valid mpsl 'false' value (NULL, "" or the "0" string),
 * returns zero, or nonzero otherwise.
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

		/* ... and it's "" or the "0" string, it's false */
		if(*ptr == L'\0' || (*ptr == L'0' && *(ptr + 1) == L'\0'))
			return(0);
	}

	/* any other case is true */
	return(1);
}


/**
 * mpsl_boolean - Returns mpsl values true or false
 * @b: boolean selector
 *
 * Returns mpsl's 'false' or 'true' values depending on the value in @b.
 */
mpdm_v mpsl_boolean(int b)
{
	static mpdm_v _true=NULL;

	if(_true == NULL)
		_true=mpdm_ref(MPDM_I(1));

	return(b ? _true : NULL);
}


/**
 * mpsl_local_add_subframe - Creates a subroutine frame
 *
 * Creates a subroutine subframe in the local variable symbol table.
 */
mpdm_v mpsl_local_add_subframe(void)
{
	/* if local symbol table don't exist, create */
	if(_mpsl_local == NULL)
		_mpsl_local=mpdm_ref(MPDM_A(0));

	/* creates a new array for holding the hashes */
	return(mpdm_apush(_mpsl_local, MPDM_A(0)));
}


/**
 * mpsl_local_del_subframe - Deletes a subroutine frame
 *
 * Deletes a subroutine frame previously created by
 * mpsl_local_add_subframe().
 */
void mpsl_local_del_subframe(void)
{
	/* simply pops the subframe */
	mpdm_apop(_mpsl_local);
}


/**
 * mpsl_local_add_blkframe - Creates a block frame
 *
 * Creates a block subframe in the local variable symbol table.
 */
mpdm_v mpsl_local_add_blkframe(void)
{
	/* pushes a new hash onto the last subframe */
	return(mpdm_apush(mpdm_aget(_mpsl_local, -1), MPDM_H(0)));
}


/**
 * mpsl_local_del_blkframe - Deletes a block frame
 *
 * Deletes a block frame previously created by
 * mpsl_local_add_blkframe().
 */
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

	/* if s is multiple, take just the first element */
	if(s->flags & MPDM_MULTIPLE)
		s=mpdm_aget(s, 0);

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


/**
 * mpsl_set_symbol - Sets value to a symbol
 * @s: symbol name
 * @v: value
 *
 * Assigns the value @v to the @s symbol. If the value exists as
 * a local symbol, it's assigned to it; otherwise, it's set as a global
 * symbol (and created if it does not exist).
 */
mpdm_v mpsl_set_symbol(mpdm_v s, mpdm_v v)
{
	mpdm_v l;

	if((l=mpsl_local_find_symbol(s)) != NULL)
	{
		if(s->flags & MPDM_MULTIPLE)
			s=mpdm_aget(s, 0);

		mpdm_hset(l, s, v);
		return(v);
	}

	mpdm_sset(NULL, s, v);
	return(v);
}


/**
 * mpsl_get_symbol - Gets the value of a symbol
 * @s: symbol name
 *
 * Gets the value of a symbol. The symbol can be local or global
 * (if the symbol exists in both tables, the local value will be returned).
 */
mpdm_v mpsl_get_symbol(mpdm_v s)
{
	mpdm_v l;

	if((l=mpsl_local_find_symbol(s)) != NULL)
	{
		if(s->flags & MPDM_MULTIPLE)
			s=mpdm_aget(s, 0);

		return(mpdm_hget(l, s));
	}

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


static mpdm_v _mpsl_op_if(mpdm_v c, mpdm_v args)
/* if/then/else structure */
{
	mpdm_v ret=NULL;

	if(mpsl_is_true(_mpsl_machine(mpdm_aget(c, 1), args)))
		ret=_mpsl_machine(mpdm_aget(c, 2), args);
	else
	{
		if(mpdm_size(c) > 3)
			ret=_mpsl_machine(mpdm_aget(c, 3), args);
	}

	return(ret);
}


static mpdm_v _mpsl_op_while(mpdm_v c, mpdm_v args)
/* while structure */
{
	mpdm_v v;
	mpdm_v b;

	v=mpdm_aget(c, 1);
	b=mpdm_aget(c, 2);

	while(mpsl_is_true(_mpsl_machine(v, args)))
		_mpsl_machine(b, args);

	return(NULL);
}


static mpdm_v _mpsl_op_subframe(mpdm_v c, mpdm_v args)
/* runs an instruction inside a subroutine frame */
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


static mpdm_v _mpsl_op_blkframe(mpdm_v c, mpdm_v args)
/* runs an instruction under a block frame */
{
	mpdm_v ret;

	mpsl_local_add_blkframe();

	ret=_mpsl_machine(mpdm_aget(c, 1), args);

	mpsl_local_del_blkframe();

	return(ret);
}


static mpdm_v _mpsl_op_local(mpdm_v c, mpdm_v args)
/* creates a bunch of local variables */
{
	int n;
	mpdm_v l;
	mpdm_v v;

	/* gets current local symbol table */
	l=mpdm_aget(mpdm_aget(_mpsl_local, -1), -1);

	/* gets symbol(s) to be created */
	v=_mpsl_machine(mpdm_aget(c, 1), args);

	if(v->flags & MPDM_MULTIPLE)
	{
		/* creates all of them as NULL values */
		for(n=0;n < mpdm_size(v);n++)
			mpdm_hset(l, mpdm_aget(v, n), NULL);
	}
	else
	{
		/* only one; create it as NULL */
		mpdm_hset(l, v, NULL);
	}

	return(NULL);
}


static mpdm_v _mpsl_op_uminus(mpdm_v c, mpdm_v args)
/* unary minus */
{
	return(MPDM_R(mpdm_rval(_mpsl_machine(mpdm_aget(c, 1), args)) * -1));
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


static mpdm_v _mpsl_op_immmath(mpdm_v c, mpdm_v args)
/* immediate math operations */
{
	mpsl_op op;
	mpdm_v s;
	mpdm_v v;
	mpdm_v ret = NULL;
	double r, r2=0;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(mpdm_aget(c, 0));

	/* gets the symbol */
	s=_mpsl_machine(mpdm_aget(c, 1), args);

	/* gets the symbol value */
	v=mpsl_get_symbol(s);
	r=mpdm_rval(v);

	/* gets the (optional) second value */
	if(mpdm_size(c) > 2)
		r2=mpdm_rval(_mpsl_machine(mpdm_aget(c, 2), args));

	switch(op)
	{
	case MPSL_OP_SINC: r ++; ret=v; break;
	case MPSL_OP_SDEC: r --; ret=v; break;
	case MPSL_OP_PINC: r ++; break;
	case MPSL_OP_PDEC: r --; break;
	case MPSL_OP_IADD: r += r2; break;
	case MPSL_OP_ISUB: r -= r2; break;
	case MPSL_OP_IMUL: r *= r2; break;
	case MPSL_OP_IDIV: r /= r2; break;
	default: r=0; break;
	}

	v=MPDM_R(r);

	/* sets the value */
	mpsl_set_symbol(s, v);

	return(ret == NULL ? v : ret);
}


static mpdm_v _mpsl_op_not(mpdm_v c, mpdm_v args)
/* boolean 'not' */
{
	mpdm_v v;

	v=_mpsl_machine(mpdm_aget(c, 1), args);
	return(mpsl_boolean(! mpsl_is_true(v)));
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
		return(mpsl_boolean(1));

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

	return(mpsl_boolean(i));
}


static mpdm_v _mpsl_op_strcat(mpdm_v c, mpdm_v args)
/* string concatenation */
{
	return(mpdm_strcat(_mpsl_machine(mpdm_aget(c, 1), args),
		_mpsl_machine(mpdm_aget(c, 2), args)));
}


static mpdm_v _mpsl_op_streq(mpdm_v c, mpdm_v args)
/* string comparison */
{
	return(MPDM_I(mpdm_cmp(_mpsl_machine(mpdm_aget(c, 1), args),
		_mpsl_machine(mpdm_aget(c, 2), args))));
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
	case MPSL_OP_IF: ret=_mpsl_op_if(c, args); break;
	case MPSL_OP_WHILE: ret=_mpsl_op_while(c, args); break;
	case MPSL_OP_SUBFRAME: ret=_mpsl_op_subframe(c, args); break;
	case MPSL_OP_BLKFRAME: ret=_mpsl_op_blkframe(c, args); break;
	case MPSL_OP_LOCAL: ret=_mpsl_op_local(c, args); break;
	case MPSL_OP_UMINUS: ret=_mpsl_op_uminus(c, args); break;
	case MPSL_OP_ADD: /* falls */
	case MPSL_OP_SUB: /* falls */
	case MPSL_OP_MUL: /* falls */
	case MPSL_OP_DIV: ret=_mpsl_op_brmath(c, args); break;
	case MPSL_OP_MOD: ret=_mpsl_op_bimath(c, args); break;
	case MPSL_OP_PINC: /* falls */
	case MPSL_OP_PDEC: /* falls */
	case MPSL_OP_SINC: /* falls */
	case MPSL_OP_SDEC: /* falls */
	case MPSL_OP_IADD: /* falls */
	case MPSL_OP_ISUB: /* falls */
	case MPSL_OP_IMUL: /* falls */
	case MPSL_OP_IDIV: ret=_mpsl_op_immmath(c, args); break;
	case MPSL_OP_NOT: ret=_mpsl_op_not(c, args); break;
	case MPSL_OP_AND: ret=_mpsl_op_and(c, args); break;
	case MPSL_OP_OR: ret=_mpsl_op_or(c, args); break;
	case MPSL_OP_NUMEQ: /* falls */
	case MPSL_OP_NUMLT: /* falls */
	case MPSL_OP_NUMLE: /* falls */
	case MPSL_OP_NUMGT: /* falls */
	case MPSL_OP_NUMGE: ret=_mpsl_op_nbool(c, args); break;
	case MPSL_OP_STRCAT: ret=_mpsl_op_strcat(c, args); break;
	case MPSL_OP_STREQ: ret=_mpsl_op_streq(c, args); break;
	}

	return(ret);
}
