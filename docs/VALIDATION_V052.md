# 0.5.2 validation record

All fourteen final controller scenarios identify the same packaged EBOOT. `scripts/package_v052.py` rejects mismatched executable hashes or missing completion assertions. Screenshots are the actual completed 480×272 framebuffer written by the PSP executable under the opt-in QA marker, before the same frame is displayed. The combat GIF uses 240 recorded frames. The controller harness sends PSP button inputs and reads telemetry; it does not write game state. Prepared saves and QA markers are excluded from the installer.

## Build identity

- EBOOT: **1,346,834 bytes**.
- SHA-256: `6dbbd8e09ed10412913dd4835be13f985433e52eb41096ba0985e93ae03dfffb`.
- Official PSPDEV `v20261101`, PSP user-mode PRX, 333 MHz, 12 MiB heap, no expanded-memory flag.
- Writes save format 5 and loads versions 1–5. Genuine v0.3, v0.4.1 and released v0.5.0 saves are regression fixtures. Equipment IDs and the 410,656-byte game-state layout remain stable. v0.5.1 clothing balance is retained; new profession perks apply to existing skill levels.
- Physical PSP-3000 testing: **not performed**. Emulator results do not establish hardware frame rate, battery life, sleep/resume or memory-stick reliability.

## Native checks

| Check | Result / scope |
|---|---|
| Critical audit | 70 assertions for exact-slot ownership, stack rollback, consumed-ingredient provenance, navigation, rescue, logistics, reachable services, map orders, paused custody and meaningful skill effects |
| Core simulation | 61 core and 50 expansion assertions |
| Playability | 1,541 assertions for interaction reach, NPC roles, recruitment, logistics, travel, town spacing and migration |
| Interior/save/grip baseline | 26,649 assertions, including 13,312 independently expected support-hand poses |
| Rendering | 291 screen, biome, guide and interior cases preserve framebuffer bounds/stride |
| Gear catalog | All 2,112 modular variants have distinct hashes at gameplay and portrait scales |
| Painted weapons | 5,760 handle-anchor samples, 14,976 cached attack poses and 36 bounded, cache-stable comparison pairs |
| Cutting-edge direction | 288 asymmetric raster reflection comparisons across 36 families and eight angles |
| Backpack fit | 1,152 combinations of four bags, 16 torsos, three bodies, two facings and three poses stay visibly on the upper back |
| Hat fit | 72 combinations of 12 headwear designs, three bodies and two facings overlap the brow and stay within fitted bounds |
| Clothing balance | 1,056 material/quality variants preserve quality progression and live passive bonuses; 96 matched heavy/light comparisons establish protection/mobility tradeoffs |
| Backpack behavior | 192 actual swaps match projected load; all 16 markets stock four accessible designs; cargo, speed, medical effects and one-time old-save stock upgrade pass |
| Journal | Destination and return-location tracking preserve the existing route; supply counts reflect held items |
| Frontier systems | Service separation, regional stock, four heavy weapons, strength/arm handling penalties, guard hire/payment/follow/heal/expiry/dismissal and genuine v4 migration |
| Sanitizers | Critical-audit, renderer, frontier, painted-weapon, attachment, backpack and clothing suites pass AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled in this environment |

Native review sheets show all 44 clothing designs in both facings, all eight back designs while standing/crouching/walking, and all 36 weapons at guard/windup/contact/recovery. The shorter prone renderer remains simplified and does not show every worn attachment. These checks do not claim every possible garment combination or campaign state was manually inspected.

## PSP controller scenarios

| Scenario | Evidence |
|---|---|
| Critical interaction audit | Title/confirmation cancel, contextual-help restoration, rescue choices, carry before first aid, comparison loot, stolen bulk stack, skill/body/squad screens, cook fetch/craft/store loop, nearby-target cycling without squad switch, bulk depot, recipes, forge, messages and save-and-title |
| Fresh start | Creation, far-mining refusal, close mining, recruitment, buying, loot/equip, save/reload, both atlas scales and guide navigation |
| Prepared combat/outpost | Greatsword windup/contact/recovery, wounds, forging, constructed outpost, faction ledger and distant regions; 240 recorded frames |
| Interiors | Shop entry, counter trade, indoor save/reload, exit, inn entry, paid sleep, exit and contextual guide |
| Fresh wilderness journey | Dusk Haven to Copper Cross with enemies enabled; atlas browsing preserves the route; arrival asserted |
| Frontier identity | Fresh guard hire and duplicate-charge prevention, escort/save/reload; prepared faction markets/four heavy weapons; genuine v0.4.1 save loaded |
| Fresh backpack expedition | Buy three alternatives using normal starting money, wear all four practical packs, compare loaded stats, accept marshal work, locate objective, save/reload |
| Weapon catalog, three scenarios | Equip all 36 weapon families and capture both facings |
| Back catalog | Equip all eight back designs and capture both facings |
| Clothing catalog, three scenarios | Equip all 44 clothing designs, capture their stat comparisons and both facings |

Prepared catalog/encounter saves are QA fixtures, not naturally earned expeditions. Long-duration contract expiry and other permutations receive native coverage rather than a full simulated day in each controller scenario.

## Interrupted attempts and corrections

An intermediate v0.5.2 build failed at startup. A text-cleanup replacement had also changed eight bitwise color-packing operators. The operators were restored, the renderer was added to AddressSanitizer/UndefinedBehaviorSanitizer coverage, and the clean rebuilt EBOOT then passed the complete fourteen-scenario run. The failure is retained in `v052-audit-startup-regression.txt`; that intermediate binary was never packaged.

Three audit-harness assumptions were corrected: signed `carrying == -1` telemetry, selecting a depot instead of its nearby doorway, and directional button-repeat overshoot in the squad menu. The corresponding signed-telemetry, door-selection and button-repeat logs are retained. The final context-cycling scenario explicitly checks that the selected squad member does not change. The latest full validation completed without retries or source changes between scenarios.

Native tests for crafting/sleeping now move the actor into the actual station or bed reach. No production reach checks were relaxed for tests. Additional provenance checks distinguish stolen ingredients that were consumed from stolen ingredients merely carried in a later inventory stack.

These short scenarios do not guarantee long-session stability or every campaign state. Prepared rescue, loot, camp and catalog saves are fixtures, not naturally earned expeditions. All 27 screen types were rendered and visually reviewed in three native contact sheets; representative interactions were also exercised with PSP controller inputs. Native screen sheets are labeled separately from EBOOT captures.

## Memory and cost

The existing 144 indexed sprites and 32 shadow masks remain **554,886 bytes**, including 77,542 shadow bytes. Palettes remain 147,456 bytes. No new textures or larger actor caches were introduced. Fixed caches: 24 × 112×104 world sprites (1,118,208 pixel bytes), 224×208 scratch (186,368), two 160×128 previews (163,840), ground cache (731,136), staged framebuffer (557,056).

The PRX has **4,055,020 BSS bytes**, 964,364 code/read-only bytes and 3,364 initialized-data bytes; total 5,022,748. BSS includes the caches, catalog and 410,656-byte game state; do not count those twice. Two VRAM framebuffers separately use 1,114,112 bytes. Heap remains 12 MiB. The loader briefly holds both raw and decoded saves. There is no runtime image decoder or per-outfit texture allocation.

Host microbenchmarks use 20 warm-up and 240 measured frames in five scenes. Stationary medians were 0.109–0.276 ms; scrolling medians 0.108–0.373 ms. CSVs include p95/worst values. These are host render timings, not PSP FPS or a controlled speedup comparison. The benchmark's Outpost label is an open-frontier camera location; constructed-outpost gameplay is tested separately.

## Reproduction

Configure `PSPDEV` and its `bin` directory on `PATH`; install Pillow/websockets and build PPSSPPHeadless separately:

```sh
bash scripts/validate_v052.sh /path/to/PPSSPPHeadless
python3 scripts/package_v052.py
```

The validation script stops on errors. Investigate a failure before resuming the affected stage; do not count retries as successes. Packaging requires all fourteen completed controller records from the exact executable. See the [critical design review](DESIGN_REVIEW_V052.md), [field guide](FIELD_GUIDE.md) and [balance table](../evidence/v052-clothing-balance.csv) for gameplay rules and remaining limitations.
