/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

    mpsl_f.c - Minimum Profit Scripting Language Function Library

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
#include <wchar.h>
#include "mpdm.h"
#include "mpsl.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

mpdm_v _mpsl_size(mpdm_v args)
{
	return(MPDM_I(mpdm_size(mpdm_aget(args, 0))));
}


mpdm_v _mpsl_clone(mpdm_v args)
{
	return(mpdm_clone(mpdm_aget(args, 0)));
}


mpdm_v _mpsl_is_array(mpdm_v args)
{
	mpdm_v v;

	v=mpdm_aget(args, 0);
	return(mpsl_true_or_false(v->flags & MPDM_MULTIPLE));
}


mpdm_v _mpsl_is_hash(mpdm_v args)
{
	mpdm_v v;

	v=mpdm_aget(args, 0);
	return(mpsl_true_or_false(v->flags & MPDM_HASH));
}


mpdm_v _mpsl_dump(mpdm_v args)
{
	mpdm_dump(mpdm_aget(args, 0));
	return(NULL);
}


mpdm_v _mpsl_splice(mpdm_v args)
{
	mpdm_v v;
	mpdm_v i;
	int offset;
	int del;

	v=mpdm_aget(args, 0);
	i=mpdm_aget(args, 1);
	offset=mpdm_ival(mpdm_aget(args, 2));
	del=mpdm_ival(mpdm_aget(args, 3));

	return(mpdm_splice(v, i, offset, del));
}


void _mpsl_lib(void)
/* inits the mpsl library */
{
	mpdm_v r;

	r=mpdm_root();

	/* already done? */
	if(mpdm_hget_s(r, L"MPSL_LIB") != NULL)
		return;

	/* creates all the symbols */

	mpdm_hset_s(r, L"MPSL_LIB", mpsl_true_or_false(1));

	mpdm_hset_s(r, L"size", MPDM_X(_mpsl_size));
	mpdm_hset_s(r, L"clone", MPDM_X(_mpsl_clone));
	mpdm_hset_s(r, L"is_array", MPDM_X(_mpsl_is_array));
	mpdm_hset_s(r, L"is_hash", MPDM_X(_mpsl_is_hash));

	mpdm_hset_s(r, L"splice", MPDM_X(_mpsl_splice));

	mpdm_hset_s(r, L"dump", MPDM_X(_mpsl_dump));
}
