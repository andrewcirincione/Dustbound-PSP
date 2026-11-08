# 0.12.0 — An unquiet frontier

The world holds 80 persistent encounter groups in addition to the eight existing event slots, settlements and named characters. The six sampled seeds contain 297–309 members in these groups. The live actor limit remains 192; distant groups keep their full rosters in saved storage and materialize when any squad member approaches. If there are too few free slots, the whole group waits. Quest actors and other protected residents are never recycled for encounters.

## Situations

| Encounter | What you can find |
|---|---|
| Scavenger camp | Fire, tent, 2–3 usable beds, supplies, workers gathering nearby resources |
| Refugee camp | Poorly equipped travelers, beds, shared supplies |
| Hunters' camp | Armed hunters and a hide cache |
| Smugglers' den | Outlaw camp, tonic cache and finite supplies for sale |
| Traders' rest | Beds, cooking and finite food, medicine and parts for sale |
| Road patrol | Armed group moving between settlement outskirts |
| Raiding band | Hostile armed travelers |
| Beast pack | Several prowlers, with actual meat and hides |
| Pilgrimage | Monastic travelers moving between destinations |
| Wounded travelers | Injured survivors and a small resting camp |
| Battlefield | Fallen actors, an injured survivor and remaining equipment |
| Marching army | 12–16 armed soldiers, with stronger leaders |
| Bandit ambush | Hostile camp beside the road or in the wilderness |
| Salvage crew | Workers collecting nearby ore, copper or timber |
| Prisoner convoy | Five guards escorting two bound prisoners; existing rescue rules apply |
| Wandering duelists | One to three experienced armed travelers |

Placement mixes settlement outskirts, routes between towns and wilderness regions. The group type, composition, species, equipment, condition and destination vary. Named champions remain separate. Encounters appear in the physical world; the atlas does not reveal their unseen locations.

## Persistent activity

Nearby members use ordinary movement, combat, injury, theft, rescue and loot systems. Camps alternate between rest and daytime foraging. Traveling groups stop at destinations before moving on. Faraway groups update travel and physiology once per second, with coarse faction battles every five seconds. Scavengers and salvagers can consume actual finite world-node resources and carry what they gathered. Offline combat changes real wounds and incapacitation without crediting the player with participation, kills or rewards.

Streaming retains identity, pack and gear, fitted limbs, stolen tags, wounds, trauma, corpse state and weapon-class experience. Camp stocks and caches are also saved. A split squad, combat, carrying, contracts or an active rescue can keep actors loaded. Freed travelers and recruited/hired members detach into persistent individual actors; freedom does not automatically recruit anyone.

An aftermath can renew only after more than 7,200 seconds of quiet aftermath time (about four game days), while unloaded and over 120 world units from every living squad member. Merely leaving and returning does not refill loot, heal wounds or reroll a fitted limb.

## Using a camp

Approach the physical object and press Cross for nearby actions:

- **Beds:** free rest when safe and unoccupied; movement wakes you. Bleeding must be dressed before ordinary recovery. A spare bed is reserved from the residents' automatic resting routine.
- **Fire:** two meat become one ration. Requires room for the result; stolen ownership is preserved.
- **Traders'/smugglers' fire:** buy finite rations (40 silver), medkits (80) and machine parts (100). Hostile or unwilling traders refuse.
- **Cache:** take its actual remaining contents once. Witnessed theft from a non-hostile camp creates a faction crime and retaliation.
- **Group:** Observe reports the situation, survivors and activity. NPC dialogue describes the group.

## Prosthetics stay uncommon

Healthy ordinary humanoids have a 4% wear chance; higher-tier travelers have 7%, and the strongest tier 10%. Most wear one part, with a small chance of two. Beasts are excluded. Lower tiers mostly wear Scrap or Standard parts; Relic parts are only possible in the strongest tier and account for 0.5% of its part-grade rolls. Across six generated populations, 122 of 1,821 members wore a replacement (6.7%, including beasts in the denominator). The strongest-tier test produced five Relic parts in 20,000 actor trials.

These are real fitted items, rolled once when that person is created. Looting one removes it from the fallen owner and preserves that loss. The six earlier named owners and regional limb shops remain; ordinary camp traders do not sell prostheses. Five grades, fitting, surgery, missing-leg crawling and missing-arm weapon penalties from 0.11.0 are retained. See [limb controls and shops](CONTENT_V110.md).

## Limits

This is an original bounded simulation inspired by the requested A-Life-style behavior. It uses coarse off-screen combat, not the full nearby animation and projectile simulation. It does not implement an unlimited population, faction command strategy or complete autonomous economies. Large encounters can wait for actor capacity, and the oldest untouched aftermaths eventually renew. Physical PSP performance and a naturally played multi-hour campaign remain unqualified.
