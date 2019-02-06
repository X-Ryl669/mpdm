/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

    mpdm_r.c - Regular expressions

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

#ifdef CONFOPT_PCRE
#include <pcreposix.h>
#endif

#ifdef CONFOPT_SYSTEM_REGEX
#include <regex.h>
#endif

#ifdef CONFOPT_INCLUDED_REGEX
#include "gnu_regex.h"
#endif


/** data **/

/* matching of the last regex */

size_t mpdm_regex_offset = -1;
size_t mpdm_regex_size = 0;

/* number of substitutions in last sregex */

int mpdm_sregex_count = 0;


/** code **/

static wchar_t *regex_flags(const mpdm_t r)
{
    wchar_t *ptr = mpdm_string(r);
    return wcsrchr(ptr, *ptr);
}


void mpdm_regex__destroy(mpdm_t v)
{
    regfree((regex_t *) v->data);
}


static mpdm_t mpdm_regcomp(mpdm_t r)
{
    mpdm_t c = NULL;
    mpdm_t regex_cache = NULL;

    mpdm_ref(r);

    /* if cache does not exist, create it */
    if ((regex_cache = mpdm_hget_s(mpdm_root(), L"__REGEX_CACHE__")) == NULL)
        regex_cache = mpdm_hset_s(mpdm_root(), L"__REGEX_CACHE__", MPDM_H(0));

    /* search the regex in the cache */
    if ((c = mpdm_hget(regex_cache, r)) == NULL) {
        mpdm_t rmb;
        regex_t re;
        char *regex;
        char *flags;
        int f = REG_EXTENDED;

        /* not found; regex must be compiled */

        /* convert to mbs */
        rmb = mpdm_ref(MPDM_2MBS(r->data));
        regex = (char *) rmb->data;

        if ((flags = strrchr(regex, *regex)) != NULL) {

            if (strchr(flags, 'i') != NULL)
                f |= REG_ICASE;
            if (strchr(flags, 'm') != NULL)
                f |= REG_NEWLINE;

            regex++;
            *flags = '\0';

            if (!regcomp(&re, regex, f)) {
                c = MPDM_C(MPDM_TYPE_REGEX, &re, sizeof(regex_t));

                mpdm_hset(regex_cache, r, c);
            }
        }

        mpdm_unref(rmb);
    }

    mpdm_unref(r);

    return c;
}


static mpdm_t regex1(mpdm_t r, const mpdm_t v, int offset)
/* test for one regex */
{
    mpdm_t w = NULL;
    mpdm_t cr;

    mpdm_ref(r);
    mpdm_ref(v);

    /* no matching yet */
    mpdm_regex_offset = -1;

    /* compile the regex */
    if ((cr = mpdm_regcomp(r)) != NULL) {
        regmatch_t rm;
        char *ptr;
        wchar_t *last;
        int o = 0;
        int f = 0;

        /* takes pointer to 'last' flag */
        if ((last = regex_flags(r)) != NULL)
            last = wcschr(last, 'l');

        /* convert to mbs */
        ptr = mpdm_wcstombs((wchar_t *) mpdm_string(v) + offset, NULL);

        /* match? */
        while (regexec((regex_t *) cr->data, ptr + o, 1,
                       &rm, offset > 0 ? REG_NOTBOL : 0) == 0) {
            f++;

            /* if 'last' is not set, it's done */
            if (last == NULL)
                break;

            rm.rm_so += o;
            rm.rm_eo += o;
            o = rm.rm_eo;
        }

        if (f) {
            /* converts to mbs the string from the beginning
               to the start of the match, just to know
               the size (and immediately frees it) */
            free(mpdm_mbstowcs(ptr, &mpdm_regex_offset, rm.rm_so));

            /* add the offset */
            mpdm_regex_offset += offset;

            /* create now the matching string */
            w = MPDM_NMBS(ptr + rm.rm_so, rm.rm_eo - rm.rm_so);

            /* and store the size */
            mpdm_regex_size = mpdm_size(w);
        }

        free(ptr);
    }

    mpdm_unref(v);
    mpdm_unref(r);

    return w;
}


/**
 * mpdm_regex - Matches a regular expression.
 * @v: the value to be matched
 * @r: the regular expression
 * @offset: offset from the start of v->data
 *
 * Matches a regular expression against a value. Valid flags are 'i',
 * for case-insensitive matching, 'm', to treat the string as a
 * multiline string (i.e., one containing newline characters), so
 * that ^ and $ match the boundaries of each line instead of the
 * whole string, 'l', to return the last matching instead of the
 * first one, or 'g', to match globally; in that last case, an array
 * containing all matches is returned instead of a string scalar.
 *
 * If @r is a string, an ordinary regular expression matching is tried
 * over the @v string. If the matching is possible, the match result
 * is returned, or NULL otherwise.
 *
 * If @r is an array (of strings), each element is tried sequentially
 * as an individual regular expression over the @v string, each one using
 * the offset returned by the previous match. All regular expressions
 * must match to be successful. If this is the case, an array (with
 * the same number of arguments) is returned containing the matched
 * strings, or NULL otherwise.
 *
 * If @r is NULL, the result of the previous regex matching
 * is returned as a two element array. The first element will contain
 * the character offset of the matching and the second the number of
 * characters matched. If the previous regex was unsuccessful, NULL
 * is returned.
 * [Regular Expressions]
 */
mpdm_t mpdm_regex(const mpdm_t v, const mpdm_t r, int offset)
{
    mpdm_t w = NULL;

    mpdm_ref(r);
    mpdm_ref(v);

    /* special case: if r is NULL, return previous match */
    if (r == NULL) {
        /* if previous regex was successful... */
        if (mpdm_regex_offset != -1) {
            w = MPDM_A(2);

            mpdm_aset(w, MPDM_I(mpdm_regex_offset), 0);
            mpdm_aset(w, MPDM_I(mpdm_regex_size), 1);
        }
    }
    else
    if (v != NULL) {
        if (r->flags & MPDM_MULTIPLE) {
            int n;
            mpdm_t t;

            /* multiple value; try sequentially all regexes,
               moving the offset forward */

            w = MPDM_A(0);

            for (n = 0; n < mpdm_size(r); n++) {
                t = mpdm_regex(v, mpdm_aget(r, n), offset);

                if (t == NULL)
                    break;

                /* found; store and move forward */
                mpdm_push(w, t);
                offset = mpdm_regex_offset + mpdm_regex_size;
            }
        }
        else {
            wchar_t *global;

            /* takes pointer to 'global' flag */
            if ((global = regex_flags(r)) !=NULL)
                global = wcschr(global, 'g');

            if (global !=NULL) {
                mpdm_t t;

                /* match sequentially until done */
                w = MPDM_A(0);

                while ((t = regex1(r, v, offset)) != NULL) {
                    mpdm_push(w, t);

                    offset = mpdm_regex_offset + mpdm_regex_size;
                }
            }
            else
                w = regex1(r, v, offset);
        }
    }

    mpdm_unref(v);
    mpdm_unref(r);

    return w;
}


static mpdm_t expand_ampersands(const mpdm_t s, const mpdm_t t)
/* substitutes all unescaped ampersands in s with t */
{
    const wchar_t *sptr = mpdm_string(s);
    wchar_t *wptr;
    wchar_t *optr = NULL;
    size_t osize = 0;
    mpdm_t r = NULL;

    mpdm_ref(s);
    mpdm_ref(t);

    if (s != NULL) {
        while ((wptr = wcschr(sptr, L'\\')) != NULL ||
               (wptr = wcschr(sptr, L'&')) != NULL) {
            int n = wptr - sptr;

            /* add the leading part */
            optr = mpdm_pokewsn(optr, &osize, sptr, n);

            if (*wptr == L'\\') {
                if (*(wptr + 1) == L'&' || *(wptr + 1) == L'\\')
                    wptr++;

                optr = mpdm_pokewsn(optr, &osize, wptr, 1);
            }
            else
            if (*wptr == '&')
                optr = mpdm_pokev(optr, &osize, t);

            sptr = wptr + 1;
        }

        /* add the rest of the string */
        optr = mpdm_pokews(optr, &osize, sptr);
        optr = mpdm_pokewsn(optr, &osize, L"", 1);
        r = MPDM_ENS(optr, osize - 1);
    }

    mpdm_unref(t);
    mpdm_unref(s);

    return r;
}


/**
 * mpdm_sregex - Matches and substitutes a regular expression.
 * @v: the value to be matched
 * @r: the regular expression
 * @s: the substitution string, hash or code
 * @offset: offset from the start of v->data
 *
 * Matches a regular expression against a value, and substitutes the
 * found substring with @s. Valid flags are 'i', for case-insensitive
 * matching, and 'g', for global replacements (all ocurrences in @v
 * will be replaced, instead of just the first found one).
 *
 * If @s is executable, it's executed with the matched part as
 * the only argument and its return value is used as the
 * substitution string.
 *
 * If @s is a hash, the matched string is used as a key to it and
 * its value used as the substitution. If this value itself is
 * executable, it's executed with the matched string as its only
 * argument and its return value used as the substitution.
 *
 * If @r is NULL, returns the number of substitutions made in the
 * previous call to mpdm_sregex() (can be zero if none was done).
 *
 * The global variables @mpdm_regex_offset and @mpdm_regex_size are
 * set to the offset of the matched string and the size of the
 * replaced string, respectively.
 *
 * Always returns a new string (either modified or an exact copy).
 * [Regular Expressions]
 */
mpdm_t mpdm_sregex(mpdm_t v, const mpdm_t r, const mpdm_t s, int offset)
{
    mpdm_t o = NULL;

    mpdm_ref(v);
    mpdm_ref(r);
    mpdm_ref(s);

    if (r == NULL) {
        /* return last count */
        o = MPDM_I(mpdm_sregex_count);
    }
    else
    if (v != NULL) {
        wchar_t *global;
        mpdm_t r2;
        mpdm_t m = NULL;

        mpdm_sregex_count = 0;

        /* FIXME: this is very ugly */

        /* make a copy of the regex to rewrite the flags */
        r2 = mpdm_ref(MPDM_S(mpdm_string(r)));

        /* takes pointer to global flag */
        if ((global = regex_flags(r2)) != NULL) {
            if ((global = wcschr(global, 'g'))) {
                *global = ' ';
            }
        }

        o = mpdm_ref(MPDM_S(mpdm_string(v)));

        while ((m = mpdm_regex(o, r2, offset)) != NULL) {
            mpdm_t w;

            /* is s an executable value? */
            if (MPDM_IS_EXEC(s)) {
                /* execute s, with match as argument */
                w = mpdm_exec_1(s, m, NULL);
            }
            else
            /* is s an object? use match as index */
            if (mpdm_type(s) == MPDM_TYPE_OBJECT) {
                mpdm_ref(m);
                w = mpdm_hget(s, m);

                /* is executable? run it through again */
                if (MPDM_IS_EXEC(w))
                    w = mpdm_exec_1(w, m, NULL);

                mpdm_unref(m);
            }
            else
                w = expand_ampersands(s, m);

            offset = mpdm_regex_offset + mpdm_size(w);

            /* splice */
            w = mpdm_splice(o, w, mpdm_regex_offset, mpdm_regex_size);

            /* take first argument and drop the rest */
            mpdm_store(&o, mpdm_aget(w, 0));
            mpdm_void(w);

            /* one more substitution */
            mpdm_sregex_count++;

            if (!global)
                break;
        }

        mpdm_unrefnd(o);

        mpdm_unref(r2);
    }

    mpdm_unref(s);
    mpdm_unref(r);
    mpdm_unref(v);

    return o;
}
