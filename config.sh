#!/bin/sh

# Configuration shell script

# gets program version
VERSION=`cut -f2 -d\" VERSION`

# parse arguments
for a in $* ; do
	[ "$a" = "--without-win32" ] && WITHOUT_WIN32=1
	[ "$a" = "--without-unix-glob" ] && WITHOUT_UNIX_GLOB=1
	[ "$a" = "--without-regex" ] && WITHOUT_REGEX=1
	[ "$a" = "--with-included-regex" ] && WITH_INCLUDED_REGEX=1
	[ "$a" = "--without-pcre" ] && WITHOUT_PCRE=1
	[ "$a" = "--help" ] && CONFIG_HELP=1
done

if [ "$CONFIG_HELP" = "1" ] ; then

	echo "Available options:"
	echo "--without-win32       Disable win32 interface detection."
	echo "--without-unix-glob   Disable glob.h usage (use workaround)."
	echo "--with-included-regex Use included regex code (gnu_regex.c)."
	echo "--without-pcre        Disable PCRE library detection."

	echo
	echo "Environment variables:"
	echo "CC                    C Compiler."

	exit 1
fi

echo "Configuring..."

echo "/* automatically created by config.sh - do not modify */" > config.h
echo "# automatically created by config.sh - do not modify" > makefile.opts
> config.ldflags
> config.cflags
> .config.log

# set compiler
if [ "$CC" = "" ] ; then
	CC=cc
	# if CC is unset, try if gcc is available
	which gcc > /dev/null && CC=gcc
fi

# set archiver
if [ "$AR" = "" ] ; then
	AR=ar
fi

echo "CC=$CC" >> makefile.opts
echo "AR=$AR" >> makefile.opts

# add version
cat VERSION >> config.h

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

	TMP_LDFLAGS="-mwindows -lcomctl32"
	$CC .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

	if [ $? = 0 ] ; then
		echo "#define CONFOPT_WIN32 1" >> config.h
		echo "$TMP_LDFLAGS " >> config.ldflags
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
		echo "No; activated workaround"
	fi
fi

# regex
echo -n "Testing for regular expressions... "

if [ "$WITHOUT_PCRE" != 1 -a "$WITH_INCLUDED_REGEX" != 1 ] ; then
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
	fi
fi


#########################################################

# final setup

echo "VERSION=$VERSION" >> makefile.opts

cat makefile.opts makefile.in makefile.depend > Makefile

#########################################################

# cleanup

rm -f .tmp.c .tmp.o

exit 0
