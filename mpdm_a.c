/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2006 Angel Ortega <angel@triptico.com>

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
static mpdm_t sort_cb = NULL;


/*******************
	Code
********************/

mpdm_t mpdm_new_a(int flags, int size)
/* creates a new array value */
{
	mpdm_t v;

	/* creates and expands */
	if((v = mpdm_new(flags|MPDM_MULTIPLE, NULL, 0)) != NULL)
		mpdm_expand(v, 0, size);

	return(v);
}


static int wrap_offset(mpdm_t a, int offset)
/* manages negative offsets */
{
	if(offset < 0) offset = mpdm_size(a) + offset;

	return(offset);
}


mpdm_t mpdm_aclone(mpdm_t v)
/* clones a multiple value */
{
	mpdm_t w;
	int n;

	/* creates a similar value */
	w = mpdm_new_a(v->flags, v->size);

	/* fills each element with duplicates of the original */
	for(n=0;n < w->size;n++)
		mpdm_aset(w, mpdm_clone(mpdm_aget(v, n)), n);

	return(w);
}


/* interface */

/**
 * mpdm_expand - Expands an array.
 * @a: the array
 * @offset: insertion offset
 * @num: number of elements to insert
 *
 * Expands an array value, inserting @num elements (initialized
 * to NULL) at the specified @offset.
 * [Arrays]
 */
mpdm_t mpdm_expand(mpdm_t a, int offset, int num)
{
	int n;
	mpdm_t * p;

	/* sanity checks */
	if(num <= 0) return(a);

	/* add size */
	a->size += num;

	/* expand, or fail */
	if((p = (mpdm_t *) realloc(a->data, a->size * sizeof(mpdm_t))) == NULL)
		return(NULL);

	/* moves up from top of the array */
	for(n = a->size - 1;n >= offset + num;n--)
		p[n] = p[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		p[n] = NULL;

	a->data = p;

	return(a);
}


/**
 * mpdm_collapse - Collapses an array.
 * @a: the array
 * @offset: deletion offset
 * @num: number of elements to collapse
 *
 * Collapses an array value, deleting @num elements at
 * the specified @offset.
 * [Arrays]
 */
mpdm_t mpdm_collapse(mpdm_t a, int offset, int num)
{
	int n;
	mpdm_t * p;

	/* sanity checks */
	if(num <= 0) return(a);

	/* don't try to delete beyond the limit */
	if(offset + num > a->size)
		num = a->size - offset;

	p = (mpdm_t *) a->data;

	/* unrefs the about-to-be-deleted elements */
	for(n = offset;n < offset + num;n++)
		mpdm_unref(p[n]);

	/* array is now shorter */
	a->size -= num;

	/* moves down the elements */
	for(n = offset;n < a->size;n++)
		p[n] = p[n + num];

	/* finally shrinks the memory block */
	if((a->data = realloc(p, a->size * sizeof(mpdm_t))) == NULL)
		return(NULL);

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
 * [Arrays]
 */
mpdm_t mpdm_aset(mpdm_t a, mpdm_t e, int offset)
{
	mpdm_t v;
	mpdm_t * p;

	offset = wrap_offset(a, offset);

	/* boundary checks */
	if(offset < 0)
		return(NULL);

	/* if the array is shorter than offset, expand to make room for it */
	if(offset >= mpdm_size(a))
		if(mpdm_expand(a, mpdm_size(a), offset - mpdm_size(a) + 1) == NULL)
			return(NULL);

	p = (mpdm_t *)a->data;

	/* assigns and references */
	v = mpdm_unref(p[offset]);
	p[offset] = mpdm_ref(e);

	return(v);
}


/**
 * mpdm_aget - Gets an element of an array.
 * @a: the array
 * @offset: the subscript of the element
 *
 * Returns the element at @offset of the array @a.
 * [Arrays]
 */
mpdm_t mpdm_aget(mpdm_t a, int offset)
{
	mpdm_t * p;

	offset = wrap_offset(a, offset);

	/* boundary checks */
	if(offset < 0 || offset >= mpdm_size(a))
		return(NULL);

	p = (mpdm_t *) a->data;

	return(p[offset]);
}


/**
 * mpdm_ins - Insert an element in an array.
 * @a: the array
 * @e: the element to be inserted
 * @offset: subscript where the element is going to be inserted
 *
 * Inserts the @e value in the @a array at @offset.
 * Further elements are pushed up, so the array increases its size
 * by one. Returns the inserted element.
 * [Arrays]
 */
mpdm_t mpdm_ins(mpdm_t a, mpdm_t e, int offset)
{
	offset = wrap_offset(a, offset);

	/* open room and set value */
	if(mpdm_expand(a, offset, 1))
		mpdm_aset(a, e, offset);

	return(e);
}


/**
 * mpdm_adel - Deletes an element of an array.
 * @a: the array
 * @offset: subscript of the element to be deleted
 *
 * Deletes the element at @offset of the @a array. The array
 * is shrinked by one. If @offset is negative, is counted from
 * the end of the array (so a value of -1 means delete the
 * last element of the array).
 *
 * Returns the deleted element.
 * [Arrays]
 */
mpdm_t mpdm_adel(mpdm_t a, int offset)
{
	mpdm_t v;

	if(a == NULL || mpdm_size(a) == 0)
		return(NULL);

	offset = wrap_offset(a, offset);

	/* gets current value */
	v = mpdm_aget(a, offset);

	/* shrinks the array */
	mpdm_collapse(a, offset, 1);

	return(v);
}


/**
 * mpdm_shift - Extracts the first element of an array.
 * @a: the array
 *
 * Extracts the first element of the array. The array
 * is shrinked by one.
 *
 * Returns the deleted element.
 * [Arrays]
 */
mpdm_t mpdm_shift(mpdm_t a)
{
	return(mpdm_adel(a, 0));
}


/**
 * mpdm_push - Pushes a value into an array.
 * @a: the array
 * @e: the value
 *
 * Pushes a value into an array (i.e. inserts at the end).
 * [Arrays]
 */
mpdm_t mpdm_push(mpdm_t a, mpdm_t e)
{
	/* inserts at the end */
	return(mpdm_ins(a, e, mpdm_size(a)));
}


/**
 * mpdm_pop - Pops a value from an array.
 * @a: the array
 *
 * Pops a value from the array (i.e. deletes from the end
 * and returns it).
 * [Arrays]
 */
mpdm_t mpdm_pop(mpdm_t a)
{
	/* deletes from the end */
	return(mpdm_adel(a, -1));
}


/**
 * mpdm_queue - Implements a queue in an array.
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
 * [Arrays]
 */
mpdm_t mpdm_queue(mpdm_t a, mpdm_t e, int size)
{
	mpdm_t v=NULL;

	/* zero size is nonsense */
	if(size == 0)
		return(NULL);

	/* loop until a has the desired size */
	while(mpdm_size(a) > size - 1)
		v = mpdm_shift(a);

	mpdm_push(a, e);
	return(v);
}


/**
 * mpdm_seek - Seeks a value in an array (sequential).
 * @a: the array
 * @k: the key
 * @step: number of elements to step
 *
 * Seeks sequentially the value @k in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 * [Arrays]
 */
int mpdm_seek(mpdm_t a, mpdm_t k, int step)
{
	int n;

	/* avoid stupid steps */
	if(step <= 0) step = 1;

	for(n=0;n < mpdm_size(a);n += step)
	{
		mpdm_t v = mpdm_aget(a, n);

		if(mpdm_cmp(v, k) == 0)
			return(n);
	}

	return(-1);
}


/**
 * mpdm_seek_s - Seeks a value in an array (sequential, string version).
 * @a: the array
 * @k: the key
 * @step: number of elements to step
 *
 * Seeks sequentially the value @k in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 * [Arrays]
 */
int mpdm_seek_s(mpdm_t a, wchar_t * k, int step)
{
	return(mpdm_seek(a, MPDM_LS(k), step));
}


/**
 * mpdm_bseek - Seeks a value in an array (binary).
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
 * [Arrays]
 */
int mpdm_bseek(mpdm_t a, mpdm_t k, int step, int * pos)
{
	int b, t, n, c;

	/* avoid stupid steps */
	if(step <= 0) step = 1;

	b=0; t = (mpdm_size(a) - 1) / step;

	while(t >= b)
	{
		mpdm_t v;

		n = (b + t) / 2;
		if((v = mpdm_aget(a, n * step)) == NULL)
			break;

		c = mpdm_cmp(k, v);

		if(c == 0)
			return(n * step);
		else
		if(c < 0)
			t = n - 1;
		else
			b = n + 1;
	}

	if(pos != NULL) *pos = b * step;
	return(-1);
}


/**
 * mpdm_bseek_s - Seeks a value in an array (binary, string version).
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
 * [Arrays]
 */
int mpdm_bseek_s(mpdm_t a, wchar_t * k, int step, int * pos)
{
	return(mpdm_bseek(a, MPDM_LS(k), step, pos));
}


static int sort_cmp(const void * s1, const void * s2)
/* qsort help function */
{
	int ret = 0;

	/* if callback is NULL, use basic value comparisons */
	if(sort_cb == NULL)
		ret = mpdm_cmp(*(mpdm_t *)s1, *(mpdm_t *)s2);
	else
	{
		/* executes the callback and converts to integer */
		ret = mpdm_ival(mpdm_exec_2(sort_cb,
			(mpdm_t) * ((mpdm_t *)s1),
			(mpdm_t) * ((mpdm_t *)s2)));
	}

	return(ret);
}


/**
 * mpdm_sort - Sorts an array.
 * @a: the array
 * @step: increment step
 *
 * Sorts the array. @step is the number of elements to group together.
 *
 * Returns the sorted array (the original one is left untouched).
 * [Arrays]
 */
mpdm_t mpdm_sort(mpdm_t a, int step)
{
	return(mpdm_sort_cb(a, step, NULL));
}


/**
 * mpdm_sort_cb - Sorts an array with a special sorting function.
 * @a: the array
 * @step: increment step
 * @asort_cb: sorting function
 *
 * Sorts the array. @step is the number of elements to group together.
 * For each pair of elements being sorted, the executable mpdm_t value
 * @sort_cb is called with an array containing the two elements as
 * argument. It must return a signed numerical mpdm_t value indicating
 * the sorting order.
 *
 * Returns the sorted array (the original one is left untouched).
 * [Arrays]
 */
mpdm_t mpdm_sort_cb(mpdm_t a, int step, mpdm_t cb)
{
	/* creates a copy to be sorted */
	a = mpdm_clone(a);

	sort_cb = cb;

	/* references the array and the code, as the latter
	   can include anything (including sweeping) */
	mpdm_ref(a);
	mpdm_ref(sort_cb);

	qsort(a->data, mpdm_size(a) / step,
		sizeof(mpdm_t) * step, sort_cmp);

	/* unreferences */
	mpdm_unref(sort_cb);
	mpdm_unref(a);

	sort_cb = NULL;

	return(a);
}


/**
 * mpdm_split - Separates a string into an array of pieces.
 * @s: the separator
 * @v: the value to be separated
 *
 * Separates the @v string value into an array of pieces, using @s
 * as a separator. If the string does not contain the separator,
 * an array holding the complete string is returned.
 * [Arrays]
 * [Strings]
 */
mpdm_t mpdm_split(mpdm_t s, mpdm_t v)
{
	mpdm_t w;
	wchar_t * ptr;
	wchar_t * sptr;

	/* nothing to split? */
	if(v == NULL) return(NULL);

	w = MPDM_A(0);

	/* travels the string finding separators and creating new values */
	for(ptr = v->data;
		*ptr != L'\0' && (sptr = wcsstr(ptr, s->data)) != NULL;
		ptr = sptr + mpdm_size(s))
		mpdm_push(w, MPDM_NS(ptr, sptr - ptr));

	/* add last part */
	mpdm_push(w, MPDM_S(ptr));

	return(w);
}


/**
 * mpdm_join - Joins all elements of an array into one.
 * @s: joiner string
 * @a: array to be joined
 *
 * Joins all elements from @a into one string, using @s as a glue.
 * [Arrays]
 * [Strings]
 */
mpdm_t mpdm_join(mpdm_t s, mpdm_t a)
{
	int n;
	wchar_t * ptr = NULL;
	int l = 0;

	/* adds the first string */
	ptr = mpdm_pokev(ptr, &l, mpdm_aget(a, 0));

	for(n = 1;n < mpdm_size(a);n++)
	{
		/* add separator */
		ptr = mpdm_pokev(ptr, &l, s);

		/* add element */
		ptr = mpdm_pokev(ptr, &l, mpdm_aget(a, n));
	}

	if(ptr == NULL)
		return(NULL);

	ptr = mpdm_poke(ptr, &l, L"", 1, sizeof(wchar_t));
	return(MPDM_ENS(ptr, l - 1));
}
