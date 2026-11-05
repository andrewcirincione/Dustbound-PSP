# Dustbound — PSP frontier survival RPG

An original isometric western–samurai squad RPG for PSP homebrew. Build a band of drifters, survive wounds and hunger, trade, steal, forge equipment and establish an outpost across a changing frontier. Inspired by Kenshi's systemic survival; this is a separate game using original code and original generated / procedural art, not a Kenshi port.

**Current milestone: 0.5.0, playable alpha.** A much broader sandbox than 0.1, with working versions of the requested core loops. It does not reproduce every Kenshi system or its scale. Physical PSP qualification remains outstanding.

[Download releases](https://github.com/andrewcirincione/Dustbound-PSP/releases) · [Field guide](docs/FIELD_GUIDE.md) · [Validation](docs/VALIDATION.md) · [Progress](docs/ROADMAP.md)

![Actual PSP EBOOT gameplay in PPSSPP](evidence/v050-03-dusk-haven.png)

## Heavy steel and recognizable people

0.5.0 adds painted greatswords, railway mauls, crescent glaives and anchor cleavers. All 36 weapon families use weathered steel, leather and wood artwork. Strength and arm health affect heavy-weapon windup/recovery; the comparison shows the current handling penalty.

Profession and faction now shape NPC outfits, services, advice and behavior. Merchants and medics flee danger; guards fight and return to their posts. Medics have a separate approach beside town recovery areas. Regional weapon stock appears first in the weapon filter, giving different towns practical shopping specialties.

Contract guards offer one simulated day of escort (240 silver) or outpost watch (180). They fight and provide nearby first aid independently, keep their own gear, wait outside interiors and return home when the contract ends. Contracts save their remaining time and do not use a permanent squad slot.

The [whole-game design review](docs/DESIGN_REVIEW_V050.md) evaluates 34 areas, records what changed, and prioritizes what still falls short. Leader-driven world consequences, richer wilderness encounters, independent squads and deeper outpost logistics remain major gaps.

![Greatsword comparison from the PSP executable](evidence/v050-58-new-weapon-0.png)
![Hired guard dialogue from the PSP executable](evidence/v050-51-guard-contract.png)

## A more readable frontier

0.4.0 adds a persistent target/action prompt, visible interaction reach, specialist NPC services, and equipment comparisons with WORN / TRY ON portraits, stat deltas and effects. Mining and lockpicking require staying within 1.15 world meters; automated miners walk into reach. Town spacing is four times greater while streets stay compact. The atlas has local and world views, player location, known towns, destination distance and travel orders.

The renderer now compiles 144 original painted sprites and 32 projected shadow masks. Calibrated building foundations use the same 2:1 projection as the terrain. Articulated painted torsos, limbs, headwear, boots and packs replace flat character shapes; weapon hands share a rigid grip throughout a strike. Textured indoor floors and walls match the exterior materials. PSP controller pictograms identify every input.

Walk through a door into a house, shop, inn, tower, smithy, workshop or depot. Counters trade, beds restore wounds, and workstations open forging, research or storage. Saves preserve the room and indoor position. Nineteen guide chapters explain the rules.

![Equipment comparison captured from the PSP executable](evidence/v050-33-gear-comparison.png)
![Walkable interior captured from the PSP executable](evidence/v050-27-shop-interior.png)

## What you can play

- A persistent 1536 × 1536 world, sixteen settlements, nine terrain regions, rivers/road fords, ruins, resource deposits, day/night, storms and environmental exposure.
- Up to eight controllable survivors; twenty skills improve through use. Followers, holding, mining, farming, medicine, smelting, cooking and depot hauling jobs.
- Character naming, five starts, three original ancestries, body build, eight skin tones, hairstyles, hair colors, beards and scars. Town barbers edit appearance.
- **2,112 generated gear variants:** 88 designs × 6 materials × 4 grades, plus base supplies and legacy equipment. Six visible slots. These are modular procedural variants, not 2,112 hand-drawn assets. Automated image hashes distinguish every variant at gameplay and portrait scale.
- Melee, polearms, bows, crossbows and frontier firearms. Windup/contact/recovery animations, directional swings, stagger, recoil, hit stop, procedural sound effects. Weapon reach, weight, skill and injured arms affect attacks.
- Six separate limb health values, armor coverage, bleeding, blood loss, knockout, permanent death, limb loss, prosthetics, first aid, carrying, paid doctors and sleep.
- NPC speech and service dialogue, finite market stocks, rotating daily gear, reputation/prosperity prices, recruitment, looting, theft, fencing, lockpicking, bounty and custody/bail/escape.
- Delivery, supply, hunt and relic contracts; persistent journal, faction ledger, town prosperity/ownership events and expedition legacy goals.
- Construction, farming/irrigation, storage, eight supply recipes, custom equipment forging and three research tiers.
- Six manual save slots plus autosave, checksums, backup files and v0.1 through v0.4.1 save migration. Nineteen contextual in-game guide chapters.

![Combat captured from the EBOOT; prepared encounter](evidence/v050-15-combat.gif)

## Install on PSP

1. Use a PSP already configured to run homebrew. This repository does not install or modify firmware.
2. Download `Dustbound-0.5.0-PSP.zip` from the release and extract its `PSP` folder to the memory stick root.
3. The executable should be at `PSP/GAME/DUSTBOUND/EBOOT.PBP`. Launch it from **Game → Memory Stick**.
4. Back up existing `DUST_*.sav` files before upgrading. v0.1 through v0.4.1 saves are migrated when loaded; save the migrated game to a different slot first. Version 5 saves cannot be loaded by older executables. Migration refreshes regional markets, town NPC outfits and medic positions once. Do not remove the memory stick during a save.

The single EBOOT includes its art, catalog and sounds. No network access or external assets are needed. Release packages exclude developer test saves and `DUST_QA` telemetry markers.

## Controls

| PSP input | Action |
|---|---|
| Analog / ![D-pad](assets/ui/dpad.svg) | Move in screen direction; navigate menus |
| ![Cross](assets/ui/cross.svg) | Interact, confirm, equip/use, buy/sell |
| ![Triangle](assets/ui/triangle.svg) | Pack; press again for worn equipment |
| ![Square](assets/ui/square.svg) | Fight/passive stance; transfer an item in pack |
| ![Circle](assets/ui/circle.svg) | Sneak; close an overlay |
| ![L](assets/ui/l.svg) | Next squad member; previous inventory tab |
| ![R](assets/ui/r.svg) | Tactical pause; next inventory tab |
| ![Select](assets/ui/select.svg) | Atlas; contextual guide inside an overlay |
| ![Start](assets/ui/start.svg) | Expedition menu |

Menus stop simulation. Attacks are automatic when fighting is enabled and a hostile is in reach. Move to interrupt work or a travel route. In the forge, ![directions](assets/ui/horizontal.svg) chooses a design, material and grade; ![cross](assets/ui/cross.svg) on the final row crafts it. The guide explains each section.

## Build and test

```sh
bash scripts/build_native.sh
export PSPDEV=/path/to/pspdev
export PATH="$PSPDEV/bin:$PATH"
make -j2
python scripts/validate_pbp.py EBOOT.PBP
```

The native build exercises the same simulation and renderer. `make` uses the PSPDEV SDK to produce a real PSP PRX inside `EBOOT.PBP`; it is not a renamed desktop executable. The read-only GitHub Actions workflow runs native checks and compiles an EBOOT artifact using a pinned official PSPDEV toolchain.

```sh
# Full release validation; requires PSPDEV, PPSSPPHeadless, Pillow and websockets:
bash scripts/validate_v050.sh /path/to/PPSSPPHeadless
# Package only after every controller run verifies the exact executable:
python3 scripts/package_v050.py
```

A desktop interactive build can define `USE_SDL` and link SDL2. Keyboard mapping: arrows, X/C/Z/V map to the four face-button pictograms; Q/E map to the shoulder keycaps; Enter and Tab map to the two system keycaps.

## Practical limits

The engine uses a fixed-size world state, capped actor/building/node pools and a software renderer designed for the 32 MB PSP model. It targets a 30 Hz display cadence at 333 MHz; emulator results do not establish physical hardware performance. This alpha has simplified town services, local navigation, faction simulation and job logistics. Interiors are single rooms with services, not multistorey NPC schedules or indoor combat. It has no slavery economy, multiple independent squads, diplomacy campaign or Kenshi asset/content parity. See the [Kenshi mechanics audit](docs/MECHANICS_AUDIT.md) and [iteration record](docs/ITERATION_LOG.md) for tested changes, simplifications and remaining work.

MIT license for the original source and included artwork. Art source atlases and the reproducible sprite packing script are included. PSPDEV and PPSSPP remain separate projects with their own licenses; neither is bundled in the game release.
