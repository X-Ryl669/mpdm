/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    stress.c - Stress tests for mpdm.

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

#include "mpdm.h"

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
	mpdm_v v;

	v=MPDM_S("65536");
	i=mpdm_ival(v);

	_test("i == 65536", (i == 65536));
	_test("v has MPDM_IVAL", (v->flags & MPDM_IVAL));

	printf("mpdm_string: %s\n", mpdm_string(MPDM_H(0)));
	printf("mpdm_string: %s\n", mpdm_string(MPDM_H(0)));

	/* partial copies of strings */
	v=MPDM_LS("this is not America");
	v=mpdm_new(MPDM_STRING|MPDM_COPY, (char *)v->data + 4, 4);

	_test("Partial string values", mpdm_cmp(v, MPDM_LS(" is ")) == 0);
}


void test_array(void)
{
	int n;
	mpdm_v a;
	mpdm_v v;

	a=MPDM_A(0);
	_test("a->size == 0", (a->size == 0));

	mpdm_apush(a, MPDM_LS("sunday"));
	mpdm_apush(a, MPDM_LS("monday"));
	mpdm_apush(a, MPDM_LS("tuesday"));
	mpdm_apush(a, MPDM_LS("wednesday"));
	mpdm_apush(a, MPDM_LS("thursday"));
	mpdm_apush(a, MPDM_LS("friday"));
	mpdm_apush(a, MPDM_LS("saturday"));
	mpdm_dump(a);
	_test("a->size == 7", (a->size == 7));

	v=mpdm_aset(a, NULL, 3);
	_test("v->ref == 0", (v->ref == 0));
	mpdm_dump(a);

	mpdm_asort(a, 1);
	_test("NULLs are sorted on top", (mpdm_aget(a, 0) == NULL));

	mpdm_aset(a, v, 0);
	v=mpdm_aget(a, 3);
	_test("v is referenced again", (v->ref > 0));

	mpdm_asort(a, 1);
	_test("mpdm_asort() works (1)",
		mpdm_cmp(mpdm_aget(a,0), MPDM_LS("friday")) == 0);
	_test("mpdm_asort() works (2)",
		mpdm_cmp(mpdm_aget(a,6), MPDM_LS("wednesday")) == 0);

	v=mpdm_aget(a, 3);
	mpdm_acollapse(a, 3, 1);
	_test("acollapse unrefs values", (v->ref == 0));

	/* test queues */
	a=MPDM_A(0);

	/* add several values */
	for(n=0;n < 10;n++)
		v=mpdm_aqueue(a, MPDM_I(n), 10);

	_test("aqueue should still output NULL", (v == NULL));

	v=mpdm_aqueue(a, MPDM_I(11), 10);
	_test("aqueue should no longer output NULL", (v != NULL));

	v=mpdm_aqueue(a, MPDM_I(12), 10);
	_test("aqueue should return 1", mpdm_ival(v) == 1);
	v=mpdm_aqueue(a, MPDM_I(13), 10);
	_test("aqueue should return 2", mpdm_ival(v) == 2);
	_test("queue size should be 10", a->size == 10);

	mpdm_dump(a);
	v=mpdm_aqueue(a, MPDM_I(14), 5);
	mpdm_dump(a);

	_test("queue size should be 5", a->size == 5);
	_test("last taken value should be 8", mpdm_ival(v) == 8);
}


void test_hash(void)
{
	mpdm_v h;
	mpdm_v v;
	int i, n;

	h=MPDM_H(0);

	mpdm_hset(h, MPDM_S("mp"), MPDM_I(6));
	v=mpdm_hget(h, MPDM_S("mp"));

	_test("hash: v != NULL", (v != NULL));
	i=mpdm_ival(v);
	_test("hash: v == 6", (i == 6));

	mpdm_hset(h, MPDM_S("mp2"), MPDM_I(66));
	v=mpdm_hget(h, MPDM_S("mp2"));

	_test("hash: v != NULL", (v != NULL));
	i=mpdm_ival(v);
	_test("hash: v == 66", (i == 66));

	/* fills 100 values */
	for(n=0;n < 50;n++)
		mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));
	for(n=100;n >= 50;n--)
		mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));

	/* tests 100 values */
	for(n=0;n < 100;n++)
	{
		v=mpdm_hget(h, MPDM_I(n));
		_test("hash: hget", (v != NULL));

		if(v != NULL)
		{
			i=mpdm_ival(v);
			_test("hash: ival", (i == n * 10));
		}
	}

	mpdm_dump(h);

	/* use of non-strings as hashes */
	h=MPDM_H(0);

	v=MPDM_A(0);
	mpdm_hset(h, v, MPDM_I(1234));
	v=MPDM_H(0);
	mpdm_hset(h, v, MPDM_I(12345));
	v=MPDM_H(0);
	mpdm_hset(h, v, MPDM_I(9876));
	v=MPDM_A(0);
	mpdm_hset(h, v, MPDM_I(6543));
	i=mpdm_ival(mpdm_hget(h, v));

	mpdm_dump(h);
	_test("hash: using non-strings as hash keys", (i == 6543));
}


void test_splice(void)
{
	mpdm_v w;
	mpdm_v v;

	w=mpdm_splice(MPDM_LS("I'm agent Johnson"), MPDM_LS("special "), 4, 0);
	_test("splice insertion", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("I'm special agent Johnson")) == 0);
	mpdm_dump(w);

	w=mpdm_splice(MPDM_LS("Life is a shit"), MPDM_LS("cheat"), 10, 4);
	_test("splice insertion and deletion (1)", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("Life is a cheat")) == 0);
	_test("splice insertion and deletion (2)", 
		mpdm_cmp(mpdm_aget(w, 1), MPDM_LS("shit")) == 0);
	mpdm_dump(w);

	w=mpdm_splice(MPDM_LS("I'm with dumb"), NULL, 4, 4);
	_test("splice deletion (1)", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("I'm  dumb")) == 0);
	_test("splice deletion (2)", 
		mpdm_cmp(mpdm_aget(w, 1), MPDM_LS("with")) == 0);
	mpdm_dump(w);

	v=MPDM_LS("It doesn't matter");
	w=mpdm_splice(v, MPDM_LS(" two"), v->size, 0);
	_test("splice insertion at the end", 
		mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("It doesn't matter two")) == 0);
	mpdm_dump(w);

	w=mpdm_splice(NULL, NULL, 0, 0);
	_test("splice with two NULLS", (mpdm_aget(w, 0) == NULL));

	w=mpdm_splice(NULL, MPDM_LS("foo"), 0, 0);
	_test("splice with first value NULL",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("foo")) == 0));

	w=mpdm_splice(MPDM_LS("foo"), NULL, 0, 0);
	_test("splice with second value NULL",
		(mpdm_cmp(mpdm_aget(w, 0), MPDM_LS("foo")) == 0));
}


void test_asplit(void)
{
	mpdm_v w;

	printf("mpdm_asplit test\n\n");

	w=mpdm_asplit(MPDM_S("."), MPDM_S("four.elems.in.string"));
	mpdm_dump(w);
	_test("4 elems: ", (w->size == 4));

	w=mpdm_asplit(MPDM_S("."), MPDM_S("unseparated string"));
	mpdm_dump(w);
	_test("1 elem: ", (w->size == 1));

	w=mpdm_asplit(MPDM_S("."), MPDM_S(".dot.at start"));
	mpdm_dump(w);
	_test("3 elems: ", (w->size == 3));

	w=mpdm_asplit(MPDM_S("."), MPDM_S("dot.at end."));
	mpdm_dump(w);
	_test("3 elems: ", (w->size == 3));

	w=mpdm_asplit(MPDM_S("."), MPDM_S("three...dots (two empty elements)"));
	mpdm_dump(w);
	_test("4 elems: ", (w->size == 4));

	w=mpdm_asplit(MPDM_S("."), MPDM_S("."));
	mpdm_dump(w);
	_test("2 elems: ", (w->size == 2));
}


void test_ajoin(void)
{
	mpdm_v v;
	mpdm_v s;
	mpdm_v w;

	printf("mpdm_ajoin test\n\n");

	/* separator */
	s=MPDM_LS("--");

	w=MPDM_A(1);
	mpdm_aset(w, MPDM_S("ce"), 0);

	v=mpdm_ajoin(NULL, w);
	_test("1 elem, no separator", (mpdm_cmp(v, MPDM_LS("ce")) == 0));

	v=mpdm_ajoin(s, w);
	_test("1 elem, '--' separator", (mpdm_cmp(v, MPDM_LS("ce")) == 0));

	mpdm_apush(w, MPDM_LS("n'est"));
	v=mpdm_ajoin(s, w);
	_test("2 elems, '--' separator", (mpdm_cmp(v, MPDM_LS("ce--n'est")) == 0));

	mpdm_apush(w, MPDM_LS("pas"));
	v=mpdm_ajoin(s, w);
	_test("3 elems, '--' separator", (mpdm_cmp(v, MPDM_LS("ce--n'est--pas")) == 0));

	v=mpdm_ajoin(NULL, w);
	_test("3 elems, no separator", (mpdm_cmp(v, MPDM_LS("cen'estpas")) == 0));
}


void test_sym(void)
{
	mpdm_v v;
	int i;

	printf("mpdm_sset / mpdm_sget tests\n\n");

	mpdm_sset(NULL, MPDM_LS("mp"), MPDM_H(7));
	mpdm_sset(NULL, MPDM_LS("mp.config"), MPDM_H(7));
	mpdm_sset(NULL, MPDM_LS("mp.config.auto_indent"), MPDM_I(16384));
	mpdm_sset(NULL, MPDM_LS("mp.config.use_regex"), MPDM_I(1357));
	mpdm_sset(NULL, MPDM_LS("mp.config.gtk_font_face"), MPDM_LS("profontwindows"));
	mpdm_sset(NULL, MPDM_LS("mp.lines"), MPDM_A(2));
	mpdm_sset(NULL, MPDM_LS("mp.lines.0"), MPDM_LS("First post!"));
	mpdm_sset(NULL, MPDM_LS("mp.lines.1"), MPDM_LS("Second post!"));
	mpdm_dump(mpdm_root());

	v=mpdm_sget(NULL, MPDM_LS("mp.config.auto_indent"));
	i=mpdm_ival(v);

	_test("auto_indent == 16384", (i == 16384));

	mpdm_sweep(-1);
}


void test_file(void)
{
	mpdm_v f;
	mpdm_v v;
	mpdm_v w;

	f=mpdm_open(MPDM_LS("test.txt"), MPDM_LS("w"));
	_test("Create test.txt", f != NULL);

	mpdm_write(f, MPDM_LS("0"));
	mpdm_write(f, MPDM_LS("1"));

	/* test an array */
	v=MPDM_A(4);
	mpdm_aset(v, MPDM_LS("2.0"), 0);
	mpdm_aset(v, MPDM_LS("2.1"), 1);
	mpdm_aset(v, MPDM_LS("2.2"), 2);

	w=MPDM_A(2);
	mpdm_aset(w, MPDM_LS("3.0.0"), 0);
	mpdm_aset(w, MPDM_LS("3.0.1"), 1);
	mpdm_aset(v, w, 3);

	mpdm_write(f, v);

	/* write its own file pointer */
	mpdm_write(f, f);

	mpdm_close(f);

	f=mpdm_open(MPDM_LS("test.txt"), MPDM_LS("r"));

	_test("test written file 0", mpdm_cmp(mpdm_read(f), MPDM_LS("0")) == 0);
	_test("test written file 1", mpdm_cmp(mpdm_read(f), MPDM_LS("1")) == 0);
	_test("test written file 2.0", mpdm_cmp(mpdm_read(f), MPDM_LS("2.0")) == 0);
	_test("test written file 2.1", mpdm_cmp(mpdm_read(f), MPDM_LS("2.1")) == 0);
	_test("test written file 2.2", mpdm_cmp(mpdm_read(f), MPDM_LS("2.2")) == 0);
	_test("test written file 3.0.1", mpdm_cmp(mpdm_read(f), MPDM_LS("3.0.0")) == 0);
	_test("test written file 3.0.2", mpdm_cmp(mpdm_read(f), MPDM_LS("3.0.1")) == 0);
	v=mpdm_read(f);
	mpdm_dump(v);

	mpdm_close(f);

	mpdm_unlink(MPDM_LS("test.txt"));
	_test("unlink", mpdm_open(MPDM_LS("test.txt"), MPDM_LS("r")) == NULL);

	v=mpdm_glob(MPDM_LS("*"));
	mpdm_dump(v);
}


void test_regex(void)
{
	mpdm_v v;
	mpdm_v w;

	v=mpdm_regex(MPDM_LS("[0-9]+"), MPDM_LS("123456"), 0, NULL);
	_test("regex 0", v != NULL);

	v=mpdm_regex(MPDM_LS("[0-9]+"), MPDM_I(65536), 0, NULL);
	_test("regex 1", v != NULL);

	v=mpdm_regex(MPDM_LS("^[0-9]+$"), MPDM_LS("12345678"), 0, NULL);
	_test("regex 2", v != NULL);

	v=mpdm_regex(MPDM_LS("^[0-9]+$"), MPDM_I(1), 0, NULL);
	_test("regex 3", v != NULL);

	v=mpdm_regex(MPDM_LS("^[0-9]+$"), MPDM_LS("A12345-678"), 0, NULL);
	_test("regex 4", v == NULL);

	w=MPDM_LS("Hell street, 666");
	v=mpdm_regex(MPDM_LS("[0-9]+"), w, 0, NULL);
	_test("regex 5", v != NULL);

	mpdm_dump(v);

	v=mpdm_regex(MPDM_LS("regex"), MPDM_LS("CASE-INSENSITIVE REGEX"), 0, "i");
	_test("regex 6", v != NULL);

	/* sregex */

	v=mpdm_sregex(MPDM_LS("A"),MPDM_LS("change all A to A"),
		MPDM_LS("E"),0,NULL);
	_test("sregex 0", mpdm_cmp(v, MPDM_LS("change all E to A")) == 0);

	v=mpdm_sregex(MPDM_LS("A"),MPDM_LS("change all A to A"),
		MPDM_LS("E"),0,"g");
	_test("sregex 1", mpdm_cmp(v, MPDM_LS("change all E to E")) == 0);

	v=mpdm_sregex(MPDM_LS("A+"),MPDM_LS("change all AAAAAA to E"),
		MPDM_LS("E"),0,"g");
	_test("sregex 2", mpdm_cmp(v, MPDM_LS("change all E to E")) == 0);

	v=mpdm_sregex(MPDM_LS("A+"),MPDM_LS("change all A A A A A A to E"),
		MPDM_LS("E"),0,"g");
	_test("sregex 3", mpdm_cmp(v, MPDM_LS("change all E E E E E E to E")) == 0);

	v=mpdm_sregex(MPDM_LS("[0-9]+"),MPDM_LS("1, 20, 333, 40 all are numbers"),
		MPDM_LS("numbers"),0,"g");
	_test("sregex 4", mpdm_cmp(v, MPDM_LS("numbers, numbers, numbers, numbers all are numbers")) == 0);

	v=mpdm_sregex(MPDM_LS("[a-zA-Z_]+"),MPDM_LS("regex, mpdm_regex, TexMex"),
		MPDM_LS("sex"),0,"g");
	_test("sregex 5", mpdm_cmp(v, MPDM_LS("sex, sex, sex")) == 0);

	v=mpdm_sregex(MPDM_LS("[a-zA-Z]+"),MPDM_LS("regex, mpdm_regex, TexMex"),
		NULL,0,"g");
	_test("sregex 6", mpdm_cmp(v, MPDM_LS(", _, ")) == 0);

	v=mpdm_sregex(MPDM_LS("\\\\"),MPDM_LS("\\MSDOS\\style\\path"),
		MPDM_LS("/"),0,"g");
	_test("sregex 7", mpdm_cmp(v, MPDM_LS("/MSDOS/style/path")) == 0);

	v=mpdm_sregex(MPDM_LS("regex"),MPDM_LS("regex, Regex, REGEX"),
		MPDM_LS("sex"),0,"gi");
	_test("sregex 8", mpdm_cmp(v, MPDM_LS("sex, sex, sex")) == 0);

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
	test_file();
	test_regex();

	printf("\n*** Total tests passed: %d/%d\n", oks, tests);
	printf("*** %s\n", oks == tests ? "ALL TESTS PASSED" : "SOME TESTS ---FAILED---");

	return(0);
}
