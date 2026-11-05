#!/usr/bin/env python3
"""Package the exact executable verified by all four controller playthroughs."""
import hashlib,json,pathlib,shutil,zipfile
root=pathlib.Path(__file__).resolve().parents[1];version='0.4.0'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
checks={k:json.loads((root/'evidence'/f'v040-{k}-checks.json').read_text()) for k in ['emulator','advanced','interior','travel']}
assert all(v[0]['eboot_sha256']==digest for v in checks.values()),'Playthroughs must match this exact EBOOT'
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
assert checks['travel'][-1]['fresh_start_journey'] and checks['travel'][-1]['map_browsing_preserved_route']
for source,name in [('v040-final-native.txt','v040-native-tests.txt'),('v040-normal-final.txt','v040-normal-playthrough.txt'),('v040-advanced-final.txt','v040-advanced-playthrough.txt'),('v040-room-final.txt','v040-interior-playthrough.txt'),('v040-travel-final.txt','v040-travel-playthrough.txt'),('v040-final-psp.txt','v040-psp-build.txt'),('v040-sanitizers.txt','v040-sanitizers.txt'),('v040-memory.txt','v040-memory.txt'),('v040-pbp-validation.txt','v040-pbp-validation.txt')]:
 shutil.copy2(root/'build'/source,root/'evidence'/name)
budget=json.loads((root/'assets/art/budget.json').read_text())
manifest=dict(version=version,eboot_bytes=eboot.stat().st_size,eboot_sha256=digest,prx_sha256=sha(root/'dustbound.prx'),baseline_simulation_checks=111,playability_checks=1513,interior_save_checks=25,grip_poses=13312,grip_assertions=26624,render_cases=272,gear_variants=2016,save_version=4,load_save_versions=[1,2,3,4],sprite_count=108,compiled_sprite_bytes=budget['rom_bytes'],shadow_bytes=budget['shadow_bytes'],palette_cache_bytes=budget['palette_cache_bytes'],combat_recording_frames=240,physical_psp_tested=False,playthroughs=['fresh_start','prepared_combat_outpost_regions','interiors','fresh_start_frontier_journey'])
(root/'evidence/v040-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,5,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
(root/'dist').mkdir(exist_ok=True)
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md','docs/MECHANICS_AUDIT.md','docs/ITERATION_LOG.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += ['evidence/'+n for n in ['v040-03-dusk-haven.png','v040-27-shop-interior.png','v040-33-gear-comparison.png','v040-39-wilderness-travel.png']]
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v040-') or p.name.startswith('catalog')) and p.suffix!='.log' and 'debug' not in p.name]
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
