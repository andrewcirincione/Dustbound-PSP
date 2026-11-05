# 0.4.1 validation record

The packaged EBOOT is the executable used by all five controller playthroughs. `scripts/package_v041.py` refuses mismatched hashes. Gameplay screenshots are captured from PPSSPP VRAM at 480×272; the combat GIF contains 240 captured frames with simulation-based durations. Test scripts send PSP buttons and read telemetry; they do not write game memory.

## Build identity

- EBOOT: 1,267,722 bytes.
- SHA-256: `68e2abba366b71c4941a43001325da8fc259764756b6ca2545d455242f965d14`.
- PSPDEV pinned to official `v20261101`; PSP user-mode PRX, 333 MHz, 12 MiB heap, no expanded-memory flag.
- Save format remains v4. Existing v0.4.0 saves require no migration.
- Physical PSP-3000 testing: **not performed**. Emulator checks and memory budgets do not establish hardware frame rate, battery use or memory-stick reliability.

## Checks

| Check | Result / scope |
|---|---|
| Baseline simulation | 61 core + 50 expansion assertions |
| Playability | 1,513 assertions covering reach, NPC roles, recruitment, logistics, ownership, town spacing, hostile spawn separation, routes and old saves |
| Interior/save/rigid grip | 26,649 assertions, including 13,312 poses with independent expected support-hand offsets |
| Rendering | 272 screen/region/guide/interior cases preserve the framebuffer bounds and stride |
| Gear catalog | All 2,016 items have distinct render hashes at gameplay and portrait scales |
| New painted grips | All 32 authored pivots touch opaque handle pixels; 2,560 rasterized samples across rotations and scales |
| New attack bounds | 13,312 actual painted attack poses fit the fixed actor cache, including long/downward blades and poles |
| New comparison bounds | All 32 equipped/try-on pairs fit their cells; repeated cached renders are pixel-identical |
| Sanitizers | Final weapon renderer/pose/preview suite passes AddressSanitizer and UndefinedBehaviorSanitizer; leak inspection disabled in this environment |
| Fresh-start EBOOT | Creation, far-mining refusal, close mining, recruitment, trade, loot/equip, save/reload, atlas and guide |
| Prepared EBOOT encounter | Windup/contact/recovery, limb wounds, forging, outpost, faction ledger and distant regions |
| Interior EBOOT | Enter shop, buy, save/reload indoor position, leave, enter inn, pay once for rest, leave |
| Frontier EBOOT | Fresh starter travels Dusk Haven → Copper Cross with enemies active; map browsing preserves the route |
| New art EBOOT | All 32 weapon families selected and equipped through controller input; 64 world and inventory captures |

The prepared encounter and weapon catalog saves are QA fixtures, separate from the fresh-start runs and excluded from the installer. The attack-bounds sweep covers a larger pose matrix than the emulator combat recording. The exact executable identity is recorded at the start of every playthrough JSON. Logs, assertions and captures are in `evidence/v041-*`.

To reproduce the additional catalog playthrough after the native/PSP builds:

```sh
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_art_fixture.c -lm -o build/make-art-fixture
mkdir -p build/art-fixtures
build/make-art-fixture build/art-fixtures
python3 scripts/emulator_art_v041.py /path/to/PPSSPPHeadless
```

## Memory and rendering cost

140 indexed sprites plus 32 shadow masks occupy 543,399 bytes (the 77,542 shadow bytes are included). Lighting palettes occupy 143,360 bytes. The 24-entry 112×104 world-actor cache uses 1,118,208 pixel bytes; its 224×208 scratch surface uses 186,368 bytes. Two 160×128 fitted portrait buffers add 163,840 pixel bytes. The ground cache is 731,136 bytes and the staged main-RAM framebuffer is 557,056 bytes.

These mutable buffers, metadata and game state are already included in the PRX's **4,020,588-byte BSS**. Code/read-only data is 899,976 bytes and initialized data 3,340 bytes; their total with BSS is 4,923,904 bytes. Two VRAM framebuffers separately use 1,114,112 bytes. The configured 12 MiB heap is unchanged. No runtime image decoder or per-outfit texture allocation was added.

Host microbenchmarks compiled both tagged v0.4.0 and current sources with the same compiler/options, with 20 warm-up and 240 measured frames for each scene. Stationary v0.4.1 median render times were 0.106–0.293 ms; scrolling medians were 0.108–0.301 ms. The v0.4.0 medians were 0.128–0.461 ms and 0.124–0.425 ms respectively. Both runs use identical five-scene inputs; retained CSVs include p95 and worst cases. These single host runs show no measured median regression, not a precise speedup claim or a PSP frame-rate estimate. The “Outpost” benchmark label names an open-frontier camera location; actual constructed outpost gameplay is covered separately by the EBOOT fixture.

## Visual review and limits

The [iteration record](ITERATION_LOG.md) records the original weapon mismatch, longbow pivot error, attack-cache clipping, inventory-preview overflow, obsolete launcher art and flat interior threshold, along with their corrections. The first clipped polearm screenshot is retained with an explicit `iteration1` name. Final native grids are renderer reviews; numbered and `art-compare`/`art-world` screenshots come from EBOOT gameplay. The launcher illustration is promotional art.

This remains a playable alpha. The [mechanics audit](MECHANICS_AUDIT.md) distinguishes simplified and absent systems. Small equipment detail, limited beast animation, sparse authored wilderness and single-room interiors remain limitations. No full Kenshi parity or physical-hardware validation is claimed.
