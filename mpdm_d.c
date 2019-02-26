/*

    MPDM - Minimum Profit Data Manager
    mpdm_d.c - Debugging utilities

    Angel Ortega <angel@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mpdm.h"

/** data **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t *ptr, int *size);
wchar_t *(*mpdm_dump_1) (const mpdm_t v, int l, wchar_t *ptr, int *size) = NULL;

/** code **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t *ptr, int *size)
/* dumps one value to the ptr dynamic string with 'l' indenting level */
{
    int n;
    wchar_t *wptr;

    mpdm_ref(v);

    /* indent */
    for (n = 0; n < l; n++)
        ptr = mpdm_pokews(ptr, size, L"  ");

    if (v != NULL) {
        char tmp[256];
        int s;
        int c = 0;
        mpdm_t w, i;
        wchar_t *str;

        /* stringify first */
        str = mpdm_string(v);

        /* add data type */
        ptr = mpdm_pokews(ptr, size, mpdm_type_wcs(v));

        sprintf(tmp, "(%d,%d):", v->ref - 1, (int) v->size);

        /* add refcount, size and flags */
        wptr = mpdm_mbstowcs(tmp, &s, -1);
        ptr = mpdm_pokews(ptr, size, wptr);
        free(wptr);

        /* add the visual representation of the value */
        ptr = mpdm_pokews(ptr, size, str);
        ptr = mpdm_pokews(ptr, size, L"\n");

        if (mpdm_type(v) == MPDM_TYPE_ARRAY) {
            while (mpdm_iterator(v, &c, &w, NULL)) {
                ptr = dump_1(w, l + 1, ptr, size);
            }
        }
        else
        if (mpdm_type(v) == MPDM_TYPE_OBJECT) {
            while (mpdm_iterator(v, &c, &w, &i)) {
                ptr = dump_1(i, l + 1, ptr, size);
                ptr = dump_1(w, l + 2, ptr, size);
            }
        }
    }
    else
        ptr = mpdm_pokews(ptr, size, L"[NULL]\n");

    mpdm_unrefnd(v);

    return ptr;
}


static wchar_t *do_dump(mpdm_t v, int *size)
{
    wchar_t *ptr;

    /* if no dumper plugin is defined, fall back to default */
    if (mpdm_dump_1 == NULL)
        mpdm_dump_1 = dump_1;

    *size = 0;
    ptr = mpdm_dump_1(v, 0, NULL, size);

    return ptr;
}


/**
 * mpdm_dumper - Returns a visual representation of a complex value.
 * @v: The value
 *
 * Returns a visual representation of a complex value.
 */
mpdm_t mpdm_dumper(const mpdm_t v)
{
    int size = 0;
    wchar_t *ptr;

    ptr = do_dump(v, &size);

    return MPDM_ENS(ptr, size);
}


/**
 * mpdm_dump - Dumps a value to stdin.
 * @v: The value
 *
 * Dumps a value to stdin. The value can be complex. This function
 * is for debugging purposes only.
 */
void mpdm_dump(const mpdm_t v)
{
    int size = 0;
    wchar_t *ptr;

    ptr = do_dump(v, &size);
    mpdm_write_wcs(stdout, ptr);
    free(ptr);
}
