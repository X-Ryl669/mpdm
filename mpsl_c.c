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
 * mpsl_local_add_blkframe - Creates a block frame
 *
 * Creates a block frame in the local variable symbol table.
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


/**
 * mpsl_local_add_subframe - Creates a subroutine frame
 *
 * Creates a subroutine frame in the local variable symbol table.
 */
void mpsl_local_add_subframe(void)
{
	/* if local symbol table doesn't exist, create */
	if(_mpsl_local == NULL)
		_mpsl_local=mpdm_ref(MPDM_A(0));

	/* creates a new array for holding the hashes */
	mpdm_apush(_mpsl_local, MPDM_A(0));

	mpsl_local_add_blkframe();
}


/**
 * mpsl_local_del_subframe - Deletes a subroutine frame
 *
 * Deletes a subroutine frame previously created by
 * mpsl_local_add_subframe().
 */
void mpsl_local_del_subframe(void)
{
	mpsl_local_del_blkframe();

	/* simply pops the subframe */
	mpdm_apop(_mpsl_local);
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


void mpsl_local_set_symbols(mpdm_v s, mpdm_v v)
{
	mpdm_v l;

	if(s == NULL)
		return;

	/* gets the top local variable frame */
	l=mpdm_aget(mpdm_aget(_mpsl_local, -1), -1);

	if(s->flags & MPDM_MULTIPLE)
	{
		int n;

		for(n=0;n < mpdm_size(s);n++)
			mpdm_hset(l, mpdm_aget(s, n), mpdm_aget(v, n));
	}
	else
		mpdm_hset(l, s, v);
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

#define C(n) mpdm_aget(c, n)
#define C0 C(0)
#define C1 C(1)

#define M(n) _mpsl_machine(C(n), a)
#define M1 M(1)
#define M2 M(2)
#define M3 M(3)

static mpdm_v _O_multi(mpdm_v c, mpdm_v a) { M1; return(M2); }
static mpdm_v _O_literal(mpdm_v c, mpdm_v a) { return(mpdm_clone(C1)); }
static mpdm_v _O_symval(mpdm_v c, mpdm_v a) { return(mpsl_get_symbol(M1)); }
static mpdm_v _O_assign(mpdm_v c, mpdm_v a) { return(mpsl_set_symbol(M1, M2)); }
static mpdm_v _O_exec(mpdm_v c, mpdm_v a) { return(mpdm_exec(M1, M2)); }
static mpdm_v _O_if(mpdm_v c, mpdm_v a) { return(mpsl_is_true(M1) ? M2 : M3); }
static mpdm_v _O_while(mpdm_v c, mpdm_v a) { while(mpsl_is_true(M1)) M2; return(NULL); }
static mpdm_v _O_local(mpdm_v c, mpdm_v a) { mpsl_local_set_symbols(M1, NULL); return(NULL); }
static mpdm_v _O_uminus(mpdm_v c, mpdm_v a) { return(MPDM_R(-mpdm_rval(M1))); }
static mpdm_v _O_add(mpdm_v c, mpdm_v a) { return(MPDM_R(mpdm_rval(M1) + mpdm_rval(M2))); }
static mpdm_v _O_sub(mpdm_v c, mpdm_v a) { return(MPDM_R(mpdm_rval(M1) - mpdm_rval(M2))); }
static mpdm_v _O_mul(mpdm_v c, mpdm_v a) { return(MPDM_R(mpdm_rval(M1) * mpdm_rval(M2))); }
static mpdm_v _O_div(mpdm_v c, mpdm_v a) { return(MPDM_R(mpdm_rval(M1) / mpdm_rval(M2))); }
static mpdm_v _O_mod(mpdm_v c, mpdm_v a) { return(MPDM_I(mpdm_ival(M1) % mpdm_ival(M2))); }
static mpdm_v _O_not(mpdm_v c, mpdm_v a) { return(mpsl_boolean(! mpsl_is_true(M1))); }
static mpdm_v _O_and(mpdm_v c, mpdm_v a) { mpdm_v r=M1; return(mpsl_is_true(r) ? M2 : r); }
static mpdm_v _O_or(mpdm_v c, mpdm_v a) { mpdm_v r=M1; return(mpsl_is_true(r) ? r : M2); }
static mpdm_v _O_numlt(mpdm_v c, mpdm_v a) { return(mpsl_boolean(mpdm_rval(M1) < mpdm_rval(M2))); }
static mpdm_v _O_numle(mpdm_v c, mpdm_v a) { return(mpsl_boolean(mpdm_rval(M1) <= mpdm_rval(M2))); }
static mpdm_v _O_numgt(mpdm_v c, mpdm_v a) { return(mpsl_boolean(mpdm_rval(M1) > mpdm_rval(M2))); }
static mpdm_v _O_numge(mpdm_v c, mpdm_v a) { return(mpsl_boolean(mpdm_rval(M1) >= mpdm_rval(M2))); }
static mpdm_v _O_strcat(mpdm_v c, mpdm_v a) { return(mpdm_strcat(M1, M2)); }
static mpdm_v _O_streq(mpdm_v c, mpdm_v a) { return(mpsl_boolean(mpdm_cmp(M1, M2) == 0)); }
static mpdm_v _O_immpinc(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) + 1))); }
static mpdm_v _O_immpdec(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) - 1))); }
static mpdm_v _O_immadd(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) + mpdm_rval(M2)))); }
static mpdm_v _O_immsub(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) - mpdm_rval(M2)))); }
static mpdm_v _O_immmul(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) * mpdm_rval(M2)))); }
static mpdm_v _O_immdiv(mpdm_v c, mpdm_v a) { mpdm_v s=M1; return(mpsl_set_symbol(s, MPDM_R(mpdm_rval(mpsl_get_symbol(s)) / mpdm_rval(M2)))); }

static mpdm_v _O_list(mpdm_v c, mpdm_v a)
/* build list from instructions */
{
	int n;
	mpdm_v ret=MPDM_A(mpdm_size(c) - 1);

	for(n=1;n < mpdm_size(c);n++)
		mpdm_aset(ret, M(n), n - 1);

	return(ret);
}


static mpdm_v _O_hash(mpdm_v c, mpdm_v a)
/* build hash from instructions */
{
	int n;
	mpdm_v ret=MPDM_H(0);

	for(n=1;n < mpdm_size(c);n += 2)
		mpdm_hset(ret, M(n), M(n + 1));

	return(ret);
}

static mpdm_v _O_subframe(mpdm_v c, mpdm_v a)
/* runs an instruction inside a subroutine frame */
{
	mpdm_v ret;

	/* creates a subroutine frame */
	mpsl_local_add_subframe();

	/* creates the arguments (if any) as local variables */
	mpsl_local_set_symbols(M2, a);

	/* execute instruction */
	ret=M1;

	/* destroys the frame */
	mpsl_local_del_subframe();

	return(ret);
}


static mpdm_v _O_blkframe(mpdm_v c, mpdm_v a)
/* runs an instruction under a block frame */
{
	mpdm_v ret;

	mpsl_local_add_blkframe();
	ret=M1;
	mpsl_local_del_blkframe();

	return(ret);
}

static mpdm_v _O_immmath(mpdm_v c, mpdm_v a)
/* immediate math operations */
{
	mpsl_op op;
	mpdm_v s;
	mpdm_v v;
	mpdm_v ret = NULL;
	double r, r2=0;

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(C0);

	/* gets the symbol */
	s=M1;

	/* gets the symbol value */
	v=mpsl_get_symbol(s);
	r=mpdm_rval(v);

	/* gets the (optional) second value */
	if(mpdm_size(c) > 2)
		r2=mpdm_rval(M2);

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



static mpdm_v _O_numeq(mpdm_v c, mpdm_v a)
/* numerical and NULL equality test */
{
	mpdm_v v1, v2;

	v1=M1; v2=M2;

	return(mpsl_boolean((v1 == NULL || v2 == NULL) ?
		(v1 == v2) : (mpdm_rval(v1) == mpdm_rval(v2))));
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
mpdm_v _mpsl_machine(mpdm_v c, mpdm_v a)
{
	mpsl_op op;
	mpdm_v ret=NULL;

	if(c == NULL)
		return(NULL);

	/* gets the opcode */
	op=(mpsl_op) mpdm_ival(C0);

	/* the very big switch */
	switch(op)
	{
	case MPSL_OP_MULTI: ret=_O_multi(c, a); break;
	case MPSL_OP_LITERAL: ret=_O_literal(c, a); break;
	case MPSL_OP_LIST: ret=_O_list(c, a); break;
	case MPSL_OP_HASH: ret=_O_hash(c, a); break;
	case MPSL_OP_SYMVAL: ret=_O_symval(c, a); break;
	case MPSL_OP_ASSIGN: ret=_O_assign(c, a); break;
	case MPSL_OP_EXEC: ret=_O_exec(c, a); break;
	case MPSL_OP_IF: ret=_O_if(c, a); break;
	case MPSL_OP_WHILE: ret=_O_while(c, a); break;
	case MPSL_OP_SUBFRAME: ret=_O_subframe(c, a); break;
	case MPSL_OP_BLKFRAME: ret=_O_blkframe(c, a); break;
	case MPSL_OP_LOCAL: ret=_O_local(c, a); break;
	case MPSL_OP_UMINUS: ret=_O_uminus(c, a); break;
	case MPSL_OP_ADD: ret=_O_add(c, a); break;
	case MPSL_OP_SUB: ret=_O_sub(c, a); break;
	case MPSL_OP_MUL: ret=_O_mul(c, a); break;
	case MPSL_OP_DIV: ret=_O_div(c, a); break;
	case MPSL_OP_MOD: ret=_O_mod(c, a); break;
	case MPSL_OP_SINC: /* falls */
	case MPSL_OP_SDEC: ret=_O_immmath(c, a); break;
	case MPSL_OP_PINC: ret=_O_immpinc(c, a); break;
	case MPSL_OP_PDEC: ret=_O_immpdec(c, a); break;
	case MPSL_OP_IADD: ret=_O_immadd(c, a); break;
	case MPSL_OP_ISUB: ret=_O_immsub(c, a); break;
	case MPSL_OP_IMUL: ret=_O_immmul(c, a); break;
	case MPSL_OP_IDIV: ret=_O_immdiv(c, a); break;
	case MPSL_OP_NOT: ret=_O_not(c, a); break;
	case MPSL_OP_AND: ret=_O_and(c, a); break;
	case MPSL_OP_OR: ret=_O_or(c, a); break;
	case MPSL_OP_NUMEQ: ret=_O_numeq(c, a); break;
	case MPSL_OP_NUMLT: ret=_O_numlt(c, a); break;
	case MPSL_OP_NUMLE: ret=_O_numle(c, a); break;
	case MPSL_OP_NUMGT: ret=_O_numgt(c, a); break;
	case MPSL_OP_NUMGE: ret=_O_numge(c, a); break;
	case MPSL_OP_STRCAT: ret=_O_strcat(c, a); break;
	case MPSL_OP_STREQ: ret=_O_streq(c, a); break;
	}

	return(ret);
}
