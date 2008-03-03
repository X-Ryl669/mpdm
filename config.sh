#!/bin/sh

# Configuration shell script

# gets program version
VERSION=`cut -f2 -d\" VERSION`

# default installation prefix
PREFIX=/usr/local

# installation directory for documents
DOCDIR=""

# parse arguments
while [ $# -gt 0 ] ; do

	case $1 in
	--without-win32)	WITHOUT_WIN32=1 ;;
	--without-unix-glob)	WITHOUT_UNIX_GLOB=1 ;;
	--with-included-regex)	WITH_INCLUDED_REGEX=1 ;;
	--with-pcre)		WITH_PCRE=1 ;;
	--without-gettext)	WITHOUT_GETTEXT=1 ;;
	--without-iconv)	WITHOUT_ICONV=1 ;;
	--without-wcwidth)	WITHOUT_WCWIDTH=1 ;;
	--help)			CONFIG_HELP=1 ;;

	--mingw32)		CC=i586-mingw32msvc-cc
				WINDRES=i586-mingw32msvc-windres
				AR=i586-mingw32msvc-ar
				;;

	--prefix)		PREFIX=$2 ; shift ;;
	--prefix=*)		PREFIX=`echo $1 | sed -e 's/--prefix=//'` ;;

	--docdir)		DOCDIR=$2 ; shift ;;
	--docdir=*)		DOCDIR=`echo $1 | sed -e 's/--docdir=//'` ;;

	esac

	shift
done

if [ "$CONFIG_HELP" = "1" ] ; then

	echo "Available options:"
	echo "--prefix=PREFIX       Installation prefix ($PREFIX)."
	echo "--docdir=DOCDIR       Instalation directory for documentation."
	echo "--without-win32       Disable win32 interface detection."
	echo "--without-unix-glob   Disable glob.h usage (use workaround)."
	echo "--with-included-regex Use included regex code (gnu_regex.c)."
	echo "--with-pcre           Enable PCRE library detection."
	echo "--without-gettext     Disable gettext usage."
	echo "--without-iconv       Disable iconv usage."
	echo "--without-wcwidth     Disable system wcwidth() (use Marcus Kuhn's)."
	echo "--mingw32             Build using the mingw32 compiler."

	echo
	echo "Environment variables:"
	echo "CC                    C Compiler."
	echo "AR                    Library Archiver."

	exit 1
fi

if [ "$DOCDIR" = "" ] ; then
	DOCDIR=$PREFIX/share/doc/mpdm
fi

echo "Configuring MPDM..."

echo "/* automatically created by config.sh - do not modify */" > config.h
echo "# automatically created by config.sh - do not modify" > makefile.opts
> config.ldflags
> config.cflags
> .config.log

# set compiler
if [ "$CC" = "" ] ; then
	CC=cc
	# if CC is unset, try if gcc is available
	which gcc > /dev/null

	if [ $? = 0 ] ; then
		CC=gcc
	fi
fi

echo "CC=$CC" >> makefile.opts

# set cflags
if [ "$CFLAGS" = "" -a "$CC" = "gcc" ] ; then
	CFLAGS="-g -Wall"
fi

echo "CFLAGS=$CFLAGS" >> makefile.opts

# Add CFLAGS to CC
CC="$CC $CFLAGS"

# set archiver
if [ "$AR" = "" ] ; then
	AR=ar
fi

echo "AR=$AR" >> makefile.opts

# add version
cat VERSION >> config.h

# add installation prefix
echo "#define CONFOPT_PREFIX \"$PREFIX\"" >> config.h

#########################################################

# configuration directives

# Win32
echo -n "Testing for win32... "
if [ "$WITHOUT_WIN32" = "1" ] ; then
	echo "Disabled by user"
else
	echo "#include <windows.h>" > .tmp.c
	echo "#include <commctrl.h>" >> .tmp.c
	echo "int STDCALL WinMain(HINSTANCE h, HINSTANCE p, LPSTR c, int m)" >> .tmp.c
	echo "{ return 0; }" >> .tmp.c

	TMP_LDFLAGS=""
	$CC .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "#define CONFOPT_WIN32 1" >> config.h
		echo "OK"
		WITHOUT_UNIX_GLOB=1
	else
		echo "No"
	fi
fi

# glob.h support
if [ "$WITHOUT_UNIX_GLOB" != 1 ] ; then
	echo -n "Testing for unix-like glob.h... "
	echo "#include <stdio.h>" > .tmp.c
	echo "#include <glob.h>" >> .tmp.c
	echo "int main(void) { glob_t g; g.gl_offs=1; glob(\"*\",GLOB_MARK,NULL,&g); return 0; }" >> .tmp.c

	$CC .tmp.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "#define CONFOPT_GLOB_H 1" >> config.h
		echo "OK"
	else
		echo "No; activating workaround"
	fi
fi

# regex
echo -n "Testing for regular expressions... "

if [ "$WITH_PCRE" = 1 ] ; then
	# try first the pcre library
	TMP_CFLAGS="-I/usr/local/include"
	TMP_LDFLAGS="-L/usr/local/lib -lpcre -lpcreposix"
	echo "#include <pcreposix.h>" > .tmp.c
	echo "int main(void) { regex_t r; regmatch_t m; regcomp(&r,\".*\",REG_EXTENDED|REG_ICASE); return 0; }" >> .tmp.c

	$CC $TMP_CFLAGS .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK (using pcre library)"
		echo "#define CONFOPT_PCRE 1" >> config.h
		echo "$TMP_CFLAGS " >> config.cflags
		echo "$TMP_LDFLAGS " >> config.ldflags
		REGEX_YET=1
	fi
fi

if [ "$REGEX_YET" != 1 -a "$WITH_INCLUDED_REGEX" != 1 ] ; then
	echo "#include <sys/types.h>" > .tmp.c
	echo "#include <regex.h>" >> .tmp.c
	echo "int main(void) { regex_t r; regmatch_t m; regcomp(&r,\".*\",REG_EXTENDED|REG_ICASE); return 0; }" >> .tmp.c

	$CC .tmp.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK (using system one)"
		echo "#define CONFOPT_SYSTEM_REGEX 1" >> config.h
		REGEX_YET=1
	fi
fi

if [ "$REGEX_YET" != 1 ] ; then
	# if system libraries lack regex, try compiling the
	# included gnu_regex.c

	$CC -c -DSTD_HEADERS -DREGEX gnu_regex.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK (using included gnu_regex.c)"
		echo "#define HAVE_STRING_H 1" >> config.h
		echo "#define REGEX 1" >> config.h
		echo "#define CONFOPT_INCLUDED_REGEX 1" >> config.h
	else
		echo "#define CONFOPT_NO_REGEX 1" >> config.h
		echo "No (No usable regex library)"
		exit 1
	fi
fi

# unistd.h detection
echo -n "Testing for unistd.h... "
echo "#include <unistd.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_UNISTD_H 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# sys/types.h detection
echo -n "Testing for sys/types.h... "
echo "#include <sys/types.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_SYS_TYPES_H 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# sys/wait.h detection
echo -n "Testing for sys/wait.h... "
echo "#include <sys/wait.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_SYS_WAIT_H 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# sys/stat.h detection
echo -n "Testing for sys/stat.h... "
echo "#include <sys/stat.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_SYS_STAT_H 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# pwd.h detection
echo -n "Testing for pwd.h... "
echo "#include <pwd.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_PWD_H 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# chown() detection
echo -n "Testing for chown()... "
echo "#include <sys/types.h>" > .tmp.c
echo "#include <unistd.h>" >> .tmp.c
echo "int main(void) { chown(\"file\", 0, 0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
	echo "#define CONFOPT_CHOWN 1" >> config.h
	echo "OK"
else
	echo "No"
fi

# gettext support
echo -n "Testing for gettext... "

if [ "$WITHOUT_GETTEXT" = "1" ] ; then
	echo "Disabled by user"
else
	echo "#include <libintl.h>" > .tmp.c
	echo "#include <locale.h>" >> .tmp.c
	echo "int main(void) { setlocale(LC_ALL, \"\"); gettext(\"hi\"); return 0; }" >> .tmp.c

	# try first to compile without -lintl
	$CC .tmp.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK"
		echo "#define CONFOPT_GETTEXT 1" >> config.h
	else
		# try now with -lintl
		TMP_LDFLAGS="-lintl"

		$CC .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

		if [ $? = 0 ] ; then
			echo "OK (libintl needed)"
			echo "#define CONFOPT_GETTEXT 1" >> config.h
			echo "$TMP_LDFLAGS" >> config.ldflags
		else
			echo "No"
		fi
	fi
fi


# iconv support
echo -n "Testing for iconv... "

if [ "$WITHOUT_ICONV" = "1" ] ; then
	echo "Disabled by user"
else
	echo "#include <iconv.h>" > .tmp.c
	echo "#include <locale.h>" >> .tmp.c
	echo "int main(void) { setlocale(LC_ALL, \"\"); iconv_open(\"WCHAR_T\", \"ISO-8859-1\"); return 0; }" >> .tmp.c

	# try first to compile without -liconv
	$CC .tmp.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK"
		echo "#define CONFOPT_ICONV 1" >> config.h
	else
		# try now with -liconv
		TMP_LDFLAGS="-liconv"

		$CC .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

		if [ $? = 0 ] ; then
			echo "OK (libiconv needed)"
			echo "#define CONFOPT_ICONV 1" >> config.h
			echo "$TMP_LDFLAGS" >> config.ldflags
		else
			echo "No"
		fi
	fi
fi

# wcwidth() existence
echo -n "Testing for wcwidth()... "

if [ "$WITHOUT_WCWIDTH" = "1" ] ; then
	echo "Disabled by user (using Markus Kuhn's)"
else
	echo "#include <wchar.h>" > .tmp.c
	echo "int main(void) { wcwidth(L'a'); return 0; }" >> .tmp.c

	$CC .tmp.c -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "OK"
		echo "#define CONFOPT_WCWIDTH 1" >> config.h
	else
		echo "No; using Markus Kuhn's wcwidth()"
	fi
fi

# test for Grutatxt
echo -n "Testing if Grutatxt is installed... "

DOCS="\$(ADD_DOCS)"

if which grutatxt > /dev/null ; then
	echo "OK"
	echo "GRUTATXT=yes" >> makefile.opts
	DOCS="$DOCS \$(GRUTATXT_DOCS)"
else
	echo "No"
	echo
	echo "Grutatxt not found; some documentation will not be generated."
	echo "You can take it from http://www.triptico.com/software/grutatxt.html"
	echo
	echo "GRUTATXT=no" >> makefile.opts
fi

# test for mp_doccer
echo -n "Testing if mp_doccer is installed... "
MP_DOCCER=$(which mp_doccer || which mp-doccer)

if [ $? == 0 ] ; then
	echo "OK"
	echo "MP_DOCCER=yes" >> makefile.opts
	DOCS="$DOCS \$(MP_DOCCER_DOCS)"
else
	echo "No"
	echo
	echo "mp_doccer not found; some documentation will not be generated."
	echo "You can take it from http://www.triptico.com/software/mp_doccer.html"
	echo "MP_DOCCER=no" >> makefile.opts
fi

if [ "$WITH_NULL_HASH" = "1" ] ; then
	echo "Selecting NULL hash function"

	echo "#define CONFOPT_NULL_HASH 1" >> config.h
fi

#########################################################

# final setup

echo "DOCS=$DOCS" >> makefile.opts
echo "VERSION=$VERSION" >> makefile.opts
echo "PREFIX=\$(DESTDIR)$PREFIX" >> makefile.opts
echo "DOCDIR=\$(DESTDIR)$DOCDIR" >> makefile.opts
echo >> makefile.opts

cat makefile.opts makefile.in makefile.depend > Makefile

#########################################################

# cleanup

rm -f .tmp.c .tmp.o

exit 0
