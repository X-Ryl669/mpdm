/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    fdm_a.c - Array management

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

#include "fdm.h"

/*******************
	Code
********************/

int _fdm_wrap_offset(fdm_v a, int offset)
{
	/* negative offset means expand from the end */
	if(offset < 0)
		offset=a->size + 1 - offset;

	return(offset);
}


/**
 * fdm_aexpand - Expands an array.
 * @a: the array
 * @offset: insertion offset
 * @num: number of elements to insert
 *
 * Expands an array value, inserting @num elements (initialized
 * to NULL) at the specified @offset.
 */
void fdm_aexpand(fdm_v a, int offset, int num)
{
	int n;
	fdm_v * p;

	/* array is now longer */
	a->size += num;
	p=(fdm_v *) realloc(a->data, a->size * sizeof(fdm_v));

	/* moves up from top of the array */
	for(n=a->size - 1;n >= offset + num;n--)
		p[n]=p[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		p[n]=NULL;

	a->data=p;
}


/**
 * fdm_acollapse - Collapses an array.
 * @a: the array
 * @offset: deletion offset
 * @num: number of elements to collapse
 *
 * Collapses an array value, deleting @num elements at
 * the specified @offset.
 */
void fdm_acollapse(fdm_v a, int offset, int num)
{
	int n;
	fdm_v * p;

	/* don't try to delete beyond the limit */
/*	if(offset + num > a->size)
		num=a->size - offset; */

	/* cleans the about-to-be-deleted space */
	for(n=offset;n < offset + num;n++)
		fdm_aset(a, NULL, n);

	/* array is now shorter */
	a->size -= num;

	/* moves down the elements */
	p=(fdm_v *) a->data;
	for(n=offset;n < a->size;n++)
		p[n]=p[n + num];

	/* finally shrinks the memory block */
	a->data=realloc(p, a->size * sizeof(fdm_v));
}


/**
 * fdm_aset - Sets the value of an array's element.
 * @a: the array
 * @e: the element to be assigned
 * @offset: the subscript of the element
 *
 * Sets the element of the array @a at @offset to be the @e value.
 * Returns the previous element.
 */
fdm_v fdm_aset(fdm_v a, fdm_v e, int offset)
{
	fdm_v v;
	fdm_v * p;

	/* boundary checks */
	if(offset < 0 || offset >= a->size)
		return(NULL);

	p=(fdm_v *)a->data;
	v=p[offset];
	p[offset]=e;

	if(v != NULL) fdm_unref(v);
	if(e != NULL) fdm_ref(e);

	return(v);
}


/**
 * fdm_aget - Gets an element of an array.
 * @a: the array
 * @offset: the subscript of the element
 *
 * Returns the element at @offset of the array @a.
 */
fdm_v fdm_aget(fdm_v a, int offset)
{
	fdm_v * p;

	/* boundary checks */
	if(offset < 0 || offset >= a->size)
		return(NULL);

	p=(fdm_v *)a->data;
	return(p[offset]);
}


/**
 * fdm_ains - Insert an element in an array.
 * @a: the array
 * @e: the element to be inserted
 * @offset: subscript where the element is going to be inserted
 *
 * Inserts the @e value in the @a array at @offset.
 * Further elements are pushed up, so the array increases its size
 * by one.
 */
void fdm_ains(fdm_v a, fdm_v e, int offset)
{
	/* open room */
	fdm_aexpand(a, offset, 1);

	/* set value */
	fdm_aset(a, e, offset);
}


/**
 * fdm_adel - Deletes an element of an array.
 * @a: the array
 * @offset: subscript of the element to be deleted
 *
 * Deletes the element at @offset of the @a array. The array
 * is shrinked by one.
 * Returns the deleted element.
 */
fdm_v fdm_adel(fdm_v a, int offset)
{
	fdm_v v;

	/* gets current value */
	v=fdm_aget(a, offset);

	/* shrinks the array */
	fdm_acollapse(a, offset, 1);

	return(v);
}


/**
 * fdm_apush - Pushes a value into an array.
 * @a: the array
 * @e: the value
 *
 * Pushes a value into an array (i.e. inserts at the end).
 */
void fdm_apush(fdm_v a, fdm_v e)
{
	/* inserts at the end */
	fdm_ains(a, e, a->size);
}


/**
 * fdm_apop - Pops a value from an array.
 * @a: the array
 *
 * Pops a value from the array (i.e. deletes from the end
 * and returns it).
 */
fdm_v fdm_apop(fdm_v a)
{
	/* deletes from the end */
	return(fdm_adel(a, a->size - 1));
}


/**
 * fdm_aqueue - Implements a queue in an array.
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
fdm_v fdm_aqueue(fdm_v a, fdm_v e, int size)
{
	fdm_v v=NULL;

	if(a->size > size - 1)
		v=fdm_adel(a, 0);

	fdm_apush(a, e);
	return(v);
}


/**
 * fdm_aseek - Seeks a value in an array (sequential).
 * @a: the array
 * @k: the key
 * @step: number of elements to step
 *
 * Seeks sequentially the value @k in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 */
int fdm_aseek(fdm_v a, fdm_v k, int step)
{
	int n;

	for(n=0;n < a->size;n+=step)
	{
		fdm_v v=fdm_aget(a, n);

		if(fdm_cmp(v, k) == 0)
			return(n);
	}

	return(-1);
}


/**
 * fdm_abseek - Seeks a value in an array (binary).
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
int fdm_abseek(fdm_v a, fdm_v k, int step, int * pos)
{
	int b, t, n, c;

	b=0; t=(a->size - 1) / step;

	while(t >= b)
	{
		fdm_v v;

		n=(b + t) / 2;
		if((v=fdm_aget(a, n * step)) == NULL)
			break;

		c=fdm_cmp(k, v);

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


static int _fdm_asort_cmp(const void * s1, const void * s2)
{
	return(fdm_cmp(*(fdm_v *)s1, *(fdm_v *)s2));
}


/**
 * fdm_asort - Sorts an array.
 * @a: the array
 * @step: increment step
 *
 * Sorts the array. @step is the number of elements to group together.
 */
void fdm_asort(fdm_v a, int step)
{
	qsort(a->data, a->size / step,
		sizeof(fdm_v) * step, _fdm_asort_cmp);
}


/**
 * fdm_asplit - Separates a string into an array of pieces
 * @s: the separator
 * @v: the value to be separated
 *
 * Separates the @v string value into an array of pieces, using @s
 * as a separator. If the string does not contain the separator,
 * an array holding the complete string is returned.
 */
fdm_v fdm_asplit(fdm_v s, fdm_v v)
{
	fdm_v w;
	char * wrk;
	char * ptr1;
	char * ptr2;

	w=FDM_A(0);

	/* create a working copy */
	wrk=malloc(v->size + 1);
	memcpy(wrk, v->data, v->size + 1);

	/* break string into pieces */
	for(ptr1=ptr2=wrk;*ptr2 != '\0' &&
		(ptr1=strstr(ptr2, s->data)) != NULL;
		ptr2=ptr1 + s->size)
	{
		*ptr1='\0';
		fdm_apush(w, FDM_S(ptr2));
	}

	/* add last part */
	fdm_apush(w, FDM_S(ptr2));

	free(wrk);

	return(w);
}


/**
 * fdm_ajoin - Joins all elements of an array into one
 * @s: joiner string
 * @a: array to be joined
 *
 * Joins all elements from @a into one string, using @s as a glue.
 */
fdm_v fdm_ajoin(fdm_v s, fdm_v a)
{
#ifdef FDM_OPT_RAW_AJOIN
	fdm_v v;
	fdm_v w;
	int n, t;

	/* special case optimization: only one element */
	if(a->size == 1)
		return(fdm_aget(a, 0));

	/* first counts the total size */
	v=fdm_aget(a, 0);
	for(t=v->size,n=1;n < a->size;n++)
	{
		v=fdm_aget(a, n);
		t+=v->size;
	}

	/* if there is a separator, update total size */
	if(s != NULL)
		t += s->size * (a->size - 1);

	/* create the value */
	w=fdm_new(FDM_STRING | FDM_COPY, NULL, t);

	/* copy now */
	v=fdm_aget(a, 0);
	memcpy(w->data, v->data, v->size);
	for(t=v->size,n=1;n < a->size;n++)
	{
		v=fdm_aget(a, n);

		if(s != NULL)
		{
			memcpy(w->data + t, s->data, s->size);
			t+=s->size;
		}

		memcpy(w->data + t, v->data, v->size);
		t+=v->size;
	}

	*((char *)(w->data + t))='\0';
	return(w);
#else
	fdm_v v;
	int n;

	/* unoptimized, but intelligible join */
	v=fdm_aget(a, 0);

	for(n=1;n < a->size;n++)
	{
		/* add separator */
		if(s != NULL)
			v=fdm_strcat(v, s);

		/* add element */
		v=fdm_strcat(v, fdm_aget(a, n));
	}

	return(v);
#endif
}
