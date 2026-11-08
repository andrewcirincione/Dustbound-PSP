# 0.11.0 validation

Qualified EBOOT SHA-256: `8bec63476ea8c90e5ad6c846a1e67ef53293d2aa3e6c07c54007a011cc9a4282` (1,662,478 bytes). PSPDEV v20261101 is checksum-pinned in CI. PPSSPP 1.20.4 used its software renderer. CI rebuilds and refuses publication unless its EBOOT matches this locally emulated binary.

## Native checks

Release gates `validate_v080.sh`, `validate_v081.sh`, `validate_v090.sh`, `validate_v100.sh` and `validate_v110.sh` passed. Logs are in `evidence/v110-native-checks.txt`.

- Core: 61/61; expansion: 50/50. Living-world generation, event storage, formation, rewards, target retaliation and earlier-save migration.
- Atlas discovery, selection, zoom, unchanged travel orders and D-pad movement isolation.
- Society, actual v8 migration, contracts, captivity, stat/class training, rescue/flee/follow/recruit behavior and 422 framebuffer-bounded screen renders.
- Civilizations/species, actual v9 migration, named owners, crafting deductions and rollback; all 38 weapon types retain at least 12 monochrome constructions, with 14,592 direction/phase renders, 5,760 grip samples, 14,976 bounds checks and garment/backpack attachment checks.
- Limb suite under AddressSanitizer and UndefinedBehaviorSanitizer: cutting versus blunt severing, ground remains, one/two-leg crawl speeds, dressing without regrowth, either-hand weapons, two-handed penalties, wounded remaining-arm condition, no-arm unarmed range, all five grades in all four sockets, parts cost, detachment, full-pack rollback, stolen ownership, copied actors, per-grade HP, repairs, mechanical destruction without bleeding, safe/reachable surgery, permanent paid removal, shop distribution, real purchases, one-time fitted loot, and actual v10 migration/save reload.
- Four new menus stay within the framebuffer guard. Actor size remains 932 bytes; Game is 627,508 bytes.

The older `build_native.sh` was also attempted. Core, expansion, 422 screen renders, both 2,112-item catalog audits and 26,649 overhaul checks passed, then its historical playability test stopped at an obsolete assertion limiting dialogue to five options (current merchants have six). That historical runner is **not** reported as fully passing, and its remaining suites were not counted. Gear-only catalog assertions were narrowed to the pre-limb item range because new prostheses are inventory parts, not armor slots. The current release gates above were all completed separately.

## Actual PSP executable

`scripts/emulator_v110.py` uses normal emulated buttons and analog-stick input. Prepared ordinary saves establish the scenarios. The QA interface only exports state and completed framebuffers; it never modifies game state. The final run captured 39 completed 480×272 frames (38 distinct image filenames; the Relic arm comparison is revisited).

Verified through the PSP UI:

1. Talk to Surgeon Venn and buy from his Mechanical Limbs shop.
2. Cancel removal without payment or loss; confirm right-arm removal and verify exactly 400 silver deducted.
3. Fit every grade of arm through the comparison screen, verify two parts consumed each time, detach and refit.
4. Remove a left leg, move by crawling, fit every grade of leg, and return to upright movement.
5. Save and reload actual newly fitted arm/leg grades and remaining money.
6. Move with one leg missing, both legs missing and one arm missing; equip a rapier into the remaining hand.
7. Loot Iron Pilgrim's actual fitted Relic arm, verify inventory increases once and the owner's fitted slot becomes empty.
8. Buy a Relic leg from Black Mesa's ordinary market.
9. Inspect four fitted Relic parts, 180-HP limb bars and effective stat bonuses.

Screenshots were visually reviewed for missing-limb anatomy, crawling, attachment, text fit, comparative stats, warnings, shops, loot and health. Review caught and corrected the Health footer's R-button label and a misleading bandage warning after closed-wound surgery. Earlier harness attempts stopped at a merchant label lookup; that lookup was corrected and the entire final run completed. Combat review also corrected remaining-arm injury handling and prevented no-arm kicks from inheriting a ranged weapon's reach/ammunition requirement.

The controller record and manifest identify the final binary. This is prepared-scenario emulator verification, not a naturally earned campaign or physical PSP test. Long-run amputation frequency, prosthetic economy and sustained hardware performance remain unqualified.
