#!/usr/bin/env python3
"""Validate production-renderer carry fits and arrange actual game captures."""
import json,re,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1];FRAMES=ROOT/'build/v122/visual';OUT=ROOT/'evidence/v123';OUT.mkdir(parents=True,exist_ok=True)
objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
binary=ROOT/'build/v123/review';binary.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/review_feedback_v122.c',*map(str,objects),'-lm','-o',str(binary)],cwd=ROOT,check=True)
run=subprocess.run([str(binary)],cwd=ROOT,check=True,capture_output=True,text=True)
(OUT/'visual-audit.txt').write_text(run.stdout);print(run.stdout,flush=True)
matched=re.search(r'(\d+) painted face pixels checked, (\d+) occluded',run.stdout);assert matched
record={'greatswords':20,'facings':2,'body_types':3,'species':5,'combinations':600,'painted_face_pixels_checked':int(matched[1]),'face_pixels_occluded':int(matched[2]),'method':'Production renderer before/after face-pixel comparison in the passive carry; existing joint/grip/animation suites validate active poses.'}
assert record['painted_face_pixels_checked']>30000 and record['face_pixels_occluded']==0
(OUT/'visual-audit.json').write_text(json.dumps(record,indent=2)+'\n')
for source in FRAMES.glob('*.ppm'):Image.open(source).save(OUT/(source.stem+'.png'))
canvas=Image.new('RGB',(1200,600),'#252924');draw=ImageDraw.Draw(canvas)
for i,w in enumerate([0,3,6,9,13,19]):
 for face in range(2):
  im=Image.frombytes('RGBA',(128,120),(FRAMES/f'greatsword-{w}-{face}.rgba').read_bytes());box=im.getbbox();assert box
  im=im.crop(box);im=im.resize((im.width*5,im.height*5),Image.Resampling.NEAREST)
  canvas.paste(im,(i*200+(200-im.width)//2,face*300+20),im);draw.text((i*200+8,face*300+280),f'Model {w} / facing {face}',fill='#e0cfae')
canvas.save(OUT/'greatsword-fit.png')
psp=OUT/'psp'
if (psp/'redoubt-command-floor.png').exists():
 names=[('redoubt-sleeping-quarters','Barracks / sleeping quarters'),('redoubt-armory-floor','Armory / maintenance'),('redoubt-command-floor','Command / briefing')]
 panel=Image.new('RGB',(1480,308),'#252924');draw=ImageDraw.Draw(panel)
 for i,(name,label) in enumerate(names):panel.paste(Image.open(psp/(name+'.png')),(i*496,0));draw.text((i*496+8,284),label,fill='#e0cfae')
 panel.save(OUT/'redoubt-psp-review.png')
print('Validated 600 carry fits and arranged production/PPSSPP previews.')

if (psp/'carry-109-1-world.png').exists():
 panel=Image.new('RGB',(1008,575),'#252924');draw=ImageDraw.Draw(panel)
 draw.text((16,12),'DUST 1.2.3 | One front hand / 90-degree elbow / 45-degree shoulder rest',fill='#eadfc9')
 for col,weapon in enumerate([100,109,119,32]):
  for row,face in enumerate([1,0]):
   im=Image.open(psp/f'carry-{weapon}-{face}-world.png').crop((205,98,276,154)).resize((213,168),Image.Resampling.NEAREST)
   panel.paste(im,(col*252+19,60+row*245));draw.text((col*252+8,235+row*245),f'Model {weapon} / facing {face}',fill='#e0cfae')
 panel.save(OUT/'shoulder-rest-psp.png')
