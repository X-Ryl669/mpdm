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

/**
 * fdm_ref - Increments the reference count.
 * @v: the value
 *
 * Increments the reference count.
 */
int fdm_ref(fdm_v v)
{
	return(v->ref++);
}


/**
 * fdm_unref - Decrements the reference count.
 * @v: the value
 *
 * Decrements the reference count.
 */
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


/**
 * fdm_new - Creates a new value.
 * @tag: flags and type
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @tag is the OR operation of the following
 * possible bits: FDM_COPY, if a copy of the data is to be stored
 * (so it uses allocated memory); FDM_STRING, if the value is a string
 * (so usual string operations as strlen or strcmp can be done over it);
 * and FDM_MULTIPLE, if the value is going to contain an array of other
 * values. @tag can also include it its lower 16 bits a user-defined
 * 'type' value that will be stored but ignored. @data is a pointer to
 * a string (if FDM_STRING is used), a block of pointers to other values
 * (if FDM_MULTIPLE is set), or any other kind of data; a NULL value
 * is also valid. @size contains the size of @data; if FDM_MULTIPLE is
 * set, the size is expressed in number of values pointed to, or
 * otherwise in bytes. A special value of -1 can be sent if FDM_STRING
 * is set to force the automatic calculation of the size (via strlen()).
 */
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


/**
 * fdm_sweep - Sweeps unused values
 * @count: number of values to be swept
 *
 * Sweeps unused values. @count is the number of values to be
 * checked for deletion; if it's -1, all currently stored
 * values are checked. A special value of zero in @count
 * tell fdm_sweep() to check a small group of them on each call.
 */
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


void fdm_poke(fdm_v v, char c, int offset)
{
	char * ptr;

	if(offset >= v->size)
	{
		/* increase size */
		v->size = offset + 32;
		v->data=realloc(v->data, v->size);
	}

	ptr=(char *)v->data;
	ptr[offset]=c;
}


fdm_v fdm_splice(fdm_v v, int offset, int size, char * new)
{
	int n, i;
	fdm_v w;
	char * ptr;

	/* creates the new value */
	w=fdm_new(v->tag, NULL, 0);

	ptr=(char *)v->data;

	/* copies source until offset */
	for(n=0;n < offset && ptr[n];n++)
		fdm_poke(w, ptr[n], n);

	/* skips size characters */
	i=n;
	for(;n < offset + size && ptr[n];n++);

	/* inserts new string */
	for(;new != NULL && *new != '\0';new++,i++)
		fdm_poke(w, *new, i);

	/* continue adding */
	for(;ptr[n];n++, i++)
		fdm_poke(w, ptr[n], i);

	/* null terminate */
	fdm_poke(w, '\0', i);

	return(w);
}
