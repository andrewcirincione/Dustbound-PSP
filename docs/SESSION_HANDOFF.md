# Current continuation: 0.11.0

The limb release supersedes the historical handoffs below. See `CONTENT_V110.md`, `VALIDATION_V110.md` and `FEEDBACK_V110.md`. `src/limbs.c` owns fitting, surgery, grades, stocks and persistence initialization. Actor size remains 932 bytes: low prosthetic-mask bits indicate presence, bits 8–19 hold grades (zero means legacy Standard), and bits 20–31 hold stolen ownership. Treat the mask as unsigned for shifts. Game size is 627,508 bytes; saves are v11 with actual v10 migration coverage. New catalog entries are prostheses and severed organic limbs, not wearable gear; gear-only loops end at V100_ITEMS. Run validate_v080, 081, 090, 100 and 110, then emulator_v110 on the final EBOOT. Published binary identity is in evidence/v110-build-manifest.json. Physical PSP qualification and naturally earned long-run economy balance remain open.

---

# Current continuation: 0.10.0

The 0.10.0 release supersedes the historical handoff below. Current source adds five species, 18 persistent named owners/36 personal items, 96 armor variants, six settlement cultures, 12 constructions across all 38 weapon types, and explicit crafting requirements. Save v10 is 626,860 bytes; all prior readers remain. `docs/CONTENT_V100.md`, `DESIGN_REVIEW_V100.md`, `VALIDATION_V100.md` and `FEEDBACK_V100.md` are the current references. The qualified EBOOT digest is in `evidence/v100-build-manifest.json` and matches the completed 82-frame PSP controller run. Run 080, 081, 090 and 100 native suites and `scripts/emulator_v100.py` after relevant future changes. Do not describe prepared review saves or native benchmarks as a natural campaign or physical PSP qualification. Next design priorities: political succession, interdependent event mechanics, resident behavior, extended balance and stronger species anatomy.

---

# Kenshi working session 1 — recovered project context

Recovered on 2026-11-06. This is the continuation record for **Dustbound**, the original PSP frontier survival RPG in [andrewcirincione/Dustbound-PSP](https://github.com/andrewcirincione/Dustbound-PSP). The conversation's project label is Kenshi; the game is not a port and uses original code and artwork.

## Recovered state and provenance

The earlier conversation, repository history, original local checkout, reconstructed checkout, generated art sources, PSPDEV toolchain, PPSSPP build, test scripts, fixtures, qualification records and final release archives were recovered. Conversation retrieval supplied the important decisions and final exchanges, not a complete verbatim transcript.

An earlier unpublished 0.6.0 candidate had been lost before reconstruction. Its missing bytes have not been recovered. The playable reconstruction starts from released 0.5.2 and is independently verifiable:

| Checkpoint | Meaning |
|---|---|
| `beaf03c` | Released 0.5.2 foundation |
| `d770f2d`, `bb618da` | Recovery scope and successful direct GitHub publishing |
| `d7b4d81` | Reconstructed Copper Road and survival systems |
| `63a6735` | Named-death permanence, room/projectile rules and controller qualification |
| `cbe21aa` | Final tested executable, captures, archives and publication workflow |

The previous session stopped before merging `frontier-depth-0.6.0` and publishing its release. GitHub run [42261185711](https://github.com/andrewcirincione/Dustbound-PSP/actions/runs/42261185711) passed native tests, PSP compilation and exact-binary verification; publication was skipped because the commit was on the development branch. The continuation corrects stale README instructions, makes qualification write its own completion record, checks the executable inside the installer, and preserves this handoff.

The initial recovered EBOOT was `ea0995ddc13e9c5def13d1e6a31dddd351012364fb8315d5bd7b0b047e5e1e7c` (1,418,194 bytes); a clean recovery build reproduced it. A fresh poor-start expedition then exposed misleading action refusal feedback. The continuation fixes incapacity messages and adds an explicit BLEEDING HUD label. The final EBOOT is `155e31c905efb923026207bd0ea53e8448ea8de5258262d6920cb5c18b8a4650` (1,420,178 bytes). All seven controller suites, the native suite and four sanitizer suites passed for this new binary; the earlier recovery records remain historical.

## User decisions to preserve

- Build a gritty, realistic western/samurai survival game for PSP, with PSP-3000 as the user's hardware target. Maintain original painted assets, consistent isometric projection, appropriate scale, fitted clothing/headwear/backpacks and correct weapon grips in both directions.
- Review from the perspective of someone who loves Kenshi. Iterate on actual play, all UI and interactions, meaningful survival decisions, distinct people/factions, and consequences. Do not mistake a large equipment catalog for depth.
- Preserve the fixes for oversized/lifted hats, reversed/low backpacks, reversed swords, short interaction reach, specialist services, separated towns, navigational atlas, multiple useful backpacks and honest gear comparisons.
- Continue implementation, compile, playtest the actual EBOOT, inspect its screenshots, and publish to GitHub. Prior session explicitly authorized pushing/publishing and addressing the full critique.
- Use the repaired direct GitHub connection. The user approved extending the existing connection used for Rift Ascent to this repository; a new account setup is unnecessary.
- Distinguish native tests, controller-driven emulator play, prepared saves and real hardware testing. Physical PSP testing has not been performed. Never label a prepared scenario a naturally earned campaign.

## Architecture and invariants

| Files | Responsibilities |
|---|---|
| `src/game.h`, `src/game.c` | Fixed state, simulation, combat, inventory, inputs, saves/migrations and telemetry |
| `src/content.c`, `src/world.c` | Item definitions, terrain, towns, routing, services, contracts, workers and economy |
| `src/expansion.c`, `src/playability.c`, `src/interior.c` | Character creation/menus, roles, mercenaries, comparisons, rescue and indoor services |
| `src/depth_types.h`, `src/depth.h`, `src/depth.c` | Copper Road, named actors, consequences, captivity, trauma, underlayers, construction/fuel, groups and priorities |
| `src/depth_combat.c` | Actual room occupants/combat, witnesses, tactical orders and moving projectiles |
| `src/render.c`, `src/depth_render.h`, `src/painted_actor.h`, `src/combat_pose.h` | 480×272 UI/world rendering, fitted articulated clothing and weapon motion |
| `assets/art/`, `scripts/pack_art.py`, `src/art_data.h` | Original source atlases, repeatable packing and compiled indexed art |
| `src/psp_main.c`, `src/native_main.c`, `src/audio.c` | PSP runtime, native harness and sound effects |
| `tests/`, `scripts/emulator*_v060.py` | Native regressions, migration fixtures and controller-only emulator scenarios |

Fixed bounds: 192 actors, 320 buildings, 640 nodes, eight survivors, 24 inventory slots, six worn slots, 24 projectiles, three expedition groups and four job priorities. The 2,112 modular gear variants derive from 88 base designs, including 36 weapon families; they are not 2,112 unique drawings.

Save v6 appends `FrontierDepth` to the stable 410,656-byte v5 prefix; total `Game` is 435,556 bytes. Retain versions 1–5 readers and item IDs. Preserve ownership tags and transactional rollback during inventory/crafting. Never recycle named actors, contracted NPCs or carried bodies. Preserve active travel when browsing the atlas. Menus pause simulation.

## Implemented 0.6.0 scope

Copper Road has eight landmarks and named roles: wounded Sora, Nell at Copper stores, Rell, two captives, two scouts and a convoy. Actual escort arrival unlocks the culvert repair; repair, toll truce, living capture and lethal resolution create distinct persistent trade/allegiance/retaliation outcomes. Named deaths remain permanent.

The rebuild also adds physical capture, timed shackle work, rescue/recruitment and temporary escape recognition; trauma ceilings and mechanical repair; light underlayers with cut/blunt coverage; construction labor, fuel and condition; ordered jobs and separate group leaders/destinations; moving projectiles with obstructions/interception; same-room combat, real occupants, witnessed trespass and trade hours. It retains the preceding equipment, UI, logistics, inventory and save fixes.

## Qualification and release

Use official PSPDEV **v20261101**, pinned and checksum-verified in `.github/workflows/build.yml`, and PPSSPP **1.20.4**. Do not infer hardware frame times from native benchmarks.

```sh
bash scripts/build_native.sh
export PSPDEV=/path/to/pspdev
export PATH="$PSPDEV/bin:$PATH"
bash scripts/validate_v060.sh /path/to/PPSSPPHeadless
python3 scripts/package_v060.py
python3 scripts/verify_release_v060.py
python3 tests/test_release_v060.py
```

The seven controller suites cover Copper Road, a fresh expedition, prepared combat/outpost, critical interactions/logistics, interiors, a fresh wilderness journey with enemies, and a continuous Outlaw rescue/salvage/repair/report expedition. All records identify the EBOOT hash. The seven-day native test isolates threats and does not establish natural campaign balance. See `VALIDATION.md` for precise coverage and interrupted-run disclosures.

For documentation-only updates, `package_v060.py --from-qualified-evidence` reuses the committed evidence only after checking the exact build, installer, manifests, controller completion records and recording. It does not claim fresh tests. The normal qualification command records completion itself and removes any stale completion marker before starting.

Publish the installer, standalone EBOOT, evidence ZIP and checksum file. The workflow independently compiles on `main` before creating the prerelease, and leaves an existing release unchanged. Never upload saves, telemetry or QA markers in the installer.

## Remaining accepted work

The user accepted the wider critique; the items below remain work, even though 0.6.0 makes substantial progress. The release must not be described as completing every Kenshi system.

1. More distinctive wilderness conflicts and rewards beyond Copper Road; varied motives, warnings, extortion and avoidance rather than repeated encounters.
2. Settlement work/rest schedules, richer relationships and personal dialogue, faction disguises and more nuanced witness/local-law behavior.
3. A broader captivity economy, forced labor, liberation networks and multi-region pursuit beyond the bounded chain-yard loop.
4. Destructible defenses/sieges, power networks, deeper production/research chains, and recruited pack animals.
5. Stronger enemy tactics, richer motion/creatures and further equipment decision depth rather than catalog inflation.
6. Long naturally played poor-start campaigns to judge money/food progression, wound recovery, difficulty, travel pacing and eventual abundance.
7. Physical PSP-3000 frame pacing, busy-town memory, controller feel, suspend/resume, battery and memory-stick endurance.

The continuation completed a natural Outlaw Copper Road expedition (180 starting silver, 150 spent on relief supplies, 480 final silver, persistent wounds, earned bypass/payment, save/reload) and addresses the misleading refusal and bleeding feedback it exposed. Next implementation priority after release: broaden this into a longer naturally played campaign, then build a second regional conflict with visibly different choices and consequences. One short expedition does not qualify long-term survival balance. Use the current review/roadmap to avoid reintroducing already fixed problems. Historical reviews describe their own versions; their old “next milestone” lists are superseded by `DESIGN_REVIEW_V060.md` and `ROADMAP.md`.

Navigation follow-up resolved: unconsciousness, being carried, custody, stun and attack/hitstop frames now hold travel rather than count as terrain obstruction. A persistent ROUTE HELD label explains why. Native regressions cover six causes lasting over eight seconds, recovery to arrival, true obstruction, death and explicit cancellation. The Copper Road controller suite adds a declared blood-loss fixture that starts travel while conscious, collapses in simulation and retains the same destination beyond the old timeout. This is prepared acceptance evidence, not natural campaign progress.

The continuation also fixes Sora's first aid becoming unavailable after the first dressing. Her existing dialogue now offers repeat treatment during the escort, spending a real dressing and retaining the physical-arrival requirement. Poor-start acceptance uses follow-up treatment and carries Sora on the escort leg. Carrying uses normal encumbrance, can be canceled by setting her down, survives saving and releases her at actual arrival. Failed escort attempts are retained and disclosed; reaching Copper without her never counted as success. Persistent companion status/distance feedback remains follow-up work.
