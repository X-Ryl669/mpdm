/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2009 Angel Ortega <angel@triptico.com>

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

#ifdef CONFOPT_GETTEXT
#include <libintl.h>
#endif

#ifdef CONFOPT_WIN32
#include <windows.h>
#endif

#include "mpdm.h"


/*******************
	Data
********************/

/*******************
	Code
********************/

void *mpdm_poke(void *dst, int *dsize, const void *org, int osize, int esize)
/* pokes (adds) org into dst, which is a dynamic string, making it grow */
{
	if (org != NULL && osize) {
		/* makes room for the new string */
		if ((dst = realloc(dst, (*dsize + osize) * esize)) != NULL) {
			/* copies it */
			memcpy((char *)dst + (*dsize * esize), org, osize * esize);

			/* adds to final size */
			*dsize += osize;
		}
	}

	return dst;
}


wchar_t *mpdm_pokews(wchar_t *dst, int *dsize, const wchar_t *str)
/* adds a wide string to dst using mpdm_poke() */
{
	return mpdm_poke(dst, dsize, str, wcslen(str), sizeof(wchar_t));
}


wchar_t *mpdm_pokev(wchar_t * dst, int *dsize, const mpdm_t v)
/* adds the string in v to dst using mpdm_poke() */
{
	if (v != NULL) {
		const wchar_t *ptr = mpdm_string(v);

		dst = mpdm_pokews(dst, dsize, ptr);
	}

	return dst;
}


wchar_t *mpdm_mbstowcs(const char *str, int *s, int l)
/* converts an mbs to a wcs, but filling invalid chars
   with question marks instead of just failing */
{
	wchar_t *ptr = NULL;
	char tmp[64];		/* really MB_CUR_MAX + 1 */
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
			if (mbstowcs(&wc, tmp, 1) == (size_t) -1) {
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
	char tmp[64];		/* really MB_CUR_MAX + 1 */
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


mpdm_t mpdm_new_wcs(int flags, const wchar_t * str, int size, int cpy)
/* creates a new string value from a wcs */
{
	wchar_t *ptr;

	/* a size of -1 means 'calculate it' */
	if (size == -1 && str != NULL)
		size = wcslen(str);

	/* create a copy? */
	if (cpy) {
		/* free() on destruction */
		flags |= MPDM_FREE;

		/* allocs */
		if ((ptr = malloc((size + 1) * sizeof(wchar_t))) == NULL)
			return NULL;

		/* if no source, reset to zeroes; otherwise, copy */
		if (str == NULL)
			memset(ptr, '\0', size * sizeof(wchar_t));
		else {
			wcsncpy(ptr, str, size);
			ptr[size] = L'\0';
		}
	}
	else
		ptr = (wchar_t *)str;

	/* it's a string */
	flags |= MPDM_STRING;

	return mpdm_new(flags, ptr, size);
}


mpdm_t mpdm_new_mbstowcs(int flags, const char *str, int l)
/* creates a new string value from an mbs */
{
	wchar_t *ptr;
	int size;

	if ((ptr = mpdm_mbstowcs(str, &size, l)) == NULL)
		return NULL;

	/* it's a string */
	flags |= (MPDM_STRING | MPDM_FREE);

	return mpdm_new(flags, ptr, size);
}


mpdm_t mpdm_new_wcstombs(int flags, const wchar_t * str)
/* creates a new mbs value from a wbs */
{
	char *ptr;
	int size;

	ptr = mpdm_wcstombs(str, &size);

	flags |= MPDM_FREE;

	/* unset the string flag; mbs,s are not 'strings' */
	flags &= ~MPDM_STRING;

	return mpdm_new(flags, ptr, size);
}


mpdm_t mpdm_new_i(int ival)
/* creates a new string value from an integer */
{
	mpdm_t v;
	char tmp[32];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp), "%d", ival);

	v = MPDM_MBS(tmp);

	return mpdm_set_ival(v, ival);
}


mpdm_t mpdm_new_r(double rval)
/* creates a new string value from a real number */
{
	mpdm_t v;
	char tmp[128];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp), "%lf", rval);

	/* manually strip useless zeroes */
	if (strchr(tmp, '.') != NULL) {
		char *ptr;

		for (ptr = tmp + strlen(tmp) - 1; *ptr == '0'; ptr--);

		/* if it's over the ., strip it also */
		if (*ptr != '.')
			ptr++;

		*ptr = '\0';
	}

	v = MPDM_MBS(tmp);

	return mpdm_set_rval(v, rval);
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
 * [Strings]
 */
wchar_t *mpdm_string(const mpdm_t v)
{
	static wchar_t wtmp[32];
	char tmp[32];

	/* if it's NULL, return a constant */
	if (v == NULL)
		return L"[NULL]";

	/* if it's a string, return it */
	if (v->flags & MPDM_STRING)
		return (wchar_t *) v->data;

	/* otherwise, return a visual representation */
	snprintf(tmp, sizeof(tmp), "%p", v);
	mbstowcs(wtmp, tmp, sizeof(wtmp));
	wtmp[(sizeof(wtmp) / sizeof(wchar_t)) - 1] = L'\0';

	return wtmp;
}


/**
 * mpdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the MPDM_STRING flag set,
 * a comparison using wcscmp() is returned; if both are arrays,
 * the size is compared first and, if they have the same number
 * elements, each one is compared; otherwise, a simple pointer
 * comparison is done.
 * [Strings]
 */
int mpdm_cmp(const mpdm_t v1, const mpdm_t v2)
{
	int r;

	/* same values? */
	if (v1 == v2)
		return 0;

	/* is any value NULL? */
	if (v1 == NULL)
		return -1;
	if (v2 == NULL)
		return 1;

	/* different values, but same content? (unlikely) */
	if (v1->data == v2->data)
		return 0;

	if (MPDM_IS_STRING(v1) && MPDM_IS_STRING(v2))
		r = wcscoll((wchar_t *) v1->data, (wchar_t *) v2->data);
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
	else
		/* in any other case, compare just pointers */
		r = (int) ((char *)v1->data - (char *)v2->data);

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

		if ((n = MPDM_NS(NULL, ns)) == NULL)
			return NULL;

		ptr = (wchar_t *)n->data;

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

	mpdm_aset(w, n, 0);
	mpdm_aset(w, d, 1);

	return w;
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
	wchar_t *ptr = NULL;
	int s = 0;

	if (s1 == NULL && s2 == NULL)
		return NULL;

	ptr = mpdm_pokev(ptr, &s, s1);
	ptr = mpdm_pokev(ptr, &s, s2);

	/* if no characters were added, returns an empty string */
	if (ptr == NULL)
		return MPDM_LS(L"");

	ptr = mpdm_poke(ptr, &s, L"", 1, sizeof(wchar_t));
	return MPDM_ENS(ptr, s - 1);
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
	if (v == NULL)
		return 0;

	/* if there is no cached integer, calculate it */
	if (!(v->flags & MPDM_IVAL)) {
		int i = 0;

		/* if it's a string, calculate it; other
		   values will have an ival of 0 */
		if (v->flags & MPDM_STRING) {
			char tmp[32];
			char *fmt = "%i";

			wcstombs(tmp, (wchar_t *) v->data, sizeof(tmp));
			tmp[sizeof(tmp) - 1] = '\0';

			/* workaround for mingw32: as it doesn't
			   correctly parse octal and hexadecimal
			   numbers, they are tried as special cases */
			if (tmp[0] == '0') {
				if (tmp[1] == 'x' || tmp[1] == 'X')
					fmt = "%x";
				else
					fmt = "%o";
			}

			sscanf(tmp, fmt, &i);
		}

		mpdm_set_ival(v, i);
	}

	return v->ival;
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
	if (v == NULL)
		return 0;

	/* if there is no cached double, calculate it */
	if (!(v->flags & MPDM_RVAL)) {
		double r = 0.0;

		/* if it's a string, calculate it; other
		   values will have an rval of 0.0 */
		if (v->flags & MPDM_STRING) {
			char tmp[128];
			char *prev_locale;

			wcstombs(tmp, (wchar_t *) v->data, sizeof(tmp));
			tmp[sizeof(tmp) - 1] = '\0';

			/* if the number starts with 0, it's
			   an octal or hexadecimal number; just
			   take the integer value and cast it */
			if (tmp[0] == '0' && tmp[1] != '.')
				r = (double) mpdm_ival(v);
			else {
				/* set locale to C for non locale-dependent
				   floating point conversion */
				prev_locale = setlocale(LC_NUMERIC, "C");

				/* read */
				sscanf(tmp, "%lf", &r);

				/* set previous locale */
				setlocale(LC_NUMERIC, prev_locale);
			}
		}

		mpdm_set_rval(v, r);
	}

	return v->rval;
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

	/* gets the cache, if any */
	if ((i18n = mpdm_hget_s(mpdm_root(), L"__I18N__")) == NULL)
		return str;

	/* try first the cache */
	if ((v = mpdm_hget(i18n, str)) == NULL) {
#ifdef CONFOPT_GETTEXT
		char *s;

		/* convert to mbs */
		v = MPDM_2MBS(str->data);

		/* ask gettext for it */
		s = gettext((char *) v->data);

		/* create new value only if it's different */
		if (s != v->data) {
			v = MPDM_MBS(s);

			/* store in the cache */
			mpdm_hset(i18n, str, v);
		}
		else
#endif				/* CONFOPT_GETTEXT */

			v = str;
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

#ifdef CONFOPT_GETTEXT

	mpdm_t dm;
	mpdm_t dt;

	/* convert both to mbs,s */
	dm = MPDM_2MBS(dom->data);
	dt = MPDM_2MBS(data->data);

	/* bind and set domain */
	bindtextdomain((char *) dm->data, (char *) dt->data);
	textdomain((char *) dm->data);

	mpdm_hset_s(mpdm_root(), L"__I18N__", MPDM_H(0));

	ret = 1;

#endif				/* CONFOPT_GETTEXT */

#ifdef CONFOPT_WIN32

	mpdm_t v;

	if ((v = mpdm_hget_s(mpdm_root(), L"ENV")) != NULL &&
		mpdm_hget_s(v, L"LANG") == NULL) {
		wchar_t *wptr = L"en";

		/* MS Windows crappy language constants... */

		switch((GetSystemDefaultLangID() & 0x00ff)) {
		case 0x01: wptr = L"ar"; break; /* arabic */
		case 0x02: wptr = L"bg"; break; /* bulgarian */
		case 0x03: wptr = L"ca"; break; /* catalan */
		case 0x04: wptr = L"zh"; break; /* chinese */
		case 0x05: wptr = L"cz"; break; /* czech */
		case 0x06: wptr = L"da"; break; /* danish */
		case 0x07: wptr = L"de"; break; /* german */
		case 0x08: wptr = L"el"; break; /* greek */
		case 0x09: wptr = L"en"; break; /* english */
		case 0x0a: wptr = L"es"; break; /* spanish */
		case 0x0b: wptr = L"fi"; break; /* finnish */
		case 0x0c: wptr = L"fr"; break; /* french */
		case 0x0d: wptr = L"he"; break; /* hebrew */
		case 0x0e: wptr = L"hu"; break; /* hungarian */
		case 0x0f: wptr = L"is"; break; /* icelandic */
		case 0x10: wptr = L"it"; break; /* italian */
		case 0x11: wptr = L"jp"; break; /* japanese */
		case 0x12: wptr = L"ko"; break; /* korean */
		case 0x13: wptr = L"nl"; break; /* dutch */
		case 0x14: wptr = L"no"; break; /* norwegian */
		case 0x15: wptr = L"po"; break; /* polish */
		case 0x16: wptr = L"pt"; break; /* portuguese */
		case 0x17: wptr = L"rm"; break; /* romansh (switzerland) */
		case 0x18: wptr = L"ro"; break; /* romanian */
		case 0x19: wptr = L"ru"; break; /* russian */
		case 0x1a: wptr = L"sr"; break; /* serbian */
		case 0x1b: wptr = L"sk"; break; /* slovak */
		case 0x1c: wptr = L"sq"; break; /* albanian */
		case 0x1d: wptr = L"sv"; break; /* swedish */
		}

		mpdm_hset_s(v, L"LANG", MPDM_S(wptr));
	}

#endif				/* CONFOPT_WIN32 */

	return ret;
}


#ifdef CONFOPT_WCWIDTH

int wcwidth(wchar_t);

int mpdm_wcwidth(wchar_t c)
{
	return wcwidth(c);
}

#else				/* CONFOPT_WCWIDTH */

#include "wcwidth.c"

int mpdm_wcwidth(wchar_t c)
{
	return mk_wcwidth(c);
}

#endif				/* CONFOPT_WCWIDTH */


/**
 * mpdm_sprintf - Formats a sprintf()-like string
 * @fmt: the string format
 * @args: an array of values
 *
 * Formats a string using the sprintf() format taking the values from @args.
 * [Strings]
 */
mpdm_t mpdm_sprintf(const mpdm_t fmt, const mpdm_t args)
{
	const wchar_t *i = fmt->data;
	wchar_t *o = NULL;
	int l = 0, n = 0;
	wchar_t c;

	/* loop all characters */
	while ((c = *i++) != L'\0') {
		int m = 0;
		wchar_t *tptr = NULL;
		wchar_t *wptr = NULL;

		if (c == L'%') {
			/* format directive */
			char t_fmt[128];
			char tmp[1024];
			mpdm_t v;
			char *ptr = NULL;

			/* transfer the % */
			t_fmt[m++] = '%';

			/* transform the format to mbs */
			while (*i != L'\0' &&
				m < (int)(sizeof(t_fmt) - MB_CUR_MAX - 1) &&
				wcschr(L"-.0123456789", *i) != NULL)
				m += wctomb(&t_fmt[m], *i++);

			/* transfer the directive */
			m += wctomb(&t_fmt[m], *i++);

			t_fmt[m] = '\0';

			/* by default, copies the format */
			strcpy(tmp, t_fmt);

			/* pick next value */
			v = mpdm_aget(args, n++);

			switch (t_fmt[m - 1]) {
			case 'd':
			case 'i':
			case 'x':
			case 'X':
			case 'o':

				/* integer value */
				snprintf(tmp, sizeof(tmp) - 1,
					t_fmt, mpdm_ival(v));
				break;

			case 'f':

				/* float (real) value */
				snprintf(tmp, sizeof(tmp) - 1,
					t_fmt, mpdm_rval(v));
				break;

			case 's':

				/* string value */
				ptr = mpdm_wcstombs(mpdm_string(v), NULL);
				snprintf(tmp, sizeof(tmp) - 1, t_fmt, ptr);
				free(ptr);

				break;

			case 'c':

				/* char */
				m = 1;
				wptr = &c;
				c = mpdm_ival(v);
				break;

			case '%':

				/* percent sign */
				m = 1;
				wptr = &c;
				break;
			}

			/* transfer */
			if (wptr == NULL)
				wptr = tptr = mpdm_mbstowcs(tmp, &m, -1);
		}
		else {
			/* raw character */
			m = 1;
			wptr = &c;
		}

		/* transfer */
		o = mpdm_poke(o, &l, wptr, m, sizeof(wchar_t));

		/* free the temporary buffer, if any */
		if (tptr != NULL)
			free(tptr);
	}

	if (o == NULL)
		return NULL;

	/* null-terminate */
	o = mpdm_poke(o, &l, L"", 1, sizeof(wchar_t));

	return MPDM_ENS(o, l - 1);
}


/**
 * mpdm_ulc - Converts a string to uppercase or lowecase
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
	int i = mpdm_size(s);

	if ((optr = malloc((i + 1) * sizeof(wchar_t))) != NULL) {
		wchar_t *iptr = mpdm_string(s);
		int n;

		for (n = 0; n < i; n++)
			optr[n] = u ? towupper(iptr[n]) : towlower(iptr[n]);

		optr[n] = L'\0';
		r = MPDM_ENS(optr, i);
	}

	return r;
}


mpdm_t mpdm_scanf(const mpdm_t fmt, const mpdm_t str, int offset)
{
	wchar_t *i = (wchar_t *)str->data;
	wchar_t *f = (wchar_t *)fmt->data;
	mpdm_t r;

	f += offset;
	r = MPDM_A(0);

	while (*i && *f) {
		if (*f == L'%') {
			wchar_t *ptr = NULL;
			int size = 0;
			wchar_t yset[1024] = L"";
			wchar_t nset[1024] = L"";
			wchar_t cmd;
			int vsize = 0;
			int ignore = 0;

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

			/* is it a number? */
			if (wcschr(L"udixf", cmd)) {
				wcscpy(yset, L"0123456789");

				if (cmd != 'u')
					wcscat(yset, L"-");
				if (cmd == 'x')
					wcscat(yset, L"xabcdefABCDEF");
				if (cmd == 'f')
					wcscat(yset, L".");
			}
			else
			/* non-space string */
			if (cmd == L's')
				wcscpy(nset, L" \t");
			else
			/* percent sign */
			if (cmd == L'%') {
				vsize = 1;
				ignore = 1;
				wcscpy(yset, L"%");
			}
			else
			/* raw set */
			if (cmd == L'[') {
				int inv = 0;
				wchar_t tmp[1024];
				int n = 0;

				/* is it an inverse set? */
				if (*f == L'^') {
					inv = 1;
					f++;
				}

				/* first one is a ]? add it */
				if (*f == L']') {
					tmp[n++] = *f;
					f++;
				}

				/* now build the set */
				for (; n < sizeof(tmp) - 1 && *f && *f != L']'; f++) {
					/* is it a range? */
					if (*f == L'-') {
						f++;

						/* start or end? hyphen itself */
						if (n == 0 || *f == L']')
							tmp[n++] = L'-';
						else {
							/* pick previous char */
							wchar_t c = tmp[n - 1];

							/* fill */
							while (n < sizeof(tmp) - 1 && c < *f)
								tmp[n++] = ++c;
						}
					}
					else
						tmp[n++] = *f;
				}

				/* skip the ] */
				f++;

				tmp[n] = L'\0';

				wcscpy(inv ? nset : yset, tmp);
			}

			/* now fill the dynamic string */
			while (vsize &&
			       !wcschr(nset, *i) &&
			       (yset[0] == L'\0' || wcschr(yset, *i))) {

				/* only add if not being ignored */
				if (!ignore)
					ptr = mpdm_poke(ptr, &size, i, 1, sizeof(wchar_t));

				i++;
				vsize--;
			}

			if (!ignore) {
				/* null terminate and push */
				ptr = mpdm_poke(ptr, &size, L"", 1, sizeof(wchar_t));
				mpdm_push(r, MPDM_ENS(ptr, size));
			}
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

	return r;
}
