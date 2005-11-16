/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

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

#ifdef CONFOPT_GETTEXT
#include <libintl.h>
#endif

#include "mpdm.h"


/*******************
	Data
********************/

/*******************
	Code
********************/

void * mpdm_poke(void * dst, int * dsize, void * org, int osize, int esize)
/* pokes (adds) org into dst, which is a dynamic string, making it grow */
{
	if(org != NULL && osize)
	{
		/* makes room for the new string */
		dst = realloc(dst, (*dsize + osize + 1) * esize);

		/* copies it */
		memcpy(dst + (*dsize * esize), org, (osize + 1) * esize);

		/* adds to final size */
		*dsize += osize;
	}

	return(dst);
}


wchar_t * mpdm_pokev(wchar_t * dst, int * dsize, mpdm_t v)
/* adds the string in v to dst using mpdm_poke() */
{
	if(v != NULL)
		dst = mpdm_poke(dst, dsize, v->data,
			mpdm_size(v), sizeof(wchar_t));

	return(dst);
}


wchar_t * mpdm_mbstowcs(char * str, int * s, int l)
/* converts an mbs to a wcs, but filling invalid chars
   with question marks instead of just failing */
{
	wchar_t * ptr = NULL;
	char tmp[MB_CUR_MAX + 1];
	wchar_t wc[2];
	int n, i, c, t;

	/* allow NULL values for s */
	if(s == NULL) s = &t;

	/* create a duplicate and optionally limit */
	str = strdup(str);
	if(l >= 0) str[l] = '\0';

	/* try first a direct conversion with mbstowcs */
	if((*s = mbstowcs(NULL, str, 0)) != -1)
	{
		/* direct conversion is possible; do it and return */
		ptr = malloc((*s + 1) * sizeof(wchar_t));
		mbstowcs(ptr, str, *s);
		ptr[*s] = L'\0';

		free(str);

		return(ptr);
	}

	/* zero everything */
	*s = n = i = 0;
	wc[1] = L'\0';

	for(;;)
	{
		/* no more characters to process? */
		if((c = str[n + i]) == '\0' && i == 0)
			break;

		tmp[i++] = c; tmp[i] = '\0';

		/* try to convert */
		if(mbstowcs(wc, tmp, 1) == -1)
		{
			/* can still be an incomplete multibyte char? */
			if(c != '\0' && i <= MB_CUR_MAX)
				continue;
			else
			{
				/* too many failing bytes; skip 1 byte */
				wc[0] = L'?';
				i = 1;
			}
		}

		/* skip used bytes and back again */
		n += i;
		i = 0;

		/* store new char */
		ptr = mpdm_poke(ptr, s, wc, 1, sizeof(wchar_t));
	}

	/* free the duplicate */
	free(str);

	return(ptr);
}


char * mpdm_wcstombs(wchar_t * str, int * s)
/* converts a wcs to an mbs, but filling invalid chars
   with question marks instead of just failing */
{
	char * ptr = NULL;
	char tmp[MB_CUR_MAX + 1];
	int l, t;

	/* allow NULL values for s */
	if(s == NULL) s = &t;

	/* try first a direct conversion with wcstombs */
	if((*s = wcstombs(NULL, str, 0)) != -1)
	{
		/* direct conversion is possible; do it and return */
		ptr = malloc(*s + 1);
		wcstombs(ptr, str, *s);
		ptr[*s] = '\0';

		return(ptr);
	}

	/* invalid encoding? convert characters one by one */
	*s = 0;

	while(*str)
	{
		if((l = wctomb(tmp, *str)) <= 0)
		{
			/* if char couldn't be converted,
			   write a question mark instead */
			l = wctomb(tmp, L'?');
		}

		tmp[l] = '\0';
		ptr = mpdm_poke(ptr, s, tmp, l, 1);

		str++;
	}

	return(ptr);
}


mpdm_t mpdm_new_wcs(int flags, wchar_t * str, int size, int cpy)
/* creates a new string value from a wcs */
{
	wchar_t * ptr;

	/* a size of -1 means 'calculate it' */
	if(size == -1 && str != NULL)
		size=wcslen(str);

	/* create a copy? */
	if(cpy)
	{
		/* free() on destruction */
		flags |= MPDM_FREE;

		/* allocs */
		if((ptr=malloc((size + 1) * sizeof(wchar_t))) == NULL)
			return(NULL);

		/* if no source, reset to zeroes; otherwise, copy */
		if(str == NULL)
			memset(ptr, '\0', size * sizeof(wchar_t));
		else
		{
			wcsncpy(ptr, str, size);
			ptr[size]=L'\0';
		}

		str=ptr;
	}

	/* it's a string */
	flags |= MPDM_STRING;

	return(mpdm_new(flags, str, size));
}


mpdm_t mpdm_new_mbstowcs(int flags, char * str, int l)
/* creates a new string value from an mbs */
{
	wchar_t * ptr;
	int size;

	if((ptr=mpdm_mbstowcs(str, &size, l)) == NULL)
		return(NULL);

	/* it's a string */
	flags |= (MPDM_STRING|MPDM_FREE);

	return(mpdm_new(flags, ptr, size));
}


mpdm_t mpdm_new_wcstombs(int flags, wchar_t * str)
/* creates a new mbs value from a wbs */
{
	char * ptr;
	int size;

	ptr=mpdm_wcstombs(str, &size);

	flags |= MPDM_FREE;

	/* unset the string flag; mbs,s are not 'strings' */
	flags &= ~ MPDM_STRING;

	return(mpdm_new(flags, ptr, size));
}


mpdm_t mpdm_new_i(int ival)
/* creates a new string value from an integer */
{
	mpdm_t v;
	char tmp[32];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp), "%d", ival);

	v=MPDM_MBS(tmp);
	v->flags |= MPDM_IVAL;
	v->ival=ival;

	return(v);
}


mpdm_t mpdm_new_r(double rval)
/* creates a new string value from a real number */
{
	mpdm_t v;
	char tmp[128];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp), "%lf", rval);

	/* manually strip useless zeroes */
	if(strchr(tmp, '.') != NULL)
	{
		char * ptr;

		for(ptr=tmp + strlen(tmp) - 1;*ptr == '0';ptr--);

		/* if it's over the ., strip it also */
		if(*ptr != '.') ptr++;

		*ptr='\0';
	}

	v=MPDM_MBS(tmp);
	v->flags |= MPDM_RVAL;
	v->rval=rval;

	return(v);
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
 */
wchar_t * mpdm_string(mpdm_t v)
{
	static wchar_t wtmp[32];
	char tmp[32];

	/* if it's NULL, return a constant */
	if(v == NULL)
		return(L"[NULL]");

	/* if it's a string, return it */
	if(v->flags & MPDM_STRING)
		return((wchar_t *)v->data);

	/* otherwise, return a visual representation */
	snprintf(tmp, sizeof(tmp), "%p", v);
	mbstowcs(wtmp, tmp, sizeof(wtmp));
	wtmp[sizeof(wtmp) - 1]=L'\0';

	return(wtmp);
}


/**
 * mpdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the MPDM_STRING flag set,
 * a comparison using strcmp() is returned; otherwise, a
 * simple pointer comparison is done.
 */
int mpdm_cmp(mpdm_t v1, mpdm_t v2)
{
	/* special treatment to NULL values */
	if(v1 == NULL)
		return(-1);
	if(v2 == NULL)
		return(1);

	/* if both values are strings, compare as such */
	if((v1->flags & MPDM_STRING) && (v2->flags & MPDM_STRING))
		return(wcscmp((wchar_t *)v1->data, (wchar_t *)v2->data));

	/* in any other case, compare just pointers */
	return((int)(v1->data - v2->data));
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
 */
mpdm_t mpdm_splice(mpdm_t v, mpdm_t i, int offset, int del)
{
	mpdm_t w;
	mpdm_t n=NULL;
	mpdm_t d=NULL;
	int os, ns, r;
	int ins=0;
	wchar_t * ptr;

	if(v != NULL)
	{
		os=mpdm_size(v);

		/* negative offsets start from the end */
		if(offset < 0) offset=os + 1 - offset;

		/* never add further the end */
		if(offset > os) offset=os;

		/* negative del counts as 'characters left' */
		if(del < 0) del=os + 1 - offset + del;

		/* something to delete? */
		if(del > 0)
		{
			/* never delete further the end */
			if(offset + del > os) del=os - offset;

			/* deleted string */
			d=MPDM_NS(((wchar_t *) v->data) + offset, del);
		}
		else
			del=0;

		/* something to insert? */
		ins=mpdm_size(i);

		/* new size and remainder */
		ns=os + ins - del;
		r=offset + del;

		if((n=MPDM_NS(NULL, ns)) == NULL)
			return(NULL);

		ptr=n->data;

		/* copy the beginning */
		if(offset > 0)
		{
			wcsncpy(ptr, v->data, offset);
			ptr += offset;
		}

		/* copy the text to be inserted */
		if(ins > 0)
		{
			wcsncpy(ptr, i->data, ins);
			ptr += ins;
		}

		/* copy the remaining */
		os -= r;
		if(os > 0)
		{
			wcsncpy(ptr, ((wchar_t *) v->data) + r, os);
			ptr += os;
		}

		/* null terminate */
		*ptr=L'\0';
	}
	else
		n=i;

	/* creates the output array */
	w=MPDM_A(2);

	mpdm_aset(w, n, 0);
	mpdm_aset(w, d, 1);

	return(w);
}


/**
 * mpdm_strcat - Concatenates two strings
 * @s1: the first string
 * @s2: the second string
 *
 * Returns a new string formed by the concatenation of @s1 and @s2.
 */
mpdm_t mpdm_strcat(mpdm_t s1, mpdm_t s2)
{
	wchar_t * ptr = NULL;
	int s = 0;

	ptr = mpdm_pokev(ptr, &s, s1);
	ptr = mpdm_pokev(ptr, &s, s2);

	return(MPDM_ENS(ptr, s));
}


/**
 * mpdm_ival - Returns a value's data as an integer
 * @v: the value
 *
 * Returns a value's data as an integer. If the value is a string,
 * it's converted via sscanf and returned; non-string values have all
 * an ival of 0. The converted integer is cached, so costly string
 * conversions are only done once. Values created with the MPDM_IVAL
 * flag set have its ival cached from the beginning.
 */
int mpdm_ival(mpdm_t v)
{
	if(v == NULL)
		return(0);

	/* if there is no cached integer, calculate it */
	if(!(v->flags & MPDM_IVAL))
	{
		int i=0;

		/* if it's a string, calculate it; other
		   values will have an ival of 0 */
		if(v->flags & MPDM_STRING)
		{
			char tmp[32];
			char * fmt = "%i";

			wcstombs(tmp, (wchar_t *)v->data, sizeof(tmp));
			tmp[sizeof(tmp) - 1]='\0';

			/* workaround for mingw32: as it doesn't
			   correctly parse octal and hexadecimal
			   numbers, they are tried as special cases */
			if(tmp[0] == '0')
			{
				if(tmp[1] == 'x' || tmp[1] == 'X')
					fmt = "%x";
				else
					fmt = "%o";
			}

			sscanf(tmp, fmt, &i);
		}

		v->ival=i;
		v->flags |= MPDM_IVAL;
	}

	return(v->ival);
}


/**
 * mpdm_rval - Returns a value's data as an real number (double)
 * @v: the value
 *
 * Returns a value's data as a real number (double float). If the value
 * is a string, it's converted via sscanf and returned; non-string values
 * have all an rval of 0. The converted double is cached, so costly string
 * conversions are only done once. Values created with the MPDM_RVAL
 * flag set have its rval cached from the beginning.
 */
double mpdm_rval(mpdm_t v)
{
	if(v == NULL)
		return(0);

	/* if there is no cached double, calculate it */
	if(!(v->flags & MPDM_RVAL))
	{
		double r=0.0;

		/* if it's a string, calculate it; other
		   values will have an rval of 0.0 */
		if(v->flags & MPDM_STRING)
		{
			char tmp[128];
			char * prev_locale;

			wcstombs(tmp, (wchar_t *)v->data, sizeof(tmp));
			tmp[sizeof(tmp) - 1]='\0';

			/* if the number starts with 0, it's
			   an octal or hexadecimal number; just
			   take the integer value and cast it */
			if(tmp[0] == '0' && tmp[1] != '.')
				r = (double) mpdm_ival(v);
			else
			{
				/* set locale to C for non locale-dependent
				   floating point conversion */
				prev_locale=setlocale(LC_NUMERIC, "C");

				/* read */
				sscanf(tmp, "%lf", &r);

				/* set previous locale */
				setlocale(LC_NUMERIC, prev_locale);
			}
		}

		v->rval=r;
		v->flags |= MPDM_RVAL;
	}

	return(v->rval);
}


/**
 * mpdm_gettext - Translates a string to the current language
 * @str: the string
 *
 * Translates the @str string to the current language. The active language
 * and data directory is set by a call to mpdm_gettext_domain().
 *
 * If the string is found in the current table, the translation is
 * returned; otherwise, the same @str value is returned.
 */
mpdm_t mpdm_gettext(mpdm_t str)
{
	mpdm_t v;

	/* try first the cache */
	if((v=mpdm_hget(mpdm->i18n, str)) == NULL)
	{
#ifdef CONFOPT_GETTEXT
		char * s;

		/* convert to mbs */
		v=MPDM_2MBS(str->data);

		/* ask gettext for it */
		s=gettext((char *)v->data);

		/* create new value only if it's different */
		if(s != v->data)
			v=MPDM_MBS(s);
		else

#endif /* CONFOPT_GETTEXT */

			v=str;

		/* store in the cache */
		mpdm_hset(mpdm->i18n, str, v);
	}

	return(v);
}


/**
 * mpdm_gettext_domain - Sets domain and data directory for translations
 * @dom: the domain (application name)
 * @data: translation data
 *
 * Sets the domain (application name) and translation data for translating
 * strings that will be returned by mpdm_gettext(). If @data is a hash, it
 * is directly used as a translation hash for future translations; if it's
 * a scalar string, @data must point to a directory containing the
 * .mo (compiled .po) files (gettext support must exist for this to work).
 */
void mpdm_gettext_domain(mpdm_t dom, mpdm_t data)
{
	mpdm_unref(mpdm->i18n);

	if(data->flags & MPDM_HASH)
		mpdm->i18n=data;
	else
	{
		mpdm->i18n=MPDM_H(0);

#ifdef CONFOPT_GETTEXT

		/* convert both to mbs,s */
		dom=MPDM_2MBS(dom->data);
		data=MPDM_2MBS(data->data);

		/* bind and set domain */
		bindtextdomain((char *)dom->data, (char *)data->data);
		textdomain((char *)dom->data);

#endif /* CONFOPT_GETTEXT */
	}

	mpdm_ref(mpdm->i18n);
}


int wcwidth(wchar_t);

int mpdm_wcwidth(wchar_t c)
{
#ifdef CONFOPT_WCWIDTH

	return(wcwidth(c));

#else

	/* borrowed from mutt */

	/* sorted list of non-overlapping intervals of non-spacing characters */
	static const struct interval {
		unsigned short first;
		unsigned short last;
	} combining[] = {
	{ 0x0300, 0x034E }, { 0x0360, 0x0362 }, { 0x0483, 0x0486 },
	{ 0x0488, 0x0489 }, { 0x0591, 0x05A1 }, { 0x05A3, 0x05B9 },
	{ 0x05BB, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
	{ 0x05C4, 0x05C4 }, { 0x064B, 0x0655 }, { 0x0670, 0x0670 },
	{ 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
	{ 0x0711, 0x0711 }, { 0x0730, 0x074A }, { 0x07A6, 0x07B0 },
	{ 0x0901, 0x0902 }, { 0x093C, 0x093C }, { 0x0941, 0x0948 },
	{ 0x094D, 0x094D }, { 0x0951, 0x0954 }, { 0x0962, 0x0963 },
	{ 0x0981, 0x0981 }, { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 },
	{ 0x09CD, 0x09CD }, { 0x09E2, 0x09E3 }, { 0x0A02, 0x0A02 },
	{ 0x0A3C, 0x0A3C }, { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 },
	{ 0x0A4B, 0x0A4D }, { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 },
	{ 0x0ABC, 0x0ABC }, { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 },
	{ 0x0ACD, 0x0ACD }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
	{ 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
	{ 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
	{ 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
	{ 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBF, 0x0CBF },
	{ 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD }, { 0x0D41, 0x0D43 },
	{ 0x0D4D, 0x0D4D }, { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 },
	{ 0x0DD6, 0x0DD6 }, { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A },
	{ 0x0E47, 0x0E4E }, { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 },
	{ 0x0EBB, 0x0EBC }, { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 },
	{ 0x0F35, 0x0F35 }, { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 },
	{ 0x0F71, 0x0F7E }, { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 },
	{ 0x0F90, 0x0F97 }, { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 },
	{ 0x102D, 0x1030 }, { 0x1032, 0x1032 }, { 0x1036, 0x1037 },
	{ 0x1039, 0x1039 }, { 0x1058, 0x1059 }, { 0x17B7, 0x17BD },
	{ 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x18A9, 0x18A9 },
	{ 0x20D0, 0x20E3 }, { 0x302A, 0x302F }, { 0x3099, 0x309A },
	{ 0xFB1E, 0xFB1E }, { 0xFE20, 0xFE23 }
	};
	int min = 0;
	int max = sizeof(combining) / sizeof(struct interval) - 1;
	int mid;

	if (c == 0)
		return 0;

	/* test for 8-bit control characters */
	if (c < 32 || (c >= 0x7f && c < 0xa0))
		return -1;

	/* first quick check for Latin-1 etc. characters */
	if (c < combining[0].first)
		return 1;

	/* binary search in table of non-spacing characters */
	while (max >= min) {
		mid = (min + max) / 2;
		if (combining[mid].last < c)
			min = mid + 1;
		else if (combining[mid].first > c)
			max = mid - 1;
		else if (combining[mid].first <= c && combining[mid].last >= c)
		return 0;
	}

	/* if we arrive here, c is not a combining or C0/C1 control character */

	/* fast test for majority of non-wide scripts */
	if (c < 0x1100)
		return 1;

	return 1 +
	((c >= 0x1100 && c <= 0x115f) || /* Hangul Jamo */
	(c >= 0x2e80 && c <= 0xa4cf && (c & ~0x0011) != 0x300a &&
		c != 0x303f) ||		/* CJK ... Yi */
	(c >= 0xac00 && c <= 0xd7a3) || /* Hangul Syllables */
	(c >= 0xf900 && c <= 0xfaff) || /* CJK Compatibility Ideographs */
	(c >= 0xfe30 && c <= 0xfe6f) || /* CJK Compatibility Forms */
	(c >= 0xff00 && c <= 0xff5f) || /* Fullwidth Forms */
	(c >= 0xffe0 && c <= 0xffe6));

#endif /* CONFOPT_WCWIDTH */

}
