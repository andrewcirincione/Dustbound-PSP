#!/usr/bin/env python3
"""Package only the EBOOT whose completed controller records agree on its hash."""
import hashlib,json,pathlib,re,shutil,subprocess,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
version='0.6.0'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
eboot=root/'EBOOT.PBP';digest=sha(eboot)
assert 'Final native, sanitizers, PSP package and controller scenarios passed.' in (root/'build/v060-qualification.txt').read_text()
checks={name:json.loads((root/'evidence'/f'v060-{name}-checks.json').read_text()) for name in ['depth','emulator','advanced','audit','interior','travel']}
for name,events in checks.items():
 assert events[0]['eboot_sha256']==digest, ('Wrong tested EBOOT',name)
assert checks['depth'][-1]['completed']
assert checks['audit'][-1]['completed']
assert checks['emulator'][-1]['normal_run_saved_reloaded']
assert set(checks['advanced'][-1]['combat_phases_observed'])=={0,1,2,3}
assert checks['interior'][-1]['indoor_save_reload'] and checks['interior'][-1]['doors_enter_exit']
assert checks['travel'][-1]['fresh_start_journey'] and checks['travel'][-1]['map_browsing_preserved_route']
for pattern in ['v060-native.txt','v060-depth.txt','v060-psp.txt','v060-memory.txt','v060-pbp-validation.txt','v060-qualification.txt','v060-depth-controller.txt','v060-*-playthrough.txt','v060-sanitize-*.txt','v060-benchmark-*.csv']:
 for p in (root/'build').glob(pattern):shutil.copy2(p,root/'evidence'/p.name)
native=(root/'evidence/v060-native.txt').read_text()
assert '377967 depth checks' in native and '350 screen/biome/guide' in native
for suite in ['depth','render','audit','playability']:
 log=(root/'evidence'/f'v060-sanitize-{suite}.txt').read_text()
 assert 'PASS' in log and 'runtime error' not in log and 'ERROR: AddressSanitizer' not in log
# Encode the actual recorded frames as a small video for repository delivery.
subprocess.run(['ffmpeg','-v','error','-y','-i',str(root/'evidence/v060-15-combat.gif'),'-movflags','+faststart','-pix_fmt','yuv420p','-crf','18',str(root/'evidence/v060-combat.mp4')],check=True)
mem=[int(x) for x in (root/'evidence/v060-memory.txt').read_text().splitlines()[1].split()[:3]]
manifest=dict(version=version,eboot_sha256=digest,eboot_bytes=eboot.stat().st_size,prx_sha256=sha(root/'dustbound.prx'),save_version=6,load_save_versions=[1,2,3,4,5,6],game_state_bytes=435556,legacy_v5_prefix_bytes=410656,text_bytes=mem[0],data_bytes=mem[1],bss_bytes=mem[2],configured_heap_bytes=12*1024*1024,screen_types=33,guide_chapters=25,render_cases=350,depth_assertions=377967,gear_variants=2112,base_gear_designs=88,projectile_pool=24,expedition_groups=3,job_priorities=4,landmarks=8,controller_suites=list(checks),combat_recorded_frames=240,physical_psp_tested=False,limitations='Host timings are not PSP timings; fixtures are not natural campaigns; mechanics remain simplified.')
(root/'evidence/v060-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as out:
  for source,name in entries:
   info=zipfile.ZipInfo(name,(2026,11,6,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   out.writestr(info,source.read_bytes())
(root/'dist').mkdir(exist_ok=True)
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs=['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md','docs/MECHANICS_AUDIT.md','docs/DESIGN_REVIEW_V060.md','docs/IMPLEMENTATION_V060.md','docs/ROADMAP.md']
docs += [p.relative_to(root).as_posix() for p in sorted((root/'assets/ui').glob('*.svg'))]
docs += re.findall(r'!\[[^\]]*\]\((evidence/[^)]+)\)',(root/'README.md').read_text())
docs=list(dict.fromkeys(docs))
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence=[p for p in sorted((root/'evidence').glob('v060-*')) if p.suffix in ['.png','.json','.txt','.csv','.mp4'] and 'failure' not in p.name and 'route-debug' not in p.name]
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files=sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
with zipfile.ZipFile(root/'dist'/f'Dustbound-{version}-PSP.zip') as z:
 assert hashlib.sha256(z.read('PSP/GAME/DUSTBOUND/EBOOT.PBP')).hexdigest()==digest
 assert not any('DUST_QA' in n or 'DUST_RECORD' in n or 'DUST_CAPTURE' in n or 'DUST_STATE' in n or n.endswith('.sav') for n in z.namelist())
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
