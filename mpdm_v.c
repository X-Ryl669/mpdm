/*

    MPDM - Minimum Profit Data Manager
    mpdm_v.c - Basic value management

    Angel Ortega <angel@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "mpdm.h"


/** data **/

mpdm_t mpdm_global_root = NULL;

/* data type information */
struct {
    wchar_t *name;
    void (*destroy)(mpdm_t);
} mpdm_type_info[] = {
    { L"null",      mpdm_dummy__destroy },
    { L"string",    mpdm_dummy__destroy },
    { L"array",     mpdm_array__destroy },
    { L"object",    mpdm_object__destroy },
    { L"file",      mpdm_file__destroy },
    { L"mbs",       mpdm_dummy__destroy },
    { L"regex",     mpdm_regex__destroy },
    { L"mutex",     mpdm_mutex__destroy },
    { L"semaphore", mpdm_semaphore__destroy },
    { L"thread",    mpdm_thread__destroy },
    { L"function",  mpdm_function__destroy },
    { L"program",   mpdm_program__destroy },
    { L"integer",   mpdm_dummy__destroy },
    { L"real",      mpdm_dummy__destroy }
};


/** code **/

void mpdm_dummy__destroy(mpdm_t v)
{
    (void)v;
}


static mpdm_t destroy_value(mpdm_t v)
/* destroys a value */
{
    /* destroy type */
    mpdm_type_info[mpdm_type(v)].destroy(v);

    /* free data */
    free((void *)v->data);

    /* garble the memory block */
    memset(v, 0xaa, sizeof(*v));

    free(v);

    return NULL;
}


/**
 * mpdm_new - Creates a new value.
 * @type: data type
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @type is the data type, @data is a
 * pointer to the data the value will store and @size the size of these
 * data (if value is to be a multiple one, @size is a number of elements,
 * or a number of bytes otherwise).
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 * [Value Creation]
 */
mpdm_t mpdm_new(mpdm_type_t type, const void *data, int size)
{
    mpdm_t v;

    v = (mpdm_t) calloc(sizeof(*v), 1);

    v->type     = type;
    v->ref      = 0;
    v->data     = data;
    v->size     = size;

    return v;
}


mpdm_type_t mpdm_type(mpdm_t v)
{
    return v ? v->type : MPDM_TYPE_NULL;
}


wchar_t *mpdm_type_wcs(mpdm_t v)
{
    return mpdm_type_info[mpdm_type(v)].name;
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
int mpdm_size(const mpdm_t v)
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
    return mpdm_global_root = mpdm_global_root ? mpdm_global_root : mpdm_ref(MPDM_H(0));
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


mpdm_t mpdm_new_copy(mpdm_type_t type, void *ptr, int size)
{
    mpdm_t r = NULL;

    if (ptr != NULL) {
        char *ptr2 = malloc(size);
        memcpy(ptr2, ptr, size);

        r = mpdm_new(type, ptr2, size);
    }

    return r;
}


int mpdm_wrap_pointers(mpdm_t v, int offset, int *del)
{
    /* wrap from the end */
    if (offset < 0) {
        offset = mpdm_size(v) + offset;

        /* still negative? restart */
        if (offset < 0)
            offset = 0;
    }

    if (del) {
        /* negative values mean 'count from the end' */
        if (*del < 0)
            *del = mpdm_size(v) + 1 - offset + *del;

        /* trim if trying to delete too far */
        if (offset + *del > mpdm_size(v))
            *del = mpdm_size(v) - offset;
    }

    return offset;
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


extern char *mpdm_build_git_rev;
extern char *mpdm_build_timestamp;

/**
 * mpdm_startup - Initializes MPDM.
 *
 * Initializes the Minimum Profit Data Manager. Returns 0 if
 * everything went OK.
 */
int mpdm_startup(void)
{
    mpdm_t r, v;

    r = mpdm_root();

    /* sets the locale */
    if (setlocale(LC_ALL, "") == NULL)
        if (setlocale(LC_ALL, "C.UTF-8") == NULL)
            setlocale(LC_ALL, "C");

    mpdm_encoding(NULL);

    /* store the MPDM object */
    v = mpdm_set_wcs(r, MPDM_O(), L"MPDM");
    mpdm_set_wcs(v, MPDM_MBS(VERSION),              L"version");
    mpdm_set_wcs(v, MPDM_MBS(mpdm_build_git_rev),   L"build_git_rev");
    mpdm_set_wcs(v, MPDM_MBS(mpdm_build_timestamp), L"build_timestamp");

    /* store the ENV hash */
    mpdm_set_wcs(r, build_env(), L"ENV");

    /* store the special values TRUE and FALSE */
    mpdm_set_wcs(r, MPDM_I(1), L"TRUE");
    mpdm_set_wcs(r, MPDM_I(0), L"FALSE");

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
 * @type: data type
 * @ptr: pointer to data
 * @size: data size
 *
 * Create a new value with a copy of a buffer. The value will store a copy
 * of @ptr and have the specified @type.
 * [Value Creation]
 */
/** mpdm_t MPDM_C(mpdm_type_t type, void *ptr, int size); */
/* ; */
