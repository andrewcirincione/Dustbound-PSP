# 0.12.0 validation

Qualified EBOOT SHA-256: `04cfa557c5fabc94541821529b194e369eead8febb3cb509e1b62b48b2dba918` (1,708,502 bytes). PSPDEV v20261101 is checksum-pinned in CI. PPSSPP 1.20.4 used the software renderer. Publication rebuilds the executable and requires this exact digest.

## Native checks

Release gates `validate_v080.sh`, `validate_v081.sh`, `validate_v090.sh`, `validate_v100.sh`, `validate_v110.sh` and `validate_v120.sh` passed. Logs are in `evidence/v120-native-checks.txt`.

The population suite runs under AddressSanitizer and UndefinedBehaviorSanitizer and verifies:

- Six seeded worlds, all sixteen situations, at least six biomes, walkable centers, 80 stored groups and 297–309 population members per sampled world.
- 80,000 independent humanoid generation trials across four tiers: 826/20,000 wearers at each low tier, 1,439/20,000 in tier two, and 2,031/20,000 in tier three. Five Relic parts in the strongest tier; none below it. Beasts never receive prosthetics.
- Actor identity, health, gear, inventory, stolen ownership, fitted grades, trauma, class experience, corpses and empty cache state through unload/reload and actual save/load. Taking a fitted limb cannot duplicate it or regrow it on returning.
- Split-squad and carried-actor retention; freed followers detach without implicit recruitment. Physical convoy prisoners can be rescued using the existing timed restraint system, retain an independent follow/flee decision and survive saving.
- Actual nearby-action bed recovery, free use, finite shop stock and exact payment, cooking deductions, full-pack rollback, enemy and distance rejection, object reach when terrain displaces a bed, and queued-action save/load.
- Real off-screen displacement, depletion of actual resource nodes and matching carried resources, faction battle wounds and persistent outcomes without awarding player kills or money.
- Whole-group allocation refusal when live actor slots are full. Protected actors are not recycled. Newly dead old groups do not immediately renew; distant aftermath renewal waits the required aftermath time.
- Migration from an actual released v11 fixture, current-format reload, invalid version/type rejection, sixteen world/action framebuffer guard checks and prepared ordinary emulator saves.

Earlier release gates retain core (61/61), expansion (50/50), living-world, atlas, society, class progression, captivity/rescue, species/civilization, crafting, gear/grip/attachment and all five-grade limb mechanics. The renderer suite checks 422 screen/biome/guide cases; weapon geometry checks cover all 38 types, 14,592 direction/phase renders, 5,760 grip samples and 14,976 attack bounds.

Actor size remains 932 bytes. Game size is 2,127,704 bytes, including a 1,500,196-byte appended population. Save version is 12; the v11 prefix remains unchanged. The PSP ELF has 1,253,196 bytes of text, 3,516 data and 6,525,940 BSS; the program keeps its existing 12 MiB heap and 32 MiB memory mode. Native CPU timing is recorded as a host benchmark, not PSP performance.

The historical omnibus `build_native.sh` was not used as this release gate. Its known obsolete five-option dialogue assertion is described in the 0.11.0 validation record; no full historical-runner pass is claimed here.

## Final PSP executable

`scripts/emulator_v120.py` controls the final EBOOT using ordinary emulated buttons and analog-stick input. QA only exports state and completed framebuffers. It does not modify game state.

The completed run captured **33 distinct 480×272 frames** and verified:

1. Ordinary character creation, a fresh 80-group population (320 members, 13 wearing prosthetics in that generated world), save and reload.
2. All sixteen encounter types, their actual members and camp objects, through prepared ordinary saves; six action-list layouts.
3. A 12–16-member army moving without player orders.
4. Starting free bed rest through the nearby-action menu, increased arm health and reduced fatigue, then waking with actual analog movement.
5. Buying a ration with exact payment and stock decrement, and cooking with exact meat consumption and ration output.
6. Saved and reloaded camp stock and money.
7. An actually randomly generated pilgrim with a fitted Scrap arm: loot that part through the UI, check the owner loses it, attempt another take without another limb, save, reload and confirm the fitted item remains absent.

All sixteen world screenshots and the camp/loot UI frames were visually reviewed. Review replaced the mistaken tent graphic at sleeping spots with the existing bed graphic, retained a separate tent, and repositioned prepared scenario viewpoints so their actors are visible. Controller-harness corrections replaced obsolete D-pad movement with analog input, matched the actual “Trail ration” label, and stopped trying to select an empty row after the compact loot list correctly removed it. The entire final controller run then passed on the digest above.

This establishes prepared-scenario emulator behavior and a fresh-start/save check. It does not establish a naturally played full campaign or sustained physical PSP performance. Offline combat is intentionally coarse; long-run population density, encounter economics and campaign balance remain review priorities.
