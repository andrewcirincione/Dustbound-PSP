#!/usr/bin/env python3
"""Compile the three generated settlement atlases into PSP indexed row spans."""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def array(name, ctype, values):
    rows = [",".join(map(str, values[i:i + 24])) for i in range(0, len(values), 24)]
    return f"static const {ctype} {name}[] = {{\n" + ",\n".join(rows) + "\n};\n"


def pack(image, name, ground_anchor, ground_x=None):
    if ground_x is None:
        ground_x = image.width // 2
    indexed = image.quantize(colors=96, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    rgba = indexed.getpalette("RGBA")
    rgba += [0] * (1024 - len(rgba))
    palette = [int.from_bytes(bytes(rgba[i:i + 4]), "little") for i in range(0, 1024, 4)]
    pixels, spans = [], []
    data = indexed.tobytes()
    for y in range(indexed.height):
        row = data[y * indexed.width:(y + 1) * indexed.width]
        visible = [x for x, color in enumerate(row) if palette[color] >> 24]
        left, right = (visible[0], visible[-1] + 1) if visible else (0, 0)
        spans.extend((len(pixels), left, right))
        pixels.extend(row[left:right])
    assert len(pixels) < 65536
    text = array(name + "_pixels", "uint8_t", pixels)
    text += array(name + "_spans", "uint16_t", spans)
    text += array(name + "_palette", "uint32_t", [f"{v:#x}u" for v in palette])
    entry = (f"{{{image.width},{image.height},{ground_x},{ground_anchor},"
             f"{name}_pixels,{name}_spans,{name}_palette}}")
    return text, entry


def main():
    code = ["/* Generated from assets/art/settlement-*.png by scripts/pack_settlement_art.py. */\n"]
    entries = []
    for kind in ("exteriors", "regions", "rooms"):
        sheet = Image.open(ROOT / "assets" / "art" / f"settlement-{kind}.png").convert("RGBA")
        width, height = sheet.width // 4, sheet.height // 2
        for index in range(8):
            x, y = index % 4 * width, index // 4 * height
            tile = sheet.crop((x, y, x + width, y + height))
            tile.putalpha(tile.getchannel("A").point(lambda a: 255 if a >= 180 else 128 if a >= 90 else 0))
            tile = tile.crop(tile.getbbox())
            assert tile.width and tile.height, (kind, index)
            tile.thumbnail((116, 128) if kind != "rooms" else (72, 78), Image.Resampling.LANCZOS)
            ground = tile.height - round(tile.width * (.24 if kind != "rooms" else .18))
            text, entry = pack(tile, "settlement_" + str(len(entries)), ground)
            code.append(text)
            entries.append(entry)
    code.append("static const ArtSprite settlementSprites[] = {\n" + ",\n".join(entries) + "\n};\n")
    (ROOT / "src" / "settlement_art.h").write_text("".join(code))


if __name__ == "__main__":
    main()
