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
#define FDM_INTEGER	0x00080000	/* integer value cached in .ival */

#define FDM_FLAGS_MASK  0xffff0000
#define FDM_FLAGS(t)	((t) & FDM_FLAGS_MASK)
#define FDM_TYPE(t)	((t) & ~ FDM_FLAGS_MASK)

#define FDM_A(n)	fdm_new(FDM_MULTIPLE,NULL,n)
#define FDM_H(n)	fdm_new(FDM_MULTIPLE,NULL,n)
#define FDM_LS(s)	fdm_new(FDM_STRING,s,-1)
#define FDM_S(s)	fdm_new(FDM_STRING|FDM_COPY,s,-1)
#define FDM_I(i)	fdm_new(FDM_INTEGER,(void *)i,-1)

typedef struct _fdm_v * fdm_v;

struct _fdm_v
{
	int tag;	/* value flags and type */
	int ref;	/* reference count */
	int size;	/* data size */
	void * data;	/* the real data */
	int ival;	/* cached integer value */
	fdm_v next;	/* next in chain */
};

fdm_v fdm_new(int tag, void * data, int size);
int fdm_ref(fdm_v v);
int fdm_unref(fdm_v v);
void fdm_sweep(int count);
int fdm_cmp(fdm_v v1, fdm_v v2);
int fdm_ival(fdm_v v);

void fdm_poke(fdm_v v, char c, int offset);

fdm_v fdm_copy(fdm_v v);
fdm_v fdm_root(void);

void fdm_aexpand(fdm_v a, int offset, int num);
void fdm_acollapse(fdm_v a, int offset, int num);
fdm_v fdm_aset(fdm_v a, fdm_v e, int offset);
fdm_v fdm_aget(fdm_v a, int offset);
void fdm_ains(fdm_v a, fdm_v e, int offset);
fdm_v fdm_adel(fdm_v a, int offset);
void fdm_apush(fdm_v a, fdm_v e);
fdm_v fdm_apop(fdm_v a);
fdm_v fdm_aqueue(fdm_v a, fdm_v e, int size);
int fdm_aseek(fdm_v a, fdm_v k, int step);
int fdm_abseek(fdm_v a, fdm_v k, int step);
void fdm_asort(fdm_v a, int step);

fdm_v fdm_splice(fdm_v v, fdm_v i, int offset, int del);
fdm_v fdm_ajoin(fdm_v s, fdm_v a);

fdm_v fdm_hget(fdm_v h, fdm_v k);
fdm_v fdm_hset(fdm_v h, fdm_v k, fdm_v v);
fdm_v fdm_hdel(fdm_v h, fdm_v k);
fdm_v fdm_hkeys(fdm_v h);

void fdm_dump(fdm_v v, int l);
