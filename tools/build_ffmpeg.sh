#!/usr/bin/env bash
# ============================================================================
#  build_ffmpeg.sh - Cross-compile a minimal FFmpeg for the OpenOrbis PS4
#  target, producing static libs usable by the optional software decoder
#  (USE_FFMPEG=1) of PS4 IPTV Player.
#
#  This is a BEST-EFFORT integration boundary. FFmpeg has no official PS4
#  target; getting it to link and run on PS4 requires a working OpenOrbis
#  libc/pthread/socket surface. Treat this as a starting point and expect to
#  iterate on configure flags. The default app build does NOT require this.
#
#  Usage:
#     export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain
#     tools/build_ffmpeg.sh [ffmpeg-source-dir] [install-prefix]
#
#  Then build the app with:
#     make USE_FFMPEG=1 FFMPEG_DIR=<install-prefix>
# ============================================================================
set -euo pipefail

: "${OO_PS4_TOOLCHAIN:?Set OO_PS4_TOOLCHAIN to your OpenOrbis PS4Toolchain}"

FF_SRC="${1:-ffmpeg}"
PREFIX="${2:-$(pwd)/third_party/ffmpeg-ps4}"
TARGET="x86_64-pc-freebsd12-elf"

if [ ! -d "$FF_SRC" ]; then
  echo "FFmpeg source not found at '$FF_SRC'."
  echo "Clone it first, e.g.:"
  echo "  git clone --depth=1 -b n6.1 https://github.com/FFmpeg/FFmpeg.git ffmpeg"
  exit 1
fi

SYSROOT="$OO_PS4_TOOLCHAIN"
CFLAGS_COMMON="--target=$TARGET -fPIC -D_BSD_SOURCE -isysroot $SYSROOT -isystem $SYSROOT/include"

pushd "$FF_SRC" >/dev/null

./configure \
  --prefix="$PREFIX" \
  --enable-cross-compile \
  --target-os=freebsd \
  --arch=x86_64 \
  --cc=clang \
  --cxx=clang++ \
  --ld=clang \
  --ar=llvm-ar \
  --ranlib=llvm-ranlib \
  --nm=llvm-nm \
  --strip=llvm-strip \
  --extra-cflags="$CFLAGS_COMMON" \
  --extra-ldflags="--target=$TARGET -L$SYSROOT/lib" \
  --disable-programs \
  --disable-doc \
  --disable-debug \
  --disable-shared \
  --enable-static \
  --disable-everything \
  --disable-network \
  --enable-protocol=file \
  --enable-demuxer=mov,matroska,mpegts,hls \
  --enable-decoder=av1,libdav1d,vp9,vvc,hevc,h264,aac,mp3 \
  --enable-parser=av1,vp9,hevc,h264 \
  --disable-x86asm

make -j"$(nproc)"
make install

popd >/dev/null

echo
echo "FFmpeg installed to: $PREFIX"
echo "Build the app with:  make USE_FFMPEG=1 FFMPEG_DIR=$PREFIX"
