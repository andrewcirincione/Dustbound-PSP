# 0.13.0: decisions and consequences

The 80 persistent groups and 16 encounter types remain. This release gives those groups more specific decisions, shared online/offline consequences and visible reports. It is a bounded world simulation, not a reproduction of every S.T.A.L.K.E.R. A-Life feature.

| Situation | New behavior |
|---|---|
| Hunters | Track real beast groups, close on prey, collect actual meat/hides from fallen beasts and return to camp. Cooking consumes two carried meat for one ration. |
| Ambushers | Lie concealed, remain hidden on the minimap and opponent selector outside detection range, then move to alternating flanks when approached. |
| Patrols / raiders | Pursue nearby hostile groups / vulnerable merchants and refugees. |
| Casualties | Survivors regroup, spend actual medicine, pick up unconscious companions and put them down after recovery. Outnumbered groups can withdraw. A single death no longer turns every survivor into a stationary ruin. |
| Camps | Crew provisions draw from finite stock or ration caches. Sustained food shortage triggers packing, a real town stock withdrawal, relocation and re-establishment; failed supply trips abandon the old camp. Sleeping players delay packing. |
| Refugees / wounded | After recovering, pack and head for a safe settlement. Arrivals become recruitable residents, without automatically joining the player. |
| Armies | Choose allied towns needing reinforcement or supplies; carry actual ration cargo between towns; attack opposing towns when their own holdings are sufficiently supplied. Conquest requires defeating the actual garrison. |
| Pilgrims / convoys | Pilgrims deliver a carried medical kit. Convoys place their remaining captives into custody on arrival. |

Wounds, carry relationships, cargo, camp supplies, objectives and government state persist through streaming and save/load. Corpses stay where they fell while survivors move on. Returning to corpses can materialize their surviving group. Freed, hired and recruited members detach into permanent individual actors.

## Succession

The death of one of the six settlement rulers starts a dispute between the caretaker administration and a rival faction. Local food, surviving guards and arriving armies affect influence. A dispute resolves after seven simulated minutes, or after three minutes when one side leads by at least 30 influence.

Use **Nearby actions > Council**, or **Start > World > World activity**. Viewing a council remotely is allowed; committing silver or relief requires being conscious, outdoors and within 24 world units of the settlement.

- Back either claimant once for **200 silver / +30 influence**, with faction reputation consequences.
- Deliver **four actual rations once / +12 caretaker influence** and improve local prosperity.
- Resolution changes ownership, ordinary resident allegiance, banners/buildings and administration prices. A surviving generic guard may be appointed as steward; dead rulers and unique equipment do not respawn.
- Relief governments reduce food, water and medicine prices 15%; military governments raise equipment prices 12%; trade governments reduce supply prices 7%. Contested settlements have a temporary 15% price surcharge.

The activity ledger reports discovered political changes and previously encountered groups. It is not an omniscient list of undiscovered encounters.

## Camp presentation

Scattered refugee and wounded camps, a hunter arc, concealed smuggler arrangements and ordered trader yards have separate prop layouts. Improvised camps use cloth bedrolls; traders retain beds. Camps have distinct work props/supply arrangements, and nearby decorative vegetation is cleared so beds and supplies remain readable. Existing tent and fire artwork is reused.

## Prosthetic specialization

Per fitted limb; arm bonuses affect strength/dexterity, while leg bonuses affect movement/load. Fitting still costs two machine parts. Shops, Surgeon Venn, severing, crawling, one-arm two-handed penalties and loot ownership remain intact.

| Grade | HP | Price | Arm STR / DEX | Leg move / load | Weight |
|---|---:|---:|---:|---:|---:|
| Scrap | 70 | 250 | -2 / -2 | -8% / 0 | 2 |
| Standard | 100 | 700 | 0 / 0 | 0% / 0 | 2 |
| Reinforced | 190 | 1,450 | +10 / -4 | -6% / +12 | 5 |
| Precision | 110 | 2,800 | -2 / +10 | +22% / -3 | 1 |
| Relic | 180 | 5,200 | +6 / +6 | +15% / +6 | 3 |

Reinforced is strongest for power, durability and hauling; Precision favors dexterity and speed; Relic remains an expensive all-round choice. The fitting screen shows old/new numerical stats and socket-specific tradeoffs. Random wearer probabilities are unchanged: nominal 4%, 4%, 7%, 10% by tier, with Relic only 0.5% of the highest-tier part rolls. Camp supply traders do not sell replacement limbs.

## Save format

Save v13 appends 15,524 bytes of behavior/politics state after the unchanged 2,127,704-byte v12 prefix. Game is 2,143,228 bytes; Actor remains 932 bytes. An actual released v12 save is covered by migration and round-trip tests. Earlier migration gates remain. Keep a backup before saving with a newer executable.
