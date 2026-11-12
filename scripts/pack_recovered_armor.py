#!/usr/bin/env python3
"""Compile only the five recovered source sheets into a native review fixture.

This intentionally does not add items to the released game's catalog or saves.
The review executable substitutes temporary definitions in its own process.
"""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'assets/art/armor-expansion'
OUT = ROOT / 'build/armor-recovery'
OUT.mkdir(parents=True, exist_ok=True)

# Collar, belt, near arm opening and far arm opening in each source sheet.
# These landmarks register the new artwork to the unchanged approved rig.
LANDMARKS = {
    'cinder-scout': (332, 116, 320, 475, 165, 216, 473, 182),
    'salt-runner': (333, 130, 320, 465, 190, 211, 460, 188),
    'caravan-leather': (331, 121, 314, 477, 165, 218, 475, 193),
    'quarry-jack': (326, 130, 313, 455, 141, 222, 470, 196),
    'road-warden': (337, 128, 322, 467, 165, 218, 468, 191),
}


def array(name, typ, values):
    return f'static const {typ} {name}[]={{\n' + ',\n'.join(
        ','.join(map(str, values[i:i+24])) for i in range(0, len(values), 24)
    ) + '\n};\n'


def pack(im, name, size):
    alpha = im.getchannel('A').point(lambda a: 255 if a >= 64 else 0)
    box = alpha.getbbox()
    assert box
    im = im.crop(box)
    original = im.size
    im.thumbnail(size, Image.Resampling.LANCZOS)
    sx, sy = im.width/original[0], im.height/original[1]
    im.putalpha(im.getchannel('A').point(lambda a: 255 if a >= 220 else (a//64)*64 if a >= 64 else 0))
    indexed = im.quantize(colors=256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    palette = indexed.getpalette('RGBA')
    palette += [0] * (1024-len(palette))
    colors = [palette[i] | palette[i+1] << 8 | palette[i+2] << 16 | palette[i+3] << 24 for i in range(0, 1024, 4)]
    raw = list(indexed.getdata())
    data, rows = [], []
    for y in range(im.height):
        row = raw[y*im.width:(y+1)*im.width]
        visible = [x for x, p in enumerate(row) if colors[p] >> 24]
        lo, hi = (min(visible), max(visible)+1) if visible else (0, 0)
        rows.extend((len(data), lo, hi))
        data.extend(row[lo:hi])
    code = array(name+'_data', 'uint8_t', data)
    code += array(name+'_rows', 'uint16_t', rows)
    code += array(name+'_palette', 'uint32_t', [hex(c)+'u' for c in colors])
    record = '{%d,%d,%d,%d,%s_data,%s_rows,%s_palette}' % (
        im.width, im.height, im.width//2, im.height, name, name, name)
    im.save(OUT / (name+'.png'))
    return code, record, (box, sx, sy)


sets = json.loads((ART/'sets.json').read_text())
found = [s for s in sets if (ART/(s['slug']+'.png')).is_file()]
lines = ['/* Recovered artwork. Native preview fixture only. */\n']
records = []
for i, s in enumerate(found):
    im = Image.open(ART/(s['slug']+'.png')).convert('RGBA')
    w, h = im.size
    sprites = []
    for part, (x, y, size) in enumerate(((0, 0, (40, 58)), (1, 0, (42, 42)), (0, 1, (20, 52)), (1, 1, (22, 26)))):
        name = f'recovered_{i}_{part}'
        code, record, transform = pack(im.crop((x*w//2, y*h//2, (x+1)*w//2, (y+1)*h//2)), name, size)
        lines.append(code)
        sprites.append(record)
        if part == 0:
            box, sx, sy = transform
            raw = LANDMARKS[s['slug']]
            fit = ','.join(f'{(v-box[j%2])*(sx if j%2 == 0 else sy):.5f}f' for j, v in enumerate(raw))
    family = ','.join(map(str, s['family']))
    name = json.dumps(s['name'])
    records.append('{%s,{%s},{%s},{%s},%d,%d,%d,%d,%d}' % (
        name, family, ','.join(sprites), fit, s['armor'], s['weight'], s['speed'], s['stealth'], s['weather']))
lines += ['typedef struct {const char *name;int family[4];ArtSprite sprite[4];TorsoFit fit;int armor,weight,speed,stealth,weather;} RecoveredSet;\n',
          'static const RecoveredSet recoveredSets[]={\n'+',\n'.join(records)+'\n};\n']
(OUT/'recovered_armor.h').write_text(''.join(lines))
(OUT/'recovery-status.json').write_text(json.dumps({
    'planned': len(sets), 'source_art_recovered': len(found),
    'game_catalog_integrated_before_recovery': 0,
    'released_version': '1.0.1',
    'preview': 'Native production renderer with temporary item/art substitutions; no saves or release catalog changed.',
    'recovered': [s['name'] for s in found],
    'missing_art': [s['name'] for s in sets if s not in found]
}, indent=2)+'\n')
print(f'Packed {len(found)} recovered sets / {len(sets)} planned; no release assets replaced.')
