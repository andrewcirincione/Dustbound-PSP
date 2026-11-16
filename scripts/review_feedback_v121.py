#!/usr/bin/env python3
"""Arrange production-renderer frames for human inspection; do not redraw assets."""
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1];FRAMES=ROOT/'build/v121/visual';OUT=ROOT/'evidence/v121';OUT.mkdir(parents=True,exist_ok=True)
def sprite(name):
    im=Image.frombytes('RGBA',(128,120),(FRAMES/(name+'.rgba')).read_bytes())
    box=im.getbbox();assert box,name
    return im.crop((max(0,box[0]-3),max(0,box[1]-3),min(128,box[2]+3),min(120,box[3]+3)))
def sheet(name,items,columns):
    result=Image.new('RGB',(columns*220,((len(items)+columns-1)//columns)*300),'#1f1f1c');draw=ImageDraw.Draw(result)
    for i,(key,label) in enumerate(items):
        im=sprite(key);scale=min(6,200/im.width,260/im.height);im=im.resize((int(im.width*scale),int(im.height*scale)),Image.Resampling.NEAREST)
        x=i%columns*220+(220-im.width)//2;y=i//columns*300+270-im.height
        result.paste(im,(x,y),im);draw.text((i%columns*220+6,i//columns*300+280),label,fill='#e0cfae')
    result.save(OUT/(name+'.png'))
sheet('skin-and-facial-fit',[(f'tone-{i}',f'Skin tone {i+1}') for i in range(8)]+[(f'face-1-hair-{i%7}-beard-{i}',f'Hair {i%7} / Beard {i}') for i in range(8)],8)
sheet('equipment-fit',[(f'armor-19-face-{f}',f'Iron Labourer / {f}') for f in range(2)]+[(f'weapon-{w}-face-{f}',f'{"Greatsword" if w else "Katana"} / {f}') for w in range(2) for f in range(2)],6)
sheet('backpack-fit',[(f'pack-{i}-face-{face}',f'Pack {i} / facing {face}') for i in range(8) for face in range(2)],8)
sheet('walking-fit',[(f'walk-19-face-1-step-{s}',f'Iron Labourer step {s}') for s in range(12)]+[(f'walk-15-face-0-step-{s}',f'Pale Wanderer step {s}') for s in range(12)],6)
for source in FRAMES.glob('*.ppm'):Image.open(source).save(OUT/(source.stem+'.png'))
print('Arranged actual renderer frames and UI screens for Dust 1.2.1.')
