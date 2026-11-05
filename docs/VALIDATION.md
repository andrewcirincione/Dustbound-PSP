# 0.5.0 validation record

All five final controller playthroughs identify the same packaged EBOOT. `scripts/package_v050.py` refuses mismatched hashes or missing scenario completion assertions. Gameplay images are direct 480×272 PPSSPP VRAM captures; the greatsword combat GIF contains 240 captured frames with simulation-based durations. Inputs are PSP buttons; telemetry is read-only. Prepared saves and QA markers are excluded from the installer.

## Build identity

- EBOOT: **1,294,642 bytes**.
- SHA-256: `2fd6fe5c179c49d628fa76557aec011399a003d73b04b240d1ab152ed0eb9b3d`.
- Official PSPDEV `v20261101`, PSP user-mode PRX, 333 MHz, 12 MiB heap, no expanded-memory flag.
- Writes save format 5; loads v1–v5. Frozen older layouts and genuine v0.3/v0.4.1 saves are regression fixtures.
- Physical PSP-3000 testing: **not performed**. Emulator results do not establish hardware frame rate, battery life or memory-stick reliability.

## Native checks

| Check | Result / scope |
|---|---|
| Core simulation | 61 core and 50 expansion assertions |
| Playability | 1,541 assertions for interaction reach, roles, recruitment, logistics, routes, town spacing and migration |
| Interior/save/grip baseline | 26,649 assertions, including 13,312 independently expected support-hand poses |
| Rendering | 272 screen, region, guide and interior cases preserve framebuffer bounds and stride |
| Gear catalog | All 2,112 modular variants have distinct hashes at gameplay and portrait scales |
| Painted grips | 2,880 samples across all 36 weapon families; each source anchor touches opaque handle art |
| Painted attack bounds | 14,976 poses fit the fixed actor cache; observed 2× bounds x −111…110, y −122…49 |
| Comparison previews | All 36 equipped/candidate pairs fit their cells and remain pixel-stable when cached |
| Frontier systems | Four guards, four new weapon families, strength/arm penalties, service separation, regional stock/filter ordering, hire reach/payment, duplicate-charge prevention, following, nearby first aid, expiry/dismissal, save roundtrip and genuine v4 migration |
| Sanitizers | Frontier and actual weapon rasterizer/pose/preview suites pass AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled in this environment |

## Final PSP controller playthroughs

| Scenario | Evidence |
|---|---|
| Fresh start | Creation, far-mining refusal, close mining, recruitment, buying, loot/equip, save/reload, both atlas scales and guide navigation |
| Prepared greatsword encounter | Windup/contact/recovery, wounds, forging, constructed outpost, faction ledger and distant regions; 240-frame combat capture |
| Interiors | Shop entry, counter trade, indoor save/reload, exit, inn entry, paid sleep and exit |
| Fresh wilderness journey | Dusk Haven → Copper Cross with enemies active; atlas browsing preserves the route; arrival asserted |
| Frontier identity | Fresh guard hire, duplicate payment blocked, unchanged squad count, escort/save/reload; prepared Shogunate/Rust/outlaw markets; four new weapons equipped; genuine v0.4.1 save loaded; combat/faction guides |

Prepared fixtures are not naturally earned expeditions. Native tests cover contract expiry and other combinations that the short controller run does not wait a full simulated day to exercise. All 36 families receive native pose coverage; the four added families receive new EBOOT equipment coverage, and the greatsword receives an EBOOT combat recording. Historical v0.4.1 captures remain separate.

Two intermediate attempts failed: one debugger request timed out; another reported `CPU not started`. Logs are retained as `v050-debugger-timeout.txt` and `v050-debugger-cpu-not-started.txt`. Fresh processes subsequently completed the affected scenarios without game-code changes for those failures. The intermittent debugger/emulator failure was not conclusively diagnosed; reruns are not a long-session stability guarantee. Final matching hashes and completion assertions determine packaging eligibility.

## Memory and rendering cost

144 indexed sprites and 32 shadow masks occupy **554,886 bytes**, including 77,542 shadow bytes. Lighting palettes use 147,456 bytes. Relative to 0.4.1, new sprites add 11,487 compiled bytes and 4,096 palette bytes. Fixed caches are unchanged: 24 × 112×104 world sprites (1,118,208 pixel bytes), 224×208 scratch surface (186,368), two 160×128 preview buffers (163,840), ground cache (731,136), and staged RAM framebuffer (557,056).

The PRX has **4,054,188 bytes BSS**, 923,668 code/read-only bytes and 3,356 initialized-data bytes, total 4,981,212. BSS includes the caches, catalog and game state; do not add those again. `Game` is 410,656 bytes. Two VRAM framebuffers separately use 1,114,112 bytes. Heap remains 12 MiB. The loader briefly holds both raw and decoded saves. No runtime image decoder or per-outfit texture allocation was added.

Final host microbenchmarks used 20 warm-up and 240 measured frames in five stationary/scrolled scenes. Stationary medians were 0.106–0.274 ms; scrolling medians 0.107–0.270 ms. CSVs retain p95 and worst cases. These are host render timings, not PSP FPS or a controlled speedup claim. The “Outpost” benchmark label is an open-frontier camera location; the constructed-outpost test is separate.

## Reproduction and limits

Configure `PSPDEV` and its `bin` directory on `PATH`; install Pillow/websockets and build PPSSPPHeadless separately:

```sh
bash scripts/validate_v050.sh /path/to/PPSSPPHeadless
python3 scripts/package_v050.py
```

The script cleans/rebuilds the PSP binary, generates QA fixtures, runs five controller scenarios, sanitizers and benchmarks. It fails on any error and does not hide retries. Investigate failures before rerunning a scenario; packaging requires completed matching-hash records.

The [design review](DESIGN_REVIEW_V050.md) and [iteration log](ITERATION_LOG.md) explain the corrected grip, service overlap, stock ordering, validation order and guide overflow. Full Kenshi parity, long-campaign qualification and physical PSP performance remain unclaimed.
