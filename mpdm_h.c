/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2018 Angel Ortega <angel@triptico.com>

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

    http://triptico.com

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mpdm.h"

/** code **/

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

#define HASH_BUCKET_S(h, k) mpdm_hash_func(k, mpdm_size(h))
#define HASH_BUCKET(h, k)   mpdm_hash_func(mpdm_string(k), mpdm_size(h))

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
mpdm_t mpdm_hget(const mpdm_t h, const mpdm_t k)
{
    mpdm_t r;

    mpdm_ref(k);
    r = mpdm_hget_s(h, mpdm_string(k));
    mpdm_unref(k);

    return r;
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
mpdm_t mpdm_hget_s(const mpdm_t h, const wchar_t *k)
{
    mpdm_t b;
    mpdm_t v = NULL;
    int n = 0;

    if (mpdm_size(h)) {
        /* if hash is not empty... */
        if ((b = mpdm_aget(h, HASH_BUCKET_S(h, k))) != NULL)
            n = mpdm_bseek_s(b, k, 2, NULL);

        if (n >= 0)
            v = mpdm_aget(b, n + 1);
    }

    return v;
}


/**
 * mpdm_exists - Tests if a key exists.
 * @h: the hash
 * @k: the key
 *
 * Returns 1 if @k is defined in @h, or 0 othersize.
 * [Hashes]
 */
int mpdm_exists(const mpdm_t h, const mpdm_t k)
{
    mpdm_t b;
    int ret = 0;

    mpdm_ref(k);

    if (MPDM_IS_HASH(h)) {
        if (mpdm_size(h)) {
            /* if hash is not empty... */
            if ((b = mpdm_aget(h, HASH_BUCKET(h, k))) != NULL) {
                /* if bucket exists, binary-seek it */
                if (mpdm_bseek(b, k, 2, NULL) >= 0)
                    ret = 1;
            }
        }
    }

    mpdm_unref(k);

    return ret;
}


/**
 * mpdm_hset - Sets a value in a hash.
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the new value (versions prior to 1.0.10 returned the old
 * value).
 * [Hashes]
 */
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v)
{
    mpdm_t b, r;
    int n;

    mpdm_ref(k);
    mpdm_ref(v);

    /* if hash is empty, create an optimal number of buckets */
    if (mpdm_size(h) == 0)
        mpdm_expand(h, 0, mpdm->hash_buckets);

    n = HASH_BUCKET(h, k);

    if ((b = mpdm_aget(h, n)) != NULL) {
        int pos;

        /* bucket exists; try to find the key there */
        n = mpdm_bseek(b, k, 2, &pos);

        if (n < 0) {
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

    r = mpdm_aset(b, v, n + 1);

    mpdm_unref(v);
    mpdm_unref(k);

    return r;
}


/**
 * mpdm_hset_s - Sets a value in a hash (string version).
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the new value (versions prior to 1.0.10 returned the old
 * value).
 * [Hashes]
 */
mpdm_t mpdm_hset_s(mpdm_t h, const wchar_t *k, mpdm_t v)
{
    return mpdm_hset(h, MPDM_S(k), v);
}


/**
 * mpdm_hdel - Deletes a key from a hash.
 * @h: the hash
 * @k: the key
 *
 * Deletes the key @k from the hash @h. Returns NULL
 * (versions prior to 1.0.10 returned the deleted value).
 * [Hashes]
 */
mpdm_t mpdm_hdel(mpdm_t h, const mpdm_t k)
{
    mpdm_t b;
    int n;

    mpdm_ref(k);

    if (mpdm_size(h)) {
        if ((b = mpdm_aget(h, HASH_BUCKET(h, k))) != NULL) {
            /* bucket exists */
            if ((n = mpdm_bseek(b, k, 2, NULL)) >= 0) {
                /* collapse the bucket */
                mpdm_collapse(b, n, 2);
            }
        }
    }

    mpdm_unref(k);

    return NULL;
}


/**
 * mpdm_keys - Returns the keys of a hash.
 * @h: the hash
 *
 * Returns an array containing all the keys of the @h hash.
 * [Hashes]
 * [Arrays]
 */
mpdm_t mpdm_keys(const mpdm_t h)
{
    int n, c;
    mpdm_t a, k;

    mpdm_ref(h);

    /* create an array with the same number of elements */
    a = MPDM_A(mpdm_hsize(h));

    c = n = 0;
    while (mpdm_iterator(h, &c, &k, NULL))
        mpdm_aset(a, k, n++);

    mpdm_unref(h);

    return a;
}
