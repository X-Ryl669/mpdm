/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2007 Angel Ortega <angel@triptico.com>

    mpdm_h.c - Hash management

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
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mpdm.h"

/*******************
	Code
********************/

/* prototype for the one-time wrapper hash function */
static int switch_hash_func(const wchar_t *, int);

/* pointer to the hashing function */
static int (*mpdm_hash_func) (const wchar_t *, int) = switch_hash_func;

static int standard_hash_func(const wchar_t * string, int mod)
/* computes a hashing function on string */
{
	int c;

	for (c = 0; *string != L'\0'; string++)
		c ^= (int) *string;

	return c % mod;
}


static int null_hash_func(const wchar_t * string, int mod)
{
	return *string % mod;
}

static int switch_hash_func(const wchar_t * string, int mod)
/* one-time wrapper for hash method autodetection */
{
	/* commute the real hashing function on
	   having the MPDM_NULL_HASH environment variable set */
	if (getenv("MPDM_NULL_HASH") != NULL)
		mpdm_hash_func = null_hash_func;
	else
		mpdm_hash_func = standard_hash_func;

	/* and fall back to it */
	return mpdm_hash_func(string, mod);
}

#define HASH_BUCKET(h, k) (mpdm_hash_func(mpdm_string(k), mpdm_size(h)))

/* interface */

/**
 * mpdm_hsize - Returns the number of pairs of a hash.
 * @h: the hash
 *
 * Returns the number of key-value pairs of a hash.
 * [Hashes]
 */
int mpdm_hsize(const mpdm_t h)
{
	int n;
	int ret = 0;

	for (n = 0; n < mpdm_size(h); n++) {
		mpdm_t b = mpdm_aget(h, n);
		ret += mpdm_size(b);
	}

	return ret / 2;
}


/**
 * mpdm_hget - Gets a value from a hash.
 * @h: the hash
 * @k: the key
 *
 * Gets the value from the hash @h having @k as key, or
 * NULL if the key does not exist.
 * [Hashes]
 */
mpdm_t mpdm_hget(mpdm_t h, mpdm_t k)
{
	mpdm_t b;
	mpdm_t v = NULL;
	int n;

	if (mpdm_size(h)) {
		/* if hash is not empty... */
		if ((b = mpdm_aget(h, HASH_BUCKET(h, k))) != NULL) {
			/* if bucket exists, binary-seek it */
			if ((n = mpdm_bseek(b, k, 2, NULL)) >= 0)
				v = mpdm_aget(b, n + 1);
		}
	}

	return v;
}


/**
 * mpdm_hget_s - Gets the value from a hash (string version).
 * @h: the hash
 * @k: the key
 *
 * Gets the value from the hash @h having @k as key, or
 * NULL if the key does not exist.
 * [Hashes]
 */
mpdm_t mpdm_hget_s(mpdm_t h, wchar_t * k)
{
	return mpdm_hget(h, MPDM_LS(k));
}


/**
 * mpdm_exists - Tests if a key exists.
 * @h: the hash
 * @k: the key
 *
 * Returns 1 if @k is defined in @h, or 0 othersize.
 * [Hashes]
 */
int mpdm_exists(mpdm_t h, mpdm_t k)
{
	mpdm_t b;
	int n;
	int ret = 0;

	if (mpdm_size(h)) {
		/* if hash is not empty... */
		if ((b = mpdm_aget(h, HASH_BUCKET(h, k))) != NULL) {
			/* if bucket exists, binary-seek it */
			if ((n = mpdm_bseek(b, k, 2, NULL)) >= 0)
				ret = 1;
		}
	}

	return ret;
}


/**
 * mpdm_hset - Sets a value in a hash.
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the previous value of the key, or NULL if the key was
 * previously undefined.
 * [Hashes]
 */
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v)
{
	mpdm_t b;
	int n;

	/* if hash is empty, create an optimal number of buckets */
	if (mpdm_size(h) == 0)
		mpdm_expand(h, 0, mpdm->hash_buckets);

	n = HASH_BUCKET(h, k);

	if ((b = mpdm_aget(h, n)) != NULL) {
		int pos;

		/* bucket exists; try to find the key there */
		if ((n = mpdm_bseek(b, k, 2, &pos)) < 0) {
			/* the pair does not exist: create it */
			n = pos;
			mpdm_expand(b, n, 2);
			mpdm_aset(b, k, n);
		}
	}
	else {
		/* the bucket does not exist; create it */
		b = MPDM_A(2);

		/* put the bucket into the hash */
		mpdm_aset(h, b, n);

		/* offset 0 */
		n = 0;

		/* insert the key */
		mpdm_aset(b, k, n);
	}

	return mpdm_aset(b, v, n + 1);
}


/**
 * mpdm_hset_s - Sets a value in a hash (string version).
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the previous value of the key, or NULL if the key was
 * previously undefined.
 * [Hashes]
 */
mpdm_t mpdm_hset_s(mpdm_t h, wchar_t * k, mpdm_t v)
{
	return mpdm_hset(h, MPDM_LS(k), v);
}


/**
 * mpdm_hdel - Deletes a key from a hash.
 * @h: the hash
 * @k: the key
 *
 * Deletes the key @k from the hash @h. Returns the previous
 * value, or NULL if the key was not defined.
 * [Hashes]
 */
mpdm_t mpdm_hdel(mpdm_t h, mpdm_t k)
{
	mpdm_t v = NULL;
	mpdm_t b;
	int n;

	if ((b = mpdm_aget(h, HASH_BUCKET(h, k))) != NULL) {
		/* bucket exists */
		if ((n = mpdm_bseek(b, k, 2, NULL)) >= 0) {
			/* the pair exists: set key and value to NULL */
			mpdm_aset(b, NULL, n);
			v = mpdm_aset(b, NULL, n + 1);

			/* collapse the bucket */
			mpdm_collapse(b, n, 2);
		}
	}

	return v;
}


/**
 * mpdm_keys - Returns the keys of a hash.
 * @h: the hash
 *
 * Returns an array containing all the keys of the @h hash.
 * [Hashes]
 * [Arrays]
 */
mpdm_t mpdm_keys(mpdm_t h)
{
	int n, m, i;
	mpdm_t b;
	mpdm_t a;

	/* create an array with the same number of elements */
	a = MPDM_A(mpdm_hsize(h));

	/* sequentially fill with keys */
	for (n = i = 0; n < mpdm_size(h); n++) {
		if ((b = mpdm_aget(h, n)) != NULL) {
			for (m = 0; m < mpdm_size(b); m += 2)
				mpdm_aset(a, mpdm_aget(b, m), i++);
		}
	}

	return a;
}


static mpdm_t mpdm_sym(mpdm_t r, mpdm_t k, mpdm_t v, int s)
{
	int n;
	mpdm_t p;

	if (r == NULL)
		r = mpdm_root();

	/* splits the path, if needed */
	if (k->flags & MPDM_MULTIPLE)
		p = k;
	else
		p = mpdm_split(MPDM_LS(L"."), k);

	for (n = 0; n < mpdm_size(p) - s; n++) {

		/* is executable? run it and take its output */
		while (MPDM_IS_EXEC(r))
			r = mpdm_exec(r, NULL);

		if (MPDM_IS_HASH(r))
			r = mpdm_hget(r, mpdm_aget(p, n));
		else
		if (MPDM_IS_ARRAY(r)) {
			int i = mpdm_ival(mpdm_aget(p, n));
			r = mpdm_aget(r, i);
		}
		else
			r = NULL;

		if (r == NULL)
			break;
	}

	/* if want to set, do it */
	if (s && r != NULL) {
		if (r->flags & MPDM_HASH)
			r = mpdm_hset(r, mpdm_aget(p, n), v);
		else {
			int i = mpdm_ival(mpdm_aget(p, n));
			r = mpdm_aset(r, v, i);
		}
	}

	return r;
}


mpdm_t mpdm_sget(mpdm_t r, mpdm_t k)
{
	return mpdm_sym(r, k, NULL, 0);
}


mpdm_t mpdm_sset(mpdm_t r, mpdm_t k, mpdm_t v)
{
	return mpdm_sym(r, k, v, 1);
}
