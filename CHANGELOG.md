# 0.5.2 — Reliable actions and recovery

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
