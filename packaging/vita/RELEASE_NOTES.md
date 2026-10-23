**Beta.** A native PS Vita port of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth), the open source engine for
Might and Magic VII. It boots, plays the intro movies with sound, and runs into the game, but the outdoor maps run
at about 5 fps for now. Expect bugs.

This build was developed with heavy AI assistance.

## Requirements

- A PS Vita or PS TV with HENkaku or Ensō, and "Enable unsafe homebrew" turned on in HENkaku Settings.
- Your own copy of Might and Magic VII (GOG or the original CD). The game data is copyrighted and is not included.

## Installing

1. Copy these folders from your Might and Magic VII install to `ux0:/data/OpenEnroth` on the Vita: `Anims`, `DATA`,
   `Music`, `SOUNDS`. Folder name case doesn't matter.
2. Copy `OpenEnroth.vpk` to the Vita and install it with VitaShell.
3. Launch OpenEnroth from the LiveArea. The first launch compiles the shaders and sits on a black screen for about
   20 seconds. Later launches skip that step.

Settings live in `ux0:/data/OpenEnroth/openenroth.ini`, logs in `ux0:/data/OpenEnroth/logs`.

## Controls

| Input | Action |
|---|---|
| Left stick | Walk and strafe |
| Right stick | Turn and look in the game view, move the cursor elsewhere |
| Front touch screen | Click |
| Cross | Turn-based mode on and off |
| Square | Interact |
| Triangle | Inventory |
| Circle | Spellbook |
| L | Attack |
| R | Quick spell |
| D-pad up / left / right | Autonotes / map / quests |

## Known issues

- Outdoor maps run at about 5 fps.
- The game can freeze right after the main menu music starts. Close it from the LiveArea and launch it again.
- The first launch can freeze while compiling shaders. Launching again usually gets past it, and once the shaders
  are cached it doesn't happen again.

## Source and licenses

Source: https://github.com/gtrias/OpenEnroth/tree/vita-port. OpenEnroth is licensed under the LGPL 3.0. The VPK also
contains FFmpeg (LGPL 2.1, built without GPL components), LAME and mpg123 (LGPL 2.1), OpenAL Soft (LGPL 2.0),
vitaGL and vitaShaRK (LGPL 3.0), SDL3 and zlib (zlib), libpng (libpng license), LuaJIT and math-neon (MIT).
