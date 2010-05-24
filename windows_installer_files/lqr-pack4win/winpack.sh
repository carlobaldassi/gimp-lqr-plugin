#!/bin/sh

# USAGE: winpack.sh [ --with-dll ] [ --skip-compile ]

set -e

P_VERSION="0.7.0"
LIB_VERSION="0.4.1"

FR_DIR="${PWD}"
P_NAME="gimp-lqr-plugin_${P_VERSION}"
LIB_NAME="liblqr_${LIB_VERSION}"

USER="${HOME#/home/}"
DESKTOP="/c/Documents and Settings/${USER}/Desktop/"

DESTDIR="$DESKTOP/LqR_Installer"
ICONFILE="LqR_icon.ico"



WITHDLL=0
SKIPCOMPILE=0

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

if [ -n "$1" ]
then
	echo "error: unknown option $1" > /dev/stderr
	exit 1;
fi

SUFFIX="_win32"

OUTDIR="${P_NAME}-${LIB_NAME}${SUFFIX}"
[ -d "$OUTDIR" ] && rm -r "$OUTDIR"

if [ $WITHDLL -eq 0 ]
then
	ADDCF="-O2 -g -DLQR_DISABLE_DECLSPEC"
else
	ADDCF="-O2 -g"
fi

pushd ~/gimp-lqr-plugin-${P_VERSION}
if [ $SKIPCOMPILE -eq 0 ]
then
	configure --prefix="${FR_DIR}/${OUTDIR}"
	make clean
	make CFLAGS="$ADDCF"
fi
make install
popd


SRCDIR="/mingw/"
PLUGINSDIR="lib/gimp/2.0/plug-ins"
OUT_PLUGINSDIR="${OUTDIR}/${PLUGINSDIR}"
BINFILES="gimp-lqr-plugin.exe plug_in_lqr_iter.exe"

LIBLQR_DLL_DIR="bin"
LIBLQR_DLL="liblqr-1-0.dll"

SCRIPTSDIR="share/gimp/2.0/scripts"
OUT_SCRIPTSDIR="${OUTDIR}/${SCRIPTSDIR}"
SCRIPTS="batch-gimp-lqr.scm"

SHAREDIR="share/gimp-lqr-plugin"
OUT_SHAREDIR="${OUTDIR}/${SHAREDIR}"

mkdir -p "$OUT_PLUGINSDIR"
for FILE in $BINFILES
do
	cp "${SRCDIR}${PLUGINSDIR}/${FILE}" "${OUT_PLUGINSDIR}"
done
if [ $WITHDLL -eq 1 ]
then
	cp "${SRCDIR}${LIBLQR_DLL_DIR}/${LIBLQR_DLL}" "${OUT_PLUGINSDIR}"
fi

mkdir -p "$OUT_SCRIPTSDIR"
for FILE in $SCRIPTS
do
	cp "${SRCDIR}${SCRIPTSDIR}/${FILE}" "${OUT_SCRIPTSDIR}"
done

cp "$ICONFILE" "$OUT_SHAREDIR"

FILE_LIST="file_list.log"
NSIS_MACRO="lqr_file_list.nsh"
NSIS_MACRO_PORTABLE="lqr_file_list-PortableApps.nsh"

tar -cf "$OUTDIR.tar" "$OUTDIR"
tar -tf "$OUTDIR.tar" | sed "s@^[^/]\+/@@" | sed "s@\([^/]\+\)/@zzz_\1 @g"| sort | sed "s@zzz_\([^ ]\+\) @\1\\\\@g" | awk '{ORS="\r\n"} (length($0)>0){print}' > "${OUT_SHAREDIR}/${FILE_LIST}"
rm "$OUTDIR.tar"

awk '
	BEGIN {
		print "!macro InstallFiles"
	}
	{
		if ($0 ~ /\\$/) {
			printf "  SetOutPath \"\$INSTDIR\\%s\"\n", $0
		} else {
			printf "  File \"${INPUT_DIR}\\%s\"\n", $0
		}
	}
	END {
		print "!macroend"
	}' "${OUT_SHAREDIR}/${FILE_LIST}" > "${OUTDIR}/${NSIS_MACRO}"

awk '
	BEGIN {
		print "!macro InstallFiles"
	}
	{
		if ($0 ~ /\\$/) {
			printf "  SetOutPath \"\$INSTDIR\\App\\gimp\\%s\"\n", $0
		} else {
			printf "  File \"${INPUT_DIR}\\%s\"\n", $0
		}
	}
	END {
		print "!macroend"
	}' "${OUT_SHAREDIR}/${FILE_LIST}" > "${OUTDIR}/${NSIS_MACRO_PORTABLE}"

if [ -d "${DESTDIR}/${OUTDIR}" ]
then
	rm -r "${DESTDIR}/${OUTDIR}"
fi

mv "$OUTDIR" "$DESTDIR"

