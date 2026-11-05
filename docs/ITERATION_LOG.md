# 0.4.0 critique and correction record

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
