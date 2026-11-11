#!/usr/bin/env python3
"""Compile the native core once and exercise the current and historical suites."""
import gzip,json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/v1000'
OUT.mkdir(parents=True,exist_ok=True)
for fixture in (ROOT/'tests/fixtures').glob('*.gz'):
 data=gzip.decompress(fixture.read_bytes())
 for dest in [ROOT/'build'/fixture.stem,ROOT/'build'/('old-v%d.sav'%struct.unpack_from('<I',data,8)[0]),ROOT/'build/v170'/('old-v%d.sav'%struct.unpack_from('<I',data,8)[0])]:
  dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
for source in (ROOT/'tests').glob('test_*.c'):
 for name in re.findall(r'"((?:build|evidence)/[^"\n]+)',source.read_text()):
  p=ROOT/name.split('%')[0]
  (p if name.split('%')[0].endswith('/') else p.parent).mkdir(parents=True,exist_ok=True)
(ROOT/'build/v1000/animation').mkdir(exist_ok=True)
subprocess.run(['make','-f','scripts/native_v1000.mk','-j4'],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
objects=[p for p in (OUT/'obj').glob('*.o') if p.stem!='native_main']
results={}
for source in sorted((ROOT/'tests').glob('test_*.c')):
 name=source.stem
 included=set(re.findall(r'#include "\.\./src/(\w+)\.c"',source.read_text()))
 binary=OUT/name
 cmd=['cc','-O1','-g','-std=gnu99','-Isrc',str(source),*[str(p) for p in objects if p.stem not in included],'-lm','-o',str(binary)]
 compile_result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True)
 if compile_result.returncode:
  results[name]={'status':'compile-failed','output':compile_result.stderr};print(name,'COMPILE FAILED',flush=True);continue
 args=[str(binary)]
 if name=='test_animation':args.append(str(OUT/'animation'))
 if name=='test_v1000':args.append(str(ROOT/'build/v0190-property.sav'))
 if name=='test_save_compat':args += [str(ROOT/'build'/p.stem) for p in sorted((ROOT/'tests/fixtures').glob('*.gz'))] + [str(ROOT/'tests/fixtures/legacy-v01.dust')]
 try:
  run=subprocess.run(['stdbuf','-oL',*args],cwd=ROOT,capture_output=True,text=True,timeout=180)
  results[name]={'status':'passed' if run.returncode==0 else 'failed','exit':run.returncode,'output':run.stdout+run.stderr}
 except subprocess.TimeoutExpired as exc:results[name]={'status':'timeout','output':str(exc)}
 (OUT/(name+'.log')).write_text(results[name]['output'])
 print(name,results[name]['status'].upper(),flush=True)
 (OUT/'native-results.json').write_text(json.dumps(results,indent=2))
print(json.dumps({s:sum(x['status']==s for x in results.values()) for s in ['passed','failed','compile-failed','timeout']}),flush=True)
