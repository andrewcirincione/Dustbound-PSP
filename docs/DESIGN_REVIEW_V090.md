# Critical review — 0.9.0

## Changes made after inspection

- The atlas exposed encounters before exploration. Both map views and the journal no longer reveal a global incident list. The town browser remains.
- An unconditional action panel wasted space. The new drawer grows only for nearby choices and stays open while queuing; resume closes it and starts work.
- A spare character model made inventory comparison confusing. One preview now uses the selected survivor's appearance, with explicit worn/preview disguise labels.
- Freeing the two original chain-yard captives left them standing at the yard unless recruited. Rescued NPCs now choose independent following or flight to refuge, with an explicit dismissal for followers, and ordinary jail prisoners use the same timed rescue actions. Recruitment remains an explicit choice.
- A generic bounty could punish a survivor in unrelated factions. Crimes and payments now operate on faction ledgers.
- The old combat XP loop trained every weapon through general melee only. It now has persistent class proficiency, stronger-opponent scaling, and correct hit/block/dodge training distinctions.
- Ordinary caravan residents had been converted into smiths, leaving escort jobs with no employer. Paid caravans now receive real, bounded employer actors. Distant active employers continue moving rather than freezing beyond the local AI radius.
- Final PSP captures showed the cooking ground was hidden by a building. Its location now sits in the open so the captive and rescue approach are visible.
- Captors initially resumed ordinary patrol after binding a victim. Cannibal guards now remain at the cooking ground while a bound victim is alive.
- Temporary combat stun could make a person eligible to carry. Carry actions now require death or unconsciousness and reject shackled targets.
- The header log initially clipped the custom atlas title. Header positions now leave it fully replaceable by one warning line.

## Remaining critiques

1. Encounter combinations renew indefinitely, but there are eight underlying incident mechanics. More distinct consequences and faction stories would add depth beyond new combinations.
2. Uniform checks and inspection suspicion support infiltration, but disguises lack conversations, documents, social rank and faction-specific schedules.
3. There are profitable routes and actual delivery/service requirements, but only one goods shipment per freight contract and four concurrent service jobs. Employer navigation can still be obstructed by terrain or fighting; the expiry rule bounds abandoned jobs.
4. Gear variety is primarily material, quality, effects and silhouette. Variants share art; additional handcrafted models and enemy-specific reward stories would make loot more distinctive.
5. Town simulation updates production, consumption, shortages and conflict. It does not simulate a full resident schedule or individual offscreen economic agents.
6. This pass verifies specific mechanics and UI states, not a full natural campaign. Progression pacing, long trade loops and low-memory hardware performance need extended play.

Screenshots in `evidence/v090-psp-*.png` are captures of the actual PSP executable. Native previews are separate. The most useful player feedback is on inventory readability at PSP size, the action drawer, map browsing and whether the top log stays unobtrusive during play.
