# 0.2.1 validation record

The release EBOOT is 504,047 bytes. SHA-256: `36695824914f634c21d528e71f703d666e315eb71367a20372326d2e9e55b3d3`.

## Checks

- 111 simulation assertions: 61 baseline and 50 expansion. Four new regressions verify no early hit, resolution at the contact marker, no duplicate resolution in follow-through, and seamless pose joins across all 32 weapon families.
- 243 screen/biome/guide render cases, with 480×272 bounds and 512-pixel stride sentinels preserved.
- All 2,016 modular gear variants remain distinct at gameplay and portrait scale. New body proportions originally obscured the distinction between monk robes and ash cloaks; the cloak now has an asymmetric overlap and hem.
- AddressSanitizer/UndefinedBehaviorSanitizer passed the rendering and expanded simulation checks. LeakSanitizer was disabled due environment process-inspection restrictions.
- Normal EBOOT playthrough: character creation, recruitment, trade, equipping loot, mining, manual save and reload, map and all guide pages.
- Prepared v0.2 save scenarios: combat, limb health, outpost, forging through PSP controls, town, map, faction ledger and contrasting biomes. This also exercises the unchanged save layout.
- Visual inspection checked sampled anticipation/contact/recovery frames, the in-game forge portrait and the corrected squad-card boundaries.

## Capture method and limits

The combat GIF uses 240 frames read back inside the PSP executable, from the same framebuffer that is displayed. The test-only `DUST_RECORD` marker enables deterministic 1/30-second simulation ticks and framebuffer recording so disk I/O cannot stretch attack timing. GIF timing follows recorded simulation timestamps; the encoder merges consecutive identical frames. This is a prepared encounter, not earned progression. The initial advanced run reached the emulator's configured timeout near the final biome; it was rerun successfully with a longer harness timeout.

Normal play uses the real clock and records no frames. Neither `DUST_RECORD`, `DUST_QA`, nor scenario saves are included in the release. Headless PPSSPP is unthrottled: neither the GIF nor host throughput proves physical PSP performance. Hardware frame rate, suspend/resume, battery, audio and long campaign qualification are still outstanding. This remains procedural pixel art with more anatomical proportions, not a realistic 3D renderer.

See `evidence/v021-*-checks.json`, `v021-native-tests.txt`, `v021-sanitizers.txt`, `v021-combat-frames.csv` and `catalog-audit.json`. v0.2.0's historical evidence is retained in its tagged release.

The final launcher metadata was rebuilt after CI exposed a stale title. The PRX runtime is byte-identical to the full-playthrough build; the final EBOOT received another passing boot check and matches the independent GitHub build.
