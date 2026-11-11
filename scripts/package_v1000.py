#!/usr/bin/env python3
"""Package the exact emulator-tested Dust 1.0.0 payload, without QA markers."""
import hashlib,json,shutil,struct,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'dist'
EV=ROOT/'evidence/v1000'
VERSION='1.0.0'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def archive(name,entries):
 with zipfile.ZipFile(OUT/name,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for source,target in entries:
   info=zipfile.ZipInfo(target,(2026,11,11,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   z.writestr(info,source.read_bytes())

eboot=ROOT/'EBOOT.PBP'
subprocess.run(['python',str(ROOT/'scripts/validate_pbp.py'),str(eboot),'--assets',str(ROOT/'assets')],check=True)
sha=digest(eboot)
assert sha==(ROOT/'build/v1000/final-psp.sha256').read_text().strip()
psp=json.loads((ROOT/'build/v1000/emu/result.json').read_text())
assert psp.get('ok') and psp.get('id')=='final-psp','Final controller test must complete successfully.'
results=json.loads((ROOT/'build/v1000/native-results.json').read_text())
assert len(results)==39 and all(v['status']=='passed' for v in results.values())
for name in ['v1000','render']:
 text=(ROOT/('build/v1000/asan/test-'+name+'.log')).read_text()
 assert 'PASS' in text and 'AddressSanitizer' not in text and 'runtime error:' not in text
sfo=(ROOT/'PARAM.SFO').read_bytes();_,_,keys,values,count=struct.unpack_from('<4s4I',sfo);fields={}
for n in range(count):
 key,fmt,size,cap,offset=struct.unpack_from('<HHIII',sfo,20+n*16)
 name=sfo[keys+key:].split(b'\0',1)[0].decode();v=sfo[values+offset:values+offset+size]
 fields[name]=struct.unpack('<I',v[:4])[0] if fmt==0x404 else v.rstrip(b'\0').decode()
assert fields['TITLE']=='Dust 1.0.0' and fields['MEMSIZE']==0
EV.mkdir(parents=True,exist_ok=True);OUT.mkdir(exist_ok=True)
manifest={'version':VERSION,'eboot_bytes':eboot.stat().st_size,'eboot_sha256':sha,'prx_sha256':digest(ROOT/'dustbound.prx'),'pspdev':'v20261101 / GCC 15.2.0','emulator':'Official PPSSPP 1.20.4','native_suites_passed':len(results),'render_cases':575,'armor_silhouettes_reviewed':16,'weapon_unarmed_families_reviewed':39,'attack_variants_per_family':3,'archived_saves_roundtripped':len(list((ROOT/'tests/fixtures').glob('*.gz')))+1,'asan_ubsan':['integration','575 renders'],'physical_psp_tested':False,'sfo':fields,'runtime_sources':{str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'src').glob('*')) if p.suffix in ['.c','.h','.inc']}}
(EV/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(EV/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
(EV/'psp-controller.json').write_text(json.dumps({k:v for k,v in psp.items() if k!='state'},indent=2)+'\n')
for name in ['v1000','render']:shutil.copy2(ROOT/('build/v1000/asan/test-'+name+'.log'),EV/('sanitizer-'+name+'.txt'))
shutil.copy2(eboot,OUT/'EBOOT.PBP')
files=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION_V1000.md']
archive('Dust-1.0.0-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(ROOT/p,'Dust-1.0.0/'+p) for p in files])
evidence=[p for p in sorted(EV.rglob('*')) if p.is_file()]
archive('Dust-1.0.0-Evidence.zip',[(p,str(p.relative_to(ROOT))) for p in evidence])
with zipfile.ZipFile(OUT/'Dust-1.0.0-PSP.zip') as z:
 assert z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')==eboot.read_bytes()
 assert not any(n.endswith(('.sav','.bak','.rgba')) or 'DUST_QA' in n or 'DUST_RECORD' in n for n in z.namelist())
artifacts=['EBOOT.PBP','Dust-1.0.0-PSP.zip','Dust-1.0.0-Evidence.zip']
(OUT/'SHA256SUMS-1.0.0.txt').write_text(''.join(digest(OUT/name)+'  '+name+'\n' for name in artifacts))
print((OUT/'SHA256SUMS-1.0.0.txt').read_text())
