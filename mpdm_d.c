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

#include <stdio.h>
#include "fdm.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

void fdm_dump(fdm_v v, int l)
{
	int n;

	/* indent */
	for(n=0;n < l * 2;n++)
		printf(" ");

	if(v == NULL)
	{
		printf("[NULL]\n");
		return;
	}

	printf("%d,[%c%c%c%c],%d:", v->ref,
		v->tag & FDM_COPY     ? 'C' : ' ',
		v->tag & FDM_STRING   ? 'S' : ' ',
		v->tag & FDM_MULTIPLE ? 'M' : ' ',
		v->tag & FDM_INTEGER  ? 'I' : ' ',
		FDM_TYPE(v->tag));

	if(v->tag & FDM_MULTIPLE)
	{
		printf("\n");

		for(n=0;n < v->size;n++)
			fdm_dump(fdm_aget(v, n), l + 1);
	}
	else
	if(v->tag & FDM_STRING)
	{
		char * ptr=(char *)v->data;

		printf("%s\n", ptr);
	}
	else
		printf("%p\n", v->data);
}
