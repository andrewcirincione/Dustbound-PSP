#!/usr/bin/env python3
"""Compile the production compositor capture harness using the tested core."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/v101'
OUT.mkdir(parents=True,exist_ok=True)
frames=OUT/'pose-frames'
frames.mkdir(exist_ok=True)
objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
binary=OUT/'review-character-poses'
subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/review_character_poses.c',*map(str,objects),'-lm','-o',str(binary)],cwd=ROOT,check=True)
subprocess.run([str(binary),str(frames)],cwd=ROOT,check=True)
