/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

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

/* structural flags */
#define MPDM_STRING	0x00000001	/* data can be string-compared */
#define MPDM_MULTIPLE	0x00000002	/* data is multiple */

#define MPDM_IVAL	0x00000010	/* integer value cached in .ival */

/* 'informative' flags */
#define MPDM_HASH	0x00010000	/* data is a hash */
#define MPDM_FILE	0x00020000	/* data is a FILE * */
#define MPDM_EXEC	0x00040000	/* data is 'executable' */

typedef struct _mpdm_v * mpdm_v;

struct _mpdm_v
{
	int flags;	/* value flags */
	int ref;	/* reference count */
	int size;	/* data size */
	void * data;	/* the real data */
	int ival;	/* cached integer value */
	mpdm_v tie;	/* array of commands */
	mpdm_v next;	/* next in chain */
};

/* tie functions */
#define MPDM_TIE_CREATE		0
#define MPDM_TIE_DESTROY	1
#define MPDM_TIE_CLONE		2
#define MPDM_TIE_HGET		3
#define MPDM_TIE_HSET		4
#define MPDM_TIE_HDEL		5
#define MPDM_TIE_HKEYS		6
#define MPDM_TIE_HSIZE		7

mpdm_v mpdm_new(int flags, void * data, int size, mpdm_v tie);
mpdm_v mpdm_ref(mpdm_v v);
mpdm_v mpdm_unref(mpdm_v v);
void mpdm_sweep(int count);

int mpdm_size(mpdm_v v);
mpdm_v mpdm_clone(mpdm_v v);
mpdm_v mpdm_root(void);

mpdm_v mpdm_exec(mpdm_v c, mpdm_v args);
mpdm_v mpdm_get_tie(mpdm_v v, int tie_func);
mpdm_v mpdm_tie(mpdm_v v, mpdm_v tie);

mpdm_v mpdm_aexpand(mpdm_v a, int offset, int num);
mpdm_v mpdm_acollapse(mpdm_v a, int offset, int num);
mpdm_v mpdm_aset(mpdm_v a, mpdm_v e, int offset);
mpdm_v mpdm_aget(mpdm_v a, int offset);
void mpdm_ains(mpdm_v a, mpdm_v e, int offset);
mpdm_v mpdm_adel(mpdm_v a, int offset);
void mpdm_apush(mpdm_v a, mpdm_v e);
mpdm_v mpdm_apop(mpdm_v a);
mpdm_v mpdm_aqueue(mpdm_v a, mpdm_v e, int size);
int mpdm_aseek(mpdm_v a, mpdm_v k, int step);
int mpdm_abseek(mpdm_v a, mpdm_v k, int step, int * pos);
void mpdm_asort(mpdm_v a, int step);
void mpdm_asort_cb(mpdm_v a, int step, mpdm_v asort_cb);

mpdm_v mpdm_asplit(mpdm_v s, mpdm_v a);
mpdm_v mpdm_ajoin(mpdm_v s, mpdm_v a);

wchar_t * mpdm_string(mpdm_v v);
mpdm_v mpdm_splice(mpdm_v v, mpdm_v i, int offset, int del);
mpdm_v mpdm_strcat(mpdm_v s1, mpdm_v s2);
int mpdm_cmp(mpdm_v v1, mpdm_v v2);
int mpdm_ival(mpdm_v v);

mpdm_v _mpdm_inew(int ival);
mpdm_v _mpdm_new_wcs(wchar_t * s, int n, mpdm_v tie);
mpdm_v _mpdm_new_mbs(char * s, int n, mpdm_v tie);

int mpdm_hsize(mpdm_v h);
mpdm_v mpdm_hget(mpdm_v h, mpdm_v k);
mpdm_v mpdm_hset(mpdm_v h, mpdm_v k, mpdm_v v);
mpdm_v mpdm_hdel(mpdm_v h, mpdm_v k);
mpdm_v mpdm_hkeys(mpdm_v h);

void mpdm_dump(mpdm_v v);

#define MPDM_SGET(r, k) mpdm_sget((r), MPDM_LS((k)))
#define MPDM_SSET(r, k, v) mpdm_sset((r), MPDM_LS((k)), (v))

mpdm_v mpdm_sget(mpdm_v r, mpdm_v k);
mpdm_v mpdm_sset(mpdm_v r, mpdm_v k, mpdm_v v);

mpdm_v mpdm_open(mpdm_v filename, mpdm_v mode);
mpdm_v mpdm_close(mpdm_v fd);
mpdm_v mpdm_read(mpdm_v fd);
int mpdm_write(mpdm_v fd, mpdm_v v);
int mpdm_unlink(mpdm_v filename);
mpdm_v mpdm_glob(mpdm_v spec);

mpdm_v mpdm_regex(mpdm_v r, mpdm_v v, int offset);
mpdm_v mpdm_sregex(mpdm_v r, mpdm_v v, mpdm_v s, int offset);

mpdm_v _mpdm_tie_mul(void);
mpdm_v _mpdm_tie_cpy(void);
mpdm_v _mpdm_tie_str(void);
mpdm_v _mpdm_tie_lstr(void);
mpdm_v _mpdm_tie_fre(void);
mpdm_v _mpdm_tie_mbstowcs(void);
mpdm_v _mpdm_tie_wcstombs(void);

/* value creation utility macros */

#define MPDM_A(n)	mpdm_new(MPDM_MULTIPLE,NULL,n,_mpdm_tie_mul())
#define MPDM_H(n)	mpdm_new(MPDM_MULTIPLE|MPDM_HASH|MPDM_IVAL,NULL,n,_mpdm_tie_mul())
#define MPDM_LS(s)	_mpdm_new_wcs(s,-1,_mpdm_tie_lstr())
#define MPDM_S(s)	_mpdm_new_wcs(s,-1,_mpdm_tie_str())
#define MPDM_NS(s,n)	_mpdm_new_wcs(s,n,_mpdm_tie_str())

#define MPDM_I(i)	_mpdm_inew((i))
#define MPDM_X(f)	mpdm_new(MPDM_EXEC,f,0,NULL)
#define MPDM_P(p)	mpdm_new(0,(void *)p,0,NULL)
#define MPDM_M(m,s)	mpdm_new(0,m,s,_mpdm_tie_cpy())
#define MPDM_MBS(s)	_mpdm_new_mbs(s,-1,_mpdm_tie_mbstowcs())
#define MPDM_2MBS(s)	_mpdm_new_wcs(s,-1,_mpdm_tie_wcstombs())
