/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

    mpsl.h - Minimum Profit Scripting Language

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

int mpsl_is_true(mpdm_v v);
mpdm_v mpsl_boolean(int b);

mpdm_v mpsl_set_symbol(mpdm_v s, mpdm_v v);
mpdm_v mpsl_get_symbol(mpdm_v s);

mpdm_v _mpsl_exec_i(mpdm_v c, mpdm_v args, int * f);
mpdm_v _mpsl_exec(mpdm_v c, mpdm_v args);

mpdm_v mpsl_compile(mpdm_v code);
