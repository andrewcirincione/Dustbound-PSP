# 0.14.0 — Read the frontier

Map Cross sets a manual reference pin. It does not clear work or move the player. Analog walking retains the pin. The former R+Cross group route command has been removed; saved group travel flags clear on load and simulation. Squad following and assigned jobs still work.

Threat is conveyed by actual equipment and movement. Low-tier raiders and starving raider/ambush groups carry low-quality weapons, with bare heads/feet, rags or bare torsos and no backpack. Bare torsos no longer render as full coats. Patrols and armies wear matching sets, with regular melee/rifle roles, leader banners and a three-wide formation. Duelists have full superior head, torso, leg, foot and back equipment. These are real items, so combat and loot use the same loadout. Existing saved equipment is preserved.

Up to eight nearby NPC labels are drawn within nine world units, prioritized by distance and spaced to avoid overlap. They show a singular role, faction-colored text/swatch, and a red ! for current hostility. Down/dead states are qualitative. Concealed ambushers do not receive labels or nearby actor actions. NPC health bars and encounter tier readouts are removed. The faction screen pairs matching color swatches with reputation and wanted standing.

The activity ledger stores last-seen group identity, observed position, faction, behavior and time. It refreshes only from a living visible member in the selected character's room and within fourteen world units. A distant survivor group cannot be tracked by standing beside a corpse. Reports survive save/load and can open a map at that historical location. They are not live tracking or movement commands.

Explicit bed/counter/workstation actions take priority over nearby NPC dialogue. The innkeeper's option is now guidance to beds, not a misleading rent transaction. Selecting a public bed charges 25 silver and rests. Squad members and current combatants receive a late silhouette pass so foreground grass cannot completely hide them.

Councils identify both claimants, compare their influence and preview the selected side's existing policy. A winning claimant's name is applied to an eligible surviving steward. Food relief and silver pledges still spend once; this adds clarity to the existing succession model rather than a new quest campaign.

Successful autosaves are silent. Routine skill and weapon-class gains remain in Recent messages and only enter the live feed when it is clear. Contract payment is prioritized.

Save v14 appends 5,120 bytes of sighting snapshots to the unchanged v13 prefix. Game size is 2,148,348 bytes. The actual v13 inn save that reproduced the service bug is included as a compressed migration fixture. New sightings begin as groups are observed after upgrading.
