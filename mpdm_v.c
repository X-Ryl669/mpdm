/*

    fdm - Filp Data Manager
    Copyright (C) 2003 Angel Ortega <angel@triptico.com>

    fdm_v.c - Basic value management

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdm.h"

/*******************
	Data
********************/

/* control structure */

static struct
{
	fdm_v head;		/* head of values */
	fdm_v tail;		/* tail of values */
	int count;		/* total count */
} _fdm;


/*******************
	Code
********************/

int fdm_ref(fdm_v v)
{
	return(v->ref++);
}


int fdm_unref(fdm_v v)
{
	return(v->ref--);
}


static void _fdm_nref(fdm_v v[], int count, int ref)
{
	int n;

	for(n=0;n < count;n++)
	{
		if(v[n] != NULL)
			v[n]->ref += ref;
	}
}


fdm_v fdm_new(int tag, void * data, int size)
{
	fdm_v v;

	/* sanity checks */
	if(tag & FDM_MULTIPLE)
	{
		/* multiple values are always copies and never strings */
		tag |= FDM_COPY;
		tag &= ~ FDM_STRING;
	}

	/* a size of -1 means 'calculate it' */
	if(size == -1)
	{
		/* only size of string values can be calculated */
		if(data == NULL || !(tag & FDM_STRING))
			return(NULL);

		size=strlen((char *) data);
	}

	/* alloc new value */
	if((v=(fdm_v) malloc(sizeof(struct _fdm_v))) == NULL)
		return(NULL);

	memset(v, '\0', sizeof(struct _fdm_v));
	v->tag=tag;
	v->size=size;

	if((tag & FDM_COPY) && size)
	{
		int s;

		s=(tag & FDM_MULTIPLE) ? s * sizeof(fdm_v) : s;

		/* alloc new space for data */
		if((v->data=malloc(s)) == NULL)
			return(NULL);

		/* zero or copy data */
		if(data == NULL)
			memset(v->data, '\0', s);
		else
		{
			memcpy(v->data, data, s);

			/* if data is multiple, re-reference its elements */
			if(tag & FDM_MULTIPLE)
				_fdm_nref((fdm_v *)v->data, size, 1);
		}
	}
	else
		v->data=data;

	/* add to the value chain and count */
	if(_fdm.head == NULL) _fdm.head=v;
	if(_fdm.tail != NULL) _fdm.tail->next=v;
	_fdm.count ++;

	return(v);
}


void fdm_sweep(int count)
{
	static int lcount=0;
	int n;

	/* if it's worthless, don't do it */
	if(_fdm.count < 16)
		return;

	/* if count is -1, sweep all */
	if(count == -1) count=_fdm.count;

	/* if count is zero, sweep 'some' values */
	if(count == 0) count=_fdm.count - lcount + 2;

	for(n=0;n < count;n++)
	{
		/* is the value referenced? */
		if(_fdm.head->ref)
		{
			/* yes; rotate to next */
			_fdm.tail->next=_fdm.head;
			_fdm.head=_fdm.head->next;
			_fdm.tail=_fdm.tail->next;
			_fdm.tail->next=NULL;
		}
		else
		{
			fdm_v v;

			/* value is to be destroyed */
			v=_fdm.head;
			_fdm.head=_fdm.head->next;

			/* unref all elements if multiple */
			if(v->tag & FDM_MULTIPLE)
				_fdm_nref((fdm_v *)v->data, v->size, -1);

			/* free data if local copy */
			if(v->tag & FDM_COPY) free(v->data);

			/* free the value itself */
			free(v);

			/* one value less */
			_fdm.count --;
		}
	}

	lcount=_fdm.count;
}


int fdm_cmp(fdm_v v1, fdm_v v2)
{
	/* if both values are strings, compare as such */
	if((v1->tag & FDM_STRING) && (v2->tag & FDM_STRING))
		return(strcmp((char *)v1->data, (char *)v2->data));

	/* in any other case, compare just pointers */
	return(v1->data - v2->data);
}
