/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    mpdm_f.c - File management

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

#ifdef CONFOPT_UNISTD_H
#include <unistd.h>
#endif

#include "mpdm.h"

/*******************
	Code
********************/

/**
 * fmd_open - Opens a file.
 * @filename: the file name
 * @mode: an fopen-like mode string
 *
 * Opens a file. If @filename can be open in the specified @mode, an
 * mpdm_v value will be returned containing the file descriptor, or NULL
 * otherwise.
 */
mpdm_v mpdm_open(mpdm_v filename, mpdm_v mode)
{
	FILE * f;
	mpdm_v fd;

	if((f=fopen((char *)filename->data, (char *)mode->data)) == NULL)
		return(NULL);

	/* creates the file value */
	fd=mpdm_new(MPDM_FILE, f, 0);

	return(fd);
}


/**
 * mpdm_close - Closes a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Closes the file descriptor. If @fd hasn't the MPDM_FILE flag,
 * returns -1, or the return value from fclose() instead.
 */
int mpdm_close(mpdm_v fd)
{
	if(! (fd->flags & MPDM_FILE))
		return(-1);

	return(fclose((FILE *)fd->data));
}


/**
 * mpdm_read - Reads a line from a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Reads a line from @fd. Returns the line, or NULL on EOF.
 */
mpdm_v mpdm_read(mpdm_v fd)
{
	char line[128];
	mpdm_v v=NULL;
	int i;
	FILE * f;

	f=(FILE *)fd->data;

	while(fgets(line, sizeof(line) - 1, f) != NULL)
	{
		if((i=strlen(line)) == 0)
			continue;

		/* if line includes \n, it's complete */
		if(line[i - 1] == '\n')
		{
			line[i - 1]='\0';
			i=0;
		}

		/* store */
		v=mpdm_strcat(v, MPDM_S(line));

		/* exit if the line is completely read */
		if(i == 0) break;
	}

	return(v);
}


/**
 * mpdm_write - Writes a value into a file descriptor.
 * @fd: the value containing the file descriptor
 * @v: the value to be written.
 *
 * Writes @v into @fd. If @v is an array, each element will be written
 * as separate lines (using \n as a separator).
 */
int mpdm_write(mpdm_v fd, mpdm_v v)
{
	if(v->flags & MPDM_MULTIPLE)
	{
		int n;

		for(n=0;n < v->size;n++)
			mpdm_write(fd, mpdm_aget(v, n));
	}
	else
		fprintf((FILE *)fd->data, "%s\n", mpdm_string(v));

	return(0);
}


/*
mpdm_v mpdm_bread(mpdm_v fd, int size)
{
}


int mpdm_bwrite(mpdm_vfd, mpdm_v v, int size)
{
}
*/


/**
 * mpdm_unlink - Deletes a file.
 * @filename: file name to be deleted
 *
 * Deletes a file.
 */
int mpdm_unlink(mpdm_v filename)
{
	return(unlink((char *)filename->data));
}
