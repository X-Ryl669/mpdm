/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

    mpdm_x.c - Extended functions

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

    http://triptico.com

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "mpdm.h"

/**
 * mpdm_exec - Executes an executable value.
 * @c: the code value
 * @args: the arguments
 * @ctxt: the context
 *
 * Executes an executable value. If @c is a scalar value, its data
 * should be a pointer to a directly executable C function with a
 * prototype of mpdm_t func(mpdm_t args, mpdm_t ctxt); if it's a multiple
 * one, the first value's data should be a pointer to a directly executable
 * C function with a prototype of
 * mpdm_t func(mpdm_t b, mpdm_t args, mpdm_t ctxt) and
 * the second value will be passed as the @b argument. This value is used
 * to store bytecode or so when implementing virtual machines or compilers.
 * The @ctxt is meant to be used as a special context to implement local
 * symbol tables and such. Its meaning is free and can be NULL.
 *
 * Returns the return value of the code. If @c is NULL or not executable,
 * returns NULL.
 * [Value Management]
 */
mpdm_t mpdm_exec(mpdm_t c, mpdm_t args, mpdm_t ctxt)
{
    mpdm_t r = NULL;

    mpdm_ref(c);
    mpdm_ref(args);
    mpdm_ref(ctxt);

    if (c != NULL && (c->flags & MPDM_EXEC)) {
        if (c->flags & MPDM_MULTIPLE) {
            mpdm_t x;
            mpdm_t(*func) (mpdm_t, mpdm_t, mpdm_t);

            /* value is multiple; first element is the
               3 argument version of the executable function,
               next its optional additional information,
               the arguments and the context */
            x = mpdm_aget(c, 0);

            if ((func = (mpdm_t(*)(mpdm_t, mpdm_t, mpdm_t)) (x->data)) != NULL)
                r = func(mpdm_aget(c, 1), args, ctxt);
        }
        else {
            mpdm_t(*func) (mpdm_t, mpdm_t);

            /* value is scalar; c is the 2 argument
               version of the executable function */
            if ((func = (mpdm_t(*)(mpdm_t, mpdm_t)) (c->data)) != NULL)
                r = func(args, ctxt);
        }
    }

    mpdm_ref(r);

    mpdm_unref(ctxt);
    mpdm_unref(args);
    mpdm_unref(c);

    return mpdm_unrefnd(r);
}


mpdm_t mpdm_exec_1(mpdm_t c, mpdm_t a1, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(1);

    mpdm_aset(a, a1, 0);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(2);

    mpdm_aset(a, a1, 0);
    mpdm_aset(a, a2, 1);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(3);

    mpdm_aset(a, a1, 0);
    mpdm_aset(a, a2, 1);
    mpdm_aset(a, a3, 2);

    return mpdm_exec(c, a, ctxt);
}

