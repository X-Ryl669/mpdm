/*

    fdm - Filp Data Manager
    Copyright (C) 2003/2004 Angel Ortega <angel@triptico.com>

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

/* structural flags */
#define FDM_COPY	0x00000001	/* create a copy of data */
#define FDM_STRING	0x00000002	/* data can be string-compared */
#define FDM_MULTIPLE	0x00000004	/* data is multiple */
#define FDM_FREE	0x00000008	/* data must be freed on destroy */

#define FDM_IVAL	0x00000010	/* integer value cached in .ival */

/* 'informative' flags */
#define FDM_HASH	0x00010000	/* data is a hash */
#define FDM_FILE	0x00020000	/* data is a FILE * */
#define FDM_BINCODE	0x00040000	/* data is executable binary code */

#define FDM_A(n)	fdm_new(FDM_MULTIPLE,NULL,n)
#define FDM_H(n)	fdm_new(FDM_MULTIPLE|FDM_HASH,NULL,n)
#define FDM_LS(s)	fdm_new(FDM_STRING,s,-1)
#define FDM_S(s)	fdm_new(FDM_STRING|FDM_COPY,s,-1)
#define FDM_I(i)	_fdm_inew((i))

typedef struct _fdm_v * fdm_v;

struct _fdm_v
{
	int flags;	/* value flags */
	int ref;	/* reference count */
	int size;	/* data size */
	void * data;	/* the real data */
	int ival;	/* cached integer value */
	fdm_v next;	/* next in chain */
};

fdm_v fdm_new(int flags, void * data, int size);
int fdm_ref(fdm_v v);
int fdm_unref(fdm_v v);
void fdm_sweep(int count);

fdm_v fdm_clone(fdm_v v);
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
int fdm_abseek(fdm_v a, fdm_v k, int step, int * pos);
void fdm_asort(fdm_v a, int step);

fdm_v fdm_asplit(fdm_v s, fdm_v a);
fdm_v fdm_ajoin(fdm_v s, fdm_v a);

char * fdm_string(fdm_v v);
fdm_v fdm_splice(fdm_v v, fdm_v i, int offset, int del);
fdm_v fdm_strcat(fdm_v s1, fdm_v s2);
int fdm_cmp(fdm_v v1, fdm_v v2);
int fdm_ival(fdm_v v);
fdm_v _fdm_inew(int ival);

fdm_v fdm_hget(fdm_v h, fdm_v k);
fdm_v fdm_hset(fdm_v h, fdm_v k, fdm_v v);
fdm_v fdm_hdel(fdm_v h, fdm_v k);
fdm_v fdm_hkeys(fdm_v h);

void fdm_dump(fdm_v v);

#define FDM_SGET(r, k) fdm_sget((r), FDM_LS((k)))
#define FDM_SSET(r, k, v) fdm_sset((r), FDM_LS((k)), (v))

fdm_v fdm_sget(fdm_v r, fdm_v k);
fdm_v fdm_sset(fdm_v r, fdm_v k, fdm_v v);
