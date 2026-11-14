#!/usr/bin/env python3
"""Arrange unmodified real PSP screenshots with captions."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];ev=root/'evidence/v1200';source=ev/'psp'
try:font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
except OSError:font=ImageFont.load_default()
shots=[('exploration-model-0','Kestrel Relay Citadel'),('exploration-model-1','Nine Wheels Turbine Plant'),('exploration-model-2','Amber Lantern Refuge'),('exploration-model-3','Needleglass Archive'),('exploration-model-4','Copperhook Guild Yard'),('exploration-model-5','Blue Spring Pumpworks'),('outpost-trading','Occupied outpost: trading'),('refuge-floor-two','Refuge: upstairs galleries'),('archive-floor-three','Archive: third floor'),('robot-combat','Old-world automaton combat'),('archive-entrance','Archive: explorable rooms'),('archive-salvage','Finite exploration loot')]
page=Image.new('RGB',(984,1872),'#211d18');d=ImageDraw.Draw(page)
for i,(name,label) in enumerate(shots):
 x=8+(i%2)*492;y=8+(i//2)*312;d.text((x,y),label,font=font,fill='#e4c78e');im=Image.open(source/(name+'.png')).convert('RGB');assert im.size==(480,272);page.paste(im,(x,y+26))
page.save(ev/'in-game-previews.png');print(ev/'in-game-previews.png')
