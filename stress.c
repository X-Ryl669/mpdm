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

void test_hash(void)
{
	fdm_v h;
	fdm_v v;
	int i, n;

	h=FDM_H(0);

	fdm_hset(h, FDM_S("mp"), FDM_I(6));
	v=fdm_hget(h, FDM_S("mp"));

	_test("hash: v != NULL", (v != NULL));
	i=fdm_ival(v);
	_test("hash: v == 6", (i == 6));

	fdm_hset(h, FDM_S("mp2"), FDM_I(66));
	v=fdm_hget(h, FDM_S("mp2"));

	_test("hash: v != NULL", (v != NULL));
	i=fdm_ival(v);
	_test("hash: v == 66", (i == 66));

	/* fills 100 values */
	for(n=0;n < 100;n++)
		fdm_hset(h, FDM_I(n), FDM_I(n * 10));

	/* tests 100 values */
	for(n=0;n < 100;n++)
	{
		v=fdm_hget(h, FDM_I(n));
		_test("hash: hget", (v != NULL));

		if(v != NULL)
		{
			i=fdm_ival(v);
			_test("hash: ival", (i == n * 10));
		}
	}

	fdm_dump(h, 0);
}


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


void test_fdm_sym(void)
{
	fdm_v v;
	int i;

	printf("fdm_sset / fdm_sget tests\n\n");

	fdm_sset(NULL, FDM_LS("mp"), FDM_H(7));
	fdm_sset(NULL, FDM_LS("mp.config"), FDM_H(7));
	fdm_sset(NULL, FDM_LS("mp.config.auto_indent"), FDM_I(16384));
	fdm_sset(NULL, FDM_LS("mp.config.use_regex"), FDM_I(1357));
	fdm_sset(NULL, FDM_LS("mp.config.gtk_font_face"), FDM_LS("profontwindows"));
	fdm_dump(fdm_root(), 0);

	v=fdm_sget(NULL, FDM_LS("mp.config.auto_indent"));
	i=fdm_ival(v);

	_test("auto_indent == 16384", (i == 16384));
}


int main(void)
{
	test_hash();
	test_fdm_asplit();
	test_fdm_sym();

	printf("\n*** Total tests passed: %d/%d\n", oks, tests);

	return(0);
}
