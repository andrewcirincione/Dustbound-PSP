# 0.4.0 validation record

The packaged EBOOT is the executable used by all four controller playthroughs. The packaging script refuses mismatched hashes. Screenshots come from PPSSPP VRAM at 480×272; the combat GIF contains 240 captured frames with simulation-based durations. No debug memory writes were used to play the game.

## Build identity

- EBOOT: 1,098,757 bytes.
- SHA-256: `e15a942ed491845938727d082d97a5ff54df146986d0dd0543a02a5fc20ae0ab`.
- PSPDEV pinned to the official `v20261101` toolchain in the read-only GitHub Actions workflow.
- PSP user-mode PRX, 333 MHz, 12 MiB heap; no expanded-memory flag.
- Physical PSP-3000 testing: **not performed**. Emulator playthroughs and memory budgets do not prove hardware frame rate, battery use or memory-stick reliability.

Independent [GitHub Actions run 41972937220](https://github.com/andrewcirincione/Dustbound-PSP/actions/runs/41972937220), source commit `68bc9ca`, passed and printed the identical EBOOT size and SHA-256 above. Its native checks and PSP compile ran on a separate GitHub runner.

## Checks

| Check | Result / scope |
|---|---|
| Baseline simulation | 61 core + 50 expansion assertions |
| Playability | 1,513 assertions: reach, role menus, recruitment, food sharing, ownership, loadouts, town spacing, hostile spawn separation, route preservation and old saves |
| Interior/save checks | 25 assertions plus 26,624 rigid-grip assertions over 13,312 weapon poses |
| Rendering | 272 screen/region/guide/interior cases preserve 480×272 bounds and 512-pixel stride |
| Gear catalog | All 2,016 items produce distinct render hashes at gameplay and portrait scales |
| Sanitizers | AddressSanitizer and UndefinedBehaviorSanitizer pass playability and renderer checks; LeakSanitizer is disabled because this environment cannot inspect process tasks |
| Save migration | Genuine shipped v0.1 fixture; genuine pre-change v0.3 fixture; v0.2 exterior-prefix compatibility; v0.4 roundtrips including interiors |
| Fresh-start EBOOT | Character creation, far-mining refusal, close mining, recruitment, trade, loot, gear comparison/equip, save/reload, local/world atlas and guide |
| Prepared EBOOT scenarios | Combat phases, wounds, forging, outpost, faction ledger and far regions |
| Interior EBOOT | Enter shop, buy, save/reload indoor position, leave, enter inn, pay once for rest, leave |
| Frontier EBOOT journey | Fresh starter travels Dusk Haven → Copper Cross; map inspection and town browsing retain the active route; hostile encounter occurs en route |

Prepared combat/outpost/region saves are separate from the two fresh-start runs. Native routing disables combat to isolate terrain traversal; the EBOOT journey leaves enemies active. All final run logs and assertions are in `evidence/v040-*`.

## Memory and rendering cost

108 indexed sprites occupy 434,662 bytes, with 77,542 bytes of shadow masks and 110,592 bytes of lighting palettes. Actor rendering uses a bounded 24-entry 96×80 pixel cache (737,280 bytes) and a 192×160 scratch surface (122,880 bytes). The ground cache is 731,136 bytes, and the staged main-RAM framebuffer is 557,056 bytes. These are included in the executable's 3,378,732-byte BSS; do not add them to BSS again. Code/read-only data is about 0.76 MiB. Two VRAM framebuffers use 1,114,112 bytes separately. The configured 12 MiB heap remains unchanged.

A native host run measured stationary median render times of 0.14–0.54 ms and scrolling medians of 0.15–0.49 ms across five scenes, with 20 warm-up frames and 240 measured frames per case. Scrolling advances 0.02 world units per frame. This is a host measurement, not a PSP speed estimate or controlled comparison with older releases. The CSV label “Outpost” names the open-frontier camera location; this microbenchmark does not construct an outpost. Actual outpost gameplay is covered by the EBOOT fixture run.

## Review limits

The [iteration record](ITERATION_LOG.md) documents defects found and corrected. The [mechanics audit](MECHANICS_AUDIT.md) distinguishes simplified loops from absent Kenshi systems. This is a playable alpha, not full Kenshi parity. Two-state animal animation, small equipment detail, single-room interiors and sparse authored wilderness content still need work.
