/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

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

    http://triptico.com

*/

#ifndef MPDM_H_
#define MPDM_H_

#ifdef CONFOPT_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPDM_TYPE_NULL,
    MPDM_TYPE_SCALAR,
    MPDM_TYPE_ARRAY,
	MPDM_TYPE_OBJECT,
	MPDM_TYPE_FILE,
    MPDM_TYPE_MBS,
    MPDM_TYPE_REGEX,
    MPDM_TYPE_MUTEX,
	MPDM_TYPE_SEMAPHORE,
	MPDM_TYPE_THREAD,
	MPDM_TYPE_FUNCTION,
	MPDM_TYPE_PROGRAM
} mpdm_type_t;

#define MPDM_MAX_TYPES 16
#define MPDM_MASK_FOR_TYPE (MPDM_MAX_TYPES - 1)

enum {
    _MPDM_FREE = MPDM_MASK_FOR_TYPE + 1,
    _MPDM_MULTIPLE,
    _MPDM_IVAL,
    _MPDM_RVAL,
    _MPDM_EXEC,
    _MPDM_CHANNEL,
    _MPDM_EXTENDED
};

enum {
    MPDM_FREE       = (1<<_MPDM_FREE),      /* free data at destroy */
    MPDM_MULTIPLE   = (1<<_MPDM_MULTIPLE),  /* data is multiple */
    MPDM_IVAL       = (1<<_MPDM_IVAL),      /* integer value cached in .ival */
    MPDM_RVAL       = (1<<_MPDM_RVAL),      /* real value cached in .rval */
    MPDM_EXEC       = (1<<_MPDM_EXEC),      /* data is 'executable' */
    MPDM_CHANNEL    = (1<<_MPDM_CHANNEL),   /* data is a channel */
    MPDM_EXTENDED   = (1<<_MPDM_EXTENDED)   /* value is an mpdm_ex_t */
};

/* mpdm values */
typedef struct mpdm_val *mpdm_t;
typedef struct mpdm_val_ex *mpdm_ex_t;

/* a value */
struct mpdm_val {
    int flags;          /* value flags */
    int ref;            /* reference count */
    size_t size;        /* data size */
    const void *data;   /* the real data */
};

/* extended value */
struct mpdm_val_ex {
    int flags;                  /* value flags */
    int ref;                    /* reference count */
    size_t size;                /* data size */
    const void *data;           /* the real data */
    int ival;                   /* cached integer value */
    double rval;                /* cache real value */
};

/* type information */
struct mpdm_type_info {
    wchar_t *name;
    void (*destroy)(mpdm_t);
};

extern struct mpdm_type_info mpdm_type_infos[MPDM_MAX_TYPES];

/* the main control structure */
struct mpdm_control {
    mpdm_t root;            /* the root hash */
    int count;              /* total count of values */
    int hash_buckets;       /* default hash buckets */
};

extern struct mpdm_control *mpdm;

/* value type testing macros */

#define MPDM_IS_EXEC(v)     ((v != NULL) && ((v)->flags) & MPDM_EXEC)
#define MPDM_IS_FILE(v)     (mpdm_type(v) == MPDM_TYPE_FILE)
#define MPDM_HAS_IVAL(v)    ((v != NULL) && ((v)->flags) & MPDM_IVAL)
#define MPDM_HAS_RVAL(v)    ((v != NULL) && ((v)->flags) & MPDM_RVAL)

#define MPDM_CAN_GET(v)     (mpdm_type(v) == MPDM_TYPE_FUNCTION || \
                             mpdm_type(v) == MPDM_TYPE_PROGRAM || \
                             mpdm_type(v) == MPDM_TYPE_ARRAY || \
                             mpdm_type(v) == MPDM_TYPE_OBJECT)

/* value creation utility macros */

#define MPDM_A(n)       mpdm_new_a(0, n)
#define MPDM_H(n)       mpdm_new_h(n)
#define MPDM_LS(s)      mpdm_new_wcs(0, s, -1, 0)
#define MPDM_S(s)       mpdm_new_wcs(0, s, -1, 1)
#define MPDM_NS(s,n)    mpdm_new_wcs(0, s, n, 1)
#define MPDM_ENS(s,n)   mpdm_new_wcs(MPDM_FREE, s, n, 0)
#define MPDM_C(f,p,s)   mpdm_new_copy(f,p,s)

#define MPDM_I(i)       mpdm_new_i((i))
#define MPDM_R(r)       mpdm_new_r((r))
#define MPDM_P(p)       mpdm_new(0, (void *)p, 0, NULL)
#define MPDM_MBS(s)     mpdm_new_mbstowcs(s, -1)
#define MPDM_NMBS(s,n)  mpdm_new_mbstowcs(s, n)
#define MPDM_2MBS(s)    mpdm_new_wcstombs(s)

#define MPDM_X(f)       mpdm_new_x(MPDM_TYPE_FUNCTION, f, NULL)
#define MPDM_X2(f,a)    mpdm_new_x(MPDM_TYPE_PROGRAM, f, a)

#define MPDM_F(f)       mpdm_new_f(f)

mpdm_t mpdm_init(mpdm_t v, int flags, const void *data, size_t size);
mpdm_t mpdm_new(int flags, const void *data, size_t size);
mpdm_type_t mpdm_type(mpdm_t v);
mpdm_t mpdm_ref(mpdm_t v);
mpdm_t mpdm_unref(mpdm_t v);
mpdm_t mpdm_unrefnd(mpdm_t v);
size_t mpdm_size(const mpdm_t v);
mpdm_t mpdm_root(void);
mpdm_t mpdm_void(mpdm_t v);
int mpdm_is_null(mpdm_t v);
mpdm_t mpdm_store(mpdm_t *v, mpdm_t w);
mpdm_t mpdm_new_copy(int flags, void *ptr, size_t size);
int mpdm_startup(void);
void mpdm_shutdown(void);

extern wchar_t * (*mpdm_dump_1) (const mpdm_t v, int l, wchar_t *ptr, size_t *size);
mpdm_t mpdm_dumper(const mpdm_t v);
void mpdm_dump(const mpdm_t v);

void mpdm_array__destroy(mpdm_t a);
mpdm_t mpdm_new_a(int flags, size_t size);
mpdm_t mpdm_expand(mpdm_t a, int index, int num);
mpdm_t mpdm_collapse(mpdm_t a, int index, int num);
mpdm_t mpdm_aset(mpdm_t a, mpdm_t e, int index);
mpdm_t mpdm_aget(const mpdm_t a, int index);
mpdm_t mpdm_ins(mpdm_t a, mpdm_t e, int index);
mpdm_t mpdm_adel(mpdm_t a, int index);
mpdm_t mpdm_shift(mpdm_t a);
mpdm_t mpdm_push(mpdm_t a, mpdm_t e);
mpdm_t mpdm_pop(mpdm_t a);
mpdm_t mpdm_queue(mpdm_t a, mpdm_t e, size_t size);
mpdm_t mpdm_clone(const mpdm_t v);
int mpdm_seek_s(const mpdm_t a, const wchar_t *v, int step);
int mpdm_seek(const mpdm_t a, const mpdm_t v, int step);
int mpdm_bseek_s(const mpdm_t a, const wchar_t *v, int step, int *pos);
int mpdm_bseek(const mpdm_t a, const mpdm_t v, int step, int *pos);
mpdm_t mpdm_sort(mpdm_t a, int step);
mpdm_t mpdm_sort_cb(mpdm_t a, int step, mpdm_t asort_cb);
mpdm_t mpdm_split_s(const mpdm_t v, const wchar_t *s);
mpdm_t mpdm_split(const mpdm_t a, const mpdm_t s);
mpdm_t mpdm_join_s(const mpdm_t a, const wchar_t *s);
mpdm_t mpdm_join(const mpdm_t a, const mpdm_t s);
mpdm_t mpdm_reverse(const mpdm_t a);

void *mpdm_poke_o(void *dst, size_t *dsize, int *offset, const void *org, size_t osize, size_t esize);
void *mpdm_poke(void *dst, size_t *dsize, const void *org, size_t osize, size_t esize);
wchar_t *mpdm_pokewsn(wchar_t *dst, size_t *dsize, const wchar_t *str, size_t slen);
wchar_t *mpdm_pokews(wchar_t *dst, size_t *dsize, const wchar_t *str);
wchar_t *mpdm_pokev(wchar_t *dst, size_t *dsize, const mpdm_t v);
wchar_t *mpdm_mbstowcs(const char *str, size_t *s, size_t l);
char *mpdm_wcstombs(const wchar_t * str, size_t *s);
mpdm_t mpdm_new_wcs(int flags, const wchar_t *str, size_t size, int cpy);
mpdm_t mpdm_new_mbstowcs(const char *str, size_t l);
mpdm_t mpdm_new_wcstombs(const wchar_t *str);
mpdm_t mpdm_new_i(int ival);
mpdm_t mpdm_new_r(double rval);
wchar_t *mpdm_string(const mpdm_t v);
int mpdm_cmp(const mpdm_t v1, const mpdm_t v2);
int mpdm_cmp_s(const mpdm_t v1, const wchar_t *v2);
mpdm_t mpdm_splice(const mpdm_t v, const mpdm_t i, int offset, int del);
mpdm_t mpdm_slice(const mpdm_t s, int offset, int num);
mpdm_t mpdm_strcat_sn(const mpdm_t s1, const wchar_t *s2, size_t size);
mpdm_t mpdm_strcat_s(const mpdm_t s1, const wchar_t *s2);
mpdm_t mpdm_strcat(const mpdm_t s1, const mpdm_t s2);
int mpdm_ival_mbs(char *str);
int mpdm_ival(mpdm_t v);
double mpdm_rval_mbs(char *str);
double mpdm_rval(mpdm_t v);
mpdm_t mpdm_gettext(const mpdm_t str);
int mpdm_gettext_domain(const mpdm_t dom, const mpdm_t data);
int mpdm_wcwidth(wchar_t c);
mpdm_t mpdm_fmt(const mpdm_t fmt, const mpdm_t arg);
mpdm_t mpdm_sprintf(const mpdm_t fmt, const mpdm_t args);
mpdm_t mpdm_ulc(const mpdm_t s, int u);
mpdm_t mpdm_sscanf(const mpdm_t str, const mpdm_t fmt, int offset);
mpdm_t mpdm_tr(mpdm_t str, mpdm_t s1, mpdm_t s2);

void mpdm_object__destroy(mpdm_t a);
mpdm_t mpdm_new_h(size_t size);
size_t mpdm_hsize(const mpdm_t h);
mpdm_t mpdm_hget_s(const mpdm_t h, const wchar_t *k);
mpdm_t mpdm_hget(const mpdm_t h, const mpdm_t k);
int mpdm_exists(const mpdm_t h, const mpdm_t i);
mpdm_t mpdm_hset(mpdm_t h, mpdm_t k, mpdm_t v);
mpdm_t mpdm_hset_s(mpdm_t h, const wchar_t *k, mpdm_t v);
mpdm_t mpdm_hdel(mpdm_t h, const mpdm_t k);
mpdm_t mpdm_keys(const mpdm_t h);
int mpdm_iterator_o(mpdm_t set, int *context, mpdm_t *v, mpdm_t *i);

wchar_t *mpdm_read_mbs(FILE *f, size_t *s);
size_t mpdm_write_wcs(FILE * f, const wchar_t * str);
mpdm_t mpdm_open(const mpdm_t filename, const mpdm_t mode);
mpdm_t mpdm_read(const mpdm_t fd);
wchar_t *mpdm_file_eol(mpdm_t fd);
mpdm_t mpdm_getchar(const mpdm_t fd);
int mpdm_putchar(const mpdm_t fd, const mpdm_t c);
size_t mpdm_write(const mpdm_t fd, const mpdm_t v);
int mpdm_fseek(const mpdm_t fd, long offset, int whence);
long mpdm_ftell(const mpdm_t fd);
FILE * mpdm_get_filehandle(const mpdm_t fd);
int mpdm_encoding(mpdm_t charset);
int mpdm_unlink(const mpdm_t filename);
int mpdm_rename(const mpdm_t o, const mpdm_t n);
mpdm_t mpdm_stat(const mpdm_t filename);
int mpdm_chmod(const mpdm_t filename, mpdm_t perms);
int mpdm_chdir(const mpdm_t dir);
mpdm_t mpdm_getcwd(void);
int mpdm_chown(const mpdm_t filename, mpdm_t uid, mpdm_t gid);
mpdm_t mpdm_glob(mpdm_t spec, mpdm_t base);
mpdm_t mpdm_popen(const mpdm_t prg, const mpdm_t mode);
mpdm_t mpdm_popen2(const mpdm_t prg);
int mpdm_pclose(mpdm_t fd);
mpdm_t mpdm_home_dir(void);
mpdm_t mpdm_app_dir(void);
mpdm_t mpdm_connect(mpdm_t host, mpdm_t serv);
void mpdm_file__destroy(mpdm_t v);
mpdm_t mpdm_new_f(FILE * f);
int mpdm_close(mpdm_t fd);

extern size_t mpdm_regex_offset;
extern size_t mpdm_regex_size;
extern int mpdm_sregex_count;

void mpdm_regex__destroy(mpdm_t v);
mpdm_t mpdm_regex(const mpdm_t v, const mpdm_t r, int offset);
mpdm_t mpdm_sregex(const mpdm_t v, const mpdm_t r, const mpdm_t s, int offset);

void mpdm_sleep(int msecs);
void mpdm_mutex__destroy(mpdm_t v);
mpdm_t mpdm_new_mutex(void);
void mpdm_mutex_lock(mpdm_t mutex);
void mpdm_mutex_unlock(mpdm_t mutex);
void mpdm_semaphore__destroy(mpdm_t v);
mpdm_t mpdm_new_semaphore(int init_value);
void mpdm_semaphore_wait(mpdm_t sem);
void mpdm_semaphore_post(mpdm_t sem);
void mpdm_thread__destroy(mpdm_t v);
mpdm_t mpdm_exec_thread(mpdm_t c, mpdm_t args, mpdm_t ctxt);
void mpdm_new_channel(mpdm_t *parent, mpdm_t *child);
mpdm_t mpdm_channel_read(mpdm_t channel);
void mpdm_channel_write(mpdm_t channel, mpdm_t v);

int mpdm_is_true(mpdm_t v);
mpdm_t mpdm_bool(int b);
mpdm_t mpdm_new_x(mpdm_type_t type, void *f, mpdm_t a);
mpdm_t mpdm_get(mpdm_t set, mpdm_t i);
mpdm_t mpdm_set(mpdm_t set, mpdm_t v, mpdm_t i);
mpdm_t mpdm_exec(mpdm_t c, mpdm_t args, mpdm_t ctxt);
mpdm_t mpdm_exec_1(mpdm_t c, mpdm_t a1, mpdm_t ctxt);
mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t ctxt);
mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3, mpdm_t ctxt);
int mpdm_iterator(mpdm_t set, int *context, mpdm_t *v, mpdm_t *i);
mpdm_t mpdm_map(mpdm_t set, mpdm_t filter, mpdm_t ctxt);
mpdm_t mpdm_hmap(mpdm_t set, mpdm_t filter, mpdm_t ctxt);
mpdm_t mpdm_grep(mpdm_t set, mpdm_t filter, mpdm_t ctxt);

#ifdef __cplusplus
}
#endif

#endif /* MPDM_H_ */
