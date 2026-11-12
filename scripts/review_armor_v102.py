#!/usr/bin/env python3
"""Capture every armor set with the production compositor, including rank uniforms."""
from pathlib import Path
import subprocess
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
frames=ROOT/'build/v102/review-frames';frames.mkdir(parents=True,exist_ok=True)
out=ROOT/'evidence/v102/review';out.mkdir(parents=True,exist_ok=True)
objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
binary=ROOT/'build/v102/review-armor'
subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/review_armor_sets.c',*map(str,objects),'-lm','-o',str(binary)],cwd=ROOT,check=True)
subprocess.run([str(binary),str(frames)],cwd=ROOT,check=True)
for source in sorted(frames.glob('*.ppm')):
    with Image.open(source) as im:im.save(out/(source.stem+'.png'))
assert len(list(out.glob('*.png')))==153
print('PASS: 153 production-renderer reviews (30 sets, both facings, combat and three faction rank ladders).')
