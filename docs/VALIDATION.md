# 0.2.0 validation record

## Executable

- Official PSPDEV v20261101 toolchain, GCC 15.2; PSP user-mode PRX packaged in a valid EBOOT/PBP.
- EBOOT: **451,492 bytes**. SHA-256: `7667615f13efcf09f5bed8cb7ab8b6eeaea53550d323ab4b85188c3c76585e90`.
- ELF text 327,736 bytes; data 3,060; BSS 1,117,932. Configured heap: 12 MiB. Two 512-stride RGBA framebuffers use 1,114,112 bytes of VRAM. Large-memory SFO flag is disabled; the target is the original 32 MB PSP memory budget.
- This is a compiled PSP executable. Native screenshots are used for development inspection only; the `v02-*.png` gameplay images were read from the emulated EBOOT's displayed VRAM.

## Automated checks

- **107 simulation checks:** 61 baseline + 46 expansion checks. Includes economy, inventory, eight-member cap, equipment, combat phases and reach, limb consequences, skills, crafting, construction, theft/fencing, contracts, weather, custody, save checksums, v0.1 migration, invalid legacy rollback, fallen-party selection and town flags.
- **243 render cases:** all 25 screens across nine regions plus all 18 guide chapters. Verified clipping to 480 × 272 and preservation of the unused 32 pixels in every 512-pixel framebuffer row.
- **2,016/2,016 unique equipment renders** at both 1× gameplay and 5× portrait scale. This checks actual rendered pixels, not names or item IDs. Variants share modular silhouettes and change materials, trim and details; they are not 2,016 hand-authored assets.
- AddressSanitizer and UndefinedBehaviorSanitizer exercised simulation and render paths. LeakSanitizer is disabled because this execution environment prevents its process inspection; no leak-test claim is made.
- The baseline includes a 1,000-second simulated world soak with bounds/finite-state checks. This is not a full-length campaign balance test.

Evidence: `evidence/v02-native-tests.txt`, `v02-sanitizers.txt`, `v02-render-checks.txt`, `catalog-audit.json`, and the two catalog CSVs.

## Actual EBOOT playthroughs

Official PPSSPP 1.20.4, headless software renderer and debugger input/VRAM APIs. The headless harness runs without real-time pacing, so host throughput is **not** a PSP frame-rate measurement. The game requests two vblanks per displayed frame (30 Hz cadence); hardware performance has not been established.

1. Fresh normal expedition: name keyboard, ronin origin, creation preview, six-slot equipment, walking, Ivo recruitment (420 → 120 silver), a ration purchase (120 → 85), weapon market filter, looting/equipping leather, mining, manual save, loading the played save, preserved party/gear/money, world atlas and all 18 guide pages.
2. Prepared test saves loaded through the same player-facing save UI: three-member combat encounter, body health, constructed outpost, custom forge interface, Shogun's Rest, surveyed atlas, faction ledger, snow highlands and pine forest.

`v02-15-combat.gif` contains 120 frames read back **inside the PSP executable** through an optional QA recorder. All four animation phases were observed. The recorder copies the same framebuffer the executable presents; it does not substitute a native renderer or fabricate animation frames. `v02-combat-frames.csv` records simulation timestamps and phases. GIF playback timing uses those recorded timestamps. The prepared encounter and advanced world saves are development fixtures, not earned progression. They are excluded from the release.

The optional `DUST_QA` marker enables pointer telemetry, and `DUST_RECORD` enables a bounded framebuffer capture for testing. Neither marker is included in a playable package; normal play does not record frames. The released EBOOT is the tested binary.

## Limits and outstanding work

- No physical PSP, suspend/resume, battery, memory-stick failure or hardware audio qualification was possible here.
- Automated checks demonstrate specific rules and paths, not complete game balance or the absence of every bug.
- Local grid routing, simulated town events, shared market services and jobs are intentionally smaller systems than Kenshi's. There is no full slavery economy, interior simulation, independent multi-squad campaign or comprehensive Kenshi parity.
- Save migration was tested against a real v0.1 outpost save. Keep backups of valuable expeditions before upgrading.
