Current release validation: [0.9.0](VALIDATION_V090.md).

Historical atlas validation: [0.8.1](VALIDATION_V081.md).

Historical release: [0.8.0 build, simulation and PSP controller validation](VALIDATION_V080.md). The records below qualify the older 0.6.0 executable only.

# 0.6.0 validation record

The rebuilt executable is independently compiled in GitHub Actions and tested through real PSP controller inputs in PPSSPP 1.20.4. The release workflow rejects an EBOOT that differs from the locally qualified executable or the committed installer. scripts/package_v060.py refuses missing completion assertions or mismatched hashes across all seven controller suites.

## Executable and memory

- EBOOT: **1,419,050 bytes**.
- SHA-256: `52ef453b8af7e26611c9ca81cf063b15027e0781a12527aecf969d099631ef30`.
- Official PSPDEV v20261101, user-mode PRX, configured 333 MHz, 12 MiB heap, no expanded-memory requirement.
- Game state: **435,556 bytes**, with the original **410,656-byte v5 prefix** intact. Appended state adds 24,900 bytes. Writes save version 6; loads versions 1–6.
- Static memory/code sizes and native benchmark CSVs are included in the evidence archive. The native renderer's host timings do not measure MIPS/PSP hardware speed.
- Physical PSP-3000 testing: **not performed**. Frame pacing, battery life, suspend/resume and memory-stick endurance remain unqualified.

## Native checks

| Check | Final scope |
|---|---|
| Core / expansion | 61 core assertions and 50 expansion assertions |
| Rebuild / economy | 377,967 assertions, dominated by full-catalog buy/resell price combinations; route branches, single payments, material rollback, escort movement, trauma, robot repair, underlayers, groups, queues, captivity, projectile collision, room combat, room loot, witnessed trespass and named-death permanence |
| Long simulation | Seven simulated days with isolated threats and a mechanical survivor; checks bounded state, not a naturally played survival campaign |
| Saves | Genuine released v0.3, v0.4.1 and v0.5.0 fixtures; v5-prefix-to-v6 migration; v6 round trips, invalid floats/references and corrupt-file rejection |
| Playability | 1,555 checks covering reach, roles, town spacing, travel, inventory and migration |
| Interior / grip baseline | 26,649 assertions, including expected support-hand poses |
| Rendering | 350 screen/biome/guide/interior cases cover all 33 screen IDs and all 25 guide chapters; framebuffer bounds and 512-pixel stride checked |
| Catalog | All 2,112 material/grade variants have distinct native rendered hashes; these derive from 88 base designs |
| Painted weapons | 5,760 grip samples, 14,976 attack poses and 36 bounded comparisons |
| Direction / attachments | 288 asymmetric weapon reflections, 1,152 backpack fit cases, 72 hat fit cases |
| Garment balance | 1,056 clothing variants and 96 heavy/light tradeoffs |
| Backpack behavior | 192 actual swaps match predicted load; all 16 markets stock practical alternatives |
| Action feedback | Incapacity versus distance, interrupted actions, supplies preserved and successful repair after recovery |
| Critical interaction audit | 70 checks for ownership, navigation, rescue, jobs, skill effects and service reach |
| Sanitizers | Rebuild, render, critical audit and playability suites run with AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled |

## Final EBOOT controller suites

| Suite | Origin and exercised behavior |
|---|---|
| Copper Road | Fresh title/creation/ledger; declared fixtures 2–6 for wounded Sora, Rell, captivity, unfinished camp and prepared culvert. Actual physical escort, bypass repair/save/reload, paid truce, timed escape, construction completion/fuel/cooking, layer fitting, tactical orders and guides |
| Fresh expedition | Normal Ronin start: creation, distant mining refusal, close mining, recruitment, purchase, looting/equipping, save/reload, atlas scales and guide navigation |
| Combat / outpost | Declared combat/outpost/region fixtures: greatsword windup/contact/recovery, wounds, forging and distant terrain. 240 actual completed EBOOT frames recorded |
| Critical interactions | Declared rescue/logistics/loot fixtures: safe cancellation, restored help selection, carrying before bandaging, ownership-preserving bulk loot, comparisons, cook fetching water/grain, production/output storage, target cycling, bulk depot transfer, guides/logs and confirmed save-and-title |
| Interiors | Fresh expedition: shop entry/counter purchase, indoor position/save/reload, door exits, inn entry and paid sleep |
| Wilderness travel | Fresh expedition with enemies enabled: Dusk Haven to Copper Cross, atlas browsing without replacing the active route, arrival assertion |
| Natural poor-start expedition | Fresh Outlaw with 180 silver; defend at the wreck, collect three iron bars, escort Sora, treat bleeding, buy 150-silver relief cargo, repair the culvert, report to Nell, reject duplicate reward, save/reload route and payment. Final silver 480; no prepared saves or state writes |

The input harness sends controller commands and reads telemetry; it never writes live game state. Screenshots are the EBOOT's own completed 480×272 frame, copied only when a developer capture request is present. The combat video encodes those recorded frames. Prepared saves are test fixtures, not naturally earned progress. Installer packages exclude saves, QA markers, telemetry and capture requests.

An earlier pre-final emulator launch disconnected its debugger before the first gameplay frame; rerunning that unchanged candidate succeeded. It is not counted as a successful playthrough. The final records identify the hash above. The earlier 0.5.2 evidence and tests remain historical; they are not relabeled as tests of this build.

## Limits and remaining design gaps

Only Copper's leadership/trade consequences are authored to this depth. Civilian daily schedules, faction disguises, animal recruitment, a full captivity/slavery economy, destructible sieges and electrical networks remain absent. Other regions use simpler event and contract systems. Long natural campaigns, every combinatorial room/campaign state and physical PSP hardware have not been exhaustively verified. See DESIGN_REVIEW_V060.md and ROADMAP.md.

Reproduce with `PSPDEV=/path/to/pspdev PATH=/path/to/pspdev/bin:$PATH bash scripts/validate_v060.sh /path/to/PPSSPPHeadless`, then `python3 scripts/package_v060.py`.

## Session recovery verification — 2026-11-06

Before the gameplay feedback changes, a clean worktree at `cbe21aa` reproduced that earlier EBOOT (`ea0995ddc13e9c5def13d1e6a31dddd351012364fb8315d5bd7b0b047e5e1e7c`) byte for byte with the pinned PSPDEV toolchain. The complete native suite passed again. The Copper Road controller suite was also rerun against that fresh binary in an isolated test directory; physical escort, bypass save/reload, truce, timed escape, construction/fuel/cooking, layer fitting and group/order changes passed. Its new records are retained as `evidence/recovery-20261106-*`; the original six-suite qualification records were subsequently replaced by fresh tests of the final feedback fixes. Selected fresh world, equipment and guide captures were visually inspected.

The continuation corrects the README's obsolete v0.5.2 commands and unsupported limitations, makes the qualification script write and clear its own completion marker, and adds `verify_release_v060.py`. The release gate now reads the EBOOT inside the installer and rejects mismatched builds, incomplete controller records, changed evidence records and developer saves. Seven integrity regressions include deliberately corrupted/replaced payloads with recomputed outer ZIP checksums, an incomplete campaign, and a missing qualification marker. The final packages use fresh qualification of the feedback fixes, not `--from-qualified-evidence`. Physical PSP qualification and a natural long campaign remain outstanding.

## Fresh poor-start play and feedback fixes

The continuous Outlaw expedition completed on the final EBOOT in 216 simulated seconds. It retained actual wounds (left arm 62/100) and spent money on supplies; it is a short authored journey, not a multi-day balance claim. The controller uses normal PSP buttons, completed-frame captures and read-only HUD values. Blood and bleeding reads inform ordinary first-aid decisions. Enemies remain active.

Earlier attempts are not counted as passes: one ignored bleeding and collapsed on the road, others assumed a stunned interaction or travel request had succeeded. The harness now confirms actual cache contents and arrival, allows stuns to expire in the running world, treats bleeding, and verifies the whole earned outcome. The failed culvert attempt exposed the incorrect distance message; the final game distinguishes incapacity from reach and shows BLEEDING explicitly. A route interrupted by knockout can still produce the generic route-blocked message; that navigation feedback remains a follow-up issue.

All six existing controller suites were replayed against this same new EBOOT, followed by the successful continuous campaign. The native suite and four sanitizer suites also passed. Fresh world, bleeding HUD, dialogue, repair, payment and wound-panel captures were visually reviewed.
