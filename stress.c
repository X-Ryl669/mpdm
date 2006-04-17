/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2006 Angel Ortega <angel@triptico.com>

    stress.c - Stress tests for MPDM.

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
#include <wchar.h>

#include <time.h>

#include "mpdm.h"

/* total number of tests and oks */
int tests = 0;
int oks = 0;

/* failed tests messages */
char * failed_msgs[5000];
int i_failed_msgs = 0;

int do_benchmarks = 0;
int do_multibyte_sregex_tests = 0;

/*******************
	Code
********************/

void do_test(char * str, int ok)
{
	char tmp[1024];

	sprintf(tmp, "%s: %s\n", str, ok ? "OK!" : "*** Failed ***");
	printf(tmp);

	tests++;

	if(ok)
		oks++;
	else
		failed_msgs[i_failed_msgs ++] = strdup(tmp);
}


/* tests */

void test_basic(void)
{
	int i;
	double r;
	mpdm_t v;
	mpdm_t w;

	v = MPDM_S(L"65536");
	mpdm_dump(v);
	i = mpdm_ival(v);

	do_test("i == 65536", (i == 65536));
	do_test("v has MPDM_IVAL", (v->flags & MPDM_IVAL));

	r = mpdm_rval(v);
	do_test("r == 65536", (r == 65536.0));
	do_test("v has MPDM_RVAL", (v->flags & MPDM_RVAL));

	printf("mpdm_string: %ls\n", mpdm_string(MPDM_H(0)));
	printf("mpdm_string: %ls\n", mpdm_string(MPDM_H(0)));

	/* partial copies of strings */
	v = MPDM_LS(L"this is not America");
	v = MPDM_NS((wchar_t *)v->data + 4, 4);

	do_test("Partial string values", mpdm_cmp(v, MPDM_LS(L" is ")) == 0);

	v = MPDM_S(L"MUAHAHAHA!");
	do_test("Testing mpdm_clone semantics 1", mpdm_clone(v) == v);

	v = MPDM_A(2);
	mpdm_aset(v, MPDM_S(L"evil"), 0);
	mpdm_aset(v, MPDM_S(L"dead"), 1);
	w = mpdm_clone(v);

	do_test("Testing mpdm_clone semantics 2.1", w != v);

	v = mpdm_aget(v, 0);
	do_test("Testing mpdm_clone semantics 2.2", v->ref > 1);
	do_test("Testing mpdm_clone semantics 2.3", mpdm_aget(w, 0) == v);

	/* mbs / wcs tests */
	v = MPDM_MBS("This is (was) a multibyte string");
	mpdm_dump(v);

	/* greek omega is 03a9 */
	v = MPDM_MBS("¡España! (non-ASCII string, as ISO-8859-1 char *)");
	mpdm_dump(v);
	printf("(Previous value will be NULL if locale doesn't match stress.c encoding)\n");

	v = MPDM_LS(L"A capital greek omega between brackets [\x03a9]");
	mpdm_dump(v);
	printf("(Previous value will only show on an Unicode terminal)\n");

	v = MPDM_R(3.1416);
	mpdm_dump(v);
	do_test("rval 1", mpdm_rval(v) == 3.1416);
	do_test("ival 1", mpdm_ival(v) == 3);

	v = MPDM_R(777777.0 / 2.0);
	mpdm_dump(v);
	do_test("mpdm_rnew 1", mpdm_cmp(v, MPDM_LS(L"388888.5")) == 0);

	v = MPDM_R(388888.500);
	mpdm_dump(v);
	do_test("mpdm_rnew 2", mpdm_cmp(v, MPDM_LS(L"388888.5")) == 0);

	v = MPDM_R(388888.412);
	mpdm_dump(v);
	do_test("mpdm_rnew 3", mpdm_cmp(v, MPDM_LS(L"388888.412")) == 0);

	v = MPDM_R(388888.6543);
	mpdm_dump(v);
	do_test("mpdm_rnew 4", mpdm_cmp(v, MPDM_LS(L"388888.6543")) == 0);

	v = MPDM_R(388888.0);
	mpdm_dump(v);
	do_test("mpdm_rnew 5", mpdm_cmp(v, MPDM_LS(L"388888")) == 0);

	v = MPDM_R(0.050000);
	mpdm_dump(v);
	do_test("mpdm_rnew 6", mpdm_cmp(v, MPDM_LS(L"0.05")) == 0);

	v = MPDM_R(0.000);
	mpdm_dump(v);
	do_test("mpdm_rnew 7", mpdm_cmp(v, MPDM_LS(L"0")) == 0);

	v = MPDM_LS(L"0177");
	do_test("mpdm_ival() for octal numbers", mpdm_ival(v) == 0x7f);

	v = MPDM_LS(L"0xFF");
	do_test("mpdm_ival() for hexadecimal numbers", mpdm_ival(v) == 255);

	v = MPDM_LS(L"001");
	do_test("mpdm_rval() for octal numbers", mpdm_rval(v) == 1.0);

	v = MPDM_LS(L"0x7f");
	do_test("mpdm_rval() for hexadecimal numbers", mpdm_ival(v) == 127.0);
}

mpdm_t sort_cb(mpdm_t args)
{
	int d;

	/* sorts reversely */
	d = mpdm_cmp(mpdm_aget(args, 1), mpdm_aget(args, 0));
	return(MPDM_I(d));
}


void test_array(void)
{
	int n;
	mpdm_t a;
	mpdm_t v;

	a = MPDM_A(0);
	do_test("a->size == 0", (a->size == 0));

	mpdm_push(a, MPDM_LS(L"sunday"));
	mpdm_push(a, MPDM_LS(L"monday"));
	mpdm_push(a, MPDM_LS(L"tuesday"));
	mpdm_push(a, MPDM_LS(L"wednesday"));
	mpdm_push(a, MPDM_LS(L"thursday"));
	mpdm_push(a, MPDM_LS(L"friday"));
	mpdm_push(a, MPDM_LS(L"saturday"));
	mpdm_dump(a);
	do_test("a->size == 7", (a->size == 7));

	v = mpdm_aset(a, NULL, 3);
	do_test("v->ref == 0", (v->ref == 0));
	mpdm_dump(a);

	a = mpdm_sort(a, 1);
	do_test("NULLs are sorted on top", (mpdm_aget(a, 0) == NULL));

	mpdm_aset(a, v, 0);
	v = mpdm_aget(a, 3);
	do_test("v is referenced again", (v != NULL && v->ref > 0));

	a = mpdm_sort(a, 1);
	do_test("mpdm_asort() works (1)",
		mpdm_cmp(mpdm_aget(a,0), MPDM_LS(L"friday")) == 0);
	do_test("mpdm_asort() works (2)",
		mpdm_cmp(mpdm_aget(a,6), MPDM_LS(L"wednesday")) == 0);

	/* asort_cb sorts reversely */
	a = mpdm_sort_cb(a, 1, MPDM_X(sort_cb));

	do_test("mpdm_asort_cb() works (1)",
		mpdm_cmp(mpdm_aget(a,6), MPDM_LS(L"friday")) == 0);
	do_test("mpdm_asort_cb() works (2)",
		mpdm_cmp(mpdm_aget(a,0), MPDM_LS(L"wednesday")) == 0);

	n = v->ref;
	v = mpdm_aget(a, 3);
	mpdm_collapse(a, 3, 1);
	do_test("acollapse unrefs values", (v->ref < n));

	/* test queues */
	a = MPDM_A(0);

	/* add several values */
	for(n = 0;n < 10;n++)
		v = mpdm_queue(a, MPDM_I(n), 10);

	do_test("queue should still output NULL", (v == NULL));

	v = mpdm_queue(a, MPDM_I(11), 10);
	do_test("queue should no longer output NULL", (v != NULL));

	v = mpdm_queue(a, MPDM_I(12), 10);
	do_test("queue should return 1", mpdm_ival(v) == 1);
	v = mpdm_queue(a, MPDM_I(13), 10);
	do_test("queue should return 2", mpdm_ival(v) == 2);
	do_test("queue size should be 10", a->size == 10);

	mpdm_dump(a);
	v = mpdm_queue(a, MPDM_I(14), 5);
	mpdm_dump(a);

	do_test("queue size should be 5", a->size == 5);
	do_test("last taken value should be 8", mpdm_ival(v) == 8);

	a = MPDM_A(4);
	mpdm_aset(a, MPDM_I(666), 6000);

	do_test("array should have been automatically expanded",
		mpdm_size(a) == 6001);

	v = mpdm_aget(a, -1);
	do_test("negative offsets in arrays 1", mpdm_ival(v) == 666);

	mpdm_aset(a, MPDM_I(777), -2);
	v = mpdm_aget(a, 5999);
	do_test("negative offsets in arrays 2", mpdm_ival(v) == 777);

	mpdm_push(a, MPDM_I(888));
	v = mpdm_aget(a, -1);
	do_test("negative offsets in arrays 3", mpdm_ival(v) == 888);

	v = MPDM_A(0);
	mpdm_push(v, MPDM_I(100));
	mpdm_pop(v);
}


void test_hash(void)
{
	mpdm_t h;
	mpdm_t v;
	int i, n;

	h = MPDM_H(0);

	do_test("hsize 1", mpdm_hsize(h) == 0);

	mpdm_hset(h, MPDM_S(L"mp"), MPDM_I(6));
	v = mpdm_hget(h, MPDM_S(L"mp"));

	do_test("hsize 2", mpdm_hsize(h) == 1);

	do_test("hash: v != NULL", (v != NULL));
	i = mpdm_ival(v);
	do_test("hash: v == 6", (i == 6));

	mpdm_hset(h, MPDM_S(L"mp2"), MPDM_I(66));
	v = mpdm_hget(h, MPDM_S(L"mp2"));

	do_test("hsize 3", mpdm_hsize(h) == 2);

	do_test("hash: v != NULL", (v != NULL));
	i = mpdm_ival(v);
	do_test("hash: v == 66", (i == 66));

	/* fills 100 values */
	for(n = 0;n < 50;n++)
		mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));
	for(n = 100;n >= 50;n--)
		mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));

	do_test("hsize 4", mpdm_hsize(h) == 103);

	/* tests 100 values */
	for(n = 0;n < 100;n++)
	{
		v = mpdm_hget(h, MPDM_I(n));

		if(v != NULL)
		{
			i = mpdm_ival(v);
			if(!(i == n * 10))
				do_test("hash: ival", (i == n * 10));
		}
		else
			do_test("hash: hget", (v != NULL));
	}

	printf("h's size: %d\n", mpdm_hsize(h));

	v = mpdm_hdel(h, MPDM_LS(L"mp"));
	do_test("hdel", mpdm_ival(v) == 6);
	do_test("hsize 5", mpdm_hsize(h) == 102);

/*
	mpdm_dump(h);

	v=mpdm_hkeys(h);
	mpdm_dump(v);
*/
	/* use of non-strings as hashes */
	h = MPDM_H(0);

	v = MPDM_A(0);
	mpdm_hset(h, v, MPDM_I(1234));
	v = MPDM_H(0);
	mpdm_hset(h, v, MPDM_I(12345));
	v = MPDM_H(0);
	mpdm_hset(h, v, MPDM_I(9876));
	v = MPDM_A(0);
	mpdm_hset(h, v, MPDM_I(6543));
	i = mpdm_ival(mpdm_hget(h, v));

	mpdm_dump(h);
	do_test("hash: using non-strings as hash keys", (i == 6543));

	mpdm_hset(h, MPDM_LS(L"ok"), MPDM_I(666));

	do_test("exists 1", mpdm_exists(h, MPDM_LS(L"ok")));
	do_test("exists 2", ! mpdm_exists(h, MPDM_LS(L"notok")));

	v = mpdm_hget_s(h, L"ok");
	do_test("hget_s 1", mpdm_ival(v) == 666);

	mpdm_hset_s(h, L"ok", MPDM_I(777));

	v = mpdm_hget_s(h, L"ok");
	do_test("hget_s + hset_s", mpdm_ival(v) == 777);
}


void test_splice(void)
{
	mpdm_t w;
	mpdm_t v;

	w = mpdm_splice(MPDM_LS(L"I'm agent Johnson"),
		MPDM_LS(L"special "), 4, 0);
	do_test("splice insertion", 
		mpdm_cmp(mpdm_aget(w, 0),
		MPDM_LS(L"I'm special agent Johnson")) == 0);
	mpdm_dump(w);

	w = mpdm_splice(MPDM_LS(L"Life is a shit"), MPDM_LS(L"cheat"), 10, 4);
	do_test("splice insertion and deletion (1)", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"Life is a cheat")) == 0);
	do_test("splice insertion and deletion (2)", 
		mpdm_cmp(mpdm_aget(w, 1), MPDM_LS(L"shit")) == 0);
	mpdm_dump(w);

	w = mpdm_splice(MPDM_LS(L"I'm with dumb"), NULL, 4, 4);
	do_test("splice deletion (1)", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm  dumb")) == 0);
	do_test("splice deletion (2)", 
		mpdm_cmp(mpdm_aget(w, 1), MPDM_LS(L"with")) == 0);
	mpdm_dump(w);

	v = MPDM_LS(L"It doesn't matter");
	w = mpdm_splice(v, MPDM_LS(L" two"), v->size, 0);
	do_test("splice insertion at the end", 
		mpdm_cmp(mpdm_aget(w, 0),
		MPDM_LS(L"It doesn't matter two")) == 0);
	mpdm_dump(w);

	w = mpdm_splice(NULL, NULL, 0, 0);
	do_test("splice with two NULLS", (mpdm_aget(w, 0) == NULL));

	w = mpdm_splice(NULL, MPDM_LS(L"foo"), 0, 0);
	do_test("splice with first value NULL",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"foo")) == 0));

	w = mpdm_splice(MPDM_LS(L"foo"), NULL, 0, 0);
	do_test("splice with second value NULL",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"foo")) == 0));

	v = MPDM_LS(L"I'm testing");

	w = mpdm_splice(v, NULL, 0, -1);
	do_test("splice with negative del (1)",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"")) == 0));

	w = mpdm_splice(v, NULL, 4, -1);
	do_test("splice with negative del (2)",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm ")) == 0));

	w = mpdm_splice(v, NULL, 4, -2);
	do_test("splice with negative del (3)",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm g")) == 0));

	w = mpdm_splice(v, NULL, 0, -4);
	do_test("splice with negative del (4)",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"ing")) == 0));
	mpdm_dump(mpdm_aget(w, 0));

	w = mpdm_splice(v, NULL, 4, -20);
	do_test("splice with out-of-bounds negative del",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm testing")) == 0));
}


void test_strcat(void)
{
	mpdm_t v;
	mpdm_t w;

	w = MPDM_LS(L"something");

	v = mpdm_strcat(NULL, NULL);
	do_test("mpdm_strcat(NULL, NULL) returns NULL",	v == NULL);

	v = mpdm_strcat(NULL, w);
	do_test("mpdm_strcat(NULL, w) returns w", mpdm_cmp(v, w) == 0);

	v = mpdm_strcat(w, NULL);
	do_test("mpdm_strcat(w, NULL) returns w", mpdm_cmp(v, w) == 0);

	w = MPDM_LS(L"");
	v = mpdm_strcat(NULL, w);
	do_test("mpdm_strcat(NULL, \"\") returns \"\"", mpdm_cmp(v, w) == 0);

	v = mpdm_strcat(w, NULL);
	do_test("mpdm_strcat(\"\", NULL) returns \"\"", mpdm_cmp(v, w) == 0);

	v = mpdm_strcat(w, w);
	do_test("mpdm_strcat(\"\", \"\") returns \"\"", mpdm_cmp(v, w) == 0);

}


void test_split(void)
{
	mpdm_t w;

	printf("mpdm_split test\n\n");

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L"four.elems.in.string"));
	mpdm_dump(w);
	do_test("4 elems: ", (w->size == 4));

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L"unseparated string"));
	mpdm_dump(w);
	do_test("1 elem: ", (w->size == 1));

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L".dot.at start"));
	mpdm_dump(w);
	do_test("3 elems: ", (w->size == 3));

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L"dot.at end."));
	mpdm_dump(w);
	do_test("3 elems: ", (w->size == 3));

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L"three...dots (two empty elements)"));
	mpdm_dump(w);
	do_test("4 elems: ", (w->size == 4));

	w = mpdm_split(MPDM_S(L"."), MPDM_S(L"."));
	mpdm_dump(w);
	do_test("2 elems: ", (w->size == 2));
}


void test_join(void)
{
	mpdm_t v;
	mpdm_t s;
	mpdm_t w;

	printf("mpdm_join test\n\n");

	/* separator */
	s = MPDM_LS(L"--");

	w = MPDM_A(1);
	mpdm_aset(w, MPDM_S(L"ce"), 0);

	v = mpdm_join(NULL, w);
	do_test("1 elem, no separator", (mpdm_cmp(v, MPDM_LS(L"ce")) == 0));

	v = mpdm_join(s, w);
	do_test("1 elem, '--' separator", (mpdm_cmp(v, MPDM_LS(L"ce")) == 0));

	mpdm_push(w, MPDM_LS(L"n'est"));
	v = mpdm_join(s, w);
	do_test("2 elems, '--' separator", (mpdm_cmp(v, MPDM_LS(L"ce--n'est")) == 0));

	mpdm_push(w, MPDM_LS(L"pas"));
	v = mpdm_join(s, w);
	do_test("3 elems, '--' separator", (mpdm_cmp(v, MPDM_LS(L"ce--n'est--pas")) == 0));

	v = mpdm_join(NULL, w);
	do_test("3 elems, no separator", (mpdm_cmp(v, MPDM_LS(L"cen'estpas")) == 0));
}


void test_sym(void)
{
	mpdm_t v;
	int i;

	printf("mpdm_sset / mpdm_sget tests\n\n");

	mpdm_sset(NULL, MPDM_LS(L"mp"), MPDM_H(7));
	mpdm_sset(NULL, MPDM_LS(L"mp.config"), MPDM_H(7));
	mpdm_sset(NULL, MPDM_LS(L"mp.config.auto_indent"), MPDM_I(16384));
	mpdm_sset(NULL, MPDM_LS(L"mp.config.use_regex"), MPDM_I(1357));
	mpdm_sset(NULL, MPDM_LS(L"mp.config.gtk_font_face"), MPDM_LS(L"profontwindows"));
	mpdm_sset(NULL, MPDM_LS(L"mp.lines"), MPDM_A(2));
	mpdm_sset(NULL, MPDM_LS(L"mp.lines.0"), MPDM_LS(L"First post!"));
	mpdm_sset(NULL, MPDM_LS(L"mp.lines.1"), MPDM_LS(L"Second post!"));
	mpdm_dump(mpdm_root());

	v = mpdm_sget(NULL, MPDM_LS(L"mp.config.auto_indent"));
	i = mpdm_ival(v);

	do_test("auto_indent == 16384", (i == 16384));

	mpdm_sweep(-1);
}


void test_file(void)
{
	mpdm_t f;
	mpdm_t v;
	mpdm_t eol = MPDM_LS(L"\n");

	f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"));

	if(f == NULL)
	{
		printf("Can't create test.txt; no further file tests possible.\n");
		return;
	}

	do_test("Create test.txt", f != NULL);

	mpdm_write(f, MPDM_LS(L"0"));
	mpdm_write(f, eol);
	mpdm_write(f, MPDM_LS(L"1"));
	mpdm_write(f, eol);

	/* write WITHOUT eol */
	mpdm_write(f, MPDM_LS(L"2"));

	mpdm_close(f);

	f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r"));

	do_test("test written file 0", mpdm_cmp(mpdm_read(f), MPDM_LS(L"0\n")) == 0);
	do_test("test written file 1", mpdm_cmp(mpdm_read(f), MPDM_LS(L"1\n")) == 0);
	do_test("test written file 2", mpdm_cmp(mpdm_read(f), MPDM_LS(L"2")) == 0);
	do_test("test written file 3", mpdm_read(f) == NULL);

	mpdm_close(f);

	mpdm_unlink(MPDM_LS(L"test.txt"));
	do_test("unlink", mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r")) == NULL);

/*	v=mpdm_glob(MPDM_LS(L"*"));*/
	printf("Glob:\n");
	v = mpdm_glob(NULL);
	mpdm_dump(v);
}


void test_regex(void)
{
	mpdm_t v;
	mpdm_t w;

	v = mpdm_regex(MPDM_LS(L"/[0-9]+/"), MPDM_LS(L"123456"), 0);
	do_test("regex 0", v != NULL);

	v = mpdm_regex(MPDM_LS(L"/[0-9]+/"), MPDM_I(65536), 0);
	do_test("regex 1", v != NULL);

	v = mpdm_regex(MPDM_LS(L"/^[0-9]+$/"), MPDM_LS(L"12345678"), 0);
	do_test("regex 2", v != NULL);

	v = mpdm_regex(MPDM_LS(L"/^[0-9]+$/"), MPDM_I(1), 0);
	do_test("regex 3", v != NULL);

	v = mpdm_regex(MPDM_LS(L"/^[0-9]+$/"), MPDM_LS(L"A12345-678"), 0);
	do_test("regex 4", v == NULL);

	w = MPDM_LS(L"Hell street, 666");
	v = mpdm_regex(MPDM_LS(L"/[0-9]+/"), w, 0);
	do_test("regex 5", mpdm_cmp(v, MPDM_I(666)) == 0);

	mpdm_dump(v);

	v = mpdm_regex(MPDM_LS(L"/regex/"), MPDM_LS(L"CASE-INSENSITIVE REGEX"), 0);
	do_test("regex 6.1 (case sensitive)", v == NULL);

	v = mpdm_regex(MPDM_LS(L"/regex/i"), MPDM_LS(L"CASE-INSENSITIVE REGEX"), 0);
	do_test("regex 6.2 (case insensitive)", v != NULL);
/*
	v=mpdm_regex(MPDM_LS(L"/[A-Z]+/"), MPDM_LS(L"case SENSITIVE regex"), 0);
	do_test("regex 6.3 (case sensitive)", mpdm_cmp(v, MPDM_LS(L"SENSITIVE")) == 0);
*/
	v = mpdm_regex(MPDM_LS(L"/^\\s*/"), MPDM_LS(L"123456"), 0);
	do_test("regex 7", v != NULL);

	v = mpdm_regex(MPDM_LS(L"/^\\s+/"), MPDM_LS(L"123456"), 0);
	do_test("regex 8", v == NULL);

	v = mpdm_regex(MPDM_LS(L"/^\\s+/"), NULL, 0);
	do_test("regex 9 (NULL string to match)", v == NULL);

	/* sregex */

	v = mpdm_sregex(MPDM_LS(L"/A/"),MPDM_LS(L"change all A to A"),
		MPDM_LS(L"E"),0);
	do_test("sregex 0", mpdm_cmp(v, MPDM_LS(L"change all E to A")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/A/g"),MPDM_LS(L"change all A to A"),
		MPDM_LS(L"E"),0);
	do_test("sregex 1", mpdm_cmp(v, MPDM_LS(L"change all E to E")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/A+/g"),MPDM_LS(L"change all AAAAAA to E"),
		MPDM_LS(L"E"),0);
	do_test("sregex 2", mpdm_cmp(v, MPDM_LS(L"change all E to E")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/A+/g"),MPDM_LS(L"change all A A A A A A to E"),
		MPDM_LS(L"E"),0);
	do_test("sregex 3", mpdm_cmp(v, MPDM_LS(L"change all E E E E E E to E")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/A+/g"),MPDM_LS(L"change all AAA A AA AAAAA A AAA to E"),
		MPDM_LS(L"E"),0);
	do_test("sregex 3.2", mpdm_cmp(v, MPDM_LS(L"change all E E E E E E to E")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/[0-9]+/g"),MPDM_LS(L"1, 20, 333, 40 all are numbers"),
		MPDM_LS(L"numbers"),0);
	do_test("sregex 4", mpdm_cmp(v, MPDM_LS(L"numbers, numbers, numbers, numbers all are numbers")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/[a-zA-Z_]+/g"),MPDM_LS(L"regex, mpdm_regex, TexMex"),
		MPDM_LS(L"sex"),0);
	do_test("sregex 5", mpdm_cmp(v, MPDM_LS(L"sex, sex, sex")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/[a-zA-Z]+/g"),MPDM_LS(L"regex, mpdm_regex, TexMex"),
		NULL,0);
	do_test("sregex 6", mpdm_cmp(v, MPDM_LS(L", _, ")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/\\\\/g"),MPDM_LS(L"\\MSDOS\\style\\path"),
		MPDM_LS(L"/"),0);
	do_test("sregex 7", mpdm_cmp(v, MPDM_LS(L"/MSDOS/style/path")) == 0);

	v = mpdm_sregex(MPDM_LS(L"/regex/gi"),MPDM_LS(L"regex, Regex, REGEX"),
		MPDM_LS(L"sex"),0);
	do_test("sregex 8", mpdm_cmp(v, MPDM_LS(L"sex, sex, sex")) == 0);

	v = mpdm_sregex(NULL, NULL, NULL, 0);
	do_test("Previous sregex substitutions must be 3", mpdm_ival(v) == 3);

	/* multiple regex tests */
	w = MPDM_A(0);

	mpdm_push(w, MPDM_LS(L"/^[ \t]*/"));
	mpdm_push(w, MPDM_LS(L"/[^ \t=]+/"));
	mpdm_push(w, MPDM_LS(L"/[ \t]*=[ \t]*/"));
	mpdm_push(w, MPDM_LS(L"/[^ \t]+/"));
	mpdm_push(w, MPDM_LS(L"/[ \t]*$/"));

	v = mpdm_regex(w, MPDM_LS(L"key=value"), 0);
	do_test("multi-regex 1.1", mpdm_cmp(mpdm_aget(v, 1),MPDM_LS(L"key")) == 0);
	do_test("multi-regex 1.2", mpdm_cmp(mpdm_aget(v, 3),MPDM_LS(L"value")) == 0);

	v = mpdm_regex(w, MPDM_LS(L" key = value"), 0);
	do_test("multi-regex 2.1", mpdm_cmp(mpdm_aget(v, 1),MPDM_LS(L"key")) == 0);
	do_test("multi-regex 2.2", mpdm_cmp(mpdm_aget(v, 3),MPDM_LS(L"value")) == 0);

	v = mpdm_regex(w, MPDM_LS(L"\t\tkey\t=\tvalue  "), 0);
	do_test("multi-regex 3.1", mpdm_cmp(mpdm_aget(v, 1),MPDM_LS(L"key")) == 0);
	do_test("multi-regex 3.2", mpdm_cmp(mpdm_aget(v, 3),MPDM_LS(L"value")) == 0);

	v = mpdm_regex(w, MPDM_LS(L"key= "), 0);
	do_test("multi-regex 4", v == NULL);

	printf("Multiple line regexes\n");
	w = MPDM_LS(L"/* this is\na C-like comment */");
	v = mpdm_regex(MPDM_LS(L"|/\\*.+\\*/|"), w, 0);
	do_test("Multiline regex 1", mpdm_cmp(v, w) == 0);

	v = mpdm_regex(MPDM_LS(L"/is$/"), w, 0);
	do_test("Multiline regex 2", v == NULL);

	v = mpdm_regex(MPDM_LS(L"/is$/m"), w, 0);
	do_test("Multiline regex 3", mpdm_cmp(v, MPDM_LS(L"is")) == 0);

	printf("Pitfalls on multibyte locales (f.e. utf-8)\n");

	w = MPDM_LS(L"-\x03a9-");

	v = mpdm_regex(MPDM_LS(L"/-$/"), w, 0);
	do_test("Multibyte environment regex 1",
		mpdm_cmp(v, MPDM_LS(L"-")) == 0);

	if(do_multibyte_sregex_tests)
	{
		v = mpdm_sregex(MPDM_LS(L"/-$/"), w, MPDM_LS(L"~"), 0);
		do_test("Multibyte environment sregex 1",
			mpdm_cmp(v, MPDM_LS(L"-\x03a9~")) == 0);

		v = mpdm_sregex(MPDM_LS(L"/-/g"), w, MPDM_LS(L"~"), 0);
		do_test("Multibyte environment sregex 2",
			mpdm_cmp(v, MPDM_LS(L"~\x03a9~")) == 0);
	}
	else
		printf("Multibyte sregex test omitted; activate with -m\n");
}


static mpdm_t dumper(mpdm_t args)
/* executable value */
{
	mpdm_dump(args);
	return(NULL);
}


static mpdm_t sum(mpdm_t args)
/* executable value: sum all args */
{
	int n,t = 0;

	if(args != NULL)
	{
		for(n = t = 0;n < args->size;n++)
			t += mpdm_ival(mpdm_aget(args, n));
	}

	return(MPDM_I(t));
}


static mpdm_t calculator(mpdm_t c, mpdm_t args)
/* 2 argument version: calculator. c contains a 'script' to
   do things with the arguments */
{
	int n,t;
	mpdm_t v;
	mpdm_t a;
	mpdm_t o;

	/* to avoid destroying args */
	a = mpdm_clone(args);

	/* unshift first argument */
	v = mpdm_adel(a, 0);
	t = mpdm_ival(v);

	for(n = 0;n < mpdm_size(c);n++)
	{
		/* gets operator */
		o = mpdm_aget(c, n);

		/* gets next value */
		v = mpdm_adel(a, 0);

		switch(*(wchar_t *)o->data)
		{
		case '+': t += mpdm_ival(v); break;
		case '-': t -= mpdm_ival(v); break;
		case '*': t *= mpdm_ival(v); break;
		case '/': t /= mpdm_ival(v); break;
		}
	}

	return(MPDM_I(t));
}


void test_exec(void)
{
	mpdm_t x;
	mpdm_t w;
	mpdm_t p;

	printf("test_exec\n");

	x = MPDM_X(dumper);

	/* a simple value */
	mpdm_exec(x, NULL);
	mpdm_exec(x, x);

	x = MPDM_X(sum);
	w = MPDM_A(3);
	mpdm_aset(w, MPDM_I(100), 0);
	mpdm_aset(w, MPDM_I(220), 1);
	mpdm_aset(w, MPDM_I(333), 2);

	do_test("exec 0", mpdm_ival(mpdm_exec(x, w)) == 653);

	mpdm_push(w, MPDM_I(1));

	/* multiple executable value: vm and compiler support */

	/* calculator 'script' */
	p = MPDM_A(0);
	mpdm_push(p, MPDM_LS(L"+"));
	mpdm_push(p, MPDM_LS(L"-"));
	mpdm_push(p, MPDM_LS(L"+"));

	/* the value */
	x = MPDM_A(2);
	x->flags |= MPDM_EXEC;

	mpdm_aset(x, MPDM_X(calculator), 0);
	mpdm_aset(x, p, 1);

	do_test("exec 1", mpdm_ival(mpdm_exec(x, w)) == -12);

	/* another 'script', different operations with the same values */
	p = MPDM_A(0);
	mpdm_push(p, MPDM_LS(L"*"));
	mpdm_push(p, MPDM_LS(L"/"));
	mpdm_push(p, MPDM_LS(L"+"));

	mpdm_aset(x, p, 1);

	do_test("exec 2", mpdm_ival(mpdm_exec(x, w)) == 67);
}


void test_nondyn(void)
{
	mpdm_t v;
	mpdm_t w;
	mpdm_t a;
	mpdm_t av[2] = { NULL, NULL };

	a = MPDM_A(1);

	printf("Non-dynamic values\n");

	v = MPDM_ND_LS(L"This is a non-dynamic value");
	mpdm_dump(v);

	do_test("Non-dynamic literal value size", mpdm_size(v) == 27);

	/* test auto-cloning when storing nondyn values to arrays */
	mpdm_aset(a, v, 0);
	w = mpdm_aget(a, 0);

	do_test("ND_LS values should have been cloned on mpdm_aset",
		w != v);

	v = MPDM_ND_A(av);
	mpdm_dump(v);

	/* test auto-cloning when storing nondyn values to arrays */
	mpdm_aset(a, v, 0);
	w = mpdm_aget(a, 0);

	do_test("ND_A values should have been cloned on mpdm_aset",
		w != v);
}


void test_encoding(void)
{
	mpdm_t f;
	mpdm_t v;
	mpdm_t w;
	wchar_t * ptr;

	v = MPDM_MBS("¡España!\n");

	printf("\nLocale encoding tests (will look bad if terminal is not ISO-8859-1)\n\n");

	if((f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"))) == NULL)
	{
		printf("Can't write test.txt; no further file test possible.\n");
		return;
	}

	mpdm_write(f, v);
	mpdm_close(f);

	f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r"));
	w = mpdm_read(f);
	mpdm_dump(w);
	do_test("Locale encoding", mpdm_cmp(w, v) == 0);
	mpdm_close(f);

	printf("\nutf8.txt loading (should look good only in UTF-8 terminals with good fonts)\n");

	f = mpdm_open(MPDM_LS(L"utf8.txt"), MPDM_LS(L"r"));
	w = mpdm_read(f);
	mpdm_dump(w);
	mpdm_close(f);

	for(ptr = w->data;*ptr != L'\0';ptr++)
		printf("%d", mpdm_wcwidth(*ptr));
	printf("\n");

	if(mpdm_encoding(MPDM_LS(L"UTF-8")) < 0)
	{
		printf("No multiple encoding (iconv) support; no more tests possible.\n");
		return;
	}

	/* new open file will use the specified encoding */
	f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"));
	mpdm_write(f, v);
	mpdm_close(f);

	f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r"));
	w = mpdm_read(f);
	mpdm_dump(w);
	do_test("iconv encoding", mpdm_cmp(w, v) == 0);
	mpdm_close(f);

	mpdm_encoding(NULL);
}


void test_gettext(void)
{
	mpdm_t v;
	mpdm_t h;

	printf("\nTesting gettext...\n");

	mpdm_gettext_domain(MPDM_LS(L"stress"), MPDM_LS(L"./po"));

	printf("Should follow a translated string of 'This is a test string':\n");
	v = mpdm_gettext(MPDM_LS(L"This is a test string"));
	mpdm_dump(v);

	printf("The same, but cached:\n");
	v = mpdm_gettext(MPDM_LS(L"This is a test string"));
	mpdm_dump(v);

	v = mpdm_gettext(MPDM_LS(L"This string is not translated"));
	mpdm_dump(v);

	printf("Ad-hoc translation hash:\n");
	h = MPDM_H(0);
	mpdm_hset(h, MPDM_LS(L"test string"), MPDM_LS(L"cadena de prueba"));

	mpdm_gettext_domain(MPDM_LS(L"stress"), h);
	v = mpdm_gettext(MPDM_LS(L"test string"));
	mpdm_dump(v);
}


void timer(int secs)
{
	static clock_t clks = 0;

	switch(secs)
	{
	case 0:
		clks = clock();
		break;

	case -1:
		printf("%.2f seconds\n",
			(float)(clock() - clks)/(float)CLOCKS_PER_SEC);
		break;
	}
}


void bench_hash(int i, mpdm_t l, int buckets)
{
	mpdm_t h;
	mpdm_t v;
	int n;

	printf("Hash of %d buckets: \n", buckets);
	h = MPDM_H(buckets);

	timer(0);
	for(n = 0;n < i;n++)
	{
		v = mpdm_aget(l, n);
		mpdm_hset(h, v, v);
	}
	timer(-1);
/*
	printf("Bucket usage:\n");
	for(n=0;n < mpdm_size(h);n++)
		printf("\t%d: %d\n", n, mpdm_size(mpdm_aget(h, n)));
*/
	mpdm_sweep(-1);
}


void benchmark(void)
{
	mpdm_t l;
	int i, n;
	char tmp[64];

	printf("\n");

	if(!do_benchmarks)
	{
		printf("Skipping benchmarks\nRun them with 'stress -b'\n");
		return;
	}

	printf("BENCHMARKS\n");

	i = 500000;

	printf("Creating %d values...\n", i);

	l = mpdm_ref(MPDM_A(i));
	for(n = 0;n < i;n++)
	{
		sprintf(tmp,"%08x",n);
/*		mpdm_aset(l, MPDM_MBS(tmp), n);*/
		mpdm_aset(l, MPDM_I(n), n);
	}

	printf("OK\n");

	bench_hash(i, l, 0);
	bench_hash(i, l, 61);
	bench_hash(i, l, 89);
	bench_hash(i, l, 127);

	mpdm_unref(l);
}


void test_conversion(void)
{
	wchar_t * wptr = NULL;
	char * ptr = NULL;
	int size = 0;

	ptr = mpdm_wcstombs(L"", &size);
	do_test("mpdm_wcstombs converts an empty string", ptr != NULL);

	wptr = mpdm_mbstowcs("", &size, 0);
	do_test("mpdm_mbstowcs converts an empty string", wptr != NULL);
}


int main(int argc, char * argv[])
{
	if(argc > 1)
	{
		if(strcmp(argv[1], "-b") == 0)
			do_benchmarks = 1;
		if(strcmp(argv[1], "-m") == 0)
			do_multibyte_sregex_tests = 1;
	}

	mpdm_startup();

	test_basic();
	test_array();
	test_hash();
	test_splice();
	test_strcat();
	test_split();
	test_join();
	test_sym();
	test_file();
	test_regex();
	test_exec();
	test_nondyn();
	test_encoding();
	test_gettext();
	test_conversion();
	benchmark();

	printf("memory: %d\n", mpdm->memory_usage);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	mpdm_sweep(-1);
	printf("memory: %d\n", mpdm->memory_usage);

	mpdm_shutdown();

	printf("\n*** Total tests passed: %d/%d\n", oks, tests);

	if(oks == tests)
		printf("*** ALL TESTS PASSED\n");
	else
	{
		int n;

		printf("*** %d %s\n", tests - oks, "TESTS ---FAILED---");

		printf("\nFailed tests:\n\n");
		for(n = 0;n < i_failed_msgs;n++)
			printf("%s", failed_msgs[n]);
	}

	return(0);
}
