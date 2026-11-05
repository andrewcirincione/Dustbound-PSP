# Kenshi mechanics audit

Checked against the Kenshi wiki while developing 0.4.0 and revisited for 0.5.0. This is a scope audit, not a claim that Dustbound reproduces Kenshi. Dustbound uses original code, artwork and setting. A working simplified loop is recorded separately from a missing system.

| Area | Dustbound 0.5.0 | Remaining depth |
|---|---|---|
| Exploration | 1536-square persistent frontier; 16 compact settlements, 9 regions, roads/fords, encounters, atlas, weather | Denser authored landmarks, ecology, more world stories |
| Survival | Hunger, automatic ration eating, nearby backpack/depot food sharing, fatigue, starvation, rest | More food types, cooking chains and race-specific diets |
| Health | Six body regions, blood, bleeding, knockout, death, missing limbs, prosthetics, carrying, first aid, doctors | Separate stomach; richer recovery states and robotic repair |
| Combat | Windup/contact/recovery, heavy/light/ranged families, blocks, dodge, stagger, armor, skill and strength/injury handling | Layered cut/blunt coverage, projectiles/friendly fire, tactical stances |
| Encumbrance | Pack/equipment/carried-body weight, strength, carry bonuses and movement penalties | Per-backpack inventory grids and combat/dodge penalties |
| Skills | 20 skills trained through actions | Weapon-category skills, swimming, assassination and full robotics |
| Gear | 2,112 modular variants; six visible slots; material, quality and effects; worn/candidate comparison | More authored silhouettes and layered armor coverage |
| Towns | Role/faction outfits, distinct services/advice, regional stocks, recruits, paid beds, doctors, barbers, contracts | Resident schedules and richer dialogue consequences |
| Crime | Theft, witnesses, stolen ownership, fencing, locks, bounty, custody/bail/escape | Trespass zones, kidnapping consequences, faction-uniform disguises |
| Slavery | **Absent** | Captivity/work orders, shackles, liberation and ex-slave recognition |
| Factions/world states | Reputation, hostility, prosperity, unrest and ownership events | Leader death/imprisonment causing persistent town overrides and retaliation |
| Outposts | Construction, crops, water, storage, eight jobs, crafting, forging, three research tiers | Construction labor/deliveries, queued job priorities, tax visits and richer raids |
| Power/research | Windpump-assisted crops and limited research | Actual power networks, batteries, fuel and artifact-based technology tree |
| Squads/animals | One party of up to eight, follow/hold/jobs, rescue/carrying; separate timed escort/camp guards with first aid | Independently commanded squads; recruited/traded/aging animals |
| Interiors/saving | Walkable single rooms with services, collision and persistent position; six manual slots plus autosave | Multi-floor rooms, resident schedules and indoor combat |

The most consequential missing core systems are slavery/captivity, disguises/trespassing, leader-driven world states, independent squads, and deeper outpost logistics. These remain open; the current alpha should not be described as a complete Kenshi equivalent.

## Reference pages

- [Getting Started](https://kenshi.fandom.com/wiki/Getting_Started): basic sandbox, crime and survival loops.
- [Statistics](https://kenshi.fandom.com/wiki/Statistics) and [Guide to Training Statistics](https://kenshi.fandom.com/wiki/Guide_to_Training_Statistics): skill coverage.
- [Guide to Health](https://kenshi.fandom.com/wiki/Guide_to_Health), [Damage](https://kenshi.fandom.com/wiki/Damage), [Food](https://kenshi.fandom.com/wiki/Food): injuries, recovery, shared food and armor distinctions.
- [Encumbrance](https://kenshi.fandom.com/wiki/Encumbrance): weight, strength and backpack effects.
- [Slavery](https://kenshi.fandom.com/wiki/Slavery) and [Disguise](https://kenshi.fandom.com/wiki/Disguise): missing crime/captivity systems.
- [World States](https://kenshi.fandom.com/wiki/World_States): causal faction and settlement changes.
- [Guide to Building an Outpost](https://kenshi.fandom.com/wiki/Guide_to_Building_an_Outpost): jobs, infrastructure, research, taxes and raids.

The wiki's mechanics are references, not assets imported into the game. Its descriptions are paraphrased here.

The [0.5.0 design review](DESIGN_REVIEW_V050.md) expands this matrix into a prioritized whole-game critique with evidence and concrete next-milestone acceptance criteria.
