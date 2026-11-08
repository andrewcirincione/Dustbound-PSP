# 0.13.0 validation

Qualified EBOOT SHA-256: `6d69d9a3d9353aaf428b93c0e629cc9d14e0f9b16e056cc9d00dcbf0ededc7b2` (1,754,838 bytes). PSPDEV v20261101 is checksum-pinned; PPSSPP 1.20.4 uses its software renderer. Publication independently rebuilds and rejects a binary that differs from this digest.

## Native checks

Release suites 080, 081, 090, 100, 110, 120 and 130 passed locally. Logs are in `evidence/v130-native-checks.txt`. Earlier gates retain core/expansion, atlas, society/rescue, species/civilization, equipment/crafting, all five limb grades, random limb rarity, streaming, finite camp actions and migration coverage. The 80,000 generation trials retain the same wearer distribution and five Relic parts in the strongest tier.

The new AddressSanitizer/UndefinedBehaviorSanitizer suite verifies:

- A single casualty leaves moving survivors and a fixed corpse; returning to a separated corpse rematerializes the group.
- Actual medical consumption, carry references across unload/save/load/rematerialization and putting down a recovered patient. Invalid carrier chains/cycles, missing mission towns and non-finite state are rejected.
- Hunter pursuit and finite carcass transfers, including live actors approaching bodies and returning when gathering finishes; hidden ambush detection/trigger/flanking; refugees arriving as recruitable individuals without automatic squad membership.
- Depleted camps packing, exact stock withdrawal, saved supply state and re-establishment. Armies select a duty, fight actual garrisons without awarding player money/kills, and transfer real ration cargo once across a save/load.
- Succession, one-time silver/relief expenditure, persistent allegiance resolution, retained dead rulers and specialized prosthetic stats.
- Actual released v12 migration and v13 round-trip. Native camp/council/activity frames keep framebuffer guard bytes intact.

Two distinct endurance checks ran on seed 73819:

| Run | Scope | Result |
|---|---|---|
| Full simulation | 12,000 full `simulate(.1)` ticks; 20 simulated minutes | 286 living members, 21 unconscious, none below 20 hunger, 11 settled groups. Player silver 650→650; kills 0. State validation/save/reload pass. |
| Coarse world endurance | 18,000 one-second population/politics/society/civil updates; five simulated hours / ten game days with town restocking | 5 rescues, 5 hunts, 11 resupplies, 11 relocations, 28 refugee arrivals, 2 army duties and 1,058 recorded group conflicts. Repeated validation and final save/reload pass. |

Conflict counts represent contacts recorded by the coarse simulation, not unique campaign battles. The coarse run does not execute every live actor's combat loop and is not equivalent to five hours of player gameplay. Host CPU timings are recorded in the logs, not presented as PSP performance.

Game state is 2,143,228 bytes, including 15,524 new bytes after the untouched v12 prefix. Actor remains 932 bytes. PSP ELF text/data/BSS are 1,292,476/3,516/6,544,628 bytes. The existing 12 MiB heap and 32 MiB memory mode remain.

The historical omnibus `build_native.sh` is not this release gate; its obsolete five-option dialogue assertion remains documented. No omnibus pass is claimed.

## Final PSP executable

`scripts/emulator_v130.py` uses ordinary emulated controller buttons/analog input and prepared ordinary save files. QA exports state and completed framebuffers; it never changes gameplay state. The final run captured **31 frames at 480×272**, all visually reviewed.

- Normal character creation, walking through six actual waypoints out of Dusk Haven, a naturally occurring hostile encounter, and saving/reloading that journey. This brief journey ran 86.6 simulated seconds; it was not a full campaign.
- Five camp arrangements, physical bed use, increasing health and decreasing fatigue.
- NPC medicine/carry activity with live carry references retained through save/load.
- Ambushers hidden before approach and visible/attacking after triggering.
- Council entry through Nearby actions; silver and rations deducted once despite repeated attempts; pledge/relief persisted after reload; a near-resolution ordinary save advancing into a new government.
- Activity ledger scrolling; Reinforced/Precision/Relic arm and leg comparisons; actual fitting deductions and saved specialized limbs.

Visual review cleared decorative vegetation from camp footprints, used bedrolls for improvised camps, checked the council and activity text, and made tradeoff descriptions specific to arms versus legs. A test-route correction navigated around a building instead of counting blocked analog input as travel. No teleport or runtime-state modification was used for the fresh journey.

## Practical limits

Physical PSP frame pacing, suspend/resume and memory-stick endurance are untested. The emulator was accelerated and instrumented, so its host frame rate is not a hardware benchmark. A naturally played full campaign remains open. Off-screen battles are deliberately coarse, local navigation can still limit routes, and army objectives are heuristics rather than a full supply-chain/strategic simulation. Camps still reuse the existing prop atlas. These are the next review targets, not claims that all long-term balance concerns are resolved.
