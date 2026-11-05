#!/usr/bin/env python3
"""Package the exact executable verified by all five controller playthroughs."""
import hashlib,json,pathlib,shutil,zipfile
root=pathlib.Path(__file__).resolve().parents[1];version='0.4.1'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
checks={k:json.loads((root/'evidence'/f'v041-{k}-checks.json').read_text()) for k in ['emulator','advanced','interior','travel','art']}
assert all(v[0]['eboot_sha256']==digest for v in checks.values()),'Playthroughs must match this exact EBOOT'
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
assert checks['travel'][-1]['fresh_start_journey'] and checks['travel'][-1]['map_browsing_preserved_route']
assert checks['art'][-1]['all_32_weapon_families_equipped']
for source,name in [('v041-native.txt','v041-native-tests.txt'),('v041-normal-playthrough.txt','v041-normal-playthrough.txt'),('v041-advanced-playthrough.txt','v041-advanced-playthrough.txt'),('v041-interior-playthrough.txt','v041-interior-playthrough.txt'),('v041-travel-playthrough.txt','v041-travel-playthrough.txt'),('v041-art-playthrough.txt','v041-art-playthrough.txt'),('v041-psp.txt','v041-psp-build.txt'),('v041-sanitizers.txt','v041-sanitizers.txt'),('v041-memory.txt','v041-memory.txt'),('v041-pbp-validation.txt','v041-pbp-validation.txt')]:
 shutil.copy2(root/'build'/source,root/'evidence'/name)
for name in ['v040-benchmark-stationary.csv','v040-benchmark-scrolling.csv','v041-benchmark-stationary.csv','v041-benchmark-scrolling.csv']:
 shutil.copy2(root/'build'/name,root/'evidence'/('v041-baseline-'+name if name.startswith('v040') else name))
budget=json.loads((root/'assets/art/budget.json').read_text())
manifest=dict(version=version,eboot_bytes=eboot.stat().st_size,eboot_sha256=digest,prx_sha256=sha(root/'dustbound.prx'),baseline_simulation_checks=111,playability_checks=1513,interior_save_checks=25,grip_poses=13312,grip_assertions=26624,render_cases=272,gear_variants=2016,save_version=4,load_save_versions=[1,2,3,4],sprite_count=140,painted_grip_samples=2560,painted_attack_bounds=13312,bounded_comparison_pairs=32,bss_bytes=4020588,compiled_sprite_bytes=budget['rom_bytes'],shadow_bytes=budget['shadow_bytes'],palette_cache_bytes=budget['palette_cache_bytes'],combat_recording_frames=240,physical_psp_tested=False,playthroughs=['fresh_start','prepared_combat_outpost_regions','interiors','fresh_start_frontier_journey','prepared_32_weapon_families'])
(root/'evidence/v041-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,5,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
(root/'dist').mkdir(exist_ok=True)
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md','docs/MECHANICS_AUDIT.md','docs/ITERATION_LOG.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += ['evidence/'+n for n in ['v041-03-dusk-haven.png','v041-27-shop-interior.png','v041-art-compare-04.png','v041-39-wilderness-travel.png']]
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v041-') or p.name.startswith('catalog')) and p.suffix!='.log' and 'debug' not in p.name]
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
