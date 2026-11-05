#!/usr/bin/env python3
"""Package the exact executable verified by all five controller playthroughs."""
import hashlib,json,pathlib,shutil,zipfile
root=pathlib.Path(__file__).resolve().parents[1];version='0.5.0'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
checks={k:json.loads((root/'evidence'/f'v050-{k}-checks.json').read_text()) for k in ['emulator','advanced','interior','travel','frontier']}
assert all(v[0]['eboot_sha256']==digest for v in checks.values()),'Playthroughs must match this exact EBOOT'
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
assert checks['travel'][-1]['fresh_start_journey'] and checks['travel'][-1]['map_browsing_preserved_route']
assert checks['frontier'][-1]['new_weapon_families_equipped']==4
assert checks['frontier'][-1]['fresh_escort_hired_saved_reloaded'] and checks['frontier'][-1]['genuine_v041_save_loaded']
for source,name in [('v050-native.txt','v050-native-tests.txt'),('v050-normal-playthrough.txt','v050-normal-playthrough.txt'),('v050-advanced-playthrough.txt','v050-advanced-playthrough.txt'),('v050-interior-playthrough.txt','v050-interior-playthrough.txt'),('v050-travel-playthrough.txt','v050-travel-playthrough.txt'),('v050-frontier-playthrough.txt','v050-frontier-playthrough.txt'),('v050-psp.txt','v050-psp-build.txt'),('v050-sanitizers.txt','v050-sanitizers.txt'),('v050-memory.txt','v050-memory.txt'),('v050-pbp-validation.txt','v050-pbp-validation.txt')]:
 shutil.copy2(root/'build'/source,root/'evidence'/name)
for name in ['v050-benchmark-stationary.csv','v050-benchmark-scrolling.csv','v050-debugger-timeout.txt','v050-debugger-cpu-not-started.txt']:
 if (root/'build'/name).exists():shutil.copy2(root/'build'/name,root/'evidence'/name)
budget=json.loads((root/'assets/art/budget.json').read_text())
manifest=dict(version=version,eboot_bytes=eboot.stat().st_size,eboot_sha256=digest,prx_sha256=sha(root/'dustbound.prx'),baseline_simulation_checks=111,playability_checks=1541,interior_save_checks=25,grip_poses=13312,grip_assertions=26624,render_cases=272,gear_variants=2112,save_version=5,load_save_versions=[1,2,3,4,5],sprite_count=144,painted_grip_samples=2880,painted_attack_bounds=14976,bounded_comparison_pairs=36,bss_bytes=4054188,compiled_sprite_bytes=budget['rom_bytes'],shadow_bytes=budget['shadow_bytes'],palette_cache_bytes=budget['palette_cache_bytes'],combat_recording_frames=240,physical_psp_tested=False,playthroughs=['fresh_start','prepared_combat_outpost_regions','interiors','fresh_start_frontier_journey','roles_contracts_four_weapons_and_v4_migration'])
(root/'evidence/v050-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,5,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
(root/'dist').mkdir(exist_ok=True)
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md','docs/MECHANICS_AUDIT.md','docs/ITERATION_LOG.md','docs/DESIGN_REVIEW_V050.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += ['evidence/'+n for n in ['v050-03-dusk-haven.png','v050-27-shop-interior.png','v050-58-new-weapon-0.png','v050-39-wilderness-travel.png']]
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v050-') or p.name.startswith('catalog')) and p.suffix!='.log' and 'route-debug' not in p.name]
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
