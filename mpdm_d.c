/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2010 Angel Ortega <angel@triptico.com>

    mpdm_d.c - Debugging utilities

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

/** data **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t * ptr, int *size);
wchar_t * (*mpdm_dump_1) (const mpdm_t v, int l, wchar_t *ptr, int *size) = NULL;

/** code **/

static wchar_t *dump_1(const mpdm_t v, int l, wchar_t *ptr, int *size)
/* dumps one value to the ptr dynamic string with 'l' indenting level */
{
	int n;
	wchar_t *wptr;

	mpdm_ref(v);

	/* indent */
	for (n = 0; n < l; n++)
		ptr = mpdm_pokews(ptr, size, L"  ");

	if (v != NULL) {
		char tmp[256];
		int s;

		if (v->flags & MPDM_DELETED)
			strcpy(tmp, "**DELETED**");
		else
			sprintf(tmp, "%d,%c%c%c%c:", v->ref,
				v->flags & MPDM_FILE ? 'F' :
				(v->flags & MPDM_STRING ? 'S' :
					(v->flags & MPDM_EXEC ? 'X' : '-')),
				v->flags & MPDM_HASH ? 'H' :
					(v->flags & MPDM_MULTIPLE ? 'M' : '-'),
				v->flags & MPDM_FREE ? 'A' : '-',
				v->flags & MPDM_IVAL ? 'I' :
					(v->flags & MPDM_RVAL ? 'R' : '-')
		    	);

		wptr = mpdm_mbstowcs(tmp, &s, -1);
		ptr = mpdm_poke(ptr, size, wptr, s, sizeof(wchar_t));
		free(wptr);

		/* if it's a multiple value, add also the number
		   of elements */
		if (v->flags & MPDM_MULTIPLE) {
			sprintf(tmp, "[%d] ", mpdm_size(v));
			wptr = mpdm_mbstowcs(tmp, &s, -1);
			ptr = mpdm_poke(ptr, size, wptr, s, sizeof(wchar_t));
			free(wptr);
		}
	}

	/* add the visual representation of the value */
	ptr = mpdm_pokev(ptr, size, v);
	ptr = mpdm_pokewsn(ptr, size, L"\n", 1);

	if (v != NULL) {
		/* if it's a hash, iterate it */
		if (v->flags & MPDM_HASH) {
			int c = 0;
			mpdm_t k, w;

			while (mpdm_iterator(v, &c, &k, &w)) {
				ptr = dump_1(k, l + 1, ptr, size);
				ptr = dump_1(w, l + 2, ptr, size);
			}
		}
		else
		if (v->flags & MPDM_MULTIPLE) {
			for (n = 0; n < mpdm_size(v); n++)
				ptr = dump_1(mpdm_aget(v, n), l + 1, ptr, size);
		}
	}

	mpdm_unrefnd(v);

	return ptr;
}


/**
 * mpdm_dumper - Returns a visual representation of a complex value.
 * @v: The value
 *
 * Returns a visual representation of a complex value.
 */
mpdm_t mpdm_dumper(const mpdm_t v)
{
	int size = 0;
	wchar_t *ptr;

	/* if no dumper plugin is defined, fall back to default */
	if (mpdm_dump_1 == NULL)
		mpdm_dump_1 = dump_1;

	ptr = mpdm_dump_1(v, 0, NULL, &size);
	ptr = mpdm_poke(ptr, &size, L"", 1, sizeof(wchar_t));

	return MPDM_ENS(ptr, size - 1);
}


/**
 * mpdm_dump - Dumps a value to stdin.
 * @v: The value
 *
 * Dumps a value to stdin. The value can be complex. This function
 * is for debugging purposes only.
 */
void mpdm_dump(const mpdm_t v)
{
	mpdm_t w;

	w = mpdm_dumper(v);
	mpdm_write_wcs(stdout, mpdm_string(w));
	mpdm_destroy(w);
}


/**
 * mpdm_dump_unref - Dumps all unreferenced values.
 *
 * Dumps all unreferenced values.
 */
void mpdm_dump_unref(void)
{
	mpdm_t v;
	int count, unref;

	/* loop all values */
	v = mpdm->cur;
	count = mpdm->count;
	unref = 0;

	printf("** Unreferenced values:\n");

	while (count--) {
		if (v->ref == 0) {
			mpdm_dump(v);
			unref++;
		}

		v = v->next;
	}

	printf("** Total unreferenced: %d\n", unref);
}
