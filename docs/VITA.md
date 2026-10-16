# PlayStation Vita

## Building FFmpeg with movie support

The FFmpeg bundled with VitaSDK omits the Smacker and Bink demuxers and decoders required by MM7 movies. Build the matching FFmpeg 9.0.1 source with the project script before building OpenEnroth:

```sh
VITASDK=/path/to/vitasdk ./scripts/build_vita_ffmpeg.sh
VITASDK=/path/to/vitasdk ./scripts/vita.sh build
```

The script keeps VitaSDK's audio, demuxer, muxer, encoder, parser, protocol, swscale and swresample configuration, then enables the Smacker and Bink support used by the game. FFmpeg is installed under `build-vita-debug/ffmpeg-vita`. It does not replace files in VitaSDK. CMake prefers that build-tree prefix for the Vita target.

Set `BUILD_DIR` to choose another build directory. Set `JOBS` to control the FFmpeg parallel build.

## Movie data and settings

Movie playback reads VID entries on demand. It does not copy an entire movie entry into memory. The game still needs the MM7 data files in the configured game data directory.

The Vita defaults now allow the intro and other movies to play. Existing `ux0:/data/OpenEnroth/openenroth.ini` settings override defaults. If movies remain disabled, edit `no_intro` and `no_video` in that file and set both to `false`.
