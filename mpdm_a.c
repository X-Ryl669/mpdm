/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpdm_a.c - Array management

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
#include <wchar.h>

#include "mpdm.h"


/*******************
	Data
********************/

/* sorting callback code */
static mpdm_v _mpdm_asort_cb=NULL;


/*******************
	Code
********************/

static int _wrap_offset(mpdm_v a, int offset)
/* manages negative offsets */
{
	if(offset < 0) offset=mpdm_size(a) + offset;

	return(offset);
}


/**
 * mpdm_aexpand - Expands an array.
 * @a: the array
 * @offset: insertion offset
 * @num: number of elements to insert
 *
 * Expands an array value, inserting @num elements (initialized
 * to NULL) at the specified @offset.
 */
mpdm_v mpdm_aexpand(mpdm_v a, int offset, int num)
{
	int n;
	mpdm_v * p;

	/* add only if the allocated block exists */
	if(a->data != NULL)
		a->size += num;
	else
		a->size = num;

	p=(mpdm_v *) realloc(a->data, a->size * sizeof(mpdm_v));

	/* moves up from top of the array */
	for(n=a->size - 1;n >= offset + num;n--)
		p[n]=p[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		p[n]=NULL;

	a->data=p;

	return(a);
}


/**
 * mpdm_acollapse - Collapses an array.
 * @a: the array
 * @offset: deletion offset
 * @num: number of elements to collapse
 *
 * Collapses an array value, deleting @num elements at
 * the specified @offset.
 */
mpdm_v mpdm_acollapse(mpdm_v a, int offset, int num)
{
	int n;
	mpdm_v * p;

	/* don't try to delete beyond the limit */
	if(offset + num > a->size)
		num=a->size - offset;

	p=(mpdm_v *) a->data;

	/* unrefs the about-to-be-deleted elements */
	for(n=offset;n < offset + num;n++)
		mpdm_unref(p[n]);

	/* array is now shorter */
	a->size -= num;

	/* moves down the elements */
	for(n=offset;n < a->size;n++)
		p[n]=p[n + num];

	/* finally shrinks the memory block */
	a->data=realloc(p, a->size * sizeof(mpdm_v));

	return(a);
}


/**
 * mpdm_aset - Sets the value of an array's element.
 * @a: the array
 * @e: the element to be assigned
 * @offset: the subscript of the element
 *
 * Sets the element of the array @a at @offset to be the @e value.
 * Returns the previous element.
 */
mpdm_v mpdm_aset(mpdm_v a, mpdm_v e, int offset)
{
	mpdm_v v;
	mpdm_v * p;

	offset=_wrap_offset(a, offset);

	/* boundary checks */
	if(offset < 0)
		return(NULL);

	/* if the array is shorter than offset, expand to make room for it */
	if(offset >= mpdm_size(a))
		mpdm_aexpand(a, mpdm_size(a), offset - mpdm_size(a) + 1);

	p=(mpdm_v *)a->data;

	/* if e is nondyn, store a clone and not the value itself */
	if(e != NULL && e->flags & MPDM_NONDYN)
		e=mpdm_clone(e);

	/* unrefs, refs and assigns */
	v=mpdm_unref(p[offset]);
	p[offset]=mpdm_ref(e);

	return(v);
}


/**
 * mpdm_aget - Gets an element of an array.
 * @a: the array
 * @offset: the subscript of the element
 *
 * Returns the element at @offset of the array @a.
 */
mpdm_v mpdm_aget(mpdm_v a, int offset)
{
	mpdm_v * p;

	offset=_wrap_offset(a, offset);

	/* boundary checks */
	if(offset < 0 || offset >= mpdm_size(a))
		return(NULL);

	p=(mpdm_v *)a->data;
	return(p[offset]);
}


/**
 * mpdm_ains - Insert an element in an array.
 * @a: the array
 * @e: the element to be inserted
 * @offset: subscript where the element is going to be inserted
 *
 * Inserts the @e value in the @a array at @offset.
 * Further elements are pushed up, so the array increases its size
 * by one.
 */
void mpdm_ains(mpdm_v a, mpdm_v e, int offset)
{
	/* open room */
	mpdm_aexpand(a, offset, 1);

	/* set value */
	mpdm_aset(a, e, offset);
}


/**
 * mpdm_adel - Deletes an element of an array.
 * @a: the array
 * @offset: subscript of the element to be deleted
 *
 * Deletes the element at @offset of the @a array. The array
 * is shrinked by one.
 * Returns the deleted element.
 */
mpdm_v mpdm_adel(mpdm_v a, int offset)
{
	mpdm_v v;

	/* gets current value */
	v=mpdm_aget(a, offset);

	/* shrinks the array */
	mpdm_acollapse(a, offset, 1);

	return(v);
}


/**
 * mpdm_apush - Pushes a value into an array.
 * @a: the array
 * @e: the value
 *
 * Pushes a value into an array (i.e. inserts at the end).
 */
void mpdm_apush(mpdm_v a, mpdm_v e)
{
	/* inserts at the end */
	mpdm_ains(a, e, mpdm_size(a));
}


/**
 * mpdm_apop - Pops a value from an array.
 * @a: the array
 *
 * Pops a value from the array (i.e. deletes from the end
 * and returns it).
 */
mpdm_v mpdm_apop(mpdm_v a)
{
	/* deletes from the end */
	return(mpdm_adel(a, -1));
}


/**
 * mpdm_aqueue - Implements a queue in an array.
 * @a: the array
 * @e: the element to be pushed
 * @size: maximum size of array
 *
 * Pushes the @e element into the @a array. If the array already has
 * @size elements, the first (oldest) element is deleted from the
 * queue and returned.
 *
 * Returns the deleted element, or NULL if the array doesn't have
 * @size elements yet.
 */
mpdm_v mpdm_aqueue(mpdm_v a, mpdm_v e, int size)
{
	mpdm_v v=NULL;

	/* a zero size queue is nonsense */
	if(size == 0)
		return(NULL);

	/* loop until a has the desired size */
	while(mpdm_size(a) > size - 1)
		v=mpdm_adel(a, 0);

	mpdm_apush(a, e);
	return(v);
}


/**
 * mpdm_aseek - Seeks a value in an array (sequential).
 * @a: the array
 * @k: the key
 * @step: number of elements to step
 *
 * Seeks sequentially the value @k in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 */
int mpdm_aseek(mpdm_v a, mpdm_v k, int step)
{
	int n;

	for(n=0;n < mpdm_size(a);n+=step)
	{
		mpdm_v v=mpdm_aget(a, n);

		if(mpdm_cmp(v, k) == 0)
			return(n);
	}

	return(-1);
}


/**
 * mpdm_abseek - Seeks a value in an array (binary).
 * @a: the ordered array
 * @k: the key
 * @step: number of elements to step
 * @pos: the position where the element should be, if it's not found
 *
 * Seeks the value @k in the @a array in increments of @step.
 * The array should be sorted to work correctly. A complete search
 * should use a step of 1.
 *
 * If the element is found, returns the offset of the element
 * as a positive number; otherwise, -1 is returned and the position
 * where the element should be is stored in @pos. You can set @pos
 * to NULL if you don't mind.
 */
int mpdm_abseek(mpdm_v a, mpdm_v k, int step, int * pos)
{
	int b, t, n, c;

	b=0; t=(mpdm_size(a) - 1) / step;

	while(t >= b)
	{
		mpdm_v v;

		n=(b + t) / 2;
		if((v=mpdm_aget(a, n * step)) == NULL)
			break;

		c=mpdm_cmp(k, v);

		if(c == 0)
			return(n * step);
		else
		if(c < 0)
			t=n - 1;
		else
			b=n + 1;
	}

	if(pos != NULL) *pos=b * step;
	return(-1);
}


static int _mpdm_asort_cmp(const void * s1, const void * s2)
/* qsort help function */
{
	int ret=0;

	/* if callback is NULL, use basic value comparisons */
	if(_mpdm_asort_cb == NULL)
		ret=mpdm_cmp(*(mpdm_v *)s1, *(mpdm_v *)s2);
	else
	{
		/* executes the callback and converts to integer */
		ret=mpdm_ival(mpdm_exec_2(_mpdm_asort_cb,
			(mpdm_v) * ((mpdm_v *)s1),
			(mpdm_v) * ((mpdm_v *)s2)));
	}

	return(ret);
}


/**
 * mpdm_asort - Sorts an array.
 * @a: the array
 * @step: increment step
 *
 * Sorts the array. @step is the number of elements to group together.
 */
void mpdm_asort(mpdm_v a, int step)
{
	mpdm_asort_cb(a, step, NULL);
}


/**
 * mpdm_asort_cb - Sorts an array with a special sorting function.
 * @a: the array
 * @step: increment step
 * @asort_cb: sorting function
 *
 * Sorts the array. @step is the number of elements to group together.
 * For each pair of elements being sorted, the executable mpdm_v value
 * @sort_cb is called with an array containing the two elements as
 * argument. It must return a signed numerical mpdm_v value indicating
 * the sorting order.
 */
void mpdm_asort_cb(mpdm_v a, int step, mpdm_v asort_cb)
{
	_mpdm_asort_cb=asort_cb;

	qsort(a->data, mpdm_size(a) / step,
		sizeof(mpdm_v) * step, _mpdm_asort_cmp);
}


/**
 * mpdm_asplit - Separates a string into an array of pieces
 * @s: the separator
 * @v: the value to be separated
 *
 * Separates the @v string value into an array of pieces, using @s
 * as a separator. If the string does not contain the separator,
 * an array holding the complete string is returned.
 */
mpdm_v mpdm_asplit(mpdm_v s, mpdm_v v)
{
	mpdm_v w;
	wchar_t * ptr;
	wchar_t * sptr;

	w=MPDM_A(0);

	/* travels the string finding separators and creating new values */
	for(ptr=v->data;
		*ptr != L'\0' && (sptr=wcsstr(ptr, s->data)) != NULL;
		ptr=sptr + mpdm_size(s))
		mpdm_apush(w, MPDM_NS(ptr, sptr - ptr));

	/* add last part */
	mpdm_apush(w, MPDM_S(ptr));

	return(w);
}


/**
 * mpdm_ajoin - Joins all elements of an array into one
 * @s: joiner string
 * @a: array to be joined
 *
 * Joins all elements from @a into one string, using @s as a glue.
 */
mpdm_v mpdm_ajoin(mpdm_v s, mpdm_v a)
{
	mpdm_v v;
	int n;

	/* unoptimized, but intelligible join */
	v=mpdm_aget(a, 0);

	for(n=1;n < mpdm_size(a);n++)
	{
		/* add separator */
		if(s != NULL)
			v=mpdm_strcat(v, s);

		/* add element */
		v=mpdm_strcat(v, mpdm_aget(a, n));
	}

	return(v);
}


/* ties */

static mpdm_v _tie_mul_c(mpdm_v v) { return(mpdm_aexpand(v, 0, v->size)); }
static mpdm_v _tie_mul_d(mpdm_v v) { return(mpdm_acollapse(v, 0, v->size)); }

static mpdm_v _tie_mul_clo(mpdm_v v)
/* clone function for standard multiple values */
{
	mpdm_v w;
	int n;

	/* creates a similar value */
	w=mpdm_new(v->flags, NULL, v->size, v->tie);

	/* fills each element with duplicates of the original */
	for(n=0;n < w->size;n++)
		mpdm_aset(w, mpdm_clone(mpdm_aget(v, n)), n);

	return(w);
}


static mpdm_v _tie_nd_mul_clo(mpdm_v v)
/* clone tie function for nondyn values */
{
	mpdm_v w;
	int n;

	/* creates a standard (dynamic) array with the same size */
	w=MPDM_A(v->size);

	/* fills each element with duplicates of the original */
	for(n=0;n < w->size;n++)
		mpdm_aset(w, mpdm_clone(mpdm_aget(v, n)), n);

	return(w);
}


mpdm_v _mpdm_tie_mul(void)
/* tie for standard multiple values */
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(mpdm_new(MPDM_MULTIPLE, NULL, 0, NULL));
		mpdm_aexpand(_tie, 0, 3);
		mpdm_aset(_tie, MPDM_X(_tie_mul_c), MPDM_TIE_CREATE);
		mpdm_aset(_tie, MPDM_X(_tie_mul_d), MPDM_TIE_DESTROY);
		mpdm_aset(_tie, MPDM_X(_tie_mul_clo), MPDM_TIE_CLONE);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_nd_mul(void)
/* tie for nondyn multiple values */
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		/* only the clone function is used */
		_tie=mpdm_ref(MPDM_A(0));
		mpdm_aset(_tie, MPDM_X(_tie_nd_mul_clo), MPDM_TIE_CLONE);
	}

	return(_tie);
}
