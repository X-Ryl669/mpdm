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

int _fdm_hashfunc(unsigned char * string, int mod)
{
	int c;

	for(c=0;*string != '\0';string++)
		c=(128 * c + *string) % mod;

	return(c);
}

#define HASH_BUCKET(h, k) (_fdm_hashfunc((unsigned char *)k->data, h->size))

fdm_v fdm_hget(fdm_v h, fdm_v k)
{
	int n;
	fdm_v b;
	fdm_v v = NULL;

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2)) > 0)
			v=fdm_aget(b, n + 1);
	}

	return(v);
}


fdm_v fdm_hset(fdm_v h, fdm_v k, fdm_v v)
{
	int n;
	fdm_v b;
	fdm_v p = NULL;

	/* if hash is empty, create an optimal number of buckets */
	if(h->size == 0)
		fdm_aexpand(h, 31, 0);

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2)) < 0)
		{
			/* the pair does not exist: create it */
			n *= -1;
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


fdm_v fdm_hdel(fdm_v h, fdm_v k)
{
	int n;
	fdm_v b;
	fdm_v p = NULL;

	n=HASH_BUCKET(h, k);
	if((b=fdm_aget(h, n)) != NULL)
	{
		if((n=fdm_abseek(b, k, 2)) > 0)
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
