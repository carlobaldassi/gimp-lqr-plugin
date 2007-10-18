#! /bin/bash

NAME=$(head -n 50 configure.in | grep "m4_define(\[plugin_name\], \[.*\])" | sed "s/m4_define(\[plugin_name\], \[\(.*\)\])/\1/")
MAJOR_VER=$(head -n 50 configure.in | grep "m4_define(\[plugin_major_version\], \[.*\])" | sed "s/m4_define(\[plugin_major_version\], \[\(.*\)\])/\1/")
MINOR_VER=$(head -n 50 configure.in | grep "m4_define(\[plugin_minor_version\], \[.*\])" | sed "s/m4_define(\[plugin_minor_version\], \[\(.*\)\])/\1/")
MICRO_VER=$(head -n 50 configure.in | grep "m4_define(\[plugin_micro_version\], \[.*\])" | sed "s/m4_define(\[plugin_micro_version\], \[\(.*\)\])/\1/")

PLUGIN_NAME="${NAME}-${MAJOR_VER}.${MINOR_VER}.${MICRO_VER}"
DEB_NAME="${NAME}_${MAJOR_VER}.${MINOR_VER}.${MICRO_VER}-1_i386.deb"
NEW_DEB_NAME="${NAME}_${MAJOR_VER}.${MINOR_VER}.${MICRO_VER}_UbuntuFeisty-i386.deb"

[ -f description-pak ] || { echo "error"; exit 1; }

sudo rm -fr "$PLUGIN_NAME" || exit 1
sudo make distdir || exit 1

cp description-pak "$PLUGIN_NAME" || exit 1

cd "$PLUGIN_NAME"

configure || exit 1
make || exit 1

sudo checkinstall \
	--pkggroup="universe/graphics" \
	--pkgaltsource="http://liquidrescale.wikidot.com" \
	--maintainer="Carlo Baldassi \<carlobaldassi@yahoo.it\>" \
	--requires="gimp \(\>= 2.2\)" \
	--install=no \
	-y \
	$@ \
	|| exit 1

cp "$DEB_NAME" ..
cd ..
sudo rm -fr "$PLUGIN_NAME"
mv "$DEB_NAME" "$NEW_DEB_NAME"
