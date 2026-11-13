#!/usr/bin/env python3
"""Package exactly the PSP executable qualified by native tests and PPSSPP."""
import hashlib,json,os,shutil,struct,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'dist';EV=ROOT/'evidence/v106';VERSION='1.0.6'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
subprocess.run(['python3','scripts/validate_pbp.py','EBOOT.PBP','--assets','assets'],cwd=ROOT,check=True)
eboot=ROOT/'EBOOT.PBP';controller=json.loads((EV/'psp/psp-controller.json').read_text())
assert controller['ok'] and controller['eboot_sha256']==digest(eboot) and controller['weapons_tested']==8 and controller['unique_greatswords_tested']==4 and controller['facings_tested']==2 and controller['combat_verified'] and controller['combat_weapons_tested']==8 and controller['smith_previews_verified']
results=json.loads((ROOT/'build/v1000/native-results.json').read_text())
assert len(results)>=47 and all(x['status']=='passed' for x in results.values()),'Native qualification failed'
assert len(list((EV/'review').glob('weapon-*.png')))==260
pbp=eboot.read_bytes();offsets=struct.unpack_from('<8I',pbp,8);sfo=pbp[offsets[0]:offsets[1]]
_,_,keys,values,count=struct.unpack_from('<4s4I',sfo);fields={}
for n in range(count):
    key,fmt,size,cap,offset=struct.unpack_from('<HHIII',sfo,20+n*16)
    name=sfo[keys+key:].split(b'\0',1)[0].decode();value=sfo[values+offset:values+offset+size]
    fields[name]=struct.unpack('<I',value[:4])[0] if fmt==0x404 else value.rstrip(b'\0').decode()
assert fields['TITLE']=='Dust '+VERSION and fields['MEMSIZE']==0,fields
manifest={'version':VERSION,'commit':os.environ.get('GITHUB_SHA') or subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
'eboot_bytes':len(pbp),'eboot_sha256':digest(eboot),'prx_sha256':hashlib.sha256(pbp[offsets[6]:offsets[7]]).hexdigest(),
'native_suites_passed':len(results),'new_weapon_models':0,'oversized_greatsword_blades_reduced':True,'catalog_models':120,'new_greatswords':0,'weapon_types_covered':38,'production_render_reviews':260,
'save_format':32,'sfo':fields,'pspdev':'v20261101 / GCC 15.2.0','emulator':controller['emulator'],
'emulator_verified':True,'physical_psp_tested':False,
'runtime_sources':{str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'src').glob('*')) if p.suffix in ('.c','.h','.inc')}}
(EV/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(EV/'native-results.json').write_text(json.dumps(results,indent=2)+'\n');OUT.mkdir(exist_ok=True);shutil.copy2(eboot,OUT/'EBOOT.PBP')
def archive(name,entries):
    with zipfile.ZipFile(OUT/name,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for source,target in entries:
            info=zipfile.ZipInfo(target,(2026,11,12,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
            z.writestr(info,source.read_bytes())
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/ARSENAL_DISTRIBUTION.md']
archive('Dust-1.0.6-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(ROOT/p,'Dust-1.0.6/'+p) for p in docs])
archive('Dust-1.0.6-Evidence.zip',[(p,str(p.relative_to(ROOT))) for p in sorted(EV.rglob('*')) if p.is_file()])
with zipfile.ZipFile(OUT/'Dust-1.0.6-PSP.zip') as z:
    assert z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')==pbp
    assert not any(n.endswith(('.sav','.bak','.rgba')) or 'DUST_QA' in n for n in z.namelist())
names=['EBOOT.PBP','Dust-1.0.6-PSP.zip','Dust-1.0.6-Evidence.zip']
(OUT/'SHA256SUMS-1.0.6.txt').write_text(''.join(digest(OUT/n)+'  '+n+'\n' for n in names))
print((OUT/'SHA256SUMS-1.0.6.txt').read_text())
