/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2010 Angel Ortega <angel@triptico.com>

    mpdm_t.c - Basic value management

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
#include <locale.h>

#include "mpdm.h"


/** data **/

/* control structure */

struct mpdm_control *mpdm = NULL;


/** code **/

static void cleanup_value(mpdm_t v)
/* cleans a value */
{
	/* collapse multiple values */
	if (v->flags & MPDM_MULTIPLE)
		mpdm_collapse(v, 0, v->size);

	/* free data if needed */
	if (v->data != NULL && v->flags & MPDM_FREE) {
		mpdm->memory_usage -= v->size;
		free((void *)v->data);
		v->data = NULL;
	}
}


int mpdm_destroy(mpdm_t v)
/* destroys a value */
{
	int ret = 0;
	mpdm_t w;

	if (v->ref == 0 && !(v->flags & MPDM_DELETED)) {
		cleanup_value(v);

		/* mark as deleted */
		v->flags |= MPDM_DELETED;

		ret = 1;
	}

	/* try to dequeue next one */
	w = v->next;

	if (w->flags & MPDM_DELETED) {
		/* dequeue */
		v->next = w->next;

		/* if it's the current one, move to next */
		if (mpdm->cur == w)
			mpdm->cur = w->next;

		/* account one value less */
		mpdm->count--;

		/* add to the deleted values queue */
		w->next = mpdm->del;
		mpdm->del = w;
	}

	return ret;
}


/**
 * mpdm_new - Creates a new value.
 * @flags: flags
 * @data: pointer to real data
 * @size: size of data
 *
 * Creates a new value. @flags is an or-ed set of flags, @data is a
 * pointer to the data the value will store and @size the size of these
 * data (if value is to be a multiple one, @size is a number of elements,
 * or a number of bytes otherwise).
 *
 * This function is normally not directly used; use any of the type
 * creation macros instead.
 * [Value Creation]
 */
mpdm_t mpdm_new(int flags, const void *data, int size)
{
	mpdm_t v = NULL;

	/* alloc */
	if ((v = mpdm->del) != NULL)
		mpdm->del = v->next;
	else
	if ((v = malloc(sizeof(struct mpdm_val))) == NULL)
		return NULL;

	/* add to the circular list */
	if (mpdm->cur == NULL)
		v->next = v;
	else {
		v->next = mpdm->cur->next;
		mpdm->cur->next = v;
	}

	mpdm->cur = v->next;

	/* account one value more */
	mpdm->count++;

	/* count memory if data is dynamic */
	if (flags & MPDM_FREE)
		mpdm->memory_usage += size;

	v->flags = flags;
	v->ref = 0;
	v->data = data;
	v->size = size;

	return v;
}


/**
 * mpdm_ref - Increments the reference count of a value.
 * @v: the value
 *
 * Increments the reference count of a value.
 * [Value Management]
 */
mpdm_t mpdm_ref(mpdm_t v)
{
	if (v != NULL)
		v->ref++;
	return v;
}


/**
 * mpdm_unref - Decrements the reference count of a value.
 * @v: the value
 *
 * Decrements the reference count of a value.
 * [Value Management]
 */
mpdm_t mpdm_unref(mpdm_t v)
{
	if (v != NULL)
		v->ref--;
	return v;
}


/**
 * mpdm_sweep - Sweeps unreferenced values.
 * @count: number of values to be swept
 *
 * Destroys values with a reference count of 0. @count is the
 * number of values to be checked for deletion; special values of
 * @count are -1, that forces a check of all currently known values
 * (can be time-consuming) and 0, which tells mpdm_sweep() to check a
 * small group of them on each call.
 * [Value Management]
 */
void mpdm_sweep(int count)
{
	/* if count is zero, sweep 'some' values */
	if (count == 0) {
		if (mpdm->default_sweep < 0)
			count = mpdm->count / -mpdm->default_sweep;
		else
			count = mpdm->default_sweep;
	}

	/* if count is -1, sweep all */
	if (count == -1)
		count = mpdm->count;

	for (; count > 0 && mpdm->count > mpdm->low_threshold; count--) {
		mpdm_destroy(mpdm->cur);
		mpdm->cur = mpdm->cur->next;
	}
}


/**
 * mpdm_size - Returns the size of an element.
 * @v: the element
 *
 * Returns the size of an element.
 * [Value Management]
 */
int mpdm_size(const mpdm_t v)
{
	/* NULL values have no size */
	if (v == NULL)
		return 0;

	return v->size;
}


/**
 * mpdm_clone - Creates a clone of a value.
 * @v: the value
 *
 * Creates a clone of a value. If the value is multiple, a new value will
 * be created containing clones of all its elements; otherwise,
 * the same unchanged value is returned.
 * [Value Management]
 */
mpdm_t mpdm_clone(const mpdm_t v)
{
	if (MPDM_IS_ARRAY(v))
		return mpdm_aclone(v);

	return v;
}


/**
 * mpdm_root - Returns the root hash.
 *
 * Returns the root hash. This hash is stored internally and can be used
 * as a kind of global symbol table.
 * [Value Management]
 */
mpdm_t mpdm_root(void)
{
	if (mpdm->root == NULL)
		mpdm->root = mpdm_ref(MPDM_H(0));

	return mpdm->root;
}


mpdm_t mpdm_set_ival(mpdm_t v, int ival)
/* sets an integer value to a value */
{
	v->flags |= MPDM_IVAL;
	v->ival = ival;

	return v;
}


mpdm_t mpdm_set_rval(mpdm_t v, double rval)
/* sets a real value to a value */
{
	v->flags |= MPDM_RVAL;
	v->rval = rval;

	return v;
}


/**
 * mpdm_exec - Executes an executable value.
 * @c: the code value
 * @args: the arguments
 *
 * Executes an executable value. If @c is a scalar value, its data
 * should be a pointer to a directly executable C function with a
 * prototype of mpdm_t func(mpdm_t args); if it's a multiple one,
 * the first value's data should be a pointer to a directly executable C
 * function with a prototype of mpdm_t func(mpdm_t b, mpdm_t args) and
 * the second value will be passed as the @b argument. This value is used
 * to store bytecode or so when implementing virtual machines or compilers.
 *
 * Returns the return value of the code. If @c is NULL or not executable,
 * returns NULL.
 * [Value Management]
 */
mpdm_t mpdm_exec(mpdm_t c, mpdm_t args)
{
	mpdm_t r = NULL;

	if (c != NULL && (c->flags & MPDM_EXEC)) {
		mpdm_ref(c);
		mpdm_ref(args);

		if (c->flags & MPDM_MULTIPLE) {
			mpdm_t x;
			mpdm_t(*func) (mpdm_t, mpdm_t);

			/* value is multiple; first element is the
			   2 argument version of the executable function,
			   next its optional additional information and
			   finally the arguments */
			x = mpdm_aget(c, 0);

			if ((func = (mpdm_t(*)(mpdm_t, mpdm_t)) (x->data)) != NULL)
				r = func(mpdm_aget(c, 1), args);
		}
		else {
			mpdm_t(*func) (mpdm_t);

			/* value is scalar; c is the 1 argument
			   version of the executable function */
			if ((func = (mpdm_t(*)(mpdm_t)) (c->data)) != NULL)
				r = func(args);
		}

		mpdm_unref(args);
		mpdm_unref(c);
	}

	return r;
}


mpdm_t mpdm_exec_1(mpdm_t c, mpdm_t a1)
{
	mpdm_t r;
	mpdm_t a = MPDM_A(1);

	mpdm_aset(a, a1, 0);
	r = mpdm_exec(c, a);
	return r;
}


mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2)
{
	mpdm_t r;
	mpdm_t a = MPDM_A(2);

	mpdm_aset(a, a1, 0);
	mpdm_aset(a, a2, 1);
	r = mpdm_exec(c, a);
	return r;
}


mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3)
{
	mpdm_t r;
	mpdm_t a = MPDM_A(3);

	mpdm_aset(a, a1, 0);
	mpdm_aset(a, a2, 1);
	mpdm_aset(a, a3, 2);
	r = mpdm_exec(c, a);
	return r;
}


mpdm_t mpdm_xnew(mpdm_t(*a1) (mpdm_t, mpdm_t), mpdm_t a2)
{
	mpdm_t x;

	x = MPDM_A(2);
	x->flags |= MPDM_EXEC;

	mpdm_aset(x, MPDM_X(a1), 0);
	mpdm_aset(x, a2, 1);

	return x;
}


static mpdm_t MPDM(const mpdm_t args)
/* accesor / mutator for MPDM internal data */
{
	mpdm_t v = mpdm_aget(args, 0);
	int n, c = 0, d = 0;

	if (v != NULL) {
		mpdm_t w;

		/* do changes */
		if ((w = mpdm_hget_s(v, L"low_threshold")) != NULL && mpdm_ival(w) > 0)
			mpdm->low_threshold = mpdm_ival(w);

		if ((w = mpdm_hget_s(v, L"default_sweep")) != NULL)
			mpdm->default_sweep = mpdm_ival(w);

		if ((w = mpdm_hget_s(v, L"hash_buckets")) != NULL)
			mpdm->hash_buckets = mpdm_ival(w);
	}

	/* loop all values counting the unreferenced and deleted ones */
	for (n = mpdm->count, v = mpdm->cur; n > 0; n--, v = v->next) {
		if (v->ref)
			continue;

		if (v->flags & MPDM_DELETED)
			d++;
		else
			c++;
	}

	/* now collect all information */
	v = MPDM_H(0);

	mpdm_hset_s(v, L"version", MPDM_MBS(VERSION));
	mpdm_hset_s(v, L"count", MPDM_I(mpdm->count));
	mpdm_hset_s(v, L"low_threshold", MPDM_I(mpdm->low_threshold));
	mpdm_hset_s(v, L"default_sweep", MPDM_I(mpdm->default_sweep));
	mpdm_hset_s(v, L"memory_usage", MPDM_I(mpdm->memory_usage));
	mpdm_hset_s(v, L"hash_buckets", MPDM_I(mpdm->hash_buckets));
	mpdm_hset_s(v, L"unreferenced", MPDM_I(c));
	mpdm_hset_s(v, L"deleted", MPDM_I(d));

	return v;
}

extern char **environ;

static mpdm_t build_env(void)
/* builds a hash with the environment */
{
	char **ptr;
	mpdm_t e = MPDM_H(0);

	for (ptr = environ; *ptr != NULL; ptr++) {
		char *eq = strchr(*ptr, '=');

		if (eq != NULL) {
			mpdm_t k, v;

			k = MPDM_NMBS((*ptr), eq - (*ptr));
			v = MPDM_MBS(eq + 1);

			mpdm_hset(e, k, v);
		}
	}

	return e;
}


/**
 * mpdm_startup - Initializes MPDM.
 *
 * Initializes the Minimum Profit Data Manager. Returns 0 if
 * everything went OK.
 */
int mpdm_startup(void)
{
	/* do the startup only unless done beforehand */
	if (mpdm == NULL) {
		/* alloc space */
		if ((mpdm = malloc(sizeof(struct mpdm_control))) == NULL)
			return -1;

		/* cleans it */
		memset(mpdm, '\0', sizeof(struct mpdm_control));

		/* sets the defaults */
		mpdm->low_threshold = 16;
		mpdm->default_sweep = -50000;
		mpdm->hash_buckets = 31;

		/* sets the locale */
		if (setlocale(LC_ALL, "") == NULL)
			setlocale(LC_ALL, "C");

		mpdm_encoding(NULL);

		/* store the MPDM() function */
		mpdm_hset_s(mpdm_root(), L"MPDM", MPDM_X(MPDM));

		/* store the ENV hash */
		mpdm_hset_s(mpdm_root(), L"ENV", build_env());
	}

	/* everything went OK */
	return 0;
}


/**
 * mpdm_shutdown - Shuts down MPDM.
 *
 * Shuts down MPDM. No MPDM functions should be used from now on.
 */
void mpdm_shutdown(void)
{
	/* dummy, by now */
}

/**
 * MPDM_A - Creates an array value.
 * @n: Number of elements
 *
 * Creates a new array value with @n elements.
 * [Value Creation]
 */
/** mpdm_t MPDM_A(int n); */
/* ; */

/**
 * MPDM_H - Creates a hash value.
 * @n: Number of buckets in the hash (0: use default)
 *
 * Creates a new hash value with @n buckets. The number
 * of buckets must be a prime number. If @n is 0, an
 * optimal number of buckets will be used.
 * [Value Creation]
 */
/** mpdm_t MPDM_H(int n); */
/* ; */

/**
 * MPDM_LS - Creates a string value from a literal string.
 * @wcs: the wide character string
 *
 * Creates a new string value from a literal, wide character string.
 * A pointer to the string will be stored in the value (not a copy).
 * [Value Creation]
 */
/** mpdm_t MPDM_LS(wchar_t * wcs); */
/* ; */

/**
 * MPDM_S - Creates a string value from a string.
 * @wcs: the wide character string
 *
 * Creates a new string value from a wide character string. The value
 * will store a copy of the string that will be freed on destruction.
 * [Value Creation]
 */
/** mpdm_t MPDM_S(wchar_t * wcs); */
/* ; */

/**
 * MPDM_NS - Creates a string value from a string, with size.
 * @wcs: the wide character string
 * @s: the size in chars the string will hold
 *
 * Creates a new string value with a copy of the first @s characters
 * from the @wcs string.
 * [Value Creation]
 */
/** mpdm_t MPDM_NS(wchar_t * wcs, int s); */
/* ; */

/**
 * MPDM_ENS - Creates a string value from an external string, with size.
 * @wcs: the external wide character string
 * @s: the size in chars the string will hold
 *
 * Creates a new string value with size @s. The @wcs string must be
 * a dynamic value (i.e. allocated by malloc()) that will be freed on
 * destruction.
 * [Value Creation]
 */
/** mpdm_t MPDM_ENS(wchar_t * wcs, int s); */
/* ; */

/**
 * MPDM_I - Creates an integer value.
 * @i: the integer
 *
 * Creates a new integer value. MPDM integers are strings.
 * [Value Creation]
 */
/** mpdm_t MPDM_I(int i); */
/* ; */

/**
 * MPDM_R - Creates a real value.
 * @r: the real number
 *
 * Creates a new real value. MPDM integers are strings.
 * [Value Creation]
 */
/** mpdm_t MPDM_R(double r); */
/* ; */

/**
 * MPDM_F - Creates a file value.
 * @f: the file descriptor
 *
 * Creates a new file value.
 * [Value Creation]
 */
/** mpdm_t MPDM_F(FILE * f); */
/* ; */

/**
 * MPDM_MBS - Creates a string value from a multibyte string.
 * @mbs: the multibyte string
 *
 * Creates a new string value from a multibyte string, that will be
 * converted to wcs by mpdm_mbstowcs().
 * [Value Creation]
 */
/** mpdm_t MPDM_MBS(char * mbs); */
/* ; */

/**
 * MPDM_NMBS - Creates a string value from a multibyte string, with size.
 * @mbs: the multibyte string
 * @s: the size
 *
 * Creates a new string value with the first @s characters from the @mbs
 * multibyte string, that will be converted to wcs by mpdm_mbstowcs().
 * [Value Creation]
 */
/** mpdm_t MPDM_NMBS(char * mbs, int s); */
/* ; */

/**
 * MPDM_2MBS - Creates a multibyte string value from a wide char string.
 * @wcs: the wide char string
 *
 * Creates a multibyte string value from the @wcs wide char string,
 * converting it by mpdm_wcstombs(). Take note that multibyte string values
 * are not properly strings, so they cannot be used for string comparison
 * and such.
 * [Value Creation]
 */
/** mpdm_t MPDM_2MBS(wchar_t * wcs); */
/* ; */

/**
 * MPDM_X - Creates a new executable value.
 * @func: the C code function
 *
 * Creates a new executable value given a pointer to the @func C code function.
 * The function must receive an mpdm_t array value (that will hold their
 * arguments) and return another one.
 * [Value Creation]
 */
/** mpdm_t MPDM_X(mpdm_t (* func)(mpdm_t args)); */
/* ; */

/**
 * MPDM_IS_ARRAY - Tests if a value is an array.
 * @v: the value
 *
 * Returns non-zero if @v is an array.
 */
/** int MPDM_IS_ARRAY(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_HASH - Tests if a value is a hash.
 * @v: the value
 *
 * Returns non-zero if @v is a hash.
 */
/** int MPDM_IS_HASH(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_EXEC - Tests if a value is executable.
 * @v: the value
 *
 * Returns non-zero if @v is executable.
 */
/** int MPDM_IS_EXEC(mpdm_t v); */
/* ; */

/**
 * MPDM_IS_STRING - Tests if a value is a string.
 * @v: the value
 *
 * Returns non-zero if @v is a string.
 */
/** int MPDM_IS_STRING(mpdm_t v); */
/* ; */
