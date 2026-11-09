# 0.17.7 — Dust soundtrack

- The full Dust.mp3 loops from the title screen through gameplay, menus and pauses. The EBOOT streams it from disk alongside the game’s sound effects.
- The PSP home screen uses a new 480×272 wallpaper. The game tile uses the original icon.

# 0.17.6 — The town watch

- Passive and unarmed walks use relaxed arms. One-handed weapon grips leave both arms readable. Fallen and resting survivors have a revised ground pose and stay down after waking until they act. Self-treatment can be done while down.
- Trading posts have more forgiving door reach. Town NPCs walk into and out of buildings on their own schedules. Inns have four beds, and smithies sell weapons and armor. Research benches are used outdoors.
- Inventory and loot show quantity and base value per item. Stolen stacks are marked red and traders show sale odds based on Stealth and Thievery. Shopkeepers watch owned stock. New low-level bandits carry fewer useful supplies.
- Factions shows charges and bounty amounts. Minor bounties prompt a fine or jail. The watch pursues serious bounties, and paying clears the charge and ends that pursuit. A jail escape renews the pursuit. Novice lockpicks usually fail and train the skill.
- Dropped bodies stay where you set them down. The last ticker entry stays longer. Skill gains are silent, room labels clear the ticker, and the minimap shows a plain N.
- Save format v23 reads v22 and older expeditions.

# 0.17.5 — Field survival

- Untreated limb damage stays until bandaged. Treated wounds recover slowly on the trail and faster in beds. Severe damage takes longer. Limb bars show healthy tissue, untreated damage and dressed damage separately, and bleeding is marked red.
- Guards carry wanted survivors into jail cells, confiscate stolen goods and provide treatment. Pick the cell lock and reach the exit before the watch relocks it.
- Slave camps issue shackles and forced work. Seized belongings stay in the camp chest. Pick the shackles or get help from an ally, and watch for former captors.
- Portable bedrolls can be bought, deployed and packed up. Outdoor chests open a contents list. Ore seams are farther apart and outside towns.
- Nearby NPCs show whether they can see a sneaking survivor. Pursuers track the last place they saw you. You can play dead and bandage yourself, and blocks have a distinct animation.
- Some traveling groups trade from their own packs. Faction conversations and rumors have separate lines. Trade, treatment and jobs improve standing. Unwelcome visitors may be warned before occupants attack.
- Map pins show on both maps with minimap guidance and clear on arrival. The title, pack and skill layouts are cleaned up. Starting backpacks belong to the Prospector and Caravan starts.
- Save v22 retains v21 and earlier expeditions.

# 0.17.4 — Town life and combat

- Towns have zero to two work offers from an identifiable local contact. The Journal tracks current objectives, progress, turn-in and payment. Retired the scripted Copper Road and regional quest menus.
- Shop and home chests hold owned supplies, with replenishment after two to four days. Residents sometimes stay home, and three recent thefts trigger temporary pocket searches for two in-game days.
- Expanded the pack to ten visible rows, enlarged the worn-gear preview, removed the creator's head inset, grouped skills and showed exact XP progress. Bow and crossbow training is listed under Weapons.
- Fight stance answers nearby aggressors and attacks at the end of recovery. Passive stows the weapon. Armed townspeople rally with the watch. Enemy formations withdraw under severe losses. Leg injuries cause slow limping or crawling.
- Put resting survivors on their beds, slowed wound recovery and scaled first-aid time to injury severity. Hungry squad members share rations with nearby allies.
- Blended region edges and daylight, reset the PSP idle timer during analog input, clarified ore seams, repaired the Dry Market shop overlap and sped up the busy news ticker.
- Save format v21 retains v20 injuries and migrates earlier expeditions.

# 0.17.3 — Recovery

- Blunt injuries recover on their own. Open cuts worsen and bleed until they clot or are dressed. Severe head trauma and blood loss can kill. Deep vital injuries can cause a Toughness-based recovery coma.
- Dressings, pressure bandages, repairs, clinic care, and blood tonics take time. Beds speed recovery.
- Jail staff treat injured prisoners. Cell locks take time to pick, with tools or improvised wire. Agility grows with travel and raises movement speed.
- Rare travelers can render first aid and sometimes carry unconscious survivors to a safe bed.

# 0.17.2 — Fieldwork

- Followers track the controlled squad member, including inside buildings. Separate assignment and priority tabs preserve direct jobs. The controlled member only begins automatic work after five idle seconds in Passive stance.
- The HUD shows menu shortcuts and scrolling news, including squad level-ups and urgent injuries. Bleeding turns squad health bars red. NPC role labels appear at close range.
- Pass items to a chosen ally. Nearby Actions offers self-treatment and explains unavailable actions. Dressed wounds recover slowly without a bed.
- Owned containers identify their faction before looting. Some witnesses warn on the first offense. Fleeing civilians keep to an escape route and wait for danger to pass.
- Scenery stays visible near actors and varies across regions. Walking uses distance-driven strides. Character creation includes larger previews and independent hair and beard styles.
- Five scenarios start at different inland settlements. The atlas, local map, and minimap use the walking view's orientation.

# 0.17.1 — Succession

- The deaths of capital leaders, named settlement chiefs and their successors can open new councils.
- Winning factions promote a surviving guard or send a new steward. Each incumbent keeps a separate identity and equipment.
- Residents can pass on the name of the new leader. Version 18 loads Three Banners saves.

# 0.17.0 — Three Banners

- Add Brass Junction, Vermilion Keep and Cinder Abbey, with distinct uniforms, civic layouts, markets and services.
- Give the three powers persistent armies, rival patrol clashes, conquest with real garrison casualties and guarded labor yards that hold defeated soldiers.
- Add overheard bandit warnings and military news from recent simulation events. Ask ordinary NPCs what they have heard.
- Save v17 adds the new frontier to older expeditions while retaining towns, equipment and skill XP.

# 0.16.3 — Crowded combat rendering

- Restrict each raster row to the weapon's projected bounds and reuse column projections. Weapon textures, warp and fittings use the original sampling.
- Cache shaded palettes across outfits and use exact integer averaging for translucent sprite pixels. The 21-fighter benchmark compares both frame and simulation hashes.

# 0.16.2 — Weapon skills and PSP performance

- Weapon training uses Slashing Blades, Light Blades, Heavy Weapons, Polearms, Archery and Firearms. Each weapon uses its own motion profile and damage balance.
- Save format v16 adds together the earned XP in each merged skill, capped at level 100. Earlier saves migrate for both live squad members and stored encounters.
- Limit sprite filtering and outlines to their painted bounds, batch ground spans, and skip distant candidates before costly lookups. The world benchmark compares framebuffer and gameplay-state hashes alongside CPU timings.

# 0.16.1 — Combat motion

- Fighters draw from three attacks per weapon type, with different windup, stroke and recovery times. Each cycle uses all three before shuffling again, and adjacent attacks use different moves.
- Katana cuts use quicker changes of direction. Greatswords use a planted cleave, a low sweep and a braced thrust.
- Unarmed fighters use a two-fist guard, a lead jab, a rear cross and a rising hook. Arms stay within reach and the rear foot stays grounded during strikes.
- Weapon proficiency and Martial arts shorten attack timing. Save format remains v15.

Run `bash scripts/validate_v160.sh build/v161`, compile the PSP build, then run `python3 scripts/emulator_v161.py /path/to/PPSSPPHeadless` to reproduce the animation checks.

# 0.10.0 — Peoples, courts and personal arms

Five species with visible base stats; 18 named owners and 36 personal items; 96 cultural armor variants; distinct settlement plans; 12 constructions across all 38 weapon types; clear crafting and forging requirements. Save v10 with earlier-save migration. See docs/RELEASE_NOTES.md and docs/VALIDATION_V100.md.

# 0.5.2 — Reliable actions and recovery

## 0.6.2 — Analog movement and D-pad menus

Analog-only character movement, live status panel on D-pad Up, Skills on Down, worn gear on Left and squad orders on Right. D-pad shortcuts preserve job queues and travel. Desktop WASD emulates the stick. Save format unchanged.

## 0.6.1 — Compact UI

20-pixel HUD; in-range-only prompts; full-width inventory/shop/loot lists without try-on models; tabbed Start menu with direct Skills access; explicit pickpocket attempt screen and owned-salvage labels. PSP compiled and static layouts inspected; no gameplay suites or emulator runs. Save format unchanged.

## 0.6.0 — Copper Road rebuild

- Reconstruct a persistent rescue/road expedition with eight landmarks, named dialogue and repair, truce, capture and lethal outcomes.
- Add town trade/allegiance consequences, a physical convoy, captives, escape recognition and retaliation.
- Add three expedition groups, four job priorities, construction labor, fuel, station condition and armor upkeep.
- Add moving projectiles, indoor actors/combat, witnessed trespass and market hours.
- Add trauma, mechanical repair, armor underlayers/coverage and six panels with contextual guides.
- Preserve complete clothing on prone bodies; remove overlapping landmark labels and misleading modal controls.
- Fix named corpse recycling, projectile collision over water, unfinished worker stations and room-distance looting.
- Write v6 saves while retaining v1–v5 migration. Physical PSP-3000 qualification remains outstanding.


- Preserve exact help/save navigation state; save-and-title selects and confirms a slot.
- Preserve clean/stolen ownership through exact-slot use, equipment, passing, selling, crafting, looting and unloaded ranged weapons; transactional full-pack failure.
- Add explicit rescue choices, loot comparisons and bulk loot/depot transfer.
- Prefer usable nearby targets; add left-shoulder/interaction target cycling.
- Enforce station/bed/depot reach and barber/marshal proximity; reject invalid travel and stop stalled routes.
- Complete fetch/walk/craft/store worker loops, including cooking water and retained food; expose job status; recognize limb injuries for medics.
- Add skill explanations/progress and bounded toughness, perception, farming, cooking, engineering and science effects.
- Expand shop previews, show actual recipe/forge requirements and effect values, fix health text, add recent messages, respect pause in custody.
- Preserve save format 5, item IDs, textures and fixed render caches. Physical PSP-3000 testing remains outstanding.

# Changelog

## 0.5.1 — Packs and proper grips

- Fit all twelve headwear designs to individual brow anchors, preserve aspect ratio, reduce width and join necks to collars.
- Replace catalog-order clothing stats with 44 authored profiles; expose actual movement, stealth, load and weather bonuses. Review all clothing silhouettes and test 1,056 variants.

- Mount each back design at the shoulders/belt; turn its straps toward the wearer.
- Reflect weapons around their authored grips so cutting edges and mirrored hands agree in both facings.
- Stock four affordable backpack types in every market; add a BACKPACKS filter with worn/candidate portraits.
- Give bags deliberate cargo-weight, capacity, combat and stealth tradeoffs, plus medical/recovery roles; show the actual swap-load result.
- Add a contextual backpack chapter and preparation advice.
- Locate active contracts on the atlas without replacing travel orders; show held supply counts.
- Preserve v5 save layout/item IDs and supplement older markets once.
- Add all-family attachment, backpack gameplay and genuine v5 migration checks, controller evidence and an updated whole-game critique.

## 0.5.0 — Heavy steel, hired hands

- Add greatsword, railway maul, crescent glaive and anchor cleaver: 36 weapon families and 2,112 modular gear variants.
- Make melee windup/recovery depend on strength and arm wounds; show requirement and delay in equipment comparison.
- Add faction/profession outfits, role/faction context labels, specialized advice and regional weapon stock shown before shared starter goods.
- Add saved one-day escort/camp guard contracts, first aid, expiry/dismissal and ledger clocks, separate from permanent recruits.
- Give merchants/medics retreat behavior, guards home posts and medics a separate walkable service location.
- Migrate v1–v4 expeditions to save format 5 without changing old item IDs; refresh NPC identity and markets once.
- Add a 34-area design critique, native contract/migration tests, 36-family art checks and five exact-EBOOT controller playthroughs.


## 0.4.1 — Worn steel

- Replace all 32 geometric weapon designs with original painted art, retaining material and quality differences.
- Author handle pivots and rifle/crossbow/bow support-hand positions; preserve rigid follow-through.
- Fix downward/long-weapon clipping in actor caches and oversized equipment escaping menu previews.
- Fit WORN and TRY ON figures at the same scale; cache menu images to avoid re-rendering unchanged art.
- Replace the obsolete PSP launcher icon/background and texture indoor thresholds.
- Add actual rasterized grip, attack silhouette and preview-bounds checks, plus controller-driven equip captures for all weapon families.

## 0.4.0 — Frontier playability

- Show the nearby target, available action and exact reach; cancel gathering/lockpicking outside reach.
- Separate merchants, medics, innkeepers, marshals, smiths, barbers and recruits.
- Expand inter-town distances fourfold; add wilderness encounters and local/world atlas views.
- Compare equipped and highlighted gear in pack and shop screens, including portraits, deltas and effects.
- Compile painted modular characters, beasts, frontier props, calibrated buildings, projected shadows and textured rooms.
- Preserve old exploration and compact town/outpost layouts when migrating saves to version 4.
- Share nearby backpack/depot rations, require food for wound recovery, preserve stolen ownership when passing items.
- Balance starting carrying capacity; test travel, mining reach, role menus, old saves and food rules.


## 0.3.0 — The weathered frontier

Replaced flat exterior art with original painted buildings and wilderness sprites, added furnishings and walkable single-room interiors with trading, beds, forging, research and storage. Furniture collision and depth ordering keep rooms navigable. Version 3 saves preserve indoor position and migrate version 1/2 saves.

Fixed sword follow-through by solving both hands and the weapon in one rigid screen-space coordinate system; fingers draw over the handle. Revised walking leg travel, cloth facets, character labels and the weathered interface. All controller prompts and guide text now use button pictograms/keycaps, including full Start/Select labels.

Added indexed row-span sprites, palette lighting, scrolling terrain reuse and a cached RAM framebuffer. Added interior, migration and rigid-grip regressions, a nineteenth guide chapter, source art and a reproducible sprite compiler. Physical PSP-3000 qualification is still outstanding.

## 0.2.1 — Dust and steel

Reworked character proportions, shaded faces, weathered cloth/metal, articulated knees, hip loading, elbows and two-handed grips. Four attack motion families blend continuously through preparation, strike and recovery. Moved damage resolution to the visible contact point and removed bright circular slash trails. Fixed squad portrait overflow and fitted menu previews to the taller silhouettes.

Added regression checks for contact timing, single-hit resolution and continuous pose transitions across all 32 weapon families. All 2,016 gear variants remain visually distinct. Save format remains v2. The optional QA recorder now captures every frame with deterministic 30 Hz simulation pacing to avoid capture I/O changing the animation. This mode is disabled without the developer marker.

## 0.2.0 — Frontier expansion

Expanded the original playable PSP sandbox with character creation, a 384-square world, sixteen settlements, nine regions, six visible equipment slots and 2,016 modular gear variants. Added custom forging, service dialogue, ambient speech, contracts, a faction ledger, weather exposure, custody, storage and eight-member squads/jobs. Added eighteen guide chapters and a save ledger with v0.1 migration.

Combat now separates windup, contact and recovery. Weapons animate with directional arcs and projectile traces, with recoil, stagger, hit stop, effects and procedural sounds. Arms affect accuracy/weapon use; leg wounds affect movement. Existing blood loss, limb loss, rescue and prosthetics remain.

Fixed stolen ownership through equipment swaps, transactional legacy loading, public inn bed fees, missing-arm weapon attacks and repeat knockout credit. Added full catalog visual uniqueness checks and 46 expansion checks in addition to the 61 baseline checks.

## 0.1.0 — Playable foundation

Initial original PSP squad survival game, native checks, EBOOT emulator playtest, source repository and downloadable release.
