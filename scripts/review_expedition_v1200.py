#!/usr/bin/env python3
"""Production-renderer previews of the release save fixtures."""
from pathlib import Path
from PIL import Image,ImageDraw
import subprocess
ROOT=Path(__file__).resolve().parents[1];RUN=ROOT/'build/v1200';RUN.mkdir(parents=True,exist_ok=True)
frames=RUN/'frames';frames.mkdir(exist_ok=True);out=ROOT/'evidence/v1200/review';out.mkdir(parents=True,exist_ok=True)
fixtures=ROOT/'build/v1000/fixtures';fixtures.mkdir(parents=True,exist_ok=True)
objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem!='native_main']
for source,name in [('make_v1200_fixture','make-fixtures'),('review_expedition','review')]:
 exe=RUN/name;obj=[p for p in objects if source=='review_expedition' or p.stem!='render']
 subprocess.run(['cc','-O2','-std=gnu99','-Isrc',f'tests/{source}.c',*map(str,obj),'-lm','-o',str(exe)],cwd=ROOT,check=True)
 subprocess.run([str(exe),str(fixtures),str(frames)] if source=='review_expedition' else [str(exe),str(fixtures)],cwd=ROOT,check=True)
for p in frames.glob('*.ppm'):
 with Image.open(p) as im:im.save(out/(p.stem+'.png'))
files=sorted(out.glob('*.png'));page=Image.new('RGB',(480*3,298*((len(files)+2)//3)),'#211d18');d=ImageDraw.Draw(page)
for i,p in enumerate(files):
 x=i%3*480;y=i//3*298;d.text((x+8,y+6),p.stem,fill='#e4c78e');page.paste(Image.open(p),(x,y+26))
page.save(out/'gallery.jpg');print(out/'gallery.jpg')
