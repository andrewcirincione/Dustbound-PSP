#!/usr/bin/env python3
"""Package the validated Dustbound 0.2.1 build without developer markers or saves."""
import hashlib, json, pathlib, shutil, zipfile
root = pathlib.Path(__file__).resolve().parents[1]
version = '0.2.1'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def archive(path, entries):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as out:
        for source, name in entries:
            info = zipfile.ZipInfo(name, (2026, 11, 3, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            out.writestr(info, source.read_bytes())
eboot = root / 'EBOOT.PBP'
assert sha(eboot) == '8409f30c662705ecc1fa5d8076a4c86730e755104bc25183621a5ddcc4b59564', 'Build changed: repeat validation.'
normal = json.loads((root/'evidence/v021-emulator-checks.json').read_text())
advanced = json.loads((root/'evidence/v021-advanced-checks.json').read_text())
assert normal[-1]['normal_run_saved_reloaded']
assert set(advanced[-1]['combat_phases_observed']) == {0, 1, 2, 3}
for source, name in [('v021-native.txt','v021-native-tests.txt'), ('v021-sanitizers.txt','v021-sanitizers.txt'), ('v021-normal-final.txt','v021-normal-playthrough.txt'), ('v021-advanced-final.txt','v021-advanced-playthrough.txt')]:
    shutil.copy2(root/'build'/source, root/'evidence'/name)
manifest = {'version':version, 'eboot_bytes':eboot.stat().st_size, 'eboot_sha256':sha(eboot), 'prx_sha256':sha(root/'dustbound.prx'), 'simulation_checks':111, 'render_cases':243, 'gear_variants':2016, 'combat_recording_frames':240, 'capture_simulation_hz':30, 'physical_psp_tested':False}
(root/'evidence/v021-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
shutil.copy2(eboot,root/'dist'/f'Dustbound-{version}-EBOOT.PBP')
docs = ['README.md','LICENSE','CHANGELOG.md','docs/FIELD_GUIDE.md','docs/RELEASE_NOTES.md','docs/VALIDATION.md']
archive(root/'dist'/f'Dustbound-{version}-PSP.zip',[(eboot,'PSP/GAME/DUSTBOUND/EBOOT.PBP')]+[(root/p,f'Dustbound-{version}/{p}') for p in docs])
evidence = [p for p in sorted((root/'evidence').iterdir()) if (p.name.startswith('v021-') or p.name.startswith('catalog')) and p.suffix != '.log']
archive(root/'dist'/f'Dustbound-{version}-Evidence.zip',[(p,'evidence/'+p.name) for p in evidence]+[(root/'docs/VALIDATION.md','VALIDATION.md')])
files = sorted((root/'dist').glob(f'Dustbound-{version}-*'))
(root/'dist'/f'SHA256SUMS-{version}.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in files))
print((root/'dist'/f'SHA256SUMS-{version}.txt').read_text())
