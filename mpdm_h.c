/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

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

static int mpdm_hash_func(wchar_t * string, int mod)
/* computes a hashing function on string */
{
	int c;

	for(c=0;*string != L'\0';string++)
		c=(128 * c + (int)*string) % mod;

	return(c);
}


#define HASH_BUCKET(h, k) (mpdm_hash_func(mpdm_string(k), mpdm_size(h)))


/* interface */

/**
 * mpdm_hsize - Returns the number of pairs of a hash
 * @h: the hash
 *
 * Returns the number of key-value pairs of a hash.
 */
int mpdm_hsize(mpdm_t h)
{
	return(h->ival);
}


/**
 * mpdm_hget - Gets a value from a hash.
 * @h: the hash
 * @k: the key
 *
 * Gets the value from the hash @h having @k as key, or
 * NULL if the key does not exist.
 */
mpdm_t mpdm_hget(mpdm_t h, mpdm_t k)
{
	mpdm_t b;
	mpdm_t v=NULL;
	int n;

	if(mpdm_size(h))
	{
		/* if hash is not empty... */
		if((b=mpdm_aget(h, HASH_BUCKET(h, k))) != NULL)
		{
			/* if bucket exists, binary-seek it */
			if((n=mpdm_abseek(b, k, 2, NULL)) >= 0)
				v=mpdm_aget(b, n + 1);
		}
	}

	return(v);
}


/**
 * mpdm_hget_s - Gets the value from a hash (string version).
 * @h: the hash
 * @k: the key
 *
 * Gets the value from the hash @h having @k as key, or
 * NULL if the key does not exist.
 */
mpdm_t mpdm_hget_s(mpdm_t h, wchar_t * k)
{
	mpdm_t v;

	MPDM_ND_BEGIN();
	v=mpdm_hget(h, MPDM_ND_LS(k));
	MPDM_ND_END();

	return(v);
}


/**
 * mpdm_hexists - Tests if a key exists
 * @h: the hash
 * @k: the key
 *
 * Returns 1 if @k is defined in @h, or 0 othersize.
 */
int mpdm_hexists(mpdm_t h, mpdm_t k)
{
	mpdm_t b;
	int n;
	int ret=0;

	if(mpdm_size(h))
	{
		/* if hash is not empty... */
		if((b=mpdm_aget(h, HASH_BUCKET(h, k))) != NULL)
		{
			/* if bucket exists, binary-seek it */
			if((n=mpdm_abseek(b, k, 2, NULL)) >= 0)
				ret=1;
		}
	}

	return(ret);
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
 */
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v)
{
	mpdm_t b;
	mpdm_t p=NULL;
	int n;
	int pos;

	/* if hash is empty, create an optimal number of buckets */
	if(mpdm_size(h) == 0)
		mpdm_aexpand(h, 0, 31);

	n=HASH_BUCKET(h, k);

	if((b=mpdm_aget(h, n)) != NULL)
	{
		/* bucket exists; try to find the key there */
		if((n=mpdm_abseek(b, k, 2, &pos)) < 0)
		{
			/* the pair does not exist: create it */
			n = pos;
			mpdm_aexpand(b, n, 2);
			mpdm_aset(b, k, n);

			/* increment number of pairs */
			h->ival ++;
		}

		/* sets the value for the key */
		p=mpdm_aset(b, v, n + 1);
	}
	else
	{
		/* the bucket does not exist; create it */
		b=MPDM_A(2);

		/* insert now both key and value */
		mpdm_aset(b, k, 0);
		mpdm_aset(b, v, 1);

		/* put the bucket into the hash */
		mpdm_aset(h, b, n);

		/* increment number of pairs */
		h->ival ++;
	}

	return(p);
}


/**
 * mpdm_hset_s - Sets a value in a hash (string version)
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the previous value of the key, or NULL if the key was
 * previously undefined.
 */
mpdm_t mpdm_hset_s(mpdm_t h, wchar_t * k, mpdm_t v)
{
	MPDM_ND_BEGIN();
	v=mpdm_hset(h, MPDM_ND_LS(k), v);
	MPDM_ND_END();

	return(v);
}


/**
 * mpdm_hdel - Deletes a key from a hash.
 * @h: the hash
 * @k: the key
 *
 * Deletes the key @k from the hash @h. Returns the previous
 * value, or NULL if the key was not defined.
 */
mpdm_t mpdm_hdel(mpdm_t h, mpdm_t k)
{
	mpdm_t v=NULL;
	mpdm_t b;
	int n;

	if((b=mpdm_aget(h, HASH_BUCKET(h, k))) != NULL)
	{
		/* bucket exists */
		if((n=mpdm_abseek(b, k, 2, NULL)) >= 0)
		{
			/* the pair exists: set key and value to NULL */
			mpdm_aset(b, NULL, n);
			v=mpdm_aset(b, NULL, n + 1);

			/* collapse the bucket */
			mpdm_acollapse(b, n, 2);

			/* decrement number of pairs */
			h->ival --;
		}
	}

	return(v);
}


/**
 * mpdm_hkeys - Returns the keys of a hash.
 * @h: the hash
 *
 * Returns an array containing all the keys of the @h hash.
 */
mpdm_t mpdm_hkeys(mpdm_t h)
{
	int n,m,i;
	mpdm_t b;
	mpdm_t a;

	/* create an array with the same number of elements */
	a=MPDM_A(mpdm_hsize(h));

	/* sequentially fill with keys */
	for(n=i=0;n < mpdm_size(h);n++)
	{
		if((b=mpdm_aget(h, n)) != NULL)
		{
			for(m=0;m < mpdm_size(b);m += 2)
				mpdm_aset(a, mpdm_aget(b, m), i++);
		}
	}

	return(a);
}


static mpdm_t mpdm_sym(mpdm_t r, mpdm_t k, mpdm_t v, int s)
{
	int n;
	mpdm_t p;

	if(r == NULL)
		r=mpdm_root();

	/* splits the path, if needed */
	if(k->flags & MPDM_MULTIPLE)
		p=k;
	else
	{
		MPDM_ND_BEGIN();
		p=mpdm_asplit(MPDM_ND_LS(L"."), k);
		MPDM_ND_END();
	}

	for(n=0;n < mpdm_size(p) - s;n++)
	{
		/* try each component as a hash, then as array */
		if(r->flags & MPDM_HASH)
			r=mpdm_hget(r, mpdm_aget(p, n));
		else
		if(r->flags & MPDM_MULTIPLE)
		{
			int i=mpdm_ival(mpdm_aget(p, n));
			r=mpdm_aget(r, i);
		}
		else
			r=NULL;

		if(r == NULL)
			break;
	}

	/* if want to set, do it */
	if(s && r != NULL)
	{
		if(r->flags & MPDM_HASH)
			r=mpdm_hset(r, mpdm_aget(p, n), v);
		else
		{
			int i=mpdm_ival(mpdm_aget(p, n));
			r=mpdm_aset(r, v, i);
		}
	}

	return(r);
}


mpdm_t mpdm_sget(mpdm_t r, mpdm_t k)
{
	return(mpdm_sym(r, k, NULL, 0));
}


mpdm_t mpdm_sset(mpdm_t r, mpdm_t k, mpdm_t v)
{
	return(mpdm_sym(r, k, v, 1));
}
