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

static mpdm_v _F_size(mpdm_v a) { return(MPDM_I(mpdm_size(A0))); }
static mpdm_v _F_clone(mpdm_v a) { return(mpdm_clone(A0)); }
static mpdm_v _F_dump(mpdm_v a) { mpdm_dump(A0); return(NULL); }

static mpdm_v _F_is_array(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_MULTIPLE)); }
static mpdm_v _F_is_hash(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_HASH)); }
static mpdm_v _F_is_exec(mpdm_v a) { return(mpsl_boolean(A0->flags & MPDM_EXEC)); }

static mpdm_v _F_splice(mpdm_v a) { return(mpdm_splice(A0,A1,IA2,IA3)); }
static mpdm_v _F_aexpand(mpdm_v a) { return(mpdm_aexpand(A0,IA1,IA2)); }
static mpdm_v _F_acollapse(mpdm_v a) { return(mpdm_acollapse(A0,IA1,IA2)); }
static mpdm_v _F_ains(mpdm_v a) { return(mpdm_ains(A0,A1,IA2)); }
static mpdm_v _F_adel(mpdm_v a) { return(mpdm_adel(A0,IA1)); }
static mpdm_v _F_apush(mpdm_v a) { return(mpdm_apush(A0,A1)); }
static mpdm_v _F_apop(mpdm_v a) { return(mpdm_apop(A0)); }
static mpdm_v _F_aqueue(mpdm_v a) { return(mpdm_aqueue(A0,A1,IA2)); }
static mpdm_v _F_aseek(mpdm_v a) { return(MPDM_I(mpdm_aseek(A0,A1,IA2))); }
static mpdm_v _F_asort(mpdm_v a) { return(mpdm_asort_cb(A0,IA1,A2)); }
static mpdm_v _F_asplit(mpdm_v a) { return(mpdm_asplit(A0,A1)); }
static mpdm_v _F_ajoin(mpdm_v a) { return(mpdm_ajoin(A0,A1)); }

static mpdm_v _F_hsize(mpdm_v a) { return(MPDM_I(mpdm_hsize(A0))); }
static mpdm_v _F_hexists(mpdm_v a) { return(mpsl_boolean(mpdm_hexists(A0, A1))); }
static mpdm_v _F_hdel(mpdm_v a) { return(mpdm_hdel(A0, A1)); }
static mpdm_v _F_hkeys(mpdm_v a) { return(mpdm_hkeys(A0)); }

static mpdm_v _F_open(mpdm_v a) { return(mpdm_open(A0, A1)); }
static mpdm_v _F_close(mpdm_v a) { return(mpdm_close(A0)); }
static mpdm_v _F_read(mpdm_v a) { return(mpdm_read(A0)); }
static mpdm_v _F_write(mpdm_v a) { return(mpsl_boolean(mpdm_write(A0,A1,A2))); }
static mpdm_v _F_unlink(mpdm_v a) { return(mpsl_boolean(mpdm_unlink(A0))); }
static mpdm_v _F_glob(mpdm_v a) { return(mpdm_glob(A0)); }

static mpdm_v _F_regex(mpdm_v a) { return(mpdm_regex(A0,A1,IA2)); }
static mpdm_v _F_sregex(mpdm_v a) { return(mpdm_sregex(A0,A1,A2,IA3)); }

static struct
{
	wchar_t * name;
	mpdm_v (* func)(mpdm_v);
} _mpsl_funcs[]=
{
	{ L"size",	_F_size },
	{ L"clone",	_F_clone },
	{ L"dump",	_F_dump },
	{ L"is_array",	_F_is_array },
	{ L"is_hash",	_F_is_hash },
	{ L"is_exec",	_F_is_exec },
	{ L"splice",	_F_splice },
	{ L"aexpand",	_F_aexpand },
	{ L"acollapse",	_F_acollapse },
	{ L"ains",	_F_ains },
	{ L"adel",	_F_adel },
	{ L"apush",	_F_apush },
	{ L"apop",	_F_apop },
	{ L"aqueue",	_F_aqueue },
	{ L"aseek",	_F_aseek },
	{ L"asort",	_F_asort },
	{ L"asplit",	_F_asplit },
	{ L"ajoin",	_F_ajoin },
	{ L"hsize",	_F_hsize },
	{ L"hexists",	_F_hexists },
	{ L"hdel",	_F_hdel },
	{ L"hkeys",	_F_hkeys },
	{ L"open",	_F_open },
	{ L"close",	_F_close },
	{ L"read",	_F_read },
	{ L"write",	_F_write },
	{ L"unlink",	_F_unlink },
	{ L"glob",	_F_glob },
	{ L"regex",	_F_regex },
	{ L"sregex",	_F_sregex },
	{ NULL,		NULL }
};

void _mpsl_lib(void)
/* inits the mpsl library */
{
	mpdm_v r;
	int n;

	r=mpdm_root();

	/* already done? */
	if(mpdm_hget_s(r, L"MPSL_LIB") != NULL)
		return;

	mpdm_hset_s(r, L"MPSL_LIB", mpsl_boolean(1));

	/* creates all the symbols */
	for(n=0;_mpsl_funcs[n].name != NULL;n++)
		mpdm_hset_s(r, _mpsl_funcs[n].name,
			MPDM_X(_mpsl_funcs[n].func));
}
