#!/bin/bash
set -e

# === settings matching your request ===
PKG_NAME="dropbox-client"
VERSION="1.0"
MAINTAINER="MrCattib"
ICON_SRC="icon.png"          # put the icon.png next to this script
ARCHIVE="dropbox-client_1.0_amd64.tar"

# temporary build dir
BUILD=$(mktemp -d)
trap "rm -rf $BUILD" EXIT

# extract the app
mkdir -p "$BUILD/opt/$PKG_NAME"
tar -xf "$ARCHIVE" -C "$BUILD/opt/$PKG_NAME"

# replace icon with the one you provided
cp "$ICON_SRC" "$BUILD/opt/$PKG_NAME/icon.png"
chmod 755 "$BUILD/opt/$PKG_NAME/dropbox-client"

# desktop entry (start-menu)
mkdir -p "$BUILD/usr/share/applications"
cat > "$BUILD/usr/share/applications/$PKG_NAME.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=DropBox GUI
GenericName=DropBox GUI
Comment=Lightweight DropBox GUI App
Exec=/opt/$PKG_NAME/dropbox-client
Icon=/opt/$PKG_NAME/icon.png
Terminal=false
Categories=Network;FileTransfer;
StartupNotify=true
EOF

# DEBIAN control
mkdir -p "$BUILD/DEBIAN"
INSTALLED_SIZE=$(du -sk "$BUILD" | cut -f1)
cat > "$BUILD/DEBIAN/control" << EOF
Package: $PKG_NAME
Version: $VERSION
Section: net
Priority: optional
Architecture: amd64
Maintainer: $MAINTAINER
Installed-Size: $INSTALLED_SIZE
Description: Lightweight DropBox GUI App
 A self-contained lightweight GUI client for Dropbox.
EOF

# build the .deb
dpkg-deb -Zgzip -z1 --root-owner-group --build "$BUILD" "${PKG_NAME}_${VERSION}_amd64.deb"
echo
echo "Created: ${PKG_NAME}_${VERSION}_amd64.deb"
ls -lh "${PKG_NAME}_${VERSION}_amd64.deb"
