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

#include "mpdm.h"

/*******************
	Code
********************/

int _mpdm_wrap_offset(mpdm_v a, int offset)
{
	/* negative offset means expand from the end */
	if(offset < 0)
		offset=mpdm_size(a) + 1 - offset;

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
void mpdm_aexpand(mpdm_v a, int offset, int num)
{
	int n;
	mpdm_v * p;

	/* array is now longer */
	a->size += num;
	p=(mpdm_v *) realloc(a->data, a->size * sizeof(mpdm_v));

	/* moves up from top of the array */
	for(n=a->size - 1;n >= offset + num;n--)
		p[n]=p[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		p[n]=NULL;

	a->data=p;
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
void mpdm_acollapse(mpdm_v a, int offset, int num)
{
	int n;
	mpdm_v * p;

	/* don't try to delete beyond the limit */
	if(offset + num > a->size)
		num=a->size - offset;

	/* cleans the about-to-be-deleted space */
	for(n=offset;n < offset + num;n++)
		mpdm_aset(a, NULL, n);

	/* array is now shorter */
	a->size -= num;

	/* moves down the elements */
	p=(mpdm_v *) a->data;
	for(n=offset;n < a->size;n++)
		p[n]=p[n + num];

	/* finally shrinks the memory block */
	a->data=realloc(p, a->size * sizeof(mpdm_v));
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

	/* boundary checks */
	if(offset < 0 || offset >= mpdm_size(a))
		return(NULL);

	p=(mpdm_v *)a->data;

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
	return(mpdm_adel(a, mpdm_size(a) - 1));
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
{
	return(mpdm_cmp(*(mpdm_v *)s1, *(mpdm_v *)s2));
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
	char * ptr;
	char * sptr;

	w=MPDM_A(0);

	/* travels the string finding separators and creating new values */
	for(ptr=v->data;
		*ptr != '\0' && (sptr=strstr(ptr, s->data)) != NULL;
		ptr=sptr + mpdm_size(s))
		mpdm_apush(w, mpdm_new(MPDM_COPY|MPDM_STRING, ptr, sptr - ptr));

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
#ifdef MPDM_OPT_RAW_AJOIN
	mpdm_v v;
	mpdm_v w;
	int n, t;

	/* special case optimization: only one element */
	if(mpdm_size(a) == 1)
		return(mpdm_aget(a, 0));

	/* first counts the total size */
	v=mpdm_aget(a, 0);
	for(t=mpdm_size(v),n=1;n < mpdm_size(a);n++)
	{
		v=mpdm_aget(a, n);
		t+=mpdm_size(v);
	}

	/* if there is a separator, update total size */
	if(s != NULL)
		t += mpdm_size(s) * (mpdm_size(a) - 1);

	/* create the value */
	w=mpdm_new(MPDM_STRING | MPDM_COPY, NULL, t);

	/* copy now */
	v=mpdm_aget(a, 0);
	memcpy(w->data, v->data, mpdm_size(v));
	for(t=mpdm_size(v),n=1;n < mpdm_size(a);n++)
	{
		v=mpdm_aget(a, n);

		if(s != NULL)
		{
			memcpy(w->data + t, s->data, mpdm_size(s));
			t += mpdm_size(s);
		}

		memcpy(w->data + t, v->data, mpdm_size(v));
		t+=mpdm_size(v);
	}

	*((char *)(w->data + t))='\0';
	return(w);
#else
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
#endif
}
