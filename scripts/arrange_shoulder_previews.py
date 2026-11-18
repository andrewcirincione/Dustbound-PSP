#!/usr/bin/env python3
"""Arrange actual PSP frames of the revised single-hand shoulder carry."""
from pathlib import Path
import sys
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'evidence/alignment-preview'
OUT=ROOT/'evidence/shoulder-carry-preview'
OUT.mkdir(parents=True,exist_ok=True)
FONT_PATH='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
try:
    FONT=ImageFont.truetype(FONT_PATH,16)
    TITLE=ImageFont.truetype(FONT_PATH,22)
except OSError:
    FONT=TITLE=ImageFont.load_default()
BG='#242722';FG='#eadfc9';MUTED='#bdbaa9';ACCENT='#c8c996'

def text(page,xy,value,font=FONT,fill=FG):
    ImageDraw.Draw(page).text(xy,value,font=font,fill=fill)

def capture(label,armor=3,weapon=109,face=1):
    path=SOURCE/label/f'alignment-set-{armor:02d}-weapon-{weapon:03d}-face-{face}-world.png'
    return Image.open(path).convert('RGB')

def detail(im,scale):
    return im.crop((205,78,276,154)).resize((71*scale,76*scale),Image.Resampling.NEAREST)

page=Image.new('RGB',(1008,790),BG)
text(page,(16,10),'DUST | Revised one-handed shoulder carry',TITLE)
for col,(label,title) in enumerate([('after','PREVIOUS PREVIEW'),('shoulder-carry','REVISED — 45° shoulder carry')]):
    x=16+col*496
    text(page,(x,44),title,fill=ACCENT)
    im=capture(label)
    page.paste(im,(x,70))
    page.paste(detail(im,5),(x+62,360))
text(page,(16,755),'Actual PPSSPP frames; enlarged details use the original pixels without smoothing.',fill=MUTED)
page.save(OUT/'previous-and-revised.png')
if '--primary-only' in sys.argv:
    print('Arranged first actual PSP shoulder-carry comparison.')
    raise SystemExit(0)

page=Image.new('RGB',(1008,695),BG)
text(page,(16,10),'DUST | Front hand carries; rear arm relaxed',TITLE)
text(page,(16,43),'Front elbow ≈90° / blade 45° back over the shoulder',fill=ACCENT)
loadouts=[(0,100,'Cinder Scout / Breaker-Yard Slab'),(3,109,'Quarry Jack / Ceramic Bulwark'),
          (9,119,'Charter Marshal / Atlas Meridian'),(26,32,'Sunken Crown / Shearblade')]
for col,(armor,weapon,title) in enumerate(loadouts):
    text(page,(col*252+8,78),title,font=ImageFont.load_default(),fill=ACCENT)
    for row,face in enumerate([1,0]):
        im=detail(capture('shoulder-carry',armor,weapon,face),3)
        page.paste(im,(col*252+19,103+row*270))
        text(page,(col*252+8,335+row*270),'Facing right' if face else 'Facing left',fill=MUTED)
text(page,(16,660),'Unretouched PPSSPP gameplay crops, enlarged 3×. Preview stage; release held.',fill=MUTED)
page.save(OUT/'both-facings.png')
print('Arranged revised single-handed shoulder-carry previews from actual PSP frames.')
