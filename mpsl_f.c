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

#define A(n) mpdm_aget(a, n)
#define A0 A(0)
#define A1 A(1)
#define A2 A(2)
#define IA(n) mpdm_ival(A(n))
#define IA1 IA(1)
#define IA2 IA(2)
#define IA3 IA(3)

mpdm_v _mpsl_size(mpdm_v a) { return(MPDM_I(mpdm_size(A0))); }
mpdm_v _mpsl_clone(mpdm_v a) { return(mpdm_clone(A0)); }

mpdm_v _mpsl_is_array(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_MULTIPLE)); }
mpdm_v _mpsl_is_hash(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_HASH)); }
mpdm_v _mpsl_is_exec(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_EXEC)); }

mpdm_v _mpsl_dump(mpdm_v a) { mpdm_dump(A0); return(NULL); }
mpdm_v _mpsl_splice(mpdm_v a) { return(mpdm_splice(A0,A1,IA2,IA3)); }
mpdm_v _mpsl_aexpand(mpdm_v a) { return(mpdm_aexpand(A0,IA1,IA2)); }
mpdm_v _mpsl_acollapse(mpdm_v a) { return(mpdm_acollapse(A0,IA1,IA2)); }
mpdm_v _mpsl_ains(mpdm_v a) { return(mpdm_ains(A0,A1,IA2)); }
mpdm_v _mpsl_adel(mpdm_v a) { return(mpdm_adel(A0,IA1)); }
mpdm_v _mpsl_apush(mpdm_v a) { return(mpdm_apush(A0,A1)); }
mpdm_v _mpsl_apop(mpdm_v a) { return(mpdm_apop(A0)); }
mpdm_v _mpsl_aqueue(mpdm_v a) { return(mpdm_aqueue(A0,A1,IA2)); }
mpdm_v _mpsl_aseek(mpdm_v a) { return(MPDM_I(mpdm_aseek(A0,A1,IA2))); }
mpdm_v _mpsl_asort(mpdm_v a) { return(mpdm_asort_cb(A0,IA1,A2)); }
mpdm_v _mpsl_asplit(mpdm_v a) { return(mpdm_asplit(A0,A1)); }
mpdm_v _mpsl_ajoin(mpdm_v a) { return(mpdm_ajoin(A0,A1)); }

mpdm_v _mpsl_hsize(mpdm_v a) { return(MPDM_I(mpdm_hsize(A0))); }
mpdm_v _mpsl_hexists(mpdm_v a) { return(mpsl_boolean(mpdm_hexists(A0, A1))); }
mpdm_v _mpsl_hdel(mpdm_v a) { return(mpdm_hdel(A0, A1)); }
mpdm_v _mpsl_hkeys(mpdm_v a) { return(mpdm_hkeys(A0)); }

mpdm_v _mpsl_open(mpdm_v a) { return(mpdm_open(A0, A1)); }
mpdm_v _mpsl_close(mpdm_v a) { return(mpdm_close(A0)); }
mpdm_v _mpsl_read(mpdm_v a) { return(mpdm_read(A0)); }
mpdm_v _mpsl_write(mpdm_v a) { return(mpsl_boolean(mpdm_write(A0,A1,A2))); }
mpdm_v _mpsl_unlink(mpdm_v a) { return(mpsl_boolean(mpdm_unlink(A0))); }
mpdm_v _mpsl_glob(mpdm_v a) { return(mpdm_glob(A0)); }

mpdm_v _mpsl_regex(mpdm_v a) { return(mpdm_regex(A0,A1,IA2)); }
mpdm_v _mpsl_sregex(mpdm_v a) { return(mpdm_sregex(A0,A1,A2,IA3)); }

void _mpsl_lib(void)
/* inits the mpsl library */
{
	mpdm_v r;

	r=mpdm_root();

	/* already done? */
	if(mpdm_hget_s(r, L"MPSL_LIB") != NULL)
		return;

	/* creates all the symbols */

	mpdm_hset_s(r, L"MPSL_LIB", mpsl_boolean(1));

	mpdm_hset_s(r, L"size", MPDM_X(_mpsl_size));
	mpdm_hset_s(r, L"clone", MPDM_X(_mpsl_clone));
	mpdm_hset_s(r, L"is_array", MPDM_X(_mpsl_is_array));
	mpdm_hset_s(r, L"is_hash", MPDM_X(_mpsl_is_hash));
	mpdm_hset_s(r, L"is_exec", MPDM_X(_mpsl_is_exec));

	mpdm_hset_s(r, L"splice", MPDM_X(_mpsl_splice));

	mpdm_hset_s(r, L"aexpand", MPDM_X(_mpsl_aexpand));
	mpdm_hset_s(r, L"acollapse", MPDM_X(_mpsl_acollapse));
	mpdm_hset_s(r, L"ains", MPDM_X(_mpsl_ains));
	mpdm_hset_s(r, L"adel", MPDM_X(_mpsl_adel));

	mpdm_hset_s(r, L"apush", MPDM_X(_mpsl_apush));
	mpdm_hset_s(r, L"apop", MPDM_X(_mpsl_apop));
	mpdm_hset_s(r, L"aqueue", MPDM_X(_mpsl_aqueue));
	mpdm_hset_s(r, L"aseek", MPDM_X(_mpsl_aseek));
	mpdm_hset_s(r, L"asort", MPDM_X(_mpsl_asort));
	mpdm_hset_s(r, L"asplit", MPDM_X(_mpsl_asplit));
	mpdm_hset_s(r, L"ajoin", MPDM_X(_mpsl_ajoin));

	mpdm_hset_s(r, L"hsize", MPDM_X(_mpsl_hsize));
	mpdm_hset_s(r, L"hexists", MPDM_X(_mpsl_hexists));
	mpdm_hset_s(r, L"hdel", MPDM_X(_mpsl_hdel));
	mpdm_hset_s(r, L"hkeys", MPDM_X(_mpsl_hkeys));

	mpdm_hset_s(r, L"open", MPDM_X(_mpsl_open));
	mpdm_hset_s(r, L"close", MPDM_X(_mpsl_close));
	mpdm_hset_s(r, L"read", MPDM_X(_mpsl_read));
	mpdm_hset_s(r, L"write", MPDM_X(_mpsl_write));
	mpdm_hset_s(r, L"unlink", MPDM_X(_mpsl_unlink));
	mpdm_hset_s(r, L"glob", MPDM_X(_mpsl_glob));

	mpdm_hset_s(r, L"regex", MPDM_X(_mpsl_regex));
	mpdm_hset_s(r, L"sregex", MPDM_X(_mpsl_sregex));

	mpdm_hset_s(r, L"dump", MPDM_X(_mpsl_dump));
}
