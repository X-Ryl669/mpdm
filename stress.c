/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2012 Angel Ortega <angel@triptico.com>

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

    http://triptico.com

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
char *failed_msgs[5000];
int i_failed_msgs = 0;

int do_benchmarks = 0;
int do_multibyte_sregex_tests = 0;
int verbose = 0;
int do_sockets = 0;

/** code **/

void _do_test(char *str, int ok, int src_line)
{
    char tmp[1024];

    sprintf(tmp, "stress.c:%d: (#%d) %s: %s\n",
        src_line, tests, str, ok ? "OK!" : "*** Failed ***"
    );

    if (!ok || verbose)
        printf("%s", tmp);

    tests++;

    if (ok)
        oks++;
    else
        failed_msgs[i_failed_msgs++] = strdup(tmp);
}

#define do_test(str, ok) _do_test(str, ok, __LINE__)

#define C { int CC = mpdm->count
#define T(i) do_test("v counter", CC + i == mpdm->count); }


/** tests **/

void test_counter(void)
{
    mpdm_t v, w;

    C;
    v = MPDM_S(L"hi");
    T(1);

    C;
    w = MPDM_A(0);
    T(1);

    C;
    mpdm_push(w, v);
    T(0);

    C;
    mpdm_adel(w, 0);
    T(0);                       /* should fail in 2.x */

    C;
    mpdm_queue(w, v, 10);
    T(0);

    C;
    v = MPDM_S(L"this is a phrase");
    T(1);
    C;
    w = mpdm_split_s(v, L" ");
    T(5);
}


void test_basic(void)
{
    int i;
    double r;
    mpdm_t v;
    mpdm_t w;
    mpdm_t t;

    v = mpdm_ref(MPDM_S(L"65536"));

    if (verbose)
        mpdm_dump(v);

    i = mpdm_ival(v);
    r = mpdm_rval(v);

    do_test("i == 65536", (i == 65536));
    do_test("r == 65536", (r == 65536));
//    do_test("v has MPDM_IVAL", (v->flags & MPDM_IVAL));

    r = mpdm_rval(v);
    do_test("r == 65536", (r == 65536.0));
//    do_test("v has MPDM_RVAL", (v->flags & MPDM_RVAL));

    mpdm_unref(v);

    if (verbose) {
        printf("mpdm_string: %ls\n", mpdm_string(MPDM_H(0)));
        printf("mpdm_string: %ls\n", mpdm_string(MPDM_H(0)));
    }

    /* partial copies of strings */
    v = MPDM_LS(L"this is not America");
    v = MPDM_NS((wchar_t *) v->data + 4, 4);

    do_test("Partial string values", mpdm_cmp(v, MPDM_LS(L" is ")) == 0);

    v = mpdm_ref(MPDM_S(L"MUAHAHAHA!"));
    w = mpdm_ref(mpdm_clone(v));
    do_test("Testing mpdm_clone semantics 1", w == v);
    mpdm_unref(v);
    mpdm_unref(w);

    v = mpdm_ref(MPDM_A(2));
    mpdm_aset(v, MPDM_S(L"evil"), 0);
    mpdm_aset(v, MPDM_S(L"dead"), 1);
    w = mpdm_ref(mpdm_clone(v));

    do_test("Testing mpdm_clone semantics 2.1", w != v);

    t = mpdm_aget(v, 0);
    do_test("Testing mpdm_clone semantics 2.2", t->ref > 1);
    do_test("Testing mpdm_clone semantics 2.3", mpdm_aget(w, 0) == t);

    mpdm_unref(w);
    mpdm_unref(v);

    /* mbs / wcs tests */
    v = MPDM_MBS("This is (was) a multibyte string");
    if (verbose)
        mpdm_dump(v);

    /* greek omega is 03a9 */
    v = MPDM_MBS("?Espa?a! (non-ASCII string, as ISO-8859-1 char *)");
    if (verbose) {
        mpdm_dump(v);
        printf("(Previous value will be NULL if locale doesn't match stress.c encoding)\n");
    }

    v = MPDM_LS(L"A capital greek omega between brackets [\x03a9]");

    if (verbose) {
        mpdm_dump(v);
        printf("(Previous value will only show on an Unicode terminal)\n");
    }

    v = mpdm_ref(MPDM_R(3.1416));
    if (verbose)
        mpdm_dump(v);
    do_test("rval 1", mpdm_rval(v) == 3.1416);
    do_test("ival 1", mpdm_ival(v) == 3);
    mpdm_unref(v);

    v = MPDM_R(777777.0 / 2.0);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 1", mpdm_cmp(v, MPDM_LS(L"388888.5")) == 0);

    v = MPDM_R(388888.500);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 2", mpdm_cmp(v, MPDM_LS(L"388888.5")) == 0);

    v = MPDM_R(388888.412);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 3", mpdm_cmp(v, MPDM_LS(L"388888.412")) == 0);

    v = MPDM_R(388888.6543);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 4", mpdm_cmp(v, MPDM_LS(L"388888.6543")) == 0);

    v = MPDM_R(388888.0);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 5", mpdm_cmp(v, MPDM_LS(L"388888")) == 0);

    v = MPDM_R(0.050000);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 6", mpdm_cmp(v, MPDM_LS(L"0.05")) == 0);

    v = MPDM_R(0.000);
    if (verbose)
        mpdm_dump(v);
    do_test("mpdm_rnew 7", mpdm_cmp(v, MPDM_LS(L"0")) == 0);

    v = MPDM_LS(L"0177");
    do_test("mpdm_ival() for octal numbers", mpdm_ival(v) == 0x7f);

    v = MPDM_LS(L"0xFF");
    do_test("mpdm_ival() for hexadecimal numbers", mpdm_ival(v) == 255);

    v = MPDM_LS(L"001");
    do_test("mpdm_rval() for octal numbers", mpdm_rval(v) == 1.0);

    v = MPDM_LS(L"0x7f");
    do_test("mpdm_rval() for hexadecimal numbers", mpdm_rval(v) == 127.0);

    do_test("Two NULLs are equal", mpdm_cmp(NULL, NULL) == 0);

    v = MPDM_LS(L"hahaha");
    mpdm_ref(v);
    do_test("mpdm_cmp_s 1", mpdm_cmp_s(v, L"hahaha") == 0);
    do_test("mpdm_cmp_s 2", mpdm_cmp_s(v, L"aahaha") > 0);
    do_test("mpdm_cmp_s 3", mpdm_cmp_s(v, L"zahaha") < 0);
    mpdm_unref(v);
}

mpdm_t sort_cb(mpdm_t args)
{
    int d;

    /* sorts reversely */
    d = mpdm_cmp(mpdm_aget(args, 1), mpdm_aget(args, 0));
    return (MPDM_I(d));
}


void test_array(void)
{
    int n;
    mpdm_t a;
    mpdm_t v;

    a = MPDM_A(0);
    mpdm_ref(a);

    do_test("a->size == 0", (a->size == 0));

    mpdm_push(a, MPDM_LS(L"sunday"));
    mpdm_push(a, MPDM_LS(L"monday"));
    mpdm_push(a, MPDM_LS(L"tuesday"));
    mpdm_push(a, MPDM_LS(L"wednesday"));
    mpdm_push(a, MPDM_LS(L"thursday"));
    mpdm_push(a, MPDM_LS(L"friday"));
    mpdm_push(a, MPDM_LS(L"saturday"));
    if (verbose)
        mpdm_dump(a);
    do_test("a->size == 7", (a->size == 7));

    v = mpdm_aget(a, 3);
    mpdm_ref(v);
    mpdm_aset(a, NULL, 3);
    if (verbose)
        mpdm_dump(a);

    mpdm_sort(a, 1);
    do_test("NULLs are sorted on top", (mpdm_aget(a, 0) == NULL));

    mpdm_aset(a, v, 0);
    mpdm_unref(v);

    v = mpdm_aget(a, 3);
    do_test("v is referenced again", (v != NULL && v->ref > 0));

    mpdm_sort(a, 1);
    do_test("mpdm_asort() works (1)",
            mpdm_cmp(mpdm_aget(a, 0), MPDM_LS(L"friday")) == 0);
    do_test("mpdm_asort() works (2)",
            mpdm_cmp(mpdm_aget(a, 6), MPDM_LS(L"wednesday")) == 0);

    /* asort_cb sorts reversely */
    mpdm_sort_cb(a, 1, MPDM_X(sort_cb));

    do_test("mpdm_asort_cb() works (1)",
            mpdm_cmp(mpdm_aget(a, 6), MPDM_LS(L"friday")) == 0);
    do_test("mpdm_asort_cb() works (2)",
            mpdm_cmp(mpdm_aget(a, 0), MPDM_LS(L"wednesday")) == 0);

    v = mpdm_aget(a, 3);
    mpdm_ref(v);
    n = v->ref;
    mpdm_collapse(a, 3, 1);
    do_test("acollapse unrefs values", (v->ref < n));
    mpdm_unref(v);

    mpdm_unref(a);

    /* test queues */
    a = MPDM_A(0);
    mpdm_ref(a);

    /* add several values */
    for (n = 0; n < 10; n++)
        v = mpdm_queue(a, MPDM_I(n), 10);

    do_test("queue should still output NULL", (v == NULL));

    v = mpdm_queue(a, MPDM_I(11), 10);
    do_test("queue should no longer output NULL", (v != NULL));

    v = mpdm_queue(a, MPDM_I(12), 10);
    do_test("queue should return 1", mpdm_ival(v) == 1);
    v = mpdm_queue(a, MPDM_I(13), 10);
    do_test("queue should return 2", mpdm_ival(v) == 2);
    do_test("queue size should be 10", a->size == 10);

    if (verbose)
        mpdm_dump(a);
    v = mpdm_queue(a, MPDM_I(14), 5);
    if (verbose)
        mpdm_dump(a);

    do_test("queue size should be 5", a->size == 5);
    do_test("last taken value should be 8", mpdm_ival(v) == 8);
    mpdm_unref(a);

    a = MPDM_A(4);
    mpdm_ref(a);
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
    mpdm_ref(v);
    mpdm_push(v, MPDM_I(100));
    mpdm_pop(v);
    mpdm_unref(v);

    mpdm_unref(a);

    /* array comparisons with mpdm_cmp() */
    a = MPDM_A(2);
    mpdm_ref(a);
    mpdm_aset(a, MPDM_I(10), 0);
    mpdm_aset(a, MPDM_I(60), 1);

    v = mpdm_ref(mpdm_clone(a));
    do_test("mpdm_cmp: array clones are equal", mpdm_cmp(a, v) == 0);

    mpdm_adel(v, -1);
    do_test("mpdm_cmp: shorter arrays are lesser", mpdm_cmp(a, v) > 0);

    mpdm_push(v, MPDM_I(80));
    do_test("mpdm_cmp: 2# element is bigger, so array is bigger",
            mpdm_cmp(a, v) < 0);

    mpdm_unref(v);
    mpdm_unref(a);

    v = mpdm_ref(mpdm_join_s(mpdm_reverse(mpdm_split_s(MPDM_LS(L"test"), NULL)), NULL));
    do_test("reverse", mpdm_cmp(v, MPDM_LS(L"tset")) == 0);
}


void test_hash(void)
{
    mpdm_t h;
    mpdm_t v;
    int i, n;

    h = MPDM_H(0);
    mpdm_ref(h);

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
    for (n = 0; n < 50; n++)
        mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));
    for (n = 100; n >= 50; n--)
        mpdm_hset(h, MPDM_I(n), MPDM_I(n * 10));

    do_test("hsize 4", mpdm_hsize(h) == 103);

    /* tests 100 values */
    for (n = 0; n < 100; n++) {
        v = mpdm_hget(h, MPDM_I(n));

        if (v != NULL) {
            i = mpdm_ival(v);
            if (!(i == n * 10))
                do_test("hash: ival", (i == n * 10));
        }
        else
            do_test("hash: hget", (v != NULL));
    }

    if (verbose)
        printf("h's size: %d\n", mpdm_hsize(h));

    mpdm_hdel(h, MPDM_LS(L"mp"));
    do_test("hsize 5", mpdm_hsize(h) == 102);

    mpdm_unref(h);

/*
    mpdm_dump(h);

    v=mpdm_hkeys(h);
    mpdm_dump(v);
*/
    /* use of non-strings as hashes */
    h = MPDM_H(0);
    mpdm_ref(h);

    v = MPDM_A(0);
    mpdm_hset(h, v, MPDM_I(1234));
    v = MPDM_H(0);
    mpdm_hset(h, v, MPDM_I(12345));
    v = MPDM_H(0);
    mpdm_hset(h, v, MPDM_I(9876));
    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_hset(h, v, MPDM_I(6543));
    i = mpdm_ival(mpdm_hget(h, v));
    mpdm_unref(v);

    if (verbose)
        mpdm_dump(h);
    do_test("hash: using non-strings as hash keys", (i == 6543));

    mpdm_hset(h, MPDM_LS(L"ok"), MPDM_I(666));

    do_test("exists 1", mpdm_exists(h, MPDM_LS(L"ok")));
    do_test("exists 2", !mpdm_exists(h, MPDM_LS(L"notok")));

    if (verbose)
        mpdm_dump(h);
    v = mpdm_hget_s(h, L"ok");

    if (verbose)
        printf("v %s\n", v == NULL ? "is NULL" : "is NOT NULL");

    do_test("hget_s 1", mpdm_ival(v) == 666);
    if (verbose) {
        mpdm_dump(v);
        mpdm_dump(h);
    }

    i = 0;
    for (n = 0; n < 1000; n++) {
        if (mpdm_hget_s(h, L"ok") != NULL)
            i++;
    }

    if (verbose)
        printf("i: %d\n", i);

    do_test("hget_s 1.2", i == 1000);

    if (i != 1000)
        mpdm_hget_s(h, L"ok");

    do_test("hget 1.2.1", mpdm_hget(h, MPDM_LS(L"ok")) != NULL);

    mpdm_hset_s(h, L"ok", MPDM_I(777));

    v = mpdm_hget_s(h, L"ok");
    do_test("hget_s + hset_s", mpdm_ival(v) == 777);

    mpdm_unref(h);
}


void test_splice(void)
{
    mpdm_t w;
    mpdm_t v;

    w = mpdm_splice(MPDM_LS(L"I'm agent Johnson"), MPDM_LS(L"special "), 4,
                    0);
    do_test("splice insertion",
            mpdm_cmp(mpdm_aget(w, 0),
                     MPDM_LS(L"I'm special agent Johnson")) == 0);
    if (verbose)
        mpdm_dump(w);

    w = mpdm_splice(MPDM_LS(L"Life is a shit"), MPDM_LS(L"cheat"), 10, 4);
    do_test("splice insertion and deletion (1)",
            mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"Life is a cheat")) == 0);
    do_test("splice insertion and deletion (2)",
            mpdm_cmp(mpdm_aget(w, 1), MPDM_LS(L"shit")) == 0);
    if (verbose)
        mpdm_dump(w);

    w = mpdm_splice(MPDM_LS(L"I'm with dumb"), NULL, 4, 4);
    do_test("splice deletion (1)",
            mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm  dumb")) == 0);
    do_test("splice deletion (2)",
            mpdm_cmp(mpdm_aget(w, 1), MPDM_LS(L"with")) == 0);
    if (verbose)
        mpdm_dump(w);

    v = MPDM_LS(L"It doesn't matter");
    w = mpdm_splice(v, MPDM_LS(L" two"), v->size, 0);
    do_test("splice insertion at the end",
            mpdm_cmp(mpdm_aget(w, 0),
                     MPDM_LS(L"It doesn't matter two")) == 0);
    if (verbose)
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
    mpdm_ref(v);

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
    if (verbose)
        mpdm_dump(mpdm_aget(w, 0));

    w = mpdm_splice(v, NULL, 4, -20);
    do_test("splice with out-of-bounds negative del",
            (mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I'm testing")) == 0));
    mpdm_unref(v);
}


void test_strcat(void)
{
    mpdm_t v;
    mpdm_t w;

    w = MPDM_LS(L"something");
    mpdm_ref(w);

    v = mpdm_strcat(NULL, NULL);
    do_test("mpdm_strcat(NULL, NULL) returns NULL", v == NULL);

    v = mpdm_strcat(NULL, w);
    do_test("mpdm_strcat(NULL, w) returns w", mpdm_cmp(v, w) == 0);

    v = mpdm_strcat(w, NULL);
    do_test("mpdm_strcat(w, NULL) returns w", mpdm_cmp(v, w) == 0);
    mpdm_unref(w);

    w = MPDM_LS(L"");
    mpdm_ref(w);
    v = mpdm_strcat(NULL, w);
    do_test("mpdm_strcat(NULL, \"\") returns \"\"", mpdm_cmp(v, w) == 0);

    v = mpdm_strcat(w, NULL);
    do_test("mpdm_strcat(\"\", NULL) returns \"\"", mpdm_cmp(v, w) == 0);

    v = mpdm_strcat(w, w);
    do_test("mpdm_strcat(\"\", \"\") returns \"\"", mpdm_cmp(v, w) == 0);

    mpdm_unref(w);
}


void test_split(void)
{
    mpdm_t w;

    w = mpdm_split(MPDM_S(L"four.elems.in.string"), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("4 elems: ", (w->size == 4));

    w = mpdm_split(MPDM_S(L"unseparated string"), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("1 elem: ", (w->size == 1));

    w = mpdm_split(MPDM_S(L".dot.at start"), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("3 elems: ", (w->size == 3));

    w = mpdm_split(MPDM_S(L"dot.at end."), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("3 elems: ", (w->size == 3));

    w = mpdm_split(MPDM_S(L"three...dots (two empty elements)"), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("4 elems: ", (w->size == 4));

    w = mpdm_split(MPDM_S(L"."), MPDM_S(L"."));
    if (verbose)
        mpdm_dump(w);
    do_test("2 elems: ", (w->size == 2));

    w = mpdm_split(MPDM_S(L"I am the man"), NULL);
    do_test("NULL split 1: ", mpdm_size(w) == 12);
    do_test("NULL split 2: ",
            mpdm_cmp(mpdm_aget(w, 0), MPDM_LS(L"I")) == 0);
}


void test_join(void)
{
    mpdm_t v;
    mpdm_t s;
    mpdm_t w;

    /* separator */
    s = mpdm_ref(MPDM_LS(L"--"));

    w = MPDM_A(1);
    mpdm_ref(w);
    mpdm_aset(w, MPDM_S(L"ce"), 0);

    v = mpdm_join(w, NULL);
    do_test("1 elem, no separator", (mpdm_cmp(v, MPDM_LS(L"ce")) == 0));

    v = mpdm_join(w, s);
    do_test("1 elem, '--' separator", (mpdm_cmp(v, MPDM_LS(L"ce")) == 0));

    mpdm_push(w, MPDM_LS(L"n'est"));
    v = mpdm_join(w, s);
    do_test("2 elems, '--' separator",
            (mpdm_cmp(v, MPDM_LS(L"ce--n'est")) == 0));

    mpdm_push(w, MPDM_LS(L"pas"));
    v = mpdm_join(w, s);
    do_test("3 elems, '--' separator",
            (mpdm_cmp(v, MPDM_LS(L"ce--n'est--pas")) == 0));

    v = mpdm_join(w, NULL);
    do_test("3 elems, no separator",
            (mpdm_cmp(v, MPDM_LS(L"cen'estpas")) == 0));

    mpdm_unref(w);
    mpdm_unref(s);
}


void test_file(void)
{
    mpdm_t f;
    mpdm_t v;
    mpdm_t eol = MPDM_LS(L"\n");

    mpdm_ref(eol);

    f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"));

    if (f == NULL) {
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

    do_test("test written file 0",
            mpdm_cmp(mpdm_read(f), MPDM_LS(L"0\n")) == 0);
    do_test("test written file 1",
            mpdm_cmp(mpdm_read(f), MPDM_LS(L"1\n")) == 0);
    do_test("test written file 2",
            mpdm_cmp(mpdm_read(f), MPDM_LS(L"2")) == 0);
    do_test("test written file 3", mpdm_read(f) == NULL);

    mpdm_close(f);

    mpdm_unlink(MPDM_LS(L"test.txt"));
    do_test("unlink",
            mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r")) == NULL);

    v = mpdm_stat(MPDM_LS(L"stress.c"));
    if (verbose) {
        printf("Stat from stress.c:\n");
        mpdm_dump(v);
    }

/*  v=mpdm_glob(MPDM_LS(L"*"));*/
    v = mpdm_glob(NULL, NULL);
    if (verbose) {
        printf("Glob:\n");
        mpdm_dump(v);
    }

    mpdm_unref(eol);
}


void test_regex(void)
{
    mpdm_t v;
    mpdm_t w;

    v = mpdm_regex(MPDM_LS(L"123456"), MPDM_LS(L"/[0-9]+/"), 0);
    do_test("regex 0", v != NULL);

    v = mpdm_regex(MPDM_I(65536), MPDM_LS(L"/[0-9]+/"), 0);
    do_test("regex 1", v != NULL);

    v = mpdm_regex(MPDM_LS(L"12345678"), MPDM_LS(L"/^[0-9]+$/"), 0);
    do_test("regex 2", v != NULL);

    v = mpdm_regex(MPDM_I(1), MPDM_LS(L"/^[0-9]+$/"), 0);
    do_test("regex 3", v != NULL);

    v = mpdm_regex(MPDM_LS(L"A12345-678"), MPDM_LS(L"/^[0-9]+$/"), 0);
    do_test("regex 4", v == NULL);

    w = MPDM_LS(L"Hell street, 666");
    mpdm_ref(w);
    v = mpdm_regex(w, MPDM_LS(L"/[0-9]+/"), 0);

    if (verbose)
        mpdm_dump(v);

    do_test("regex 5", mpdm_cmp(v, MPDM_I(666)) == 0);

    v = mpdm_regex(MPDM_LS(L"CASE-INSENSITIVE REGEX"), MPDM_LS(L"/regex/"), 
                   0);
    do_test("regex 6.1 (case sensitive)", v == NULL);

    v = mpdm_regex(MPDM_LS(L"CASE-INSENSITIVE REGEX"),
                    MPDM_LS(L"/regex/i"),
                   0);
    do_test("regex 6.2 (case insensitive)", v != NULL);
/*
    v=mpdm_regex(MPDM_LS(L"/[A-Z]+/"), MPDM_LS(L"case SENSITIVE regex"), 0);
    do_test("regex 6.3 (case sensitive)", mpdm_cmp(v, MPDM_LS(L"SENSITIVE")) == 0);
*/
    v = mpdm_regex(MPDM_LS(L"123456"), MPDM_LS(L"/^\\s*/"), 0);
    do_test("regex 7", v != NULL);

    v = mpdm_regex(MPDM_LS(L"123456"), MPDM_LS(L"/^\\s+/"), 0);
    do_test("regex 8", v == NULL);

    v = mpdm_regex(NULL, MPDM_LS(L"/^\\s+/"), 0);
    do_test("regex 9 (NULL string to match)", v == NULL);

    /* sregex */

    v = mpdm_sregex(MPDM_LS(L"change all A to A"),
                    MPDM_LS(L"/A/"), 
                    MPDM_LS(L"E"), 0);
    do_test("sregex 0", mpdm_cmp(v, MPDM_LS(L"change all E to A")) == 0);

    v = mpdm_sregex(MPDM_LS(L"change all A to A"),
                    MPDM_LS(L"/A/g"), 
                    MPDM_LS(L"E"), 0);
    do_test("sregex 1", mpdm_cmp(v, MPDM_LS(L"change all E to E")) == 0);

    v = mpdm_sregex(MPDM_LS(L"change all AAAAAA to E"),
                    MPDM_LS(L"/A+/g"), 
                    MPDM_LS(L"E"), 0);
    do_test("sregex 2", mpdm_cmp(v, MPDM_LS(L"change all E to E")) == 0);

    v = mpdm_sregex(MPDM_LS(L"change all A A A A A A to E"), 
                    MPDM_LS(L"/A+/g"),
                    MPDM_LS(L"E"),
                    0);
    do_test("sregex 3",
            mpdm_cmp(v, MPDM_LS(L"change all E E E E E E to E")) == 0);

    v = mpdm_sregex(MPDM_LS(L"change all AAA A AA AAAAA A AAA to E"),
                    MPDM_LS(L"/A+/g"),
                    MPDM_LS(L"E"), 0);
    do_test("sregex 3.2",
            mpdm_cmp(v, MPDM_LS(L"change all E E E E E E to E")) == 0);

    v = mpdm_sregex(MPDM_LS(L"1, 20, 333, 40 all are numbers"),
                    MPDM_LS(L"/[0-9]+/g"),
                    MPDM_LS(L"numbers"), 0);
    do_test("sregex 4",
            mpdm_cmp(v,
                     MPDM_LS
                     (L"numbers, numbers, numbers, numbers all are numbers"))
            == 0);

    v = mpdm_sregex(MPDM_LS(L"regex, mpdm_regex, TexMex"), 
                    MPDM_LS(L"/[a-zA-Z_]+/g"),
                    MPDM_LS(L"sex"),
                    0);
    do_test("sregex 5", mpdm_cmp(v, MPDM_LS(L"sex, sex, sex")) == 0);

    v = mpdm_sregex(MPDM_LS(L"regex, mpdm_regex, TexMex"), 
                    MPDM_LS(L"/[a-zA-Z]+/g"),
                    NULL, 0);
    do_test("sregex 6", mpdm_cmp(v, MPDM_LS(L", _, ")) == 0);

    v = mpdm_sregex(MPDM_LS(L"\\MSDOS\\style\\path"),
                    MPDM_LS(L"/\\\\/g"), 
                    MPDM_LS(L"/"), 0);
    do_test("sregex 7", mpdm_cmp(v, MPDM_LS(L"/MSDOS/style/path")) == 0);

    v = mpdm_sregex(MPDM_LS(L"regex, Regex, REGEX"),
                    MPDM_LS(L"/regex/gi"), 
                    MPDM_LS(L"sex"), 0);
    do_test("sregex 8", mpdm_cmp(v, MPDM_LS(L"sex, sex, sex")) == 0);

    v = mpdm_sregex(NULL, NULL, NULL, 0);
    do_test("Previous sregex substitutions must be 3", mpdm_ival(v) == 3);

    /* & in substitution tests */
    v = MPDM_LS(L"this string has many words");
    v = mpdm_sregex(v, MPDM_LS(L"/[a-z]+/g"), MPDM_LS(L"[&]"), 0);
    do_test("& in sregex target",
            mpdm_cmp(v,
                     MPDM_LS(L"[this] [string] [has] [many] [words]")) ==
            0);

    v = MPDM_LS(L"this string has many words");
    v = mpdm_sregex(v, MPDM_LS(L"/[a-z]+/g"), MPDM_LS(L"[\\&]"), 0);
    do_test("escaped & in sregex target",
            mpdm_cmp(v, MPDM_LS(L"[&] [&] [&] [&] [&]")) == 0);

    v = MPDM_LS(L"this string has many words");
    v = mpdm_sregex(v, MPDM_LS(L"/[a-z]+/g"), MPDM_LS(L"\\\\&"), 0);
    do_test("escaped \\ in sregex target",
            mpdm_cmp(v,
                     MPDM_LS(L"\\this \\string \\has \\many \\words")) ==
            0);

    v = MPDM_LS(L"hola ");
    v = mpdm_sregex(v, MPDM_LS(L"/[ \t]$/"), MPDM_LS(L""), 0);
    do_test("sregex output size 1", v->size == 4);

    v = MPDM_LS(L"hola ");
    v = mpdm_sregex(v, MPDM_LS(L"/[ \t]$/"), NULL, 0);
    do_test("sregex output size 2", v->size == 4);

    v = MPDM_LS(L"hola ");
    v = mpdm_sregex(v, MPDM_LS(L"/[ \t]$/"), MPDM_LS(L"!"), 0);
    do_test("sregex output size 3", v->size == 5);

    v = MPDM_LS(L"holo");
    v = mpdm_sregex(v, MPDM_LS(L"/o/g"), MPDM_LS(L"!!"), 0);
    do_test("sregex output size 4", v->size == 6);

    /* multiple regex tests */
    w = MPDM_A(0);
    mpdm_ref(w);

    mpdm_push(w, MPDM_LS(L"/^[ \t]*/"));
    mpdm_push(w, MPDM_LS(L"/[^ \t=]+/"));
    mpdm_push(w, MPDM_LS(L"/[ \t]*=[ \t]*/"));
    mpdm_push(w, MPDM_LS(L"/[^ \t]+/"));
    mpdm_push(w, MPDM_LS(L"/[ \t]*$/"));

    v = mpdm_regex(MPDM_LS(L"key=value"), w, 0);
    mpdm_ref(v);
    do_test("multi-regex 1.1",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"key")) == 0);
    do_test("multi-regex 1.2",
            mpdm_cmp(mpdm_aget(v, 3), MPDM_LS(L"value")) == 0);
    mpdm_unref(v);

    v = mpdm_regex(MPDM_LS(L" key = value"), w, 0);
    mpdm_ref(v);
    do_test("multi-regex 2.1",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"key")) == 0);
    do_test("multi-regex 2.2",
            mpdm_cmp(mpdm_aget(v, 3), MPDM_LS(L"value")) == 0);
    mpdm_unref(v);

    v = mpdm_regex(MPDM_LS(L"\t\tkey\t=\tvalue  "), w, 0);
    mpdm_ref(v);
    do_test("multi-regex 3.1",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"key")) == 0);
    do_test("multi-regex 3.2",
            mpdm_cmp(mpdm_aget(v, 3), MPDM_LS(L"value")) == 0);
    mpdm_unref(v);

    mpdm_unref(w);

/*  v = mpdm_regex(w, MPDM_LS(L"key= "), 0);
    do_test("multi-regex 4", v == NULL);
*/
    w = MPDM_LS(L"/* this is\na C-like comment */");
    mpdm_ref(w);

    v = mpdm_regex(w, MPDM_LS(L"|/\\*.+\\*/|"), 0);
    do_test("Multiline regex 1", mpdm_cmp(v, w) == 0);

    v = mpdm_regex(w, MPDM_LS(L"/is$/"), 0);
    do_test("Multiline regex 2", v == NULL);

    v = mpdm_regex(w, MPDM_LS(L"/is$/m"), 0);
    do_test("Multiline regex 3", mpdm_cmp(v, MPDM_LS(L"is")) == 0);
    mpdm_unref(w);

    if (verbose)
        printf("Pitfalls on multibyte locales (f.e. utf-8)\n");

    w = MPDM_LS(L"-\x03a9-");
    mpdm_ref(w);

    v = mpdm_regex(w, MPDM_LS(L"/-$/"), 0);
    do_test("Multibyte environment regex 1",
            mpdm_cmp(v, MPDM_LS(L"-")) == 0);

    if (do_multibyte_sregex_tests) {
        v = mpdm_sregex(w, MPDM_LS(L"/-$/"), MPDM_LS(L"~"), 0);
        do_test("Multibyte environment sregex 1",
                mpdm_cmp(v, MPDM_LS(L"-\x03a9~")) == 0);

        v = mpdm_sregex(w, MPDM_LS(L"/-/g"), MPDM_LS(L"~"), 0);
        do_test("Multibyte environment sregex 2",
                mpdm_cmp(v, MPDM_LS(L"~\x03a9~")) == 0);
    }
    else
        printf("Multibyte sregex test disabled -- use -m\n");

    mpdm_unref(w);

    /* 'last' flag tests */
    v = MPDM_LS(L"this string has many words");
    v = mpdm_regex(v, MPDM_LS(L"/[a-z]+/l"), 0);
    do_test("Flag l in mpdm_regex", mpdm_cmp(v, MPDM_LS(L"words")) == 0);
}


static mpdm_t dumper(mpdm_t args, mpdm_t ctxt)
/* executable value */
{
    mpdm_dump(args);
    return (NULL);
}


static mpdm_t sum(mpdm_t args, mpdm_t ctxt)
/* executable value: sum all args */
{
    int n, t = 0;

    if (args != NULL) {
        for (n = t = 0; n < args->size; n++)
            t += mpdm_ival(mpdm_aget(args, n));
    }

    return (MPDM_I(t));
}


static mpdm_t calculator(mpdm_t c, mpdm_t args, mpdm_t ctxt)
/* 3 argument version: calculator. c contains a 'script' to
   do things with the arguments */
{
    int n, t;
    mpdm_t v;
    mpdm_t a;
    mpdm_t o;

    mpdm_ref(args);

    /* to avoid destroying args */
    a = mpdm_ref(mpdm_clone(args));

    /* shift first argument */
    v = mpdm_shift(a);
    t = mpdm_ival(v);

    for (n = 0; n < mpdm_size(c); n++) {
        /* gets operator */
        o = mpdm_aget(c, n);

        /* gets next value */
        v = mpdm_shift(a);

        switch (*(wchar_t *) o->data) {
        case '+':
            t += mpdm_ival(v);
            break;
        case '-':
            t -= mpdm_ival(v);
            break;
        case '*':
            t *= mpdm_ival(v);
            break;
        case '/':
            t /= mpdm_ival(v);
            break;
        }
    }

    mpdm_unref(a);
    mpdm_unref(args);

    return (MPDM_I(t));
}


void test_exec(void)
{
    mpdm_t x;
    mpdm_t w;
    mpdm_t p;

    x = MPDM_X(dumper);

    /* a simple value */
    if (verbose) {
        mpdm_ref(x);
        mpdm_exec(x, NULL, NULL);
        mpdm_exec(x, x, NULL);
        mpdm_unref(x);
    }

    x = mpdm_ref(MPDM_X(sum));
    w = mpdm_ref(MPDM_A(3));
    mpdm_aset(w, MPDM_I(100), 0);
    mpdm_aset(w, MPDM_I(220), 1);
    mpdm_aset(w, MPDM_I(333), 2);

    do_test("exec 0", mpdm_ival(mpdm_exec(x, w, NULL)) == 653);
    x = mpdm_unref(x);

    mpdm_push(w, MPDM_I(1));

    /* multiple executable value: vm and compiler support */

    /* calculator 'script' */
    p = mpdm_ref(MPDM_A(0));
    mpdm_push(p, MPDM_LS(L"+"));
    mpdm_push(p, MPDM_LS(L"-"));
    mpdm_push(p, MPDM_LS(L"+"));

    /* the value */
    x = mpdm_ref(MPDM_A(2));
    x->flags |= MPDM_EXEC;

    mpdm_aset(x, MPDM_X(calculator), 0);
    mpdm_aset(x, p, 1);

    do_test("exec 1", mpdm_ival(mpdm_exec(x, w, NULL)) == -12);

    mpdm_unref(p);

    /* another 'script', different operations with the same values */
    p = mpdm_ref(MPDM_A(0));
    mpdm_push(p, MPDM_LS(L"*"));
    mpdm_push(p, MPDM_LS(L"/"));
    mpdm_push(p, MPDM_LS(L"+"));

    mpdm_aset(x, p, 1);

    do_test("exec 2", mpdm_ival(mpdm_exec(x, w, NULL)) == 67);
    x = mpdm_unref(x);
    p = mpdm_unref(p);
    w = mpdm_unref(w);
}


void test_encoding(void)
{
    mpdm_t f;
    mpdm_t v;
    mpdm_t w;
    const wchar_t *ptr;

    v = MPDM_MBS("?Espa?a!\n");
    mpdm_ref(v);

    if (verbose)
        printf("\nLocale encoding tests (will look bad if terminal is not ISO-8859-1)\n\n");

    if ((f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"))) == NULL) {
        printf("Can't write test.txt; no further file test possible.\n");
        return;
    }

    mpdm_write(f, v);
    mpdm_close(f);

    f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r"));
    w = mpdm_read(f);
    if (verbose)
        mpdm_dump(w);
    do_test("Locale encoding", mpdm_cmp(w, v) == 0);
    mpdm_close(f);

    if (verbose)
        printf("\nutf8.txt loading (should look good only in UTF-8 terminals with good fonts)\n");

    f = mpdm_open(MPDM_LS(L"utf8.txt"), MPDM_LS(L"r"));
    w = mpdm_read(f);
    if (verbose)
        mpdm_dump(w);
    mpdm_close(f);

    if (verbose) {
        for (ptr = w->data; *ptr != L'\0'; ptr++)
            printf("%d", mpdm_wcwidth(*ptr));
        printf("\n");
    }

    if (mpdm_encoding(MPDM_LS(L"UTF-8")) < 0) {
        printf
            ("No multiple encoding (iconv) support; no more tests possible.\n");
        return;
    }

    if (verbose)
        printf("\nForced utf8.txt loading (should look good only in UTF-8 terminals with good fonts)\n");

    f = mpdm_open(MPDM_LS(L"utf8.txt"), MPDM_LS(L"r"));
    w = mpdm_read(f);
    if (verbose)
        mpdm_dump(w);
    mpdm_close(f);

    /* new open file will use the specified encoding */
    f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"w"));
    mpdm_write(f, v);
    mpdm_close(f);

    f = mpdm_open(MPDM_LS(L"test.txt"), MPDM_LS(L"r"));
    w = mpdm_read(f);
    if (verbose)
        mpdm_dump(w);
    do_test("iconv encoding", mpdm_cmp(w, v) == 0);
    mpdm_close(f);

    mpdm_encoding(NULL);
    mpdm_unref(v);
}


void test_gettext(void)
{
    mpdm_t v;
    mpdm_t h;

    mpdm_gettext_domain(MPDM_LS(L"stress"), MPDM_LS(L"./po"));

    if (verbose)
        printf("Should follow a translated string of 'This is a test string':\n");

    v = mpdm_gettext(MPDM_LS(L"This is a test string"));
    if (verbose)
        mpdm_dump(v);

    if (verbose)
        printf("The same, but cached:\n");
    v = mpdm_gettext(MPDM_LS(L"This is a test string"));
    if (verbose)
        mpdm_dump(v);

    v = mpdm_gettext(MPDM_LS(L"This string is not translated"));
    if (verbose)
        mpdm_dump(v);

    h = MPDM_H(0);
    mpdm_ref(h);
    mpdm_hset(h, MPDM_LS(L"test string"), MPDM_LS(L"cadena de prueba"));

    mpdm_gettext_domain(MPDM_LS(L"stress"), h);
    v = mpdm_gettext(MPDM_LS(L"test string"));
    if (verbose)
        mpdm_dump(v);
    mpdm_unref(h);
}


void timer(int secs)
{
    static clock_t clks = 0;

    switch (secs) {
    case 0:
        clks = clock();
        break;

    case -1:
        printf("%.2f seconds\n",
               (float) (clock() - clks) / (float) CLOCKS_PER_SEC);
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
    mpdm_ref(h);

    timer(0);
    for (n = 0; n < i; n++) {
        v = mpdm_aget(l, n);
        mpdm_hset(h, v, v);
    }
    timer(-1);

    mpdm_unref(h);

/*
    printf("Bucket usage:\n");
    for(n=0;n < mpdm_size(h);n++)
        printf("\t%d: %d\n", n, mpdm_size(mpdm_aget(h, n)));
*/
}


void benchmark(void)
{
    mpdm_t l;
    int i, n;
    char tmp[64];

    if (!do_benchmarks) {
        printf("Benchmarks disabled -- use -b\n");
        return;
    }

    printf("BENCHMARKS\n");

    i = 500000;

    printf("Creating %d values...\n", i);

    l = mpdm_ref(MPDM_A(i));
    for (n = 0; n < i; n++) {
        sprintf(tmp, "%08x", n);
/*      mpdm_aset(l, MPDM_MBS(tmp), n);*/
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
    wchar_t *wptr = NULL;
    char *ptr = NULL;
    int size = 0;

    ptr = mpdm_wcstombs(L"", &size);
    do_test("mpdm_wcstombs converts an empty string", ptr != NULL);

    wptr = mpdm_mbstowcs("", &size, 0);
    do_test("mpdm_mbstowcs converts an empty string", wptr != NULL);
}


void test_pipes(void)
{
    mpdm_t f;

    if ((f = mpdm_popen(MPDM_LS(L"date"), MPDM_LS(L"r"))) != NULL) {
        mpdm_t v;

        v = mpdm_read(f);
        mpdm_pclose(f);

        if (verbose) {
            printf("Pipe from 'date':\n");
            mpdm_dump(v);
        }
    }
    else
        printf("Can't pipe to 'date'\n");
}


void test_misc(void)
{
    if (verbose) {
        printf("Home dir:\n");
        mpdm_dump(mpdm_home_dir());
        printf("App dir:\n");
        mpdm_dump(mpdm_app_dir());
    }
}


void test_sprintf(void)
{
    mpdm_t v;
    mpdm_t w;

    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_push(v, MPDM_I(100));
    mpdm_push(v, MPDM_LS(L"beers"));

    w = mpdm_sprintf(MPDM_LS(L"%d %s for me"), v);
    do_test("sprintf 1", mpdm_cmp(w, MPDM_LS(L"100 beers for me")) == 0);

    w = mpdm_sprintf(MPDM_LS(L"%d %s for me %d"), v);
    do_test("sprintf 2", mpdm_cmp(w, MPDM_LS(L"100 beers for me %d")) == 0);

    w = mpdm_sprintf(MPDM_LS(L"%10d %s for me"), v);
    do_test("sprintf 3",
            mpdm_cmp(w, MPDM_LS(L"       100 beers for me")) == 0);

    w = mpdm_sprintf(MPDM_LS(L"%010d %s for me"), v);
    do_test("sprintf 4",
            mpdm_cmp(w, MPDM_LS(L"0000000100 beers for me")) == 0);

    mpdm_unref(v);

    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_push(v, MPDM_R(3.1416));

    w = mpdm_sprintf(MPDM_LS(L"Value for PI is %6.4f"), v);
    do_test("sprintf 2.1",
            mpdm_cmp(w, MPDM_LS(L"Value for PI is 3.1416")) == 0);

/*  w = mpdm_sprintf(MPDM_LS(L"Value for PI is %08.2f"), v);
    do_test("sprintf 2.2", mpdm_cmp(w, MPDM_LS(L"Value for PI is 00003.14")) == 0);
*/
    mpdm_unref(v);

    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_push(v, MPDM_LS(L"stress"));

    w = mpdm_sprintf(MPDM_LS(L"This is a |%10s| test"), v);
    do_test("sprintf 3.1",
            mpdm_cmp(w, MPDM_LS(L"This is a |    stress| test")) == 0);

    w = mpdm_sprintf(MPDM_LS(L"This is a |%-10s| test"), v);
    do_test("sprintf 3.2",
            mpdm_cmp(w, MPDM_LS(L"This is a |stress    | test")) == 0);

    mpdm_unref(v);

    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_push(v, MPDM_I(0x263a));

    w = mpdm_sprintf(MPDM_LS(L"%c"), v);
    do_test("sprintf 3.3", mpdm_cmp(w, MPDM_LS(L"\x263a")) == 0);
    mpdm_unref(v);

    v = MPDM_A(0);
    mpdm_ref(v);
    mpdm_push(v, MPDM_I(75));

    w = mpdm_sprintf(MPDM_LS(L"%d%%"), v);
    do_test("sprintf 4.1", mpdm_cmp(w, MPDM_LS(L"75%%")) == 0);

    w = mpdm_sprintf(MPDM_LS(L"%b"), v);
    do_test("sprintf 5.1", mpdm_cmp(w, MPDM_LS(L"1001011")) == 0);
    mpdm_unref(v);

    do_test("fmt 1", mpdm_cmp(
        mpdm_fmt(MPDM_LS(L"%d beers for %s"), MPDM_I(100)),
        MPDM_LS(L"100 beers for %s")) == 0);
    do_test("fmt 2",
        mpdm_cmp(
            mpdm_fmt(
                mpdm_fmt(MPDM_LS(L"%d beers for %s"), MPDM_I(100)),
                MPDM_LS(L"me")
            ),
            MPDM_LS(L"100 beers for me")
        ) == 0
    );
}


void test_ulc(void)
{
    mpdm_t v = mpdm_ref(MPDM_S(L"string"));
    mpdm_t w = mpdm_ref(mpdm_ulc(v, 1));

    do_test("mpdm_ulc 1", mpdm_cmp(mpdm_ulc(v, 1), w) == 0);
    do_test("mpdm_ulc 2", mpdm_cmp(mpdm_ulc(w, 0), v) == 0);

    mpdm_unref(w);

    do_test("mpdm_tr",
        mpdm_cmp(mpdm_tr(v, MPDM_LS(L"si"), MPDM_LS(L"Z1")),
            MPDM_LS(L"Ztr1ng")) == 0
    );

    mpdm_unref(v);
}


void test_scanf(void)
{
    mpdm_t v;

    v = mpdm_sscanf(MPDM_LS(L"1234 5678"), MPDM_LS(L"%d %d"), 0);
    do_test("mpdm_sscanf_1.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"1234")) == 0);
    do_test("mpdm_sscanf_1.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"5678")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"this 12.34 5678"), MPDM_LS(L"%s %f %d"), 0);
    do_test("mpdm_sscanf_2.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"this")) == 0);
    do_test("mpdm_sscanf_2.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"12.34")) == 0);
    do_test("mpdm_sscanf_2.3",
            mpdm_cmp(mpdm_aget(v, 2), MPDM_LS(L"5678")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"this 12.34 5678"), MPDM_LS(L"%s %*f %d"), 0);
    do_test("mpdm_sscanf_3.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"this")) == 0);
    do_test("mpdm_sscanf_3.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"5678")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"12341234121234567890"), 
                    MPDM_LS(L"%4d%4d%2d%10d"),
                    0);
    do_test("mpdm_sscanf_4.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"1234")) == 0);
    do_test("mpdm_sscanf_4.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"1234")) == 0);
    do_test("mpdm_sscanf_4.3",
            mpdm_cmp(mpdm_aget(v, 2), MPDM_LS(L"12")) == 0);
    do_test("mpdm_sscanf_4.4",
            mpdm_cmp(mpdm_aget(v, 3), MPDM_LS(L"1234567890")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"ccbaabcxaaae and more"), 
                    MPDM_LS(L"%[abc]%s"),
                    0);
    do_test("mpdm_sscanf_5.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"ccbaabc")) == 0);
    do_test("mpdm_sscanf_5.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"xaaae")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"ccbaabcxaaae and more"),
                    MPDM_LS(L"%[a-d]%s"),
                    0);
    do_test("mpdm_sscanf_6.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"ccbaabc")) == 0);
    do_test("mpdm_sscanf_6.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"xaaae")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"ccbaabcxaaae and more"),
                    MPDM_LS(L"%[^x]%s"),
                    0);
    do_test("mpdm_sscanf_7.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"ccbaabc")) == 0);
    do_test("mpdm_sscanf_7.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"xaaae")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"key: value"),
                    MPDM_LS(L"%[^:]: %s"),
                    0);
    do_test("mpdm_sscanf_8.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"key")) == 0);
    do_test("mpdm_sscanf_8.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"value")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"this is code /* comment */ more code"), 
                    MPDM_LS(L"%*[^/]/* %s */"),
                    0);
    do_test("mpdm_sscanf_9.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"comment")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"1234%5678"), MPDM_LS(L"%d%%%d"), 0);
    do_test("mpdm_sscanf_10.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"1234")) == 0);
    do_test("mpdm_sscanf_10.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"5678")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"ccbaabcxaaae and more"), 
                    MPDM_LS(L"%*[abc]%n%*[^ ]%n"),
                    0);
    do_test("mpdm_sscanf_11.1", mpdm_ival(mpdm_aget(v, 0)) == 7);
    do_test("mpdm_sscanf_11.2", mpdm_ival(mpdm_aget(v, 1)) == 12);

    v = mpdm_sscanf(MPDM_LS(L"/* inside the comment */"),
                    MPDM_LS(L"/* %S */"),
                    0);
    do_test("mpdm_sscanf_12.1",
            mpdm_cmp(mpdm_aget(v, 0),
                     MPDM_LS(L"inside the comment")) == 0);

    v = mpdm_sscanf(MPDM_LS(L"/* inside the comment */outside"),
                    MPDM_LS(L"/* %S */%s"),
                    0);
    do_test("mpdm_sscanf_13.1",
            mpdm_cmp(mpdm_aget(v, 0),
                     MPDM_LS(L"inside the comment")) == 0);
    do_test("mpdm_sscanf_13.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"outside")) == 0);

    v = mpdm_sscanf(MPDM_LS(L""), MPDM_LS(L"%n"), 0);
    do_test("mpdm_sscanf_14.1", mpdm_size(v) == 1
            && mpdm_ival(mpdm_aget(v, 0)) == 0);

    v = mpdm_sscanf(MPDM_LS(L"this 12.34 5678#12@34"),
                    MPDM_LS(L"%[^%f]%f %[#%d@]"),
                    0);
    do_test("mpdm_sscanf_15.1",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"this ")) == 0);
    do_test("mpdm_sscanf_15.2",
            mpdm_cmp(mpdm_aget(v, 1), MPDM_LS(L"12.34")) == 0);
    do_test("mpdm_sscanf_15.3",
            mpdm_cmp(mpdm_aget(v, 2), MPDM_LS(L"5678#12@34")) == 0);

    v = mpdm_sscanf( MPDM_LS(L"a \"bbb\" c;"),
                    MPDM_LS(L"%*S\"%[^\n\"]\""),
                    0);
    do_test("mpdm_sscanf_16",
            mpdm_cmp(mpdm_aget(v, 0), MPDM_LS(L"bbb")) == 0);

    do_test("mpdm_sscanf_17", mpdm_aget(v, 0)->size == 3);
}


mpdm_t mutex = NULL;
int t_finished = -1;

mpdm_t the_thread(mpdm_t args, mpdm_t ctxt)
/* running from a thread */
{
    mpdm_t fn = mpdm_ref(MPDM_LS(L"thread.txt"));
    mpdm_t f;

    if (verbose)
        printf("thread: start writing from thread\n");

    if ((f = mpdm_open(fn, MPDM_LS(L"w"))) != NULL) {
        int n;

        for (n = 0; n < 1000; n++) {
            mpdm_write(f, MPDM_I(n));
            mpdm_write(f, MPDM_LS(L"\n"));
        }

        mpdm_close(f);
    }

    if (verbose)
        printf("thread: finished writing from thread\n");

    mpdm_mutex_lock(mutex);
    t_finished = 1;
    mpdm_mutex_unlock(mutex);

    if (verbose)
        printf("thread: t_finished set\n");

    return NULL;
}


void test_thread(void)
{
    mpdm_t fn = mpdm_ref(MPDM_LS(L"thread.txt"));
    mpdm_t x, v;
    int done;

    mpdm_unlink(fn);

    /* create the executable value */
    x = mpdm_ref(MPDM_X(the_thread));

    /* create a mutex */
    mutex = mpdm_ref(mpdm_new_mutex());

    t_finished = 0;

    v = mpdm_exec_thread(x, NULL, NULL);

    if (verbose)
        printf("parent: waiting for the thread to finish...\n");
    mpdm_ref(v);

    done = 0;
    while (!done) {
        mpdm_sleep(10);

        mpdm_mutex_lock(mutex);

        if (t_finished > 0)
            done = 1;

        mpdm_mutex_unlock(mutex);
    }

    if (verbose)
        printf("parent: thread said it has finished.\n");

    mpdm_unref(v);

    mpdm_unref(mutex);
    mpdm_unref(x);
    mpdm_unref(fn);
}


mpdm_t sem = NULL;

mpdm_t sem_thread(mpdm_t args, mpdm_t ctxt)
{
    if (verbose)
        printf("thread: waiting for semaphore...\n");

    mpdm_semaphore_wait(sem);

    if (verbose)
        printf("thread: got semaphore.\n");

    return NULL;
}


void test_sem(void)
{
    mpdm_t x, v;

    /* create the executable value */
    x = mpdm_ref(MPDM_X(sem_thread));

    /* creates the semaphore */
    sem = mpdm_ref(mpdm_new_semaphore(0));

    if (verbose)
        printf("parent: launching thread.\n");

    v = mpdm_exec_thread(x, NULL, NULL);
    mpdm_ref(v);

    mpdm_sleep(10);

    if (verbose)
        printf("parent: posting semaphore.\n");

    mpdm_semaphore_post(sem);

    mpdm_sleep(10);

    mpdm_unref(v);
    mpdm_unref(sem);
    mpdm_unref(x);
}


void test_sock(void)
{
    mpdm_t f;

    if (do_sockets) {
        printf("socket: connecting to google\n");

        f = mpdm_connect(MPDM_LS(L"www.google.com"), MPDM_LS(L"www"));

        if (f == NULL) {
            printf("Connection failed (Internet access?)\n");
        }
        else {
            int n;
            mpdm_t r;

            printf("Connection successful!\n");

            mpdm_write(f, MPDM_LS(L"GET / HTTP/1.1\r\n"));
            mpdm_write(f, MPDM_LS(L"Host: www.google.com\r\n\r\n"));

            r = mpdm_ref(MPDM_A(0));

            for (n = 0; n < 10; n++)
                mpdm_push(r, mpdm_read(f));

            printf("First 10 lines:\n");
            mpdm_dump(r);

            mpdm_unref(r);

            printf("Closing connection\n");
            mpdm_close(f);
        }
    }
    else
        printf("Socket test disabled -- use -s\n");
}


void test_channel(void)
{
    mpdm_t p, c, v1, v2;

    mpdm_new_channel(&p, &c);

    mpdm_ref(p);
    mpdm_ref(c);

    v1 = mpdm_ref(MPDM_LS(L"Testing"));
    mpdm_write(p, v1);
    v2 = mpdm_read(c);

    do_test("Channel (scalar)", mpdm_cmp(v1, v2) == 0);

    mpdm_unref(v1);

    v1 = mpdm_ref(MPDM_H(0));
    mpdm_hset_s(v1, L"ein",     MPDM_I(1));
    mpdm_hset_s(v1, L"zwei",    MPDM_I(2));
    mpdm_write(p, v1);
    v2 = mpdm_read(c);

    do_test("Channel (hash)", mpdm_cmp(v1, v2) == 0);

    mpdm_unref(v1);

    mpdm_unref(c);
    mpdm_unref(p);
}

wchar_t *json_parser(wchar_t *s, int *t, mpdm_t *pv);

void test_json_in(void)
{
    mpdm_t v;
    int t;

    json_parser(L"1234", &t, &v);
    do_test("JSON 1", t == -1);
    json_parser(L" [1,2]", &t, &v);
    do_test("JSON 2", t >= 0);
    do_test("JSON 2.1", mpdm_ival(mpdm_aget(v, 0)) == 1);
    do_test("JSON 2.2", mpdm_ival(mpdm_aget(v, 1)) == 2);
    json_parser(L"[3, [4, 5]]", &t, &v);
    do_test("JSON 3", t >= 0);
    do_test("JSON 3.1", mpdm_ival(mpdm_aget(v, 0)) == 3);
    do_test("JSON 3.2", mpdm_ival(mpdm_aget(mpdm_aget(v, 1), 1)) == 5);
    json_parser(L"{\"k1\": 10, \"k2\":20}", &t, &v);
    mpdm_ref(v);
    do_test("JSON 4", t >= 0);
    do_test("JSON 4.1", mpdm_ival(mpdm_hget_s(v, L"k2")) == 20);
    do_test("JSON 4.2", mpdm_ival(mpdm_hget_s(v, L"k1")) == 10);
    mpdm_unref(v);
    json_parser(L"{\"k1\":[1,2,3,4],\"k2\":{\"skey\":\"svalue\"}}", &t, &v);
    mpdm_ref(v);
    do_test("JSON 5", t >= 0);
    mpdm_unref(v);
    json_parser(L"{\"k1\":true,\"k2\":false,\"k3\":null}", &t, &v);
    mpdm_ref(v);
    do_test("JSON 6", t >= 0);
    mpdm_unref(v);
    json_parser(L"{\"k1\":\"-\\u005f-\"}", &t, &v);
    mpdm_ref(v);
    do_test("JSON 7", t >= 0);
    mpdm_unref(v);
}


void (*func) (void) = NULL;

int main(int argc, char *argv[])
{
    printf("MPDM stress tests\n\n");

    if (argc > 1) {
        if (strcmp(argv[1], "-b") == 0)
            do_benchmarks = 1;
        if (strcmp(argv[1], "-m") == 0)
            do_multibyte_sregex_tests = 1;
        if (strcmp(argv[1], "-v") == 0)
            verbose = 1;
        if (strcmp(argv[1], "-s") == 0)
            do_sockets = 1;
    }

    mpdm_startup();

    mpdm_sscanf(MPDM_LS(L"123 17/08/1968 456"), MPDM_LS(L"%d %t{%d/%m/%Y} %d"), 0);

/*
    printf("sizeof(struct mpdm_val): %ld\n",
           (long) sizeof(struct mpdm_val));
    printf("sizeof(void *): %d\n", sizeof(void *));
    printf("sizeof(void (*func)(void)): %d\n", sizeof(func));
*/

/*    test_counter();*/
    test_basic();
    test_array();
    test_hash();
    test_splice();
    test_strcat();
    test_split();
    test_join();
    test_file();
    test_regex();
    test_exec();
    test_encoding();
    test_gettext();
    test_conversion();
    test_pipes();
    test_misc();
    test_sprintf();
    test_ulc();
    test_scanf();
    test_thread();
    test_sem();
    test_sock();
    test_channel();
    test_json_in();

    benchmark();

    mpdm_shutdown();

    printf("\n*** Total tests passed: %d/%d\n", oks, tests);

    if (oks == tests)
        printf("*** ALL TESTS PASSED\n");
    else {
        int n;

        printf("*** %d %s\n", tests - oks, "TESTS ---FAILED---");

        printf("\nFailed tests:\n\n");
        for (n = 0; n < i_failed_msgs; n++)
            printf("%s", failed_msgs[n]);
    }

    return oks == tests ? 0 : 1;
}
