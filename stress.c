/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    stress.c - Stress tests for fdm.

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
#include <string.h>

#include "fdm.h"

/* total number of tests and errors */
int tests=0;
int errors=0;


/*******************
	Code
********************/

void _test(char * str, int ok)
{
	printf("%s: %s\n", str, ok ? "OK!" : "Failed");

	tests++;
	if(!ok) errors++;
}


/* tests */

void test_fdm_asplit(void)
{
	fdm_v w;

	w=fdm_asplit(FDM_S("."), FDM_S("this.separated.string"));

	_test("fdm_asplit: number of elements", (w->size == 3));
}


int main(void)
{
	test_fdm_asplit();

	printf("Total tests / errors: %d/%d\n", tests, errors);

	return(0);
}
