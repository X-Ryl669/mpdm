/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpdm_ph.c - Persistent (disk-based) hash support

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

#ifdef CONFOPT_GDBM
#include <gdbm.h>
#endif

#include "mpdm.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

#ifdef CONFOPT_GDBM

static void _gdbm_key_to_datum(mpdm_v k, datum * dat)
{
	wchar_t * ptr;

	ptr=mpdm_string(k);
	dat->dptr=(char *)ptr;
	dat->dsize=(wcslen(ptr) + 1) * sizeof(wchar_t);
}


/* gdbm ties */

static mpdm_v _tie_gdbm_d(mpdm_v v)
{
	if(v->data != NULL)
	{
		gdbm_close((GDBM_FILE) v->data);
		v->data=NULL;
	}

	return(NULL);
}


static mpdm_v _tie_gdbm_hget(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v v=NULL;
	datum key, val;

	/* get hash and key */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);

	/* build a key to be fetched */
	_gdbm_key_to_datum(k, &key);

	/* try to fetch the value, and if found, create as a
	   free()able value */
	val=gdbm_fetch((GDBM_FILE) h->data, key);

	if(val.dptr != NULL)
		v=mpdm_new(MPDM_STRING,val.dptr,val.dsize,_mpdm_tie_fre());

	return(v);
}


static mpdm_v _tie_gdbm_hset(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v v;
	mpdm_v ov;
	datum key, val;

	/* fetch old value */
	ov=_tie_gdbm_hget(a);

	/* get hash, key and value */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);
	v=mpdm_aget(a, 2);

	/* build key and val */
	_gdbm_key_to_datum(k, &key);
	_gdbm_key_to_datum(v, &val);

	/* do the insertion */
	gdbm_store((GDBM_FILE) h->data, key, val, GDBM_REPLACE);

	/* return old value */
	return(ov);
}


static mpdm_v _tie_gdbm_hdel(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v ov;
	datum key;

	/* fetch old value */
	ov=_tie_gdbm_hget(a);

	/* get hash and key */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);

	/* build a key to be fetched */
	_gdbm_key_to_datum(k, &key);

	gdbm_delete((GDBM_FILE) h->data, key);

	return(ov);
}


static mpdm_v _tie_gdbm_hkeys(mpdm_v h)
{
	mpdm_v a;
	mpdm_v v;
	datum key;

	a=MPDM_A(0);

	key=gdbm_firstkey((GDBM_FILE) h->data);

	while(key.dptr)
	{
		v=mpdm_new(MPDM_STRING,key.dptr,key.dsize,_mpdm_tie_fre());
		mpdm_apush(a, v);

		key=gdbm_nextkey((GDBM_FILE) h->data, key);
	}

	return(a);
}


static mpdm_v _tie_gdbm_hsize(mpdm_v v)
{
	v=_tie_gdbm_hkeys(v);
	return(MPDM_I(mpdm_size(v)));
}


#endif /* CONFOPT_GDBM */


mpdm_v mpdm_ph_gdbm(mpdm_v filename)
{
	mpdm_v v=NULL;

#ifdef CONFOPT_GDBM

	static mpdm_v _tie=NULL;
	GDBM_FILE dbf;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(0));
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_d), MPDM_TIE_DESTROY);
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_hget), MPDM_TIE_HGET);
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_hset), MPDM_TIE_HSET);
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_hdel), MPDM_TIE_HDEL);
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_hkeys), MPDM_TIE_HKEYS);
		mpdm_aset(_tie, MPDM_X(_tie_gdbm_hsize), MPDM_TIE_HSIZE);
	}

	/* convert to mbs,s */
	filename=MPDM_2MBS(filename->data);

	if((dbf = gdbm_open(filename->data, 512,
		GDBM_WRCREAT, 0666, NULL)) != NULL)
			v=mpdm_new(MPDM_HASH|MPDM_DESTROY, dbf, 0, _tie);

#endif /* CONFOPT_GDBM */

	return(v);
}


/* plain text file persistent hashes */

static mpdm_v _ph_plain_filename=NULL;

static mpdm_v _tie_plain_d(mpdm_v h)
{
	mpdm_v f;

	/* gets the filename from the cache */
	if((f=mpdm_hget(_ph_plain_filename, h)) != NULL)
	{
		/* opens for writing and dumps the hash */
		if((f=mpdm_open(f, MPDM_LS(L"w"))) != NULL)
		{
			int n;
			mpdm_v a;
			mpdm_v k;
			mpdm_v v;

			for(a=mpdm_hkeys(h),n=0;n < mpdm_size(a);n++)
			{
				k=mpdm_aget(a, n);
				v=mpdm_hget(h, k);

				mpdm_write(f, k);
				mpdm_write(f, v);
			}

			mpdm_close(f);
		}
	}

	return(NULL);
}


mpdm_v mpdm_ph_plain(mpdm_v filename)
{
	mpdm_v h=NULL;
	static mpdm_v _tie=NULL;
	mpdm_v f;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(mpdm_clone(_mpdm_tie_mul()));
		mpdm_aset(_tie, MPDM_X(_tie_plain_d), MPDM_TIE_DESTROY);

		/* creates the filename hash */
		_ph_plain_filename=mpdm_ref(MPDM_H(0));
	}

	h=mpdm_new(MPDM_MULTIPLE|MPDM_HASH|MPDM_IVAL|MPDM_DESTROY,NULL,0,_tie);

	if((f=mpdm_open(filename, MPDM_LS(L"r"))) != NULL)
	{
		mpdm_v k;
		mpdm_v v;

		/* reads from disk */
		for(;;)
		{
			k=mpdm_read(f);
			v=mpdm_read(f);

			if(k == NULL || v == NULL)
				break;

			mpdm_hset(h, k, v);
		}

		mpdm_close(f);
	}

	/* store the filename for destroy use */
	mpdm_hset(_ph_plain_filename, h, filename);

	return(h);
}


mpdm_v mpdm_ph(mpdm_v filename)
{
	mpdm_v v=NULL;

	if(v == NULL)
		v=mpdm_ph_gdbm(filename);

	if(v == NULL)
		v=mpdm_ph_plain(filename);

	return(v);
}
