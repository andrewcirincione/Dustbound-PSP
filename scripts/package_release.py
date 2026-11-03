#!/usr/bin/env python3
"""Package the already-built, tested 0.2.0 EBOOT with deterministic ZIP metadata."""
import hashlib,json,pathlib,re,shutil,struct,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256((root/p).read_bytes()).hexdigest()
def archive(name,entries):
 with zipfile.ZipFile(root/'dist'/name,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for src,dest in entries:
   info=zipfile.ZipInfo(dest,(2026,11,3,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   z.writestr(info,(root/src).read_bytes())
eb=root/'EBOOT.PBP';size=eb.stat().st_size;digest=sha('EBOOT.PBP')
p=root/'docs'/'VALIDATION.md';s=p.read_text();s=re.sub(r'EBOOT: \*\*[\d,]+ bytes\*\*\. SHA-256: `[a-f0-9]+`',f'EBOOT: **{size:,} bytes**. SHA-256: `{digest}`',s);p.write_text(s)
sfo=(root/'PARAM.SFO').read_bytes();_,_,ko,do,count=struct.unpack_from('<4s4I',sfo);values={}
for i in range(count):
 key,fmt,n,cap,off=struct.unpack_from('<HHIII',sfo,20+i*16);name=sfo[ko+key:].split(b'\0',1)[0].decode();v=sfo[do+off:do+off+n];values[name]=struct.unpack('<I',v[:4])[0] if fmt==0x404 else v.rstrip(b'\0').decode(errors='replace')
assert values['MEMSIZE']==0 and values['TITLE']=='Dustbound 0.2.0'
manifest={'version':'0.2.0','eboot_bytes':size,'eboot_sha256':digest,'prx_sha256':sha('dustbound.prx'),'full_playthrough_eboot_sha256':'7667615f13efcf09f5bed8cb7ab8b6eeaea53550d323ab4b85188c3c76585e90','full_playthrough_prx_sha256':'cf626f33c20434067d73a94bb19a3db4a4d216a92503a50cc023ae9a71b07654','final_package_change':'Updated PSP menu background only; byte-identical runtime payload; final boot passed.','sfo':values,'simulation_checks':107,'render_cases':243,'gear_variants':2016,'physical_psp_tested':False}
assert manifest['prx_sha256']==manifest['full_playthrough_prx_sha256'],'Runtime changed: repeat playtests before releasing.'
(root/'evidence'/'v02-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
shutil.copy2(root/'build'/'final-boot.log',root/'evidence'/'v02-final-boot.txt')
(root/'dist').mkdir(exist_ok=True);shutil.copy2(eb,root/'dist'/'Dustbound-0.2.0-EBOOT.PBP')
files=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md']
archive('Dustbound-0.2.0-PSP.zip',[('EBOOT.PBP','PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(p,'Dustbound-0.2.0/'+p) for p in files])
evidence=[p.relative_to(root).as_posix() for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v02-') or p.name.startswith('catalog')) and p.suffix!='.log']
archive('Dustbound-0.2.0-Evidence.zip',[(p,p) for p in evidence]+[('docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob('Dustbound-0.2.0-*'))
(root/'dist'/'SHA256SUMS-0.2.0.txt').write_text(''.join(sha(p.relative_to(root))+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/'Dustbound-0.2.0-PSP.zip') as z:
 assert z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')==eb.read_bytes()
 assert not any('.sav' in n or 'DUST_QA' in n or 'DUST_RECORD' in n for n in z.namelist())
print((root/'dist'/'SHA256SUMS-0.2.0.txt').read_text())
