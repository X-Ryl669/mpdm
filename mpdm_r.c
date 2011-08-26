/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2010 Angel Ortega <angel@triptico.com>

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

int mpdm_regex_offset = -1;
int mpdm_regex_size = 0;

/* number of substitutions in last sregex */

int mpdm_sregex_count = 0;


/** code **/

static wchar_t *regex_flags(const mpdm_t r)
{
    return wcsrchr((wchar_t *) r->data, *(wchar_t *) r->data);
}


static mpdm_t mpdm_regcomp(mpdm_t r)
{
    mpdm_t c = NULL;
    mpdm_t regex_cache = NULL;

    mpdm_ref(r);

    /* if cache does not exist, create it */
    if ((regex_cache =
         mpdm_hget_s(mpdm_root(), L"__REGEX_CACHE__")) == NULL) {
        regex_cache = MPDM_H(0);
        mpdm_hset_s(mpdm_root(), L"__REGEX_CACHE__", regex_cache);
    }

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
                c = MPDM_C(MPDM_REGEX, &re, sizeof(regex_t));
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

            mpdm_ref(w);
            mpdm_aset(w, MPDM_I(mpdm_regex_offset), 0);
            mpdm_aset(w, MPDM_I(mpdm_regex_size), 1);
            mpdm_unrefnd(w);
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
            mpdm_ref(w);

            for (n = 0; n < mpdm_size(r); n++) {
                t = mpdm_regex(v, mpdm_aget(r, n), offset);

                if (t == NULL)
                    break;

                /* found; store and move forward */
                mpdm_push(w, t);
                offset = mpdm_regex_offset + mpdm_regex_size;
            }

            mpdm_unrefnd(w);
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
                mpdm_ref(w);

                while ((t = regex1(r, v, offset)) != NULL) {
                    mpdm_push(w, t);

                    offset = mpdm_regex_offset + mpdm_regex_size;
                }

                mpdm_unrefnd(w);
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
    int osize = 0;
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
    mpdm_t cr;
    wchar_t *optr = NULL;
    int osize = 0;
    mpdm_t o = NULL;

    mpdm_ref(r);
    mpdm_ref(v);
    mpdm_ref(s);

    if (r == NULL) {
        /* return last count */
        o = MPDM_I(mpdm_sregex_count);
    }
    else
    if (v != NULL) {
        /* compile the regex */
        if ((cr = mpdm_regcomp(r)) != NULL) {
            char *ptr;
            int f, i = 0;
            wchar_t *global;
            mpdm_t t;

            /* takes pointer to global flag */
            if ((global = regex_flags(r)) !=NULL)
                global = wcschr(global, 'g');

            /* store the first part */
            optr = mpdm_pokewsn(optr, &osize, v->data, offset);

            /* convert to mbs */
            ptr = mpdm_wcstombs((wchar_t *) v->data + offset, NULL);

            /* reset count */
            mpdm_sregex_count = 0;
            mpdm_regex_offset = -1;

            do {
                regmatch_t rm;

                /* try match */
                f = !regexec((regex_t *) cr->data, ptr + i,
                             1, &rm, offset > 0 ? REG_NOTBOL : 0);

                if (f) {
                    /* creates a string from the beginning
                       to the start of the match */
                    t = mpdm_ref(MPDM_NMBS(ptr + i, rm.rm_so));
                    optr = mpdm_pokev(optr, &osize, t);

                    /* store offset of substitution */
                    mpdm_regex_offset = mpdm_size(t) + offset;

                    mpdm_unref(t);

                    /* get the matched part */
                    t = MPDM_NMBS(ptr + i + rm.rm_so, rm.rm_eo - rm.rm_so);

                    /* is s an executable value? */
                    if (MPDM_IS_EXEC(s)) {
                        /* execute s, with t as argument */
                        t = mpdm_exec_1(s, t, NULL);
                    }
                    else
                        /* is s a hash? use match as key */
                    if (MPDM_IS_HASH(s)) {
                        mpdm_t v;

                        mpdm_ref(t);
                        v = mpdm_hget(s, t);

                        /* is the value executable? */
                        if (MPDM_IS_EXEC(v)) {
                            mpdm_t w = mpdm_ref(v);

                            v = mpdm_exec_1(w, t, NULL);

                            mpdm_unref(w);
                        }

                        mpdm_unref(t);

                        t = v;
                    }
                    else
                        t = expand_ampersands(s, t);

                    /* appends the substitution string */
                    mpdm_ref(t);

                    optr = mpdm_pokev(optr, &osize, t);

                    /* store size of substitution */
                    mpdm_regex_size = mpdm_size(t);

                    mpdm_unref(t);

                    i += rm.rm_eo;

                    /* one more substitution */
                    mpdm_sregex_count++;
                }
            } while (f && global);

            /* no (more) matches; convert and append the rest */
            t = MPDM_MBS(ptr + i);
            optr = mpdm_pokev(optr, &osize, t);

            free(ptr);
        }

        /* NULL-terminate */
        optr = mpdm_pokewsn(optr, &osize, L"", 1);

        o = MPDM_ENS(optr, osize - 1);
    }

    mpdm_unref(s);
    mpdm_unref(v);
    mpdm_unref(r);

    return o;
}
