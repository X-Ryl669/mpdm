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

/* translated string cache */
static mpdm_v _cache_i18n=NULL;


/*******************
	Code
********************/

wchar_t * _mpdm_mbstowcs(char * str, int * s)
/* converts an mbs to a wcs, but filling invalid chars
   with question marks instead of just failing */
{
	wchar_t * ptr=NULL;
	char tmp[MB_CUR_MAX + 1];
	wchar_t wc;
	int n, i, c;

	/* zero everything */
	*s=n=i=0;

	/* now loop cptr converting each character */
	for(;;)
	{
		/* no more characters to process? */
		if((c = str[n + i]) == '\0' && i == 0)
			break;

		tmp[i++]=c; tmp[i]='\0';

		/* try to convert */
		if(mbstowcs(&wc, tmp, 1) == -1)
		{
			/* can still be an incomplete multibyte char? */
			if(c != '\0' && i <= MB_CUR_MAX)
				continue;
			else
			{
				/* too many failing bytes; skip 1 byte */
				wc=L'?';
				i=1;
			}
		}

		/* skip used bytes and back again */
		n += i;
		i=0;

		/* alloc space */
		if((ptr=realloc(ptr, (*s + 2) * sizeof(wchar_t))) == NULL)
			return(NULL);

		/* store new char */
		ptr[(*s)++]=wc;
	}

	if(ptr != NULL)
		ptr[*s]=L'\0';

	return(ptr);
}


mpdm_v _mpdm_new_wcs(int flags, wchar_t * str, int size, int cpy)
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


mpdm_v _mpdm_new_mbstowcs(int flags, char * str)
/* creates a new string value from an mbs */
{
	wchar_t * ptr;
	int size;

	if((ptr=_mpdm_mbstowcs(str, &size)) == NULL)
		return(NULL);

	/* it's a string */
	flags |= (MPDM_STRING|MPDM_FREE);

	return(mpdm_new(flags, ptr, size));
}


mpdm_v _mpdm_new_wcstombs(int flags, wchar_t * str)
/* creates a new mbs value from a wbs */
{
	char * ptr;
	int size;

	/* calculate needed space */
	size=wcstombs(NULL, str, 0);

	if(size == -1 || (ptr=malloc(size + 1)) == NULL)
		return(NULL);

	wcstombs(ptr, str, size);
	ptr[size]='\0';

	flags |= MPDM_FREE;

	/* unset the string flag; mbs,s are not 'strings' */
	flags &= ~ MPDM_STRING;

	return(mpdm_new(flags, ptr, size));
}


mpdm_v _mpdm_new_i(int ival)
/* creates a new string value from an integer */
{
	mpdm_v v;
	char tmp[32];

	/* creates the visual representation */
	snprintf(tmp, sizeof(tmp), "%d", ival);

	v=MPDM_MBS(tmp);
	v->flags |= MPDM_IVAL;
	v->ival=ival;

	return(v);
}


mpdm_v _mpdm_new_r(double rval)
/* creates a new string value from a real number */
{
	mpdm_v v;
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
wchar_t * mpdm_string(mpdm_v v)
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
int mpdm_cmp(mpdm_v v1, mpdm_v v2)
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
 * If @offset is negative, is assumed as counting from the end of @v
 * (so -1 means append at the end). If @v is NULL, @i will become the
 * new string, and both @offset and @del will be ignored. If @v is not
 * NULL and @i is, no insertion process is done (only deletion, if
 * applicable).
 *
 * Returns a two element array, with the new string in the first
 * element and the deleted string in the second (with a NULL value
 * if @del is 0).
 */
mpdm_v mpdm_splice(mpdm_v v, mpdm_v i, int offset, int del)
{
	mpdm_v w;
	mpdm_v n=NULL;
	mpdm_v d=NULL;
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

		/* something to delete? */
		if(del > 0)
		{
			/* never delete further the end */
			if(offset + del > os) del=os - offset;

			/* deleted string */
			d=MPDM_NS(((wchar_t *) v->data) + offset, del);
		}

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
mpdm_v mpdm_strcat(mpdm_v s1, mpdm_v s2)
{
	mpdm_v v;

	v=mpdm_splice(s1, s2, -1, 0);
	v=mpdm_aget(v, 0);

	return(v);
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
int mpdm_ival(mpdm_v v)
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

			wcstombs(tmp, (wchar_t *)v->data, sizeof(tmp));
			tmp[sizeof(tmp) - 1]='\0';
			sscanf(tmp, "%i", &i);
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
double mpdm_rval(mpdm_v v)
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

			/* set locale to C for non locale-dependent
			   floating point conversion */
			prev_locale=setlocale(LC_NUMERIC, "C");

			/* read */
			sscanf(tmp, "%lf", &r);

			/* set previous locale */
			setlocale(LC_NUMERIC, prev_locale);
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
mpdm_v mpdm_gettext(mpdm_v str)
{
	mpdm_v v;
	char * s;

	/* try first the cache */
	if((v=mpdm_hget(_cache_i18n, str)) == NULL)
	{
#ifdef CONFOPT_GETTEXT
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
		mpdm_hset(_cache_i18n, str, v);
	}

	return(v);
}


/**
 * mpdm_gettext_domain - Sets domain and data directory for translations
 * @dom: the domain (application name)
 * @dir: data were the .mo files live
 *
 * Sets the domain (application name) and data directory for translating
 * strings. If real gettext support exists, @dir must point to a directory
 * containing the .mo (compiled .po) files.
 */
void mpdm_gettext_domain(mpdm_v dom, mpdm_v dir)
{
#ifdef CONFOPT_GETTEXT

	/* convert both to mbs,s */
	dom=MPDM_2MBS(dom->data);
	dir=MPDM_2MBS(dir->data);

	/* bind and set domain */
	bindtextdomain((char *)dom->data, (char *)dir->data);
	textdomain((char *)dom->data);

#endif

	mpdm_unref(_cache_i18n);
	_cache_i18n=mpdm_ref(MPDM_H(0));
}
