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

void fdm_aexpand(fdm_v v, int offset, int num)
{
	int n;
	fdm_v * a;

	/* negative offset means expand from the end */
	if(offset < 0)
		offset=v->size + 1 - offset;

	/* array is now longer */
	v->size += num;
	a=realloc(v->data, v->size * sizeof(struct _fdm_v));

	/* moves up from top of the array */
	for(n=v->size - 1;v >= offset + num;n--)
		a[n]=a[n - num];

	/* fills the new space with blanks */
	for(;n >= offset;n--)
		a[n]=NULL;

	v->data=a;
}
