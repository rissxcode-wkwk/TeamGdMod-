#!/usr/bin/env bash
# Membangun FFmpeg 7.1 (+ libx264) dengan toolchain MSVC untuk Windows x64 (dijalankan di MSYS2,
# dengan lingkungan MSVC sudah aktif lewat ilammy/msvc-dev-cmd). Hasil: lib/*.lib
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FFMPEG_REF="${FFMPEG_REF:-n7.1}"
X264_REF="${X264_REF:-stable}"
WORK="$ROOT/build-ffmpeg-win"
PREFIX="$WORK/prefix"
OUT="$ROOT/lib"
mkdir -p "$WORK" "$PREFIX" "$OUT"
cd "$WORK"

# link.exe milik MSYS2 bentrok dengan link.exe MSVC
rm -f /usr/bin/link.exe || true

echo "== x264 ($X264_REF)"
[ -d x264 ] || git clone --depth 1 -b "$X264_REF" https://code.videolan.org/videolan/x264.git x264
pushd x264 >/dev/null
CC=cl ./configure --prefix="$PREFIX" --enable-static --disable-cli --disable-opencl \
    --extra-cflags="-MD"
make -j"$(nproc)"; make install
popd >/dev/null
# FFmpeg mencari x264.lib; x264 menghasilkan libx264.lib -> sediakan keduanya
if [ -f "$PREFIX/lib/libx264.lib" ]; then cp "$PREFIX/lib/libx264.lib" "$PREFIX/lib/x264.lib"; fi

echo "== FFmpeg ($FFMPEG_REF)"
[ -d ffmpeg ] || git clone --depth 1 -b "$FFMPEG_REF" https://github.com/FFmpeg/FFmpeg.git ffmpeg
pushd ffmpeg >/dev/null
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
./configure --prefix="$PREFIX" \
    --toolchain=msvc --arch=x86_64 --target-os=win64 \
    --disable-autodetect --disable-shared --enable-static \
    --disable-programs --disable-doc --disable-debug --disable-avdevice \
    --enable-gpl --enable-version3 --enable-libx264 \
    --extra-cflags="-MD -I$PREFIX/include" \
    --extra-ldflags="-LIBPATH:$PREFIX/lib"
make -j"$(nproc)"; make install
popd >/dev/null

# Hasil FFmpeg bisa bernama avcodec.lib atau libavcodec.a (isinya tetap format MSVC); samakan jadi *.lib
for lib in avcodec avformat avutil avfilter swresample swscale postproc; do
    for f in "$PREFIX/lib/$lib.lib" "$PREFIX/lib/lib$lib.a"; do
        if [ -f "$f" ]; then cp "$f" "$OUT/$lib.lib"; break; fi
    done
done
if [ -f "$PREFIX/lib/libx264.lib" ]; then cp "$PREFIX/lib/libx264.lib" "$OUT/libx264.lib"; fi
echo "== Selesai. Isi $OUT:"; ls -la "$OUT"
