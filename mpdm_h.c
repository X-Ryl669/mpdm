/*

    fdm - Filp Data Manager
    Copyright (C) 2003 Angel Ortega <angel@triptico.com>

    fdm_h.c - Hash management

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
#include <stdlib.h>
#include <string.h>
#include "fdm.h"

/*******************
	Code
********************/

static int _fdm_hashfunc(unsigned char * string, int mod)
{
	int c;

	for(c=0;*string != '\0';string++)
		c=(128 * c + *string) % mod;

	return(c);
}

#define HASH_BUCKET(h, k) (_fdm_hashfunc((unsigned char *)k->data, h->size))

/**
 * fdm_hget - Gets an value from a hash.
 * @h: the hash
 * @k: the key
 *
 * Gets the value from the hash @h having @k as key, or
 * NULL if the key does not exist.
 */
fdm_v fdm_hget(fdm_v h, fdm_v k)
{
	int n;
	fdm_v b;
	fdm_v v = NULL;

	/* if hash is empty, nothing can be found */
	if(h->size == 0)
		return(v);

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2, NULL)) >= 0)
			v=fdm_aget(b, n + 1);
	}

	return(v);
}


/**
 * fdm_hset - Sets a value in a hash.
 * @h: the hash
 * @k: the key
 * @v: the value
 *
 * Sets the value @v to the key @k in the hash @h. Returns
 * the previous value of the key, or NULL if the key was
 * previously undefined.
 */
fdm_v fdm_hset(fdm_v h, fdm_v k, fdm_v v)
{
	int n, pos;
	fdm_v b;
	fdm_v p = NULL;

	/* if hash is empty, create an optimal number of buckets */
	if(h->size == 0)
		fdm_aexpand(h, 0, 31);

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2, &pos)) < 0)
		{
			/* the pair does not exist: create it */
			n = pos;
			fdm_aexpand(b, n, 2);
			fdm_aset(b, k, n);
		}

		/* sets the value for the key */
		p=fdm_aset(b, v, n + 1);
	}
	else
	{
		/* the bucket does not exist; create it */
		b=fdm_new(FDM_MULTIPLE, NULL, 2);

		/* insert now both key and value */
		fdm_aset(b, k, 0);
		fdm_aset(b, v, 1);

		/* put the bucket into the hash */
		fdm_aset(h, b, n);
	}

	return(p);
}


/**
 * fdm_hdel - Deletes a key from a hash.
 * @h: the hash
 * @k: the key
 *
 * Deletes the key @k from the hash @h. Returns the previous
 * value, or NULL if the key was not defined.
 */
fdm_v fdm_hdel(fdm_v h, fdm_v k)
{
	int n;
	fdm_v b;
	fdm_v p = NULL;

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2, NULL)) >= 0)
		{
			/* the pair exists: set both to NULL */
			fdm_aset(b, NULL, n);
			p=fdm_aset(b, NULL, n + 1);

			/* collapse the bucket */
			fdm_acollapse(b, n, 2);
		}
	}

	return(p);
}


/**
 * fdm_hkeys - Returns the keys of a hash.
 * @h: the hash
 *
 * Returns an array containing all the keys of the @h hash.
 */
fdm_v fdm_hkeys(fdm_v h)
{
	int n,m;
	fdm_v a;
	fdm_v b;

	a=fdm_new(FDM_MULTIPLE, NULL, 0);

	for(n=0;n < h->size;n++)
	{
		if((b=fdm_aget(h, n)) != NULL)
		{
			for(m=0;m < b->size;m+=2)
				fdm_apush(a, fdm_aget(b, m));
		}
	}

	return(a);
}


static fdm_v _fdm_sym(fdm_v r, fdm_v k, fdm_v v, int s)
{
	int n;
	fdm_v p;

	if(r == NULL)
		r=fdm_root();

	/* splits the path */
	p=fdm_asplit(FDM_LS("."), k);

	for(n=0;n < p->size - s;n++)
	{
		if((r=fdm_hget(r, fdm_aget(p, n))) == NULL)
			break;
	}

	/* if want to set, do it */
	if(s && r != NULL)
		r=fdm_hset(r, fdm_aget(p, n), v);

	return(r);
}


fdm_v fdm_sget(fdm_v r, fdm_v k)
{
	return(_fdm_sym(r, k, NULL, 0));
}


fdm_v fdm_sset(fdm_v r, fdm_v k, fdm_v v)
{
	return(_fdm_sym(r, k, v, 1));
}
