#!/usr/bin/env bash
# Builds a standalone Windows copy of the game in dist/ and zips it.
# The result runs on any 64-bit Windows PC, no MSYS2 or Qt install needed.
#
# Run from the project folder in Git Bash or the MSYS2 UCRT64 terminal:
#   ./package.sh
set -euo pipefail

export PATH=/c/msys64/ucrt64/bin:$PATH

APP=MazeSolver
OUT=dist/$APP
ZIP=dist/$APP-windows-x64.zip

echo "== Building optimized (Release) version"
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 > /dev/null
cmake --build build-release

echo "== Copying the app and the Qt files it needs"
rm -rf "$OUT" "$ZIP"
mkdir -p "$OUT"
cp build-release/$APP.exe "$OUT/"
strip "$OUT/$APP.exe"
windeployqt6 --release --no-translations --no-system-d3d-compiler --no-opengl-sw \
    --no-compiler-runtime --skip-plugin-types tls,networkinformation,generic \
    "$OUT/$APP.exe" > /dev/null
rm -f "$OUT/Qt6Network.dll" # not used by the game

echo "== Copying the other DLLs those files depend on"
# windeployqt only copies Qt itself, so pull in every MSYS2 DLL the app and
# Qt need (compiler runtime, ICU, fonts, images...). Repeat until nothing new.
while :; do
    added=0
    for dep in $(find "$OUT" -iname '*.exe' -o -iname '*.dll' | while read -r f; do
                     ldd "$f" 2>/dev/null | awk '/\/ucrt64\//{print $3}'
                 done | sort -u); do
        if [ ! -f "$OUT/$(basename "$dep")" ]; then
            cp "$dep" "$OUT/"
            added=1
        fi
    done
    [ "$added" = 0 ] && break
done

echo "== Zipping"
powershell -NoProfile -Command "Compress-Archive -Path '$OUT' -DestinationPath '$ZIP'"

echo "Done:"
echo "  $OUT/$APP.exe   (double-click to play)"
echo "  $ZIP   (share this)"
