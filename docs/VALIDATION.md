# 0.3.0 validation record

The release EBOOT is 750,528 bytes. SHA-256: `72e48dd77dbda25dffa8811f1cdc3ef4dfe7ca2bdad99913900d1600d8f8fc7a`.

## Functional and visual checks

- 111 existing simulation assertions passed: 61 baseline and 50 expansion, including strike contact timing and single-hit resolution.
- 25 new interior/save checks passed: door eligibility, collision, trading, paid rest, wound recovery, continued bleeding, indoor save/load, v2-prefix migration, safe exterior return and dismissing a fallen indoor leader.
- 13,312 grip poses cover 32 weapon families, four phases, eight directions and 13 progress samples. Each pose checks rigid hand spacing and alignment, for 26,624 geometric assertions.
- 272 screen/biome/guide/interior renders preserve 480×272 bounds and the 512-pixel framebuffer stride.
- All 2,016 gear variants produce distinct gameplay and portrait renders.
- AddressSanitizer/UndefinedBehaviorSanitizer passed the renderer and new interior/grip checks. LeakSanitizer is disabled because process inspection is restricted in this environment.
- The exact release EBOOT passed the normal PPSSPP playthrough: creation, recruitment, trade, loot/equip, mining, manual save/reload, atlas and all nineteen guide chapters.
- The same EBOOT passed prepared combat/outpost scenarios: all four attack phases, limb health, construction, forging an item through PSP controls, factions, atlas, snow and pine regions.
- The same EBOOT passed a button-only interior playthrough: enter shop, walk around furniture, buy at the counter, save/reload the indoor position, leave, enter inn, pay for a bed, wake, exit and open the interior guide. The debugger only reads state and VRAM; it does not write gameplay state.
- Visual inspection checked anticipation, contact and recovery, native-resolution controls, interior draw order, sleeping placement and facade cutaways. A white matte in the first wilderness atlas was caught in native capture and removed before PSP testing.

## PSP budget and host benchmark

The packed artwork occupies 130,303 bytes of executable data, with a 24,576-byte lighting palette cache. The scrolling ground cache is 731,136 bytes; the cached main-RAM frame buffer is 557,056 bytes. The PSP ELF reports 2,430,892 bytes of BSS, including the fixed game state and rendering buffers, plus roughly 0.48 MiB of code/read-only data. The configured heap remains 12 MiB and the executable does not request the expanded-memory flag. Two 512×272×4 display buffers occupy 1,114,112 bytes of VRAM.

`tests/benchmark_render.c` compares v0.2.1 and v0.3.0 at the same seed and five camera locations. Both were compiled with native `cc -O2` and pinned to the same host CPU. Each case warms 20 frames and measures 240 frames. Scrolling advances the camera 0.02 world units per frame. No emulator ran during this comparison.

| Scene | Stationary before / after (median ms) | Scrolling before / after (median ms) |
|---|---:|---:|
| Dusk Haven | 0.448 / 0.259 | 0.393 / 0.229 |
| Shogun's Rest | 0.356 / 0.173 | 0.375 / 0.171 |
| Outpost | 0.269 / 0.092 | 0.272 / 0.096 |
| Snow | 0.306 / 0.109 | 0.299 / 0.114 |
| Pine | 0.315 / 0.139 | 0.310 / 0.137 |

Median native render cost fell about 42–66%. CSVs include p95 and worst samples; host scheduling affects them. These are CPU rendering measurements on a desktop host, not PSP-3000 frame rates. Hardware frame time, suspend/resume, battery, audio and long-campaign qualification remain outstanding.

## Capture and harness limits

The combat GIF uses 240 frames written from the PSP executable's display buffer. The test-only `DUST_RECORD` marker sets a deterministic 1/30-second simulation step; GIF timing follows simulation timestamps and duplicate frames may be merged. This is a prepared encounter, not earned progression. Normal gameplay uses the real clock with no frame recording. Neither QA marker is included in release packages.

An initial advanced harness run lost its debugger WebSocket while the title loop remained active; a subsequent run passed, and the final package passed all three playthroughs. The interior harness initially resumed before CPU startup and later stopped too close to an NPC instead of the inn door; startup waiting and precise button-driven door positioning resolved those harness issues.

`evidence/v030-*-checks.json` records the exact EBOOT checksum for each final playthrough. The build manifest, test transcripts, benchmark CSVs, screenshots and combat frames CSV provide the reproducible evidence. Source art and sprite packing/export scripts are included in the repository.
