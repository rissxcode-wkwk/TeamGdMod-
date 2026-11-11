#!/usr/bin/env bash
# Membangun FFmpeg 7.1 (+ libx264) universal (arm64 + x86_64) untuk macOS, lalu digabung dengan lipo.
# Hasil: lib/lib*.a (dipakai CMakeLists.txt untuk macOS).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FFMPEG_REF="${FFMPEG_REF:-n7.1}"
X264_REF="${X264_REF:-stable}"
MIN="11.0"
OUT="$ROOT/lib"
mkdir -p "$OUT"

build_arch() {
    local ARCH="$1" HOST="$2"
    local WORK="$ROOT/build-ffmpeg-mac-$ARCH" PREFIX="$ROOT/build-ffmpeg-mac-$ARCH/prefix"
    local FLAGS="-arch $ARCH -mmacosx-version-min=$MIN"
    mkdir -p "$WORK" "$PREFIX"; cd "$WORK"

    echo "== x264 ($ARCH)"
    [ -d x264 ] || git clone --depth 1 -b "$X264_REF" https://code.videolan.org/videolan/x264.git x264
    pushd x264 >/dev/null
    CC="clang $FLAGS" ./configure --host="$HOST" --prefix="$PREFIX" \
        --enable-static --enable-pic --disable-cli --disable-opencl
    make -j"$(sysctl -n hw.ncpu)"; make install
    popd >/dev/null

    echo "== FFmpeg ($ARCH)"
    [ -d ffmpeg ] || git clone --depth 1 -b "$FFMPEG_REF" https://github.com/FFmpeg/FFmpeg.git ffmpeg
    pushd ffmpeg >/dev/null
    export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
    ./configure --prefix="$PREFIX" \
        --enable-cross-compile --target-os=darwin --arch="$ARCH" \
        --cc="clang $FLAGS" --cxx="clang++ $FLAGS" \
        --pkg-config-flags=--static \
        --disable-autodetect --disable-shared --enable-static --enable-pic \
        --disable-programs --disable-doc --disable-debug --disable-avdevice \
        --enable-gpl --enable-version3 --enable-libx264 \
        --enable-zlib --enable-videotoolbox --enable-audiotoolbox \
        --extra-cflags="-I$PREFIX/include -fvisibility=hidden -O3" \
        --extra-ldflags="-L$PREFIX/lib"
    make -j"$(sysctl -n hw.ncpu)"; make install
    popd >/dev/null
}

build_arch arm64  aarch64-apple-darwin
build_arch x86_64 x86_64-apple-darwin

for lib in avcodec avformat avutil avfilter swresample swscale postproc x264; do
    A="$ROOT/build-ffmpeg-mac-arm64/prefix/lib/lib$lib.a"
    X="$ROOT/build-ffmpeg-mac-x86_64/prefix/lib/lib$lib.a"
    if [ -f "$A" ] && [ -f "$X" ]; then lipo -create "$A" "$X" -output "$OUT/lib$lib.a"; fi
done
echo "== Selesai. Isi $OUT:"; ls -la "$OUT"
