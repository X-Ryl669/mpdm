/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

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
static regmatch_t rm;

/*******************
	Code
********************/

static mpdm_v _tie_d(mpdm_v v)
{
	if(v->data != NULL)
	{
		/* frees the regex data */
		regfree((regex_t *)v->data);

		/* frees the struct itself */
		free(v->data);
		v->data=NULL;
	}

	return(NULL);
}


static mpdm_v _tie_regex(void)
{
	static mpdm_v _tie=NULL;

	if(_tie == NULL)
	{
		/* creates a clone of the cpy tie */
		_tie=mpdm_ref(mpdm_clone(_mpdm_tie_cpy()));

		/* replaces the destructor with its own */
		mpdm_aset(_tie, MPDM_X(_tie_d), MPDM_TIE_DESTROY);
	}

	return(_tie);
}


static wchar_t * _regex_flags(mpdm_v r)
{
	return(wcsrchr((wchar_t *)r->data, *(wchar_t *)r->data));
}


mpdm_v _mpdm_regcomp(mpdm_v r)
{
	static mpdm_v _regex=NULL;
	mpdm_v c;

	/* if cache does not exist, create it */
	if(_regex == NULL)
		_regex=mpdm_ref(MPDM_H(0));

	/* search the regex in the cache */
	if((c=mpdm_hget(_regex, r)) == NULL)
	{
		mpdm_v rmb;
		regex_t re;
		char * regex;
		char * flags;
		int f=REG_EXTENDED;

		/* not found; regex must be compiled */

		/* convert to mbs */
		rmb=MPDM_2MBS(r->data);

		regex=(char *)rmb->data;
		if((flags=strrchr(regex, *regex)) == NULL)
			return(NULL);

		if(strchr(flags, 'i') != NULL)
			f |= REG_ICASE;

		regex++;
		*flags='\0';

		if(!regcomp(&re, regex, f))
		{
			/* correctly compiled; create value */
			c=mpdm_new(0, &re, sizeof(regex_t), _tie_regex());

			/* stores */
			mpdm_hset(_regex, r, c);
		}
	}

	return(c);
}


/**
 * mpdm_regex - Matches a regular expression
 * @r: the regular expression
 * @v: the value to be matched
 * @offset: offset from the start of v->data
 *
 * Matches a regular expression against a value. Valid flags are 'i',
 * for case-insensitive matching, and 'c', to return the coordinates
 * where the match has been found (offset and size) instead of the
 * found substring.
 * FIXME: Document @r as a multiple value case.
 *
 * If the regex is not matched, returns NULL. If it's matched, the
 * matched string is returned unless the 'c' flag is used; in that
 * case, a two element array is returned, with the offset as the
 * first one and the size as the second.
 */
mpdm_v mpdm_regex(mpdm_v r, mpdm_v v, int offset)
{
	mpdm_v w=NULL;

	if(r->flags & MPDM_MULTIPLE)
	{
		int n;
		mpdm_v t;

		/* multiple value; try sequentially all regexes,
		   moving the offset forward */

		w=MPDM_A(mpdm_size(r));

		for(n=0;n < mpdm_size(r);n++)
		{
			t=mpdm_regex(mpdm_aget(r, n), v, offset);

			/* if not found, invalid all search and exit */
			if(t == NULL)
			{
				w=NULL;
				break;
			}

			/* found; store and move forward */
			mpdm_aset(w, t, n);
			offset += rm.rm_eo;
		}
	}
	else
	{
		mpdm_v cr;

		/* single value; really do the regex */

		/* compile the regex */
		if((cr=_mpdm_regcomp(r)) != NULL)
		{
			mpdm_v vmb;

			/* convert to mbs */
			vmb=MPDM_2MBS(v->data);

			/* match? */
			if(regexec((regex_t *) cr->data,
				(char *) vmb->data + offset, 1,
				&rm, offset > 0 ? REG_NOTBOL : 0) == 0)
			{
				wchar_t * ptr=_regex_flags(r);

				/* found; test if coordinates wanted */
				if(wcschr(ptr, 'c') != NULL)
				{
					w=MPDM_A(2);

					mpdm_aset(w,
						MPDM_I(offset + rm.rm_so), 0);
					mpdm_aset(w,
						MPDM_I(rm.rm_eo - rm.rm_so), 1);
				}
				else
				{
					/* just the found string wanted */
					ptr=v->data;

					w=MPDM_NS(ptr + offset + rm.rm_so,
						rm.rm_eo - rm.rm_so);
				}
			}
		}
	}

	return(w);
}


/**
 * mpdm_sregex - Matches and substitutes a regular expression
 * @r: the regular expression
 * @v: the value to be matched
 * @s: the string that will substitute the matched string
 * @offset: offset from the start of v->data
 *
 * Matches a regular expression against a value, and substitutes the
 * found substring with @s. Valid flags are 'i', for case-insensitive
 * matching, and 'g', for global replacements (all ocurrences in @v
 * will be replaced, instead of just the first found one).
 *
 * Returns the modified string, or the original one if no substitutions
 * were done.
 */
mpdm_v mpdm_sregex(mpdm_v r, mpdm_v v, mpdm_v s, int offset)
{
	mpdm_v cr;
	char * ptr;
	int f, i;
	wchar_t * global;
	mpdm_v t;
	int woffset=offset;

	/* compile the regex */
	if((cr=_mpdm_regcomp(r)) != NULL)
	{
		/* takes pointer to global flag */
		if((global=_regex_flags(r)) != NULL)
			global=wcschr(global, 'g');

		/* convert to mbs */
		t=MPDM_2MBS(v->data);

		do
		{
			ptr=(char *) t->data + offset;

			/* try match */
			f=!regexec((regex_t *) cr->data, ptr,
				1, &rm, offset > 0 ? REG_NOTBOL : 0);

			if(f)
			{
				/* size of the found string */
				i=rm.rm_eo - rm.rm_so;

				/* move on */
				offset += rm.rm_eo;
				woffset += rm.rm_so;

				/* do the substitution */
				v=mpdm_splice(v, s, woffset, i);
				v=mpdm_aget(v, 0);

				woffset += mpdm_size(s);
			}

		} while(f && global);
	}

	return(v);
}
