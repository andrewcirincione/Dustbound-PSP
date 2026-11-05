# Dustbound 0.5.0 — a Kenshi player's design review

## Verdict

The previous build had many working systems, but too few reasons to care about the people or places using them. A merchant, recruit and hired blade should represent different relationships, costs and risks. Random clothing, repeated news and broadly interchangeable equipment markets undermined that distinction. More item variants alone would not solve it.

This iteration makes a concrete improvement: recognizable professions and faction equipment, regional weapon markets, four new heavy weapon silhouettes, strength-dependent handling, and independent paid protection. There is now a useful early decision between buying supplies, recruiting a permanent companion, or paying for a temporary escort. It is still an alpha with much less systemic depth than Kenshi. The biggest remaining gap is a world that remembers specific people and consequential events.

## Evidence and method

I replayed the released 0.4.1 EBOOT, reviewed its actual 480×272 captures, inspected the simulation and UI, and then exercised the revised PSP executable through controller input in PPSSPP. The new release has separate runs for a fresh character, a fresh intertown journey, interiors, a prepared combat/outpost scenario, and the new NPC/weapon/save features. Native regression tests cover additional state combinations. Prepared saves are identified as fixtures, not evidence of a naturally earned playthrough. Test automation reads telemetry but does not write game memory.

This is a broad design and implementation audit, not a claim to have personally exhausted every possible expedition. Long campaigns, all reputation combinations and physical PSP performance remain unqualified. See [VALIDATION.md](VALIDATION.md) for executable hashes and the precise test scope; `evidence/v050-*` contains captures and logs.

I also checked the Kenshi wiki's Heavy Weapons, Strength, Mercenary Guild, Mercenary Captain and World States pages. They reinforce three design priorities: equipment should impose meaningful training/injury tradeoffs; temporary allies should differ from recruits; and world changes should have intelligible causes. These are reference principles, not a promise to reproduce every Kenshi rule exactly.

## Changes made from this critique

| Problem | Change in 0.5.0 | What remains limited |
|---|---|---|
| No unmistakable greatsword; creative weapons were hard to discover | Greatsword, railway maul, crescent glaive and anchor cleaver, each with original painted art and different weight, reach, damage/effect profile | Four new authored families, not hundreds of new drawings |
| Weapon choice favored raw damage too easily | Melee handling checks strength against twice weapon weight; injured arms reduce effective strength; weak handling increases windup/recovery | No individual weapon-category skills or manual directional attacks |
| The comparison did not explain heavy-weapon difficulty | Candidate strength requirement and current delay multiplier appear beside WORN / TRY ON statistics | Small 480×272 typography; effects remain abbreviated |
| NPC profession was hard to read | Context prompt includes profession and faction; role/faction outfits, dialogue and practical advice | Characters still use the shared articulated sprite rig |
| Services were physically ambiguous | Medics moved away from the overlapping merchant position to a separate recovery-side location | No full service-sign or town directory system |
| Every resident felt like the same service terminal | Merchants trade; medics heal; smiths forge; innkeepers rent beds; marshals offer work/bounties; recruits join permanently; contract guards sell temporary protection | Most dialogue is short functional exposition |
| No actual mercenary relationship | Four hireable guards in frontier hubs; escort or camp duty for one simulated day, saved expiry, dismissal and visible contract time | Individual guards, not full mercenary squads; no renewal negotiation |
| All NPCs behaved similarly in danger | Guards fight and return to posts; hired guards follow/watch camp and provide nearby first aid; merchants and medics flee threats | No indoor evacuation, schedules, morale or coordinated formations |
| Factions looked like palette labels | Faction weapons/armor, selected-faction descriptions and regional stock pools | Law, religion and political ideology are still shallow |
| Random markets gave little reason to travel | Compact greatswords/rifles, Rust mauls/work gear, Shogunate katanas/glaives, monk medicine/staves, outlaw pistols/cleavers | Restock is still a timed abstraction, not a supply-chain economy |
| New content could invalidate an expedition | Appended item IDs; v5 saves with explicit v2–v4 stock-layout migration; genuine v0.4.1 save tested | Migration refreshes NPC outfits/service positions and regional markets once |

The four new families add 96 material/grade variants. Total modular gear is **2,112**, across 88 designs. There are **36 weapon families**. Variant counts are content combinations, not proof of distinct playstyles.

## Whole-game critique

Priority means player impact: **P0** blocks understanding or trust; **P1** limits the central survival sandbox; **P2** adds breadth/polish. “Observed” refers to controller play or rendered captures; “audit” refers to source/tests rather than a complete natural campaign.

| Area | Assessment and evidence | Next useful improvement | Priority / current outcome |
|---|---|---|---|
| First five minutes | The action prompt, role label and reach indicator help; no strong first expedition objective teaches the whole loop. Fresh-start run covers mining, recruitment, trade and saving. | Offer a modest optional route: provision, reach a nearby settlement, return with a trade good. Explain danger before sending a weak drifter out. | P1; roles improved, onboarding still open |
| NPC readability | Previously a medic and trader occupied almost the same point; the new playtest selected the medic when trying to shop. | Keep distinct approaches and professional silhouettes; add unobtrusive service signs. | P0; overlap fixed and regression checked |
| NPC identity | Profession and faction now affect outfits, speech, services and combat behavior. They still lack personal relationships and schedules. | A few named residents with persistent problems and responses to the player's actions. | P1; functional distinction shipped |
| Recruitment | Permanent control, equipment access and squad slots make recruits fundamentally different from mercenaries. Price still dominates personality. | Origin-specific recruit conditions, loyalty events and short backstories. | P2; distinction clarified |
| Mercenaries | Escort payment, independent AI, first aid and expiry create a real provision-versus-protection decision. | A captain with a small cohesive company, renewal, failure terms and faction consequences. | P1; individual contracts shipped |
| Faction geography | Regional equipment now gives practical destinations; ledger explains specialties. | Distinct architecture clusters, border checks, scarce resources and local attitudes. | P1; equipment/stock shipped |
| Faction law | Reputation, theft, bounty and custody operate, but political identity is mostly presentation. | Different offenses, contraband rules, tolerated allies and enforcement priorities. | P1; open |
| World states | Ownership/unrest can change through timers and prosperity, not primarily through named leader death/capture. Audit: `world.c`. | A small explicit leader-state graph with visible town changes, retaliation and journal cause/effect. | P1; major missing system |
| Economy | Stock and prices exist; daily restocks are generated, not delivered by functioning caravans. | Tie a few goods to producers and visible convoys, with recoverable shortages and bounded price changes. | P1; regional demand/supply flavor improved |
| Wilderness scale | Towns are genuinely separated; the fresh journey tests the route with enemies active. Space alone does not create interesting travel. | Authored landmark chains, camps, salvage sites, alternate crossings and visible danger clues. | P1; route works, density still insufficient |
| Encounters | Hostiles between towns provide danger. Audit shows limited motive/formation variation. | Distinguish hungry robbers, territorial beasts, patrols and scavengers; allow warning, extortion, retreat and avoidance. | P1; open |
| Exploration reward | Ruins/resources and regional gear provide incentives, but discoveries lack enough unique identity. | Named relics with location-specific leads and modest mechanical tradeoffs, not just higher numbers. | P1; open |
| Atlas | Local/world views, party location, known settlements and travel destination are useful. Controller journey confirms browsing preserves the route. | Player pins, service legends, danger notes and more clearly communicated unexplored areas. | P2; functional baseline retained |
| Weapon repertoire | Existing blade/polearm/blunt/ranged families were broader than they looked. Greatsword, maul, glaive and anchor cleaver now add clear silhouettes. | More distinct move sets before adding another large numeric catalog. | P1; four families shipped |
| Combat weight | Windup, contact, recovery, stagger, hit stop and rigid grip are present. Strength/injury handling makes heavy steel less universally optimal. | Better foot placement, readable enemy tells, directional guard/retreat decisions and controlled multi-target sweeps. | P1; handling improved |
| Animation/art | Painted weapons match the characters better than flat geometry. All 36 families pass pose/cache bounds and grip checks. | Locomotion blending, beast variety and less stiff body turns; assess at actual PSP size, not enlarged marketing art. | P2; new art integrated |
| Tactical control | Autoattack, pause, follow/hold and selecting party members work; intervention is limited. | Explicit passive/block/ranged priorities and dependable disengagement orders. | P1; open |
| Ranged combat | Ammo and ranged modifiers exist. No fully simulated projectile interception/friendly fire. | Visible travel, obstruction and careful ally-hit rules within a fixed projectile pool. | P2; missing depth |
| Injury and recovery | Six regions, bleed/blood, unconsciousness, loss/prosthetics, first aid and beds exist. Prepared encounter covers wounds. | Better incapacitation forecast, clearer recovery state, dedicated robotic repairs and a separate abdomen. | P1; mercenary aid added |
| Defeat as a story | Rescue/carrying/custody can preserve an expedition; death can still end a run abruptly. | Enemies with selective looting, abandonment/capture behavior and better rescue opportunities. | P1; major fun opportunity |
| Survival | Food, fatigue, exposure, shelter and healing provide costs. Repetition risks becoming maintenance without travel choices. | Routes with distinct water/shelter opportunities and readable preparation advice. | P1; open |
| Inventory | Six slots, preview comparison, deltas, effects and weight work. New requirement display improves weapon evaluation. | Sorting, stack actions, stronger effect explanations and clearer tradeoff summaries; grid inventory only if it earns its screen space. | P1; handling display shipped |
| Skill progression | Twenty use-trained skills are present. Heavy melee now trains strength through resolved strikes. | Explain XP sources and soft difficulty scaling; add category mastery only when it creates choices. | P2; strength loop improved |
| Crime | Witnessed theft, stolen ownership, fences, lockpicking, bounty and custody exist. | Trespass zones, disguise recognition, kidnapping consequences and differentiated local laws. | P1; systems absent/limited |
| Slavery/captivity | Custody is not a slavery simulation. No slave labor, shackles economy, liberation networks or ex-slave recognition. | A deliberately scoped captivity/escape loop with persistent recognition, only after rescue/AI reliability. | P1; absent |
| Building interiors | Walkable single-room services and indoor saving work in EBOOT. NPCs do not live full indoor schedules; combat stays outside. | Believable resident presence, safe transitions and persistent containers before multi-floor expansion. | P1; baseline retained |
| Outposts | Building, farming, crafting, depots and jobs exist. Construction/logistics are compressed. | Material delivery, construction labor, queued priorities, blocked-job reasons and meaningful visitors/raids. | P1; camp guard added, deeper logistics open |
| Research/power | Limited tiers and crop support are not a connected power grid or a deep technology economy. | A small coherent production chain, generator/load feedback and artifact-gated discoveries. | P2; open |
| Squad structure | One controlled party of up to eight; paid guards remain independent. | Two independently ordered groups with clear offscreen simulation and alerts. | P1; independent player squads absent |
| Animals | Hostile beasts exist, but no recruited, aging, carrying pack-animal system. | One useful pack species with readable needs and bounded AI before many species. | P2; absent |
| Dialogue/quests | Functional contracts and advice are useful but repetitive. | Contextual barks about injuries, regional history and completed deeds; small branching encounters. | P1; role advice shipped |
| Save trust | Version/checksum/size validation and backups exist. Expanded catalog required explicit migration rather than a raw struct copy. | Corruption recovery UX and long-session hardware save qualification. | P0; v1–v5 coverage and genuine v4 fixture |
| PSP usability/performance | Software renderer, compiled palettes and bounded caches avoid external decoding. Emulator correctness is not hardware qualification. | Real PSP-3000 frame-time, thermal/battery, memory-stick and crowded-town tests. | P0 before calling hardware-ready |
| Long-term purpose | Legacy goals exist, but player-authored stories need stronger simulation consequences. | Let one rescued person, destroyed raider camp or deposed leader change a later expedition. | P1; principal remaining design gap |

## Weapon decisions now available

| Weapon | Intended choice | Cost / limitation |
|---|---|---|
| Greatsword | Long, high-damage balanced blade; Compact specialty | Heavy and slow for an untrained or injured survivor |
| Railway maul | Armor penetration and stagger; Rust salvage identity | Shorter reach than the greatsword/glaive, very heavy |
| Crescent glaive | Longest reach of the new four; piercing Shogunate polearm | Less raw damage; still demands healthy arms |
| Anchor cleaver | Brutal bleeding weapon associated with raiders/outlaws | Heaviest new option with a large recovery commitment |

Materials and grades modify these profiles. A strong candidate on paper can be a poor weapon for the current survivor. The requirement is a handling penalty, not a hard equip lock. Existing ranged families do not use this melee-strength penalty. The game does not yet forbid every two-handed animation when an arm is unusable; injury slows handling, and the existing missing-right-arm fallback remains. This is an explicit remaining fidelity issue.

## Mercenary rules and failure cases

Contract guards stand in Dusk Haven, Copper Cross, Red Gulch and Black Mesa. An escort costs 240 silver; camp duty costs 180 and requires an existing outpost. Each purchases one simulated day, equivalent to 30 minutes of unpaused simulation. Menus/pause do not consume the contract. Up to four contracts can be recorded.

These are autonomous allied NPCs, not extra controllable squad slots. Their gear remains theirs. Escorts follow the selected survivor; camp guards head toward the outpost. Both use ordinary guard combat AI. They stop nearby allies' bleeding with their own finite medical supplies outside combat. They wait outside interiors, can be injured or killed, and return home after expiry/dismissal. Dismissal has no refund. Repeated hiring while already under contract takes no additional money. Contract identity, mode and absolute expiry persist in saves.

This gives a solo traveler a practical safety purchase, but not guaranteed survival. It is not yet a full mercenary-company simulation. Individual guards do not negotiate, coordinate formations, renew themselves, or escort characters into buildings.

## Iteration findings caught during this pass

1. The first anchor-cleaver grip coordinate landed outside the handle. The opaque-pivot regression rejected it; the calibrated anchor was corrected before packaging.
2. The first strength hint conflicted with the comparison layout. It now shares the bottom statistics line, with shorter effect text on the left. The full pose/preview suite was rerun.
3. A real merchant playtest opened the medic instead. Their old positions were half a world unit apart. Medics now stand at a separate recovery-side location; native checks require walkable positions and separation from merchants.
4. The controller harness occasionally advanced dialogue twice. Dialogue navigation now converges on the intended row, like the existing pack/menu driver. This was a test-input issue, distinct from the overlapping-NPC game defect.
5. The weapon filter initially displayed the same shared starter goods before regional equipment. Regional designs now lead that tab, so the differences are visible on opening the market.
6. Save review caught an unsafe validation order: contract actor references must be checked after actor-count bounds. This was corrected before the final emulator runs.

7. The expanded faction guide reached the footer on the PSP capture. Its wording was shortened while retaining contract prices, duration, behavior and faction specialties.

8. A defeated raider inherited the generic resident role label. Raiders and beasts now retain their correct identity in the contextual prompt, including while being looted.

## What would make the next iteration more fun

The strongest next milestone is one deliberately authored regional loop, not another catalog expansion. Give Copper Cross a failing salvage route, a named raider leader and a stranded specialist. Let the player hire protection, discover alternate paths, rescue the specialist, or remove the leader. Persist the consequences: a safer road, changed stock, a thankful recruit and a later retaliation. Each outcome should be legible in dialogue, the map and the town itself.

Build that from three bounded systems: a small leader/world-state table, encounter groups with motives, and event-aware dialogue. Keep the PSP pools fixed. Acceptance should require two different controller-played solutions, save/reload at each consequence, and a measurable town/route change. That would test whether the sandbox produces stories rather than merely contains menus.

After that, deepen defeat/recovery and outpost logistics, then independent squads. More biomes, weapons and architectural variants should support those loops. Physical PSP-3000 qualification should run alongside the next milestone, not be inferred from desktop render timings.

## Upgrade notes

0.5.0 writes save format 5. Existing item IDs are preserved, and v1–v4 saves load through migration. A first migrated load assigns faction/profession outfits to surviving non-squad town NPCs, moves medics, introduces available contract guards and refreshes regional stocks. Player equipment, skills, coordinates, structures and indoor position are preserved. Save to a new slot first: older executables cannot read v5 saves.

## References

- [Kenshi: Heavy Weapons](https://kenshi.fandom.com/wiki/Heavy_Weapons)
- [Kenshi: Strength](https://kenshi.fandom.com/wiki/Strength)
- [Kenshi: Mercenary Guild](https://kenshi.fandom.com/wiki/Mercenary_Guild)
- [Kenshi: Mercenary Captain](https://kenshi.fandom.com/wiki/Mercenary_Captain)
- [Kenshi: World States](https://kenshi.fandom.com/wiki/World_States)
- [Existing mechanics matrix](MECHANICS_AUDIT.md)

References were checked on 2026-11-05 through indexed wiki page content. The game's code and gameplay captures, not wiki descriptions, establish what Dustbound actually implements.
