/*

    fdm - Filp Data Manager
    Copyright (C) 2003 Angel Ortega <angel@triptico.com>

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
 * the specified @offset. The elements should be
 * unreferenced beforehand.
 */
void fdm_acollapse(fdm_v a, int offset, int num)
{
	int n;
	fdm_v * p;

	/* don't try to delete beyond the limit */
/*	if(offset + num > a->size)
		num=a->size - offset; */

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

	p=(fdm_v *)a->data;
	v=p[offset];
	p[offset]=e;

	if(v != NULL) fdm_unref(v);
	if(e != NULL) fdm_ref(v);

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

	/* sets a NULL value there */
	v=fdm_aset(a, NULL, offset);

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
	fdm_ains(a, e, a->size - 1);
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
 * fdm_aseek - Seeks a value in an array (sequential)
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

		if(v != NULL && fdm_cmp(v, k)==0)
			return(n);
	}

	return(-1);
}


/**
 * fdm_abseek - Seeks a value in an array (binary)
 * @a: the ordered array
 * @k: the key
 * @step: number of elements to step
 *
 * Seeks the value @k in the @a array in increments of @step.
 * The array should be sorted to work correctly. A complete search
 * should use a step of 1.
 * If the element is found, returns the offset of the element
 * as a positive number; otherwise, if the element is not found,
 * returns the offset where the element should be as a negative number.
 */
int fdm_abseek(fdm_v a, fdm_v k, int step)
{
	int b, t, n, c;

	b=n=0; t=(a->size - 1) / step;

	while(t >= b)
	{
		fdm_v v;

		n=(b + t) / 2;
		if((v=fdm_aget(a, n * step))==NULL)
			break;

		c=fdm_cmp(v, k);

		if(c == 0)
			return(n * step);
		else
		if(c < 0)
			t=n - 1;
		else
			b=n + 1;
	}

	return(-(b * step));
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
