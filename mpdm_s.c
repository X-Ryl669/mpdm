/*

    MPDM - Minimum Profit Data Manager
    mpdm_s.c - String management

    Angel Ortega <angel@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

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

void *mpdm_poke_2(void *dst, int *dsize, int *offset, const void *org,
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


void *mpdm_poke(void *dst, int *dsize, const void *org, int osize, int esize)
/* pokes (adds) org into dst, which is a dynamic string, making it grow */
{
    int offset = *dsize;

    return mpdm_poke_2(dst, dsize, &offset, org, osize, esize);
}


wchar_t *mpdm_pokewsn(wchar_t *dst, int *size, const wchar_t *str, int slen)
/* adds a wide string to dst */
{
    if (str && slen) {
        /* open space */
        dst = realloc(dst, (*size + slen + 1) * sizeof(wchar_t));

        /* copy */
        memcpy(&dst[*size], str, slen * sizeof(wchar_t));

        /* increment counter */
        *size += slen;

        /* NULL-terminate */
        dst[*size] = L'\0';
    }

    return dst;
}


wchar_t *mpdm_pokews(wchar_t *dst, int *dsize, const wchar_t *str)
/* adds a wide string to dst */
{
    return mpdm_pokewsn(dst, dsize, str, wcslen(str));
}


wchar_t *mpdm_pokev(wchar_t *dst, int *dsize, const mpdm_t v)
/* adds the string in v to dst */
{
    if (v != NULL) {
        mpdm_ref(v);
        dst = mpdm_pokews(dst, dsize, mpdm_string(v));
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
    int n, i, c;
    int t = 0;
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
        ptr = calloc((*s + 1), sizeof(wchar_t));
        mbstowcs(ptr, cstr, *s);
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
            if (mbstowcs(&wc, tmp, 1) == (int) - 1) {
                /* can still be an incomplete multibyte char? */
                if (c != '\0' && i <= (int) MB_CUR_MAX)
                    continue;
                else {
                    /* too many failing bytes; skip 1 byte
                       and use the Unicode replacement char */
                    wc = L'\xfffd';
                    i = 1;
                }
            }

            /* skip used bytes and back again */
            n += i;
            i = 0;

            /* store new char */
            if ((ptr = mpdm_pokewsn(ptr, s, &wc, 1)) == NULL)
                break;
        }
    }

    /* free the duplicate */
    if (cstr != str)
        free(cstr);

    return ptr;
}


char *mpdm_wcstombs(const wchar_t *str, int *s)
/* converts a wcs to an mbs, but filling invalid chars
   with question marks instead of just failing */
{
    char *ptr = NULL;
    char tmp[64];               /* really MB_CUR_MAX + 1 */
    int l;
    int t = 0;

    /* allow NULL values for s */
    if (s == NULL)
        s = &t;

    /* try first a direct conversion with wcstombs */
    if ((*s = wcstombs(NULL, str, 0)) != -1) {
        /* direct conversion is possible; do it */
        ptr = calloc(*s + 1, 1);
        wcstombs(ptr, str, *s);
    }
    else {
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
    }

    return ptr;
}


mpdm_t mpdm_new_wcs(const wchar_t *str, int size, int cpy)
/* creates a new string value from a wcs */
{
    wchar_t *ptr = NULL;

    /* a size of -1 means 'calculate it' */
    if (size == -1 && str != NULL)
        size = wcslen(str);

    /* create a copy? */
    if (size >= 0 && cpy) {
        ptr = calloc(size + 1, sizeof(wchar_t));

        /* if there is a source, copy it */
        if (str != NULL)
            wcsncpy(ptr, str, size);
    }

    return mpdm_new(MPDM_TYPE_STRING, ptr ? ptr : str, size);
}


mpdm_t mpdm_new_mbstowcs(const char *str, int l)
/* creates a new string value from an mbs */
{
    wchar_t *ptr;
    int size;

    ptr = mpdm_mbstowcs(str, &size, l);

    return mpdm_new(MPDM_TYPE_STRING, ptr, size);
}


mpdm_t mpdm_new_wcstombs(const wchar_t *str)
/* creates a new mbs value from a wbs */
{
    char *ptr;
    int size;

    ptr = mpdm_wcstombs(str, &size);

    return mpdm_new(MPDM_TYPE_MBS, ptr, size);
}


mpdm_t mpdm_new_i(int ival)
/* creates a new string value from an integer */
{
    return MPDM_C(MPDM_TYPE_INTEGER, &ival, sizeof(ival));
}


mpdm_t mpdm_new_r(double rval)
/* creates a new string value from a real number */
{
    return MPDM_C(MPDM_TYPE_REAL, &rval, sizeof(rval));
}


/* interface */

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
    char tmp[64];
    wchar_t wstr[64] = L"";
    wchar_t *ret = NULL;

    mpdm_ref(v);

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        ret = L"[NULL]";
        break;

    case MPDM_TYPE_STRING:
        ret = (wchar_t *) v->data;
        break;

    case MPDM_TYPE_INTEGER:
        sprintf(tmp, "%d", mpdm_ival(v));
        mbstowcs(wstr, tmp, sizeof(wstr));
        break;

    case MPDM_TYPE_REAL:
        {
            char *prev_locale = setlocale(LC_NUMERIC, "C");

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

            mbstowcs(wstr, tmp, sizeof(wstr));
        }

        break;

    default:
        /* create a visual representation for the value */
        sprintf(tmp, "%p (", v);
        mbstowcs(wstr, tmp, sizeof(wstr) - 2);
        wcscat(wstr, mpdm_type_wcs(v));
        wcscat(wstr, L")");
        break;
    }

    if (ret == NULL) {
        if (wstr[0]) {
            mpdm_t c, w;

            if ((c = mpdm_get_wcs(mpdm_root(), L"__STRINGIFY__")) == NULL)
                c = mpdm_set_wcs(mpdm_root(), MPDM_O(), L"__STRINGIFY__");

            if ((w = mpdm_get_wcs(c, wstr)) == NULL) {
                w = MPDM_S(wstr);
                mpdm_set(c, w, w);
            }

            ret = (wchar_t *) w->data;
        }
        else
            ret = L"[UNKNOWN]";
    }

    mpdm_unrefnd(v);

    return ret;
}


/**
 * mpdm_cmp_wcs - Compares two values (string version).
 * @v1: the first value
 * @v2: the second value
 *
 * Compares the @v2 string against the stringified view of @v1.
 */
int mpdm_cmp_wcs(const mpdm_t v1, const wchar_t *v2)
{
    int r;

    mpdm_ref(v1);
    r = v2 == NULL ? 1 : wcscoll(mpdm_string(v1), v2);
    mpdm_unref(v1);

    return r;
}


/**
 * mpdm_splice_s - Creates a new string value from another.
 * @v: the original value
 * @i: the value to be inserted
 * @offset: offset where the substring is to be inserted
 * @del: number of characters to delete
 * @n: an optional pointer to the new string
 * @d: an optional pointer to the deleted string
 *
 * Creates a new string value from @v, deleting @del chars at @offset
 * and substituting them by @i. If @del is 0, no deletion is done.
 * both @offset and @del can be negative; if this is the case, it's
 * assumed as counting from the end of @v. If @v is NULL, @i will become
 * the new string, and both @offset and @del will be ignored. If @v is
 * not NULL and @i is, no insertion process is done (only deletion, if
 * applicable).
 *
 * Fills @n (in not NULL) with the new string, and @d (if not NULL)
 * with the deleted portion.
 *
 * Returns the new value (if created) or the deleted value (if created).
 * [Strings]
 */
mpdm_t mpdm_splice_s(const mpdm_t v, const mpdm_t i,
                     int offset, int del, mpdm_t *n, mpdm_t *d)
/* do not use this; use mpdm_splice() */
{
    mpdm_ref(v);
    mpdm_ref(i);

    if (n) *n = NULL;
    if (d) *d = NULL;

    if (v != NULL) {
        wchar_t *str = mpdm_string(v);

        offset = mpdm_wrap_pointers(v, offset, &del);

        if (offset > mpdm_size(v))
            offset = mpdm_size(v);

        if (d) {
            /* deleted string */
            *d = MPDM_NS(str + offset, del);
        }

        if (n) {
            wchar_t *ptr = NULL;
            int s = 0;

            /* copy the start of the string */
            ptr = mpdm_pokewsn(ptr, &s, str, offset);

            /* copy the inserted string */
            ptr = mpdm_pokev(ptr, &s, i);

            /* copy the reminder */
            ptr = mpdm_pokews(ptr, &s, str + offset + del);

            *n = ptr ? MPDM_ENS(ptr, s) : MPDM_S(L"");
        }
    }

    mpdm_unref(i);
    mpdm_unref(v);

    /* returns the new value or the deleted value */
    return n ? *n : (d ? *d : NULL);
}


/**
 * mpdm_strcat_wcsn - Concatenates two strings (string with size version).
 * @s1: the first string
 * @s2: the second string
 * @size: the size of the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 * [Strings]
 */
mpdm_t mpdm_strcat_wcsn(const mpdm_t s1, const wchar_t *s2, int size)
{
    mpdm_t r = NULL;

    if (s1 != NULL || s2 != NULL) {
        wchar_t *ptr = NULL;
        int s = 0;

        ptr = mpdm_pokev(ptr, &s, s1);
        ptr = mpdm_pokewsn(ptr, &s, s2, size);

        r = ptr == NULL ? MPDM_S(L"") : MPDM_ENS(ptr, s);
    }

    return r;
}


/**
 * mpdm_strcat_wcs - Concatenates two strings (string version).
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 * [Strings]
 */
mpdm_t mpdm_strcat_wcs(const mpdm_t s1, const wchar_t *s2)
{
    return mpdm_strcat_wcsn(s1, s2, s2 ? wcslen(s2) : 0);
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
    r = mpdm_strcat_wcs(s1, s2 ? mpdm_string(s2) : NULL);
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
 * Returns a value's data as an integer.
 * [Strings]
 * [Value Management]
 */
int mpdm_ival(mpdm_t v)
{
    int i = 0;

    mpdm_ref(v);

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        break;

    case MPDM_TYPE_INTEGER:
        i = *((int *)v->data);
        break;

    case MPDM_TYPE_REAL:
        i = (int) mpdm_rval(v);
        break;

    case MPDM_TYPE_STRING:
        {
            char *mbs = mpdm_wcstombs(mpdm_string(v), NULL);
            i = mpdm_ival_mbs(mbs);
            free(mbs);
        }

        break;

    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_OBJECT:
        i = mpdm_count(v);
        break;

    default:
        break;
    }

    mpdm_unref(v);

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
 * Returns a value's data as a real number (double float).
 * [Strings]
 * [Value Management]
 */
double mpdm_rval(mpdm_t v)
{
    double r = 0.0;

    mpdm_ref(v);

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        break;

    case MPDM_TYPE_REAL:
        r = *((double *)v->data);
        break;

    case MPDM_TYPE_INTEGER:
        r = (double) mpdm_ival(v);
        break;

    case MPDM_TYPE_STRING:
        {
            /* otherwise, calculate it */
            char *mbs = mpdm_wcstombs(mpdm_string(v), NULL);
            r = mpdm_rval_mbs(mbs);
            free(mbs);
        }

        break;

    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_OBJECT:
        r = (double) mpdm_count(v);
        break;

    default:
        break;
    }

    mpdm_unref(v);

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
    mpdm_t v = NULL;

    if (str) {
        mpdm_t i18n = NULL;

        /* gets the cache */
        if ((i18n = mpdm_get_wcs(mpdm_root(), L"__I18N__")) == NULL)
            i18n = mpdm_set_wcs(mpdm_root(), MPDM_O(), L"__I18N__");

        mpdm_ref(str);

        /* try first the cache */
        if ((v = mpdm_get(i18n, str)) == NULL) {
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
            mpdm_set(i18n, v, str);
        }

        mpdm_unref(str);
    }

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

    mpdm_set_wcs(mpdm_root(), MPDM_O(), L"__I18N__");

    mpdm_unref(dt);
    mpdm_unref(dm);

    ret = 1;

#endif                          /* CONFOPT_GETTEXT */

#ifdef CONFOPT_WIN32

    mpdm_t v;

    if ((v = mpdm_get_wcs(mpdm_root(), L"ENV")) != NULL &&
             mpdm_get_wcs(v, L"LANG") == NULL) {
        const char *ptr = "en";
        int c;
        const char *win_langs[] = {
            "en", "ar", "bg", "ca", "zh", "cz", "da", "de", "el", "en", /* 00-09 */
            "es", "fi", "fr", "he", "hu", "is", "it", "jp", "ko", "nl", /* 0a-13 */
            "no", "po", "pt", "rm", "ro", "ru", "sr", "sk", "sq", "sv", /* 14-1d */
            "th", "tr", "ur", "id", "uk", "be", "sl", "et", "lv", "lt", /* 1e-27 */
            "tg", "fa", "vi", "hy", "az"                                /* 28-2d */
        };

        c = GetSystemDefaultLangID() & 0x00ff;

        /* MS Windows crappy language constants... */
        if (c < sizeof(win_langs) / sizeof(char *))
            ptr = win_langs[c];

        mpdm_set_wcs(v, MPDM_MBS(ptr), L"LANG");
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
            o = mpdm_pokews(o, l, L"\\n");
        else
        if (*p == L'\\')
            o = mpdm_pokews(o, l, L"\\\\");
        else
        if (*p == L'"')
            o = mpdm_pokews(o, l, L"\\\"");
        else
        if (*p < 32) {
            char tmp[7];
            wchar_t wtmp[7];

            sprintf(tmp, "\\u%04x", (unsigned int) *p);
            o = mpdm_pokews(o, l, s_mbstowcs(tmp, wtmp));
        }
        else
            o = mpdm_pokewsn(o, l, p, 1);

        p++;
    }

    return o;
}


static wchar_t *json_f(wchar_t *o, int *z, mpdm_t v, int l)
/* fills a %j JSON format */
{
    mpdm_t w, i;
    int n = 0, c = 0;

    /* special test: upper level can only be array or object */
    if (!l && mpdm_type(v) != MPDM_TYPE_ARRAY && mpdm_type(v) != MPDM_TYPE_OBJECT)
        goto end;

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        o = mpdm_pokews(o, z, L"null");
        break;

    case MPDM_TYPE_OBJECT:
        o = mpdm_pokews(o, z, L"{");

        while (mpdm_iterator(v, &n, &w, &i)) {
            if (c)
                o = mpdm_pokews(o, z, L",");

            o = mpdm_pokews(o, z, L"\"");
            o = json_s(o, z, i);
            o = mpdm_pokews(o, z, L"\":");

            o = json_f(o, z, w, l + 1);

            c++;
        }

        o = mpdm_pokews(o, z, L"}");

        break;

    case MPDM_TYPE_ARRAY:
        o = mpdm_pokews(o, z, L"[");

        while (mpdm_iterator(v, &n, &w, NULL)) {
            if (c)
                o = mpdm_pokews(o, z, L",");

            o = json_f(o, z, w, l + 1);

            c++;
        }

        o = mpdm_pokews(o, z, L"]");

        break;

    case MPDM_TYPE_INTEGER:
    case MPDM_TYPE_REAL:
        o = mpdm_pokev(o, z, v);
        break;

    case MPDM_TYPE_STRING:
        o = mpdm_pokews(o, z, L"\"");
        o = json_s(o, z, v);
        o = mpdm_pokews(o, z, L"\"");

        break;

    default:
        o = mpdm_pokews(o, z, L"\"");
        o = mpdm_pokev(o, z, v);
        o = mpdm_pokews(o, z, L"\"");
        break;
    }

end:
    return o;
}


mpdm_t mpdm_fmt(const mpdm_t fmt, const mpdm_t arg)
{
    const wchar_t *i = fmt->data;
    wchar_t c, *o = NULL;
    int l = 0;
    int n = 0;

    mpdm_ref(fmt);
    mpdm_ref(arg);

    /* find first mark */
    while ((c = i[n]) != L'\0' && c != L'%')
        n++;

    o = mpdm_pokewsn(o, &l, i, n);
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
            /* binary dump */
            ptr = tmp;
            unsigned int mask;
            int p = 0;
            int bits = 0;

            /* zero pad? */
            if (t_fmt[1] == '0') {
                p = 1;
                sscanf(&t_fmt[2], "%d", &bits);
            }
            else
                sscanf(&t_fmt[1], "%d", &bits);

            if (bits == 0)
                bits = sizeof(int) * 8;

            mask = 1 << (bits - 1);
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
            o = json_f(o, &l, arg, 0);
            break;

        case 'J':
            /* 'lax' JSON: can be literal */
            o = json_f(o, &l, arg, 1);
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
            o = mpdm_pokewsn(o, &l, &c, 1);
            break;
        }

        /* transfer */
        if (wptr != NULL) {
            o = mpdm_pokewsn(o, &l, wptr, m);
            free(wptr);
        }
    }

    /* fill the rest up to the end */
    n = 0;
    while (i[n] != L'\0')
        n++;

    o = mpdm_pokewsn(o, &l, i, n);

    mpdm_unref(arg);
    mpdm_unref(fmt);

    return o ? MPDM_ENS(o, l) : MPDM_S(L"");
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
        v = mpdm_fmt(v, mpdm_get_i(args, n));

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
    wchar_t *iptr, *optr;
    int i, n;

    mpdm_ref(s);

    i = mpdm_size(s);

    optr = calloc((i + 1), sizeof(wchar_t));
    iptr = mpdm_string(s);

    for (n = 0; n < i; n++)
        optr[n] = u ? towupper(iptr[n]) : towlower(iptr[n]);

    r = MPDM_ENS(optr, i);

    mpdm_unref(s);

    return r;
}


enum {
    JS_ERROR = -1,
    JS_INCOMPLETE,
    JS_OCURLY,
    JS_OBRACK,
    JS_CCURLY,
    JS_CBRACK,
    JS_COMMA,
    JS_COLON,
    JS_VALUE,
    JS_STRING,
    JS_INTEGER,
    JS_REAL,
    JS_TRUE,
    JS_FALSE,
    JS_NULL,
    JS_ARRAY,
    JS_OBJECT
};

static mpdm_t json_lexer(wchar_t **sp, int *t)
{
    wchar_t c;
    wchar_t *ptr = NULL;
    int size = 0;
    mpdm_t v = NULL;
    wchar_t *s = *sp;

    /* skip blanks */
    while (*s == L' ' || *s == L'\t' || *s == L'\n' || *s == L'\r')
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

            if (c == L'\\') {
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

        v = MPDM_ENS(ptr, size);
    }
    else
    if (c == L'-' || (c >= L'0' && c <= L'9') || c == L'.') {
        *t = JS_INTEGER;

        ptr = mpdm_pokewsn(ptr, &size, &c, 1);

        while (((c = *s) >= L'0' && c <= L'9') || c == L'.') {
            if (c == L'.')
                *t = JS_REAL;

            ptr = mpdm_pokewsn(ptr, &size, &c, 1);
            s++;
        }

        v = MPDM_ENS(ptr, size);

        if (*t == JS_REAL)
            v = MPDM_R(mpdm_rval(v));
        else
            v = MPDM_I(mpdm_ival(v));
    }
    else
    if (c == 't' && wcsncmp(s, L"rue", 3) == 0) {
        s += 3;
        *t = JS_TRUE;
        v = mpdm_bool(1);
    }
    else
    if (c == 'f' && wcsncmp(s, L"alse", 4) == 0) {
        s += 4;
        *t = JS_FALSE;
        v = mpdm_bool(0);
    }
    else
    if (c == 'n' && wcsncmp(s, L"ull", 3) == 0) {
        s += 3;
        *t = JS_NULL;
    }
    else
        *t = JS_ERROR;

    *sp = s;

    return v;
}


static mpdm_t json_parse_array(wchar_t **s, int *t);
static mpdm_t json_parse_object(wchar_t **s, int *t);

static mpdm_t json_value(wchar_t **s, int *t, mpdm_t v)
{
    if (*t == JS_OBRACK)
        v = json_parse_array(s, t);
    else
    if (*t == JS_OCURLY)
        v = json_parse_object(s, t);

    if (*t >= JS_VALUE)
        *t = JS_VALUE;
    else {
        v = mpdm_void(v);
        *t = JS_ERROR;
    }

    return v;
}


static mpdm_t json_pair(wchar_t **s, int *t, mpdm_t k)
{
    mpdm_t v = NULL;

    if (*t == JS_STRING) {
        v = json_lexer(s, t);

        if (*t == JS_COLON) {
            v = json_lexer(s, t);
            v = json_value(s, t, v);
        }
        else
            *t = JS_ERROR;
    }
    else
        *t = JS_ERROR;

    if (*t >= JS_VALUE)
        *t = JS_VALUE;
    else {
        k = mpdm_void(k);
        v = mpdm_void(v);
        *t = JS_ERROR;
    }

    return v;
}


static mpdm_t json_parse_object(wchar_t **s, int *t)
{
    mpdm_t h = MPDM_O();
    mpdm_t k = NULL;
    int tt;

    *t = JS_INCOMPLETE;

    k = json_lexer(s, &tt);

    if (tt == JS_CCURLY)
        *t = JS_OBJECT;
    else {
        mpdm_t w = NULL;

        w = json_pair(s, &tt, k);

        if (tt == JS_VALUE) {
            mpdm_set(h, w, k);

            while (*t == JS_INCOMPLETE) {
                k = json_lexer(s, &tt);

                if (tt == JS_CCURLY)
                    *t = JS_OBJECT;
                else
                if (tt == JS_COMMA) {
                    k = json_lexer(s, &tt);
                    w = json_pair(s, &tt, k);

                    if (tt == JS_VALUE)
                        mpdm_set(h, w, k);
                    else
                        *t = JS_ERROR;
                }
                else
                    *t = JS_ERROR;
            }
        }
        else
            *t = JS_ERROR;
    }

    if (*t == JS_ERROR)
        h = mpdm_void(h);

    return h;
}


static mpdm_t json_parse_array(wchar_t **s, int *t)
{
    mpdm_t a = MPDM_A(0);
    mpdm_t w = NULL;
    int tt;

    *t = JS_INCOMPLETE;

    w = json_lexer(s, &tt);

    if (tt == JS_CBRACK)
        *t = JS_ARRAY;
    else {
        w = json_value(s, &tt, w);

        if (tt == JS_VALUE) {
            mpdm_push(a, w);

            while (*t == JS_INCOMPLETE) {
                w = json_lexer(s, &tt);

                if (tt == JS_CBRACK)
                    *t = JS_ARRAY;
                else
                if (tt == JS_COMMA) {
                    w = json_lexer(s, &tt);
                    w = json_value(s, &tt, w);

                    if (tt == JS_VALUE)
                        mpdm_push(a, w);
                    else
                        *t = JS_ERROR;
                }
                else
                    *t = JS_ERROR;
            }
        }
        else
            *t = JS_ERROR;
    }

    if (*t == JS_ERROR)
        a = mpdm_void(a);

    return a;
}


mpdm_t json_parser(wchar_t **s)
{
    mpdm_t v = NULL;
    int t;

    v = json_lexer(s, &t);

    if (t == JS_OCURLY)
        v = json_parse_object(s, &t);
    else
    if (t == JS_OBRACK)
        v = json_parse_array(s, &t);
    else
        t = JS_ERROR;

    if (t != JS_ARRAY && t != JS_OBJECT)
        v = mpdm_void(v);

    return v;
}


mpdm_t json_parser_lax(wchar_t **s)
{
    mpdm_t v = NULL;
    int t;

    v = json_lexer(s, &t);

    if (t == JS_OCURLY)
        v = json_parse_object(s, &t);
    else
    if (t == JS_OBRACK)
        v = json_parse_array(s, &t);

    if (t <= JS_VALUE)
        v = mpdm_void(v);

    return v;
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
    wchar_t *i = mpdm_string(str);
    wchar_t *f = mpdm_string(fmt);
    mpdm_t r;

    mpdm_ref(fmt);
    mpdm_ref(str);

    i += offset;
    r = MPDM_A(0);

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
                mpdm_push(r, MPDM_I(i - mpdm_string(str)));
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
                mpdm_push(r, json_parser(&i));
            }
                /* 'lax' JSON parsing */
            if (cmd == L'J') {
                mpdm_push(r, json_parser_lax(&i));
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
                    ptr = mpdm_pokewsn(ptr, &size, i, 1);

                i++;
                vsize--;
            }

            if (!ignore && size) {
                mpdm_push(r, MPDM_ENS(ptr, size));
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
    r = MPDM_S(mpdm_string(str));

    ptr = mpdm_string(r);
    cs1 = mpdm_string(s1);
    cs2 = mpdm_string(s2);

    while ((c = *ptr)) {
        int n;

        for (n = 0; cs1[n] && cs2[n]; n++) {
            if (c == cs1[n]) {
                *ptr = cs2[n];
                break;
            }
        }

        ptr++;
    }

    mpdm_unref(s2);
    mpdm_unref(s1);
    mpdm_unref(str);

    return r;
}


mpdm_t mpdm_escape(mpdm_t v, wchar_t low, wchar_t high, mpdm_t f)
/**
 * mpdm_escape - Escapes sets of characters in a string.
 * @v: the string
 * @low: lower character limit
 * @high: higher character limit
 * @f: format to apply
 *
 * Escapes characters from the @v string that are lower than
 * @low or higher than @high, applying the @f format, that can
 * be a string for a fmt() / sprintf() format or an executable
 * value.
 */
{
    wchar_t *iptr, *optr;
    int z = 0;
    int n = 0;

    mpdm_ref(v);
    mpdm_ref(f);

    iptr = mpdm_string(v);
    optr = NULL;

    while (iptr[n]) {
        int m;

        /* skip characters inside range */
        for (m = n; iptr[m] && iptr[m] >= low && iptr[m] <= high; m++);

        /* copy them */
        optr = mpdm_pokewsn(optr, &z, &iptr[n], m - n);

        /* now apply format to all characters outside the range */
        while (iptr[m] && (iptr[m] < low || iptr[m] > high)) {
            mpdm_t w;
            wchar_t wc = iptr[m];

            switch (mpdm_type(f)) {
            case MPDM_TYPE_STRING:
                w = mpdm_fmt(f, MPDM_I((int) wc));
                optr = mpdm_pokev(optr, &z, w);
                break;

            default:
                break;
            }

            m++;
        }

        n = m;
    }

    mpdm_unref(f);
    mpdm_unref(v);

    return optr ? MPDM_NS(optr, z) : MPDM_S(L"");
}
