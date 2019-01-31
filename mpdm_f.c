/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

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

#ifdef CONFOPT_CANONICALIZE_FILE_NAME
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef CONFOPT_WIN32

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>

#else /* CONFOPT_WIN32 */

#ifdef CONFOPT_GLOB_H
#include <glob.h>
#endif

#ifdef CONFOPT_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef CONFOPT_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef CONFOPT_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef CONFOPT_NETDB_H
#include <netdb.h>
#endif

#include <fcntl.h>

#endif /* CONFOPT_WIN32 */

#ifdef CONFOPT_UNISTD_H
#include <unistd.h>
#endif

#ifdef CONFOPT_PWD_H
#include <pwd.h>
#endif

#ifdef CONFOPT_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "mpdm.h"

#ifdef CONFOPT_ICONV
#include <iconv.h>
#endif

/* file structure */
struct mpdm_file {
    FILE *in;
    FILE *out;

    int sock;
    int is_pipe;

    wchar_t *(*f_read) (struct mpdm_file *, size_t *, size_t *);
    size_t (*f_write)  (struct mpdm_file *, const wchar_t *);

#ifdef CONFOPT_ICONV
    iconv_t ic_enc;
    iconv_t ic_dec;
#endif                          /* CONFOPT_ICONV */

#ifdef CONFOPT_WIN32
    HANDLE hin;
    HANDLE hout;
    HANDLE process;
#endif                          /* CONFOPT_WIN32 */
};

#include <errno.h>
extern int errno;


/** code **/


static void store_syserr(void)
/* stores the system error inside the global ERRNO */
{
    mpdm_hset_s(mpdm_root(), L"ERRNO", MPDM_MBS(strerror(errno)));
}


static int get_byte(struct mpdm_file *f)
/* reads a byte from a file structure */
{
    int c = EOF;

#ifdef CONFOPT_WIN32

    if (f->hin != NULL) {
        char tmp;
        DWORD n;

        if (ReadFile(f->hin, &tmp, 1, &n, NULL) && n > 0)
            c = (int) tmp;
    }

#endif /* CONFOPT_WIN32 */

    if (f->in != NULL) {
        /* read (converting to positive if needed) */
        if ((c = fgetc(f->in)) < 0 && !feof(f->in))
            c += 256;
    }

    if (f->sock != -1) {
        char b;

        if (recv(f->sock, &b, sizeof(b), 0) == sizeof(b))
            c = b;
    }

    return c;
}


static size_t put_buf(const char *ptr, size_t s, struct mpdm_file *f)
/* writes s bytes in the buffer in ptr to f */
{
#ifdef CONFOPT_WIN32

    if (f->hout != NULL) {
        DWORD n;

        if (WriteFile(f->hout, ptr, s, &n, NULL) && n > 0)
            s = n;
    }
    else
#endif                          /* CONFOPT_WIN32 */

    if (f->out != NULL)
        s = fwrite(ptr, s, 1, f->out);

    if (f->sock != -1)
        s = send(f->sock, ptr, s, 0);

    return s;
}


static int put_char(int c, struct mpdm_file *f)
/* writes a character in a file structure */
{
    char tmp = c;

    if (put_buf(&tmp, 1, f) != 1)
        c = EOF;

    return c;
}


static int store_in_line(wchar_t **ptr, size_t *s, size_t *eol, wchar_t c)
/* store the c in the line, keeping track for EOLs */
{
    int done = 0;

    /* track EOL sequence position */
    if (eol && *eol == -1) {
        if (c == L'\r' || c == L'\n')
            *eol = *s;
    }

    /* store */
    *ptr = mpdm_pokewsn(*ptr, s, &c, 1);

    /* end of line? finish */
    if (c == L'\n') {
        *ptr = mpdm_pokewsn(*ptr, s, L"", 1);
        (*s)--;
        done = 1;
    }

    return done;
}


static wchar_t *read_mbs(struct mpdm_file *f, size_t *s, size_t *eol)
/* reads a multibyte string from a mpdm_file into a dynamic string */
{
    char tmp[128];
    wchar_t *ptr = NULL;
    int c, i = 0;
    wchar_t wc;
    mbstate_t ps;

    while ((c = get_byte(f)) != EOF) {
        size_t r = -1;

        if (i < sizeof(tmp)) {
            tmp[i++] = c;

            /* try to convert what is read by now */
            memset(&ps, '\0', sizeof(ps));
            r = mbrtowc(&wc, tmp, i, &ps);

            /* incomplete sequence? keep trying */
            if (r == -2)
                continue;
        }

        if (r == -1) {
            /* too many failing bytes or invalid sequence;
               use the Unicode replacement char */
            wc = L'\xfffd';
        }

        i = 0;

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_wcs(struct mpdm_file *f, const wchar_t *str)
/* writes a wide string to an struct mpdm_file */
{
    size_t s;
    char *ptr;

    ptr = mpdm_wcstombs(str, &s);
    s = put_buf(ptr, s, f);
    free(ptr);

    return s;
}


#ifdef CONFOPT_ICONV

static wchar_t *read_iconv(struct mpdm_file *f, size_t *s, size_t *eol)
/* reads a multibyte string transforming with iconv */
{
    char tmp[128];
    wchar_t *ptr = NULL;
    int c, i = 0;
    wchar_t wc;

    /* resets the decoder */
    iconv(f->ic_dec, NULL, NULL, NULL, NULL);

    while ((c = get_byte(f)) != EOF) {
        size_t il, ol;
        char *iptr, *optr;

        tmp[i++] = c;

        /* too big? shouldn't happen */
        if (i == sizeof(tmp))
            break;

        il = i;
        iptr = tmp;
        ol = sizeof(wchar_t);
        optr = (char *) &wc;

        /* write to file */
        if (iconv(f->ic_dec, &iptr, &il, &optr, &ol) == (size_t) - 1) {
            /* found incomplete multibyte character */
            if (errno == EINVAL)
                continue;

            /* otherwise, return '?' */
            wc = L'\xfffd';
        }

        i = 0;

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_iconv(struct mpdm_file *f, const wchar_t *str)
/* writes a wide string to a stream using iconv */
{
    char tmp[128];
    size_t cnt = 0;

    /* resets the encoder */
    iconv(f->ic_enc, NULL, NULL, NULL, NULL);

    /* convert char by char */
    for (; *str != L'\0'; str++) {
        size_t il, ol;
        char *iptr, *optr;
        int n;

        il = sizeof(wchar_t);
        iptr = (char *) str;
        ol = sizeof(tmp);
        optr = tmp;

        /* write to file */
        if (iconv(f->ic_enc, &iptr, &il, &optr, &ol) == (size_t) - 1) {
            /* error converting; convert a '?' instead */
            wchar_t q = L'?';

            il = sizeof(wchar_t);
            iptr = (char *) &q;
            ol = sizeof(tmp);
            optr = tmp;

            iconv(f->ic_enc, &iptr, &il, &optr, &ol);
        }

        for (n = 0; n < (int) (sizeof(tmp) - ol); n++, cnt++) {
            if (put_char(tmp[n], f) == EOF)
                return -1;
        }
    }

    return cnt;
}


#endif /* CONFOPT_ICONV */

#define BYTE_OR_BREAK(c, f) if((c = get_byte(f)) == EOF) break

static wchar_t *read_utf8(struct mpdm_file *f, size_t *s, size_t *eol)
/* utf8 reader */
{
    wchar_t *ptr = NULL;
    wchar_t wc;
    int c;

    for (;;) {
        wc = L'\0';

        BYTE_OR_BREAK(c, f);

        if ((c & 0x80) == 0)
            wc = c;
        else
        if ((c & 0xe0) == 0xe0) {
            wc = (c & 0x1f) << 12;
            BYTE_OR_BREAK(c, f);
            wc |= (c & 0x3f) << 6;
            BYTE_OR_BREAK(c, f);
            wc |= (c & 0x3f);
        }
        else {
            wc = (c & 0x3f) << 6;
            BYTE_OR_BREAK(c, f);
            wc |= (c & 0x3f);
        }

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_utf8(struct mpdm_file *f, const wchar_t *str)
/* utf8 writer */
{
    int cnt = 0;
    wchar_t wc;

    /* convert char by char */
    for (; (wc = *str) != L'\0'; str++) {
        if (wc < 0x80)
            put_char((int) wc, f);
        else
        if (wc < 0x800) {
            put_char((int) (0xc0 | (wc >> 6)), f);
            put_char((int) (0x80 | (wc & 0x3f)), f);
            cnt++;
        }
        else {
            put_char((int) (0xe0 | (wc >> 12)), f);
            put_char((int) (0x80 | ((wc >> 6) & 0x3f)), f);
            put_char((int) (0x80 | (wc & 0x3f)), f);
            cnt += 2;
        }

        cnt++;
    }

    return cnt;
}


static wchar_t *read_utf8_bom(struct mpdm_file *f, size_t *s, size_t *eol)
/* utf-8 reader with BOM detection */
{
    wchar_t *enc = L"";

    f->f_read = NULL;

    /* autodetection */
    if (get_byte(f) == 0xef && get_byte(f) == 0xbb && get_byte(f) == 0xbf)
        enc = L"utf-8bom";
    else {
        enc = L"utf-8";
        fseek(f->in, 0, SEEK_SET);
    }

    mpdm_hset_s(mpdm_root(), L"DETECTED_ENCODING", MPDM_LS(enc));

    /* we're utf-8 from now on */
    f->f_read = read_utf8;

    return f->f_read(f, s, eol);
}


static size_t write_utf8_bom(struct mpdm_file *f, const wchar_t *str)
/* utf-8 writer with BOM */
{
    /* store the BOM */
    put_char(0xef, f);
    put_char(0xbb, f);
    put_char(0xbf, f);

    /* we're utf-8 from now on */
    f->f_write = write_utf8;

    return f->f_write(f, str);
}


static wchar_t *read_iso8859_1(struct mpdm_file *f, size_t *s, size_t *eol)
/* iso8859-1 reader */
{
    wchar_t *ptr = NULL;
    wchar_t wc;
    int c;

    while ((c = get_byte(f)) != EOF) {
        wc = c;

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_iso8859_1(struct mpdm_file *f, const wchar_t *str)
/* iso8859-1 writer */
{
    size_t cnt = 0;
    wchar_t wc;

    /* convert char by char */
    for (; (wc = *str) != L'\0'; str++)
        put_char(wc <= 0xff ? (int) wc : '?', f);

    return cnt;
}


static wchar_t *read_utf16ae(struct mpdm_file *f, size_t *s, size_t *eol, int le)
/* utf16 reader, ANY ending */
{
    wchar_t *ptr = NULL;
    wchar_t wc;
    int c1, c2;

    for (;;) {
        wc = L'\0';

        BYTE_OR_BREAK(c1, f);
        BYTE_OR_BREAK(c2, f);

        if (le)
            wc = c1 | (c2 << 8);
        else
            wc = c2 | (c1 << 8);

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_utf16ae(struct mpdm_file *f, const wchar_t *str, int le)
/* utf16 writer, ANY ending */
{
    size_t cnt = 0;
    wchar_t wc;

    /* convert char by char */
    for (; (wc = *str) != L'\0'; str++) {

        if (le) {
            put_char(wc & 0xff, f);
            put_char((wc & 0xff00) >> 8, f);
        }
        else {
            put_char((wc & 0xff00) >> 8, f);
            put_char(wc & 0xff, f);
        }
    }

    return cnt;
}


static wchar_t *read_utf16le(struct mpdm_file *f, size_t *s, size_t *eol)
{
    return read_utf16ae(f, s, eol, 1);
}


static size_t write_utf16le(struct mpdm_file *f, const wchar_t *str)
{
    return write_utf16ae(f, str, 1);
}


static wchar_t *read_utf16be(struct mpdm_file *f, size_t *s, size_t *eol)
{
    return read_utf16ae(f, s, eol, 0);
}


static size_t write_utf16be(struct mpdm_file *f, const wchar_t *str)
{
    return write_utf16ae(f, str, 0);
}


static wchar_t *read_utf16(struct mpdm_file *f, size_t *s, size_t *eol)
{
    int c1, c2;
    wchar_t *enc = L"utf-16le";

    /* assume little-endian */
    f->f_read = read_utf16le;

    /* autodetection */
    c1 = get_byte(f);
    c2 = get_byte(f);

    if (c1 == 0xfe && c2 == 0xff) {
        enc = L"utf-16be";
        f->f_read = read_utf16be;
    }
    else
    if (c1 != 0xff || c2 != 0xfe) {
        /* no BOM; rewind and hope */
        fseek(f->in, 0, SEEK_SET);
    }

    mpdm_hset_s(mpdm_root(), L"DETECTED_ENCODING", MPDM_LS(enc));

    return f->f_read(f, s, eol);
}


static size_t write_utf16le_bom(struct mpdm_file *f, const wchar_t *str)
{
    /* store the LE signature */
    put_char(0xff, f);
    put_char(0xfe, f);

    /* we're 16le from now on */
    f->f_write = write_utf16le;

    return f->f_write(f, str);
}


static size_t write_utf16be_bom(struct mpdm_file *f, const wchar_t *str)
{
    /* store the BE signature */
    put_char(0xfe, f);
    put_char(0xff, f);

    /* we're 16be from now on */
    f->f_write = write_utf16be;

    return f->f_write(f, str);
}


static wchar_t *read_utf32ae(struct mpdm_file *f, size_t *s, size_t *eol, int le)
/* utf32 reader, ANY ending */
{
    wchar_t *ptr = NULL;
    wchar_t wc;
    int c1, c2, c3, c4;

    for (;;) {
        wc = L'\0';

        BYTE_OR_BREAK(c1, f);
        BYTE_OR_BREAK(c2, f);
        BYTE_OR_BREAK(c3, f);
        BYTE_OR_BREAK(c4, f);

        if (le)
            wc = c1 | (c2 << 8) | (c3 << 16) | (c4 << 24);
        else
            wc = c4 | (c3 << 8) | (c2 << 16) | (c1 << 24);

        if (store_in_line(&ptr, s, eol, wc))
            break;
    }

    return ptr;
}


static size_t write_utf32ae(struct mpdm_file *f, const wchar_t *str, int le)
/* utf32 writer, ANY ending */
{
    size_t cnt = 0;
    wchar_t wc;

    /* convert char by char */
    for (; (wc = *str) != L'\0'; str++) {

        if (le) {
            put_char((wc & 0x000000ff), f);
            put_char((wc & 0x0000ff00) >> 8, f);
            put_char((wc & 0x00ff0000) >> 16, f);
            put_char((wc & 0xff000000) >> 24, f);
        }
        else {
            put_char((wc & 0xff000000) >> 24, f);
            put_char((wc & 0x00ff0000) >> 16, f);
            put_char((wc & 0x0000ff00) >> 8, f);
            put_char((wc & 0x000000ff), f);
        }
    }

    return cnt;
}


static wchar_t *read_utf32le(struct mpdm_file *f, size_t *s, size_t *eol)
{
    return read_utf32ae(f, s, eol, 1);
}


static size_t write_utf32le(struct mpdm_file *f, const wchar_t *str)
{
    return write_utf32ae(f, str, 1);
}


static wchar_t *read_utf32be(struct mpdm_file *f, size_t *s, size_t *eol)
{
    return read_utf32ae(f, s, eol, 0);
}


static size_t write_utf32be(struct mpdm_file *f, const wchar_t *str)
{
    return write_utf32ae(f, str, 0);
}


static wchar_t *read_utf32(struct mpdm_file *f, size_t *s, size_t *eol)
{
    int c1, c2, c3, c4;
    wchar_t *enc = L"utf-32le";

    f->f_read = read_utf32le;

    /* autodetection */
    c1 = get_byte(f);
    c2 = get_byte(f);
    c3 = get_byte(f);
    c4 = get_byte(f);

    if (c1 == 0 && c2 == 0 && c3 == 0xfe && c4 == 0xff) {
        enc = L"utf-32be";
        f->f_read = read_utf32be;
    }
    if (c1 != 0xff || c2 != 0xfe || c3 != 0 || c4 != 0) {
        /* no BOM; assume le and hope */
        fseek(f->in, 0, SEEK_SET);
    }

    mpdm_hset_s(mpdm_root(), L"DETECTED_ENCODING", MPDM_LS(enc));

    return f->f_read(f, s, eol);
}


static size_t write_utf32le_bom(struct mpdm_file *f, const wchar_t *str)
{
    /* store the LE signature */
    put_char(0xff, f);
    put_char(0xfe, f);
    put_char(0, f);
    put_char(0, f);

    /* we're 32le from now on */
    f->f_write = write_utf32le;

    return f->f_write(f, str);
}


static size_t write_utf32be_bom(struct mpdm_file *f, const wchar_t *str)
{
    /* store the BE signature */
    put_char(0, f);
    put_char(0, f);
    put_char(0xfe, f);
    put_char(0xff, f);

    /* we're 32be from now on */
    f->f_write = write_utf32be;

    return f->f_write(f, str);
}


static wchar_t *read_auto(struct mpdm_file *f, size_t *s, size_t *eol)
/* autodetects different encodings based on the BOM */
{
    wchar_t *enc = L"";

    /* by default, multibyte reading */
    f->f_read = read_mbs;

    /* ensure seeking is possible */
    if (f->in != NULL && fseek(f->in, 0, SEEK_CUR) != -1) {
        int c;

        c = get_byte(f);

        if (c == 0xff) {
            /* can be utf32le or utf16le */
            if (get_byte(f) == 0xfe) {
                /* if next 2 chars are 0x00, it's utf32; otherwise utf16 */
                if (get_byte(f) == 0x00 && get_byte(f) == 0x00) {
                    enc = L"utf-32le";
                    f->f_read = read_utf32le;
                    goto got_encoding;
                }
                else {
                    /* rewind to 3rd character */
                    fseek(f->in, 2, SEEK_SET);

                    enc = L"utf-16le";
                    f->f_read = read_utf16le;
                    goto got_encoding;
                }
            }
        }
        else
        if (c == 0x00) {
            /* can be utf32be */
            if (get_byte(f) == 0x00 && get_byte(f) == 0xfe
                && get_byte(f) == 0xff) {
                enc = L"utf-32be";
                f->f_read = read_utf32be;
                goto got_encoding;
            }
        }
        else
        if (c == 0xfe) {
            /* can be utf16be */
            if (get_byte(f) == 0xff) {
                enc = L"utf-16be";
                f->f_read = read_utf16be;
                goto got_encoding;
            }
        }
        else
        if (c == 0xef) {
            /* can be utf8 with BOM */
            if (get_byte(f) == 0xbb && get_byte(f) == 0xbf) {
                enc = L"utf-8bom";
                f->f_read = read_utf8;
                goto got_encoding;
            }
        }
        else {
            /* try if a first bunch of chars are valid UTF-8 */
            int p = c;
            int n = 10000;
            int u = 0;

            while (--n && (c = get_byte(f)) != EOF) {
                if ((c & 0xc0) == 0x80) {
                    if ((p & 0xc0) == 0xc0)
                        u++;
                    else
                    if ((p & 0x80) == 0x00) {
                        u = -1;
                        break;
                    }
                }
                else
                if ((p & 0xc0) == 0xc0) {
                    u = -1;
                    break;
                }

                p = c;
            }

            if (u < 0) {
                /* invalid utf-8; fall back to 8bit */
                enc = L"8bit";
                f->f_read = read_iso8859_1;
            }
            else
            if (u > 0) {
                /* utf-8 sequences found */
                enc = L"utf-8";
                f->f_read = read_utf8;
            }

            /* 7 bit ASCII: do nothing */
        }

        /* none of the above; restart */
        fseek(f->in, 0, SEEK_SET);
    }

got_encoding:
    mpdm_hset_s(mpdm_root(), L"DETECTED_ENCODING", MPDM_LS(enc));

    return f->f_read(f, s, eol);
}


/** interface **/

wchar_t *mpdm_read_mbs(FILE *f, size_t *s)
/* reads a multibyte string from a stream into a dynamic string */
{
    struct mpdm_file fs;

    /* reset the structure */
    memset(&fs, '\0', sizeof(fs));
    fs.in = f;

    *s = 0;
    return read_mbs(&fs, s, NULL);
}


size_t mpdm_write_wcs(FILE *f, const wchar_t *str)
/* writes a wide string to a stream */
{
    struct mpdm_file fs;

    /* reset the structure */
    memset(&fs, '\0', sizeof(fs));
    fs.out = f;

    return write_wcs(&fs, str);
}


/**
 * mpdm_open - Opens a file.
 * @filename: the file name
 * @mode: an fopen-like mode string
 *
 * Opens a file. If @filename can be open in the specified @mode, an
 * mpdm_t value will be returned containing the file descriptor, or NULL
 * otherwise.
 *
 * If the file is open for reading, some charset detection methods are
 * used. If any of them is successful, its name is stored in the
 * DETECTED_ENCODING element of the mpdm_root() hash. This value is
 * suitable to be copied over ENCODING or TEMP_ENCODING.
 *
 * If the file is open for writing, the encoding to be used is read from
 * the ENCODING element of mpdm_root() and, if not set, from the
 * TEMP_ENCODING one. The latter will always be deleted afterwards.
 * [File Management]
 */
mpdm_t mpdm_open(const mpdm_t filename, const mpdm_t mode)
{
    FILE *f = NULL;
    mpdm_t fn;
    mpdm_t m;

    mpdm_ref(filename);
    mpdm_ref(mode);

    if (filename != NULL && mode != NULL) {
        /* convert to mbs,s */
        fn = mpdm_ref(MPDM_2MBS(filename->data));
        m = mpdm_ref(MPDM_2MBS(mode->data));

        if ((f = fopen((char *) fn->data, (char *) m->data)) == NULL)
            store_syserr();
        else {
#if defined(CONFOPT_SYS_STAT_H) && defined(S_ISDIR) && defined(EISDIR)
            struct stat s;

            /* test if the open file is a directory */
            if (fstat(fileno(f), &s) != -1 && S_ISDIR(s.st_mode)) {
                /* it's a directory; fail */
                errno = EISDIR;
                store_syserr();
                fclose(f);
                f = NULL;
            }
#endif
        }

        mpdm_unref(m);
        mpdm_unref(fn);
    }

    mpdm_unref(mode);
    mpdm_unref(filename);

    return f ? MPDM_F(f) : NULL;
}


/**
 * mpdm_read - Reads a line from a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Reads a line from @fd. Returns the line, or NULL on EOF.
 * [File Management]
 * [Character Set Conversion]
 */
mpdm_t mpdm_read(const mpdm_t fd)
{
    mpdm_t v = NULL;

    if (fd != NULL) {
        struct mpdm_file *fs = (struct mpdm_file *) fd->data;

        if (fs != NULL) {
            wchar_t *ptr;
            size_t s = 0;

            ptr = fs->f_read(fs, &s, NULL);

            if (ptr != NULL)
                v = MPDM_ENS(ptr, s);
        }
    }

    return v;
}


mpdm_t mpdm_getchar(const mpdm_t fd)
{
    int c;
    wchar_t tmp[2];
    mpdm_t r = NULL;
    struct mpdm_file *fs = (struct mpdm_file *) fd->data;

    if (fs != NULL && (c = get_byte(fs)) != EOF) {
        /* get the char as-is */
        tmp[0] = (wchar_t) c;
        tmp[1] = L'\0';

        r = MPDM_S(tmp);
    }

    return r;
}


int mpdm_putchar(const mpdm_t fd, const mpdm_t c)
{
    struct mpdm_file *fs = (struct mpdm_file *) fd->data;
    const wchar_t *ptr = mpdm_string(c);
    int r = 1;

    mpdm_ref(c);

    if (fs == NULL || put_char(*ptr, fs) == -1)
        r = 0;

    mpdm_unref(c);

    return r;
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
size_t mpdm_write(const mpdm_t fd, const mpdm_t v)
{
    size_t ret = -1;

    mpdm_ref(v);

    if (fd) {
        struct mpdm_file *fs = (struct mpdm_file *) fd->data;

        if (fs != NULL)
            ret = fs->f_write(fs, mpdm_string(v));
    }

    mpdm_unref(v);

    return ret;
}


int mpdm_fseek(const mpdm_t fd, long offset, int whence)
{
    struct mpdm_file *fs = (struct mpdm_file *) fd->data;

    return fseek(fs->in, offset, whence);
}


long mpdm_ftell(const mpdm_t fd)
{
    struct mpdm_file *fs = (struct mpdm_file *) fd->data;

    return ftell(fs->in);
}


FILE *mpdm_get_filehandle(const mpdm_t fd)
{
    FILE *f = NULL;

    if (fd->flags & MPDM_FILE && fd->data != NULL) {
        struct mpdm_file *fs = (struct mpdm_file *) fd->data;
        f = fs->in;
    }

    return f;
}


/*
mpdm_t mpdm_bread(mpdm_t fd, int size)
{
}


int mpdm_bwrite(mpdm_tfd, mpdm_t v, int size)
{
}
*/


static mpdm_t embedded_encodings(void)
{
    mpdm_t e;
    wchar_t *e2e[] = {
        L"utf-8", L"utf-8",
        L"utf8", NULL,
        L"iso8859-1", L"iso8859-1",
        L"iso-8859-1", NULL,
        L"8bit", NULL,
        L"latin1", NULL,
        L"latin-1", NULL,
        L"utf-16le", L"utf-16le",
        L"utf16le", NULL,
        L"ucs-2le", NULL,
        L"utf-16be", L"utf-16be",
        L"utf16be", NULL,
        L"ucs-2be", NULL,
        L"utf-16", L"utf-16",
        L"utf16", NULL,
        L"ucs-2", NULL,
        L"ucs2", NULL,
        L"utf-32le", L"utf-32le",
        L"utf32le", NULL,
        L"ucs-4le", NULL,
        L"utf-32be", L"utf-32be",
        L"utf32be", NULL,
        L"ucs-4be", NULL,
        L"utf-32", L"utf-32",
        L"utf32", NULL,
        L"ucs-4", NULL,
        L"ucs4", NULL,
        L"utf-8bom", L"utf-8bom",
        L"utf8bom", NULL,
        NULL, NULL
    };

    if ((e = mpdm_hget_s(mpdm_root(), L"EMBEDDED_ENCODINGS")) == NULL) {
        int n;
        mpdm_t p = NULL;

        e = mpdm_ref(MPDM_H(0));

        for (n = 0; e2e[n] != NULL; n += 2) {
            mpdm_t v = MPDM_S(e2e[n]);

            if (e2e[n + 1] != NULL)
                p = MPDM_S(e2e[n + 1]);

            mpdm_hset(e, v, p);
            mpdm_hset(e, mpdm_ulc(v, 1), p);
        }

        mpdm_hset_s(mpdm_root(), L"EMBEDDED_ENCODINGS", e);

        mpdm_unref(e);
    }

    return e;
}


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
 *
 * This function stores the @charset value into the ENCODING item
 * of the mpdm_root() hash.
 *
 * Returns a negative number if @charset is unsupported, or zero
 * if no errors were found.
 * [File Management]
 * [Character Set Conversion]
 */
int mpdm_encoding(mpdm_t charset)
{
    int ret = -1;
    mpdm_t e = embedded_encodings();
    mpdm_t v = NULL;

    mpdm_ref(charset);

    /* NULL encoding? done */
    if (mpdm_size(charset) == 0) {
        mpdm_hset_s(mpdm_root(), L"ENCODING", NULL);
        ret = 0;
    }

#ifdef CONFOPT_ICONV
    else {
        iconv_t ic;
        mpdm_t cs = mpdm_ref(MPDM_2MBS(charset->data));

        /* tries to create an iconv encoder and decoder for this charset */

        if ((ic = iconv_open("WCHAR_T", (char *) cs->data)) == (iconv_t) - 1)
            ret = -1;
        else {
            iconv_close(ic);

            if ((ic = iconv_open((char *) cs->data, "WCHAR_T")) == (iconv_t) - 1)
                ret = -2;
            else {
                iconv_close(ic);

                /* got a valid encoding */
                v = charset;
                ret = 0;
            }
        }

        mpdm_unref(cs);
    }
#endif                          /* CONFOPT_ICONV */

    if (ret != 0 && (v = mpdm_hget(e, charset)) != NULL)
        ret = 0;

    if (ret == 0)
        mpdm_hset_s(mpdm_root(), L"ENCODING", v);

    mpdm_unref(charset);

    return ret;
}


/**
 * mpdm_unlink - Deletes a file.
 * @filename: file name to be deleted
 *
 * Deletes a file.
 * [File Management]
 */
int mpdm_unlink(const mpdm_t filename)
{
    int ret;
    mpdm_t fn;

    mpdm_ref(filename);

    /* convert to mbs */
    fn = mpdm_ref(MPDM_2MBS(filename->data));

    if ((ret = unlink((char *) fn->data)) == -1)
        store_syserr();

    mpdm_unref(fn);
    mpdm_unref(filename);

    return ret;
}


/**
 * mpdm_rename - Renames a file.
 * @o: old path
 * @n: new path
 *
 * Renames a file.
 * [File Management]
 */
int mpdm_rename(const mpdm_t o, const mpdm_t n)
{
    int ret;
    mpdm_t om, nm;

    mpdm_ref(o);
    mpdm_ref(n);

    om = mpdm_ref(MPDM_2MBS(o->data));
    nm = mpdm_ref(MPDM_2MBS(n->data));

    if ((ret = rename((char *)om->data, (char *)nm->data)) == -1)
        store_syserr();

    mpdm_unref(nm);
    mpdm_unref(om);
    mpdm_unref(n);
    mpdm_unref(o);

    return ret;
}


/**
 * mpdm_stat - Gives status from a file.
 * @filename: file name to get the status from
 *
 * Returns a 14 element array of the status (permissions, onwer, etc.)
 * from the desired @filename, or NULL if the file cannot be accessed.
 * (man 2 stat).
 *
 * The values are: 0, device number of filesystem; 1, inode number;
 * 2, file mode; 3, number of hard links to the file; 4, uid; 5, gid;
 * 6, device identifier; 7, total size of file in bytes; 8, atime;
 * 9, mtime; 10, ctime; 11, preferred block size for system I/O;
 * 12, number of blocks allocated and 13, canonicalized file name.
 * Not all elements have necesarily meaningful values, as most are
 * system-dependent.
 * [File Management]
 */
mpdm_t mpdm_stat(const mpdm_t filename)
{
    mpdm_t r = NULL;

    mpdm_ref(filename);

#ifdef CONFOPT_SYS_STAT_H
    struct stat s;
    mpdm_t fn;

    fn = mpdm_ref(MPDM_2MBS(filename->data));

    if (stat((char *) fn->data, &s) != -1) {
        r = MPDM_A(14);

        mpdm_ref(r);

        mpdm_aset(r, MPDM_I(s.st_dev), 0);
        mpdm_aset(r, MPDM_I(s.st_ino), 1);
        mpdm_aset(r, MPDM_I(s.st_mode), 2);
        mpdm_aset(r, MPDM_I(s.st_nlink), 3);
        mpdm_aset(r, MPDM_I(s.st_uid), 4);
        mpdm_aset(r, MPDM_I(s.st_gid), 5);
        mpdm_aset(r, MPDM_I(s.st_rdev), 6);
        mpdm_aset(r, MPDM_I(s.st_size), 7);
        mpdm_aset(r, MPDM_I(s.st_atime), 8);
        mpdm_aset(r, MPDM_I(s.st_mtime), 9);
        mpdm_aset(r, MPDM_I(s.st_ctime), 10);
        mpdm_aset(r, MPDM_I(0), 11);    /* s.st_blksize */
        mpdm_aset(r, MPDM_I(0), 12);    /* s.st_blocks */

#ifdef CONFOPT_CANONICALIZE_FILE_NAME

        {
            char *ptr;

            if ((ptr = canonicalize_file_name((char *) fn->data)) != NULL) {
                mpdm_aset(r, MPDM_MBS(ptr), 13);
                free(ptr);
            }
        }
#endif

#ifdef CONFOPT_REALPATH
        {
            char tmp[2048];

            if (realpath((char *) fn->data, tmp) != NULL)
                mpdm_aset(r, MPDM_MBS(tmp), 13);
        }
#endif

#ifdef CONFOPT_FULLPATH
        {
            char tmp[_MAX_PATH + 1];

            if (_fullpath(tmp, (char *) fn->data, _MAX_PATH) != NULL)
                mpdm_aset(r, MPDM_MBS(tmp), 13);
        }
#endif

        mpdm_unrefnd(r);
    }
    else
        store_syserr();

    mpdm_unref(fn);

#endif                          /* CONFOPT_SYS_STAT_H */

    mpdm_unref(filename);

    return r;
}


/**
 * mpdm_chmod - Changes a file's permissions.
 * @filename: the file name
 * @perms: permissions (element 2 from mpdm_stat())
 *
 * Changes the permissions for a file.
 * [File Management]
 */
int mpdm_chmod(const mpdm_t filename, mpdm_t perms)
{
    int r = -1;

    mpdm_ref(filename);
    mpdm_ref(perms);

    mpdm_t fn = mpdm_ref(MPDM_2MBS(filename->data));

    if ((r = chmod((char *) fn->data, mpdm_ival(perms))) == -1)
        store_syserr();

    mpdm_unref(fn);
    mpdm_unref(perms);
    mpdm_unref(filename);

    return r;
}


/**
 * mpdm_chdir - Changes the working directory
 * @dir: the new path
 *
 * Changes the working directory
 * [File Management]
 */
int mpdm_chdir(const mpdm_t dir)
{
    int r = -1;

    mpdm_ref(dir);
    mpdm_t fn = mpdm_ref(MPDM_2MBS(dir->data));

    if ((r = chdir((char *) fn->data)) == -1)
        store_syserr();

    mpdm_unref(fn);
    mpdm_unref(dir);

    return r;
}


/**
 * mpdm_getcwd - Get current working directory
 *
 * Returns the current working directory.
 * [File Management]
 */
mpdm_t mpdm_getcwd(void)
{
    char tmp[4096];

    getcwd(tmp, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    return MPDM_MBS(tmp);
}


/**
 * mpdm_chown - Changes a file's owner.
 * @filename: the file name
 * @uid: user id (element 4 from mpdm_stat())
 * @gid: group id (element 5 from mpdm_stat())
 *
 * Changes the owner and group id's for a file.
 * [File Management]
 */
int mpdm_chown(const mpdm_t filename, mpdm_t uid, mpdm_t gid)
{
    int r = -1;

    mpdm_ref(filename);
    mpdm_ref(uid);
    mpdm_ref(gid);

#ifdef CONFOPT_CHOWN

    mpdm_t fn = mpdm_ref(MPDM_2MBS(filename->data));

    if ((r = chown((char *) fn->data, mpdm_ival(uid), mpdm_ival(gid))) == -1)
        store_syserr();

    mpdm_unref(fn);

#endif /* CONFOPT_CHOWN */

    mpdm_unref(gid);
    mpdm_unref(uid);
    mpdm_unref(filename);

    return r;
}


/**
 * mpdm_glob - Executes a file globbing.
 * @spec: Globbing spec
 * @base: Optional base directory
 *
 * Executes a file globbing. @spec is system-dependent, but usually
 * the * and ? metacharacters work everywhere. @base can contain a
 * directory; if that's the case, the output strings will include it.
 * In any case, each returned value will be suitable for a call to
 * mpdm_open().
 *
 * Returns an array of files that match the globbing (can be an empty
 * array if no file matches), or NULL if globbing is unsupported.
 * [File Management]
 */
mpdm_t mpdm_glob(mpdm_t spec, mpdm_t base)
{
    mpdm_t d, f, v;
    char *ptr;

#ifdef CONFOPT_WIN32
    WIN32_FIND_DATA fd;
    HANDLE h;
    const wchar_t *def_spec = L"*.*";
    mpdm_t p;
#endif

#if CONFOPT_GLOB_H
    glob_t globbuf;
    const wchar_t *def_spec = L"*";
#endif

    /* build full path */
    if (base != NULL) {
        base = mpdm_strcat_s(base, L"/");

        /* escape expandable chars */
        base = mpdm_sregex(base, MPDM_LS(L"@[]\\[]@g"), MPDM_LS(L"\\\\&"), 0);
    }

    if (spec == NULL)
        spec = mpdm_strcat_s(base, def_spec);
    else
        spec = mpdm_strcat(base, spec);

    /* delete repeated directory delimiters */
    spec = mpdm_sregex(spec, MPDM_LS(L"@[\\/]{2,}@g"), MPDM_LS(L"/"), 0);

    mpdm_ref(spec);
    ptr = mpdm_wcstombs(mpdm_string(spec), NULL);
    mpdm_unref(spec);

    v = MPDM_A(0);
    d = mpdm_ref(MPDM_A(0));
    f = mpdm_ref(MPDM_A(0));

#ifdef CONFOPT_WIN32
    if ((h = FindFirstFile(ptr, &fd)) != INVALID_HANDLE_VALUE) {
        char *b;

        /* if spec includes a directory, store in s */
        if ((b = strrchr(ptr, '/')) != NULL) {
            *(b + 1) = '\0';
            p = MPDM_MBS(ptr);
        }
        else
            p = NULL;

        mpdm_ref(p);

        do {
            mpdm_t t;

            /* ignore . and .. */
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
                continue;

            /* concat base directory and file names */
            t = mpdm_strcat(p, MPDM_MBS(fd.cFileName));

            /* if it's a directory, add a / */
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                mpdm_push(d, mpdm_strcat_s(t, L"/"));
            else
                mpdm_push(f, t);
        }
        while (FindNextFile(h, &fd));

        FindClose(h);

        mpdm_unref(p);
    }
#endif

#if CONFOPT_GLOB_H
    globbuf.gl_offs = 1;

    if (glob(ptr, GLOB_MARK, NULL, &globbuf) == 0) {
        int n;

        for (n = 0; globbuf.gl_pathv[n] != NULL; n++) {
            char *p = globbuf.gl_pathv[n];
            mpdm_t t = MPDM_MBS(p);

            /* if last char is /, add to directories */
            if (p[strlen(p) - 1] == '/')
                mpdm_push(d, t);
            else
                mpdm_push(f, t);
        }
    }

    globfree(&globbuf);
#endif

    free(ptr);

    if (v != NULL) {
        int n;

        mpdm_sort(d, 1);
        mpdm_sort(f, 1);

        mpdm_ref(v);

        /* transfer all data in d and f */
        for (n = 0; n < mpdm_size(d); n++)
            mpdm_push(v, mpdm_aget(d, n));
        for (n = 0; n < mpdm_size(f); n++)
            mpdm_push(v, mpdm_aget(f, n));

        mpdm_unref(f);
        mpdm_unref(d);

        mpdm_unrefnd(v);
    }

    return v;
}


/** pipes **/


#ifdef CONFOPT_WIN32

static void win32_pipe(HANDLE *h, int n)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE cp, t;

    memset(&sa, '\0', sizeof(sa));
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = NULL;

    cp = GetCurrentProcess();

    CreatePipe(&h[0], &h[1], &sa, 0);
    DuplicateHandle(cp, h[n], cp, &t, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(h[n]);
    h[n] = t;
}


static int sysdep_popen(mpdm_t v, char *prg, int rw)
/* win32-style pipe */
{
    HANDLE pr[2];
    HANDLE pw[2];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    int ret;
    struct mpdm_file *fs = (struct mpdm_file *) v->data;

    fs->is_pipe = 1;

    /* init all */
    pr[0] = pr[1] = pw[0] = pw[1] = NULL;

    if (rw & 0x01)
        win32_pipe(pr, 0);
    if (rw & 0x02)
        win32_pipe(pw, 1);

    /* spawn new process */
    memset(&pi, '\0', sizeof(pi));
    memset(&si, '\0', sizeof(si));

    si.cb = sizeof(STARTUPINFO);
    si.hStdError = pr[1];
    si.hStdOutput = pr[1];
    si.hStdInput = pw[0];
    si.dwFlags |= STARTF_USESTDHANDLES;

    ret = CreateProcess(NULL, prg, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    if (rw & 0x01)
        CloseHandle(pr[1]);
    if (rw & 0x02)
        CloseHandle(pw[0]);

    fs->hin     = pr[0];
    fs->hout    = pw[1];
    fs->process = pi.hProcess;

    return ret;
}


static int sysdep_pclose(const mpdm_t v)
{
    struct mpdm_file *fs = (struct mpdm_file *) v->data;
    DWORD out = 0;

    if (fs->hin != NULL)
        CloseHandle(fs->hin);

    if (fs->hout != fs->hin && fs->hout != NULL)
        CloseHandle(fs->hout);

    fs->hin = NULL;
    fs->hout = NULL;

    /* waits until the process terminates */
    WaitForSingleObject(fs->process, 1000);
    GetExitCodeProcess(fs->process, &out);

    return (int) out;
}


#else /* CONFOPT_WIN32 */

static int sysdep_popen(mpdm_t v, char *prg, int rw)
/* unix-style pipe open */
{
    int pr[2], pw[2];
    struct mpdm_file *fs = (struct mpdm_file *) v->data;

    fs->is_pipe = 1;

    /* init all */
    pr[0] = pr[1] = pw[0] = pw[1] = -1;

    if (rw & 0x01)
        pipe(pr);
    if (rw & 0x02)
        pipe(pw);

    if (fork() == 0) {
        setsid();

        /* child process */
        if (rw & 0x01) {
            close(1);
            dup(pr[1]);
            close(pr[0]);
        }
        if (rw & 0x02) {
            close(0);
            dup(pw[0]);
            close(pw[1]);
        }

        /* redirect stderr to stdout */
        close(2);
        dup(1);

        /* run the program */
        execlp("/bin/sh", "/bin/sh", "-c", prg, NULL);
        execlp(prg, prg, NULL);

        /* still here? exec failed; close pipes and exit */
        close(0);
        close(1);
        exit(0);
    }

    /* create the pipes as non-buffered streams */
    if (rw & 0x01) {
        fs->in = fdopen(pr[0], "r");
        setvbuf(fs->in, NULL, _IONBF, 0);
        close(pr[1]);
    }

    if (rw & 0x02) {
        fs->out = fdopen(pw[1], "w");
        setvbuf(fs->out, NULL, _IONBF, 0);
        close(pw[0]);
    }

    return 1;
}


static int sysdep_pclose(const mpdm_t v)
/* unix-style pipe close */
{
    int s = 0;

    wait(&s);

    return s;
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
mpdm_t mpdm_popen(const mpdm_t prg, const mpdm_t mode)
{
    mpdm_t v = NULL;

    mpdm_ref(prg);
    mpdm_ref(mode);

    if (prg != NULL && mode != NULL) {
        mpdm_t pr, md;
        char *m;
        int rw = 0;

        v = MPDM_F(NULL);

        /* convert to mbs,s */
        pr = mpdm_ref(MPDM_2MBS(prg->data));
        md = mpdm_ref(MPDM_2MBS(mode->data));

        /* get the mode */
        m = (char *) md->data;

        /* set the mode */
        if (m[0] == 'r')
            rw = 0x01;
        if (m[0] == 'w')
            rw = 0x02;
        if (m[1] == '+')
            rw = 0x03;          /* r+ or w+ */

        if (!sysdep_popen(v, (char *) pr->data, rw)) {
            mpdm_void(v);
            v = NULL;
        }

        mpdm_unref(md);
        mpdm_unref(pr);
    }

    mpdm_unref(mode);
    mpdm_unref(prg);

    return v;
}


/**
 * mpdm_popen2 - Opens a pipe and returns 2 descriptors.
 * @prg: the program to pipe
 *
 * Opens a read-write pipe and returns an array of two descriptors,
 * one for reading and one for writing. If @prg could not be piped to,
 * returns NULL.
 * [File Management]
 */
mpdm_t mpdm_popen2(const mpdm_t prg)
{
    mpdm_t i, o;
    mpdm_t p = NULL;

    if ((i = mpdm_popen(prg, MPDM_LS(L"r+"))) != NULL) {
        struct mpdm_file *ifs;
        struct mpdm_file *ofs;

        o = MPDM_C(i->flags, (void *)i->data, i->size);

        ifs = (struct mpdm_file *)i->data;
        ofs = (struct mpdm_file *)o->data;

        ofs->in = ifs->out;
        ifs->out = NULL;

#ifdef CONFOPT_WIN32
        ofs->hin = ifs->hout;
        ifs->hout = NULL;
#endif

        p = mpdm_ref(MPDM_A(2));
        mpdm_aset(p, i, 0);
        mpdm_aset(p, o, 1);
        mpdm_unrefnd(p);
    }

    return p;
}


/**
 * mpdm_pclose - Closes a pipe.
 * @fd: the value containing the file descriptor
 *
 * Closes a pipe.
 * [File Management]
 */
int mpdm_pclose(mpdm_t fd)
{
    return mpdm_close(fd);
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
    char *ptr;
    char tmp[512];

    tmp[0] = '\0';

#ifdef CONFOPT_WIN32

    LPITEMIDLIST pidl;

    /* get the 'My Documents' folder */
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
    SHGetPathFromIDList(pidl, tmp);
    strcat(tmp, "\\");

#endif

#ifdef CONFOPT_PWD_H

    struct passwd *p;

    /* get home dir from /etc/passwd entry */
    if (tmp[0] == '\0' && (p = getpwuid(getpid())) != NULL) {
        strcpy(tmp, p->pw_dir);
        strcat(tmp, "/");
    }

#endif

    /* still none? try the ENV variable $HOME */
    if (tmp[0] == '\0' && (ptr = getenv("HOME")) != NULL) {
        strcpy(tmp, ptr);
        strcat(tmp, "/");
    }

    if (tmp[0] != '\0')
        r = MPDM_MBS(tmp);

    return r;
}


/**
 * mpdm_app_dir - Returns the applications directory.
 *
 * Returns a system-dependent directory where the applications store
 * their private data, as components or resources.
 *
 * If the global APPID MPDM variable is set, it's used to search for
 * the specific application installation folder (on MS Windows' registry)
 * and / or appended as the final folder.
 * [File Management]
 */
mpdm_t mpdm_app_dir(void)
{
    mpdm_t r = NULL;
    mpdm_t appid = mpdm_hget_s(mpdm_root(), L"APPID");
    int aok = 0;

#ifdef CONFOPT_WIN32

    HKEY hkey;
    char tmp[MAX_PATH];
    LPITEMIDLIST pidl;

    tmp[0] = '\0';

    if (appid != NULL) {
        /* find the installation folder in the registry */
        char *ptr = mpdm_wcstombs(mpdm_string(appid), NULL);

        sprintf(tmp, "SOFTWARE\\%s\\appdir", ptr);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, tmp, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
            int n = sizeof(tmp);

            if (RegQueryValueEx(hkey, NULL, NULL, NULL,
                (LPBYTE) tmp, (LPDWORD) &n) == ERROR_SUCCESS)
                    aok = 1;
                else
                    tmp[0] = '\0';
        }
        else
            tmp[0] = '\0';

        free(ptr);
    }

    if (tmp[0] == '\0') {
        /* get the 'Program Files' folder (can fail) */
        if (SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl) == S_OK)
            SHGetPathFromIDList(pidl, tmp);

        /* if it's still empty, get from the registry */
        if (tmp[0] == '\0' && RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                     0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
            int n = sizeof(tmp);

            if (RegQueryValueEx(hkey, "ProgramFilesDir",
                                NULL, NULL, (LPBYTE) tmp,
                                (LPDWORD) & n) != ERROR_SUCCESS)
                tmp[0] = '\0';
        }
    }

    if (tmp[0] != '\0') {
        strcat(tmp, "\\");
        r = MPDM_MBS(tmp);
    }
#endif

    /* still none? get the configured directory */
    if (r == NULL)
        r = MPDM_MBS(CONFOPT_PREFIX "/share/");

    if (appid && !aok)
        r = mpdm_strcat(r, appid);

    return r;
}


/** sockets **/

void init_sockets(void)
{
    static int init = 0;

    if (init == 0) {
        init = 1;

#ifdef CONFOPT_WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
}


mpdm_t mpdm_connect(mpdm_t host, mpdm_t serv)
{
    mpdm_t f = NULL;
    char *h;
    char *s;
    int d = -1;

    mpdm_ref(host);
    mpdm_ref(serv);

    h = mpdm_wcstombs(mpdm_string(host), NULL);
    s = mpdm_wcstombs(mpdm_string(serv), NULL);

    init_sockets();

    {

#ifndef CONFOPT_WITHOUT_GETADDRINFO

    struct addrinfo *res;
    struct addrinfo hints;

    memset(&hints, '\0', sizeof(hints));

    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_ADDRCONFIG;

    if (getaddrinfo(h, s, &hints, &res) == 0) {
        struct addrinfo *r;

        for (r = res; r != NULL; r = r->ai_next) {
            d = socket(r->ai_family, r->ai_socktype, r->ai_protocol);

            if (d != -1) {
                if (connect(d, r->ai_addr, r->ai_addrlen) == 0)
                    break;

                close(d);
                d = -1;
            }
        }

        freeaddrinfo(res);
    }

#else   /* CONFOPT_WITHOUT_GETADDRINFO */

    /* traditional socket interface */
    struct hostent *he;

    if ((he = gethostbyname(h)) != NULL) {
        struct servent *se;

        if ((se = getservbyname(s, "tcp")) != NULL) {
            struct sockaddr_in host;
    
            memset(&host, '\0', sizeof(host));

            memcpy(&host.sin_addr, he->h_addr_list[0], he->h_length);
            host.sin_family = he->h_addrtype;
            host.sin_port   = se->s_port;
    
            if ((d = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
                if (connect(d, (struct sockaddr *)&host, sizeof(host)) == -1)
                    d = -1;
            }
        }
    }

#endif  /* CONFOPT_WITHOUT_GETADDRINFO */

    }

    /* create file value */
    if (d != -1) {
        struct mpdm_file *fs;

        f = MPDM_F(NULL);
        fs = (struct mpdm_file *) f->data;

        fs->sock = d;
    }

    free(s);
    free(h);

    mpdm_unref(serv);
    mpdm_unref(host);

    return f;
}


static int file_close(mpdm_t v)
/* close any type of file / pipe / socket */
{
    int r = -1;
    struct mpdm_file *fs;

    if (v && (fs = (struct mpdm_file *) v->data)) {
#ifdef CONFOPT_ICONV
        if (fs->ic_enc != (iconv_t) - 1)
            iconv_close(fs->ic_enc);

        if (fs->ic_dec != fs->ic_enc && fs->ic_dec != (iconv_t) - 1)
            iconv_close(fs->ic_dec);

        fs->ic_enc = (iconv_t) - 1;
        fs->ic_dec = (iconv_t) - 1;
#endif

        if (fs->in != NULL)
            r = fclose(fs->in);

        if (fs->out != fs->in && fs->out != NULL)
            r = fclose(fs->out);

        fs->in = NULL;
        fs->out = NULL;

        if (fs->is_pipe) {
            r = sysdep_pclose(v);
            fs->is_pipe = 0;
        }

        if (fs->sock != -1) {
#ifdef CONFOPT_WIN32
            r = closesocket(fs->sock);
#else
            r = close(fs->sock);
#endif
            fs->sock = -1;
        }
    }

    return r;
}


static void file_destroy(mpdm_ex_t v)
/* destroys and file value */
{
    file_close((mpdm_t) v);
}


mpdm_t mpdm_new_f(FILE *f)
/* creates a new file value */
{
    mpdm_t v = NULL;
    mpdm_ex_t ev;
    struct mpdm_file *fs;
    mpdm_t e;

    fs = calloc(sizeof(struct mpdm_file), 1);

    fs->sock    = -1;
    fs->is_pipe = 0;
    fs->in      = f;
    fs->out     = f;

    /* default I/O functions */
    fs->f_read = read_auto;
    fs->f_write = write_wcs;

#ifdef CONFOPT_ICONV
    /* no iconv encodings by default */
    fs->ic_enc = fs->ic_dec = (iconv_t) - 1;
#endif

    ev = (mpdm_ex_t) mpdm_new(MPDM_FILE | MPDM_FREE | MPDM_EXTENDED, fs, sizeof(struct mpdm_file));
    ev->destroy = file_destroy;
    v = (mpdm_t) ev;

    e = mpdm_hget_s(mpdm_root(), L"ENCODING");

    if (mpdm_size(e) == 0)
        e = mpdm_hget_s(mpdm_root(), L"TEMP_ENCODING");

    if (mpdm_size(e)) {
        wchar_t *enc = mpdm_string(e);

        if (wcscmp(enc, L"utf-8") == 0) {
            fs->f_read = read_utf8_bom;
            fs->f_write = write_utf8;
        }
        else
        if (wcscmp(enc, L"utf-8bom") == 0) {
            fs->f_read = read_utf8_bom;
            fs->f_write = write_utf8_bom;
        }
        else
        if (wcscmp(enc, L"iso8859-1") == 0 ||
                 wcscmp(enc, L"8bit") == 0) {
            fs->f_read = read_iso8859_1;
            fs->f_write = write_iso8859_1;
        }
        else
        if (wcscmp(enc, L"utf-16le") == 0) {
            fs->f_read = read_utf16le;
            fs->f_write = write_utf16le_bom;
        }
        else
        if (wcscmp(enc, L"utf-16be") == 0) {
            fs->f_read = read_utf16be;
            fs->f_write = write_utf16be_bom;
        }
        else
        if (wcscmp(enc, L"utf-16") == 0) {
            fs->f_read = read_utf16;
            fs->f_write = write_utf16le_bom;
        }
        else
        if (wcscmp(enc, L"utf-32le") == 0) {
            fs->f_read = read_utf32le;
            fs->f_write = write_utf32le_bom;
        }
        else
        if (wcscmp(enc, L"utf-32be") == 0) {
            fs->f_read = read_utf32be;
            fs->f_write = write_utf32be_bom;
        }
        else
        if (wcscmp(enc, L"utf-32") == 0) {
            fs->f_read = read_utf32;
            fs->f_write = write_utf32le_bom;
        }
        else {
#ifdef CONFOPT_ICONV
            mpdm_t cs = mpdm_ref(MPDM_2MBS(e->data));

            if ((fs->ic_enc = iconv_open((char *) cs->data, "WCHAR_T")) != (iconv_t) - 1 &&
                (fs->ic_dec = iconv_open("WCHAR_T", (char *) cs->data)) != (iconv_t) - 1) {

                fs->f_read  = read_iconv;
                fs->f_write = write_iconv;
            }

            mpdm_unref(cs);
#endif                          /* CONFOPT_ICONV */
        }

        mpdm_hset_s(mpdm_root(), L"TEMP_ENCODING", NULL);
    }

    return v;
}


/**
 * mpdm_close - Closes a file descriptor.
 * @fd: the value containing the file descriptor
 *
 * Closes the file descriptor.
 * [File Management]
 */
int mpdm_close(mpdm_t fd)
{
    int r;

    mpdm_ref(fd);
    r = file_close(fd);
    mpdm_unref(fd);

    return r;
}
