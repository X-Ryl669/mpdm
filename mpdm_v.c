/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

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

#include "mpdm.h"

/*******************
	Data
********************/

/* control structure */

static struct
{
	mpdm_v root;		/* the root hash */
	mpdm_v head;		/* head of values */
	mpdm_v tail;		/* tail of values */
	int count;		/* total count */
	int lcount;		/* last count */
} _mpdm;


/*******************
	Code
********************/

#define _mpdm_alloc() (mpdm_v)malloc(sizeof(struct _mpdm_v))
#define _mpdm_free(v) free(v)


/**
 * mpdm_new - Creates a new value.
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @flags is an or-ed set of flags,
 * @data is a pointer to the data the value will
 * store and @size the size of these data. The flags in @tag define
 * how the data will be stored and its behaviour:
 *
 * If the MPDM_COPY flag is set, the value will store a copy of the data
 * using an allocated block of memory. Otherwise, the @data pointer is
 * stored inside the value as is. MPDM_COPY implies MPDM_FREE.
 *
 * If MPDM_STRING is set, it means @data can be treated as a string
 * (i.e., operations like strcmp() or strlen() can be done on it).
 * Otherwise, @data is treated as an opaque value. For MPDM_STRING
 * values, @size can be -1 to force a calculation using strlen().
 * This flag is incompatible with MPDM_MULTIPLE.
 *
 * IF MPDM_MULTIPLE is set, it means the value itself is an array
 * of values. @Size indicates the number of elements instead of
 * a quantity in bytes. MPDM_MULTIPLE implies MPDM_COPY and not
 * MPDM_STRING, and @data is ignored.
 *
 * If MPDM_FREE is set, memory in @data will be released using free()
 * when the value is destroyed.
 *
 * There are other informative flags that do nothing but describe
 * the information stored in the value: they are MPDM_HASH, MPDM_FILE
 * and MPDM_BINCODE.
 */
mpdm_v mpdm_new(int flags, void * data, int size)
{
	mpdm_v v;

	/* a size of -1 means 'calculate it' */
	if(size == -1)
	{
		/* only calculate size for string values; others get 0 */
		if((flags & MPDM_STRING) && data != NULL)
			size=strlen((char *) data);
		else
			size=0;
	}

	/* alloc new value and init */
	if((v=_mpdm_alloc()) == NULL)
		return(NULL);

	memset(v, '\0', sizeof(struct _mpdm_v));

	v->flags=flags;

	if(flags & MPDM_MULTIPLE)
		mpdm_aexpand(v, 0, size);
	else
	if(flags & MPDM_COPY)
	{
		v->flags |= MPDM_FREE;
		v->size=size;

		/* alloc new space for data */
		if((v->data=malloc(size + 1)) == NULL)
			return(NULL);

		/* zero or copy the block */
		if(data == NULL)
			memset(v->data, '\0', size);
		else
		{
			if(flags & MPDM_STRING)
			{
				strncpy(v->data, data, size);
				((char *)v->data)[size]='\0';
			}
			else
				memcpy(v->data, data, size);
		}
	}
	else
	{
		v->data=data;
		v->size=size;
	}

	/* add to the value chain and count */
	if(_mpdm.head == NULL) _mpdm.head=v;
	if(_mpdm.tail != NULL) _mpdm.tail->next=v;
	_mpdm.tail=v;
	_mpdm.count ++;

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
	/* if it's worthless, don't do it */
	if(_mpdm.count < 16)
	{
		/* store current count, to avoid a sweeping
		   rush when the threshold is crossed */
		_mpdm.lcount=_mpdm.count;
		return;
	}

	/* if count is -1, sweep all */
	if(count == -1) count=_mpdm.count * 2;

	/* if count is zero, sweep 'some' values */
	if(count == 0) count=_mpdm.count - _mpdm.lcount + 2;

	while(count > 0)
	{
		/* is the value referenced? */
		if(_mpdm.head->ref)
		{
			/* yes; rotate to next */
			_mpdm.tail->next=_mpdm.head;
			_mpdm.head=_mpdm.head->next;
			_mpdm.tail=_mpdm.tail->next;
			_mpdm.tail->next=NULL;
		}
		else
		{
			mpdm_v v;

			/* value is to be destroyed */
			v=_mpdm.head;
			_mpdm.head=_mpdm.head->next;

			/* destroy */
			if(v->flags & MPDM_MULTIPLE)
				mpdm_acollapse(v, 0, v->size);
			else
			if(v->flags & MPDM_FREE)
				free(v->data);

			/* free the value itself */
			_mpdm_free(v);

			/* one value less */
			_mpdm.count--;
		}

		count--;
	}

	_mpdm.lcount=_mpdm.count;
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
	mpdm_v w;
	int n;

	/* if NULL or value is not multiple, return as is */
	if(v == NULL || !(v->flags & MPDM_MULTIPLE))
		return(v);

	/* create new */
	w=mpdm_new(v->flags, NULL, v->size);

	/* fills each element with duplicates of the original */
	for(n=0;n < v->size;n++)
		mpdm_aset(w, mpdm_clone(mpdm_aget(v, n)), n);

	return(w);
}


/**
 * mpdm_root - Returns the root hash.
 *
 * Returns the root hash. This hash is stored internally and can be used
 * as a kind of global symbol table.
 */
mpdm_v mpdm_root(void)
{
	if(_mpdm.root == NULL)
	{
		_mpdm.root=MPDM_H(0);
		mpdm_ref(_mpdm.root);
	}

	return(_mpdm.root);
}


/**
 * mpdm_exec - Executes an executable value
 * @c: the code value
 * @args: the arguments
 *
 * Executes an executable value. If @c is a scalar value, it's data
 * should be a pointer to a directly executable C function with a
 * prototype of mpdm_v func(mpdm_v args); if it's a  multiple one,
 * the first value should be the function and the second
 * the arguments (so, in this case, @args will be ignored). Virtual
 * machines and compilers should use the second variant, with a pointer
 * to the executor as the first argument and an array of executable
 * code as the second.
 *
 * Returns the return value of the code. If @c is NULL or not executable,
 * returns NULL.
 */
mpdm_v mpdm_exec(mpdm_v c, mpdm_v args)
{
	mpdm_v r=NULL;
	mpdm_v (* func)(mpdm_v);

	if(c != NULL && (c->flags & MPDM_EXEC))
	{
		if(c->flags & MPDM_MULTIPLE)
		{
			/* if it's multiple, first arg should
			   be the function and second the args */
			r=mpdm_exec(mpdm_aget(c, 0), mpdm_aget(c, 1));
		}
		else
		{
			mpdm_ref(args);

			/* value is a pointer to function */
			func=(mpdm_v (*)(mpdm_v))(c->data);
			if(func) r=func(args);

			mpdm_unref(args);
		}
	}

	return(r);
}
