# Dustbound 0.6.0 — critical rebuild review

## Assessment

The strongest criticism of 0.5.2 was that its large equipment catalog and map did not create a memorable reason to travel. The Copper Road now connects injury, escorting, salvage, crafting, negotiation, combat, capture and town trade into one persistent conflict. It is a concrete expedition, not a claim of Kenshi-scale simulation. Its locations and branch rewards are authored; most of the wider world still uses the older systemic contracts and encounters.

The rebuild starts from released 0.5.2. The unpublished earlier candidate was lost. Only the reconstructed source, new checks and exact EBOOT identified in VALIDATION.md qualify this release.

## Findings and responses

| Area | Critique | Rebuild response / remaining limit |
|---|---|---|
| Motivation | A map and many items do not establish a journey | Sora's rescue leads to the Copper Road conflict, Nell's report and lasting consequences |
| Exploration | Empty distances have few distinct destinations | Eight named landmarks, caches and atlas leads; wider wilderness still needs authored encounters |
| Choice | Similar dialogue produces similar outcomes | Repair, negotiated toll, living capture and lethal resolution use different requirements and rewards |
| Consequences | Town control changes randomly, disconnected from play | Copper ownership, prices, supplies, prosperity, dialogue and patrol allegiance follow the selected route; other towns retain event logic |
| Escort | A dialogue flag could pretend an escort happened | Sora physically follows within range and must reach Copper alive; controller test travels the complete route |
| Rescue | Wounded characters are chiefly loot targets | Treatment, carried rescue, shackles and recruiting freed captives add recovery paths |
| Defeat | Death or reload offers little recovery play | Nearby hostile captors carry unconscious squad members to the yard; escape takes time and guards interrupt |
| Recognition | Escape warning had no corresponding behavior | Former captors recognize escapees for a simulated day; other identity/disguise systems remain absent |
| Leader permanence | Camp raids could recycle a dead named NPC | Named characters and carried/contracted bodies are excluded; ordinary recycled actors have their state reset |
| Captivity depth | A timer is not a slavery economy | Physical capture and liberation are implemented; forced labor, slave trading and multi-settlement pursuit are not |
| Tactics | A single automatic combat toggle is restrictive | Balanced, defensive, keep-range and hold-fire orders; three groups have separate leaders and routes |
| Ranged combat | Instant hit resolution ignores intervening actors | Bounded moving projectiles collide with structures and actors, including allies; water no longer acts as a wall |
| Indoor combat | Interiors previously isolated combat | Real residents/followers and same-room combat; looting and tactics use room positions |
| Trespass | Omniscient alarms punish unseen actions | A conscious same-faction occupant must witness private-room trespass; warning precedes bounty |
| Residents | Every room used a visual stand-in | Existing appropriate residents enter; no full home/work/sleep schedule simulation is claimed |
| Commerce | Always-open counters weaken time decisions | Trade hours 06:00–22:00; role-specific services remain separate |
| Wounds | Medicine and sleeping blur different injuries | Damage creates a trauma ceiling; organic rest reduces trauma, bandaging does not instantly erase it |
| Mechanical bodies | Dressings effectively repair metal | Mechanical bodies/limbs require parts; no organic food/blood drain for mechanical ancestry |
| Armor | Torso protection acts like a universal stat | Separate body, head, arms, leg and boot coverage; cut/blunt responses and armor upkeep |
| Layering | No shirt beneath armor | Light body items form an ownership-preserving underlayer; exposed underlayers render on the world actor |
| Gear comparisons | Variant totals can obscure useful choices | Existing worn/candidate load, speed, stealth and effect comparisons retained; underlayer panel compares coverage/protection |
| Presentation | Hats/packs disappear on fallen characters | Rotate the complete dressed rig for prone characters; weapon grips and fitted head/back anchors retained |
| World labels | Landmark text overlaps the selected NPC role | Only the targeted landmark receives an overhead name; small ground markers remain |
| Controls | World buttons still appear beneath modal menus | Overlay footer states that time is paused; actionable icon instructions belong to the open panel |
| Construction | Structures appear instantly | Material validation followed by interruptible labor; unfinished stations cannot produce or attract workers |
| Production | Fire and forge output costs no energy | Timber/coal fuel charges, input fetching, station travel and output storage; no electrical grid is claimed |
| Jobs | One fixed job cannot express fallback priorities | Four-entry ordered queue, readiness checks, pause/resume and explicit selected group |
| Outposts | More structures alone do not create a siege game | Work, fuel and condition provide upkeep; destructible siege tactics and a full power network remain open |
| Map | Groups cannot embark independently | Three saved leaders/routes, selected-group routing on the atlas; no recruited animal system |
| Saves | New state could break released expeditions | Append v6 state after the stable v5 prefix, validate references/floats, retain v1–v5 readers; full actor pools are never overwritten to insert story NPCs |
| Evidence | A native screenshot is not a PSP playtest | Controller input drives the actual EBOOT, which records its completed frame; fixtures are explicitly identified |
| Performance | Desktop speed does not prove PSP readiness | Fixed actor/projectile pools, software-renderer budgets and host profiling; physical PSP-3000 timing remains unqualified |

## What is still weak

The frontier needs additional conflicts of Copper Road's density. Building interiors share a small set of layouts; civilians do not have independent daily schedules. Faction disguises, animal recruitment, a broader captivity economy, destructive sieges and electrical production are absent. Offscreen simulation is bounded and simplified. The catalog contains 88 base designs with material/grade variants, not thousands of unique painted designs.

Balance still needs long natural campaigns with player decisions. Seven simulated days with isolated threats checks state stability, not whether an ordinary survivor can prosper for a week. Prepared test saves exercise difficult transitions but do not demonstrate an organically earned endgame. The controller record separately includes fresh starts and a wilderness journey with enemies enabled.

## Reference check

Compared the design against indexed Kenshi wiki entries for [World States](https://kenshi.fandom.com/wiki/World_States), [Slavery](https://kenshi.fandom.com/wiki/Slavery), [Armour](https://kenshi.fandom.com/wiki/Armour) and the existing [mechanics audit](MECHANICS_AUDIT.md). This does not claim a complete page-by-page wiki audit or parity with Kenshi. The game's original setting, assets and simplified rules remain its own.
