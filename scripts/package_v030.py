#!/usr/bin/env python3
"""Package only the exact EBOOT exercised by all three PSP playthroughs."""
import hashlib,json,pathlib,shutil,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
version='0.3.0'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
assert digest=='72e48dd77dbda25dffa8811f1cdc3ef4dfe7ca2bdad99913900d1600d8f8fc7a'
checks={k:json.loads((root/'evidence'/f'v030-{k}-checks.json').read_text()) for k in ['emulator','advanced','interior']}
assert all(v[0]['eboot_sha256']==digest for v in checks.values())
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
for source,name in [('v030-native-final.txt','v030-native-tests.txt'),('v030-normal-final.txt','v030-normal-playthrough.txt'),('v030-advanced-final.txt','v030-advanced-playthrough.txt'),('v030-interiors-final.txt','v030-interior-playthrough.txt'),('v030-build-final.txt','v030-psp-build.txt')]:
 shutil.copy2(root/'build'/source,root/'evidence'/name)
sanitizers=(root/'build/v030-sanitizers.txt').read_text().splitlines()[0]+'\n'+(root/'build/v030-overhaul-final.txt').read_text()
(root/'evidence/v030-sanitizers.txt').write_text(sanitizers)
manifest=dict(version=version,eboot_bytes=eboot.stat().st_size,eboot_sha256=digest,prx_sha256=sha(root/'dustbound.prx'),existing_simulation_checks=111,interior_save_checks=25,grip_poses=13312,grip_assertions=26624,render_cases=272,gear_variants=2016,save_version=3,load_save_versions=[1,2,3],compiled_sprite_bytes=130303,palette_cache_bytes=24576,combat_recording_frames=240,physical_psp_tested=False,playthroughs=['normal','combat_outpost','interiors'])
(root/'evidence/v030-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,3,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += ['evidence/'+n for n in ['v030-03-dusk-haven.png','v030-27-shop-interior.png','v030-15-combat.gif']]
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v030-') or p.name.startswith('catalog')) and p.suffix!='.log']
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
