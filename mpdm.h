/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

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
#define MPDM_FREE	0x00000004	/* free data at destroy */
#define MPDM_NONDYN	0x00000008	/* value is non-dynamic */

#define MPDM_IVAL	0x00000010	/* integer value cached in .ival */
#define MPDM_RVAL	0x00000020	/* real value cached in .rval */

/* 'informative' flags */
#define MPDM_HASH	0x00010000	/* data is a hash */
#define MPDM_FILE	0x00020000	/* data is a FILE * */
#define MPDM_EXEC	0x00040000	/* data is 'executable' */

/* mpdm values */
typedef struct _mpdm_v * mpdm_t;

/* a value */
struct _mpdm_v
{
	int flags;	/* value flags */
	int ref;	/* reference count */
	int size;	/* data size */
	void * data;	/* the real data */
	int ival;	/* cached integer value */
	double rval;	/* cache real value */
	mpdm_t next;	/* next in chain */
};


/* the main control structure */
struct _mpdm_ctl
{
	mpdm_t root;		/* the root hash */
	mpdm_t cur;		/* current value (circular list) */
	int count;		/* total count of values */
	int low_threshold;	/* minimum number of values to start sweeping */
	int default_sweep;	/* default swept values on mpdm_sweep(0) */
	int memory_usage;	/* approximate total memory used */
	int nd_index;		/* index to next non-dyn value */
	int nd_size;		/* size of nd_pool */
	mpdm_t nd_pool;		/* pool of non-dyn values */
	mpdm_t encoding;	/* file encoding charset */
	mpdm_t regex;		/* regular expression cache */
	mpdm_t i18n;		/* translated strings cache */
};

extern struct _mpdm_ctl * _mpdm;

mpdm_t mpdm_new(int flags, void * data, int size);
mpdm_t mpdm_ref(mpdm_t v);
mpdm_t mpdm_unref(mpdm_t v);
void mpdm_sweep(int count);

int mpdm_size(mpdm_t v);
mpdm_t mpdm_clone(mpdm_t v);
mpdm_t mpdm_root(void);

mpdm_t mpdm_exec(mpdm_t c, mpdm_t args);
mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2);
mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3);

mpdm_t _mpdm_new_a(int flags, int size);
mpdm_t _mpdm_aclone(mpdm_t v);

mpdm_t mpdm_aexpand(mpdm_t a, int offset, int num);
mpdm_t mpdm_acollapse(mpdm_t a, int offset, int num);
mpdm_t mpdm_aset(mpdm_t a, mpdm_t e, int offset);
mpdm_t mpdm_aget(mpdm_t a, int offset);
mpdm_t mpdm_ains(mpdm_t a, mpdm_t e, int offset);
mpdm_t mpdm_adel(mpdm_t a, int offset);
mpdm_t mpdm_apush(mpdm_t a, mpdm_t e);
mpdm_t mpdm_apop(mpdm_t a);
mpdm_t mpdm_aqueue(mpdm_t a, mpdm_t e, int size);
int mpdm_aseek(mpdm_t a, mpdm_t k, int step);
int mpdm_abseek(mpdm_t a, mpdm_t k, int step, int * pos);
mpdm_t mpdm_asort(mpdm_t a, int step);
mpdm_t mpdm_asort_cb(mpdm_t a, int step, mpdm_t asort_cb);

mpdm_t mpdm_asplit(mpdm_t s, mpdm_t a);
mpdm_t mpdm_ajoin(mpdm_t s, mpdm_t a);

wchar_t * _mpdm_mbstowcs(char * str, int * s);
mpdm_t _mpdm_new_wcs(int flags, wchar_t * str, int size, int cpy);
mpdm_t _mpdm_new_mbstowcs(int flags, char * str);
mpdm_t _mpdm_new_wcstombs(int flags, wchar_t * str);
mpdm_t _mpdm_new_i(int ival);
mpdm_t _mpdm_new_r(double rval);

wchar_t * mpdm_string(mpdm_t v);
mpdm_t mpdm_splice(mpdm_t v, mpdm_t i, int offset, int del);
mpdm_t mpdm_strcat(mpdm_t s1, mpdm_t s2);
int mpdm_cmp(mpdm_t v1, mpdm_t v2);
int mpdm_ival(mpdm_t v);
double mpdm_rval(mpdm_t v);

mpdm_t _mpdm_xnew(mpdm_t (* a1)(mpdm_t, mpdm_t), mpdm_t a2);

int mpdm_hsize(mpdm_t h);
mpdm_t mpdm_hget(mpdm_t h, mpdm_t k);
mpdm_t mpdm_hget_s(mpdm_t h, wchar_t * k);
int mpdm_hexists(mpdm_t h, mpdm_t k);
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v);
mpdm_t mpdm_hset_s(mpdm_t h, wchar_t * k, mpdm_t v);
mpdm_t mpdm_hdel(mpdm_t h, mpdm_t k);
mpdm_t mpdm_hkeys(mpdm_t h);

void mpdm_dump(mpdm_t v);

#define MPDM_SGET(r, k) mpdm_sget((r), MPDM_LS((k)))
#define MPDM_SSET(r, k, v) mpdm_sset((r), MPDM_LS((k)), (v))

mpdm_t mpdm_sget(mpdm_t r, mpdm_t k);
mpdm_t mpdm_sset(mpdm_t r, mpdm_t k, mpdm_t v);

void _mpdm_write_wcs(FILE * f, wchar_t * str);
mpdm_t mpdm_open(mpdm_t filename, mpdm_t mode);
mpdm_t mpdm_close(mpdm_t fd);
mpdm_t mpdm_read(mpdm_t fd);
int mpdm_write(mpdm_t fd, mpdm_t v);
int mpdm_encoding(mpdm_t charset);
int mpdm_unlink(mpdm_t filename);
mpdm_t mpdm_glob(mpdm_t spec);

mpdm_t mpdm_regex(mpdm_t r, mpdm_t v, int offset);
mpdm_t mpdm_sregex(mpdm_t r, mpdm_t v, mpdm_t s, int offset);

mpdm_t mpdm_gettext(mpdm_t str);
void mpdm_gettext_domain(mpdm_t dom, mpdm_t data);

/* value creation utility macros */

#define MPDM_A(n)	_mpdm_new_a(0,n)
#define MPDM_H(n)	_mpdm_new_a(MPDM_HASH|MPDM_IVAL,n)
#define MPDM_LS(s)	_mpdm_new_wcs(0, s, -1, 0)
#define MPDM_S(s)	_mpdm_new_wcs(0, s, -1, 1)
#define MPDM_NS(s,n)	_mpdm_new_wcs(0, s, n, 1)

#define MPDM_I(i)	_mpdm_new_i((i))
#define MPDM_R(r)	_mpdm_new_r((r))
#define MPDM_P(p)	mpdm_new(0,(void *)p,0,NULL)
#define MPDM_MBS(s)	_mpdm_new_mbstowcs(0,s)
#define MPDM_2MBS(s)	_mpdm_new_wcstombs(0,s)

#define MPDM_X(f)	mpdm_new(MPDM_EXEC,f,0)
#define MPDM_X2(f,b)	_mpdm_xnew(f,b)

#define MPDM_ND_BEGIN()	unsigned int _mpdm_nd_save=_mpdm->nd_index
#define MPDM_ND_END()	_mpdm->nd_index=_mpdm_nd_save

#define MPDM_ND_LS(s)	_mpdm_new_wcs(MPDM_NONDYN, s, -1, 0)
#define MPDM_ND_A(v)	mpdm_new(MPDM_MULTIPLE|MPDM_NONDYN,\
				v,(sizeof(v) / sizeof(mpdm_t)))

int mpdm_startup(void);
void mpdm_shutdown(void);
