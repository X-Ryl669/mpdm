/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2018 Angel Ortega <angel@triptico.com>

    mpdm_s.c - String management

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
#include <locale.h>
#include <wctype.h>
#include <time.h>

#ifdef CONFOPT_GETTEXT
#include <libintl.h>
#endif

#ifdef CONFOPT_WIN32
#include <windows.h>
#endif

#include "mpdm.h"


/** code **/

void *mpdm_poke_o(void *dst, int *dsize, int *offset, const void *org,
                  int osize, int esize)
{
    if (org != NULL && osize) {
        /* enough room? */
        if (*offset + osize > *dsize) {
            /* no; enlarge */
            *dsize += osize;

            dst = realloc(dst, *dsize * esize);
        }

        memcpy((char *) dst + (*offset * esize), org, osize * esize);
        *offset += osize;
    }

    return dst;
}


void *mpdm_poke(void *dst, int *dsize, const void *org, int osize,
                int esize)
/* pokes (adds) org into dst, which is a dynamic string, making it grow */
{
    int offset = *dsize;

    return mpdm_poke_o(dst, dsize, &offset, org, osize, esize);
}


wchar_t *mpdm_pokewsn(wchar_t * dst, int *dsize, const wchar_t * str,
                      int slen)
/* adds a wide string to dst using mpdm_poke() with size */
{
    if (str)
        dst = mpdm_poke(dst, dsize, str, slen, sizeof(wchar_t));

    return dst;
}


wchar_t *mpdm_pokews(wchar_t * dst, int *dsize, const wchar_t * str)
/* adds a wide string to dst using mpdm_poke() */
{
    if (str)
        dst = mpdm_pokewsn(dst, dsize, str, wcslen(str));

    return dst;
}


wchar_t *mpdm_pokev(wchar_t * dst, int *dsize, const mpdm_t v)
/* adds the string in v to dst using mpdm_poke() */
{
    if (v != NULL) {
        const wchar_t *ptr = mpdm_string(v);

        mpdm_ref(v);
        dst = mpdm_pokews(dst, dsize, ptr);
        mpdm_unref(v);
    }

    return dst;
}


wchar_t *mpdm_mbstowcs(const char *str, int *s, int l)
/* converts an mbs to a wcs, but filling invalid chars
   with question marks instead of just failing */
{
    wchar_t *ptr = NULL;
    char tmp[64];               /* really MB_CUR_MAX + 1 */
    wchar_t wc;
    int n, i, c, t = 0;
    char *cstr;

    /* allow NULL values for s */
    if (s == NULL)
        s = &t;

    /* if there is a limit, duplicate and break the string */
    if (l >= 0) {
        cstr = strdup(str);
        cstr[l] = '\0';
    }
    else
        cstr = (char *) str;

    /* try first a direct conversion with mbstowcs */
    if ((*s = mbstowcs(NULL, cstr, 0)) != -1) {
        /* direct conversion is possible; do it */
        if ((ptr = malloc((*s + 1) * sizeof(wchar_t))) != NULL) {
            mbstowcs(ptr, cstr, *s);
            ptr[*s] = L'\0';
        }
    }
    else {
        /* zero everything */
        *s = n = i = 0;

        for (;;) {
            /* no more characters to process? */
            if ((c = cstr[n + i]) == '\0' && i == 0)
                break;

            tmp[i++] = c;
            tmp[i] = '\0';

            /* try to convert */
            if (mbstowcs(&wc, tmp, 1) == (size_t) - 1) {
                /* can still be an incomplete multibyte char? */
                if (c != '\0' && i <= (int) MB_CUR_MAX)
                    continue;
                else {
                    /* too many failing bytes; skip 1 byte */
                    wc = L'?';
                    i = 1;
                }
            }

            /* skip used bytes and back again */
            n += i;
            i = 0;

            /* store new char */
            if ((ptr = mpdm_poke(ptr, s, &wc, 1, sizeof(wchar_t))) == NULL)
                break;
        }

        /* null terminate and count one less */
        if (ptr != NULL) {
            ptr = mpdm_poke(ptr, s, L"", 1, sizeof(wchar_t));
            (*s)--;
        }
    }

    /* free the duplicate */
    if (cstr != str)
        free(cstr);

    return ptr;
}


char *mpdm_wcstombs(const wchar_t * str, int *s)
/* converts a wcs to an mbs, but filling invalid chars
   with question marks instead of just failing */
{
    char *ptr = NULL;
    char tmp[64];               /* really MB_CUR_MAX + 1 */
    int l, t = 0;

    /* allow NULL values for s */
    if (s == NULL)
        s = &t;

    /* try first a direct conversion with wcstombs */
    if ((*s = wcstombs(NULL, str, 0)) != -1) {
        /* direct conversion is possible; do it and return */
        if ((ptr = malloc(*s + 1)) != NULL) {
            wcstombs(ptr, str, *s);
            ptr[*s] = '\0';
        }

        return ptr;
    }

    /* invalid encoding? convert characters one by one */
    *s = 0;

    while (*str) {
        if ((l = wctomb(tmp, *str)) <= 0) {
            /* if char couldn't be converted,
               write a question mark instead */
            l = wctomb(tmp, L'?');
        }

        tmp[l] = '\0';
        if ((ptr = mpdm_poke(ptr, s, tmp, l, 1)) == NULL)
            break;

        str++;
    }

    /* null terminate and count one less */
    if (ptr != NULL) {
        ptr = mpdm_poke(ptr, s, "", 1, 1);
        (*s)--;
    }

    return ptr;
}


mpdm_t mpdm_new_wcs(int flags, const wchar_t *str, int size, int cpy)
/* creates a new string value from a wcs */
{
    wchar_t *ptr;

    /* a size of -1 means 'calculate it' */
    if (size == -1 && str != NULL)
        size = wcslen(str);

    /* create a copy? */
    if (size >= 0 && cpy) {
        /* free() on destruction */
        flags |= MPDM_FREE;

        ptr = calloc(size + 1, sizeof(wchar_t));

        /* if there is a source, copy it */
        if (str != NULL)
            wcsncpy(ptr, str, size);
    }
    else
        ptr = (wchar_t *) str;

    return mpdm_new(flags | MPDM_STRING, ptr, size);
}


mpdm_t mpdm_new_mbstowcs(int flags, const char *str, int l)
/* creates a new string value from an mbs */
{
    mpdm_t v = NULL;
    wchar_t *ptr;
    int size;

    if ((ptr = mpdm_mbstowcs(str, &size, l)) != NULL)
        v = mpdm_new(flags | MPDM_STRING | MPDM_FREE, ptr, size);

    return v;
}


mpdm_t mpdm_new_wcstombs(int flags, const wchar_t *str)
/* creates a new mbs value from a wbs */
{
    char *ptr;
    int size;

    ptr = mpdm_wcstombs(str, &size);

    /* unset the string flag; mbs,s are not 'strings' */
    return mpdm_new((flags | MPDM_FREE) & ~MPDM_STRING, ptr, size);
}


mpdm_t mpdm_new_i(int ival)
/* creates a new string value from an integer */
{
    mpdm_ex_t ev;

    /* create a string value, but without the 'string' */
    ev = (mpdm_ex_t) mpdm_new(MPDM_STRING | MPDM_FREE | MPDM_IVAL | MPDM_EXTENDED, NULL, 0);
    ev->ival = ival;

    return (mpdm_t) ev;
}


mpdm_t mpdm_new_r(double rval)
/* creates a new string value from a real number */
{
    mpdm_ex_t ev;

    /* create a string value, but without the 'string' */
    ev = (mpdm_ex_t) mpdm_new(MPDM_STRING | MPDM_FREE | MPDM_RVAL | MPDM_EXTENDED, NULL, 0);
    ev->rval = rval;

    return (mpdm_t) ev;
}


/* interface */

/**
 * mpdm_string2 - Returns a printable representation of a value (with buffer).
 * @v: the value
 * @wtmp: the external buffer
 *
 * Returns a printable representation of a value. For strings, it's
 * the value data itself; for any other type, a conversion to string
 * is returned instead. If @v is not a string, the @wtmp buffer
 * can be used as a placeholder for the string representation.
 *
 * The reference count value in @v is not touched.
 * [Strings]
 */
wchar_t *mpdm_string2(const mpdm_t v, wchar_t *wtmp)
{
    char tmp[128] = "";
    wchar_t *ret = L"[UNKNOWN]";

    mpdm_ref(v);

    /* if it's NULL, return a constant */
    if (v == NULL)
        ret = L"[NULL]";
    else
    /* if it's a string, return it */
    if (v->flags & MPDM_STRING) {
        if (v->data == NULL) {
            /* string but no data? most probably a 'lazy' number */
            if (v->flags & MPDM_RVAL) {
                char *prev_locale = setlocale(LC_NUMERIC, "C");

                /* creates the visual representation */
                sprintf(tmp, "%lf", mpdm_rval(v));
            
                setlocale(LC_NUMERIC, prev_locale);

                /* manually strip useless zeroes */
                if (strchr(tmp, '.') != NULL) {
                    char *ptr;
            
                    for (ptr = tmp + strlen(tmp) - 1; *ptr == '0'; ptr--);
            
                    /* if it's over the ., strip it also */
                    if (*ptr != '.')
                        ptr++;
            
                    *ptr = '\0';
                }
            }
            else
            if (v->flags & MPDM_IVAL) {
                /* creates the visual representation */
                sprintf(tmp, "%d", mpdm_ival(v));
            }

            v->data = (void *)mpdm_mbstowcs(tmp, &v->size, -1);
        }

        ret = (wchar_t *) v->data;
    }
    else {
        /* otherwise, return a visual representation */
        sprintf(tmp, "%p", v);
        mbstowcs(wtmp, tmp, sizeof(tmp) * sizeof(wchar_t));

        ret = wtmp;
    }

    mpdm_unrefnd(v);

    return ret;
}


/**
 * mpdm_string - Returns a printable representation of a value.
 * @v: the value
 *
 * Returns a printable representation of a value. For strings, it's
 * the value data itself; for any other type, a conversion to string
 * is returned instead. This value should be used immediately, as it
 * can be a pointer to a static buffer.
 *
 * The reference count value in @v is not touched.
 * [Strings]
 */
wchar_t *mpdm_string(const mpdm_t v)
{
    static wchar_t tmp[32];

    return mpdm_string2(v, tmp);
}


/**
 * mpdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the MPDM_STRING flag set,
 * a comparison using wcscoll() is returned; if both are arrays,
 * the size is compared first and, if they have the same number
 * elements, each one is compared; otherwise, a simple visual
 * representation comparison is done.
 * [Strings]
 */
int mpdm_cmp(const mpdm_t v1, const mpdm_t v2)
{
    int r;

    mpdm_ref(v1);
    mpdm_ref(v2);

    /* same values? */
    if (v1 == v2)
        r = 0;
    else
        /* is any value NULL? */
    if (v1 == NULL)
        r = -1;
    else
    if (v2 == NULL)
        r = 1;
    else
    if (MPDM_IS_ARRAY(v1) && MPDM_IS_ARRAY(v2)) {
        /* compare first the sizes */
        if ((r = mpdm_size(v1) - mpdm_size(v2)) == 0) {
            int n;

            /* they have the same size;
               compare each pair of elements */
            for (n = 0; n < mpdm_size(v1); n++) {
                if ((r = mpdm_cmp(mpdm_aget(v1, n),
                                  mpdm_aget(v2, n))) != 0)
                    break;
            }
        }
    }
    else {
        wchar_t tmp[32];

        r = mpdm_cmp_s(v1, mpdm_string2(v2, tmp));
    }

    mpdm_unref(v2);
    mpdm_unref(v1);

    return r;
}


/**
 * mpdm_cmp_s - Compares two values (string version).
 * @v1: the first value
 * @v2: the second value
 *
 * Compares the @v2 string against the stringified view of @v1.
 */
int mpdm_cmp_s(const mpdm_t v1, const wchar_t *v2)
{
    wchar_t tmp[32];
    int r;

    mpdm_ref(v1);
    r = wcscoll(mpdm_string2(v1, tmp), v2);
    mpdm_unref(v1);

    return r;
}


/**
 * mpdm_splice - Creates a new string value from another.
 * @v: the original value
 * @i: the value to be inserted
 * @offset: offset where the substring is to be inserted
 * @del: number of characters to delete
 *
 * Creates a new string value from @v, deleting @del chars at @offset
 * and substituting them by @i. If @del is 0, no deletion is done.
 * both @offset and @del can be negative; if this is the case, it's
 * assumed as counting from the end of @v. If @v is NULL, @i will become
 * the new string, and both @offset and @del will be ignored. If @v is
 * not NULL and @i is, no insertion process is done (only deletion, if
 * applicable).
 *
 * Returns a two element array, with the new string in the first
 * element and the deleted string in the second (with a NULL value
 * if @del is 0).
 * [Strings]
 */
mpdm_t mpdm_splice(const mpdm_t v, const mpdm_t i, int offset, int del)
{
    mpdm_t w;
    mpdm_t n = NULL;
    mpdm_t d = NULL;
    int os, ns, r;
    int ins = 0;
    wchar_t *ptr;

    mpdm_ref(v);
    mpdm_ref(i);

    if (v != NULL) {
        os = mpdm_size(v);

        /* negative offsets start from the end */
        if (offset < 0)
            offset = os + 1 - offset;

        /* never add further the end */
        if (offset > os)
            offset = os;

        /* negative del counts as 'characters left' */
        if (del < 0)
            del = os + 1 - offset + del;

        /* something to delete? */
        if (del > 0) {
            /* never delete further the end */
            if (offset + del > os)
                del = os - offset;

            /* deleted string */
            d = MPDM_NS(((wchar_t *) v->data) + offset, del);
        }
        else
            del = 0;

        /* something to insert? */
        ins = mpdm_size(i);

        /* new size and remainder */
        ns = os + ins - del;
        r = offset + del;

        n = MPDM_NS(NULL, ns);

        ptr = (wchar_t *) n->data;

        /* copy the beginning */
        if (offset > 0) {
            wcsncpy(ptr, v->data, offset);
            ptr += offset;
        }

        /* copy the text to be inserted */
        if (ins > 0) {
            wcsncpy(ptr, i->data, ins);
            ptr += ins;
        }

        /* copy the remaining */
        os -= r;
        if (os > 0) {
            wcsncpy(ptr, ((wchar_t *) v->data) + r, os);
            ptr += os;
        }

        /* null terminate */
        *ptr = L'\0';
    }
    else
        n = i;

    /* creates the output array */
    w = MPDM_A(2);

    mpdm_ref(w);
    mpdm_aset(w, n, 0);
    mpdm_aset(w, d, 1);
    mpdm_unrefnd(w);

    mpdm_unref(i);
    mpdm_unref(v);

    return w;
}


/**
 * mpdm_slice - Returns a slice of a string.
 * @v: the string
 * @offset: offset
 * @num: number of characters
 *
 * Returns the substring of @v that starts from @offset
 * and have @num characters. A negative @offset is also valid,
 * with -1 as the last character and counting down to the start
 * of the string.
 * [Strings]
 */
mpdm_t mpdm_slice(const mpdm_t s, int offset, int num)
{
    mpdm_t r = NULL;

    mpdm_ref(s);

    if (s != NULL) {
        int os = mpdm_size(s);
        wchar_t *ptr = mpdm_string(s);

        if (offset < 0) {
            if ((offset = os + offset) < 0)
                offset = 0;
        }

        if (offset + num > os)
            num = os - offset;

        r = MPDM_NS(ptr + offset, num);
    }

    mpdm_unref(s);

    return r;
}


/**
 * mpdm_strcat_sn - Concatenates two strings (string with size version).
 * @s1: the first string
 * @s2: the second string
 * @size: the size of the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 * [Strings]
 */
mpdm_t mpdm_strcat_sn(const mpdm_t s1, const wchar_t * s2, int size)
{
    mpdm_t r = NULL;

    if (s1 != NULL || s2 != NULL) {
        wchar_t *ptr = NULL;
        int s = 0;

        ptr = mpdm_pokev(ptr, &s, s1);
        ptr = mpdm_pokewsn(ptr, &s, s2, size);

        ptr = mpdm_poke(ptr, &s, L"", 1, sizeof(wchar_t));
        r = MPDM_ENS(ptr, s - 1);
    }

    return r;
}


/**
 * mpdm_strcat_s - Concatenates two strings (string version).
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 * [Strings]
 */
mpdm_t mpdm_strcat_s(const mpdm_t s1, const wchar_t * s2)
{
    return mpdm_strcat_sn(s1, s2, s2 ? wcslen(s2) : 0);
}


/**
 * mpdm_strcat - Concatenates two strings.
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 * [Strings]
 */
mpdm_t mpdm_strcat(const mpdm_t s1, const mpdm_t s2)
{
    mpdm_t r;

    mpdm_ref(s2);
    r = mpdm_strcat_s(s1, s2 ? mpdm_string(s2) : NULL);
    mpdm_unref(s2);

    return r;
}


int mpdm_ival_mbs(char *str)
/* converts str to integer */
{
    int i = 0;
    char *fmt = "%i";

    /* workaround for mingw32: as it doesn't
       correctly parse octal and hexadecimal
       numbers, they are tried as special cases */
    if (str[0] == '0') {
        if (str[1] == 'b' || str[1] == 'B') {
            /* binary number */
            fmt = NULL;
            char *ptr = &str[2];

            while (*ptr == '0' || *ptr == '1') {
                i <<= 1;

                if (*ptr == '1')
                    i |= 1;

                ptr++;
            }
        }
        else
        if (str[1] == 'x' || str[1] == 'X')
            fmt = "%x";
        else
            fmt = "%o";
    }

    if (fmt != NULL)
        sscanf(str, fmt, &i);

    return i;
}


/**
 * mpdm_ival - Returns a value's data as an integer.
 * @v: the value
 *
 * Returns a value's data as an integer. If the value is a string,
 * it's converted via sscanf and returned; non-string values have all
 * an ival of 0. The converted integer is cached, so costly string
 * conversions are only done once. Values created with the MPDM_IVAL
 * flag set have its ival cached from the beginning.
 * [Strings]
 * [Value Management]
 */
int mpdm_ival(mpdm_t v)
{
    int i = 0;

    if (v != NULL) {
        mpdm_ref(v);

        /* does it have a cached value? */
        if ((v->flags & MPDM_IVAL)) {
            mpdm_ex_t ev = (mpdm_ex_t) v;

            i = ev->ival;
        }
        else
        /* does it have a real cached value? */
        if ((v->flags & MPDM_RVAL)) {
            i = (int) mpdm_rval(v);
        }
        else
        /* it's a string: calculate */
        if ((v->flags & MPDM_STRING)) {
            char tmp[32];

            wcstombs(tmp, (wchar_t *) v->data, sizeof(tmp));
            tmp[sizeof(tmp) - 1] = '\0';

            i = mpdm_ival_mbs(tmp);
        }

        /* cache it if possible */
        if ((v->flags & MPDM_EXTENDED)) {
            mpdm_ex_t ev = (mpdm_ex_t) v;

            ev->flags   |= MPDM_IVAL;
            ev->ival    = i;
        }

        mpdm_unref(v);
    }

    return i;
}


double mpdm_rval_mbs(char *str)
{
    double r = 0.0;
    char *prev_locale;

    /* set locale to C for non locale-dependent
       floating point conversion */
    prev_locale = setlocale(LC_NUMERIC, "C");

    /* read */
    sscanf(str, "%lf", &r);

    /* set previous locale */
    setlocale(LC_NUMERIC, prev_locale);

    return r;
}


/**
 * mpdm_rval - Returns a value's data as a real number (double).
 * @v: the value
 *
 * Returns a value's data as a real number (double float). If the value
 * is a string, it's converted via sscanf and returned; non-string values
 * have all an rval of 0. The converted double is cached, so costly string
 * conversions are only done once. Values created with the MPDM_RVAL
 * flag set have its rval cached from the beginning.
 * [Strings]
 * [Value Management]
 */
double mpdm_rval(mpdm_t v)
{
    double r = 0.0;

    if (v != NULL) {
        mpdm_ref(v);

        /* does it have a cached value? */
        if ((v->flags & MPDM_RVAL)) {
            mpdm_ex_t ev = (mpdm_ex_t) v;

            r = ev->rval;
        }
        else
        /* does it have an integer cached value? */
        if ((v->flags & MPDM_IVAL)) {
            r = (double) mpdm_ival(v);
        }
        else
        /* it's a string: calculate */
        if ((v->flags & MPDM_STRING)) {
            char tmp[32];

            wcstombs(tmp, (wchar_t *) v->data, sizeof(tmp));
            tmp[sizeof(tmp) - 1] = '\0';

            r = mpdm_rval_mbs(tmp);
        }

        /* cache it if possible */
        if ((v->flags & MPDM_EXTENDED)) {
            mpdm_ex_t ev = (mpdm_ex_t) v;

            ev->flags   |= MPDM_RVAL;
            ev->rval    = r;
        }

        mpdm_unref(v);
    }

    return r;
}


/**
 * mpdm_gettext - Translates a string to the current language.
 * @str: the string
 *
 * Translates the @str string to the current language.
 *
 * This function can still be used even if there is no real gettext
 * support() by manually filling the __I18N__ hash.
 *
 * If the string is found in the current table, the translation is
 * returned; otherwise, the same @str value is returned.
 * [Strings]
 * [Localization]
 */
mpdm_t mpdm_gettext(const mpdm_t str)
{
    mpdm_t v;
    mpdm_t i18n = NULL;

    /* gets the cache */
    if ((i18n = mpdm_hget_s(mpdm_root(), L"__I18N__")) == NULL)
        i18n = mpdm_hset_s(mpdm_root(), L"__I18N__", MPDM_H(0));

    mpdm_ref(str);

    /* try first the cache */
    if ((v = mpdm_hget(i18n, str)) == NULL) {
#ifdef CONFOPT_GETTEXT
        char *s;
        mpdm_t t;

        /* convert to mbs */
        t = mpdm_ref(MPDM_2MBS(str->data));

        /* ask gettext for it */
        s = gettext((char *) t->data);

        if (s != t->data)
            v = MPDM_MBS(s);
        else
            v = str;

        mpdm_unref(t);

#else                           /* CONFOPT_GETTEXT */

        v = str;

#endif                          /* CONFOPT_GETTEXT */

        /* store in the cache */
        mpdm_hset(i18n, str, v);
    }

    mpdm_unref(str);

    return v;
}


/**
 * mpdm_gettext_domain - Sets domain and data directory for translations.
 * @dom: the domain (application name)
 * @data: directory contaning the .mo files
 *
 * Sets the domain (application name) and translation data for translating
 * strings that will be returned by mpdm_gettext().@data must point to a
 * directory containing the .mo (compiled .po) files.
 *
 * If there is no gettext support, returns 0, or 1 otherwise.
 * [Strings]
 * [Localization]
 */
int mpdm_gettext_domain(const mpdm_t dom, const mpdm_t data)
{
    int ret = 0;

    mpdm_ref(dom);
    mpdm_ref(data);

#ifdef CONFOPT_GETTEXT

    mpdm_t dm;
    mpdm_t dt;

    /* convert both to mbs,s */
    dm = mpdm_ref(MPDM_2MBS(dom->data));
    dt = mpdm_ref(MPDM_2MBS(data->data));

    /* bind and set domain */
    bindtextdomain((char *) dm->data, (char *) dt->data);
    textdomain((char *) dm->data);

    mpdm_hset_s(mpdm_root(), L"__I18N__", MPDM_H(0));

    mpdm_unref(dt);
    mpdm_unref(dm);

    ret = 1;

#endif                          /* CONFOPT_GETTEXT */

#ifdef CONFOPT_WIN32

    mpdm_t v;

    if ((v = mpdm_hget_s(mpdm_root(), L"ENV")) != NULL &&
        mpdm_hget_s(v, L"LANG") == NULL) {
        wchar_t *wptr = L"en";

        /* MS Windows crappy language constants... */

        switch ((GetSystemDefaultLangID() & 0x00ff)) {
        case 0x01:
            wptr = L"ar";
            break;              /* arabic */
        case 0x02:
            wptr = L"bg";
            break;              /* bulgarian */
        case 0x03:
            wptr = L"ca";
            break;              /* catalan */
        case 0x04:
            wptr = L"zh";
            break;              /* chinese */
        case 0x05:
            wptr = L"cz";
            break;              /* czech */
        case 0x06:
            wptr = L"da";
            break;              /* danish */
        case 0x07:
            wptr = L"de";
            break;              /* german */
        case 0x08:
            wptr = L"el";
            break;              /* greek */
        case 0x09:
            wptr = L"en";
            break;              /* english */
        case 0x0a:
            wptr = L"es";
            break;              /* spanish */
        case 0x0b:
            wptr = L"fi";
            break;              /* finnish */
        case 0x0c:
            wptr = L"fr";
            break;              /* french */
        case 0x0d:
            wptr = L"he";
            break;              /* hebrew */
        case 0x0e:
            wptr = L"hu";
            break;              /* hungarian */
        case 0x0f:
            wptr = L"is";
            break;              /* icelandic */
        case 0x10:
            wptr = L"it";
            break;              /* italian */
        case 0x11:
            wptr = L"jp";
            break;              /* japanese */
        case 0x12:
            wptr = L"ko";
            break;              /* korean */
        case 0x13:
            wptr = L"nl";
            break;              /* dutch */
        case 0x14:
            wptr = L"no";
            break;              /* norwegian */
        case 0x15:
            wptr = L"po";
            break;              /* polish */
        case 0x16:
            wptr = L"pt";
            break;              /* portuguese */
        case 0x17:
            wptr = L"rm";
            break;              /* romansh (switzerland) */
        case 0x18:
            wptr = L"ro";
            break;              /* romanian */
        case 0x19:
            wptr = L"ru";
            break;              /* russian */
        case 0x1a:
            wptr = L"sr";
            break;              /* serbian */
        case 0x1b:
            wptr = L"sk";
            break;              /* slovak */
        case 0x1c:
            wptr = L"sq";
            break;              /* albanian */
        case 0x1d:
            wptr = L"sv";
            break;              /* swedish */
        }

        mpdm_hset_s(v, L"LANG", MPDM_S(wptr));
    }

#endif                          /* CONFOPT_WIN32 */

    mpdm_unref(data);
    mpdm_unref(dom);

    return ret;
}


#ifdef CONFOPT_WCWIDTH

int wcwidth(wchar_t);

int mpdm_wcwidth(wchar_t c)
{
    return wcwidth(c);
}

#else                           /* CONFOPT_WCWIDTH */

#include "wcwidth.c"

int mpdm_wcwidth(wchar_t c)
{
    return mk_wcwidth(c);
}

#endif                          /* CONFOPT_WCWIDTH */

static wchar_t *s_mbstowcs(char *mbs, wchar_t *wcs)
{
    int n;

    for (n = 0; mbs[n] != '\0'; n++)
        wcs[n] = (wchar_t) mbs[n];

    return wcs;
}


static wchar_t *json_s(wchar_t *o, int *l, mpdm_t v)
{
    wchar_t *p = mpdm_string(v);

    while (*p) {
        if (*p == L'\n')
            o = mpdm_poke(o, l, L"\\n", 2, sizeof(wchar_t));
        else
        if (*p == L'\\')
            o = mpdm_poke(o, l, L"\\\\", 2, sizeof(wchar_t));
        else
        if (*p < 32) {
            char tmp[7];
            wchar_t wtmp[7];

            sprintf(tmp, "\\u%04x", (unsigned int) *p);
            o = mpdm_poke(o, l, s_mbstowcs(tmp, wtmp), 6, sizeof(wchar_t));
        }
        else
            o = mpdm_poke(o, l, p, 1, sizeof(wchar_t));

        p++;
    }

    return o;
}


static wchar_t *json_f(wchar_t *o, int *l, mpdm_t v)
/* fills a %j JSON format */
{
    int n = 0, c = 0;
    mpdm_t k, w;

    if (MPDM_IS_HASH(v)) {
        o = mpdm_poke(o, l, L"{", 1, sizeof(wchar_t));

        while (mpdm_iterator(v, &n, &k, &w)) {
            if (c)
                o = mpdm_poke(o, l, L",", 1, sizeof(wchar_t));

            o = mpdm_poke(o, l, L"\"", 1, sizeof(wchar_t));
            o = json_s(o, l, k);
            o = mpdm_poke(o, l, L"\":", 2, sizeof(wchar_t));

            if (w == NULL)
                o = mpdm_poke(o, l, L"null", 4, sizeof(wchar_t));
            else
            if (MPDM_IS_ARRAY(w) || MPDM_IS_HASH(w))
                o = json_f(o, l, w);
            else {
                if (w->flags & MPDM_IVAL || w->flags & MPDM_RVAL)
                    o = mpdm_pokev(o, l, w);
                else {
                    o = mpdm_poke(o, l, "\"", 1, sizeof(wchar_t));
                    o = json_s(o, l, w);
                    o = mpdm_poke(o, l, "\"", 1, sizeof(wchar_t));
                }
            }

            c++;
        }

        o = mpdm_poke(o, l, L"}", 1, sizeof(wchar_t));
    }
    else
    if (MPDM_IS_ARRAY(v)) {
        o = mpdm_poke(o, l, L"[", 1, sizeof(wchar_t));

        while (mpdm_iterator(v, &n, &k, &w)) {
            if (c)
                o = mpdm_poke(o, l, L",", 1, sizeof(wchar_t));

            if (w == NULL)
                o = mpdm_poke(o, l, L"NULL", 4, sizeof(wchar_t));
            else
            if (MPDM_IS_ARRAY(w) || MPDM_IS_HASH(w))
                o = json_f(o, l, w);
            else {
                if (w->flags & MPDM_IVAL || w->flags & MPDM_RVAL)
                    o = mpdm_pokev(o, l, w);
                else {
                    o = mpdm_poke(o, l, "\"", 1, sizeof(wchar_t));
                    o = json_s(o, l, w);
                    o = mpdm_poke(o, l, "\"", 1, sizeof(wchar_t));
                }
            }

            c++;
        }

        o = mpdm_poke(o, l, L"]", 1, sizeof(wchar_t));
    }

    return o;
}


mpdm_t mpdm_fmt(const mpdm_t fmt, const mpdm_t arg)
{
    const wchar_t *i = fmt->data;
    wchar_t c, *o = NULL;
    int l = 0, n = 0;

    mpdm_ref(fmt);
    mpdm_ref(arg);

    /* find first mark */
    while ((c = i[n]) != L'\0' && c != L'%')
        n++;

    o = mpdm_poke(o, &l, i, n, sizeof(wchar_t));
    i = &i[n];

    /* format directive */
    if (c == L'%') {
        char t_fmt[128];
        char tmp[1024];
        char *ptr = NULL;
        wchar_t *wptr = NULL;
        int m = 0;

        /* transfer the % */
        t_fmt[m++] = '%';
        i++;

        /* transform the format to mbs */
        while (*i != L'\0' &&
               m < (int) (sizeof(t_fmt) - MB_CUR_MAX - 1) &&
               wcschr(L"-.0123456789", *i) != NULL)
            m += wctomb(&t_fmt[m], *i++);

        /* transfer the directive */
        m += wctomb(&t_fmt[m], *i++);

        t_fmt[m] = '\0';

        /* by default, copies the format */
        strcpy(tmp, t_fmt);

        switch (t_fmt[m - 1]) {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':

            /* integer value */
            snprintf(tmp, sizeof(tmp) - 1, t_fmt, mpdm_ival(arg));
            wptr = mpdm_mbstowcs(tmp, &m, -1);
            break;

        case 'f':

            /* float (real) value */
            snprintf(tmp, sizeof(tmp) - 1, t_fmt, mpdm_rval(arg));
            wptr = mpdm_mbstowcs(tmp, &m, -1);
            break;

        case 's':

            /* string value */
            ptr = mpdm_wcstombs(mpdm_string(arg), NULL);
            snprintf(tmp, sizeof(tmp) - 1, t_fmt, ptr);
            free(ptr);
            wptr = mpdm_mbstowcs(tmp, &m, -1);
            break;

        case 'b':

            ptr = tmp;
            unsigned int mask;
            int p = 0;

            mask = 1 << ((sizeof(int) * 8) - 1);
            while (mask) {
                if (mask & (unsigned int) mpdm_ival(arg)) {
                    *ptr++ = '1';
                    p = 1;
                }
                else
                if (p)
                    *ptr++ = '0';

                mask >>= 1;
            }

            if (ptr == tmp)
                *ptr++ = '0';

            *ptr = '\0';
            wptr = mpdm_mbstowcs(tmp, &m, -1);
            break;

        case 'j':
            o = json_f(o, &l, arg);
            break;

        case 't':
            /* time: brace-enclosed strftime mask */
            if (*i == L'{') {
                char tmp2[4096];
                int j = 0;
                struct tm *tm;
                time_t t = mpdm_ival(arg);

                i++;
                while (*i != L'\0' && *i != L'}')
                    wctomb(&tmp2[j++], *i++);
                tmp2[j] = '\0';
                if (*i)
                    i++;

                tm = localtime(&t);
                strftime(tmp, sizeof(tmp), tmp2, tm);
                wptr = mpdm_mbstowcs(tmp, &m, -1);
            }
            break;

        case 'c':

            /* char */
            c = mpdm_ival(arg);
            /* fallthrough ... */

        case '%':

            /* percent sign */
            o = mpdm_poke(o, &l, &c, 1, sizeof(wchar_t));
            break;
        }

        /* transfer */
        if (wptr != NULL) {
            o = mpdm_poke(o, &l, wptr, m, sizeof(wchar_t));
            free(wptr);
        }
    }

    /* fill the rest up to the end */
    n = 0;
    while (i[n] != L'\0')
        n++;

    o = mpdm_poke(o, &l, i, n, sizeof(wchar_t));

    /* null-terminate */
    o = mpdm_poke(o, &l, L"", 1, sizeof(wchar_t));

    mpdm_unref(arg);
    mpdm_unref(fmt);

    return MPDM_ENS(o, l - 1);
}


/**
 * mpdm_sprintf - Formats a sprintf()-like string.
 * @fmt: the string format
 * @args: an array of values
 *
 * Formats a string using the sprintf() format taking the values from @args.
 * [Strings]
 */
mpdm_t mpdm_sprintf(const mpdm_t fmt, const mpdm_t args)
{
    int n;
    mpdm_t v;

    mpdm_ref(args);

    v = fmt;
    for (n = 0; n < mpdm_size(args); n++)
        v = mpdm_fmt(v, mpdm_aget(args, n));

    mpdm_unref(args);

    return v;
}


/**
 * mpdm_ulc - Converts a string to uppercase or lowecase.
 * @s: the string
 * @u: convert to uppercase (1) or to lowercase (0).
 *
 * Converts @s to uppercase (for @u == 1) or to lowercase (@u == 0).
 * [Strings]
 */
mpdm_t mpdm_ulc(const mpdm_t s, int u)
{
    mpdm_t r = NULL;
    wchar_t *optr;
    int i;

    mpdm_ref(s);

    i = mpdm_size(s);

    if ((optr = malloc((i + 1) * sizeof(wchar_t))) != NULL) {
        wchar_t *iptr = mpdm_string(s);
        int n;

        for (n = 0; n < i; n++)
            optr[n] = u ? towupper(iptr[n]) : towlower(iptr[n]);

        optr[n] = L'\0';
        r = MPDM_ENS(optr, i);
    }

    mpdm_unref(s);

    return r;
}


enum {
    JS_ERROR = -1,
    JS_OCURLY,
    JS_OBRACK,
    JS_CCURLY,
    JS_CBRACK,
    JS_COMMA,
    JS_COLON,
    JS_VALUE,
    JS_STRING,
    JS_SSTRING,
    JS_NUMBER,
    JS_NULL,
    JS_ARRAY,
    JS_OBJECT
};

static wchar_t *json_lexer(wchar_t *s, int *t, mpdm_t *pv)
{
    wchar_t c;
    wchar_t *ptr = NULL;
    int size = 0;
    mpdm_t v = NULL;

    /* skip blanks */
    while (*s == L' '  || *s == L'\t' ||
           *s == L'\n' || *s == L'\r')
        s++;

    c = *s++;

    if (c == L'{')
        *t = JS_OCURLY;
    else
    if (c == L'}')
        *t = JS_CCURLY;
    else
    if (c == L'[')
        *t = JS_OBRACK;
    else
    if (c == L']')
        *t = JS_CBRACK;
    else
    if (c == L',')
        *t = JS_COMMA;
    else
    if (c == L':')
        *t = JS_COLON;
    else
    if (c == L'"') {
        *t = JS_STRING;

        while ((c = *s) != L'"' && c != L'\0') {
            char tmp[5];
            int i;

            if (c == '\\') {
                s++;
                c = *s;
                switch (c) {
                case 'n': c = L'\n'; break;
                case 'r': c = L'\r'; break;
                case 't': c = L'\t'; break;
                case 'u': /* hex char */
                    s++;
                    tmp[0] = (char)*s; s++;
                    tmp[1] = (char)*s; s++;
                    tmp[2] = (char)*s; s++;
                    tmp[3] = (char)*s;
                    tmp[4] = '\0';

                    sscanf(tmp, "%04x", &i);
                    c = (wchar_t) i;
                    break;
                }
            }

            ptr = mpdm_pokewsn(ptr, &size, &c, 1);
            s++;
        }

        if (c != L'\0')
            s++;

        ptr = mpdm_pokewsn(ptr, &size, L"", 1);
        v = MPDM_ENS(ptr, size);
    }
    else
    if ((c >= L'0' && c <= L'9') || c == L'.') {
        *t = JS_NUMBER;

        ptr = mpdm_pokewsn(ptr, &size, &c, 1);

        while (((c = *s) >= L'0' && c <= L'9') || c == L'.') {
            ptr = mpdm_pokewsn(ptr, &size, &c, 1);
            s++;
        }

        ptr = mpdm_pokewsn(ptr, &size, L"", 1);
        v = MPDM_ENS(ptr, size);
    }
    else
    if (c == 't' && wcsncmp(s, L"rue", 3) == 0) {
        s += 3;
        *t = JS_NUMBER;
        v = MPDM_I(1);
    }
    else
    if (c == 'f' && wcsncmp(s, L"alse", 4) == 0) {
        s += 4;
        *t = JS_NUMBER;
        v = MPDM_I(0);
    }
    else
    if (c == 'n' && wcsncmp(s, L"ull", 3) == 0) {
        s += 3;
        *t = JS_NULL;
    }
    else
        *t = JS_ERROR;

    *pv = v;

    return s;
}

wchar_t *json_parser(wchar_t *s, int *t, mpdm_t *pv)
{
    mpdm_t v = NULL;

    s = json_lexer(s, t, &v);

    if (*t == JS_OCURLY) {
        v = mpdm_ref(MPDM_H(0));

        for (;;) {
            wchar_t *os;
            mpdm_t k, w;

            s = json_lexer(s, t, &k);

            if (*t != JS_STRING) {
                *t = JS_ERROR;
                break;
            }

            s = json_lexer(s, t, &w);

            if (*t != JS_COLON) {
                *t = JS_ERROR;
                break;
            }

            os = s;
            s = json_lexer(os, t, &w);

            if (*t == JS_OCURLY || *t == JS_OBRACK)
                s = json_parser(os, t, &w);

            if (*t > JS_VALUE)
                mpdm_hset(v, k, w);
            else {
                *t = JS_ERROR;
                break;
            }

            s = json_lexer(s, t, &w);

            if (*t == JS_CCURLY) {
                *t = JS_OBJECT;
                break;
            }
            else
            if (*t != JS_COMMA) {
                *t = JS_ERROR;
                break;
            }
        }

        mpdm_unrefnd(v);
    }
    else
    if (*t == JS_OBRACK) {
        v = mpdm_ref(MPDM_A(0));

        for (;;) {
            mpdm_t w;
            wchar_t *os = s;

            s = json_lexer(os, t, &w);

            if (*t == JS_OCURLY || *t == JS_OBRACK)
                s = json_parser(os, t, &w);

            if (*t > JS_VALUE)
                mpdm_push(v, w);
            else {
                *t = JS_ERROR;
                break;
            }

            s = json_lexer(s, t, &w);

            if (*t == JS_CBRACK) {
                *t = JS_ARRAY;
                break;
            }
            else
            if (*t != JS_COMMA) {
                *t = JS_ERROR;
                break;
            }
        }

        mpdm_unrefnd(v);
    }
    else
        *t = JS_ERROR;

    *pv = v;

    return s;
}


/* scanf working buffers */
#define SCANF_BUF_SIZE 1024
static wchar_t scanf_yset[SCANF_BUF_SIZE];
static wchar_t scanf_nset[SCANF_BUF_SIZE];
static wchar_t scanf_mark[SCANF_BUF_SIZE];

struct {
    wchar_t cmd;
    wchar_t *yset;
    wchar_t *nset;
} scanf_sets[] = {
    { L's',  L"",                         L" \t"},
    { L'u',  L"0123456789",               L""},
    { L'd',  L"-0123456789",              L""},
    { L'i',  L"-0123456789",              L""},
    { L'f',  L"-0123456789.",             L""},
    { L'x',  L"-0123456789xabcdefABCDEF", L""},
    { L'\0', NULL,                        NULL},
};

char *strptime(const char *s, const char *format, struct tm *tm);


/**
 * mpdm_sscanf - Extracts data like sscanf().
 * @str: the string to be parsed
 * @fmt: the string format
 * @offset: the character offset to start scanning
 *
 * Extracts data from a string using a special format pattern, very
 * much like the scanf() series of functions in the C library. Apart
 * from the standard percent-sign-commands (s, u, d, i, f, x,
 * n, [, with optional size and * to ignore), it implements S,
 * to match a string of characters upto what follows in the format
 * string. Also, the [ set of characters can include other % formats.
 *
 * Returns an array with the extracted values. If %n is used, the
 * position in the scanned string is returned as the value.
 * [Strings]
 */
mpdm_t mpdm_sscanf(const mpdm_t str, const mpdm_t fmt, int offset)
{
    wchar_t *i = (wchar_t *) str->data;
    wchar_t *f = (wchar_t *) fmt->data;
    mpdm_t r;

    mpdm_ref(fmt);
    mpdm_ref(str);

    i += offset;
    r = MPDM_A(0);
    mpdm_ref(r);

    while (*f) {
        if (*f == L'%') {
            wchar_t *ptr = NULL;
            int size = 0;
            wchar_t cmd;
            int vsize = 0;
            int ignore = 0;
            int msize = 0;

            /* empty all buffers */
            scanf_yset[0] = scanf_nset[0] = scanf_mark[0] = L'\0';

            f++;

            /* an asterisk? don't return next value */
            if (*f == L'*') {
                ignore = 1;
                f++;
            }

            /* does it have a size? */
            while (wcschr(L"0123456789", *f)) {
                vsize *= 10;
                vsize += *f - L'0';
                f++;
            }

            /* if no size, set it to an arbitrary big limit */
            if (!vsize)
                vsize = 0xfffffff;

            /* now *f should contain a command */
            cmd = *f;
            f++;

            /* is it a verbatim percent sign? */
            if (cmd == L'%') {
                vsize = 1;
                ignore = 1;
                wcscpy(scanf_yset, L"%");
            }
            else
                /* a position? */
            if (cmd == L'n') {
                vsize = 0;
                ignore = 1;
                mpdm_push(r, MPDM_I(i - (wchar_t *) str->data));
            }
            else
                /* string upto a mark */
            if (cmd == L'S') {
                wchar_t *tmp = f;

                /* fill the mark upto another command */
                while (*tmp) {
                    if (*tmp == L'%') {
                        tmp++;

                        /* is it an 'n'? ignore and go on */
                        if (*tmp == L'n') {
                            tmp++;
                            continue;
                        }
                        else
                        if (*tmp == L'%')
                            scanf_mark[msize++] = *tmp;
                        else
                            break;
                    }
                    else
                        scanf_mark[msize++] = *tmp;

                    tmp++;
                }

                scanf_mark[msize] = L'\0';
            }
            else
                /* raw set */
            if (cmd == L'[') {
                int n = 0;
                wchar_t *set = scanf_yset;

                /* is it an inverse set? */
                if (*f == L'^') {
                    set = scanf_nset;
                    f++;
                }

                /* first one is a ]? add it */
                if (*f == L']') {
                    set[n++] = *f;
                    f++;
                }

                /* now build the set */
                for (; n < SCANF_BUF_SIZE - 1 && *f && *f != L']'; f++) {
                    /* is it a range? */
                    if (*f == L'-') {
                        f++;

                        /* start or end? hyphen itself */
                        if (n == 0 || *f == L']')
                            set[n++] = L'-';
                        else {
                            /* pick previous char */
                            wchar_t c = set[n - 1];

                            /* fill */
                            while (n < SCANF_BUF_SIZE - 1 && c < *f)
                                set[n++] = ++c;
                        }
                    }
                    else
                        /* is it another command? */
                    if (*f == L'%') {
                        int i;

                        f++;
                        for (i = 0; scanf_sets[i].cmd; i++) {
                            if (*f == scanf_sets[i].cmd) {
                                set[n] = L'\0';
                                wcscat(set, scanf_sets[i].yset);
                                n += wcslen(scanf_sets[i].yset);
                                break;
                            }
                        }
                    }
                    else
                        set[n++] = *f;
                }

                /* skip the ] */
                f++;

                set[n] = L'\0';
            }
            else
                /* strptime() format */
            if (cmd == L't') {
                if (*f == L'{') {
                    char tmp_f[2048];
                    int n = 0;
                    struct tm tm;
                    char *cptr, *cptr2;

                    f++;
                    while (*f != L'\0' && *f != L'}')
                        wctomb(&tmp_f[n++], *f++);
                    tmp_f[n] = '\0';

                    if (*f)
                        f++;

                    cptr = mpdm_wcstombs(i, NULL);
                    memset(&tm, '\0', sizeof(tm));

#ifdef CONFOPT_STRPTIME
                    cptr2 = strptime(cptr, tmp_f, &tm);
#else
                    cptr2 = NULL;
#endif

                    if (cptr2 != NULL) {
                        time_t t = mktime(&tm);

                        i += (cptr2 - cptr);
                        mpdm_push(r, MPDM_I(t));
                    }

                    free(cptr);
                    continue;
                }
            }
            else
                /* JSON parsing */
            if (cmd == L'j') {
                int t;
                mpdm_t v;

                i = json_parser(i, &t, &v);

                if (t == -1)
                    break;
                else
                    mpdm_push(r, v);
            }
            else
                /* a standard set? */
            {
                int n;

                for (n = 0; scanf_sets[n].cmd != L'\0'; n++) {
                    if (cmd == scanf_sets[n].cmd) {
                        wcscpy(scanf_yset, scanf_sets[n].yset);
                        wcscpy(scanf_nset, scanf_sets[n].nset);
                        break;
                    }
                }
            }

            /* now fill the dynamic string */
            while (vsize &&
                   !wcschr(scanf_nset, *i) &&
                   (scanf_yset[0] == L'\0' || wcschr(scanf_yset, *i)) &&
                   (msize == 0 || wcsncmp(i, scanf_mark, msize) != 0)) {

                /* only add if not being ignored */
                if (!ignore)
                    ptr = mpdm_poke(ptr, &size, i, 1, sizeof(wchar_t));

                i++;
                vsize--;
            }

            if (!ignore && size) {
                /* null terminate and push */
                ptr = mpdm_poke(ptr, &size, L"", 1, sizeof(wchar_t));
                mpdm_push(r, MPDM_ENS(ptr, size - 1));
            }
        }
        else
        if (*f == L' ' || *f == L'\t') {
            /* if it's a blank, sync to next non-blank */
            f++;

            while (*i == L' ' || *i == L'\t')
                i++;
        }
        else
            /* test for literals in the format string */
        if (*i == *f) {
            i++;
            f++;
        }
        else
            break;
    }

    mpdm_unref(str);
    mpdm_unref(fmt);

    mpdm_unrefnd(r);

    return r;
}


/**
 * mpdm_tr - Transliterates a string.
 * @str: the strnig
 * @s1: characters to be changed
 * @s2: characters to replace those in s1
 *
 * Creates a copy of @str, which will have all characters in @s1
 * replaced by those in @s2 matching their position.
 */
mpdm_t mpdm_tr(mpdm_t str, mpdm_t s1, mpdm_t s2)
{
    mpdm_t r;
    wchar_t *ptr;
    wchar_t *cs1;
    wchar_t *cs2;
    wchar_t c;

    mpdm_ref(str);
    mpdm_ref(s1);
    mpdm_ref(s2);

    /* create a copy of the string */
    r = MPDM_NS((wchar_t *)str->data, mpdm_size(str));
    mpdm_ref(r);

    ptr = (wchar_t *)r->data;
    cs1 = (wchar_t *)s1->data;
    cs2 = (wchar_t *)s2->data;

    while ((c = *ptr) != L'\0') {
        int n;

        for (n = 0; cs1[n] != '\0'; n++) {
            if (c == cs1[n]) {
                *ptr = cs2[n];
                break;
            }
        }

        ptr++;
    }

    mpdm_unrefnd(r);
    mpdm_unref(s2);
    mpdm_unref(s1);
    mpdm_unref(str);

    return r;
}
