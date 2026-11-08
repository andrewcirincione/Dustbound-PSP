"""Refuse to publish a binary/source tree that differs from the audited build."""
import hashlib,json,pathlib
root=pathlib.Path(__file__).resolve().parents[1]
m=json.loads((root/'evidence/v160-build-manifest.json').read_text())
def digest(p):return hashlib.sha256((root/p).read_bytes()).hexdigest()
assert m['version']=='0.16.0'
assert (root/'EBOOT.PBP').stat().st_size==m['eboot_bytes']
assert digest('EBOOT.PBP')==m['eboot_sha256'],'EBOOT differs from the emulated executable'
for p,wanted in m['sources'].items():assert digest(p)==wanted,('source differs',p)
for p,wanted in m['evidence'].items():assert digest(p)==wanted,('evidence differs',p)
c=json.loads((root/m['controller_checks']).read_text())
assert c['version']==m['version'] and c['eboot_sha256']==m['eboot_sha256']
assert len(c['cases'])==39 and c['total_frames']==9360
assert sorted(case['case'] for case in c['cases'])==list(range(39))
assert sum(case['frames'] for case in c['cases'])==c['total_frames']
for case in c['cases']:
 assert case['completed'] and case['frames']==240
 assert all(case['attack_phase_counts'].get(str(p),0)>0 for p in range(4))
 assert sum(case['attack_phase_counts'].values())==case['frames']
 number=case['case']
 case_path=f'evidence/v160/case-{number:02d}.json'
 assert json.loads((root/case_path).read_text())==case
 for path in (case_path,f'evidence/v160/psp-family-{number:02d}.png',f'evidence/v160/psp-family-{number:02d}.gif'):
  assert path in m['evidence'] and (root/path).stat().st_size>0,('missing audited evidence',path)
assert c['cases'][1]['extra']['passive_cancel']
print('0.16 source, EBOOT, evidence hashes and 39 PSP animation cases verified.')
