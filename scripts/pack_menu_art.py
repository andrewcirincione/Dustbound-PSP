#!/usr/bin/env python3
"""Compile the generated PSP wallpaper into a compact, indexed menu backdrop."""
from pathlib import Path
from PIL import Image
from pack_settlement_art import array
r=Path(__file__).resolve().parents[1]
im=Image.open(r/'assets/PIC1.png').convert('RGB').resize((480,272),Image.Resampling.LANCZOS)
idx=im.quantize(colors=256,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.FLOYDSTEINBERG)
p=idx.getpalette()
palette=[0xff000000 | p[i] | (p[i+1]<<8) | (p[i+2]<<16) for i in range(0,768,3)]
(r/'src/menu_art.h').write_text('/* Generated from assets/PIC1.png by scripts/pack_menu_art.py. */\n'+array('menuPixels','uint8_t',list(idx.tobytes()))+array('menuColors','uint32_t',[hex(v)+'u' for v in palette]))
