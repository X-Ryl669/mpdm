/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

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
	Code
********************/

mpdm_v mpdm_sregex(mpdm_v r, mpdm_v v, mpdm_v s, int offset, char * flags)
{
	char * ptr;
	regex_t re;
	regmatch_t rm;
	int f=0;
	int i,g;

	/* flags */
	if(flags != NULL)
	{
		g=strchr(flags, 'g') != NULL ? 1 : 0;
		i=strchr(flags, 'i') != NULL ? REG_ICASE : 0;
	}
	else
		i=g=0;

	/* compile the regex or fail */
	if(regcomp(&re, (char *) r->data, REG_EXTENDED | i))
		return(NULL);

	do
	{
		ptr=(char *) v->data + offset;

		/* try match */
		f=!regexec(&re, ptr, 1, &rm, offset > 0 ? REG_NOTBOL : 0);

		if(f)
		{
			/* size of the found string */
			i=rm.rm_eo - rm.rm_so;

			/* do the substitution */
			v=mpdm_splice(v, s, offset + rm.rm_so, i);
			v=mpdm_aget(v, 0);

			/* move on */
			offset+=rm.rm_so;
			if(s != NULL) offset+=s->size;
		}

	} while(f && g);

	regfree(&re);

	return(v);
}
