/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

    mpdm_v.c - Basic value management

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
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "mpdm.h"

/*******************
	Data
********************/

/* control structure */

struct _mpdm_ctl * _mpdm=NULL;

/*******************
	Code
********************/

static mpdm_v _mpdm_alloc(int flags)
{
	mpdm_v v=NULL;

	if(flags & MPDM_NONDYN)
	{
		if(_mpdm->nd_size <= _mpdm->nd_index)
		{
			/* if not enough, create more nondyn values */
			_mpdm->nd_size ++;

			_mpdm->nd_pool=realloc(_mpdm->nd_pool,
				sizeof(struct _mpdm_v) * _mpdm->nd_size);
		}

		/* return next one */
		v=&_mpdm->nd_pool[_mpdm->nd_index++];
	}
	else
	{
		/* if it's dynamic, just alloc */
		v=malloc(sizeof(struct _mpdm_v));
	}

	return(v);
}


static void _mpdm_free(mpdm_v v)
{
	if(v->flags & MPDM_NONDYN)
	{
		/* count back */
		_mpdm->nd_index--;
	}
	else
	{
		/* collapse multiple values */
		if(v->flags & MPDM_MULTIPLE)
			mpdm_acollapse(v, 0, v->size);

		/* free data if needed */
		if(v->data != NULL && v->flags & MPDM_FREE)
			free(v->data);

		free(v);
	}
}


/**
 * mpdm_new - Creates a new value.
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @flags is an or-ed set of flags, @data is a
 * pointer to the data the value will store and @size the size of these
 * data (if value is to be a multiple one, @size is a number of elements,
 * or a number of bytes otherwise).
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 */
mpdm_v mpdm_new(int flags, void * data, int size)
{
	mpdm_v v;

	/* alloc new value */
	if((v=_mpdm_alloc(flags)) == NULL)
		return(NULL);

	memset(v, '\0', sizeof(struct _mpdm_v));

	v->flags=flags;
	v->data=data;
	v->size=size;

	if(! (flags & MPDM_NONDYN))
	{
		/* add to the circular list and count */
		if(_mpdm->cur == NULL)
			v->next=v;
		else
		{
			v->next=_mpdm->cur->next;
			_mpdm->cur->next=v;
		}

		_mpdm->cur=v;

		_mpdm->count ++;
	}

	return(v);
}


/**
 * mpdm_ref - Increments the reference count of a value.
 * @v: the value
 *
 * Increments the reference count of a value.
 */
mpdm_v mpdm_ref(mpdm_v v)
{
	if(v != NULL) v->ref++;
	return(v);
}


/**
 * mpdm_unref - Decrements the reference count of a value.
 * @v: the value
 *
 * Decrements the reference count of a value.
 */
mpdm_v mpdm_unref(mpdm_v v)
{
	if(v != NULL) v->ref--;
	return(v);
}


/**
 * mpdm_sweep - Sweeps unreferenced values
 * @count: number of values to be swept
 *
 * Destroys values with a reference count of 0. @count is the
 * number of values to be checked for deletion; special values of
 * @count are -1, that forces a check of all currently known values
 * (can be time-consuming) and 0, which tells mpdm_sweep() to check a
 * small group of them on each call.
 */
void mpdm_sweep(int count)
{
	mpdm_v v;

	/* if count is -1, sweep all */
	if(count == -1) count=_mpdm->count;

	/* if count is zero, sweep 'some' values */
	if(count == 0) count=_mpdm->default_sweep;

	while(count > 0 && _mpdm->count > _mpdm->low_threshold)
	{
		/* takes next value */
		v=_mpdm->cur->next;

		/* is the value referenced? */
		if(! v->ref)
		{
			/* dequeue */
			_mpdm->cur->next=v->next;

			/* free the value itself */
			_mpdm_free(v);

			/* one value less */
			_mpdm->count--;
		}

		/* move to next */
		count --;
		_mpdm->cur=_mpdm->cur->next;
	}
}


/**
 * mpdm_size - Returns the size of an element
 * @v: the element
 *
 * Returns the size of an element.
 */
int mpdm_size(mpdm_v v)
{
	/* NULL values have no size */
	if(v == NULL) return(0);

	return(v->size);
}


/**
 * mpdm_clone - Creates a clone of a value
 * @v: the value
 *
 * Creates a clone of a value. If the value is multiple, a new value will
 * be created containing clones of all its elements; otherwise,
 * the same unchanged value is returned.
 */
mpdm_v mpdm_clone(mpdm_v v)
{
	if(v != NULL)
	{
		if(v->flags & MPDM_MULTIPLE)
			v=_mpdm_aclone(v);
		else
		if(v->flags & MPDM_NONDYN)
			v=MPDM_S(v->data);
	}

	return(v);
}


/**
 * mpdm_root - Returns the root hash.
 *
 * Returns the root hash. This hash is stored internally and can be used
 * as a kind of global symbol table.
 */
mpdm_v mpdm_root(void)
{
	if(_mpdm->root == NULL)
		_mpdm->root=mpdm_ref(MPDM_H(0));

	return(_mpdm->root);
}


/**
 * mpdm_exec - Executes an executable value
 * @c: the code value
 * @args: the arguments
 *
 * Executes an executable value. If @c is a scalar value, its data
 * should be a pointer to a directly executable C function with a
 * prototype of mpdm_v func(mpdm_v args); if it's a multiple one,
 * the first value should be a pointer to a directly executable C
 * function with a prototype of mpdm_v func(mpdm_v c, mpdm_v args).
 * The rest of the elements of @c can be any data, and are used to
 * store bytecode or so when implementing virtual machines or compilers.
 *
 * Returns the return value of the code. If @c is NULL or not executable,
 * returns NULL.
 */
mpdm_v mpdm_exec(mpdm_v c, mpdm_v args)
{
	mpdm_v r=NULL;

	if(c != NULL && (c->flags & MPDM_EXEC))
	{
		mpdm_ref(c);
		mpdm_ref(args);

		if(c->flags & MPDM_MULTIPLE)
		{
			mpdm_v x;
			mpdm_v (* func)(mpdm_v, mpdm_v);

			/* value is multiple; first element is the
			   2 argument version of the executable function,
			   next its optional additional information and
			   finally the arguments */
			x=mpdm_aget(c, 0);

			if((func=(mpdm_v (*)(mpdm_v, mpdm_v))(x->data)) != NULL)
				r=func(mpdm_aget(c, 1), args);
		}
		else
		{
			mpdm_v (* func)(mpdm_v);

			/* value is scalar; c is the 1 argument
			   version of the executable function */
			if((func=(mpdm_v (*)(mpdm_v))(c->data)) != NULL)
				r=func(args);
		}

		mpdm_unref(args);
		mpdm_unref(c);
	}

	return(r);
}


mpdm_v mpdm_exec_2(mpdm_v c, mpdm_v a1, mpdm_v a2)
{
	mpdm_v av[] = { a1, a2 };
	mpdm_v r;

	MPDM_ND_BEGIN();

	r=mpdm_exec(c, MPDM_ND_A(av));

	MPDM_ND_END();

	return(r);
}


mpdm_v mpdm_exec_3(mpdm_v c, mpdm_v a1, mpdm_v a2, mpdm_v a3)
{
	mpdm_v av[] = { a1, a2, a3 };
	mpdm_v r;

	MPDM_ND_BEGIN();

	r=mpdm_exec(c, MPDM_ND_A(av));

	MPDM_ND_END();

	return(r);
}


mpdm_v _mpdm_xnew(mpdm_v (* a1)(mpdm_v, mpdm_v), mpdm_v a2)
{
	mpdm_v x;

	x=MPDM_A(2);
	x->flags |= MPDM_EXEC;

	mpdm_aset(x, MPDM_X(a1), 0);
	mpdm_aset(x, a2, 1);

	return(x);
}


int mpdm_startup(void)
{
	/* do the startup only unless done beforehand */
	if(_mpdm == NULL)
	{
		/* alloc space */
		_mpdm=malloc(sizeof(struct _mpdm_ctl));

		/* cleans it */
		memset(_mpdm, '\0', sizeof(struct _mpdm_ctl));

		/* sets the defaults */
		_mpdm->low_threshold=16;
		_mpdm->default_sweep=16;

		/* sets the locale */
		if(setlocale(LC_ALL, "") == NULL)
			setlocale(LC_ALL, "C");
	}

	/* everything went OK */
	return(0);
}


void mpdm_shutdown(void)
{
}
