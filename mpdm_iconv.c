/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

    mpdm_iconv.c - Iconv support

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

#ifdef CONFOPT_ICONV

#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>

#include "mpdm.h"

/*******************
	Code
********************/

static mpdm_v _tie_d(mpdm_v v)
{
	if(v->data != NULL)
	{
		/* closes the iconv data */
		iconv_close((iconv_t *)v->data);

		/* frees the struct itself */
		free(v->data);
		v->data=NULL;
	}

	return(NULL);
}


static mpdm_v _tie_iconv(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		/* creates a clone of the cpy tie */
		_tie=mpdm_ref(mpdm_clone(_mpdm_tie_cpy()));

		/* replaces the destructor with its own */
		mpdm_aset(_tie, MPDM_X(_tie_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


static mpdm_v _mpdm_iconv_open(int from, mpdm_v enc)
{
	static mpdm_v _iconv_from=NULL;
	static mpdm_v _iconv_to=NULL;
	mpdm_v h;
	mpdm_v e;
	char * enc_from;
	char * enc_to;
	iconv_t ic;

	/* converts encoding to char * */
	e=MPDM_2MBS(enc->data);

	/* iconv cache */
	if(from)
	{
		if(_iconv_from == NULL)
			_iconv_from=mpdm_ref(MPDM_H(0));

		h=_iconv_from;

		enc_from=(char *)e->data;
		enc_to="WCHAR_T";
	}
	else
	{
		if(_iconv_to == NULL)
			_iconv_to=mpdm_ref(MPDM_H(0));

		h=_iconv_to;

		enc_from="WCHAR_T";
		enc_to=(char *)e->data;
	}

	if((e=mpdm_hget(h, enc)) == NULL)
	{
		/* creates the converter */
		if((ic=iconv_open(enc_to, enc_from)) != NULL)
		{
			/* create value */
			e=mpdm_new(0, &ic, sizeof(iconv_t), _tie_iconv());

			/* stores */
			mpdm_hset(h, enc, e);
		}
	}

	return(e);
}


static mpdm_v _mpdm_iconv(int from, mpdm_v enc, mpdm_v s)
{
	mpdm_v v;
	iconv_t * ic;
	size_t o, r;
	char * op;

	/* creates a new (or gets a cached) iconv converter */
	if((v=_mpdm_iconv_open(from, enc)) == NULL)
		return(NULL);

	ic=(iconv_t *)v->data;

	op=NULL;
	r=o=1 + s->size * (from ? 1 : sizeof(wchar_t));

	/* convert using brute force; instead of trying the cumbersome
	   block-based logic documented everywhere, just try resizing the
	   output buffer until it fits */

	for(;;)
	{
		size_t n;
		size_t i;
		char * np;
		char * ip=(char *)s->data;

		/* make (more) size in the output buffer */
		op=realloc(op, o);

		i=r;
		n=o;
		np=op;

		/* reset converter */
		iconv(*ic, NULL, NULL, NULL, NULL);

		/* do the (possibly partial) conversion */
		if(iconv(*ic, &ip, &i, &np, &n) == -1)
		{
			if(errno != E2BIG)
			{
				/* not a space problem; string is invalid */
				free(op);
				return(NULL);
			}

			/* try again, using more space */
			o *= 2;
		}
		else
		{
			/* resize block to match exactly the processed bytes */
			if(n)
			{
				o -= n;
				op=realloc(op, o);
			}

			break;
		}
	}

	if(from)
	{
		/* converting from enc to wchar_t: create a string value */
		v=mpdm_new(MPDM_STRING,
			(wchar_t *)op, o / sizeof(wchar_t), _mpdm_tie_fre());
	}
	else
	{
		/* converting from wchar_t to enc: create a value */
		v=mpdm_new(0, s, o, _mpdm_tie_fre());
	}

	return(v);
}


mpdm_v mpdm_iconv_from(mpdm_v enc, mpdm_v s)
{
	return(_mpdm_iconv(1, enc, s));
}


mpdm_v mpdm_iconv_to(mpdm_v enc, mpdm_v s)
{
	return(_mpdm_iconv(0, enc, s));
}


#endif /* CONFOPT_ICONV */
