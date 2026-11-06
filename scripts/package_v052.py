#!/usr/bin/env python3
"""Package the exact executable verified by all fourteen controller scenarios."""
import hashlib,json,pathlib,re,shutil,zipfile
root=pathlib.Path(__file__).resolve().parents[1];version='0.5.2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
checks={k:json.loads((root/'evidence'/f'v052-{k}-checks.json').read_text()) for k in ['emulator','advanced','interior','travel','frontier','attachments','audit']}
assert all(event['eboot_sha256']==digest for events in checks.values() for event in events if 'eboot_sha256' in event),'Playthroughs must match this exact EBOOT'
for mode in ['shopping','weapons0','weapons1','weapons2','backs','clothing0','clothing1','clothing2']:
 events=json.loads((root/'evidence'/f'v052-attachments-{mode}-checks.json').read_text())
 assert events[0]['eboot_sha256']==digest and events[-1]['completed'] and events[-1]['scenario']==mode
assert checks['attachments'][-1]['clothing_families_equipped_both_facings']==44
assert checks['attachments'][-1]['fresh_shopping_saved_reloaded']
assert checks['attachments'][-1]['weapons_both_facings']==36
assert 'All fourteen PSP controller scenarios' in (root/'build/v052-validation-run.txt').read_text()
assert checks['audit'][-1]['completed'] and checks['audit'][-1]['cook_fetched_water_and_grain_then_stored_rations']
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
assert checks['travel'][-1]['fresh_start_journey'] and checks['travel'][-1]['map_browsing_preserved_route']
assert checks['frontier'][-1]['new_weapon_families_equipped']==4
assert checks['frontier'][-1]['fresh_escort_hired_saved_reloaded'] and checks['frontier'][-1]['genuine_v041_save_loaded']
for source,name in [('v052-audit-playthrough.txt','v052-audit-playthrough.txt'),('v052-attachments-playthrough.txt','v052-attachments-playthrough.txt'),('v052-native.txt','v052-native-tests.txt'),('v052-normal-playthrough.txt','v052-normal-playthrough.txt'),('v052-advanced-playthrough.txt','v052-advanced-playthrough.txt'),('v052-interior-playthrough.txt','v052-interior-playthrough.txt'),('v052-travel-playthrough.txt','v052-travel-playthrough.txt'),('v052-frontier-playthrough.txt','v052-frontier-playthrough.txt'),('v052-psp.txt','v052-psp-build.txt'),('v052-sanitizers.txt','v052-sanitizers.txt'),('v052-memory.txt','v052-memory.txt'),('v052-pbp-validation.txt','v052-pbp-validation.txt')]:
 shutil.copy2(root/'build'/source,root/'evidence'/name)
for name in ['v052-benchmark-stationary.csv','v052-benchmark-scrolling.csv','v052-audit-startup-regression.txt','v052-audit-signed-telemetry.txt','v052-audit-door-selection.txt','v052-audit-button-repeat.txt']:
 if (root/'build'/name).exists():shutil.copy2(root/'build'/name,root/'evidence'/name)
shutil.copy2(root/'evidence/v051-clothing-balance.csv',root/'evidence/v052-clothing-balance.csv')
memory=[int(n) for n in (root/'build/v052-memory.txt').read_text().splitlines()[1].split()[:3]]
render_cases=int(re.search(r'PASS: (\d+) screen/biome/guide', (root/'build/v052-native.txt').read_text()).group(1))
budget=json.loads((root/'assets/art/budget.json').read_text())
manifest=dict(version=version,eboot_bytes=eboot.stat().st_size,eboot_sha256=digest,prx_sha256=sha(root/'dustbound.prx'),baseline_simulation_checks=111,playability_checks=1541,interior_save_checks=25,grip_poses=13312,grip_assertions=26624,render_cases=render_cases,gear_variants=2112,save_version=5,load_save_versions=[1,2,3,4,5],sprite_count=144,painted_grip_samples=5760,painted_attack_bounds=14976,bounded_comparison_pairs=36,bss_bytes=memory[2],text_bytes=memory[0],data_bytes=memory[1],clothing_variants_tested=1056,hat_fit_cases=72,pack_fit_cases=1152,asymmetric_weapon_reflections=288,actual_pack_swaps=192,controller_scenarios=14,critical_audit_checks=70,compiled_sprite_bytes=budget['rom_bytes'],shadow_bytes=budget['shadow_bytes'],palette_cache_bytes=budget['palette_cache_bytes'],combat_recording_frames=240,physical_psp_tested=False,playthroughs=['critical_ui_recovery_and_camp_logistics','fresh_start','prepared_combat_outpost_regions','interiors','fresh_start_frontier_journey','roles_contracts_four_weapons_and_v4_migration','fresh_pack_shopping_contract_save_reload','36_weapons_both_facings_3_scenarios','8_back_designs_both_facings','44_clothing_designs_comparisons_both_facings_3_scenarios'])
(root/'evidence/v052-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,6,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
(root/'dist').mkdir(exist_ok=True)
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md','docs/MECHANICS_AUDIT.md','docs/ITERATION_LOG.md','docs/DESIGN_REVIEW_V050.md','docs/DESIGN_REVIEW_V051.md','docs/DESIGN_REVIEW_V052.md','evidence/v052-clothing-balance.csv','docs/ROADMAP.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += re.findall(r'!\[[^\]]*\]\((evidence/[^)]+)\)',(root/'README.md').read_text())
docs = list(dict.fromkeys(docs))
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v052-') or p.name.startswith('catalog')) and p.suffix!='.log' and 'route-debug' not in p.name]
equipment=[p for p in evidence if re.match(r'v052-(?:77-|78-|79-|80-|after-|before-)',p.name)]
core=[p for p in evidence if p not in equipment]
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in core]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
archive(root/'dist'/f'Dustbound-{version}-Equipment-Evidence.zip',[(p,'evidence/'+p.name) for p in equipment]+[(root/'evidence/v052-build-manifest.json','BUILD-MANIFEST.json'),(root/'docs/VALIDATION.md','VALIDATION.md')])
assert all(p.stat().st_size < 25*1024*1024 for p in (root/'dist').glob(f'Dustbound-{version}-*')), 'Keep repository-upload artifacts below 25 MiB'
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or 'DUST_CAPTURE' in n or 'DUST_STATE' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
