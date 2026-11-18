#!/usr/bin/env python3
"""Label and enlarge actual PSP frames; keep the original captures beside them."""
from pathlib import Path
import sys
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'evidence/alignment-preview'
try:
    FONT=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
    TITLE=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',22)
except OSError:
    FONT=TITLE=ImageFont.load_default()
BG='#242722';FG='#eadfc9';MUTED='#bdbaa9';ACCENT='#c8c996'

def text(page,xy,value,font=FONT,fill=FG):
    ImageDraw.Draw(page).text(xy,value,font=font,fill=fill)

def capture(label,armor=3,weapon=109,face=1):
    return Image.open(OUT/label/f'alignment-set-{armor:02d}-weapon-{weapon:03d}-face-{face}-world.png').convert('RGB')

def detail(im,scale=5):
    return im.crop((211,99,273,153)).resize((62*scale,54*scale),Image.Resampling.NEAREST)

page=Image.new('RGB',(1008,680),BG)
text(page,(16,10),'DUST | Right-facing boot and greatsword alignment',TITLE)
for col,(label,title) in enumerate([('before','BEFORE — released 1.2.2'),('after','AFTER — preview build')]):
    x=16+col*496
    text(page,(x,44),title,fill=ACCENT)
    im=capture(label)
    page.paste(im,(x,70))
    page.paste(detail(im),(x+85,365))
text(page,(16,649),'Actual PPSSPP frames above; matching details enlarged 5× below. No smoothing.',fill=MUTED)
page.save(OUT/'before-after-right.png')
if '--primary-only' in sys.argv:
    print('Arranged the primary matched PSP before/after preview.')
    raise SystemExit(0)

page=Image.new('RGB',(1008,660),BG)
text(page,(16,10),'DUST | Updated carry and boot placement — both facings',TITLE)
loadouts=[(0,100,'Cinder Scout / Breaker-Yard Slab'),(3,109,'Quarry Jack / Ceramic Bulwark'),
          (9,119,'Charter Marshal / Atlas Meridian'),(26,32,'Sunken Crown / Shearblade')]
for col,(armor,weapon,title) in enumerate(loadouts):
    text(page,(col*252+8,48),title,font=ImageFont.load_default(),fill=ACCENT)
    for row,face in enumerate([1,0]):
        im=detail(capture('after',armor,weapon,face),4)
        page.paste(im,(col*252+2,80+row*275))
        text(page,(col*252+8,305+row*275),'Facing right' if face else 'Facing left',fill=MUTED)
text(page,(16,625),'Unretouched crops from the preview PSP executable; enlarged 4× without smoothing.',fill=MUTED)
page.save(OUT/'both-facings.png')

page=Image.new('RGB',(1008,670),BG)
text(page,(16,10),'DUST | Right-facing boots — matched before / after',TITLE)
for col,(armor,weapon,title) in enumerate(loadouts):
    text(page,(col*252+8,49),title.split(' / ')[0],fill=ACCENT)
    for row,label in enumerate(['before','after']):
        im=capture(label,armor,weapon,1).crop((229,125,254,148))
        im=im.resize((225,207),Image.Resampling.NEAREST)
        page.paste(im,(col*252+13,79+row*267))
        text(page,(col*252+13,294+row*267),label.upper(),fill=MUTED)
text(page,(16,638),'Same scene and loadout in both builds. Actual PPSSPP pixels enlarged 9×.',fill=MUTED)
page.save(OUT/'boots-before-after.png')
print('Arranged actual PSP before/after frames and both-facing equipment previews.')
