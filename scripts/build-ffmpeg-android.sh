#!/usr/bin/env bash
# Membangun FFmpeg 7.1 (+ libx264) sebagai library statis untuk Android arm64-v8a.
# Hasil: lib/arm64-v8a/lib*.a (dipakai CMakeLists.txt). Versi FFmpeg harus sama dengan header di include/ (libavcodec 61.19).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NDK="${ANDROID_NDK_LATEST_HOME:-${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}}"
if [ -z "$NDK" ]; then echo "NDK tidak ditemukan (set ANDROID_NDK_HOME)"; exit 1; fi

API="${ANDROID_API:-23}"
FFMPEG_REF="${FFMPEG_REF:-n7.1}"
X264_REF="${X264_REF:-stable}"

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
BIN="$TOOLCHAIN/bin"
TRIPLE="aarch64-linux-android"
CC="$BIN/${TRIPLE}${API}-clang"
CXX="$BIN/${TRIPLE}${API}-clang++"

WORK="$ROOT/build-ffmpeg"
PREFIX="$WORK/prefix"
OUT="$ROOT/lib/arm64-v8a"
mkdir -p "$WORK" "$PREFIX" "$OUT"
cd "$WORK"

echo "== x264 ($X264_REF)"
if [ ! -d x264 ]; then git clone --depth 1 -b "$X264_REF" https://code.videolan.org/videolan/x264.git x264; fi
pushd x264 >/dev/null
CC="$CC" AS="$CC" ./configure \
    --host=aarch64-linux-android \
    --cross-prefix="$BIN/llvm-" \
    --prefix="$PREFIX" \
    --enable-static --enable-pic --disable-cli --disable-opencl
make -j"$(nproc)"
make install
popd >/dev/null

echo "== FFmpeg ($FFMPEG_REF)"
if [ ! -d ffmpeg ]; then git clone --depth 1 -b "$FFMPEG_REF" https://github.com/FFmpeg/FFmpeg.git ffmpeg; fi
pushd ffmpeg >/dev/null
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
# PENTING: -fvisibility=hidden membuat simbol FFmpeg (mis. ff_tx_tab_*) tidak preemptible.
# Tanpa ini, kode asm NEON (tx_float_neon.S) yang memakai adrp/add gagal di-link ke .so
# dengan error "relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol ... recompile with -fPIC".
./configure \
    --prefix="$PREFIX" \
    --enable-cross-compile --target-os=android --arch=aarch64 --cpu=armv8-a \
    --cc="$CC" --cxx="$CXX" \
    --ar="$BIN/llvm-ar" --nm="$BIN/llvm-nm" --ranlib="$BIN/llvm-ranlib" --strip="$BIN/llvm-strip" \
    --pkg-config=pkg-config --pkg-config-flags=--static \
    --disable-autodetect --disable-shared --enable-static --enable-pic \
    --disable-programs --disable-doc --disable-debug --disable-avdevice \
    --enable-gpl --enable-version3 --enable-libx264 \
    --enable-zlib --enable-jni --enable-mediacodec --enable-neon --enable-asm \
    --extra-cflags="-I$PREFIX/include -fPIC -fvisibility=hidden -O3" \
    --extra-ldflags="-L$PREFIX/lib"
make -j"$(nproc)"
make install
popd >/dev/null

for lib in avcodec avformat avutil avfilter swresample swscale postproc x264; do
    if [ -f "$PREFIX/lib/lib$lib.a" ]; then cp "$PREFIX/lib/lib$lib.a" "$OUT/"; fi
done
echo "== Selesai. Isi $OUT:"
ls -la "$OUT"
