/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

    fdm_f.c - File management

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

#include "fdm.h"

/*******************
	Code
********************/

/**
 * fmd_open - Opens a file.
 * @filename: the file name
 * @mode: an fopen-like mode string
 *
 * Opens a file. If @filename can be open in the specified @mode, an
 * fdm_v value will be returned containing the file descriptor, or NULL
 * otherwise.
 */
fdm_v fdm_open(char * filename, char * mode)
{
	FILE * f;
	fdm_v fd;

	if((f=fopen(filename, mode)) == NULL)
		return(NULL);

	/* creates the file value */
	fd=fdm_new(FDM_FILE, f, 0);

	return(fd);
}


/**
 * fdm_close - Closes a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Closes the file descriptor. If @fd hasn't the FDM_FILE flag,
 * returns -1, or the return value from fclose() instead.
 */
int fdm_close(fdm_v fd)
{
	if(! (fd->flags & FDM_FILE))
		return(-1);

	return(fclose((FILE *)fd->data));
}


/**
 * fdm_read - Reads a line from a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Reads a line from @fd. Returns the line, or NULL on EOF.
 */
fdm_v fdm_read(fdm_v fd)
{
	char line[128];
	fdm_v v=NULL;
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
		v=fdm_strcat(v, FDM_S(line));

		/* exit if the line is completely read */
		if(i == 0) break;
	}

	return(v);
}


/**
 * fdm_write - Writes a value into a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Writes @v into @fd, and returns the return value from fputs().
 */
int fdm_write(fdm_v fd, fdm_v v)
{
	return(fputs((char *)v->data, (FILE *)fd->data));
}


/*
fdm_v fdm_bread(fdm_v fd, int size)
{
}


int fdm_bwrite(fdm_vfd, fdm_v v, int size)
{
}
*/
