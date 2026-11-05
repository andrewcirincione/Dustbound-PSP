# Dustbound 0.5.1 — equipment, expeditions and the remaining sandbox gaps

## Assessment

Dustbound has a playable survival loop, but a long feature list is not the same as Kenshi's depth. A Kenshi player should be choosing how to survive a trip, recognizing what an NPC represents, recovering from bad decisions, and seeing consequences later. This pass found two immediate failures of visual trust—low, reversed backpacks and cutting edges that did not mirror—and two failures of usability: backpack choices were buried in armor stock, and accepted work did not provide a direct way to locate its destination.

Those are corrected in this release. Four affordable backpack designs now have deliberate hauling, stealth, combat and recovery tradeoffs. The shop exposes them with worn/candidate portraits, and the comparison predicts the actual loaded result of a swap. Contracts can locate their current destination on the atlas. These changes improve decisions within the existing loop. They do not turn the alpha into a full Kenshi-equivalent simulation.

## What was examined

- Production sprite source, authored pivots, transforms, actor cache and actual 480×272 PSP captures. Native review sheets cover standing, crouching and walking back gear and both facings at guard, windup, contact and recovery for all weapon families.
- A fresh-start backpack shopping expedition: walk to a trader, buy three alternatives with ordinary starting money, wear all four pack designs, inspect differences, accept work, locate the destination, save and reload.
- Controller replays for normal play, interiors, intertown travel with enemies enabled, prepared combat/outpost scenarios, and faction/mercenary services. Prepared catalog saves exercise all 36 weapons, eight back designs and 44 clothing designs in both facings. They are explicitly test fixtures, not naturally acquired loot.
- Source/test review of the broader mechanics below. A row marked “audit” is not a claim that every possible campaign state was played through. See [VALIDATION.md](VALIDATION.md) for the final executable identity, assertions and interrupted-run disclosures.
- Kenshi wiki reference checks on backpacks, encumbrance, world states and slavery. Direct wiki fetches returned errors; indexed page content was available. Reference mechanics informed the critique; Dustbound's rules and tests establish its implemented behavior.

## Corrections made

| Finding | Result | Evidence |
|---|---|---|
| Pack bases reached toward the calves | Per-design shoulder/belt mounts; separate heights and widths for flags, scabbards, cape and mantle | 1,152 pack/body/torso/facing/pose raster checks; native before/after sheets; PSP movement captures |
| Pack straps projected away from the wearer | Reflect back artwork toward the torso and scale the mounting offset with body width | Both facings of all eight back designs in the EBOOT |
| Curved swords changed cutting side when turned | Authored edge-side metadata and reflection around the handle-to-tip axis | 288 whole-raster reflection comparisons and 5,760 grip samples across all 36 families |
| Left-facing arms reached from the wrong shoulder | Mirror shoulder anchors, elbow bend and hand artwork with facing; mirror coat follow-through | Production pose sweep and controller captures |
| Passing pivot tests had missed orientation errors | Add asymmetric reflection and upper-back visibility assertions, rather than only checking a point of contact | Dedicated attachment regression suite runs in GitHub CI |
| “More variants” hid a lack of practical choice | Four identifiable packs with fixed roles; all settlements stock two affordable examples of each daily | Stock/filter tests across all 16 towns; fresh purchases in PSP gameplay |
| Backpacks were mixed into armor listings | Dedicated BACKPACKS filter, clear current filter heading, WORN/TRY ON shop portraits | Direct EBOOT shopping captures |
| Carry numbers did not explain the expedition consequence | Compare effective loaded weight, extra allowance, empty weight, melee/defence penalty and stealth; account for the old bag returning to inventory | 192 actual equip swaps exactly match their load preview |
| Back gear could receive unrelated randomized perks | Fixed family roles: medicine aids first aid, bedroll aids recovery, cape protects from exposure; cargo packs carry loads | Native gameplay checks and catalog audit |
| Accepted work lacked an obvious next action | Journal map action locates the delivery destination, then the issuer once the delivery is complete; supply text reports held/needed items | Native tracking checks and fresh controller-played contract |
| Browsing a contract could accidentally become an order | Locating opens the atlas without changing the existing travel destination; travel still requires confirmation with the map action | Route-preservation regression |
| New stock could replenish on every reload | One-time market supplement for older v5 expeditions; sold-out stock stays sold out after subsequent loads | Controlled migration and genuine released-0.5.0 save fixture |
| New guide text overlapped the footer | Shortened and recaptured the complete backpack chapter | Final chapter fits above the controls at 480×272 |

## Clothing fit and balance

Reviewing the source art exposed a second anchor error: chin straps, cheek guards and crests were all being fitted into one generic hat box. Each of the twelve headwear designs now has a calibrated brow row and width, with its source aspect ratio retained. A short neck bridge joins the chin and collar. Native sheets inspect all 44 clothing families; 72 headwear/body/facing raster checks require brow overlap and bounded dimensions. The PSP catalog scenarios equip every clothing design through the inventory, capture its comparison, and show both facings.

The old armor formula used family index modulo eight. That made some cloth garments stronger than plate and assigned unrelated perks. Forty-four authored garment profiles now determine protection, weight, movement, stealth and role; material represents reinforcement/finish, while quality improves that role without adding mass. At worn Scrap grade, monk robes have armor 3, weight 1 and +1.5% movement; an iron cuirass has armor 30, weight 14, −6% movement and −18 stealth. The [complete same-grade balance table](../evidence/v051-clothing-balance.csv) supports comparison without mixing quality tiers.

The 1,056 clothing variants are tested for monotonic quality, consistent live bonuses, heavy/light tradeoffs and worn-only effects. Item effect values now report the actual carrying, weather, speed or stealth bonus instead of an unrelated generic potency number. Torso armor covers chest and arms, headwear covers head, and legs receive leg armor plus half boot armor. Coverage percentages, separate cut/blunt layers, an abdomen and independent shirt slots remain absent; this is a coherent simplified ruleset, not a full Kenshi armor simulation.

The balance change exposed a test dependency: the first crossbow shot could now leave its target stunned when the next empty-ammunition check ran. The test now clears that prior hit response before checking the independent ammo fallback; it does not weaken the expected fallback behavior.

## Backpack decisions

Values below are worn Scrap examples; higher quality adds carrying allowance to storage designs. Material/quality variants retain their original item IDs and artwork families.

| Design | Price | Empty weight | Extra allowance | Cargo weight counted | Melee/defence | Stealth | Practical role |
|---|---:|---:|---:|---:|---:|---:|---|
| Travel pack | 120 | 2 | +16 | 80% | −2 | −3 | Modest general travel load |
| Medicine roll | 180 | 1 | +10 | 85% | 0 | 0 | Mobile first-aid carrier; improves treatment |
| Ore harness | 230 | 6 | +36 | 60% | −7 | −15 | Hauling salvage with a clear fight/stealth cost |
| Bedroll pack | 200 | 4 | +24 | 70% | −4 | −8 | Longer expeditions; improved resting recovery |

Prices vary with prosperity and reputation. Cargo reduction applies to carried inventory only; worn equipment and a carried person retain full weight. An unworn bag provides no equipment bonuses. All designs use the existing 24-slot inventory: there is no separate nested bag grid, bag-specific stack capacity, or one-button drop-and-recover-loaded-bag system. A bedroll pack does not create a sleeping spot. The banner, sword rack, cape and mantle remain distinct back equipment and appear in rotating stock/forging.

In the fresh shopping test, buying the three alternatives uses 610 of the initial 650 silver. That is an availability test, not an advisable starting strategy. A more interesting expedition chooses one pack and preserves money for rations, treatment, recruitment or protection. The merchant now explicitly points out that preparation cost.

## Whole-game review

P0 affects trust or basic use; P1 affects the survival sandbox; P2 adds depth or polish. “Retained” means the existing system was kept, not newly implemented in 0.5.1.

| Area | Current critique | Action/status and next acceptance target |
|---|---|---|
| First expedition | Activities are available, but the first meaningful journey still needs a clearer voluntary goal | **P1, improved:** affordable pack choices and contract-to-map link. Next: one short local expedition with visible preparation and return reward |
| Available actions/reach | Context prompts and close mining checks work; crowded targets can still compete | **P0, retained/tested:** regression and normal controller run. Next: reliable target cycling with visible selection |
| Backpack availability | Four practical bags were obscured by rotating armor stock | **P0, fixed:** dedicated tab and guaranteed worn stock in every town; no need to grind for an accidental stock roll |
| Equipment tradeoffs | Capacity rose by family number; some perks did not fit the object | **P1, improved:** deliberate load/weight/stealth/combat/recovery roles, applied only when worn |
| Inventory organization | Comparisons are useful; flat inventory navigation remains laborious | **P1, open:** sorting, transfer quantities and clearer stolen ownership; test full-inventory swaps without losing items |
| Attachment art | Good painted source assets cannot compensate for incorrect placement | **P0, fixed for this report:** per-design mounts and reflection; retain before/after evidence |
| Character animation | Hands stay attached, but binary body facing and shared movement still feel stiff; prone rendering omits some attachments | **P1, partly improved:** mirrored shoulder/hand geometry. Next: directional torso poses and grounded steps within the same memory budget |
| Weapon variety | 36 authored families are broad enough; many share timing/poses | **P1, retained:** prioritize distinct reach/commitment and readable tells over another numeric expansion |
| Combat weight | Windup, contact, recovery, injury handling and stagger work; crowd fights can remain visually noisy | **P1, open:** improve target intent, disengagement and footwork; verify readable attacks at actual PSP scale |
| Tactical orders | Pause/follow/hold provide control but not enough tactical roles | **P1, open:** passive/block/ranged preferences with clear UI and predictable order cancellation |
| Ranged combat | Ammunition matters, but projectile obstruction/friendly fire are not fully simulated | **P2, open:** bounded projectile pool, visible travel and explicit ally-obstruction rules |
| Wounds | Six body regions, blood, bleeding, knockout and limb loss exist | **P1, retained/tested:** next improve bleeding urgency, recovery estimates and robotic repairs; no separate abdomen yet |
| Defeat/recovery | A rescue can preserve the story; enemy treatment of defeated characters lacks variety | **P1, major gap:** selective robbery, abandonment/capture motives and reliable rescue opportunities before more lethal enemies |
| Food/fatigue/weather | These create expedition costs but risk repetitive upkeep | **P1, improved preparation:** pack choices matter; next add route-specific shelter and resource opportunities |
| Skill progression | Use-training exists, but it is not always obvious what an activity trains | **P2, open:** explain activity/skill relationships and meaningful thresholds in the guide/UI |
| NPC professions | Merchants, medics, smiths, guards and recruits have distinct services/outfits | **P1, retained/tested:** still mostly functional personalities. Next: named residents with persistent problems and reactions |
| Mercenaries | Timed individual guards create a choice between protection and ownership | **P1, retained/tested:** not coordinated companies. Next: renewal and clear danger/expiry reporting before larger groups |
| Faction identity | Equipment, stock, reputation and advice differ; ideology/law remain shallow | **P1, open:** a small set of legible local prohibitions and attitudes with visible consequences |
| Crime | Theft, stolen tags, fencing, bounty and custody exist | **P1, audit:** no complete trespass/disguise/recognition/kidnapping system; add them with explicit witnesses and failure feedback |
| Captivity/slavery | Custody is not Kenshi's slavery loop | **P1, absent:** shackles, labor, escape recognition and liberation require a separate designed system; do not claim current jail time substitutes for it |
| Town geography | Settlements are separated by meaningful travel; streets remain compact | **P1, retained/tested:** next distinguish approach landmarks, architecture and safe refuges |
| Wilderness interest | Distance and enemies alone do not create exploration | **P1, major gap:** authored landmark chains, ruined work sites, alternate crossings and clues to danger |
| Enemy motives | Hostiles mostly function as combat encounters | **P1, open:** hungry robbers, patrols, territorial wildlife and scavengers should have different demands and retreat behavior |
| Exploration rewards | Resources, ruins and regional gear reward travel; unique stories are scarce | **P1, open:** named salvage with contextual leads and situational utility, not just larger stats |
| Atlas/route planning | Local/world maps are useful but contract text did not connect to them | **P0, improved:** direct current-objective location without silently changing travel; next add player pins and service/danger legends |
| Contracts | Delivery/supply/hunt/relic tasks work but remain repetitive | **P1, improved usability:** held-item counts and map link. Next: one authored route with two viable solutions and saved outcomes |
| Economy | Finite stock, prices and regional specialties exist; restock is generated | **P1, audit:** no functioning production-to-caravan supply chain. Add one recoverable shortage before systemic scarcity everywhere |
| World consequences | Prosperity/ownership changes are not a named-leader cause/effect system | **P1, largest strategic gap:** leader capture/death, a changed road/town and understandable retaliation; persist and explain each event |
| Interiors | Walkable furnished rooms, counters, beds and indoor saves work | **P1, retained/tested:** resident schedules, indoor combat, multi-floor navigation and richer persistent containers remain limited/absent |
| Outposts/jobs | Building, farming, crafting and depots exist; labor/logistics are compressed | **P1, audit/tested baseline:** add material delivery, construction stages and blocked-job reasons before expanding recipes |
| Research/power | A small tier system does not equal an industrial economy | **P2, open:** one complete power/production chain with visible bottlenecks and bounded simulation cost |
| Squads/animals | One controllable party and independent hired guards; no recruitable pack animals | **P1/P2, absent:** independent player squads and one useful pack species need explicit offscreen-simulation rules |
| Dialogue/onboarding | Functional advice helps, but repeated barks do not create relationships | **P1, partly improved:** pack advice and a new guide chapter. Next: event-aware lines linked to actual saved deeds |
| Save trust | Version/size/checksum validation, backups and migration are present | **P0, retained/extended:** genuine v4 and v5 fixtures; market supplement must not duplicate NPCs, money or sold stock |
| PSP constraints | Fixed pools, indexed art and bounded actor caches help; desktop tests are insufficient | **P0 qualification gap:** real PSP-3000 frame pacing, busy-town memory, sleep/resume and memory-stick endurance remain untested |
| Long-term purpose | The game offers many activities but too little personal history tying them together | **P1, major gap:** later dialogue, safety and commerce should visibly remember a rescued person or defeated leader |

## Next meaningful milestone

The strongest next addition is still a small authored salvage-route story: Copper Cross has a shortage, a specialist is stranded, and a named raider controls a route. The player can prepare cargo, hire protection, find another crossing, rescue the specialist or confront the leader. Outcomes should change stock, a resident's dialogue and road danger, survive saving, and permit retaliation. Acceptance should require two controller-played solutions and a later return that demonstrates different consequences.

That is preferable to claiming completion from item counts. Follow it with defeat/recovery motives and outpost job feedback, then independent squads. Keep the remaining gaps in [ROADMAP.md](ROADMAP.md) explicitly unchecked.

## References

- [Kenshi wiki: Backpacks](https://kenshi.fandom.com/wiki/Backpacks): carrying space, cargo-weight relief, combat tradeoffs, stock and food sharing.
- [Kenshi wiki: Encumbrance](https://kenshi.fandom.com/wiki/Encumbrance): worn versus carried load and movement consequences.
- [Kenshi wiki: World States](https://kenshi.fandom.com/wiki/World_States): named-character conditions and consequences for towns/spawns.
- [Kenshi wiki: Slavery](https://kenshi.fandom.com/wiki/Slavery): captivity, work, equipment confiscation and escape as a system distinct from ordinary custody.
- [Previous whole-game review](DESIGN_REVIEW_V050.md) and [mechanics matrix](MECHANICS_AUDIT.md) preserve the earlier assessment rather than treating previously identified gaps as new discoveries.
