# 0.10.0 validation

Qualified November 8, 2026. PSPDEV v20261101; PPSSPP 1.20.4 headless. The installer executable must match the completed controller run, and CI independently rebuilds and enforces that match.

Publication verification repeated a clean PSP rebuild, all four native regression scripts (080, 081, 090 and 100), and the complete 82-capture controller run. The clean build reproduced the qualified digest below. Installer verification checked the memory-stick path, ZIP integrity, absence of QA/save files, and the embedded executable digest. The fresh [verification record](../evidence/v100-publication-checks.json) and [native output](../evidence/v100-publication-native-checks.txt) accompany the original qualification evidence.

- EBOOT: **1,634,502 bytes**, SHA-256 `02af5224b66e80639a2ee3d5c7faec5b77c76141907dd09ab368bd733dd9bf09`.
- Save v10: `Game` is 626,860 bytes. An actual v0.9 save, rather than a relabeled current struct, passes migration, resave/reload and repeated migration checks. Player buildings and existing XP/item IDs are retained.
- ELF text/data/BSS: 1,192,792 / 3,516 / 5,010,624 bytes. These are build sizes, not a measured physical-PSP memory high-water mark.
- Fresh world: 173 actors and 267 buildings; bounded actor pools remain in use.

## Native checks

`bash scripts/validate_v100.sh` passes:

- All 25 species/origin combinations, species movement/load/food and species-aware healing/repair, including barber preservation.
- Eighteen distinct persistent owners, 36 physically carried personal items, 96 new armor variants, traveling owners, and no loot refill after death/load.
- Service access in all 15 rebuilt settlements, open gates in both fortresses, continuous wall seams, player-built structure preservation and idempotent migration.
- At least 12 distinct **monochrome silhouettes** for each of 38 weapon types, so color alone cannot satisfy the construction check. 14,592 variant/direction/phase renders stay within the framebuffer.
- 5,760 grip samples, 14,976 attack poses inside the actor cache, 36 bounded/cache-stable comparisons, 288 reflections preserving edge orientation, 1,152 backpack attachment renders and 72 headwear fit checks.
- Owned/required ingredient quantities (green when sufficient, including exact amounts; red when short), grain/meat alternatives, research/material/fuel shortages, no mutation on failed work, output counts and exact material/fuel deductions on success. All 38 weapon and 20 torso designs are selectable; named regalia cannot be forged.

Regression scripts `validate_v080.sh`, `validate_v081.sh` and `validate_v090.sh` pass. Coverage includes the core and expansion suites, 1,200 renewed event incidents, bounded armies, atlas event-marker suppression, trade and contracts, warrants and disguises, cannibal captivity, legacy saves, combat progression, and freed NPCs following/fleeing/dismissing independently of recruitment. The UI pass renders 422 screen/biome/guide cases with correct 480×272 bounds and 512-pixel stride.

The 0.9 regression's 1,000 full-world ticks with 173 actors took 0.126 seconds on the native test CPU. This is **not a PSP frame-rate result**.

## Actual PSP executable

`python3 scripts/emulator_v100.py /path/to/PPSSPPHeadless` completed with **82 full 480×272 framebuffer captures**, recorded in [the controller evidence](../evidence/v100-controller-checks.json). All captures have their expected screen number. The final record names the exact EBOOT hash above.

The harness uses normal prepared save files and emulated buttons. Its QA snapshot/framebuffer mechanism reads state and requests a completed frame; it does not write game memory. Prepared saves isolate the review scenarios; they are not claimed to be naturally earned campaign progress.

Verified in the executable:

- All five creation choices; a created Stonekin actually has 130 limb health and 15 starting strength.
- Six settlement views.
- Physical looting, inspection and equipment of a named owner's weapon.
- Six constructions each for eight weapon families in the inventory, plus each family's equipped world view. The other families have production-renderer coverage and sheets, not a claim of separate PSP controller inspection for every catalog entry.
- Missing requirements, out-of-range workstations, ready recipes, alternative food ingredients and forging readiness. A fine Scrap Rapier is actually forged, followed by two iron bars; materials and fuel decrease accordingly.

Earlier harness attempts stopped on incorrect expected screen IDs and a text-match assertion. Those harness mistakes were corrected; the completed run is the evidence used here. Earlier debug failure captures are excluded from the release. No runtime crash is hidden by that exclusion.

Visual review also caught and corrected detached blade fittings, the ornate scrap-village tavern, long-weapon attack clipping and inaccurate creation loadouts/health assumptions before the final run.

## Limits

This qualifies the listed scenarios, not every interaction in a full natural campaign. No physical PSP session, battery/suspend test, extended economy balance run or political succession system is claimed. Shared art families and procedural silhouettes are disclosed in the content notes. See [the current critique](DESIGN_REVIEW_V100.md) for remaining design work.
