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
	Data
********************/

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


void fdm_aexpand(fdm_v a, int offset, int num)
{
	int n;
	fdm_v * p;

	/* array is now longer */
	a->size += num;
	p=(fdm_v *) realloc(a->data, a->size * sizeof(struct _fdm_v *));

	/* moves up from top of the array */
	for(n=a->size - 1;n >= offset + num;n--)
		p[n]=p[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		p[n]=NULL;

	a->data=p;
}


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
	a->data=realloc(p, a->size * sizeof(struct _fdm_v *));
}


fdm_v fdm_aset(fdm_v a, fdm_v e, int offset)
{
	fdm_v v;
	fdm_v * p;

/*	if(offset >= a->size)
		return(NULL); */

	p=(fdm_v *)a->data;
	v=p[offset];
	p[offset]=e;

	if(v != NULL) fdm_unref(v);
	if(e != NULL) fdm_ref(v);

	return(v);
}


fdm_v fdm_aget(fdm_v a, int offset)
{
	fdm_v * va;

/*	if(offset >= a->size)
		return(NULL); */

	va=(fdm_v *)a->data;
	return(va[offset]);
}
