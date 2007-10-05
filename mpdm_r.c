/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2007 Angel Ortega <angel@triptico.com>

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

/*******************
	Data
********************/

/* matching of the last regex */

int mpdm_regex_offset = -1;
int mpdm_regex_size = 0;

/* number of substitutions in last sregex */

int mpdm_sregex_count = 0;

/*******************
	Code
********************/

static wchar_t *regex_flags(mpdm_t r)
{
	return (wcsrchr((wchar_t *) r->data, *(wchar_t *) r->data));
}


mpdm_t mpdm_regcomp(mpdm_t r)
{
	mpdm_t c = NULL;
	mpdm_t regex_cache = NULL;

	/* if cache does not exist, create it */
	if ((regex_cache = mpdm_hget_s(mpdm_root(), L"__REGEX_CACHE__")) == NULL) {
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
		rmb = MPDM_2MBS(r->data);

		regex = (char *) rmb->data;
		if ((flags = strrchr(regex, *regex)) == NULL)
			return (NULL);

		if (strchr(flags, 'i') != NULL)
			f |= REG_ICASE;
		if (strchr(flags, 'm') != NULL)
			f |= REG_NEWLINE;

		regex++;
		*flags = '\0';

		if (!regcomp(&re, regex, f)) {
			void *ptr;

			if ((ptr = malloc(sizeof(regex_t))) != NULL) {
				/* copies */
				memcpy(ptr, &re, sizeof(regex_t));

				/* create value */
				c = mpdm_new(MPDM_FREE, ptr, sizeof(regex_t));

				/* stores */
				mpdm_hset(regex_cache, r, c);
			}
		}
	}

	return (c);
}


/**
 * mpdm_regex - Matches a regular expression.
 * @r: the regular expression
 * @v: the value to be matched
 * @offset: offset from the start of v->data
 *
 * Matches a regular expression against a value. Valid flags are 'i',
 * for case-insensitive matching, 'm', to treat the string as a
 * multiline string (i.e., one containing newline characters), so
 * that ^ and $ match the boundaries of each line instead of the
 * whole string, or 'l', to return the last matching instead of
 * the first one.
 *
 * If @r is a string, an ordinary regular expression matching is tried
 * over the @v string. If the matching is possible, the matched string
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
mpdm_t mpdm_regex(mpdm_t r, mpdm_t v, int offset)
{
	mpdm_t w = NULL;
	mpdm_t t;

	/* special case: if r is NULL, return previous match */
	if (r == NULL) {
		/* if previous regex was successful... */
		if (mpdm_regex_offset != -1) {
			w = MPDM_A(2);

			mpdm_aset(w, MPDM_I(mpdm_regex_offset), 0);
			mpdm_aset(w, MPDM_I(mpdm_regex_size), 1);
		}

		return (w);
	}

	/* if the string to be tested is NULL, return NULL */
	if (v == NULL)
		return (NULL);

	if (r->flags & MPDM_MULTIPLE) {
		int n;

		/* multiple value; try sequentially all regexes,
		   moving the offset forward */

		w = MPDM_A(mpdm_size(r));

		for (n = 0; n < mpdm_size(r); n++) {
			t = mpdm_regex(mpdm_aget(r, n), v, offset);

			/* if not found, invalid all search and exit */
			if (t == NULL) {
				w = NULL;
				break;
			}

			/* found; store and move forward */
			mpdm_aset(w, t, n);
			offset = mpdm_regex_offset + mpdm_regex_size;
		}
	}
	else {
		mpdm_t cr;

		/* single value; really do the regex */

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
			ptr = mpdm_wcstombs((wchar_t *) v->data + offset, NULL);

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
	}

	return (w);
}


static mpdm_t expand_ampersands(mpdm_t s, mpdm_t t)
/* substitutes all unescaped ampersands in s with t */
{
	wchar_t *sptr = mpdm_string(s);
	wchar_t *wptr;

	if ((wptr = wcschr(sptr, L'&')) != NULL) {
		mpdm_t v = NULL;

		while (wptr != NULL) {
			int n = wptr - sptr;
			mpdm_t t2 = t;

			if (n && *(wptr - 1) == '\\') {
				/* is it escaped? avoid the \ */
				n--;

				/* and set the substitution string to & */
				t2 = MPDM_LS(L"&");
			}

			/* add the leading part */
			v = mpdm_strcat(v, MPDM_NS(sptr, n));

			/* now add the substitution string */
			v = mpdm_strcat(v, t2);

			sptr = wptr + 1;
			wptr = wcschr(sptr, L'&');
		}

		/* add the rest of the string */
		s = mpdm_strcat(v, MPDM_S(sptr));
	}

	return (s);
}


/**
 * mpdm_sregex - Matches and substitutes a regular expression.
 * @r: the regular expression
 * @v: the value to be matched
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
 * its value used as the substitution.
 *
 * If @r is NULL, returns the number of substitutions made in the
 * previous call to mpdm_sregex() (can be zero if none was done).
 *
 * The global variables @mpdm_regex_offset and @mpdm_regex_size are
 * set to the offset of the matched string and the size of the
 * replaced string, respectively.
 *
 * Returns the modified string, or the original one if no substitutions
 * were done.
 * [Regular Expressions]
 */
mpdm_t mpdm_sregex(mpdm_t r, mpdm_t v, mpdm_t s, int offset)
{
	mpdm_t cr;
	mpdm_t o = v;

	if (r == NULL) {
		/* return last count */
		return (MPDM_I(mpdm_sregex_count));
	}

	if (v == NULL)
		return (NULL);

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
		o = MPDM_NS(v->data, offset);

		/* convert to mbs */
		if ((ptr = mpdm_wcstombs((wchar_t *) v->data + offset, NULL)) == NULL)
			return (NULL);

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
				t = MPDM_NMBS(ptr + i, rm.rm_so);
				o = mpdm_strcat(o, t);

				/* store offset of substitution */
				mpdm_regex_offset = mpdm_size(t) + offset;

				/* get the matched part */
				t = MPDM_NMBS(ptr + i + rm.rm_so, rm.rm_eo - rm.rm_so);

				/* is s an executable value? */
				if (MPDM_IS_EXEC(s)) {
					/* protect o from sweeping */
					mpdm_ref(o);

					/* execute s, with t as argument */
					t = mpdm_exec_1(s, t);

					mpdm_unref(o);
				}
				else
					/* is s a hash? use match as key */
				if (MPDM_IS_HASH(s))
					t = mpdm_hget(s, t);
				else
					t = expand_ampersands(s, t);

				/* appends the substitution string */
				o = mpdm_strcat(o, t);

				/* store size of substitution */
				mpdm_regex_size = mpdm_size(t);

				i += rm.rm_eo;

				/* one more substitution */
				mpdm_sregex_count++;
			}

		} while (f && global);

		/* no (more) matches; convert and append the rest */
		t = MPDM_MBS(ptr + i);
		o = mpdm_strcat(o, t);

		free(ptr);
	}

	return (o);
}
