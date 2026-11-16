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
