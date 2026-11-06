# Dustbound — PSP frontier survival RPG

An original isometric western–samurai squad RPG for PSP homebrew. Build a band of drifters, survive wounds and hunger, trade, steal, forge equipment and establish an outpost across a changing frontier. Inspired by Kenshi's systemic survival; this is a separate game using original code and original generated / procedural art, not a Kenshi port.

**Current milestone: 0.6.0, Copper Road rebuild — playable alpha.** A much broader sandbox than 0.1, with working versions of the requested core loops. It does not reproduce every Kenshi system or its scale. Physical PSP qualification remains outstanding.

[Download releases](https://github.com/andrewcirincione/Dustbound-PSP/releases) · [Field guide](docs/FIELD_GUIDE.md) · [Validation](docs/VALIDATION.md) · [Progress](docs/ROADMAP.md)

![Copper Road rescue dialogue from the PSP EBOOT](evidence/v060-05-sora-wreck.png)

## 0.6.0: the Copper Road

An authored expedition gives the starting frontier a lasting conflict. Find wounded engineer Sora at the Salt Road wreck and escort her home. Repair the dry culvert, buy a truce, deliver Rell alive, or defeat him. Report to Nell at Copper stores. These outcomes persist in town allegiance, prices, stock, dialogue, convoy travel and retaliation. Eight landmarks add caches and route leads; the expedition ledger locates them on the atlas without replacing your travel order.

Construction now takes work, and owned fires/forges consume timber or coal. Four ordered job priorities and three expedition groups support separate travel leaders. Recovery distinguishes bandages, lasting trauma and mechanical repair. Captors carry fallen survivors to the chain yard; timed lock work, rescue and escape recognition offer a way back. Light cloth can be fitted beneath outer armor with distinct coverage and cut/blunt protection.

Projectiles move through the world, strike intervening actors and stop at structures. Defensive, ranged and hold-fire orders apply indoors and outdoors. Actual residents and nearby followers enter rooms; trespass requires a conscious witness. Markets open from 06:00 to 22:00. Complete dressed bodies retain their clothing and backpacks when prone.

Use the new expedition-menu entries for Copper Road, Orders, Camp Work, Recovery and Armor Layers. Each has a contextual guide and controller icons. Read the [critical review](docs/DESIGN_REVIEW_V060.md) and [validation record](docs/VALIDATION.md) for implemented scope and limits. This is an original, bounded PSP survival RPG, not a complete reproduction of Kenshi.

![Copper Road outcomes in the PSP EBOOT](evidence/v060-11-truce-outcome.png)
![Ordered squad roles in the PSP EBOOT](evidence/v060-19-orders.png)

The sections below record the earlier systems retained by the rebuild.

## Reliable actions, recovery and camp work

0.5.2 audits all 27 screen types and the survival loop. Help returns to the exact selection; title/load cancellation returns to its origin; save-and-title asks for an explicit slot confirmation. Inventory operations preserve the selected stack and its stolen owner, including equipment, crafting, passing and salvage. Loot has worn/candidate previews and full-stack transfer.

A rescue menu separates first aid, carrying and recovering gear. Nearby actions can be cycled with ![L](assets/ui/l.svg) held and ![Cross](assets/ui/cross.svg). Workstations and beds require actual reach. Cooks fetch water and food, walk to campfires, and return output to depots; job status explains what they need. Shops show weapon/armor previews, recipes show held/needed materials, skills explain training and live effects, and recent messages remain accessible.

Toughness, perception and four production professions gain bounded practical benefits. See the [critical review](docs/DESIGN_REVIEW_V052.md) for the complete finding/change matrix, all-screen audit, balance rules and remaining limitations.

![Explicit rescue choices in the PSP EBOOT](evidence/v052-91-rescue-choices.png)
![Loot and greatsword comparison in the PSP EBOOT](evidence/v052-98-loot-greatsword-comparison.png)

## Equipment fit and useful choices

0.5.1 corrects low/reversed backpacks, sword cutting-edge direction and left-facing shoulder/hand attachment. Per-design mounts place packs at the upper back, with separate shapes for flags, cloaks and scabbards. All twelve hats/hoods have smaller, aspect-correct silhouettes and individual brow anchors; necks meet collars. All 44 clothing designs receive native fit review and controller-driven equipment captures.

Garment-specific profiles replace catalog-order armor values. Plate offers protection at a weight, movement and stealth cost; robes remain light, and travel clothes favor weather, recovery or movement. Comparisons show actual movement percentages, stealth, carrying/weather bonuses and effects. The [balance table](evidence/v051-clothing-balance.csv) lists every design at the same grade.

Every trader now stocks four affordable bag types: travel pack, medicine roll, ore harness and bedroll pack. A dedicated BACKPACKS tab shows WORN / TRY ON portraits. Packs offer different cargo-weight relief, carrying allowance, combat/stealth penalties and medical/recovery benefits. The comparison predicts actual loaded weight after an equipment swap; all use the existing 24-slot inventory.

The Journal can locate an active contract on the atlas without changing an existing travel order. Supply objectives show held/needed counts. A new contextual guide explains backpack choices. Existing v5 saves keep their item IDs and receive the new market stock once.

The [updated whole-game critique](docs/DESIGN_REVIEW_V051.md) separates these shipped fixes from the remaining gaps in world consequences, wilderness encounters, defeat/recovery, independent squads and outpost logistics.

![Backpack shop captured from the PSP EBOOT](evidence/v051-70-backpack-market.png)
![Loaded backpack comparison captured from the PSP EBOOT](evidence/v051-73-pack-compare-79.png)

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

Walk through a door into a house, shop, inn, tower, smithy, workshop or depot. Counters trade, beds restore wounds, and workstations open forging, research or storage. Saves preserve the room and indoor position. Twenty guide chapters explain the rules.

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
- Six manual save slots plus autosave, checksums, backup files and v0.1 through v0.5.2 save compatibility. Twenty-five contextual in-game guide chapters.

![Combat captured from the EBOOT; prepared encounter](evidence/v050-15-combat.gif)

## Install on PSP

1. Use a PSP already configured to run homebrew. This repository does not install or modify firmware.
2. Download `Dustbound-0.6.0-PSP.zip` from the release and extract its `PSP` folder to the memory stick root.
3. The executable should be at `PSP/GAME/DUSTBOUND/EBOOT.PBP`. Launch it from **Game → Memory Stick**.
4. Back up existing `DUST_*.sav` files before upgrading. v0.1 through v0.5.2 saves are supported. Older formats migrate when loaded; save the migrated game to a different slot first. Keep a backup before using an older executable. Older-format migration refreshes regional markets, town NPC outfits and medic positions once; existing v5 saves receive the backpack supplement and the new frontier state once, preserving inventory and item IDs. New saves use version 6. Do not remove the memory stick during a save.

The single EBOOT includes its art, catalog and sounds. No network access or external assets are needed. Release packages exclude developer test saves and `DUST_QA` telemetry markers.

## Controls

| PSP input | Action |
|---|---|
| Analog / ![D-pad](assets/ui/dpad.svg) | Move in screen direction; navigate menus |
| ![Cross](assets/ui/cross.svg) | Interact, confirm, equip/use, buy/sell |
| ![Triangle](assets/ui/triangle.svg) | Pack; press again for worn equipment |
| ![Square](assets/ui/square.svg) | Fight/passive stance; pass one item in pack; move a full stack in loot/storage |
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
bash scripts/validate_v052.sh /path/to/PPSSPPHeadless
# Package only after every controller run verifies the exact executable:
python3 scripts/package_v052.py
```

A desktop interactive build can define `USE_SDL` and link SDL2. Keyboard mapping: arrows, X/C/Z/V map to the four face-button pictograms; Q/E map to the shoulder keycaps; Enter and Tab map to the two system keycaps.

## Practical limits

The engine uses a fixed-size world state, capped actor/building/node pools and a software renderer designed for the 32 MB PSP model. It targets a 30 Hz display cadence at 333 MHz; emulator results do not establish physical hardware performance. This alpha has simplified town services, local navigation, faction simulation and job logistics. Interiors are single rooms with services, not multistorey NPC schedules or indoor combat. It has no slavery economy, multiple independent squads, diplomacy campaign or Kenshi asset/content parity. See the [Kenshi mechanics audit](docs/MECHANICS_AUDIT.md) and [iteration record](docs/ITERATION_LOG.md) for tested changes, simplifications and remaining work.

MIT license for the original source and included artwork. Art source atlases and the reproducible sprite packing script are included. PSPDEV and PPSSPP remain separate projects with their own licenses; neither is bundled in the game release.
