# Dust 1.2.2 work checkpoint

Base: published 1.2.1, `db2341042a5c6d40aaeee345d271db7f6e9d48fe`.

Requested scope (2026-11-16):

- Louder menu navigation; preserve selection sound.
- Bend the greatsword carrying elbow, lift the grip, lower the blade angle and keep the face clear in both facings.
- Increase Toughness training; increase attack/defence-related XP 50%; double field medicine XP.
- Remove the inventory `(E) EQUIPPED` legend; retain `(E)` on worn items.
- Remove Farming, Cooking, Engineering and Science as playable skills. Keep only bonfire and bedroll in player camp construction. Preserve old save field/item IDs.
- Shared storage is available only at actual depots and owned-home chests, never abandoned storehouses or other wilderness sites.
- Restore ordinary material item descriptions; remove the unsolicited leatherworking definition.
- Add WT beside QTY and VAL in inventory and trading lists.
- Remove limping text; preserve the Fight/Passive stance display.
- Rework Redoubt Nine and audit other expedition interiors: practical layouts, barracks beds, distinct floors and rewards, location at top left only, investigate and reduce indoor rendering/simulation cost.
- Clear stolen tags after two in-game days, with persistent timing across saves and transfers.

Status: implementation started; source checkout recovered. No 1.2.2 build or tests claimed yet.

Release requires native regressions, old-save migration, actual EBOOT controller checks, visual inspection, and published GitHub assets. Physical PSP verification is unavailable.

Implementation checkpoint:

- All requested gameplay/UI changes are implemented, including separate theft deadlines per stack and v36 save migration.
- Redoubt Nine has 8 sleeping berths on the barracks floor, a separate armory/maintenance floor and a command floor. All 46 expedition sites use floor-specific layouts and useful role/tier-based rewards; exhausted loot is never refilled by migration.
- Shared storage access now excludes player-built stores as well as ruins/outposts/storehouses; automatic workers also honor the depot allowlist.
- Native tests: 54 passing; one wilderness pacing regression remains under investigation (130-second gap against a 120-second limit). The 106-floor connectivity test passes.
- Two-day theft acceptance tests pass through equipping, unequipping, passing, dropping/picking up, home storage and save/reload. Separate stolen batches do not refresh each other's deadlines.
- Redoubt native standing render medians improved from 0.70–0.89 ms to 0.08–0.23 ms; tick p95 from 0.57–0.61 ms to 0.13–0.14 ms. These are host measurements, not PSP hardware measurements.
- Initial EBOOT compilation passed. Final pose changes, emulator verification and release publication remain pending.
