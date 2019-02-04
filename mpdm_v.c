/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

    mpdm_v.c - Basic value management

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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "mpdm.h"


/** data **/

/* control structure */
struct mpdm_control *mpdm = NULL;

/* data information */
struct mpdm_type_info mpdm_type_infos[MPDM_MAX_TYPES] = {
    { L"null",      NULL },
    { L"scalar",    NULL },
    { L"array",     mpdm_array__destroy },
    { L"object",    mpdm_object__destroy },
    { L"file",      mpdm_file__destroy },
    { L"mbs",       NULL },
    { L"regex",     mpdm_regex__destroy },
    { L"mutex",     mpdm_mutex__destroy },
    { L"semaphore", mpdm_semaphore__destroy },
    { L"thread",    mpdm_thread__destroy }
};


/** code **/

#define V_SIZE(f) f & MPDM_EXTENDED ? sizeof(struct mpdm_val_ex) : sizeof(struct mpdm_val)


static mpdm_t destroy_value(mpdm_t v)
/* destroys a value */
{
    struct mpdm_type_info *ti = &mpdm_type_infos[mpdm_type(v)];

    /* if type has a destroy function, call it */
    if (ti->destroy)
        ti->destroy(v);

    /* free data if needed */
    if (v->flags & MPDM_FREE)
        free((void *)v->data);

    mpdm->count--;

    /* garble the memory block */
    memset(v, 0xaa, V_SIZE(v->flags));

    free(v);

    return NULL;
}


/**
 * mpdm_init - Initializes a value.
 * @v: the value to initialize
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 *
 * Initializes a value.
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 * [Value Creation]
 */
mpdm_t mpdm_init(mpdm_t v, int flags, const void *data, size_t size)
{
    /* if v is NULL crash JUST NOW */
    v->flags    = flags;
    v->ref      = 0;
    v->data     = data;
    v->size     = size;

    mpdm->count++;

    return v;
}


/**
 * mpdm_new - Creates a new value.
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @flags is an or-ed set of flags, @data is a
 * pointer to the data the value will store and @size the size of these
 * data (if value is to be a multiple one, @size is a number of elements,
 * or a number of bytes otherwise).
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 * [Value Creation]
 */
mpdm_t mpdm_new(int flags, const void *data, size_t size)
{
    mpdm_t v;

    v = (mpdm_t) calloc(V_SIZE(flags), 1);

    return mpdm_init(v, flags, data, size);
}


mpdm_type_t mpdm_type(mpdm_t v)
{
    return (mpdm_type_t) (v ? (v->flags & MPDM_MASK_FOR_TYPE) : MPDM_TYPE_NULL);
}


/**
 * mpdm_ref - Increments the reference count of a value.
 * @v: the value
 *
 * Increments the reference count of a value.
 * [Value Management]
 */
mpdm_t mpdm_ref(mpdm_t v)
{
    if (v != NULL)
        v->ref++;

    return v;
}


/**
 * mpdm_unref - Decrements the reference count of a value.
 * @v: the value
 *
 * Decrements the reference count of a value. If the reference
 * count of the value reaches 0, it's destroyed.
 * [Value Management]
 */
mpdm_t mpdm_unref(mpdm_t v)
{
    if (v != NULL && --v->ref <= 0)
        v = destroy_value(v);

    return v;
}


/**
 * mpdm_unrefnd - Decrements the reference count of a value, without destroy.
 * @v: the value
 *
 * Decrements the reference count of a value, without destroying
 * the value if it's unreferenced.
 * [Value Management]
 */
mpdm_t mpdm_unrefnd(mpdm_t v)
{
    if (v != NULL)
        v->ref--;

    return v;
}


/**
 * mpdm_size - Returns the size of an element.
 * @v: the element
 *
 * Returns the size of an element. It does not change the
 * reference count of the value.
 * [Value Management]
 */
size_t mpdm_size(const mpdm_t v)
{
    return v ? v->size : 0;
}


/**
 * mpdm_root - Returns the root hash.
 *
 * Returns the root hash. This hash is stored internally and can be used
 * as a kind of global symbol table.
 * [Value Management]
 */
mpdm_t mpdm_root(void)
{
    return mpdm->root = mpdm->root ? mpdm->root : mpdm_ref(MPDM_H(0));
}


/**
 * mpdm_void - Refs then unrefs a value.
 * @v: the value
 *
 * References and unreferences a value. To be used to receive
 * the output of mpdm_exec() in case of it being void (i.e.
 * its return value ignored).
 */
mpdm_t mpdm_void(mpdm_t v)
{
    return mpdm_unref(mpdm_ref(v));
}


/**
 * mpdm_is_null - Returns 1 if a value is NULL.
 * @v: the value
 *
 * Returns 1 if a value is NULL. The reference count is touched.
 */
int mpdm_is_null(mpdm_t v)
{
    int r;

    mpdm_ref(v);
    r = (v == NULL);
    mpdm_unref(v);

    return r;
}


mpdm_t mpdm_store(mpdm_t *v, mpdm_t w)
{
    mpdm_ref(w);
    mpdm_unref(*v);
    *v = w;

    return w;
}


mpdm_t mpdm_new_copy(int flags, void *ptr, size_t size)
{
    mpdm_t r = NULL;

    if (ptr != NULL) {
        char *ptr2 = malloc(size);
        memcpy(ptr2, ptr, size);

        r = mpdm_new(MPDM_FREE | flags, ptr2, size);
    }

    return r;
}


extern char *mpdm_build_git_rev;
extern char *mpdm_build_timestamp;

static mpdm_t MPDM(const mpdm_t args, mpdm_t ctxt)
/* accesor / mutator for MPDM internal data */
{
    mpdm_t v;

    mpdm_ref(args);

    v = mpdm_aget(args, 0);

    if (v != NULL) {
        mpdm_t w;

        /* do changes */
        if ((w = mpdm_hget_s(v, L"hash_buckets")) != NULL)
            mpdm->hash_buckets = mpdm_ival(w);
    }

    /* now collect all information */
    v = MPDM_H(0);

    mpdm_ref(v);

    mpdm_hset_s(v, L"version",          MPDM_MBS(VERSION));
    mpdm_hset_s(v, L"count",            MPDM_I(mpdm->count));
    mpdm_hset_s(v, L"hash_buckets",     MPDM_I(mpdm->hash_buckets));
    mpdm_hset_s(v, L"build_git_rev",    MPDM_MBS(mpdm_build_git_rev));
    mpdm_hset_s(v, L"build_timestamp",  MPDM_MBS(mpdm_build_timestamp));

    mpdm_unref(args);

    mpdm_unrefnd(v);

    return v;
}

extern char **environ;

static mpdm_t build_env(void)
/* builds a hash with the environment */
{
    char **ptr;
    mpdm_t e = MPDM_H(0);

    mpdm_ref(e);

    for (ptr = environ; *ptr != NULL; ptr++) {
        char *eq = strchr(*ptr, '=');

        if (eq != NULL) {
            mpdm_t k, v;

            k = MPDM_NMBS((*ptr), eq - (*ptr));
            v = MPDM_MBS(eq + 1);

            mpdm_hset(e, k, v);
        }
    }

    mpdm_unrefnd(e);

    return e;
}


/**
 * mpdm_startup - Initializes MPDM.
 *
 * Initializes the Minimum Profit Data Manager. Returns 0 if
 * everything went OK.
 */
int mpdm_startup(void)
{
    /* do the startup only unless done beforehand */
    if (mpdm == NULL) {
        /* alloc space */
        mpdm = calloc(sizeof(struct mpdm_control), 1);

        /* sets the defaults */
        mpdm->hash_buckets = 31;

        /* sets the locale */
        if (setlocale(LC_ALL, "") == NULL)
            if (setlocale(LC_ALL, "C.UTF-8") == NULL)
                setlocale(LC_ALL, "C");

        mpdm_encoding(NULL);

        /* store the MPDM() function */
        mpdm_hset_s(mpdm_root(), L"MPDM", MPDM_X(MPDM));

        /* store the ENV hash */
        mpdm_hset_s(mpdm_root(), L"ENV", build_env());

        /* store the special values TRUE and FALSE */
        mpdm_hset_s(mpdm_root(), L"TRUE",  MPDM_I(1));
        mpdm_hset_s(mpdm_root(), L"FALSE", MPDM_I(0));
    }

    /* everything went OK */
    return 0;
}


/**
 * mpdm_shutdown - Shuts down MPDM.
 *
 * Shuts down MPDM. No MPDM functions should be used from now on.
 */
void mpdm_shutdown(void)
{
    /* dummy, by now */
}

/**
 * MPDM_A - Creates an array value.
 * @n: Number of elements
 *
 * Creates a new array value with @n elements.
 * [Value Creation]
 */
/** mpdm_t MPDM_A(int n); */
/* ; */

/**
 * MPDM_H - Creates a hash value.
 * @n: Number of buckets in the hash (0: use default)
 *
 * Creates a new hash value with @n buckets. The number
 * of buckets must be a prime number. If @n is 0, an
 * optimal number of buckets will be used.
 * [Value Creation]
 */
/** mpdm_t MPDM_H(int n); */
/* ; */

/**
 * MPDM_LS - Creates a string value from a literal string.
 * @wcs: the wide character string
 *
 * Creates a new string value from a literal, wide character string.
 * A pointer to the string will be stored in the value (not a copy).
 * [Value Creation]
 */
/** mpdm_t MPDM_LS(wchar_t * wcs); */
/* ; */

/**
 * MPDM_S - Creates a string value from a string.
 * @wcs: the wide character string
 *
 * Creates a new string value from a wide character string. The value
 * will store a copy of the string that will be freed on destruction.
 * [Value Creation]
 */
/** mpdm_t MPDM_S(wchar_t * wcs); */
/* ; */

/**
 * MPDM_NS - Creates a string value from a string, with size.
 * @wcs: the wide character string
 * @s: the size in chars the string will hold
 *
 * Creates a new string value with a copy of the first @s characters
 * from the @wcs string.
 * [Value Creation]
 */
/** mpdm_t MPDM_NS(wchar_t * wcs, int s); */
/* ; */

/**
 * MPDM_ENS - Creates a string value from an external string, with size.
 * @wcs: the external wide character string
 * @s: the size in chars the string will hold
 *
 * Creates a new string value with size @s. The @wcs string must be
 * a dynamic value (i.e. allocated by malloc()) that will be freed on
 * destruction.
 * [Value Creation]
 */
/** mpdm_t MPDM_ENS(wchar_t * wcs, int s); */
/* ; */

/**
 * MPDM_I - Creates an integer value.
 * @i: the integer
 *
 * Creates a new integer value. MPDM integers are strings.
 * [Value Creation]
 */
/** mpdm_t MPDM_I(int i); */
/* ; */

/**
 * MPDM_R - Creates a real value.
 * @r: the real number
 *
 * Creates a new real value. MPDM integers are strings.
 * [Value Creation]
 */
/** mpdm_t MPDM_R(double r); */
/* ; */

/**
 * MPDM_F - Creates a file value.
 * @f: the file descriptor
 *
 * Creates a new file value.
 * [Value Creation]
 */
/** mpdm_t MPDM_F(FILE * f); */
/* ; */

/**
 * MPDM_MBS - Creates a string value from a multibyte string.
 * @mbs: the multibyte string
 *
 * Creates a new string value from a multibyte string, that will be
 * converted to wcs by mpdm_mbstowcs().
 * [Value Creation]
 */
/** mpdm_t MPDM_MBS(char * mbs); */
/* ; */

/**
 * MPDM_NMBS - Creates a string value from a multibyte string, with size.
 * @mbs: the multibyte string
 * @s: the size
 *
 * Creates a new string value with the first @s characters from the @mbs
 * multibyte string, that will be converted to wcs by mpdm_mbstowcs().
 * [Value Creation]
 */
/** mpdm_t MPDM_NMBS(char * mbs, int s); */
/* ; */

/**
 * MPDM_2MBS - Creates a multibyte string value from a wide char string.
 * @wcs: the wide char string
 *
 * Creates a multibyte string value from the @wcs wide char string,
 * converting it by mpdm_wcstombs(). Take note that multibyte string values
 * are not properly strings, so they cannot be used for string comparison
 * and such.
 * [Value Creation]
 */
/** mpdm_t MPDM_2MBS(wchar_t * wcs); */
/* ; */

/**
 * MPDM_X - Creates a new executable value.
 * @func: the C code function
 *
 * Creates a new executable value given a pointer to the @func C code function.
 * The function must receive an mpdm_t array value (that will hold their
 * arguments) and return another one.
 * [Value Creation]
 */
/** mpdm_t MPDM_X(mpdm_t (* func)(mpdm_t args)); */
/* ; */

/**
 * MPDM_C - Creates a new value with a copy of a buffer.
 * @flags: additional flags
 * @ptr: pointer to data
 * @size: data size
 *
 * Create a new value with a copy of a buffer. The value will store a copy
 * of @ptr and have the additional @flags.
 * [Value Creation]
 */
/** mpdm_t MPDM_C(int flags, void *ptr, int size); */
/* ; */

/**
 * MPDM_IS_ARRAY - Tests if a value is an array.
 * @v: the value
 *
 * Returns non-zero if @v is an array.
 */
/** int MPDM_IS_ARRAY(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_HASH - Tests if a value is a hash.
 * @v: the value
 *
 * Returns non-zero if @v is a hash.
 */
/** int MPDM_IS_HASH(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_EXEC - Tests if a value is executable.
 * @v: the value
 *
 * Returns non-zero if @v is executable.
 */
/** int MPDM_IS_EXEC(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_STRING - Tests if a value is a string.
 * @v: the value
 *
 * Returns non-zero if @v is a string.
 */
/** int MPDM_IS_STRING(mpdm_t v); */
/* ; */
