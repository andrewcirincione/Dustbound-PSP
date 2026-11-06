#!/usr/bin/env python3
"""Check that the installer, evidence and optional fresh build describe one tested EBOOT."""
import argparse
import hashlib
import json
import pathlib
import zipfile

VERSION = '0.6.0'
SUITES = ('depth', 'emulator', 'advanced', 'audit', 'interior', 'travel', 'campaign')
ARTIFACTS = tuple(f'Dustbound-{VERSION}-{suffix}' for suffix in ('EBOOT.PBP', 'Evidence.zip', 'PSP.zip'))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify_release(root, eboot=None):
    root = pathlib.Path(root)
    dist, evidence = root / 'dist', root / 'evidence'
    manifest = json.loads((evidence / 'v060-build-manifest.json').read_text())
    expected = manifest['eboot_sha256']
    require(manifest['version'] == VERSION, 'Wrong release version')
    require(manifest['controller_suites'] == list(SUITES), 'Controller suite list changed')
    require('Final native, sanitizers, PSP package and controller scenarios passed.' in
            (evidence / 'v060-qualification.txt').read_text(), 'Qualification did not finish')
    sums = {}
    for line in (dist / f'SHA256SUMS-{VERSION}.txt').read_text().splitlines():
        checksum, name = line.split()
        require(name not in sums, 'Duplicate checksum entry')
        sums[name] = checksum
    require(set(sums) == set(ARTIFACTS), 'Incomplete release artifact set')
    for name, checksum in sums.items():
        require(digest((dist / name).read_bytes()) == checksum, f'Checksum mismatch: {name}')
    binary = (dist / ARTIFACTS[0]).read_bytes()
    require(digest(binary) == expected and len(binary) == manifest['eboot_bytes'], 'EBOOT differs from qualification')
    if eboot is not None:
        require(pathlib.Path(eboot).read_bytes() == binary, 'Fresh build differs from qualified EBOOT')
    for suite in SUITES:
        records = json.loads((evidence / f'v060-{suite}-checks.json').read_text())
        require(records[0]['eboot_sha256'] == expected, f'Wrong controller binary: {suite}')
        last = records[-1]
        complete = (last.get('completed') if suite in ('depth', 'audit') else
                    last.get('normal_run_saved_reloaded') if suite == 'emulator' else
                    set(last.get('combat_phases_observed', [])) == {0, 1, 2, 3} if suite == 'advanced' else
                    last.get('indoor_save_reload') and last.get('doors_enter_exit') if suite == 'interior' else
                    last.get('completed') and last.get('escort_bypass_report_saved_reloaded') if suite == 'campaign' else
                    last.get('fresh_start_journey') and last.get('map_browsing_preserved_route'))
        require(complete, f'Incomplete controller suite: {suite}')
        if suite == 'depth':
            require(last.get('knockout_route_preserved'), 'Missing knockout travel acceptance')
        if suite == 'campaign':
            require(last.get('escort_followup_care'), 'Missing ongoing escort care acceptance')
            require(last.get('escort_carried_to_arrival'), 'Missing carried escort acceptance')
    with zipfile.ZipFile(dist / ARTIFACTS[2]) as archive:
        require(archive.testzip() is None, 'Damaged installer ZIP')
        require(archive.read('PSP/GAME/DUSTBOUND/EBOOT.PBP') == binary, 'Installer contains a different EBOOT')
        forbidden = ('DUST_QA', 'DUST_RECORD', 'DUST_CAPTURE', 'DUST_STATE')
        require(not any(any(marker in name for marker in forbidden) or '.sav' in name.lower()
                        for name in archive.namelist()), 'Installer contains developer state')
    with zipfile.ZipFile(dist / ARTIFACTS[1]) as archive:
        require(archive.testzip() is None, 'Damaged evidence ZIP')
        for name in ('v060-build-manifest.json', 'v060-combat.mp4', 'v060-qualification.txt',
                     *(f'v060-{suite}-checks.json' for suite in SUITES)):
            require(archive.read('evidence/' + name) == (evidence / name).read_bytes(),
                    f'Evidence archive differs from qualified record: {name}')
    return expected


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument('--eboot', type=pathlib.Path, default=pathlib.Path('EBOOT.PBP'))
    args = parser.parse_args()
    print('Release integrity PASS:', verify_release(args.root, args.eboot))
