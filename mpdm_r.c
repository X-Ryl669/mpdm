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

static int last_match_offset=0;
static int last_match_size=0;

/*******************
	Code
********************/

static wchar_t * regex_flags(mpdm_t r)
{
	return(wcsrchr((wchar_t *)r->data, *(wchar_t *)r->data));
}


mpdm_t mpdm_regcomp(mpdm_t r)
{
	mpdm_t c=NULL;

	/* if cache does not exist, create it */
	if(mpdm->regex == NULL)
		mpdm->regex=mpdm_ref(MPDM_H(0));

	/* search the regex in the cache */
	if((c=mpdm_hget(mpdm->regex, r)) == NULL)
	{
		mpdm_t rmb;
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

		if(strchr(flags, 'i') != NULL) f |= REG_ICASE;
		if(strchr(flags, 'm') != NULL) f |= REG_NEWLINE;

		regex++;
		*flags='\0';

		if(!regcomp(&re, regex, f))
		{
			void * ptr;

			if((ptr=malloc(sizeof(regex_t))) != NULL)
			{
				/* copies */
				memcpy(ptr, &re, sizeof(regex_t));

				/* create value */
				c=mpdm_new(MPDM_FREE, ptr, sizeof(regex_t));

				/* stores */
				mpdm_hset(mpdm->regex, r, c);
			}
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
 * for case-insensitive matching, or 'm', to treat the string as a
 * multiline string (i.e., one containing newline characters), so
 * that ^ and $ match the boundaries of each line instead of the
 * whole string.
 *
 * If @r or @v are NULL, the result of the previous regex matching
 * is returned as a two element array. The first element will contain
 * the character offset of the matching and the second the number of
 * characters matched.
 *
 * FIXME: Document @r as a multiple value case.
 *
 * If @r and @v are defined, the matching string is returned or NULL
 * if no string could be matched; otherwise, the previous match
 * coordinates are returned.
 */
mpdm_t mpdm_regex(mpdm_t r, mpdm_t v, int offset)
{
	mpdm_t w=NULL;
	mpdm_t t;

	/* special case: return previous match */
	if(r == NULL || v == NULL)
	{
		w = MPDM_A(2);

		mpdm_aset(w, MPDM_I(last_match_offset), 0);
		mpdm_aset(w, MPDM_I(last_match_size), 1);

		return(w);
	}

	if(r->flags & MPDM_MULTIPLE)
	{
		int n;

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
			offset = last_match_offset + last_match_size;
		}
	}
	else
	{
		mpdm_t cr;

		/* single value; really do the regex */

		/* compile the regex */
		if((cr=mpdm_regcomp(r)) != NULL)
		{
			regmatch_t rm;
			char * ptr;

			/* convert to mbs */
			if((t=MPDM_2MBS((wchar_t *)v->data + offset)) == NULL)
				return(NULL);

			ptr=t->data;

			/* match? */
			if(regexec((regex_t *) cr->data, ptr, 1,
				&rm, offset > 0 ? REG_NOTBOL : 0) == 0)
			{
				/* create a string with the start, just
				   to get the size */
				w=MPDM_NMBS(ptr, rm.rm_so);
				last_match_offset = offset + mpdm_size(w);

				/* create now the matching string */
				w=MPDM_NMBS(ptr + rm.rm_so, rm.rm_eo - rm.rm_so);

				/* and store the size */
				last_match_size = mpdm_size(w);
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
mpdm_t mpdm_sregex(mpdm_t r, mpdm_t v, mpdm_t s, int offset)
{
	mpdm_t cr;
	char * ptr;
	int f;
	wchar_t * global;
	mpdm_t t;
	mpdm_t o=v;

	/* compile the regex */
	if((cr=mpdm_regcomp(r)) != NULL)
	{
		/* takes pointer to global flag */
		if((global=regex_flags(r)) != NULL)
			global=wcschr(global, 'g');

		/* store the first part */
		o=MPDM_NS(v->data, offset);

		/* convert to mbs */
		if((t=MPDM_2MBS((wchar_t *)v->data + offset)) == NULL)
			return(v);

		ptr=(char *) t->data;

		do
		{
			regmatch_t rm;

			/* try match */
			f=!regexec((regex_t *) cr->data, ptr,
				1, &rm, offset > 0 ? REG_NOTBOL : 0);

			if(f)
			{
				/* creates a string from the beginning
				   to the start of the match */
				t=MPDM_NMBS(ptr, rm.rm_so);
				o=mpdm_strcat(o, t);

				/* appends the substitution string */
				o=mpdm_strcat(o, s);

				ptr += rm.rm_eo;
			}

		} while(t && f && global);

		/* no (more) matches; convert and append the rest */
		t=MPDM_MBS(ptr);
		o=mpdm_strcat(o, t);
	}

	return(o);
}
