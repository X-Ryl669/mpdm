/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    fdm_d.c - Debugging utilities

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

#include "fdm.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

void _fdm_dump(fdm_v v, int l)
{
	int n;
	char * ptr;

	ptr=fdm_string(v);

	/* indent */
	for(n=0;n < l * 2;n++)
		printf(" ");

	if(v == NULL)
	{
		printf("%s\n", ptr);
		return;
	}

	printf("%d,%c%c%c%c:", v->ref,
		v->flags & FDM_COPY	? 'C' : (v->flags & FDM_FREE ? 'F' : '-'),
		v->flags & FDM_STRING	? 'S' : '-',
		v->flags & FDM_HASH	? 'H' : (v->flags & FDM_MULTIPLE ? 'M' : '-'),
		v->flags & FDM_IVAL	? 'I' : '-');

	if(v->flags & FDM_MULTIPLE)
	{
		printf("[%d]\n", v->size);

		for(n=0;n < v->size;n++)
			_fdm_dump(fdm_aget(v, n), l + 1);
	}
	else
		printf("%s\n", ptr);
}


void fdm_dump(fdm_v v)
{
	_fdm_dump(v, 0);
}
