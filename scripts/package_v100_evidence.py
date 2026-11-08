#!/usr/bin/env python3
"""Package existing completed captures; this does not run or claim new tests."""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
E = ROOT / 'evidence'
font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 16)
small = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 13)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

checks = json.loads((E / 'v100-controller-checks.json').read_text())
assert checks[-1]['completed']
assert checks[-1]['eboot_sha256'] == digest(ROOT / 'EBOOT.PBP')
captures = [c for c in checks if 'capture' in c]
assert len(captures) == 82
for capture in captures:
    assert capture['screen'] == capture['expected']
    assert Image.open(E / (capture['capture'] + '.png')).size == (480, 272)

def sheet(name, title, entries, cols=2):
    rows = (len(entries) + cols - 1) // cols
    im = Image.new('RGB', (cols * 488 + 8, 56 + rows * 306), '#191a18')
    d = ImageDraw.Draw(im)
    d.text((12, 8), title, font=font, fill='#e4d2a4')
    d.text((12, 31), 'Actual PSP executable in PPSSPP / full 480 x 272 frames / prepared review saves', font=small, fill='#b9bbae')
    for i, (path, label) in enumerate(entries):
        x, y = 8 + (i % cols) * 488, 54 + (i // cols) * 306
        d.text((x + 4, y + 1), label, font=small, fill='#e4d2a4')
        im.paste(Image.open(E / ('v100-psp-' + path + '.png')), (x, y + 24))
    im.save(E / ('v100-review-' + name + '.png'))

sheet('crafting', 'Crafting requirements and readiness', [
    ('crafting-missing', 'Missing materials and research'), ('crafting-ready', 'Materials, station and fuel available'),
    ('forging-missing', 'Equipment: unmet requirements'), ('forging-ready', 'Equipment: ready to make a rapier')])
sheet('species', 'Species and origin determine starting statistics', [
    ('species-3', 'Reedkin: fast, fragile, lower food use'), ('species-4', 'Stonekin: strong, tough, slower and hungry')])
sheet('towns', 'Civilizations have different settlement plans', [
    ('town-fortress', 'Fortress court'), ('town-village', 'Scrap village'),
    ('town-temple', 'Temple cloister'), ('town-foundry', 'Foundry enclave'),
    ('town-cannibals', 'Cannibal cooking ground'), ('town-grove', 'Grove market')])
sheet('named-loot', 'Personal equipment can be physically looted and equipped', [
    ('named-loot', 'Glass Ronin: real equipment in the loot screen'), ('glass-horizon', 'Glass Horizon: inventory inspection')])

im = Image.new('RGB', (884, 676), '#191a18')
d = ImageDraw.Draw(im)
d.text((12, 8), 'Weapon constructions across four different classes', font=font, fill='#e4d2a4')
d.text((12, 31), 'Actual PSP inventory portraits / cropped and enlarged 2x / full original frames included', font=small, fill='#b9bbae')
families = [(0, 'Katana'), (32, 'Greatsword'), (29, 'Ranger rifle'), (37, 'Rapier')]
for row, (typ, label) in enumerate(families):
    y = 61 + row * 150
    d.text((12, y), label, font=font, fill='#e4d2a4')
    for col, variant in enumerate((0, 2, 7, 11)):
        src = Image.open(E / f'v100-psp-weapon-{typ:02d}-{variant:02d}.png')
        crop = src.crop((28, 81, 131, 146)).resize((206, 130), Image.Resampling.NEAREST)
        im.paste(crop, (12 + col * 218, y + 21))
im.save(E / 'v100-review-weapons.png')

for i in range(38):
    Image.open(ROOT / f'build/v100/weapons-{i:02d}.ppm').save(E / f'v100-weapons-{i:02d}.png')

manifest = {
    'version': '0.10.0', 'save_version': 10, 'qualified_date_utc': '2026-11-08',
    'eboot_sha256': digest(ROOT / 'EBOOT.PBP'), 'eboot_bytes': (ROOT / 'EBOOT.PBP').stat().st_size,
    'prx_sha256': digest(ROOT / 'dustbound.prx'), 'game_bytes': 626860,
    'elf_text_bytes': 1192792, 'elf_data_bytes': 3516, 'elf_bss_bytes': 5010624,
    'catalog_items': 2368, 'species': 5, 'named_owners': 18, 'personal_items': 36,
    'new_armor_variants': 96, 'weapon_types': 38, 'variants_per_weapon_type': 24,
    'monochrome_constructions_per_weapon_type': 12, 'fresh_actor_count': 173, 'fresh_building_count': 267,
    'native_screen_renders': 422, 'variant_pose_renders': 14592, 'grip_samples': 5760,
    'attack_pose_bounds_checks': 14976, 'reflection_checks': 288,
    'psp_framebuffer_captures': len(captures), 'psp_emulator_controller_completed': True,
    'physical_psp_tested': False, 'natural_full_campaign_tested': False,
    'controller_evidence': 'evidence/v100-controller-checks.json',
    'capture_method': 'Completed PSP EBOOT framebuffers; controller-driven prepared saves; read-only QA snapshots',
    'weapon_sheets': 'Native production renderer at 3x; not PSP framebuffer captures',
}
(E / 'v100-build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Packaged {len(captures)} PSP frames, 5 comparison sheets and 38 weapon sheets; build hash matches.')
