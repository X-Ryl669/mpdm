/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    fdm_s.c - String management

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

/**
 * fdm_splice - Creates a new string value from another.
 * @v: the original value
 * @i: the value to be inserted
 * @offset: offset where the substring is to be inserted
 * @del: number of characters to delete
 *
 * Creates a new string value from @v, deleting @del chars
 * at @offset and substituting them by @i.
 *
 * Returns a two element array, with the new string in the first
 * element and the deleted string in the second.
 */
fdm_v fdm_splice(fdm_v v, fdm_v i, int offset, int del)
{
	fdm_v w;
	fdm_v n=NULL;
	fdm_v d=NULL;
	int ns, r;
	int ins=0;

	/* negative offsets start from the end */
	if(offset < 0) offset=v->size + 1 - offset;

	/* never add further the end */
	if(offset > v->size) offset=v->size;

	/* never delete further the end */
	if(offset + del > v->size) del=v->size - offset;

	/* deleted space */
	if(del > 0)
	{
		d=fdm_new(FDM_COPY | FDM_STRING, NULL, del);

		memcpy(d->data, v->data + offset, del);
		((char *)(d->data))[del]='\0';
	}

	/* something to insert? */
	if(i != NULL)
		ins=i->size;

	/* new size and remainder */
	ns=v->size + ins - del;
	r=offset + del;

	if((n=fdm_new(FDM_COPY | FDM_STRING, NULL, ns)) == NULL)
		return(NULL);

	/* copy the beginning */
	if(offset > 0)
		memcpy(n->data, v->data, offset);

	/* copy the text to be inserted */
	if(ins > 0)
		memcpy(n->data + offset, i->data, ins);

	/* copy the remaining */
	if(v->size - r > 0)
		memcpy(n->data + offset + ins, v->data + r, v->size - r);

	/* null terminate */
	((char *)(n->data))[ns]='\0';

	/* creates the output array */
	w=FDM_A(2);

	fdm_aset(w, n, 0);
	fdm_aset(w, d, 1);

	return(w);
}


/**
 * fdm_strcat - Concatenates two strings
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 */
fdm_v fdm_strcat(fdm_v s1, fdm_v s2)
{
	fdm_v v;

	v=fdm_splice(s1, s2, -1, 0);
	v=fdm_aget(v, 0);

	return(v);
}


char * fdm_printable(fdm_v v)
{
	static char tmp[32];

	/* if it's NULL, return a constant */
	if(v == NULL)
		return("[NULL]");

	/* if it's a string, return it */
	if(v->flags & FDM_STRING)
		return((char *)v->data);

	/* otherwise, convert to printable */
	snprintf(tmp, sizeof(tmp), "%p", v->data);
	return(tmp);
}
