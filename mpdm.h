/*

    fdm - Filp Data Manager
    Copyright (C) 2003 Angel Ortega <angel@triptico.com>

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

/* tag flags */

#define FDM_COPY	0x00010000	/* data is a private copy */
#define FDM_STRING	0x00020000	/* data can be compared */
#define FDM_MULTIPLE	0x00040000	/* data is multiple */

#define FDM_FLAGS(t)	((t)&0xffff0000)
#define FDM_TYPE(t)	((t)&0x0000ffff)

typedef struct _fdm_v * fdm_v;

struct _fdm_v
{
	int tag;	/* value flags and type */
	int ref;	/* reference count */
	int size;	/* data size */
	void * data;	/* the real data */
	fdm_v next;	/* next in chain */
};
