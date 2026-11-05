# 0.5.1 validation record

All thirteen final controller scenarios identify the same packaged EBOOT. `scripts/package_v051.py` rejects mismatched executable hashes or missing completion assertions. Screenshots are the actual completed 480×272 framebuffer written by the PSP executable under the opt-in QA marker, before the same frame is displayed. The combat GIF uses 240 recorded frames. The controller harness sends PSP button inputs and reads telemetry; it does not write game state. Prepared saves and QA markers are excluded from the installer.

## Build identity

- EBOOT: **1,323,330 bytes**.
- SHA-256: `34a577821da3730b56f2cc969fdafe273926482569227d97da20e1b1b8554ade`.
- Official PSPDEV `v20261101`, PSP user-mode PRX, 333 MHz, 12 MiB heap, no expanded-memory flag.
- Writes save format 5 and loads versions 1–5. Genuine v0.3, v0.4.1 and released v0.5.0 saves are regression fixtures. Equipment IDs remain stable; statistics intentionally update to the new balance.
- Physical PSP-3000 testing: **not performed**. Emulator results do not establish hardware frame rate, battery life, sleep/resume or memory-stick reliability.

## Native checks

| Check | Result / scope |
|---|---|
| Core simulation | 61 core and 50 expansion assertions |
| Playability | 1,541 assertions for interaction reach, NPC roles, recruitment, logistics, travel, town spacing and migration |
| Interior/save/grip baseline | 26,649 assertions, including 13,312 independently expected support-hand poses |
| Rendering | 273 screen, biome, guide and interior cases preserve framebuffer bounds/stride |
| Gear catalog | All 2,112 modular variants have distinct hashes at gameplay and portrait scales |
| Painted weapons | 5,760 handle-anchor samples, 14,976 cached attack poses and 36 bounded, cache-stable comparison pairs |
| Cutting-edge direction | 288 asymmetric raster reflection comparisons across 36 families and eight angles |
| Backpack fit | 1,152 combinations of four bags, 16 torsos, three bodies, two facings and three poses stay visibly on the upper back |
| Hat fit | 72 combinations of 12 headwear designs, three bodies and two facings overlap the brow and stay within fitted bounds |
| Clothing balance | 1,056 material/quality variants preserve quality progression and live passive bonuses; 96 matched heavy/light comparisons establish protection/mobility tradeoffs |
| Backpack behavior | 192 actual swaps match projected load; all 16 markets stock four accessible designs; cargo, speed, medical effects and one-time old-save stock upgrade pass |
| Journal | Destination and return-location tracking preserve the existing route; supply counts reflect held items |
| Frontier systems | Service separation, regional stock, four heavy weapons, strength/arm handling penalties, guard hire/payment/follow/heal/expiry/dismissal and genuine v4 migration |
| Sanitizers | Frontier, painted-weapon, attachment, backpack and clothing suites pass AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled in this environment |

Native review sheets show all 44 clothing designs in both facings, all eight back designs while standing/crouching/walking, and all 36 weapons at guard/windup/contact/recovery. The shorter prone renderer remains simplified and does not show every worn attachment. These checks do not claim every possible garment combination or campaign state was manually inspected.

## PSP controller scenarios

| Scenario | Evidence |
|---|---|
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

## Interrupted attempts and harness corrections

Earlier live-debugger-memory attempts timed out or reported `CPU not started`; three interruption logs are retained. They were not conclusively diagnosed as game or emulator faults. Final instrumentation avoids those pause/read/resume operations: optional read-only snapshots and completed-frame files supply telemetry, while the debugger only starts execution and sends button input. Atomic capture-request delivery also fixes a host-side empty-request race found during instrumentation work. QA file I/O is disabled without the marker.

The final batch initially stopped before the interior scenario because its Python `Play` instance lacked the telemetry-directory assignment. The retained `v051-harness-interior-setup.txt` records that error. The runner was corrected and the affected interior scenario passed on the unchanged EBOOT, followed by travel, sanitizers and benchmarks. The interior guide navigation was also updated for the twentieth chapter. Completed matching-hash records, not failed attempts, qualify the package. These short scenarios are not a long-session stability guarantee.

The armor rebalance also exposed a native test dependency: the preceding crossbow shot left its target stunned. The empty-ammunition test now resets that prior hit response before asserting the independent fallback behavior. Game attack preconditions were not relaxed to make the test pass.

## Memory and cost

The existing 144 indexed sprites and 32 shadow masks remain **554,886 bytes**, including 77,542 shadow bytes. Palettes remain 147,456 bytes. No new textures or larger actor caches were introduced. Fixed caches: 24 × 112×104 world sprites (1,118,208 pixel bytes), 224×208 scratch (186,368), two 160×128 previews (163,840), ground cache (731,136), staged framebuffer (557,056).

The PRX has **4,054,188 BSS bytes**, 945,556 code/read-only bytes and 3,364 initialized-data bytes; total 5,003,108. BSS includes the caches, catalog and 410,656-byte game state; do not count those twice. Two VRAM framebuffers separately use 1,114,112 bytes. Heap remains 12 MiB. The loader briefly holds both raw and decoded saves. There is no runtime image decoder or per-outfit texture allocation.

Host microbenchmarks use 20 warm-up and 240 measured frames in five scenes. Stationary medians were 0.104–0.266 ms; scrolling medians 0.147–0.252 ms. CSVs include p95/worst values. These are host render timings, not PSP FPS or a controlled speedup comparison. The benchmark's Outpost label is an open-frontier camera location; constructed-outpost gameplay is tested separately.

## Reproduction

Configure `PSPDEV` and its `bin` directory on `PATH`; install Pillow/websockets and build PPSSPPHeadless separately:

```sh
bash scripts/validate_v051.sh /path/to/PPSSPPHeadless
python3 scripts/package_v051.py
```

The validation script stops on errors. Investigate a failure before resuming the affected stage; do not count retries as successes. Packaging requires all thirteen completed controller records from the exact executable. See the [36-area review](DESIGN_REVIEW_V051.md), [field guide](FIELD_GUIDE.md) and [balance table](../evidence/v051-clothing-balance.csv) for gameplay rules and remaining limitations.
