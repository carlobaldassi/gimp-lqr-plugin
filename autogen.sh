#!/bin/sh

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a cvs checkout.  You need a
# couple of extra tools to run this script successfully.
#
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.

PROJECT="GIMP Liquid Rescale Plug-In"
TEST_TYPE=-f
FILE=src/render.c

ACLOCAL=${ACLOCAL-aclocal-1.9}
AUTOCONF=${AUTOCONF-autoconf}
AUTOHEADER=${AUTOHEADER-autoheader}
AUTOMAKE=${AUTOMAKE-automake-1.9}
LIBTOOLIZE=${LIBTOOLIZE-libtoolize}

AUTOCONF_REQUIRED_VERSION=2.54
AUTOMAKE_REQUIRED_VERSION=1.10
GLIB_REQUIRED_VERSION=2.0.0
INTLTOOL_REQUIRED_VERSION=0.17

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
ORIGDIR=`pwd`
cd $srcdir

check_version ()
{
    if expr $1 \>= $2 > /dev/null; then
        echo "yes (version $1)"
    else
        echo "Too old (found version $1)!"
        DIE=1
    fi
}

echo
echo "I am testing that you have the required versions of autoconf," 
echo "automake, glib-gettextize and intltoolize..."
echo

DIE=0

printf "checking for autoconf >= %s ... " "$AUTOCONF_REQUIRED_VERSION"
if (autoconf --version) < /dev/null > /dev/null 2>&1; then
    VER=`autoconf --version \
         | grep -iw autoconf | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $AUTOCONF_REQUIRED_VERSION
else
    echo
    echo "  You must have autoconf installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    DIE=1;
fi

printf "checking for automake >= %s ... " "$AUTOMAKE_REQUIRED_VERSION"
if ($AUTOMAKE --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=$AUTOMAKE
    ACLOCAL=$ACLOCAL
elif (automake-1.19 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.19
    ACLOCAL=aclocal-1.19
elif (automake-1.18 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.18
    ACLOCAL=aclocal-1.18
elif (automake-1.17 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.17
    ACLOCAL=aclocal-1.17
elif (automake-1.16 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.16
    ACLOCAL=aclocal-1.16
elif (automake-1.15 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.15
    ACLOCAL=aclocal-1.15
elif (automake-1.14 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.14
    ACLOCAL=aclocal-1.14
elif (automake-1.13 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.13
    ACLOCAL=aclocal-1.13
elif (automake-1.12 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.12
    ACLOCAL=aclocal-1.12
elif (automake-1.11 --version) < /dev/null > /dev/null 2>&1; then
    AUTOMAKE=automake-1.11
    ACLOCAL=aclocal-1.11
else
    echo
    echo "  You must have automake $AUTOMAKE_REQUIRED_VERSION or newer installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/automake/"
    echo
    DIE=1
fi

if test x$AUTOMAKE != x; then
    VER=`$AUTOMAKE --version \
         | grep automake | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $AUTOMAKE_REQUIRED_VERSION
fi

printf "checking for glib-gettextize >= %s ... " "$GLIB_REQUIRED_VERSION"
if (glib-gettextize --version) < /dev/null > /dev/null 2>&1; then
    VER=`glib-gettextize --version \
         | grep glib-gettextize | sed "s/.* \([0-9.]*\)/\1/"`
    check_version $VER $GLIB_REQUIRED_VERSION
else
    echo
    echo "  You must have glib-gettextize installed to compile $PROJECT."
    echo "  glib-gettextize is part of glib-2.0, so you should already"
    echo "  have it. Make sure it is in your PATH."
    DIE=1
fi

printf "checking for intltool >= %s ... " "$INTLTOOL_REQUIRED_VERSION"
if (intltoolize --version) < /dev/null > /dev/null 2>&1; then
    VER=`intltoolize --version \
         | grep intltoolize | sed "s/.* \([0-9.]*\)/\1/"`
    check_version $VER $INTLTOOL_REQUIRED_VERSION
else
    echo
    echo "  You must have intltool installed to compile $PROJECT."
    echo "  Get the latest version from"
    echo "  ftp://ftp.gnome.org/pub/GNOME/sources/intltool/"
    DIE=1
fi

if test "$DIE" -eq 1; then
    echo
    echo "Please install/upgrade the missing tools and call me again."
    echo  
    exit 1
fi


test $TEST_TYPE $FILE || {
    echo
    echo "You must run this script in the top-level $PROJECT directory."
    echo
    exit 1
}


echo
echo "I am going to run ./configure with the following arguments:"
echo
echo "  --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS $@"
echo

if test -z "$*"; then
    echo "If you wish to pass additional arguments, please specify them "
    echo "on the $0 command line or set the AUTOGEN_CONFIGURE_ARGS "
    echo "environment variable."
    echo
fi

if test -z "$ACLOCAL_FLAGS"; then

    acdir=`$ACLOCAL --print-ac-dir`
    m4list="glib-gettext.m4 intltool.m4"

    for file in $m4list
    do
        if [ ! -f "$acdir/$file" ]; then
            echo
            echo "WARNING: aclocal's directory is $acdir, but..."
            echo "         no file $acdir/$file"
            echo "         You may see fatal macro warnings below."
            echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
            echo "         environment variable to \"-I /some/dir\", or install"
            echo "         $acdir/$file."
            echo
        fi
    done
fi

rm -rf autom4te.cache

$ACLOCAL $ACLOCAL_FLAGS
RC=$?
if test $RC -ne 0; then
    echo "$ACLOCAL gave errors. Please fix the error conditions and try again."
    exit 1
fi

# optionally feature autoheader
(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader || exit 1

$AUTOMAKE --add-missing || exit 1
autoconf || exit 1

glib-gettextize --copy --force || exit 1
intltoolize --copy --force --automake || exit 1

cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS "$@"
RC=$?
if test $RC -ne 0; then
    echo
    echo "Configure failed or did not finish!"
    exit $RC
fi

echo
echo "Now type 'make' to compile $PROJECT."
