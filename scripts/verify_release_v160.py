"""Refuse to publish a binary/source tree that differs from the audited build."""
import hashlib,json,pathlib
root=pathlib.Path(__file__).resolve().parents[1]
m=json.loads((root/'evidence/v160-build-manifest.json').read_text())
def digest(p):return hashlib.sha256((root/p).read_bytes()).hexdigest()
assert digest('EBOOT.PBP')==m['eboot_sha256'],'EBOOT differs from the emulated executable'
for p,wanted in m['sources'].items():assert digest(p)==wanted,('source differs',p)
for p,wanted in m['evidence'].items():assert digest(p)==wanted,('evidence differs',p)
c=json.loads((root/m['controller_checks']).read_text())
assert c['eboot_sha256']==m['eboot_sha256']
assert len(c['cases'])==39 and c['total_frames']==9360
assert all(case['completed'] and all(str(p) in case['attack_phase_counts'] for p in range(4)) for case in c['cases'])
print('0.16 source, EBOOT, evidence hashes and 39 PSP animation cases verified.')
