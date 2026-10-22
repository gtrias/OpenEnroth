#!/usr/bin/env bash
#
# Build the Vita FFmpeg prefix required by scripts/vita.sh.
#
# Usage:
#   scripts/build_vita_ffmpeg.sh
#   VITASDK=/path/to/vitasdk BUILD_DIR=build-vita-debug scripts/build_vita_ffmpeg.sh

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VITASDK="${VITASDK:-$HOME/vitasdk/vitasdk}"
BUILD_DIR="${BUILD_DIR:-build-vita-debug}"
BUILD_PATH="$REPO_ROOT/$BUILD_DIR"
SOURCE_DIR="$BUILD_PATH/ffmpeg-9.0.1"
PREFIX="$BUILD_PATH/ffmpeg-vita"
ARCHIVE="$BUILD_PATH/ffmpeg-9.0.1.tar.xz"
SOURCE_URL="https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz"
JOBS="${JOBS:-$(nproc)}"

if [[ ! -f "$ARCHIVE" ]]; then
    mkdir -p "$BUILD_PATH"
    curl --fail --location --retry 3 --output "$ARCHIVE" "$SOURCE_URL"
fi
echo "cf38e0e28c7e5605942c4a77755349b0145804a397af37eb1fb4c77cb237f635  $ARCHIVE" | sha256sum --check --status || {
    printf 'FFmpeg source archive checksum mismatch: %s\n' "$ARCHIVE" >&2
    exit 1
}

if [[ ! -d "$SOURCE_DIR" ]]; then
    tar -xf "$ARCHIVE" -C "$BUILD_PATH"
fi

export PATH="$VITASDK/bin:$PATH"
export PKG_CONFIG_LIBDIR="$VITASDK/arm-vita-eabi/lib/pkgconfig"

cd "$SOURCE_DIR"
./configure \
    --prefix="$PREFIX" \
    --enable-cross-compile \
    --cross-prefix="$VITASDK/bin/arm-vita-eabi-" \
    --disable-shared \
    --disable-runtime-cpudetect \
    --disable-armv5te \
    --disable-programs \
    --disable-doc \
    --disable-network \
    --enable-libmp3lame \
    --disable-everything \
    --enable-decoder='aac,adpcm_ima_qt,alac,ape,ac3,cinepak,flac,mp3,mpeg4,opus,pcm_s16le,pcm_s24le,pcm_s32le,pcm_s8,pcm_u8,svq1,tta,vorbis,wmalossless,wavpack,smacker,smackaud,bink,binkaudio_rdft,binkaudio_dct' \
    --enable-demuxer='aac,ac3,aiff,ape,asf,avi,caf,cine,flac,matroska,mp3,mp4,m4a,pcm_s16le,pcm_s24le,pcm_s32le,pcm_s8,pcm_u8,ogg,opus,tta,wav,webm,wv,smacker,bink' \
    --enable-muxer='aac,ac3,aiff,ape,asf,avi,caf,flac,matroska,mp3,mp4,m4a,pcm_s16le,pcm_s24le,pcm_s32le,pcm_s8,pcm_u8,ogg,opus,tta,wav,webm,wv' \
    --enable-encoder='aac,alac,ape,ac3,flac,libmp3lame,mpeg4,opus,pcm_s16le,pcm_s24le,pcm_s32le,pcm_s8,pcm_u8,tta,vorbis,wmalossless,wavpack' \
    --enable-parser='aac,ac3,flac,h263,mpeg4video,mpegaudio,opus,vorbis' \
    --enable-protocol=file \
    --enable-static \
    --enable-small \
    --disable-debug \
    --arch=armv7-a \
    --cpu=cortex-a9 \
    --disable-armv6t2 \
    --target-os=none \
    --extra-cflags='-std=gnu11 -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -Wl,-q -O2 -ftree-vectorize -fomit-frame-pointer -D_BSD_SOURCE' \
    --extra-cxxflags='-Wl,-q -O2 -ftree-vectorize -fomit-frame-pointer -fno-rtti -fno-exceptions -std=gnu++11 -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -D_BSD_SOURCE' \
    --extra-ldflags="-L$VITASDK/arm-vita-eabi/lib" \
    --extra-libs=-lmpg123 \
    --disable-bzlib \
    --disable-iconv \
    --disable-lzma \
    --disable-sdl2 \
    --disable-securetransport \
    --disable-xlib \
    --enable-pthreads

make -j"$JOBS"
make install
sha256sum "$REPO_ROOT/scripts/build_vita_ffmpeg.sh" | cut -d' ' -f1 > "$PREFIX/.build_script.sha256"
printf 'Installed Vita FFmpeg 9.0.1 to %s\n' "$PREFIX"
