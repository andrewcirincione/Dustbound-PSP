# Dustbound 0.8.0 — Living frontier

- A persistent event director renews eight concurrent incidents indefinitely, combining mechanics, conditions, factions, destinations, strength and enemy roles while avoiding recent combinations. Includes caravans, raids, relief appeals, caches, stranded pilgrims, beast packs, armies and warlords. Failed attacks can lead to relief appeals. This is endless recombination of supported mechanics, not infinitely many unique authored stories.
- Incidents affect supplies, prosperity and faction control. Armies can take towns; visible formations must reach them and defeat the defending garrison. Off-screen movement and outcomes use a strategic abstraction.
- Roaming groups: armies up to 16, raiders 10, warlord retinues 9, beast packs 7, caravans 6. Distant formations release physical slots while preserving their wounds, gear, inventory and hostility. Existing residents are not overwritten.
- Five encounter strength tiers and eight enemy roles govern equipment and supplies. Very strong leaders wear signature gear. Added 16 specialist weapons/armor pieces and 8 signature pieces, using existing sprite families with distinct stats and trim.
- Deliberate attack: hold L and press Square, or Start > Squad > Choose opponent. Select someone and press Cross. Neutral/friendly assault costs 100 bounty and 20 faction standing; the victim/group retaliates. Civilians flee. Move into weapon reach to strike; Square cancels the attack order.
- Small allegiance marks: teal line friendly, gold dot neutral, red chevron hostile. Small red corner brackets show your opponent. Active opponents no longer advertise conversation or theft prompts during combat.
- Map: undistorted scale, cartographic colors, local contours, clearer roads, incidents and cursor-relative town information. Whole-world clutter reduced.

## Access

Start > World > Journal & local conflicts; L/R cycles Contracts, Local conflicts and Living frontier. Square locates an incident. Cross delivers aid or searches a site when nearby. Escort and combat incidents resolve through world actions. Analog remains movement; D-pad Up toggles detailed status, Down opens Skills, Left equipment, Right squad management.

## Install and saves

Extract PSP from Dustbound-0.8.0-PSP.zip to the memory-stick root, or replace PSP/GAME/DUSTBOUND/EBOOT.PBP. Keep your saves. Older supported formats through v7 migrate; new saves use v8. Older executables cannot read v8 saves.

## Verification and limits

Focused PPSSPP controller playthrough and completed-EBOOT framebuffer screenshots; native event/regression checks with AddressSanitizer and UndefinedBehaviorSanitizer; core and expansion regression checks; package validation. Test scenes use prepared saves. No full natural campaign or physical PSP performance qualification is claimed. Large-group routing and long-session variety remain areas for further playtesting. See docs/DESIGN_REVIEW_V080.md.
