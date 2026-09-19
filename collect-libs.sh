#!/bin/bash

set -e

APP_DIR="$(cd "$(dirname "$0")" && pwd)"
APP="$APP_DIR/dropbox-client"
LIBS="$APP_DIR/.libs"

if [ ! -f "$APP" ]; then
    echo "ERROR: dropbox-client not found:"
    echo "$APP"
    exit 1
fi

echo "==> DropBox portable library collector"
echo "    App:  $APP"
echo "    Libs: $LIBS"
echo

rm -rf "$LIBS"
mkdir -p "$LIBS"

copy_file()
{
    local FILE="$1"

    if [ -f "$FILE" ]; then
        local NAME
        NAME="$(basename "$FILE")"

        if [ ! -f "$LIBS/$NAME" ]; then
            echo "  + $NAME"
            cp -L "$FILE" "$LIBS/$NAME"
        fi
    fi
}

copy_library()
{
    local LIB="$1"

    if [ -z "$LIB" ]; then
        return
    fi

    if [ -f "$LIB" ]; then
        copy_file "$LIB"
    fi
}

echo "==> Collecting ELF dependencies..."

ldd "$APP" 2>/dev/null |
while read -r LINE; do

    LIB="$(echo "$LINE" |
        sed -n 's/.*=> \([^ ]*\).*/\1/p')"

    if [ -n "$LIB" ]; then
        copy_library "$LIB"
    else
        LIB="$(echo "$LINE" |
            awk '{print $1}')"

        case "$LIB" in
            /*)
                copy_library "$LIB"
                ;;
        esac
    fi

done


echo
echo "==> Collecting Qt WebEngine libraries..."

QT_LIB_DIRS=()

QT_CORE="$(qmake6 -query QT_INSTALL_LIBS 2>/dev/null || true)"

if [ -n "$QT_CORE" ]; then
    QT_LIB_DIRS+=("$QT_CORE")
fi

if command -v pkg-config >/dev/null 2>&1; then

    while read -r DIR; do
        if [ -n "$DIR" ]; then
            QT_LIB_DIRS+=("$DIR")
        fi
    done < <(
        pkg-config \
            --variable=libdir \
            Qt6Core \
            Qt6Gui \
            Qt6Widgets \
            Qt6WebEngineCore \
            Qt6WebEngineWidgets \
            2>/dev/null |
        sort -u
    )

fi


echo "==> Searching Qt directories..."

for DIR in "${QT_LIB_DIRS[@]}"; do

    [ -d "$DIR" ] || continue

    find "$DIR" \
        -maxdepth 2 \
        -type f \
        \( \
            -name "libQt6*.so*" \
            -o -name "libQt6*.so" \
        \) \
        -print0 2>/dev/null |
    while IFS= read -r -d '' FILE; do
        copy_file "$FILE"
    done

done


echo
echo "==> Collecting Qt WebEngineProcess..."

WEBENGINE_PROCESS="$(
    find \
        /usr/lib \
        /usr/lib64 \
        /lib \
        /lib64 \
        /usr \
        -type f \
        -name "QtWebEngineProcess" \
        2>/dev/null |
    head -n 1
)"

if [ -n "$WEBENGINE_PROCESS" ]; then
    copy_file "$WEBENGINE_PROCESS"

    echo "  + QtWebEngineProcess"
else
    echo "  ! QtWebEngineProcess not found"
fi


echo
echo "==> Collecting Qt WebEngine resources..."

RESOURCE_FILES=(
    "qtwebengine_resources.pak"
    "qtwebengine_resources_100p.pak"
    "qtwebengine_resources_200p.pak"
    "icudtl.dat"
    "v8_context_snapshot.bin"
    "snapshot_blob.bin"
)

for FILE_NAME in "${RESOURCE_FILES[@]}"; do

    FOUND="$(
        find \
            /usr/lib \
            /usr/lib64 \
            /usr/share \
            /usr \
            -type f \
            -name "$FILE_NAME" \
            2>/dev/null |
        head -n 1
    )"

    if [ -n "$FOUND" ]; then
        copy_file "$FOUND"
    fi

done


echo
echo "==> Collecting Qt WebEngine translations..."

find \
    /usr/share \
    /usr/lib \
    /usr/lib64 \
    -type f \
    \( \
        -path "*/qtwebengine_locales/*.pak" \
        -o -path "*/qt6/translations/*WebEngine*.qm" \
        -o -path "*/qt6/translations/*webengine*.qm" \
    \) \
    -print0 2>/dev/null |
while IFS= read -r -d '' FILE; do
    copy_file "$FILE"
done


echo
echo "==> Collecting Chromium/Qt WebEngine data..."

find \
    /usr/lib \
    /usr/lib64 \
    /usr/share \
    -type f \
    \( \
        -name "*.pak" \
        -o -name "*.bin" \
    \) \
    2>/dev/null |
while IFS= read -r -d '' FILE; do

    NAME="$(basename "$FILE")"

    case "$NAME" in
        qtwebengine_*)
            copy_file "$FILE"
            ;;
        chrome_*.pak)
            copy_file "$FILE"
            ;;
        resources.pak)
            copy_file "$FILE"
            ;;
    esac

done


echo
echo "==> Collecting remaining shared-library dependencies..."

find "$LIBS" \
    -type f \
    -name "*.so*" \
    -print0 |
while IFS= read -r -d '' LIB; do

    ldd "$LIB" 2>/dev/null |
    while read -r LINE; do

        DEP="$(echo "$LINE" |
            sed -n 's/.*=> \([^ ]*\).*/\1/p')"

        if [ -n "$DEP" ] && [ -f "$DEP" ]; then
            copy_library "$DEP"
        else
            DEP="$(echo "$LINE" |
                awk '{print $1}')"

            case "$DEP" in
                /*)
                    copy_library "$DEP"
                    ;;
            esac
        fi

    done

done


echo
echo "==> Creating local library configuration..."

cat > "$LIBS/README.txt" <<EOF
DropBox portable runtime

This directory contains libraries and runtime files
collected for:

dropbox-client

Do not delete files from this directory.
EOF


echo
echo "==> Done."
echo
echo "Portable layout:"
echo
echo "  dropbox-client"
echo "  icon.png"
echo "  .libs/"
echo
echo "Files collected:"
find "$LIBS" -maxdepth 1 -type f | wc -l
