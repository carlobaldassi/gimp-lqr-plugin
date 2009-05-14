#!/bin/sh

# USAGE: winpack.sh [ --with-dll ] [ --skip-compile ] [ --portable ]

set -e

P_VERSION="0.6.1"
LIB_VERSION="0.4.1"
WITHDLL=0
SKIPCOMPILE=0
PORTABLE=0

if [ "$1" == "--with-dll" ]
then
	WITHDLL=1
	shift;
fi

if [ "$1" == "--skip-compile" ]
then
	SKIPCOMPILE=1
	shift;
fi

if [ "$1" == "--portable" ]
then
	PORTABLE=1
	shift;
fi

if [ -n "$1" ]
then
	echo "error: unknown option $1" > /dev/stderr
	exit 1;
fi

FR_DIR="${PWD}"

if [ $WITHDLL -eq 0 ]
then
	ADDCF="-O2 -g -DLQR_DISABLE_DECLSPEC"
else
	ADDCF="-O2 -g"
fi
if [ $PORTABLE -eq 0 ]
then
	PREFIX="//ROGRA~1/GIMP-2.0"
else
	PREFIX="//ortableApps/GIMPPortable/App/gimp"
fi

pushd ~/gimp-lqr-plugin-${P_VERSION}
if [ $SKIPCOMPILE -eq 0 ]
then
	configure --prefix="$PREFIX"
	make clean
	make CFLAGS="$ADDCF"
	pushd src
	vim -b gimp-lqr-plugin.exe
	popd
	configure --prefix="${FR_DIR}"
fi
make install
popd

P_NAME="gimp-lqr-plugin_${P_VERSION}"
LIB_NAME="liblqr_${LIB_VERSION}"
if [ $PORTABLE -eq 0 ]
then
	SUFFIX="_win32"
else
	SUFFIX="_win32-PortableApps"
fi
ZIPFILE="${P_NAME}-${LIB_NAME}${SUFFIX}.zip"
ICONFILE="LqR_icon.ico"

ZIPS="lib share"
USER="${HOME#/home/}"
DESKTOP="/c/Documents and Settings/${USER}/Desktop/"


SRCDIR="/mingw/"
PLUGINSDIR="lib/gimp/2.0/plug-ins"
BINFILES="gimp-lqr-plugin.exe plug_in_lqr_iter.exe"

LIBLQR_DLL_DIR="bin"
LIBLQR_DLL="liblqr-1-0.dll"

SCRIPTSDIR="share/gimp/2.0/scripts"
SCRIPTS="batch-gimp-lqr.scm"

#cd ~/fakeroot

mkdir -p "$PLUGINSDIR"
for FILE in $BINFILES
do
	cp "${SRCDIR}${PLUGINSDIR}/${FILE}" "${PLUGINSDIR}"
done
if [ $WITHDLL -eq 1 ]
then
	cp "${SRCDIR}${LIBLQR_DLL_DIR}/${LIBLQR_DLL}" "${PLUGINSDIR}"
fi

mkdir -p "$SCRIPTSDIR"
for FILE in $SCRIPTS
do
	cp "${SRCDIR}${SCRIPTSDIR}/${FILE}" "${SCRIPTSDIR}"
done

zip -r -9 "$ZIPFILE" $ZIPS

mv "$ZIPFILE" "$DESKTOP"
cp "$ICONFILE" "$DESKTOP"
rm -r $ZIPS

