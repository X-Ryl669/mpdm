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

	if(v == NULL)
	{
		printf("[NULL]\n");
		return;
	}

	/* indent */
	for(n=0;n < l * 2;n++)
		printf(" ");

	printf("[%d,%08X] ", v->ref, v->tag);

	if(v->tag & FDM_MULTIPLE)
	{
		printf("(\n");

		for(n=0;n < v->size;n++)
			fdm_dump(fdm_aget(v, n), l + 1);

		printf(")\n");
	}
	else
	if(v->tag & FDM_STRING)
	{
		char * ptr=(char *)v->data;

		printf("%s", ptr);
	}
	else
		printf("%p", v->data);

	printf("\n");
}
