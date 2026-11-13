#!/usr/bin/env python3
"""Arrange actual PSP captures, preserving originals and labeling enlarged details."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];EV=ROOT/'evidence/v106';SOURCE=EV/'psp'
try:font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
except OSError:font=ImageFont.load_default()
page=Image.new('RGB',(800,972),'#1c1916');draw=ImageDraw.Draw(page)
draw.text((12,10),'DUST 1.0.6 | PSP captures and enlarged equipped details',font=font,fill='#e4c78e')
for row,(index,label)in enumerate([(100,'Breaker-Yard Slab - salvaged'),(109,'Ceramic Bulwark - military'),(119,'Atlas Meridian - legendary')]):
 im=Image.open(SOURCE/('greatsword-%03d-1-inventory.png'%index)).convert('RGB');assert im.size==(480,272)
 y=48+row*306;draw.text((12,y),label,font=font,fill='#eee4d3');page.paste(im,(8,y+28))
 detail=im.crop((30,70,135,165)).resize((294,266),Image.Resampling.NEAREST);page.paste(detail,(500,y+28))
page.save(EV/'in-game-previews-final.png');print(EV/'in-game-previews-final.png')
