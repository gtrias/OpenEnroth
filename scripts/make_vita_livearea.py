#!/usr/bin/env python3
#
# Regenerates the PS Vita bubble and LiveArea images in packaging/vita/sce_sys from resources/images/OpenEnroth.png.
#
# The Vita installer rejects truecolor PNGs in sce_sys, so every image is written as an 8-bit palette PNG. The griffin
# is pixel art, so it is only ever scaled by whole factors with nearest-neighbour sampling.
#
# Usage: scripts/make_vita_livearea.py   (needs Pillow)

from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCE = REPO_ROOT / "resources" / "images" / "OpenEnroth.png"
SCE_SYS = REPO_ROOT / "packaging" / "vita" / "sce_sys"

TOP_COLOR = (27, 31, 42)
BOTTOM_COLOR = (58, 50, 38)


def gradient(width, height):
    image = Image.new("RGB", (width, height))
    pixels = image.load()
    for y in range(height):
        t = y / max(1, height - 1)
        color = tuple(round(a + (b - a) * t) for a, b in zip(TOP_COLOR, BOTTOM_COLOR))
        for x in range(width):
            pixels[x, y] = color
    return image


def compose(width, height, griffin, position):
    image = gradient(width, height)
    image.paste(griffin, position, griffin)
    return image


def save_indexed(image, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    image.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).save(path, optimize=True)


def main():
    griffin = Image.open(SOURCE).convert("RGBA")
    assert griffin.size == (512, 512)

    icon = griffin.resize((128, 128), Image.NEAREST)
    save_indexed(compose(128, 128, icon, (0, 0)), SCE_SYS / "icon0.png")

    # 840x500 LiveArea background, griffin flush with the right and bottom edges.
    save_indexed(compose(840, 500, griffin, (840 - 512, 500 - 512)), SCE_SYS / "livearea" / "contents" / "bg.png")

    # 280x158 start gate image.
    gate = griffin.resize((256, 256), Image.NEAREST)
    save_indexed(compose(280, 158, gate, (280 - 256, 158 - 256 + 40)),
                 SCE_SYS / "livearea" / "contents" / "startup.png")


if __name__ == "__main__":
    main()
