/*

    mpdm - Minimum Profit Data Manager
    Copyright (C) 2003/2005 Angel Ortega <angel@triptico.com>

    mpdm_f.c - File management

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
#include <wchar.h>

#ifdef CONFOPT_UNISTD_H
#include <unistd.h>
#endif

#ifdef CONFOPT_GLOB_H
#include <glob.h>
#endif

#ifdef CONFOPT_WIN32
#include <windows.h>
#include <commctrl.h>
#endif

#include "mpdm.h"

#ifdef CONFOPT_ICONV

#include <iconv.h>
#include <errno.h>

#endif

/* file structure */
struct _mpdm_file
{
	FILE * fd;

#ifdef CONFOPT_ICONV

	iconv_t ic_enc;
	iconv_t ic_dec;
	int has_iconv:1;

#endif /* CONFOPT_ICONV */
};


/*******************
	Code
********************/


wchar_t * _mpdm_read_mbs(FILE * f, int * s)
/* reads a multibyte string from a stream into a dynamic string */
{
	wchar_t * ptr=NULL;

#ifdef CONFOPT_MBRTOWC_
	int c;
	mbstate_t ps;

	*s=0;

	memset(&ps, '\0', sizeof(ps));

	while((c = fgetc(f)) != EOF)
	{
		char cc=c;
		wchar_t wc=L'?';

		/* convert char */
		if(mbrtowc(&wc, &cc, 1, &ps) == -2)
			continue;

		/* alloc space */
		ptr=realloc(ptr, (*s + 2) * sizeof(wchar_t));

		/* store new char and null-terminate */
		ptr[*s]=wc; ptr[*s + 1]=L'\0';
		(*s)++;

		/* if it's an end of line, finish */
		if(wc == L'\n')
			break;
	}

#else /* CONFOPT_MBRTOWC */

	char * cptr=NULL;
	char tmp[MB_CUR_MAX + 1];
	wchar_t wc;
	int c,i,n;

	*s=0;

	for(n=0;(c = fgetc(f)) != EOF;n++)
	{
		/* alloc space */
		cptr=realloc(cptr, n + 2);

		cptr[n]=c; cptr[n + 1]='\0';

		/* if it's an end of line, finish */
		if(c == '\n')
			break;
	}

	/* now loop cptr converting each character */
	for(n=i=0;cptr != NULL && (c = cptr[n + i]) != '\0';)
	{
		tmp[i++]=c; tmp[i]='\0';

		if(mbstowcs(&wc, tmp, i) == -1)
		{
			if(i <= MB_CUR_MAX)
				continue;
			else
			{
				/* too much failing chars; back to 1 */
				wc=L'?';
				i=1;
			}
		}

		n += i;
		i=0;

		/* alloc space */
		ptr=realloc(ptr, (*s + 2) * sizeof(wchar_t));

		/* store new char and null-terminate */
		ptr[*s]=wc; ptr[*s + 1]=L'\0';
		(*s)++;

		/* if it's an end of line, finish */
		if(wc == L'\n')
			break;
	}

	if(cptr != NULL)
		free(cptr);

#endif /* CONFOPT_MBRTOWC */

	return(ptr);
}


void _mpdm_write_wcs(FILE * f, wchar_t * str)
/* writes a wide string to a stream, converting */
{
	char tmp[MB_CUR_MAX + 1];
	int l,n;

	while(*str != L'\0')
	{
		if((l=wctomb(tmp, *str)) <= 0)
		{
			/* if char couldn't be converted,
			   write a question mark instead */
			l=wctomb(tmp, L'?');
		}

		for(n=0;n < l;n++)
			fputc(tmp[n], f);

		str++;
	}
}


#ifdef CONFOPT_ICONV

extern int errno;

wchar_t * _mpdm_read_dec(FILE * f, iconv_t ic, int * s)
/* reads a multibyte string from a stream into a dynamic string, using dec as decoder */
{
	char tmp[128];
	wchar_t * ptr=NULL;
	int c, i;

	*s=i=0;

	/* resets the decoder */
	iconv(ic, NULL, NULL, NULL, NULL);

	while((c = fgetc(f)) != EOF)
	{
		wchar_t wc;
		size_t il, ol;
		char * iptr, * optr;

		tmp[i++]=c;

		/* too big? shouldn't happen */
		if(i == sizeof(tmp))
			break;

		il=i; iptr=tmp;
		ol=sizeof(wchar_t); optr=(char *)&wc;

		/* write to file */
		if(iconv(ic, &iptr, &il, &optr, &ol) == -1)
		{
			/* found incomplete multibyte character */
			if(errno == EINVAL)
				continue;

			/* otherwise, return '?' */
			wc=L'?';
		}

		i=0;

		/* alloc space */
		ptr=realloc(ptr, (*s + 2) * sizeof(wchar_t));

		/* store new char and null-terminate */
		ptr[*s]=wc; ptr[*s + 1]=L'\0';
		(*s)++;

		/* if it's an end of line, finish */
		if(wc == L'\n')
			break;
	}

	return(ptr);
}


void _mpdm_write_enc(FILE * f, iconv_t ic, wchar_t * str)
/* writes a wide string to a stream, using enc as encoder */
{
	char tmp[128];

	/* resets the encoder */
	iconv(ic, NULL, NULL, NULL, NULL);

	/* convert char by char */
	for(;*str != L'\0';str++)
	{
		size_t il, ol;
		char * iptr, * optr;

		il=sizeof(wchar_t); iptr=(char *)str;
		ol=sizeof(tmp); optr=tmp;

		/* write to file */
		if(iconv(ic, &iptr, &il, &optr, &ol) == -1)
		{
			/* error converting; convert a '?' instead */
			wchar_t q=L'?';

			il=sizeof(wchar_t); iptr=(char *)&q;
			ol=sizeof(tmp); optr=tmp;

			iconv(ic, &iptr, &il, &optr, &ol);
		}

		fwrite(tmp, 1, sizeof(tmp) - ol, f);
	}
}

#endif /* CONFOPT_ICONV */


/**
 * mpdm_open - Opens a file.
 * @filename: the file name
 * @mode: an fopen-like mode string
 *
 * Opens a file. If @filename can be open in the specified @mode, an
 * mpdm_v value will be returned containing the file descriptor, or NULL
 * otherwise.
 */
mpdm_v mpdm_open(mpdm_v filename, mpdm_v mode)
{
	FILE * f;
	struct _mpdm_file * fs;

	/* convert to mbs,s */
	filename=MPDM_2MBS(filename->data);
	mode=MPDM_2MBS(mode->data);

	if((f=fopen((char *)filename->data, (char *)mode->data)) == NULL)
		return(NULL);

	if((fs=malloc(sizeof(struct _mpdm_file))) == NULL)
	{
		fclose(f);
		return(NULL);
	}

	memset(fs, '\0', sizeof(struct _mpdm_file));

	fs->fd=f;

#ifdef CONFOPT_ICONV

	if(_mpdm->encoding)
	{
		mpdm_v cs=MPDM_2MBS(_mpdm->encoding->data);

		fs->ic_enc=iconv_open((char *)cs->data, "WCHAR_T");
		fs->ic_dec=iconv_open("WCHAR_T", (char *)cs->data);

		fs->has_iconv=1;
	}

#endif

	return(mpdm_new(MPDM_FILE|MPDM_FREE, fs,
		sizeof(struct _mpdm_file)));
}


/**
 * mpdm_close - Closes a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Closes the file descriptor.
 */
mpdm_v mpdm_close(mpdm_v fd)
{
	struct _mpdm_file * fs=fd->data;

	if((fd->flags & MPDM_FILE) && fs != NULL)
	{
		if(fs->fd != NULL)
		{
			fclose(fs->fd);
			fs->fd=NULL;
		}

#ifdef CONFOPT_ICONV

		if(fs->has_iconv)
		{
			iconv_close(fs->ic_enc);
			iconv_close(fs->ic_dec);
			fs->has_iconv=0;
		}

#endif
	}

	return(NULL);
}


/**
 * mpdm_read - Reads a line from a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Reads a line from @fd. Returns the line, or NULL on EOF.
 */
mpdm_v mpdm_read(mpdm_v fd)
{
	mpdm_v v=NULL;
	wchar_t * ptr;
	int s;
	struct _mpdm_file * fs=fd->data;

	if(fs == NULL)
		return(NULL);

#ifdef CONFOPT_ICONV

	if(fs->has_iconv)
		ptr=_mpdm_read_dec(fs->fd, fs->ic_dec, &s);
	else

#endif /* CONFOPT_ICONV */

		ptr=_mpdm_read_mbs(fs->fd, &s);

	if(ptr != NULL)
		v=mpdm_new(MPDM_STRING|MPDM_FREE, ptr, s);

	return(v);
}


/**
 * mpdm_write - Writes a value into a file.
 * @fd: the file descriptor.
 * @v: the value to be written.
 *
 * Writes the @v string value into @fd, using the current encoding.
 */
int mpdm_write(mpdm_v fd, mpdm_v v)
{
	struct _mpdm_file * fs=fd->data;

	if(fs == NULL)
		return(-1);

#ifdef CONFOPT_ICONV

	if(fs->has_iconv)
		_mpdm_write_enc(fs->fd, fs->ic_enc, mpdm_string(v));
	else

#endif /* CONFOPT_ICONV */

		_mpdm_write_wcs(fs->fd, mpdm_string(v));

	return(0);
}


/*
mpdm_v mpdm_bread(mpdm_v fd, int size)
{
}


int mpdm_bwrite(mpdm_vfd, mpdm_v v, int size)
{
}
*/


/**
 * mpdm_encoding - Sets the current charset encoding for files.
 * @charset: the charset name.
 *
 * Sets the current charset encoding for files. Future opened
 * files will be assumed to be encoded with @charset, which can
 * be any of the supported charset names (utf-8, iso-8859-1, etc.),
 * and converted on each read / write. If charset is NULL, it
 * is reverted to default charset conversion (i.e. the one defined
 * in the locale).
 * Returns a negative number if @charset is unsupported, or zero
 * if no errors were found.
 */
int mpdm_encoding(mpdm_v charset)
{
	int ret = -1;

#ifdef CONFOPT_ICONV

	iconv_t ic;
	mpdm_v cs=MPDM_2MBS(charset->data);

	/* tries to create an encoder and a decoder for this charset */

	if((ic=iconv_open("WCHAR_T", (char *)cs->data)) == (iconv_t) -1)
		return(-1);

	iconv_close(ic);

	if((ic=iconv_open((char *)cs->data, "WCHAR_T")) == (iconv_t) -1)
		return(-2);

	iconv_close(ic);

	/* can create; store and exit */

	mpdm_unref(_mpdm->encoding);
	_mpdm->encoding=mpdm_ref(charset);

	ret=0;

#endif

	return(ret);
}


/**
 * mpdm_unlink - Deletes a file.
 * @filename: file name to be deleted
 *
 * Deletes a file.
 */
int mpdm_unlink(mpdm_v filename)
{
	/* convert to mbs */
	filename=MPDM_2MBS(filename->data);

	return(unlink((char *)filename->data));
}


/**
 * mpdm_glob - Executes a file globbing.
 * @spec: Globbing spec
 *
 * Executes a file globbing. @spec is system-dependent, but usually
 * the * and ? metacharacters work everywhere. @spec can contain a
 * directory; if that's the case, the output strings will include it.
 * In any case, each returned value will be suitable for a call to
 * mpdm_open().
 *
 * Returns an array of files that match the globbing (can be an empty
 * array if no file matches), or NULL if globbing is unsupported.
 */
mpdm_v mpdm_glob(mpdm_v spec)
{
	mpdm_v v=NULL;

#ifdef CONFOPT_WIN32

	WIN32_FIND_DATA f;
	HANDLE h;
	char * ptr;
	mpdm_v w;
	mpdm_v s=NULL;

	/* convert to mbs */
	if(spec != NULL)
		spec=MPDM_2MBS(spec->data);
	else
		spec=MPDM_2MBS(L"*.*");

	ptr=(char *)spec->data;

	/* convert MSDOS dir separators into Unix ones */
	for(;*ptr != '\0';ptr++)
	{
		if(*ptr == '\\')
			*ptr='/';
	}

	v=MPDM_A(0);

	if((h=FindFirstFile((char *)spec->data, &f)) != INVALID_HANDLE_VALUE)
	{
		/* if spec includes a directory, store in s */
		if((ptr=strrchr((char *)spec->data, '/')) != NULL)
		{
			*(ptr+1)='\0';
			s=MPDM_S(spec->data);
		}

		do
		{
			/* ignore . and .. */
			if(strcmp(f.cFileName,".") == 0 ||
			   strcmp(f.cFileName,"..") == 0)
				continue;

			/* concat base directory and file names */
			w=mpdm_strcat(s, MPDM_MBS(f.cFileName));

			/* if it's a directory, add a / */
			if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				w=mpdm_strcat(w, MPDM_LS(L"/"));

			mpdm_apush(v, w);
		}
		while(FindNextFile(h,&f));

		FindClose(h);
	}

#endif

#if CONFOPT_GLOB_H

	/* glob.h support */
	int n;
	glob_t globbuf;
	char * ptr;

	/* convert to mbs */
	if(spec != NULL)
	{
		spec=MPDM_2MBS(spec->data);

		ptr=spec->data;
		if(ptr == NULL || *ptr == '\0')
			ptr="*";
	}
	else
		ptr="*";

	globbuf.gl_offs=1;
	v=MPDM_A(0);

	if(glob(ptr, GLOB_MARK, NULL, &globbuf) == 0)
	{
		for(n=0;globbuf.gl_pathv[n]!=NULL;n++)
			mpdm_apush(v, MPDM_MBS(globbuf.gl_pathv[n]));
	}

	globfree(&globbuf);

#else

	/* no win32 nor glob.h; try workaround */
	/* ... */

#endif

	return(v);
}
