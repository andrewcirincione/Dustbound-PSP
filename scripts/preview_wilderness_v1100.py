#!/usr/bin/env python3
"""Arrange unmodified real PSP screenshots with captions."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];ev=root/'evidence/v1100';source=ev/'psp'
try:font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
except OSError:font=ImageFont.load_default()
shots=[('wilderness-bandits','Wilderness raiders'),('wilderness-camp','Scavenger camp'),('landmark-0-exterior','Cinderwheel Foundry'),('landmark-0-interior','Foundry interior'),('landmark-1-exterior','Last Lantern Watch'),('landmark-2-interior','Shrine interior'),('greatsword-stance-passive','Passive: shoulder rest'),('greatsword-stance-fight','Fight: ready guard')]
page=Image.new('RGB',(984,1248),'#211d18');d=ImageDraw.Draw(page)
for i,(name,label) in enumerate(shots):
 x=8+(i%2)*492;y=8+(i//2)*312;d.text((x,y),label,font=font,fill='#e4c78e');im=Image.open(source/(name+'.png')).convert('RGB');assert im.size==(480,272);page.paste(im,(x,y+26))
page.save(ev/'in-game-previews.png');print(ev/'in-game-previews.png')
