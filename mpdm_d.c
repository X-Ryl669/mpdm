/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpdm_d.c - Debugging utilities

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

#include "mpdm.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

void _mpdm_dump(mpdm_v v, int l)
{
	int n;
	char * ptr;

	ptr=mpdm_string(v);

	/* indent */
	for(n=0;n < l * 2;n++)
		printf(" ");

	if(v == NULL)
	{
		printf("%s\n", ptr);
		return;
	}

	printf("%d,%c%c%c%c:", v->ref,
		v->flags & MPDM_COPY	? 'C' : (v->flags & MPDM_FREE ? 'F' : '-'),
		v->flags & MPDM_FILE	? 'F' :
			(v->flags & MPDM_STRING	? 'S' :
				(v->flags & MPDM_EXEC ? 'X' : '-')),
		v->flags & MPDM_HASH	? 'H' : (v->flags & MPDM_MULTIPLE ? 'M' : '-'),
		v->flags & MPDM_IVAL	? 'I' : '-');

	if(v->flags & MPDM_MULTIPLE)
	{
		printf("[%d]\n", v->size);

		for(n=0;n < v->size;n++)
			_mpdm_dump(mpdm_aget(v, n), l + 1);
	}
	else
		printf("%s\n", ptr);
}


void mpdm_dump(mpdm_v v)
{
	_mpdm_dump(v, 0);
}
