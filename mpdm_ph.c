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

#ifdef CONF_GDBM
#include <gdbm.h>
#endif

#include "mpdm.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

#ifdef CONF_GDBM

/* gdbm ties */

static mpdm_v _mpdm_tie_gdbm_d(mpdm_v v)
{
	if(v->data != NULL)
	{
		gdbm_close((GDBM_FILE) v->data);
		v->data=NULL;
	}

	return(NULL);
}


static mpdm_v _mpdm_tie_gdbm_hget(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v v=NULL;
	datum key, val;
	wchar_t * ptr;

	/* get hash and key */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);

	/* build a key to be fetched */
	ptr=mpdm_string(k);
	key.dptr=(char *)ptr;
	key.dsize=wcslen(ptr) + 1;

	/* try to fetch the value, and if found, create as a
	   free()able value */
	if((val=gdbm_fetch((GDBM_FILE) h->data, key)) != NULL)
		v=mpdm_new(MPDM_STRING,val.dptr,val.dsize,_mpdm_tie_fre());

	return(v);
}


static mpdm_v _mpdm_tie_gdbm_hset(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v v;
	mpdm_v ov;
	datum key, val;
	wchar * ptr;

	/* fetch old value */
	ov=_mpdm_tie_gdbm_hget(a);

	/* get hash, key and value */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);
	v=mpdm_aget(a, 2);

	/* build key and val */
	ptr=mpdm_string(k);
	key.dptr=(char *)ptr;
	key.dsize=wcslen(ptr) + 1;

	ptr=mpdm_string(v);
	val.dptr=(char *)ptr;
	val.dsize=wcslen(ptr) + 1;

	/* do the insertion */
	gdbm_store((GDBM_FILE) h->data, key, val, GDBM_REPLACE);

	/* return old value */
	return(ov);
}


static mpdm_v _mpdm_tie_gdbm_hdel(mpdm_v a)
{
	mpdm_v h;
	mpdm_v k;
	mpdm_v v=NULL;
	mpdm_v ov;
	datum key, val;
	wchar * ptr;

	/* fetch old value */
	ov=_mpdm_tie_gdbm_hget(a);

	/* get hash and key */
	h=mpdm_aget(a, 0);
	k=mpdm_aget(a, 1);

	/* build a key to be fetched */
	ptr=mpdm_string(k);
	key.dptr=(char *)ptr;
	key.dsize=wcslen(ptr);

	gdbm_delete((GDBM_FILE) h->data, key);

	return(ov);
}


static mpdm_v _mpdm_tie_gdbm_hkeys(mpdm_v v)
{
	return(NULL);
}


static mpdm_v _mpdm_tie_gdbm_hsize(mpdm_v v)
{
	return(MPDM_I(0));
}


mpdm_v _mpdm_tie_ph_gdbm(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(0));
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_d), MPDM_TIE_DESTROY);
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_hget), MPDM_TIE_HGET);
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_hset), MPDM_TIE_HSET);
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_hdel), MPDM_TIE_HDEL);
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_hkeys), MPDM_TIE_HKEYS);
		mpdm_aset(_tie, MPDM_X(_mpdm_tie_gdbm_hsize), MPDM_TIE_HSIZE);
	}

	return(_tie);
}

#endif /* CONF_GDBM */


mpdm_v mpdm_ph_gdbm(mpdm_v filename)
{
	mpdm_v v=NULL;

#ifdef CONF_GDBM

	GDBM_FILE dbf;

	/* convert to mbs,s */
	filename=MPDM_2MBS(filename->data);

	if((dbf = gdbm_open(filename->data, 512,
		GDBM_WRCREAT, 0666, NULL) != NULL)
			v=mpdm_new(MPDM_HASH|MPDM_DESTROY, dbf,
			0, _mpdm_tie_ph_gdbm());

#endif /* CONF_GDBM */

	return(v);
}
11

mpdm_v mpdm_ph(mpdm_v filename)
{
	mpdm_v v=NULL;

	if(v == NULL)
		v=mpdm_ph_gdbm(filename);

	return(v);
}
