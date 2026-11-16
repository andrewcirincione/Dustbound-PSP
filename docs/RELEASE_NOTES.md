# Dust 1.2.2 — Carry and quarters

Menu navigation sounds are twice as loud. The selection sound is unchanged.

Greatswords use a shallower shoulder carry with a raised grip and bent elbows. The blade passes below the face. Both facings and all 20 expansion greatswords are reviewed with the production renderer and representative weapons in PPSSPP.

Attack and defence-related XP is 50% faster, including weapon classes, melee attack/defence, dodge, martial arts, ranged weapons, perception, strength and dexterity. Toughness and field medicine earn twice their previous XP. Skill caps and existing progress are preserved.

Farming, Cooking, Engineering and Science are removed from the playable skill lists and no longer train or apply skill perks. Mechanical repairs use Smithing. Camp construction contains only Bonfire and Bedroll. Existing town services, cooking recipes and owned-home furnishings remain available. Bonfires cost three timber; bedrolls must be carried in the inventory.

Inventory and shop lists include WT alongside QTY and VAL. Weight is per item; the LOAD total includes quantities and equipped gear. Equipped items retain their `(E)` marker; the explanatory `(E) EQUIPPED` legend is gone. The hide item has its original name and brief material description again. Injury animations and speed penalties remain, while limping/crawling text no longer replaces Fight/Passive.

Shared storage can be opened and transferred only at an actual settlement depot or a chest in an owned home. Expedition sites, abandoned storehouses, ruins, outposts and old player-built stores do not offer depot services. Their local loot chests remain ordinary finite loot. Automatic workers honor the same depot restrictions.

All 46 expedition locations have floor-specific interiors. Redoubt Nine has an eight-bed sleeping wing and mess area, a separate armory/maintenance floor, and a command/briefing floor. Other locations have machine halls, control rooms, archives, treatment wards, freight stores or sleeping galleries as appropriate. Armor stands are limited to armories. Doors, stairs, beds, caches and counters remain reachable across all 106 floors. Upper floors and guarded caches provide useful weapons, armor, medicine, parts and supplies matched to the location and difficulty. Save upgrades preserve exhausted and partly emptied loot quantities.

The current building and floor appear at the top left. The duplicate bottom-right label is removed. Indoor background caching reuses existing memory, fixed wall geometry and navigation samples are shared, and floor texturing avoids per-pixel division. Upper-floor collision now uses the actual floor plan. Native benchmarks and PPSSPP checks are included in the evidence; physical PSP hardware has not been measured.

Stolen tags expire after two in-game days of simulation. Each stolen stack/piece has its own persistent deadline. Equipping, passing between squad members, dropping/picking up, home storage, crafting, deployed bedrolls, seized property, and save/reload preserve its age. Separately stolen batches do not refresh each other's deadlines. Time does not advance in menus. Older stolen items without recorded dates begin their two-day timer when the old save is first loaded.

An outdoor pacing regression was also corrected: enemies outside the visible area no longer suppress travel encounters as if they were on screen. Four 20-minute travel simulations retain their encounter-density limits.

Save format 36 appends the new theft timestamps. Older expeditions upgrade automatically. Replace the EBOOT and keep your saves.
