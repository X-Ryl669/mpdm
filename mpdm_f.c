/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2006 Angel Ortega <angel@triptico.com>

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
#include <shlobj.h>

#endif

#ifdef CONFOPT_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef CONFOPT_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef CONFOPT_PWD_H
#include <pwd.h>
#endif

#include "mpdm.h"

#ifdef CONFOPT_ICONV

#include <iconv.h>
#include <errno.h>

#endif

/* file structure */
struct mpdm_file
{
	FILE * in;
	FILE * out;

#ifdef CONFOPT_ICONV

	iconv_t ic_enc;
	iconv_t ic_dec;

#endif /* CONFOPT_ICONV */

#ifdef CONFOPT_WIN32

	HANDLE hin;
	HANDLE hout;

#endif /* CONFOPT_WIN32 */
};


/*******************
	Code
********************/


static int get_char(struct mpdm_file * f)
/* reads a character from a file structure */
{
	int c = EOF;

#ifdef CONFOPT_WIN32

	if(f->hin != NULL)
	{
		char tmp;
		DWORD n;

		if(ReadFile(f->hin, &tmp, 1, &n, NULL) && n > 0)
			c = (int) tmp;
	}

#endif /* CONFOPT_WIN32 */

	if(f->in != NULL)
		c = fgetc(f->in);

	return(c);
}


static int put_char(int c, struct mpdm_file * f)
/* writes a character in a file structure */
{
	int ret = EOF;

#ifdef CONFOPT_WIN32

	if(f->hout != NULL)
	{
		char tmp = c;
		DWORD n;

		if(WriteFile(f->hout, &tmp, 1, &n, NULL) && n > 0)
			ret = c;
	}

#endif /* CONFOPT_WIN32 */

	if(f->out != NULL)
		ret = fputc(c, f->out);

	return(ret);
}


static wchar_t * read_mbs(struct mpdm_file * f, int * s)
/* reads a multibyte string from a mpdm_file into a dynamic string */
{
	wchar_t * ptr = NULL;
	char * cptr = NULL;
	char tmp[2];
	int c, n = 0;

	tmp[1] = '\0';

	while((c = get_char(f)) != EOF)
	{
		tmp[0] = c;
		if((cptr = mpdm_poke(cptr, &n, tmp, 1, sizeof(char))) == NULL)
			break;

		/* if it's an end of line, finish */
		if(c == '\n')
			break;
	}

	if(cptr != NULL)
	{
		/* do the conversion */
		ptr = mpdm_mbstowcs(cptr, s, -1);

		free(cptr);
	}

	return(ptr);
}


static int write_wcs(struct mpdm_file * f, wchar_t * str)
/* writes a wide string to an struct mpdm_file */
{
	int s, n;
	char * ptr;

	ptr = mpdm_wcstombs(str, &s);

	for(n = 0;n < s;n++)
	{
		if(put_char(ptr[n], f) == EOF)
		{
			n = -1;
			break;
		}
	}

	free(ptr);

	return(n);
}


#ifdef CONFOPT_ICONV

extern int errno;

static wchar_t * read_iconv(struct mpdm_file * f, int * s)
/* reads a multibyte string transforming with iconv */
{
	char tmp[128];
	wchar_t * ptr = NULL;
	int c, i;
	wchar_t wc[2];

	*s = i = 0;
	wc[1] = L'\0';

	/* resets the decoder */
	iconv(f->ic_dec, NULL, NULL, NULL, NULL);

	while((c = get_char(f)) != EOF)
	{
		size_t il, ol;
		char * iptr, * optr;

		tmp[i++] = c;

		/* too big? shouldn't happen */
		if(i == sizeof(tmp))
			break;

		il = i; iptr = tmp;
		ol = sizeof(wchar_t); optr = (char *)wc;

		/* write to file */
		if(iconv(f->ic_dec, &iptr, &il, &optr, &ol) == -1)
		{
			/* found incomplete multibyte character */
			if(errno == EINVAL)
				continue;

			/* otherwise, return '?' */
			wc[0] = L'?';
		}

		i = 0;

		if((ptr = mpdm_poke(ptr, s, wc, 1, sizeof(wchar_t))) == NULL)
			break;

		/* if it's an end of line, finish */
		if(wc[0] == L'\n')
			break;
	}

	return(ptr);
}


static int write_iconv(struct mpdm_file * f, wchar_t * str)
/* writes a wide string to a stream using iconv */
{
	char tmp[128];
	int cnt = 0;

	/* resets the encoder */
	iconv(f->ic_enc, NULL, NULL, NULL, NULL);

	/* convert char by char */
	for(;*str != L'\0';str++)
	{
		size_t il, ol;
		char * iptr, * optr;
		int n;

		il = sizeof(wchar_t); iptr = (char *)str;
		ol = sizeof(tmp); optr = tmp;

		/* write to file */
		if(iconv(f->ic_enc, &iptr, &il, &optr, &ol) == -1)
		{
			/* error converting; convert a '?' instead */
			wchar_t q = L'?';

			il = sizeof(wchar_t); iptr = (char *)&q;
			ol = sizeof(tmp); optr = tmp;

			iconv(f->ic_enc, &iptr, &il, &optr, &ol);
		}

		for(n = 0;n < sizeof(tmp) - ol;n++, cnt++)
		{
			if(put_char(tmp[n], f) == EOF)
				return(-1);
		}
	}

	return(cnt);
}

#endif /* CONFOPT_ICONV */


static mpdm_t new_mpdm_file(void)
/* creates a new file value */
{
	mpdm_t v = NULL;
	struct mpdm_file * fs;

	if((fs = malloc(sizeof(struct mpdm_file))) == NULL)
		return(NULL);

	memset(fs, '\0', sizeof(struct mpdm_file));

#ifdef CONFOPT_ICONV

	if((v = mpdm_hget_s(mpdm_root(), L"ENCODING")) != NULL)
	{
		mpdm_t cs = MPDM_2MBS(v->data);

		fs->ic_enc = iconv_open((char *)cs->data, "WCHAR_T");
		fs->ic_dec = iconv_open("WCHAR_T", (char *)cs->data);
	}
	else
		fs->ic_enc = fs->ic_dec = (iconv_t) -1;

#endif

	if((v = mpdm_new(MPDM_FILE|MPDM_FREE, fs,
		sizeof(struct mpdm_file))) == NULL)
		free(fs);

	return(v);
}


static void destroy_mpdm_file(mpdm_t v)
/* destroys and file value */
{
	struct mpdm_file * fs = v->data;

	if(fs != NULL)
	{
#ifdef CONFOPT_ICONV

		if(fs->ic_enc != (iconv_t) -1)
		{
			iconv_close(fs->ic_enc);
			fs->ic_enc = (iconv_t) -1;
		}

		if(fs->ic_dec != (iconv_t) -1)
		{
			iconv_close(fs->ic_dec);
			fs->ic_dec = (iconv_t) -1;
		}
#endif

		free(fs);
		v->data = NULL;
	}
}


/** interface **/

wchar_t * mpdm_read_mbs(FILE * f, int * s)
/* reads a multibyte string from a stream into a dynamic string */
{
	struct mpdm_file fs;

	/* reset the structure */
	memset(&fs, '\0', sizeof(fs));
	fs.in = f;

	return(read_mbs(&fs, s));
}


int mpdm_write_wcs(FILE * f, wchar_t * str)
/* writes a wide string to a stream */
{
	struct mpdm_file fs;

	/* reset the structure */
	memset(&fs, '\0', sizeof(fs));
	fs.out = f;

	return(write_wcs(&fs, str));
}


mpdm_t mpdm_new_f(FILE * f)
/* creates a new file value from a FILE * */
{
	mpdm_t v = NULL;

	if(f == NULL) return(NULL);

	if((v = new_mpdm_file()) != NULL)
	{
		struct mpdm_file * fs = v->data;
		fs->in = fs->out = f;
	}

	return(v);
}


/**
 * mpdm_open - Opens a file.
 * @filename: the file name
 * @mode: an fopen-like mode string
 *
 * Opens a file. If @filename can be open in the specified @mode, an
 * mpdm_t value will be returned containing the file descriptor, or NULL
 * otherwise.
 * [File Management]
 */
mpdm_t mpdm_open(mpdm_t filename, mpdm_t mode)
{
	FILE * f;

	if(filename == NULL || mode == NULL)
		return(NULL);

	/* convert to mbs,s */
	filename = MPDM_2MBS(filename->data);
	mode = MPDM_2MBS(mode->data);

	f = fopen((char *)filename->data, (char *)mode->data);

	return(MPDM_F(f));
}


/**
 * mpdm_close - Closes a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Closes the file descriptor.
 * [File Management]
 */
mpdm_t mpdm_close(mpdm_t fd)
{
	struct mpdm_file * fs = fd->data;

	if((fd->flags & MPDM_FILE) && fs != NULL)
	{
		if(fs->in != NULL)
			fclose(fs->in);

		if(fs->out != fs->in && fs->out != NULL)
			fclose(fs->out);

		destroy_mpdm_file(fd);
	}

	return(NULL);
}


/**
 * mpdm_read - Reads a line from a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Reads a line from @fd. Returns the line, or NULL on EOF.
 * [File Management]
 * [Character Set Conversion]
 */
mpdm_t mpdm_read(mpdm_t fd)
{
	mpdm_t v = NULL;
	wchar_t * ptr;
	int s;
	struct mpdm_file * fs = fd->data;

	if(fs == NULL)
		return(NULL);

#ifdef CONFOPT_ICONV

	if(fs->ic_dec != (iconv_t) -1)
		ptr = read_iconv(fs, &s);
	else

#endif /* CONFOPT_ICONV */

		ptr = read_mbs(fs, &s);

	if(ptr != NULL)
		v = MPDM_ENS(ptr, s);

	return(v);
}


/**
 * mpdm_write - Writes a value into a file.
 * @fd: the file descriptor.
 * @v: the value to be written.
 *
 * Writes the @v string value into @fd, using the current encoding.
 * [File Management]
 * [Character Set Conversion]
 */
int mpdm_write(mpdm_t fd, mpdm_t v)
{
	struct mpdm_file * fs = fd->data;
	int ret = -1;

	if(fs == NULL)
		return(-1);

#ifdef CONFOPT_ICONV

	if(fs->ic_enc != (iconv_t) -1)
		ret = write_iconv(fs, mpdm_string(v));
	else

#endif /* CONFOPT_ICONV */

		ret = write_wcs(fs, mpdm_string(v));

	return(ret);
}


/*
mpdm_t mpdm_bread(mpdm_t fd, int size)
{
}


int mpdm_bwrite(mpdm_tfd, mpdm_t v, int size)
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
 * [File Management]
 * [Character Set Conversion]
 */
int mpdm_encoding(mpdm_t charset)
{
	int ret = -1;

#ifdef CONFOPT_ICONV

	if(charset != NULL)
	{
		iconv_t ic;
		mpdm_t cs = MPDM_2MBS(charset->data);

		/* tries to create an encoder and a decoder for this charset */

		if((ic = iconv_open("WCHAR_T", (char *)cs->data)) == (iconv_t) -1)
			return(-1);

		iconv_close(ic);

		if((ic = iconv_open((char *)cs->data, "WCHAR_T")) == (iconv_t) -1)
			return(-2);

		iconv_close(ic);
	}

	/* can create; store and exit */

	mpdm_hset_s(mpdm_root(), L"ENCODING", charset);

	ret = 0;

#endif

	return(ret);
}


/**
 * mpdm_unlink - Deletes a file.
 * @filename: file name to be deleted
 *
 * Deletes a file.
 * [File Management]
 */
int mpdm_unlink(mpdm_t filename)
{
	/* convert to mbs */
	filename = MPDM_2MBS(filename->data);

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
 * [File Management]
 */
mpdm_t mpdm_glob(mpdm_t spec)
{
	mpdm_t v = NULL;

#ifdef CONFOPT_WIN32

	WIN32_FIND_DATA f;
	HANDLE h;
	char * ptr;
	mpdm_t w;
	mpdm_t s = NULL;

	/* convert to mbs */
	if(spec != NULL)
		spec = MPDM_2MBS(spec->data);
	else
		spec = MPDM_2MBS(L"*.*");

	ptr = (char *)spec->data;

	/* convert MSDOS dir separators into Unix ones */
	for(;*ptr != '\0';ptr++)
	{
		if(*ptr == '\\')
			*ptr = '/';
	}

	v = MPDM_A(0);

	if((h = FindFirstFile((char *)spec->data, &f)) != INVALID_HANDLE_VALUE)
	{
		/* if spec includes a directory, store in s */
		if((ptr = strrchr((char *)spec->data, '/')) != NULL)
		{
			*(ptr + 1) = '\0';
			s = MPDM_S(spec->data);
		}

		do
		{
			/* ignore . and .. */
			if(strcmp(f.cFileName,".") == 0 ||
			   strcmp(f.cFileName,"..") == 0)
				continue;

			/* concat base directory and file names */
			w = mpdm_strcat(s, MPDM_MBS(f.cFileName));

			/* if it's a directory, add a / */
			if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				w = mpdm_strcat(w, MPDM_LS(L"/"));

			mpdm_push(v, w);
		}
		while(FindNextFile(h, &f));

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
		spec = MPDM_2MBS(spec->data);

		ptr = spec->data;
		if(ptr == NULL || *ptr == '\0')
			ptr = "*";
	}
	else
		ptr = "*";

	globbuf.gl_offs = 1;
	v = MPDM_A(0);

	if(glob(ptr, GLOB_MARK, NULL, &globbuf) == 0)
	{
		for(n = 0;globbuf.gl_pathv[n] != NULL;n++)
			mpdm_push(v, MPDM_MBS(globbuf.gl_pathv[n]));
	}

	globfree(&globbuf);

#else

	/* no win32 nor glob.h; try workaround */
	/* ... */

#endif

	return(v);
}


#ifdef CONFOPT_WIN32

static void win32_pipe(HANDLE * h, int n)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE cp, t;

	memset(&sa, '\0', sizeof(sa));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	cp = GetCurrentProcess();

	CreatePipe(&h[0], &h[1], &sa, 0);
	DuplicateHandle(cp, h[n], cp, &t, 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(h[n]);
	h[n] = t;
}


static int sysdep_popen(mpdm_t v, char * prg, int rw)
/* win32-style pipe */
{
	HANDLE pr[2]; HANDLE pw[2];
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	int ret;
	struct mpdm_file * fs = v->data;

	/* init all */
	pr[0] = pr[1] = pw[0] = pw[1] = NULL;

	if(rw & 0x01) win32_pipe(pr, 0);
	if(rw & 0x02) win32_pipe(pw, 1);

	/* spawn new process */
	memset(&pi, '\0', sizeof(pi));
	memset(&si, '\0', sizeof(si));

	si.cb = sizeof(STARTUPINFO);
	si.hStdError = pr[1];
	si.hStdOutput = pr[1];
	si.hStdInput = pw[0];
	si.dwFlags |= STARTF_USESTDHANDLES;

	ret = CreateProcess(NULL, prg, NULL, NULL, TRUE,
			0, NULL, NULL, &si, &pi);

	if(rw & 0x01) CloseHandle(pr[1]);
	if(rw & 0x02) CloseHandle(pw[0]);

	fs->hin = pr[0];
	fs->hout = pw[1];

	return(ret);
}


static int sysdep_pclose(mpdm_t v)
{
	struct mpdm_file * fs = v->data;

	if(fs->hin != NULL)
		CloseHandle(fs->hin);

	if(fs->hout != NULL)
		CloseHandle(fs->hout);

	/* how to know process exit code? */
	return(0);
}


#else /* CONFOPT_WIN32 */

static int sysdep_popen(mpdm_t v, char * prg, int rw)
/* unix-style pipe open */
{
	int pr[2], pw[2];
	struct mpdm_file * fs = v->data;

	/* init all */
	pr[0] = pr[1] = pw[0] = pw[1] = -1;

	if(rw & 0x01) pipe(pr);
	if(rw & 0x02) pipe(pw);

	if(fork() == 0)
	{
		/* child process */
		if(rw & 0x01) { close(1); dup(pr[1]); close(pr[0]); }
		if(rw & 0x02) { close(0); dup(pw[0]); close(pw[1]); }

		/* redirect stderr to stdout */
		close(2); dup(1);

		/* run the program */
		execlp("/bin/sh", "/bin/sh", "-c", prg, NULL);
		execlp(prg, prg, NULL);

		/* still here? exec failed; close pipes and exit */
		close(0); close(1);
		exit(0);
	}

	/* create the pipes as non-buffered streams */
	if(rw & 0x01) { fs->in = fdopen(pr[0], "r");
		setvbuf(fs->in, NULL, _IONBF, 0); close(pr[1]); }

	if(rw & 0x02) { fs->out = fdopen(pw[1], "w");
		setvbuf(fs->out, NULL, _IONBF, 0); close(pw[0]); }

	return(1);
}


static int sysdep_pclose(mpdm_t v)
/* unix-style pipe close */
{
	int s;
	struct mpdm_file * fs = v->data;

	if(fs->in != NULL)
		fclose(fs->in);

	if(fs->out != fs->in && fs->out != NULL)
		fclose(fs->out);

	wait(&s);

	return(s);
}


#endif /* CONFOPT_WIN32 */


/**
 * mpdm_popen - Opens a pipe.
 * @prg: the program to pipe
 * @mode: an fopen-like mode string
 *
 * Opens a pipe to a program. If @prg can be open in the specified @mode, an
 * mpdm_t value will be returned containing the file descriptor, or NULL
 * otherwise.
 * [File Management]
 */
mpdm_t mpdm_popen(mpdm_t prg, mpdm_t mode)
{
	mpdm_t v;
	char * m;
	int rw = 0;

	if(prg == NULL || mode == NULL)
		return(NULL);

	if((v = new_mpdm_file()) == NULL)
		return(NULL);

	/* convert to mbs,s */
	prg = MPDM_2MBS(prg->data);
	mode = MPDM_2MBS(mode->data);

	/* get the mode */
	m = (char *)mode->data;

	/* set the mode */
	if(m[0] == 'r') rw = 0x01;
	if(m[0] == 'w') rw = 0x02;
	if(m[1] == '+') rw = 0x03; /* r+ or w+ */

	if(!sysdep_popen(v, (char *)prg->data, rw))
	{
		destroy_mpdm_file(v);
		v = NULL;
	}

	return(v);
}


/**
 * mpdm_pclose - Closes a pipe.
 * @fd: the value containing the file descriptor
 *
 * Closes a pipe.
 * [File Management]
 */
mpdm_t mpdm_pclose(mpdm_t fd)
{
	mpdm_t r = NULL;

	if((fd->flags & MPDM_FILE) && fd->data != NULL)
	{
		r = MPDM_I(sysdep_pclose(fd));
		destroy_mpdm_file(fd);
	}

	return(r);
}


/**
 * mpdm_home_dir - Returns the home user directory.
 *
 * Returns a system-dependent directory where the user can write
 * documents and create subdirectories.
 * [File Management]
 */
mpdm_t mpdm_home_dir(void)
{
	mpdm_t r = NULL;
	char * ptr;
	char tmp[512] = "";

#ifdef CONFOPT_WIN32

	LPITEMIDLIST pidl;

	/* get the 'My Documents' folder */
	SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
	SHGetPathFromIDList(pidl, tmp);
	strcat(tmp, "\\");

#endif

#ifdef CONFOPT_PWD_H

	struct passwd * p;

	/* get home dir from /etc/passwd entry */
	if(tmp[0] == '\0' && (p = getpwuid(getpid())) != NULL)
	{
		strcpy(tmp, p->pw_dir);
		strcat(tmp, "/");
	}

#endif

	/* still none? try the ENV variable $HOME */
	if(tmp[0] == '\0' && (ptr = getenv("HOME")) != NULL)
	{
		strcpy(tmp, ptr);
		strcat(tmp, "/");
	}

	if(tmp[0] != '\0')
		r = MPDM_MBS(tmp);

	return(r);
}


/**
 * mpdm_app_dir - Returns the applications directory.
 *
 * Returns a system-dependent directory where the applications store
 * their private data, as components or resources.
 * [File Management]
 */
mpdm_t mpdm_app_dir(void)
{
	mpdm_t r = NULL;

#ifdef CONFOPT_WIN32

	HKEY hkey;
	char tmp[MAX_PATH];
	LPITEMIDLIST pidl;

	/* get the 'Program Files' folder (can fail) */
	tmp[0] = '\0';
	if(SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl) == S_OK)
		SHGetPathFromIDList(pidl, tmp);

	/* if it's still empty, get from the registry */
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
		0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		int n = sizeof(tmp);

		if(RegQueryValueEx(hkey, "ProgramFilesDir",
			NULL, NULL, tmp, (LPDWORD) &n) != ERROR_SUCCESS)
			tmp[0] = '\0';
	}

	if(tmp[0] != '\0')
	{
		strcat(tmp, "\\");
		r = MPDM_MBS(tmp);
	}
#endif

	/* still none? get the configured directory */
	if(r == NULL)
		r = MPDM_MBS(CONFOPT_PREFIX "/share/");

	return(r);
}
