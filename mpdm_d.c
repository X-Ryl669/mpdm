/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

    mpdm_d.c - Debugging utilities

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
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mpdm.h"

/** data **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t *ptr, size_t *size);
wchar_t *(*mpdm_dump_1) (const mpdm_t v, int l, wchar_t *ptr, size_t *size) = NULL;

/** code **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t *ptr, size_t *size)
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
        size_t s;
        int c = 0;
        mpdm_t w, i;
        wchar_t *str;

        /* stringify first */
        str = mpdm_string(v);

        /* add data type */
        ptr = mpdm_pokews(ptr, size, mpdm_type_infos[mpdm_type(v)].name);

        sprintf(tmp, "(%d,%d,%c%c%c%c):", v->ref - 1, (int) v->size,
            v->flags & MPDM_EXTENDED ? 'X' : '-',
            v->flags & MPDM_FREE     ? 'F' : '-',
            MPDM_HAS_IVAL(v)         ? 'I' : '-',
            MPDM_HAS_RVAL(v)         ? 'R' : '-'
        );

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


static wchar_t *do_dump(mpdm_t v, size_t *size)
{
    wchar_t *ptr;

    /* if no dumper plugin is defined, fall back to default */
    if (mpdm_dump_1 == NULL)
        mpdm_dump_1 = dump_1;

    *size = 0;
    ptr = mpdm_dump_1(v, 0, NULL, size);
    ptr = mpdm_pokewsn(ptr, size, L"", 1);

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
    size_t size = 0;
    wchar_t *ptr;

    ptr = do_dump(v, &size);

    return MPDM_ENS(ptr, size - 1);
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
    size_t size = 0;
    wchar_t *ptr;

    ptr = do_dump(v, &size);
    mpdm_write_wcs(stdout, ptr);
    free(ptr);
}
