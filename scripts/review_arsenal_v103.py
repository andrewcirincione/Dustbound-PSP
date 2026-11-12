#!/usr/bin/env python3
from pathlib import Path
from PIL import Image,ImageDraw
import subprocess,json
ROOT=Path(__file__).resolve().parents[1];RUN=ROOT/'build/v103';RUN.mkdir(parents=True,exist_ok=True)
frames=RUN/'weapon-frames';frames.mkdir(exist_ok=True);out=ROOT/'evidence/v103/review';out.mkdir(parents=True,exist_ok=True)
objects=[str(p) for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
exe=RUN/'review-arsenal';subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/review_arsenal.c',*objects,'-lm','-o',str(exe)],cwd=ROOT,check=True)
subprocess.run([str(exe),str(frames)],cwd=ROOT,check=True)
for p in frames.glob('*.ppm'):
 with Image.open(p) as im:im.save(out/(p.stem+'.png'))
for kind in ['held','attack']:
 files=sorted(out.glob('*-'+kind+'.png'))
 for offset in range(0,len(files),20):
  page=Image.new('RGB',(480*4,272*5),'#302a23')
  for k,p in enumerate(files[offset:offset+20]):
   with Image.open(p) as im:page.paste(im,((k%4)*480,(k//4)*272))
  page.save(out/('audit-'+kind+'-%02d.jpg'%(offset//20)))
print('Equipped reviews:',len(list(out.glob('weapon-*.png'))))
