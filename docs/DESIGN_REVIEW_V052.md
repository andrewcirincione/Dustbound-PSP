# Dustbound 0.5.2 — critical UI and gameplay review

## Verdict

The alpha was asking the player to trust actions that were not consistent. A save cancellation could return to the wrong context; an item operation could act on a different stack; a job could appear assigned without collecting the ingredients it needed. Those failures undermine a survival game more than another hundred equipment variants would help it.

This pass repairs those foundations and makes recovery, production, equipment choices and skill progression more legible. It does **not** establish full Kenshi parity. The biggest remaining design problem is that a long journey seldom changes a place in a memorable, authored way. The existing contract/prosperity system is a modest systemic loop, not a substitute for leader-driven world states or a diplomacy campaign.

## Method and evidence boundaries

- Reviewed all 27 screen types, including two new screens and the legacy recruitment screen. Native contact sheets cover their layout; framebuffer tests exercise nine terrain settings, all guide chapters and interiors. Legacy recruitment is reviewed through the renderer because ordinary recruitment now uses role dialogue.
- Exercised fresh-start play and prepared recovery, logistics, combat and equipment situations through **PSP controller input in PPSSPP**, reading the EBOOT's own completed framebuffer. Prepared saves are explicitly fixtures, not naturally earned expeditions.
- Tested negative paths: insufficient reach, wrong service, full pack, separate stolen/clean stacks, cancelled help/save/load flows, living loot target, unavailable job inputs, blocked map destination, and paused custody.
- Replayed identical native combat rolls to check toughness damage reduction and perception accuracy. Checked complete worker trips, output storage and save/reload rather than only calling the crafting function.
- Rechecked indexed [Kenshi Jobs](https://kenshi.fandom.com/wiki/Jobs) and [Armour](https://kenshi.fandom.com/wiki/Armour) entries. Direct wiki bodies failed retrieval in this environment. The broader [mechanics audit](MECHANICS_AUDIT.md) records remaining differences. This is a design comparison, not a claim to have copied every rule or read every wiki page.
- Exact binary identity, successful runs, interrupted attempts and hardware limits belong in [VALIDATION.md](VALIDATION.md). A source audit or a native screenshot is not labeled a controller playtest.

## Findings and implemented changes

| Area | Critical finding | Change and verification |
|---|---|---|
| Help navigation | Reading instructions reset the item/contract/menu selection | Restore originating screen, selection and scroll exactly; native and PSP round trips |
| Continue | Cancelling the title's load ledger opened the expedition menu | Return to the title's Continue row; PSP cancellation test |
| Confirmation | Cancelling a slot confirmation lost the ledger location | Return to the selected save slot; show its name/day in confirmation |
| Save and title | Could overwrite an implicitly selected slot immediately | Open the save ledger, choose a slot, then explicitly confirm; return to title only after successful write |
| Hidden controls | Left/right in the expedition menu silently changed save slot | Remove the unrelated slot mutation |
| Inventory ownership | Clean and stolen copies merged; selected operations removed the first matching item | Ownership-aware stacks and exact-slot removal for use, sale, pass and equip |
| Equipment swaps | Returning worn gear could overwrite ownership flags on every matching item | Preserve equipped ownership and return only that item; failed swap remains atomic |
| Craft rollback | A full-pack failure restored quantities but not ownership metadata | Restore the complete actor transaction snapshot |
| Manufactured goods | Consuming stolen materials could produce clean goods | Output keeps the first consumed stolen faction tag; merely carrying stolen material does not taint a clean recipe |
| Empty ranged weapon | Moving a stolen unloaded weapon to the pack lost its tag | Keep ownership when switching to unarmed |
| Passing cargo | Could pass through an interior boundary and mix provenance | Require the same room/exterior and three-meter proximity; transfer the selected stack's tag |
| Selling | Hostile markets refused purchases but still accepted sales | Apply faction trade refusal consistently to selling |
| Loot | No worn/candidate comparison, one item at a time, stale equipped tags | Add portraits/stat comparison and full-stack take; clear the source's ownership tag correctly |
| Loot access | Loot actions relied on the menu having opened legitimately | Revalidate consciousness, fallen target, room and reach for each transfer |
| Crime | Looting a neutral fallen NPC always created an omniscient bounty | Mark the goods; add bounty/reputation loss when a conscious same-faction witness is nearby |
| Recovery | The interact button decided between bandaging and carrying | Explicit First Aid, Carry, Recover Gear and Take Control choices for squad members |
| Nearby targets | Out-of-reach resources could mask a usable service; crowded targets could not be selected | Prefer actionable targets; hold the left shoulder and tap the interaction button to cycle nearby choices; release alone still cycles squad |
| Mining | Full miners kept attempting work with no useful result | Check inventory capacity before starting another mining task; expose the blockage in squad status |
| Stations | Crafting/forging/research accepted five-to-six-meter distances | Enforce 1.7-meter exterior reach and the actual indoor workbench context |
| Depots | Goods moved from six meters away | Enforce exterior reach or the active indoor depot prop; full-stack transfer stops safely when capacity is exhausted |
| Sleeping | Menu shortcut could start sleeping from twelve meters away | Require a bed within reach or the active indoor bed; the inn's existing bedside service remains available |
| Specialists | Menu shortcuts bypassed the barber and repaired unrelated faction reputations | Appearance requires a nearby barber; bounty shortcut opens a nearby marshal's actual dialogue |
| Camp workers | Cooks fetched grain but not water, and production did not reliably walk through a supply loop | Fetch missing inputs, walk to the station, craft, return output to a nearby depot; cooks retain two rations |
| Work under fire | Production could continue while engaged; passive sleepers could keep resting after a hit | Suspend production when engaged; a landed hit cancels resting/gathering |
| Medics | Automatic aid only noticed chest wounds and bleeding | Include seriously injured surviving limbs; move into treatment reach; do not treat across room boundaries |
| Job feedback | Assigned jobs had no explanation when idle | Display current work, supply, station, injury, custody and capacity status |
| Crafting UI | Costs duplicated recipe data and omitted owned quantities/output | Derive held/needed counts, output, tier and station readiness from the recipe implementation; show grain and meat alternatives |
| Forging UI | Material requirements lacked held amounts; special effects showed a generic potency | Show held/needed components, tier and smithing; show the actual item effect value |
| Market UI | Visual comparison was restricted to backpacks | WORN/TRY ON for weapon and armor filters as well |
| Pack UI | Stolen provenance and specialized pack effects were hard to discover | Show the stolen faction and medicine/recovery pack effect alongside load/combat/stealth comparisons |
| Health UI | Vital text ran toward the panel edge | Separate blood/bleed, hunger/fatigue/dressings and consciousness into readable rows |
| Skills UI | Selection was invisible; bars showed total level, not progress | Highlight the selected skill, show progress to the next level and explain training/effect |
| Skill usefulness | Several level counters had little or no live effect | Add bounded toughness, perception, farming, cooking, engineering and science effects; see balance table below |
| Journal | Supply progress row lagged the held/needed description | Show current selected-survivor supplies directly |
| Map orders | Blocked land/water targets or indoor orders could silently fail | Reject blocked destinations and invalid traveler states; stop stalled travel with a message after eight seconds without half a meter of progress |
| Custody | The restricted-input branch advanced time despite tactical pause | Respect pause and simulation speed while imprisoned |
| Feedback history | Short notices vanished before the player could interpret them | Recent Messages screen retains the last six session notices |
| Layout polish | New ownership row overlapped the list; unsupported separator glyphs appeared | Adjust spacing, use supported punctuation, inspect native sheets and actual EBOOT captures |

The crime model still stores one faction tag per stack. If a recipe consumes materials stolen from multiple factions, the first consumed stolen faction is retained. It does not model a provenance graph, individual private owners, or territorial warrant databases.

## Skill effects and balance intent

| Skill | Implemented effect in this revision | Reasonable bound |
|---|---|---|
| Toughness | 0.2% damage reduction per level before injury/hunger modifiers | At most 20%; does not erase the minimum-damage rule |
| Perception | +0.15 ranged attack rating per level | At most +15 rating; normal hit-chance cap still applies |
| Farming | One extra grain per 30 levels | Five base grain becomes at most eight |
| Cooking | One extra ration per batch per 35 levels | Two base rations becomes at most four; ingredients unchanged |
| Engineering | One timber saved per 30 levels | Construction always costs at least one timber; iron/technology/placement requirements remain |
| Science | 1% research silver reduction per five levels | At most 20%; still requires a relic and a bench |

Skill gains and equipment stats remain separate. Material/quality IDs and the 0.5.1 garment profiles remain stable. This is not a comprehensive economy calibration: long-term food abundance and high-level profits need extended campaign testing.

## Screen-by-screen audit

| Screen | Result / evidence |
|---|---|
| World HUD | Reach and current action remain visible; nearby cycling has a button-icon hint; fresh/road/outpost PSP scenes |
| Pack | Exact-slot operations, stolen owner, pack effect, current/candidate view; PSP ownership and gear captures |
| Skills | Selected skill, training explanation, within-level progress; PSP capture and native all-screen bounds |
| Body | Limb numbers plus separated vital rows; PSP wound/health captures |
| Squad | Assigned order plus actionable status; PSP cook assignment and Hold change |
| Atlas | Local/world views, preserved browsing route, invalid-order refusal; fresh travel and native negative paths |
| Trade | Supply prices and equipment visual comparison; role markets and fresh shopping |
| Legacy recruit | Renderer/bounds reviewed; normal recruitment uses the dialogue screen |
| Build | Actual engineering-adjusted timber cost and held materials; prepared outpost and native placement failures |
| Craft | Authoritative requirements and alternative ingredients; PSP camp recipe screen |
| Research | Held relic/silver, actual discount, station status, completion state; native research loop and visual review |
| Expedition menu | Stable 17 entries; message history added; hidden save-slot control removed |
| Guide | Twenty chapters; preserves previous screen/cursor; new controls/recovery/logistics explained |
| Title | Continue cancellation cannot bypass character creation; PSP round trip |
| Confirmation | Names the exact slot; cancel restores the ledger; PSP save-and-title flow |
| Creator | Existing name/origin/appearance flow retained; normal PSP fresh start |
| Name keyboard | Existing character entry/deletion/return flow retained; normal PSP fresh start |
| Dialogue | Role-specific actions retained; marshal/barber shortcuts honor proximity; fresh recruit/trader/guard and native role tests |
| Forge | Actual effects and held/needed materials; prepared PSP forge transaction |
| Journal | Current supply counts; locate/return behavior preserved; fresh contract journey and native checks |
| Factions | Regional identities, reputation and hired-guard expiry information retained; PSP frontier ledger |
| Storage | Both directions, one/full-stack transfer, reach and ownership protection; PSP produced-food withdrawal |
| Saves | Title/menu origin, selected slot, corrupt/incompatible handling and backups; PSP/native save checks |
| Worn gear | Ownership-safe unequip and actual effect value; equipment sweeps |
| Loot | Full-stack take, candidate preview, legal status, access revalidation; PSP fallen raider and companion scenes |
| Rescue (new) | Distinct treatment/carry/recover/control choices; actual PSP carry-before-bandage path |
| Recent messages (new) | Six session notices, wrapped within panel; PSP capture |

## Kenshi-player critique of the wider loop

| System | Current assessment | Remaining work |
|---|---|---|
| Starting vulnerability | Starting gear/food lets a new survivor learn, but prices and progression can become generous | Longer poor-start expeditions and tighter high-level economy |
| Exploration | Town distance and wilderness danger now matter; local/world map works | Authored landmarks, useful discoveries and multi-route objectives |
| Combat | Weight, strength, injured arms, reach, stagger and timing create differences | Stronger enemy tactics, projectile travel/friendly fire and richer defensive orders |
| Injuries | Six limbs/blood, knockout, rescue, loss and prosthetics create recovery choices | Stomach, separate bandaged damage, coma/critical recovery and robotic repair |
| Equipment | All 36 weapons, 44 garments and eight back designs have visible identities and tradeoffs | More silhouette variety; layered probabilistic cut/blunt coverage |
| Crime | Ownership, witnesses, fencing, bounties and custody are more internally consistent | Trespassing, disguises, kidnapping and regional warrants |
| NPC roles | Merchants, guards, mercenaries and specialists have different stock/services/behavior | Workday schedules, named relationships and more individual motives |
| Trade | Finite stock and faction/prosperity pricing support a basic loop | Convoy-driven supply, shortages and regional demand linked to player action |
| Contracts | Accept, locate, deliver and claim work; supply/defeat progress persists | Authored choices, alternate resolutions and distinct consequences |
| Outposts | Fetch/craft/store now forms a useful loop; farming, shelter, irrigation and raids exist | Ordered job queues, construction labor, power/fuel, defenses and material networks |
| Squads | Up to eight survivors, jobs, selection and independent hired guards | Multiple independently controlled expedition groups and animals |
| Faction world | Reputation, prosperity and town control change | Named leader death/capture, occupation states, retaliation and diplomacy |
| Interiors | Walkable rooms, services, collision, save position and weather shelter | Multilevel buildings, schedules, trespassing and indoor combat |
| Presentation | Painted equipment and environment remain cohesive; screenshots use the real renderer | Prone actors and creature motion remain simplified; wilderness repetition is visible |
| PSP qualification | Fixed pools, indexed art, bounded caches and real PSP compilation | Physical PSP-3000 frame times, memory-stick soak, suspend/resume, battery behavior |

## Next substantial design milestone

Build one persistent expedition around Copper Cross: an actual supply shortage, a stranded specialist, and a named raider controlling a route. Let preparation, escort hiring, an alternate crossing, rescue and confrontation lead to different outcomes. Returning later must reveal changed stock, dialogue and road safety. Validate two controller-played solutions and their saved consequences before adding more generic contracts or gear counts.

That milestone is still open. This release earns a more reliable foundation for it; it does not claim that the larger sandbox gaps have disappeared.
