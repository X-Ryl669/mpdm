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
		/* return next one */
		v=&_mpdm->nd_pool[_mpdm->nd_index++];

		/* skip to next and wrap */
		if(_mpdm->nd_index >= _mpdm->nd_size)
			_mpdm->nd_index=0;
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
		if(-- _mpdm->nd_index <= 0)
			_mpdm->nd_index = _mpdm->nd_size - 1;
	}
	else
		free(v);
}


/**
 * mpdm_new - Creates a new value.
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 * @tie: array of functions
 *
 * Creates a new value. @flags is an or-ed set of flags, @data is a
 * pointer to the data the value will store, @size the size of these
 * data (if value is to be a multiple one, @size is a number of elements,
 * or a number of bytes otherwise) and @tie an optional multiple
 * value with an array of functions (see tie documentation).
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 */
mpdm_v mpdm_new(int flags, void * data, int size, mpdm_v tie)
{
	mpdm_v v;
	mpdm_v r=NULL;

	/* if high_threshold is overpassed, auto-sweep */
	/* CAUTION: auto-sweep is extremely unsafe by now */
	if(_mpdm->high_threshold && _mpdm->count > _mpdm->high_threshold)
		mpdm_sweep(_mpdm->count - _mpdm->high_threshold);

	/* alloc new value */
	if((v=_mpdm_alloc(flags)) == NULL)
		return(NULL);

	memset(v, '\0', sizeof(struct _mpdm_v));

	v->flags=flags;
	v->data=data;
	v->size=size;

	/* tie (can fail) */
	if((r=mpdm_tie(v, tie)) != NULL)
	{
		if(! (flags & MPDM_NONDYN))
		{
			/* add to the value chain and count */
			if(_mpdm->head == NULL) _mpdm->head=v;
			if(_mpdm->tail != NULL) _mpdm->tail->next=v;
			_mpdm->tail=v;
			_mpdm->count ++;
		}
	}
	else
	{
		/* tie creation failed; free the new value */
		_mpdm_free(v);
	}

	return(r);
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
	if(_mpdm->count > _mpdm->low_threshold)
	{
		int rf=0;
		int uf=0;

		/* if count is -1, sweep all */
		if(count == -1) count=_mpdm->count;

		/* if count is zero, sweep 'some' values */
		if(count == 0) count=16;

		while(count > 0)
		{
			/* is the value referenced? */
			if(_mpdm->head->ref)
			{
				/* yes; rotate to next */
				_mpdm->tail->next=_mpdm->head;
				_mpdm->head=_mpdm->head->next;
				_mpdm->tail=_mpdm->tail->next;
				_mpdm->tail->next=NULL;

				rf++;
				count--;
			}
			else
			{
				mpdm_v v;

				/* value is to be destroyed */
				v=_mpdm->head;
				_mpdm->head=_mpdm->head->next;

				/* untie and destroy */
				mpdm_tie(v, NULL);

				/* free the value itself */
				_mpdm_free(v);

				/* one value less */
				_mpdm->count--;

				uf++;
				count -= 2;
			}
		}

		/* printf("#### SWEEP %d, %d, %d\n", rf, uf, _mpdm->count); */
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
	mpdm_v t;

	/* if a clone tie exists, run it; otherwise, return same value */
	if((t=mpdm_get_tie(v, MPDM_TIE_CLONE)) != NULL)
		v=mpdm_exec(t, v);

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
	mpdm_v (* func1)(mpdm_v);
	mpdm_v (* func2)(mpdm_v, mpdm_v);

	if(c != NULL && (c->flags & MPDM_EXEC))
	{
		mpdm_ref(c);
		mpdm_ref(args);

		if(c->flags & MPDM_MULTIPLE)
		{
			mpdm_v x;

			/* value is multiple; first element is the
			   2 argument version of the executable function,
			   next its optional additional information and
			   finally the arguments */
			x=mpdm_aget(c, 0);
			func2=(mpdm_v (*)(mpdm_v, mpdm_v))(x->data);

			if(func2) r=func2(mpdm_aget(c, 1), args);
		}
		else
		{
			/* value is scalar; c is the 1 argument
			   version of the executable function */
			func1=(mpdm_v (*)(mpdm_v))(c->data);
			if(func1) r=func1(args);
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


/**
 * mpdm_get_tie - Gets a tied function from a value
 * @v: the value
 * @tie_func: the wanted function number
 *
 * Returns the tie function number @tie_func from the value @v.
 */
mpdm_v mpdm_get_tie(mpdm_v v, _mpdm_tie_func tie_func)
{
	mpdm_v t=NULL;

	if(v != NULL && v->tie != NULL)
		t=mpdm_aget(v->tie, tie_func);

	return(t);
}


/**
 * mpdm_tie - Ties an array of functions to a value
 * @v: the value
 * @tie: the array of function ties
 *
 * Ties the array of functions @tie to the value @v. Each of the
 * elements of @tie must be executable.
 */
mpdm_v mpdm_tie(mpdm_v v, mpdm_v tie)
{
	mpdm_v t;

	if(v != NULL)
	{
		/* execute current destroyer */
		if((t=mpdm_get_tie(v, MPDM_TIE_DESTROY)) != NULL)
			mpdm_exec(t, v);

		/* store tie */
		v->tie=tie;

		/* execute new creator */
		if((t=mpdm_get_tie(v, MPDM_TIE_CREATE)) != NULL)
			v=mpdm_exec(t, v);
	}

	return(v);
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
		_mpdm->high_threshold=0;

		/* alloc the non-dyn pool */
		_mpdm->nd_size=16;
		_mpdm->nd_pool=malloc(sizeof(struct _mpdm_v) *
			_mpdm->nd_size);

		/* sets the locale */
		if(setlocale(LC_ALL, "") == NULL)
			setlocale(LC_ALL, "C");
	}

	/* everything went OK */
	return(0);
}


void mpdm_shutdown(void)
{
	mpdm_v v;

	/* travels the complete list of values */
	for(v=_mpdm->head;v != NULL;v=v->next)
	{
		/* destroys all values that need to be destroyed */
		if(v->flags & MPDM_DESTROY)
			mpdm_tie(v, NULL);
	}
}
