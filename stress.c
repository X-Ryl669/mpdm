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

void test_basic(void)
{
	int i;
	fdm_v v;

	v=FDM_S("65536");
	i=fdm_ival(v);

	_test("i == 65536", (i == 65536));
	_test("v has FDM_INTEGER", (v->flags & FDM_INTEGER));

	printf("fdm_string: %s\n", fdm_string(FDM_H(0)));
	printf("fdm_string: %s\n", fdm_string(FDM_H(0)));
}


void test_array(void)
{
	fdm_v a;
	fdm_v v;

	a=FDM_A(0);
	_test("a->size == 0", (a->size == 0));

	fdm_apush(a, FDM_LS("sunday"));
	fdm_apush(a, FDM_LS("monday"));
	fdm_apush(a, FDM_LS("tuesday"));
	fdm_apush(a, FDM_LS("wednesday"));
	fdm_apush(a, FDM_LS("thursday"));
	fdm_apush(a, FDM_LS("friday"));
	fdm_apush(a, FDM_LS("saturday"));
	fdm_dump(a);
	_test("a->size == 7", (a->size == 7));

	v=fdm_aset(a, NULL, 3);
	_test("v->ref == 0", (v->ref == 0));
	fdm_dump(a);

	fdm_asort(a, 1);
	_test("NULLs are sorted on top", (fdm_aget(a, 0) == NULL));

	fdm_aset(a, v, 0);
	v=fdm_aget(a, 3);
	_test("v is referenced again", (v->ref > 0));

	fdm_asort(a, 1);
	_test("fdm_asort() works (1)",
		fdm_cmp(fdm_aget(a,0), FDM_LS("friday")) == 0);
	_test("fdm_asort() works (2)",
		fdm_cmp(fdm_aget(a,6), FDM_LS("wednesday")) == 0);

	v=fdm_aget(a, 3);
	fdm_acollapse(a, 3, 1);
	_test("acollapse unrefs values", (v->ref == 0));
}


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
	for(n=0;n < 50;n++)
		fdm_hset(h, FDM_I(n), FDM_I(n * 10));
	for(n=100;n >= 50;n--)
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

	fdm_dump(h);

	/* use of non-strings as hashes */
	h=FDM_H(0);

	v=FDM_A(0);
	fdm_hset(h, v, FDM_I(1234));
	v=FDM_H(0);
	fdm_hset(h, v, FDM_I(12345));
	v=FDM_H(0);
	fdm_hset(h, v, FDM_I(9876));
	v=FDM_A(0);
	fdm_hset(h, v, FDM_I(6543));
	i=fdm_ival(fdm_hget(h, v));

	fdm_dump(h);
	_test("hash: using non-strings as hash keys", (i == 6543));
}


void test_splice(void)
{
	fdm_v w;
	fdm_v v;

	w=fdm_splice(FDM_LS("I'm agent Johnson"), FDM_LS("special "), 4, 0);
	_test("splice insertion", 
		fdm_cmp(fdm_aget(w, 0), FDM_LS("I'm special agent Johnson")) == 0);
	fdm_dump(w);

	w=fdm_splice(FDM_LS("Life is a shit"), FDM_LS("cheat"), 10, 4);
	_test("splice insertion and deletion (1)", 
		fdm_cmp(fdm_aget(w, 0), FDM_LS("Life is a cheat")) == 0);
	_test("splice insertion and deletion (2)", 
		fdm_cmp(fdm_aget(w, 1), FDM_LS("shit")) == 0);
	fdm_dump(w);

	w=fdm_splice(FDM_LS("I'm with dumb"), NULL, 4, 4);
	_test("splice deletion (1)", 
		fdm_cmp(fdm_aget(w, 0), FDM_LS("I'm  dumb")) == 0);
	_test("splice deletion (2)", 
		fdm_cmp(fdm_aget(w, 1), FDM_LS("with")) == 0);
	fdm_dump(w);

	v=FDM_LS("It doesn't matter");
	w=fdm_splice(v, FDM_LS(" two"), v->size, 0);
	_test("splice insertion at the end", 
		fdm_cmp(fdm_aget(w, 0), FDM_LS("It doesn't matter two")) == 0);
	fdm_dump(w);
}


void test_asplit(void)
{
	fdm_v w;

	printf("fdm_asplit test\n\n");

	w=fdm_asplit(FDM_S("."), FDM_S("four.elems.in.string"));
	fdm_dump(w);
	_test("4 elems: ", (w->size == 4));

	w=fdm_asplit(FDM_S("."), FDM_S("unseparated string"));
	fdm_dump(w);
	_test("1 elem: ", (w->size == 1));

	w=fdm_asplit(FDM_S("."), FDM_S(".dot.at start"));
	fdm_dump(w);
	_test("3 elems: ", (w->size == 3));

	w=fdm_asplit(FDM_S("."), FDM_S("dot.at end."));
	fdm_dump(w);
	_test("3 elems: ", (w->size == 3));

	w=fdm_asplit(FDM_S("."), FDM_S("three...dots (two empty elements)"));
	fdm_dump(w);
	_test("4 elems: ", (w->size == 4));

	w=fdm_asplit(FDM_S("."), FDM_S("."));
	fdm_dump(w);
	_test("2 elems: ", (w->size == 2));
}


void test_ajoin(void)
{
	fdm_v v;
	fdm_v s;
	fdm_v w;

	printf("fdm_ajoin test\n\n");

	/* separator */
	s=FDM_LS("--");

	w=FDM_A(1);
	fdm_aset(w, FDM_S("ce"), 0);

	v=fdm_ajoin(NULL, w);
	_test("1 elem, no separator", (fdm_cmp(v, FDM_LS("ce")) == 0));

	v=fdm_ajoin(s, w);
	_test("1 elem, '--' separator", (fdm_cmp(v, FDM_LS("ce")) == 0));

	fdm_apush(w, FDM_LS("n'est"));
	v=fdm_ajoin(s, w);
	_test("2 elems, '--' separator", (fdm_cmp(v, FDM_LS("ce--n'est")) == 0));

	fdm_apush(w, FDM_LS("pas"));
	v=fdm_ajoin(s, w);
	_test("3 elems, '--' separator", (fdm_cmp(v, FDM_LS("ce--n'est--pas")) == 0));

	v=fdm_ajoin(NULL, w);
	_test("3 elems, no separator", (fdm_cmp(v, FDM_LS("cen'estpas")) == 0));
}


void test_sym(void)
{
	fdm_v v;
	int i;

	printf("fdm_sset / fdm_sget tests\n\n");

	fdm_sset(NULL, FDM_LS("mp"), FDM_H(7));
	fdm_sset(NULL, FDM_LS("mp.config"), FDM_H(7));
	fdm_sset(NULL, FDM_LS("mp.config.auto_indent"), FDM_I(16384));
	fdm_sset(NULL, FDM_LS("mp.config.use_regex"), FDM_I(1357));
	fdm_sset(NULL, FDM_LS("mp.config.gtk_font_face"), FDM_LS("profontwindows"));
	fdm_sset(NULL, FDM_LS("mp.lines"), FDM_A(2));
	fdm_sset(NULL, FDM_LS("mp.lines.0"), FDM_LS("First post!"));
	fdm_sset(NULL, FDM_LS("mp.lines.1"), FDM_LS("Second post!"));
	fdm_dump(fdm_root());

	v=fdm_sget(NULL, FDM_LS("mp.config.auto_indent"));
	i=fdm_ival(v);

	_test("auto_indent == 16384", (i == 16384));

	fdm_sweep(-1);
}


int main(void)
{
	test_basic();
	test_array();
	test_hash();
	test_splice();
	test_asplit();
	test_ajoin();
	test_sym();

	printf("\n*** Total tests passed: %d/%d\n", oks, tests);
	printf("*** %s\n", oks == tests ? "ALL TESTS PASSED" : "SOME TESTS ---FAILED---");

	return(0);
}
