# PlayStation Vita

## Building FFmpeg with movie support

The FFmpeg bundled with VitaSDK omits the Smacker and Bink demuxers and decoders required by MM7 movies. `scripts/vita.sh build` runs `scripts/build_vita_ffmpeg.sh` automatically when its FFmpeg prefix is missing. To build it separately, run:

```sh
VITASDK=/path/to/vitasdk ./scripts/build_vita_ffmpeg.sh
```

The script builds matching FFmpeg 9.0.1 with VitaSDK's audio support plus the Smacker and Bink support used by the game. FFmpeg is installed under `build-vita-debug/ffmpeg-vita` and does not replace files in VitaSDK. CMake prefers that build-tree prefix for the Vita target.

Set `BUILD_DIR` to choose another build directory. Set `JOBS` to control the FFmpeg parallel build.

## Packaging a VPK

`scripts/vita.sh vpk` builds the game and packages `build-vita-debug/OpenEnroth.vpk` with the bubble icon and LiveArea images from `packaging/vita/sce_sys`. `scripts/vita.sh deploy-vpk` uploads it to `ux0:/data/OpenEnroth.vpk` for installing from VitaShell. The game data in `ux0:/data/OpenEnroth` survives a reinstall. The images are generated from `resources/images/OpenEnroth.png` by `scripts/make_vita_livearea.py`.

## Vita controls and rendering

vitaGL lacks texture arrays, so the renderer emulates them with 2D atlases limited by the device's maximum texture size. The right stick turns and looks in the game view, and moves a virtual cursor elsewhere. The L2 and R2 triggers click the cursor.

## Movie data and settings

Movie playback reads VID entries on demand. It does not copy an entire movie entry into memory. The game still needs the MM7 data files in the configured game data directory.

The Vita defaults now allow the intro and other movies to play. Existing `ux0:/data/OpenEnroth/openenroth.ini` settings override defaults. If movies remain disabled, edit `no_intro` and `no_video` in that file and set both to `false`.
