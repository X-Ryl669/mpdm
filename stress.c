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

/* total number of tests and oks */
int tests=0;
int oks=0;


/*******************
	Code
********************/

void _test(char * str, int ok)
{
	printf("%s: %s\n", str, ok ? "OK!" : "*** Failed ***");

	tests++;
	if(ok) oks++;
}


/* tests */

void test_fdm_asplit(void)
{
	fdm_v w;

	printf("fdm_asplit test\n\n");

	w=fdm_asplit(FDM_S("."), FDM_S("four.elems.in.string"));
	fdm_dump(w, 0);
	_test("4 elems: ", (w->size == 4));

	w=fdm_asplit(FDM_S("."), FDM_S("unseparated string"));
	fdm_dump(w, 0);
	_test("1 elem: ", (w->size == 1));

	w=fdm_asplit(FDM_S("."), FDM_S(".dot.at start"));
	fdm_dump(w, 0);
	_test("3 elems: ", (w->size == 3));

	w=fdm_asplit(FDM_S("."), FDM_S("dot.at end."));
	fdm_dump(w, 0);
	_test("3 elems: ", (w->size == 3));

	w=fdm_asplit(FDM_S("."), FDM_S("three...dots (two empty elements)"));
	fdm_dump(w, 0);
	_test("4 elems: ", (w->size == 4));

	w=fdm_asplit(FDM_S("."), FDM_S("."));
	fdm_dump(w, 0);
	_test("2 elems: ", (w->size == 2));
}


int main(void)
{
	test_fdm_asplit();

	printf("\n*** Total tests passed: %d/%d\n", tests, oks);

	return(0);
}
