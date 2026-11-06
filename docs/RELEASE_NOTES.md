# Dustbound 0.6.1 — compact UI

- Reduced the bottom HUD from 60 pixels to a single 20-pixel status strip. Removed the permanent controls footer and indoor diagram.
- Removed nearest-object distance/readiness banners. The one-line action prompt and target marker only appear within usable interaction range; target cycling includes reachable targets only.
- Removed duplicate Worn/Try On models from inventory, stores and loot. Full-width item lists retain equipped-item comparisons, weight, penalties and effects. The worn-equipment page still shows your actual character.
- Replaced the long Start list with Pause, Squad and World tabs. L/R changes tabs; Circle returns from a menu subpage, and Start resumes. Skills, body, orders, inventory and camp tools have explicit entries. Bounty payment, barbers, beds and smith services are accessed where they exist in the world.
- Pickpocketing now opens an explicit attempt screen showing the item, success chance and caught penalty before committing. Indoor residents support the same action. Empty pockets and a full inventory do not roll a theft attempt. Owned salvage says STEAL.
- Updated the in-game guide and control documentation.

## Controls for the reported questions

Skills: Triangle, then R once; alternatively Start > Squad > Skills.

Steal: Circle to sneak, approach a civilian, Cross to inspect Pickpocket, then Cross to steal or Circle to cancel. Named story/rescue characters retain their authored outdoor interactions. Owned salvage uses Cross when its prompt says STEAL. Stolen items retain their owner; Black Mesa fences accept them.

## Install

Extract the PSP folder from Dustbound-0.6.1-PSP.zip to the memory stick root, or replace PSP/GAME/DUSTBOUND/EBOOT.PBP. Keep your existing save files. Save format is unchanged from 0.6.0.

## Verification

PSP compilation and static 480x272 native-renderer layout inspection. The screenshots are native-rendered layout frames, not PSP emulator captures. No gameplay test suites or emulator runs were performed for this update. Physical PSP verification remains outstanding.
