/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpdm_s.c - String management

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

/**
 * mpdm_string - Returns a printable representation of a value.
 * @v: the value
 *
 * Returns a printable representation of a value. For strings, it's
 * the value data itself; for any other type, a conversion to string
 * is returned instead. This value should be used immediately, as it
 * can be a pointer to a static buffer.
 */
char * mpdm_string(mpdm_v v)
{
	static char tmp[32];

	/* if it's NULL, return a constant */
	if(v == NULL)
		return("[NULL]");

	/* if it's a string, return it */
	if(v->flags & MPDM_STRING)
		return((char *)v->data);

	/* otherwise, convert to printable */
	snprintf(tmp, sizeof(tmp), "%p", v->data);
	return(tmp);
}


/**
 * mpdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the MPDM_STRING flag set,
 * a comparison using strcmp() is returned; otherwise, a
 * simple pointer comparison is done.
 */
int mpdm_cmp(mpdm_v v1, mpdm_v v2)
{
	/* special treatment to NULL values */
	if(v1 == NULL)
		return(-1);
	if(v2 == NULL)
		return(1);

	/* if both values are strings, compare as such */
	if((v1->flags & MPDM_STRING) && (v2->flags & MPDM_STRING))
		return(strcmp((char *)v1->data, (char *)v2->data));

	/* in any other case, compare just pointers */
	return(v1->data - v2->data);
}


/**
 * mpdm_splice - Creates a new string value from another.
 * @v: the original value
 * @i: the value to be inserted
 * @offset: offset where the substring is to be inserted
 * @del: number of characters to delete
 *
 * Creates a new string value from @v, deleting @del chars at @offset
 * and substituting them by @i. If @del is 0, no deletion is done.
 * If @offset is negative, is assumed as counting from the end of @v
 * (so -1 means append at the end). If @v is NULL, @i will become the
 * new string, and both @offset and @del will be ignored. If @v is not
 * NULL and @i is, no insertion process is done (only deletion, if
 * applicable).
 *
 * Returns a two element array, with the new string in the first
 * element and the deleted string in the second (with a NULL value
 * if @del is 0).
 */
mpdm_v mpdm_splice(mpdm_v v, mpdm_v i, int offset, int del)
{
	mpdm_v w;
	mpdm_v n=NULL;
	mpdm_v d=NULL;
	int ns, r;
	int ins=0;

	if(v != NULL)
	{
		/* negative offsets start from the end */
		if(offset < 0) offset=v->size + 1 - offset;

		/* never add further the end */
		if(offset > v->size) offset=v->size;

		/* never delete further the end */
		if(offset + del > v->size) del=v->size - offset;

		/* deleted space */
		if(del > 0)
			d=mpdm_new(MPDM_COPY | MPDM_STRING,
				v->data + offset, del);

		/* something to insert? */
		if(i != NULL)
			ins=i->size;

		/* new size and remainder */
		ns=v->size + ins - del;
		r=offset + del;

		if((n=mpdm_new(MPDM_COPY | MPDM_STRING, NULL, ns)) == NULL)
			return(NULL);

		/* copy the beginning */
		if(offset > 0)
			memcpy(n->data, v->data, offset);

		/* copy the text to be inserted */
		if(ins > 0)
			memcpy(n->data + offset, i->data, ins);

		/* copy the remaining */
		if(v->size - r > 0)
			memcpy(n->data + offset + ins,
				v->data + r, v->size - r);

		/* null terminate */
		((char *)(n->data))[ns]='\0';
	}
	else
		n=i;

	/* creates the output array */
	w=MPDM_A(2);

	mpdm_aset(w, n, 0);
	mpdm_aset(w, d, 1);

	return(w);
}


/**
 * mpdm_strcat - Concatenates two strings
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 */
mpdm_v mpdm_strcat(mpdm_v s1, mpdm_v s2)
{
	mpdm_v v;

	v=mpdm_splice(s1, s2, -1, 0);
	v=mpdm_aget(v, 0);

	return(v);
}


/**
 * mpdm_ival - Returns a value's data as an integer
 * @v: the value
 *
 * Returns a value's data as an integer. If the value is a string,
 * it's converted via sscanf and returned; non-string values have all
 * an ival of 0. The converted integer is cached, so costly string
 * conversions are only done once. Values created with the MPDM_IVAL
 * flag set have its ival cached from the beginning.
 */
int mpdm_ival(mpdm_v v)
{
	if(v == NULL)
		return(0);

	/* if there is no cached integer, calculate it */
	if(!(v->flags & MPDM_IVAL))
	{
		int i=0;

		/* if it's a string, calculate it; other
		   values will have an ival of 0 */
		if(v->flags & MPDM_STRING)
			sscanf((char *)v->data, "%i", &i);

		v->ival=i;
		v->flags |= MPDM_IVAL;
	}

	return(v->ival);
}


mpdm_v _mpdm_inew(int ival)
{
	mpdm_v v;
	char tmp[32];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp) - 1, "%d", ival);

	v=mpdm_new(MPDM_COPY | MPDM_STRING | MPDM_IVAL, tmp, -1);
	v->ival=ival;

	return(v);
}


