# Dust 1.2.2 — completed

Published release: https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v1.2.2

Release commit: f04ebe8feb8162cf40b9f768f668651c50147c87

EBOOT SHA-256: a2782c183a2adc9107a1215a2c677d0dd59fe621c38e5e9fb8f4c0bcbfac2a96

| Feedback | Implemented behavior |
| --- | --- |
| Louder navigation; selection sound already fine | Double navigation amplitude; leave the selection sound unchanged. |
| Greatsword blocks the face | Shallower shoulder carry, raised grip and bent elbows; blade stays in front of the coat and behind the chin. |
| Toughness trains too slowly | Twice the previous Toughness XP. |
| Remove the equipped-item legend | Keep `(E)` beside worn items; remove the explanatory legend. |
| Remove four trade skills and excess camp construction | Remove Farming, Cooking, Engineering and Science from playable skill lists and bonuses; only Bonfire and Bedroll remain in camp construction. Remove obsolete iron/research readouts. Repairs use Smithing. |
| No depot service at storehouses or other sites | Allow shared storage only at actual settlement depots and owned-home chests. Gate exterior actions, counters, clerk dialogue, transfers and automatic workers. Local loot chests remain finite loot. |
| Restore material descriptions | Restore the pre-1.2.1 Cured hide name and brief material description. |
| Faster attack/defence progression | 50% more XP for weapon classes, melee attack/defence, dodge, martial arts, ranged weapons, perception, strength and dexterity. |
| Inventory/shop weight column | WT appears between QTY and VAL; weights are per item. |
| Field medicine trains too slowly | Twice the previous medicine XP. |
| Limping hides the stance | Remove injury text from the stance position; Fight/Passive remains visible while injured. Keep the physical injury animations and penalties. |
| Redoubt Nine and other interiors | Barracks have eight beds and a mess area; upper floors have separate armory/maintenance and command/briefing uses. All 46 sites use purposeful floor layouts and location/tier-appropriate rewards. |
| Indoor slowdown and duplicate location label | Cache backgrounds, walls and navigation samples, reuse existing scenery memory, reduce texture arithmetic, and correct upper-floor collision. Show the current building/floor at the top left only. |
| Stolen tags last indefinitely | Expire after two in-game days. Preserve deadlines through equipping, squad transfers, partial consumable use, crafting/forging, dropping/picking up, home storage, deployed bedrolls, seized property, confiscated weapons and saving. |

Qualification: all 55 native suites passed; the complete PSP controller run passed with 62 captures. All 20 added greatswords across two facings, three builds and five species passed the carry audit: 34,570 painted face pixels checked and zero covered. Existing joint/grip/attack tests, 720 walking poses, old-save fixtures, all 106-floor connectivity and prop-footprint checks, and four 20-minute wilderness simulations passed.

The native Redoubt comparison measured 73–83% lower standing rendering cost and 21–27% lower scrolling rendering cost. These are host component measurements, not PSP hardware frame rates. PPSSPP scenarios use declared prepared saves; the wilderness simulation holds player health constant to isolate encounter availability. Physical PSP hardware was not tested.

Save format 36 appends theft timestamps. Released save formats remain compatible on upgrade. Exhausted loot stays exhausted, partly depleted caches do not refill, and existing player progress is preserved.

The release contains the EBOOT, PSP installer, evidence archive and checksums. This completed record supersedes intermediate work checkpoints.
