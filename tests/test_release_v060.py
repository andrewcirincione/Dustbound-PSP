"""Reject stale/replaced installer payloads even if their outer checksum was updated."""
import hashlib
import json
import pathlib
import shutil
import sys
import tempfile
import unittest
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_release_v060 import ARTIFACTS, verify_release


class ReleaseIntegrity(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = pathlib.Path(self.directory.name)
        shutil.copytree(ROOT / 'dist', self.root / 'dist', ignore=lambda _, names: [n for n in names if '0.6.0' not in n])
        (self.root / 'evidence').mkdir()
        for name in ('v060-build-manifest.json', 'v060-combat.mp4', 'v060-qualification.txt',
                     'v060-depth-checks.json', 'v060-emulator-checks.json', 'v060-advanced-checks.json',
                     'v060-audit-checks.json', 'v060-interior-checks.json', 'v060-travel-checks.json',
                     'v060-campaign-checks.json'):
            shutil.copy2(ROOT / 'evidence' / name, self.root / 'evidence' / name)

    def alter_installer(self, name, payload):
        path = self.root / 'dist' / ARTIFACTS[2]
        with zipfile.ZipFile(path) as old:
            entries = {key: old.read(key) for key in old.namelist()}
        entries[name] = payload
        with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as new:
            for key, value in entries.items():
                new.writestr(key, value)
        (self.root / 'dist' / 'SHA256SUMS-0.6.0.txt').write_text(''.join(
            hashlib.sha256((self.root / 'dist' / item).read_bytes()).hexdigest() + '  ' + item + '\n'
            for item in ARTIFACTS))

    def test_qualified_package(self):
        verify_release(self.root, ROOT / 'dist' / ARTIFACTS[0])

    def test_stale_installer_binary(self):
        self.alter_installer('PSP/GAME/DUSTBOUND/EBOOT.PBP', b'old executable')
        with self.assertRaisesRegex(ValueError, 'different EBOOT'):
            verify_release(self.root)

    def test_developer_save_excluded(self):
        self.alter_installer('PSP/GAME/DUSTBOUND/DUST_1.sav', b'test state')
        with self.assertRaisesRegex(ValueError, 'developer state'):
            verify_release(self.root)

    def test_unfinished_controller_run(self):
        path = self.root / 'evidence' / 'v060-depth-checks.json'
        records = json.loads(path.read_text())
        records[-1]['completed'] = False
        path.write_text(json.dumps(records))
        with self.assertRaisesRegex(ValueError, 'Incomplete controller suite'):
            verify_release(self.root)

    def test_fresh_build_mismatch(self):
        path = self.root / 'EBOOT.PBP'
        path.write_bytes(b'unqualified executable')
        with self.assertRaisesRegex(ValueError, 'Fresh build differs'):
            verify_release(self.root, path)

    def test_missing_knockout_acceptance(self):
        path = self.root / 'evidence' / 'v060-depth-checks.json'
        records = json.loads(path.read_text())
        records[-1].pop('knockout_route_preserved', None)
        path.write_text(json.dumps(records))
        with self.assertRaisesRegex(ValueError, 'Missing knockout travel acceptance'):
            verify_release(self.root)

    def test_missing_qualification_completion(self):
        (self.root / 'evidence/v060-qualification.txt').write_text('Interrupted')
        with self.assertRaisesRegex(ValueError, 'Qualification did not finish'):
            verify_release(self.root)

    def test_unfinished_natural_campaign(self):
        path = self.root / 'evidence/v060-campaign-checks.json'
        records = json.loads(path.read_text())
        records[-1]['completed'] = False
        path.write_text(json.dumps(records))
        with self.assertRaisesRegex(ValueError, 'Incomplete controller suite: campaign'):
            verify_release(self.root)

    def test_missing_carried_escort_acceptance(self):
        path = self.root / 'evidence' / 'v060-campaign-checks.json'
        records = json.loads(path.read_text())
        records[-1].pop('escort_carried_to_arrival', None)
        path.write_text(json.dumps(records))
        with self.assertRaisesRegex(ValueError, 'Missing carried escort acceptance'):
            verify_release(self.root)


if __name__ == '__main__':
    unittest.main()
