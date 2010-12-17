/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2010 Angel Ortega <angel@triptico.com>

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

#ifdef __cplusplus
extern "C" {
#endif

enum {
    _MPDM_STRING,
    _MPDM_MULTIPLE,
    _MPDM_FREE,
    _MPDM_IVAL,
    _MPDM_RVAL,
    _MPDM_HASH,
    _MPDM_FILE,
    _MPDM_EXEC,
    _MPDM_REGEX,
    _MPDM_MUTEX,
    _MPDM_SEMAPHORE,
    _MPDM_THREAD
};

enum {
    MPDM_STRING     = (1<<_MPDM_STRING),    /* data can be string-compared */
    MPDM_MULTIPLE   = (1<<_MPDM_MULTIPLE),  /* data is multiple */
    MPDM_FREE       = (1<<_MPDM_FREE),      /* free data at destroy */
    MPDM_IVAL       = (1<<_MPDM_IVAL),      /* integer value cached in .ival */
    MPDM_RVAL       = (1<<_MPDM_RVAL),      /* real value cached in .rval */
    MPDM_HASH       = (1<<_MPDM_HASH),      /* data is a hash */
    MPDM_FILE       = (1<<_MPDM_FILE),      /* data is a FILE * */
    MPDM_EXEC       = (1<<_MPDM_EXEC),      /* data is 'executable' */
    MPDM_REGEX      = (1<<_MPDM_REGEX),     /* data is a compiled regex */
    MPDM_MUTEX      = (1<<_MPDM_MUTEX),     /* data is a mutex */
    MPDM_SEMAPHORE  = (1<<_MPDM_SEMAPHORE), /* data is a semaphore */
    MPDM_THREAD     = (1<<_MPDM_THREAD)     /* data is a thread handle */
};

/* mpdm values */
typedef struct mpdm_val *mpdm_t;

/* a value */
struct mpdm_val {
    int flags;          /* value flags */
    int ref;            /* reference count */
    int size;           /* data size */
    int ival;           /* cached integer value */
    double rval;        /* cache real value */
    const void *data;   /* the real data */
    mpdm_t next;        /* next in chain */
};


/* the main control structure */
struct mpdm_control {
    mpdm_t root;        /* the root hash */
    mpdm_t del;         /* list of deleted values */
    int count;          /* total count of values */
    int hash_buckets;   /* default hash buckets */
};

extern struct mpdm_control *mpdm;

mpdm_t mpdm_new(int flags, const void *data, int size);
mpdm_t mpdm_ref(mpdm_t v);
mpdm_t mpdm_unref(mpdm_t v);
mpdm_t mpdm_unrefnd(mpdm_t v);

int mpdm_size(const mpdm_t v);
mpdm_t mpdm_clone(const mpdm_t v);
mpdm_t mpdm_root(void);

void mpdm_void(mpdm_t v);
int mpdm_is_null(mpdm_t v);

mpdm_t mpdm_exec(mpdm_t c, mpdm_t args, mpdm_t ctxt);
mpdm_t mpdm_exec_1(mpdm_t c, mpdm_t a1, mpdm_t ctxt);
mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t ctxt);
mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3, mpdm_t ctxt);

mpdm_t mpdm_new_a(int flags, int size);
mpdm_t mpdm_aclone(const mpdm_t v);

mpdm_t mpdm_expand(mpdm_t a, int offset, int num);
mpdm_t mpdm_collapse(mpdm_t a, int offset, int num);
mpdm_t mpdm_aset(mpdm_t a, mpdm_t e, int offset);
mpdm_t mpdm_aget(const mpdm_t a, int offset);
mpdm_t mpdm_ins(mpdm_t a, mpdm_t e, int offset);
mpdm_t mpdm_adel(mpdm_t a, int offset);
mpdm_t mpdm_shift(mpdm_t a);
mpdm_t mpdm_push(mpdm_t a, mpdm_t e);
mpdm_t mpdm_pop(mpdm_t a);
mpdm_t mpdm_queue(mpdm_t a, mpdm_t e, int size);
int mpdm_seek(const mpdm_t a, const mpdm_t k, int step);
int mpdm_seek_s(const mpdm_t a, const wchar_t * k, int step);
int mpdm_bseek(const mpdm_t a, const mpdm_t k, int step, int *pos);
int mpdm_bseek_s(const mpdm_t a, const wchar_t * k, int step, int *pos);
mpdm_t mpdm_sort(mpdm_t a, int step);
mpdm_t mpdm_sort_cb(mpdm_t a, int step, mpdm_t asort_cb);

mpdm_t mpdm_split_s(const wchar_t *s, const mpdm_t v);
mpdm_t mpdm_split(const mpdm_t s, const mpdm_t a);
mpdm_t mpdm_join_s(const wchar_t *s, const mpdm_t a);
mpdm_t mpdm_join(const mpdm_t s, const mpdm_t a);

void *mpdm_poke_o(void *dst, int *dsize, int *offset, const void *org, int osize, int esize);
void *mpdm_poke(void *dst, int *dsize, const void *org, int osize, int esize);
wchar_t *mpdm_pokewsn(wchar_t *dst, int *dsize, const wchar_t *str, int slen);
wchar_t *mpdm_pokews(wchar_t *dst, int *dsize, const wchar_t *str);
wchar_t *mpdm_pokev(wchar_t *dst, int *dsize, const mpdm_t v);
wchar_t *mpdm_mbstowcs(const char *str, int *s, int l);
char *mpdm_wcstombs(const wchar_t * str, int *s);
mpdm_t mpdm_new_wcs(int flags, const wchar_t * str, int size, int cpy);
mpdm_t mpdm_new_mbstowcs(int flags, const char *str, int l);
mpdm_t mpdm_new_wcstombs(int flags, const wchar_t * str);
mpdm_t mpdm_new_i(int ival);
mpdm_t mpdm_new_r(double rval);
int mpdm_wcwidth(wchar_t c);
mpdm_t mpdm_sprintf(const mpdm_t fmt, const mpdm_t args);
mpdm_t mpdm_ulc(const mpdm_t s, int u);
mpdm_t mpdm_sscanf(const mpdm_t fmt, const mpdm_t str, int offset);

wchar_t *mpdm_string(const mpdm_t v);
mpdm_t mpdm_splice(const mpdm_t v, const mpdm_t i, int offset, int del);
mpdm_t mpdm_strcat_sn(const mpdm_t s1, const wchar_t *s2, int size);
mpdm_t mpdm_strcat_s(const mpdm_t s1, const wchar_t *s2);
mpdm_t mpdm_strcat(const mpdm_t s1, const mpdm_t s2);
int mpdm_cmp(const mpdm_t v1, const mpdm_t v2);
int mpdm_cmp_s(const mpdm_t v1, const wchar_t *v2);
int mpdm_ival(mpdm_t v);
double mpdm_rval(mpdm_t v);
mpdm_t mpdm_set_ival(mpdm_t v, int ival);
mpdm_t mpdm_set_rval(mpdm_t v, double rval);

mpdm_t mpdm_xnew(mpdm_t(*a1) (mpdm_t, mpdm_t, mpdm_t), mpdm_t a2);
mpdm_t mpdm_new_copy(int flags, void *ptr, int size);

int mpdm_hsize(const mpdm_t h);
mpdm_t mpdm_hget(const mpdm_t h, const mpdm_t k);
mpdm_t mpdm_hget_s(const mpdm_t h, const wchar_t *k);
int mpdm_exists(const mpdm_t h, const mpdm_t k);
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v);
mpdm_t mpdm_hset_s(mpdm_t h, const wchar_t *k, mpdm_t v);
mpdm_t mpdm_hdel(mpdm_t h, const mpdm_t k);
mpdm_t mpdm_keys(const mpdm_t h);
int mpdm_iterator(mpdm_t h, int *context, mpdm_t *v1, mpdm_t *v2);

extern wchar_t * (*mpdm_dump_1) (const mpdm_t v, int l, wchar_t *ptr, int *size);
mpdm_t mpdm_dumper(const mpdm_t v);
void mpdm_dump(const mpdm_t v);

#define MPDM_SGET(r, k) mpdm_sget((r), MPDM_LS((k)))
#define MPDM_SSET(r, k, v) mpdm_sset((r), MPDM_LS((k)), (v))

mpdm_t mpdm_sget(mpdm_t r, mpdm_t k);
mpdm_t mpdm_sset(mpdm_t r, mpdm_t k, mpdm_t v);

int mpdm_write_wcs(FILE * f, const wchar_t * str);
mpdm_t mpdm_new_f(FILE * f);
mpdm_t mpdm_open(const mpdm_t filename, const mpdm_t mode);
mpdm_t mpdm_close(mpdm_t fd);
mpdm_t mpdm_read(const mpdm_t fd);
int mpdm_write(const mpdm_t fd, const mpdm_t v);
mpdm_t mpdm_getchar(const mpdm_t fd);
int mpdm_putchar(const mpdm_t fd, const mpdm_t c);
int mpdm_fseek(const mpdm_t fd, long offset, int whence);
long mpdm_ftell(const mpdm_t fd);
FILE * mpdm_get_filehandle(const mpdm_t fd);
int mpdm_encoding(mpdm_t charset);
int mpdm_unlink(const mpdm_t filename);
mpdm_t mpdm_stat(const mpdm_t filename);
int mpdm_chmod(const mpdm_t filename, mpdm_t perms);
int mpdm_chdir(const mpdm_t dir);
int mpdm_chown(const mpdm_t filename, mpdm_t uid, mpdm_t gid);
mpdm_t mpdm_glob(mpdm_t spec, mpdm_t base);

mpdm_t mpdm_popen(const mpdm_t prg, const mpdm_t mode);
mpdm_t mpdm_popen2(const mpdm_t prg);
mpdm_t mpdm_pclose(mpdm_t fd);

extern int mpdm_regex_offset;
extern int mpdm_regex_size;
extern int mpdm_sregex_count;

mpdm_t mpdm_regex(mpdm_t r, const mpdm_t v, int offset);
mpdm_t mpdm_sregex(mpdm_t r, const mpdm_t v, const mpdm_t s, int offset);

mpdm_t mpdm_gettext(const mpdm_t str);
int mpdm_gettext_domain(const mpdm_t dom, const mpdm_t data);

mpdm_t mpdm_home_dir(void);
mpdm_t mpdm_app_dir(void);

/* value type testing macros */

#define MPDM_IS_ARRAY(v)    ((v != NULL) && ((v)->flags) & MPDM_MULTIPLE)
#define MPDM_IS_HASH(v)     ((v != NULL) && ((v)->flags) & MPDM_HASH)
#define MPDM_IS_EXEC(v)     ((v != NULL) && ((v)->flags) & MPDM_EXEC)
#define MPDM_IS_STRING(v)   ((v != NULL) && ((v)->flags) & MPDM_STRING)

/* value creation utility macros */

#define MPDM_A(n)       mpdm_new_a(0,n)
#define MPDM_H(n)       mpdm_new_a(MPDM_HASH|MPDM_IVAL,n)
#define MPDM_LS(s)      mpdm_new_wcs(0, s, -1, 0)
#define MPDM_S(s)       mpdm_new_wcs(0, s, -1, 1)
#define MPDM_NS(s,n)    mpdm_new_wcs(0, s, n, 1)
#define MPDM_ENS(s,n)   mpdm_new(MPDM_STRING|MPDM_FREE, s, n)
#define MPDM_C(f,p,s)   mpdm_new_copy(f,p,s)

#define MPDM_I(i)       mpdm_new_i((i))
#define MPDM_R(r)       mpdm_new_r((r))
#define MPDM_P(p)       mpdm_new(0,(void *)p, 0, NULL)
#define MPDM_MBS(s)     mpdm_new_mbstowcs(0, s, -1)
#define MPDM_NMBS(s,n)  mpdm_new_mbstowcs(0, s, n)
#define MPDM_2MBS(s)    mpdm_new_wcstombs(0, s)

#define MPDM_X(f)       mpdm_new(MPDM_EXEC, (const void *)f, 0)
#define MPDM_X2(f,b)    mpdm_xnew(f,b)

#define MPDM_F(f)       mpdm_new_f(f)

int mpdm_startup(void);
void mpdm_shutdown(void);

void mpdm_sleep(int msecs);

mpdm_t mpdm_new_mutex(void);
void mpdm_mutex_lock(mpdm_t mutex);
void mpdm_mutex_unlock(mpdm_t mutex);

mpdm_t mpdm_new_semaphore(int init_value);
void mpdm_semaphore_wait(mpdm_t sem);
void mpdm_semaphore_post(mpdm_t sem);

mpdm_t mpdm_exec_thread(mpdm_t c, mpdm_t args, mpdm_t ctxt);

#ifdef __cplusplus
}
#endif
