# 0.5.1 critique and correction record

The complete finding/change/evidence table is in [DESIGN_REVIEW_V051.md](DESIGN_REVIEW_V051.md). This pass corrected low/reversed packs, weapon edge parity, left-facing shoulder geometry, hidden backpack stock, arbitrary bag roles, missing loaded comparisons, contract-map navigation and an overflowing guide page found in the EBOOT. Validation records distinguish incomplete debugger runs from the final successful scenarios.

# 0.4.1 art critique and correction record

| Finding | Correction | Verification |
|---|---|---|
| Weapons remained flat polygons against painted characters | Generated two original sixteen-cell weapon atlases; compiled all 32 families with material-aware palettes | All family native review grids and controller-driven EBOOT equip captures |
| Curved blades and asymmetric stocks cannot use image centres as grips | Explicit source-space hand anchors and rigid transforms | 2,560 rendered grip samples and 13,312 attack poses |
| Long/downward weapons disappeared beyond the actor image | Expand the fixed cache to 112×104; blit only visible bounds | Full rasterized pose sweep, both sides and eight attack directions |
| Longbow grip was on the thin arrow instead of the bow | Move the anchor to the wooden riser; place support hands on strings/fore-ends | Grip coverage check and bow/firearm inspection |
| Naginata reached outside the inventory panel in the first EBOOT review | Fit the whole figure into each preview cell; use the same scale for both comparison figures | Retained iteration1 capture versus corrected final capture; 32 bounded/cache-stable pairs |
| Other preview screens could overlap labels with long weapons | Apply fitting to creator, worn gear, forge, health and dialogue portraits | Screen render checks and emulator menu captures |
| PSP launcher icon still showed early polygon graphics | Generate matching launcher illustration and compile both PSP images | Embedded PBP image validation |
| Interior doorway was a flat beige diamond | Reuse the existing painted wood texture, keeping the exit label | Interior EBOOT capture |

Buildings, vegetation, rocks, furnishings, clothing and creature source art were inspected for the same mismatch; they already use the painted palette. UI button pictograms remain crisp functional icons. This revision does not claim that two-state creature animation or sparse wilderness detail is finished. The launcher illustration is promotional art, not a gameplay screenshot.

## Previous 0.4.0 critique and correction record

| Finding | Correction | Evidence |
|---|---|---|
| Actions were hidden behind generic prompts | Always show target and specific action, plus reach when unavailable | EBOOT world/mining captures; interaction regression tests |
| Mining worked across implausible distances | Enforce 1.15 m at start, during work and completion; jobs walk into reach | Far attempt yields no ore; close attempt succeeds; forced movement cancels |
| Every NPC offered all services | Shared role/action table drives both menu rendering and input | Specialist menu checks and actual recruitment/trading |
| Settlements were packed together | Fourfold regional spacing while keeping streets compact | Closest centers 134 m; Dusk–Copper terrain route approximately 96 seconds in native routing test |
| A bigger world made the old atlas unreadable | Local/world toggle, player marker, known town names and destination distance | EBOOT atlas captures |
| Gear selection hid replacement consequences | WORN / TRY ON portraits, stat deltas and old/new effects in pack and market | Actual looted-armor comparison/equip playthrough |
| Building foundations and shadows disagreed with the camera | Calibrate bases to 2:1 slopes; project alpha silhouettes using one light direction | Source calibration points and native/EBOOT visual reviews |
| Polygon props and simple characters clashed with painted buildings | Painted frontier props, beasts and modular articulated body parts; cached supersampling | 108 packed sprites; 2,016 distinct equipment renders |
| Indoor walls were still flat | Textured wood, plaster and beams on the same 2:1 projection | Iteration-1 versus iteration-2 room captures; EBOOT interior pass |
| Labels crowded streets; notifications covered controls | Label only the current NPC target; give modal notifications their own bottom strip | Iteration captures and menu reviews |
| Old saves could mix coordinate systems | Migrate old objects before adding expanded-world objects; reproject explored cells | Genuine v0.1 and v0.3 fixtures; v0.2 exterior-prefix test; v0.4 roundtrips |
| Passing items cleared stolen ownership | Preserve the ownership marker on the receiving stack | Transfer regression test |
| Food logistics required needless manual splitting | Nearby companions share backpack rations; owned depots feed nearby workers | Range, backpack, interior boundary and single-consumption tests |
| Some fresh starts were already overloaded | Give each origin strength appropriate to its starting loadout | All five origins start within carrying capacity |
| Rounded distance could show equal values while refusing an action | Display both distance and reach to two decimal places | Final HUD review |

| Browsing the map silently changed the travel destination | Separate the map cursor from the active route | Regression test and fresh-start PSP journey with map inspection |
| Hostiles wandered into the starting town during service tests | Place new-game hostile spawns at least 32 m from friendly town centers | All spawn/town pairs checked; indoor controller pass |
| Door prompt offered entry while combat blocked it | Explain the combat restriction in the target panel | Combat-state HUD branch and render checks |
| Town text obscured the map player label | Suppress overlapping town text; retain settlement list | Final journey map capture |

## Remaining critique

The game is still an alpha. The larger wilderness needs more authored destinations and encounter variety, the two-state beast animations are limited, and conversations need richer consequences. Characters are small at 480×272 and equipment materials can be difficult to distinguish in motion. Building occlusion uses a dithered cutaway rather than full roof removal. Interiors are functional single rooms. Physical PSP-3000 frame pacing, controls and battery behavior remain untested. The mechanics audit records larger missing systems without disguising them as completed features.

Native `iteration` images are renderer reviews. Numbered `v040` playthrough images and the combat GIF are captured from EBOOT VRAM in PPSSPP. Prepared combat/outpost/region fixtures are labeled separately from the fresh-start playthrough.

## 0.5.0 — player critique and iteration

The released 0.4.1 build was replayed before changes. The [full review](DESIGN_REVIEW_V050.md) covers 34 areas and distinguishes observed controller behavior, source audit and remaining design goals.

The pass added four heavy weapon designs, strength/injury handling, role/faction identity and regional trade, plus independent timed guards. During review the anchor-cleaver pivot failed its opaque-handle check, a strength hint conflicted with the comparison layout, a medic overlapped the merchant, common starter goods hid regional stock, and save contract validation referenced actors before validating the actor count. The expanded faction-guide text also reached the footer in a PSP capture and was shortened. A defeated raider also inherited a resident label; raiders and beasts now have explicit context labels. All were corrected before the final runs. A separate controller-harness double-step was corrected by converging on the desired dialogue row.

The final controller scenarios use the same EBOOT: fresh-start survival/save, prepared greatsword combat/outpost, shop/inn interiors, a fresh Dusk-to-Copper journey, and hire/save/three regional markets/four weapons/genuine v0.4.1 migration. The new combat recording is an actual prepared EBOOT encounter. Native checks cover all 36 weapon families and contract edge cases. Physical PSP qualification and full emergent-world depth remain open.

### Follow-up: headwear and clothing balance

The user identified large, lifted hats. Full-image bounds included straps/crests, so generic hat rectangles were the wrong anchor. Individual brow rows, preserved aspect ratios and smaller widths now seat headwear on the forehead. A neck/collar bridge removes a separate floating-head gap. All 44 garment families receive fit sheets and real EBOOT equipment captures. The stat audit found catalog-index armor and unrelated clothing perks; authored family profiles now establish consistent plate/cloth/travel tradeoffs, with 1,056 variant checks and actual bonus values in comparison.
