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
	fdm_v root;		/* the root hash */
} _fdm;


/*******************
	Code
********************/

static void _fdm_nref(fdm_v v[], int count, int iref)
{
	int n;

	for(n=0;n < count;n++)
	{
		if(v[n] != NULL)
			v[n]->ref += iref;
	}
}


/**
 * _fdm_new - Creates a new value, without caching.
 * @tag: flags and type
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value, without caching usual values. See fdm_new()
 * for details.
 */
fdm_v _fdm_new(int tag, void * data, int size)
{
	fdm_v v;
	char tmp[32];

	/* alloc new value and init */
	if((v=(fdm_v) malloc(sizeof(struct _fdm_v))) == NULL)
		return(NULL);

	memset(v, '\0', sizeof(struct _fdm_v));

	/* sanity checks */
	if(tag & FDM_MULTIPLE)
	{
		/* multiple values are always copies and never strings */
		tag |= FDM_COPY;
		tag &= ~ (FDM_STRING | FDM_INTEGER);
	}

	/* is data an integer? */
	if(tag & FDM_INTEGER)
	{
		v->ival=(int) data;
		snprintf(tmp, sizeof(tmp) - 1,"%d", v->ival);
		data=tmp;

		/* force copy, string and calculation */
		tag |= (FDM_COPY | FDM_STRING);
		size=-1;
	}

	/* a size of -1 means 'calculate it' */
	if(size == -1)
	{
		/* only size of string values can be calculated */
		if(data == NULL || !(tag & FDM_STRING))
		{
			free(v);
			return(NULL);
		}

		size=strlen((char *) data);
	}

	v->tag=tag;
	v->size=size;

	if((tag & FDM_COPY) && size)
	{
		int s;

		s=(tag & FDM_MULTIPLE) ? size * sizeof(fdm_v) : size;

		/* alloc new space for data */
		if((v->data=malloc(s + 1)) == NULL)
			return(NULL);

		/* zero or copy the block */
		if(data == NULL)
			memset(v->data, '\0', s);
		else
		{
			if(tag & FDM_STRING)
				strcpy(v->data, data);
			else
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
	_fdm.tail=v;
	_fdm.count ++;

	return(v);
}


/**
 * fdm_new - Creates a new value.
 * @tag: flags and type
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @tag is an or-ed set of flags and an optional
 * user-defined type, @data is a pointer to the data the value will
 * store and @size the size of these data. The flags in @tag define
 * how the data will be stored and its behaviour:
 *
 * If the FDM_COPY flag is set, the value will store a copy of the data
 * using an allocated block of memory. Otherwise, the @data pointer is
 * stored inside the value as is.
 *
 * If FDM_STRING is set, it means @data can be treated as a string
 * (i.e., operations like strcmp() or strlen() can be done on it).
 * Otherwise, @data is treated as an opaque value. For FDM_STRING
 * values, @size can be -1 to force a calculation using strlen().
 * This flag is incompatible with FDM_MULTIPLE.
 *
 * If FDM_INTEGER is set, it means @data holds an integer instead of
 * a pointer to void. The integer value will be converted to string
 * and stored as the value's data and cached internally for future
 * use. FDM_INTEGER implies FDM_COPY and FDM_STRING, and is incompatible
 * with FDM_MULTIPLE.
 *
 * IF FDM_MULTIPLE is set, it means the value itself is an array
 * of values. @Size indicates the number of elements instead of
 * a quantity in bytes. FDM_MULTIPLE implies FDM_COPY and not
 * FDM_STRING. For FDM_MULTIPLE values, @data is usually NULL
 * (meaning to allocate a NULL-initialized array of @size values),
 * but can also be the @data pointer of another multiple value;
 * in this case, the values will be re-referenced.
 *
 * The user defined type optionally stored in @tag must range
 * from 0 to FDM_FLAGS_MASK-1.
 */
fdm_v fdm_new(int tag, void * data, int size)
{
	static fdm_v _fdm_0=NULL;
	static fdm_v _fdm_1=NULL;
	static fdm_v _fdm_empty=NULL;
	fdm_v v=NULL;

	/* first time init */
	if(_fdm_0 == NULL)
	{
		_fdm_0=_fdm_new(FDM_INTEGER, (void *)0, -1);
		_fdm_1=_fdm_new(FDM_INTEGER, (void *)1, -1);
		_fdm_empty=_fdm_new(FDM_STRING, "", 0);

		/* reference them */
		fdm_ref(_fdm_0);
		fdm_ref(_fdm_1);
		fdm_ref(_fdm_empty);
	}

	/* try very common values of data */
	if(data != NULL)
	{
		if(tag & FDM_STRING)
		{
			if(*((char *)data) == '\0')
				v=_fdm_empty;
		}

		if(tag & FDM_INTEGER)
		{
			if((int) data == 0)
				v=_fdm_0;
			else
			if((int) data == 1)
				v=_fdm_1;
		}
	}

	if(v == NULL)
		v=_fdm_new(tag, data, size);

	return(v);
}


/**
 * fdm_ref - Increments the reference count of a value.
 * @v: the value
 *
 * Increments the reference count of a value.
 */
int fdm_ref(fdm_v v)
{
	return(v->ref++);
}


/**
 * fdm_unref - Decrements the reference count of a value.
 * @v: the value
 *
 * Decrements the reference count of a value.
 */
int fdm_unref(fdm_v v)
{
	return(v->ref--);
}


/**
 * fdm_sweep - Sweeps unreferenced values
 * @count: number of values to be swept
 *
 * Destroys values with a reference count of 0. @count is the
 * number of values to be checked for deletion; special values of
 * @count are -1, that forces a check of all currently known values
 * (can be time-consuming) and 0, which tells fdm_sweep() to check a
 * small group of them on each call.
 */
void fdm_sweep(int count)
{
	static int lcount=0;
	int n;

	/* if it's worthless, don't do it */
	if(_fdm.count < 16)
	{
		/* store current count, to avoid a sweeping
		   rush when the threshold is crossed */
		lcount=_fdm.count;
		return;
	}

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


/**
 * fdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the FDM_STRING flag set,
 * a comparison using strcmp() is returned; otherwise, a
 * simple pointer comparison is done.
 */
int fdm_cmp(fdm_v v1, fdm_v v2)
{
	/* if both values are strings, compare as such */
	if((v1->tag & FDM_STRING) && (v2->tag & FDM_STRING))
		return(strcmp((char *)v1->data, (char *)v2->data));

	/* in any other case, compare just pointers */
	return(v1->data - v2->data);
}


/**
 * fdm_ival - Returns a value's data as an integer
 * @v: the value
 *
 * Returns a value's data as an integer. If the value is a string,
 * it's converted via sscanf and returned; non-string values have all
 * an ival of 0. The converted integer is cached, so costly string
 * conversions are only done once. Values created with the FDM_INTEGER
 * flag set have its ival cached from the beginning.
 */
int fdm_ival(fdm_v v)
{
	/* if there is no cached integer, calculate it */
	if(!(v->tag & FDM_INTEGER))
	{
		int i=0;

		/* if it's a string, calculate it; other
		   values will have an ival of 0 */
		if(v->tag & FDM_STRING)
			sscanf((char *)v->data, "%i", &i);

		v->ival=i;
		v->tag |= FDM_INTEGER;
	}

	return(v->ival);
}


/**
 * fdm_poke - Inserts a character in a dynamic string value.
 * @v: the value
 * @c: the char
 * @offset: offset where the character is to be inserted
 *
 * Inserts the character @c at @offset inside a value, that
 * should have the FDM_COPY and FDM_STRING flags set (no check
 * is done). If there is no room in the buffer (i.e. offset
 * is bigger than the value's size), it is expanded via realloc()
 * to make it fit.
 */
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


/**
 * fdm_dup - Duplicates a value
 * @v: the value
 *
 * Duplicates a value. If the value is multiple, a new value will
 * be created containing duplicates of all its elements; otherwise,
 * the same unchanged value is returned.
 */
fdm_v fdm_dup(fdm_v v)
{
	fdm_v w;
	int n;

	/* if NULL or value is not multiple, return as is */
	if(v == NULL || !(v->tag & FDM_MULTIPLE))
		return(v);

	/* create new */
	w=fdm_new(v->tag, NULL, v->size);

	/* fills each element with duplicates of the original */
	for(n=0;n < v->size;n++)
		fdm_aset(w, fdm_dup(fdm_aget(v, n)), n);

	return(w);
}


fdm_v fdm_root(void)
{
	if(_fdm.root == NULL)
	{
		_fdm.root=FDM_H(0);
		fdm_ref(_fdm.root);
	}

	return(_fdm.root);
}
