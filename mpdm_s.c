/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

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

#include "mpdm.h"

/*******************
	Code
********************/

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
	static wchar_t tmp[32];

	/* if it's NULL, return a constant */
	if(v == NULL)
		return(L"[NULL]");

	/* if it's a string, return it */
	if(v->flags & MPDM_STRING)
		return((wchar_t *)v->data);

	/* otherwise, return a visual representation */
	SWPRINTF(tmp, sizeof(tmp) / sizeof(wchar_t), L"%p", v);

	return(tmp);
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
	return((int)v1->data - (int)v2->data);
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

		/* never delete further the end */
		if(offset + del > os) del=os - offset;

		/* deleted space */
		if(del > 0)
			d=MPDM_NS(((wchar_t *) v->data) + offset, del);

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
			wmemcpy(ptr, v->data, offset);
			ptr += offset;
		}

		/* copy the text to be inserted */
		if(ins > 0)
		{
			wmemcpy(ptr, i->data, ins);
			ptr += ins;
		}

		/* copy the remaining */
		if(os - r > 0)
		{
			wmemcpy(ptr, ((wchar_t *) v->data) + r, os - r);
			ptr += (os - r);
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
			swscanf((wchar_t *)v->data, L"%i", &i);

		v->ival=i;
		v->flags |= MPDM_IVAL;
	}

	return(v->ival);
}


mpdm_v _mpdm_inew(int ival)
{
	mpdm_v v;
	wchar_t tmp[32];

	/* creates the visual representation */
	SWPRINTF(tmp, (sizeof(tmp) / sizeof(wchar_t)), L"%d", ival);

	v=MPDM_S(tmp);
	v->flags |= MPDM_IVAL;
	v->ival=ival;

	return(v);
}


mpdm_v _mpdm_new_wcs(wchar_t * s, int n, mpdm_v tie)
/* wrapper function to ensure wchar_t * type testing by the compiler */
{
	return(mpdm_new(0, s, n, tie));
}


mpdm_v _mpdm_new_mbs(char * s, int n, mpdm_v tie)
/* wrapper function to ensure char * type testing by the compiler */
{
	return(mpdm_new(0, s, n, tie));
}


/* tie functions */

static mpdm_v _tie_cpy_c(mpdm_v v)
{
	void * ptr;

	/* creates a copy of data */
	if(v != NULL && v->size)
	{
		if((ptr=malloc(v->size)) == NULL)
			return(NULL);

		if(v->data == NULL)
			memset(ptr, '\0', v->size);
		else
			memcpy(ptr, v->data, v->size);

		v->data=ptr;
	}

	/* unset the string flag; memory blocks are not strings */
	v->flags &= ~ MPDM_STRING;

	return(v);
}


static mpdm_v _tie_cpy_d(mpdm_v v)
{
	/* destroys the copy of data */
	if(v->data != NULL)
	{
		free(v->data);
		v->data=NULL;
	}

	return(v);
}


static mpdm_v _tie_lstr_c(mpdm_v v)
{
	/* just tests if size has to be calculated */
	if(v->size == -1 && v->data != NULL)
		v->size=wcslen((wchar_t *) v->data);

	/* it's a string */
	v->flags |= MPDM_STRING;

	return(v);
}


static mpdm_v _tie_str_c(mpdm_v v)
{
	wchar_t * ptr;

	/* tests if size has to be calculated */
	if(v->size == -1 && v->data != NULL)
		v->size=wcslen((wchar_t *) v->data);

	/* creates a copy */
	if((ptr=malloc((v->size + 1) * sizeof(wchar_t))) == NULL)
		return(NULL);

	if(v->data == NULL)
		wmemset(ptr, L'\0', v->size);
	else
	{
		wcsncpy(ptr, v->data, v->size);
		ptr[v->size]=L'\0';
	}

	v->data=ptr;

	/* it's a string */
	v->flags |= MPDM_STRING;

	return(v);
}


static mpdm_v _tie_mbstowcs_c(mpdm_v v)
{
	wchar_t * ptr;

	/* v->data contains a multibyte string (char *) */

	/* always calculate needed space; v->size is currently ignored */
	v->size=mbstowcs(NULL, v->data, 0);

	/* this can occur in case of invalid encodings */
	if(v->size < 0)
		return(NULL);

	if((ptr=malloc((v->size + 1) * sizeof(wchar_t))) == NULL)
		return(NULL);

	mbstowcs(ptr, v->data, v->size);
	ptr[v->size]=L'\0';

	v->data=ptr;

	/* it's a string */
	v->flags |= MPDM_STRING;

	return(v);
}


static mpdm_v _tie_wcstombs_c(mpdm_v v)
{
	char * ptr;

	/* v->data contains a wide char string (wchar_t *) */

	/* always calculate needed space; v->size is currently ignored */
	v->size=wcstombs(NULL, v->data, 0);

	if(v->size == -1 || (ptr=malloc(v->size + 1)) == NULL)
		return(NULL);

	wcstombs(ptr, v->data, v->size);
	ptr[v->size]='\0';

	v->data=ptr;

	/* unset the string flag; mbs,s are not 'strings' */
	v->flags &= ~ MPDM_STRING;

	return(v);
}


/* ties */

mpdm_v _mpdm_tie_cpy(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(2));
		mpdm_aset(_tie, MPDM_X(_tie_cpy_c), MPDM_TIE_CREATE);
		mpdm_aset(_tie, MPDM_X(_tie_cpy_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_str(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(2));
		mpdm_aset(_tie, MPDM_X(_tie_str_c), MPDM_TIE_CREATE);
		mpdm_aset(_tie, MPDM_X(_tie_cpy_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_lstr(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(1));
		mpdm_aset(_tie, MPDM_X(_tie_lstr_c), MPDM_TIE_CREATE);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_fre(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(2));
		mpdm_aset(_tie, MPDM_X(_tie_cpy_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_mbstowcs(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(2));
		mpdm_aset(_tie, MPDM_X(_tie_mbstowcs_c), MPDM_TIE_CREATE);
		mpdm_aset(_tie, MPDM_X(_tie_cpy_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


mpdm_v _mpdm_tie_wcstombs(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		_tie=mpdm_ref(MPDM_A(2));
		mpdm_aset(_tie, MPDM_X(_tie_wcstombs_c), MPDM_TIE_CREATE);
		mpdm_aset(_tie, MPDM_X(_tie_cpy_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}
