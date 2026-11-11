#!/usr/bin/env python3
"""Package only the executable that passed this release's emulator check."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'dist'
EV=ROOT/'evidence/v101'
VERSION='1.0.1'

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def archive(name,entries):
    with zipfile.ZipFile(OUT/name,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for source,target in entries:
            info=zipfile.ZipInfo(target,(2026,11,11,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=0o100644<<16
            z.writestr(info,source.read_bytes())

subprocess.run(['python3','scripts/validate_pbp.py','EBOOT.PBP','--assets','assets'],cwd=ROOT,check=True)
eboot=ROOT/'EBOOT.PBP'
controller=json.loads((EV/'psp-controller.json').read_text())
assert controller['ok'] and controller['eboot_sha256']==digest(eboot),'PSP-tested executable changed'
results=json.loads((ROOT/'build/v1000/native-results.json').read_text())
assert len(results)>=41 and all(r['status']=='passed' for r in results.values()),'Native qualification failed'
sfo=(ROOT/'PARAM.SFO').read_bytes()
_,_,keys,values,count=struct.unpack_from('<4s4I',sfo)
fields={}
for n in range(count):
    key,fmt,size,cap,offset=struct.unpack_from('<HHIII',sfo,20+n*16)
    name=sfo[keys+key:].split(b'\0',1)[0].decode()
    value=sfo[values+offset:values+offset+size]
    fields[name]=struct.unpack('<I',value[:4])[0] if fmt==0x404 else value.rstrip(b'\0').decode()
assert fields['TITLE']=='Dust '+VERSION and fields['MEMSIZE']==0,fields
assert len(list((EV/'review').glob('Dust-weapon-catalog-*.png')))==7
manifest={'version':VERSION,'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
          'eboot_bytes':eboot.stat().st_size,'eboot_sha256':digest(eboot),
          'prx_sha256':digest(ROOT/'dustbound.prx'),'native_suites_passed':len(results),
          'render_cases':575,'torso_silhouettes':16,'weapon_unarmed_sets':39,
          'attack_variants_per_set':3,'save_format':29,'sfo':fields,
          'pspdev':'v20261101 / GCC 15.2.0','emulator':controller['emulator'],
          'physical_psp_tested':False,
          'runtime_sources':{str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'src').glob('*')) if p.suffix in ('.c','.h','.inc')}}
(EV/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(EV/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
OUT.mkdir(exist_ok=True)
shutil.copy2(eboot,OUT/'EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION_V101.md']
archive('Dust-1.0.1-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(ROOT/p,'Dust-1.0.1/'+p) for p in docs])
archive('Dust-1.0.1-Evidence.zip',[(p,str(p.relative_to(ROOT))) for p in sorted(EV.rglob('*')) if p.is_file()])
with zipfile.ZipFile(OUT/'Dust-1.0.1-PSP.zip') as z:
    assert z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')==eboot.read_bytes()
    assert not any(n.endswith(('.sav','.bak','.rgba')) or 'DUST_QA' in n or 'DUST_RECORD' in n for n in z.namelist())
names=['EBOOT.PBP','Dust-1.0.1-PSP.zip','Dust-1.0.1-Evidence.zip']
(OUT/'SHA256SUMS-1.0.1.txt').write_text(''.join(digest(OUT/n)+'  '+n+'\n' for n in names))
print((OUT/'SHA256SUMS-1.0.1.txt').read_text())
