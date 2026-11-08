# 0.15.0 implementation

## Combat information

Only characters engaged with the squad get enemy combat bars. Normal proximity reveals a broad role, faction color and hostility, not skill or strength values. The faction screen remains the color/standing key. Hidden ambushers stay hidden.

The bar is `clamp(min(head / speciesHP, chest / speciesHP, (blood - 20) / 80), 0, 1)`. An unconscious or dead character has zero knockout reserve. Head/chest failure or blood below 20 causes knockout in the simulation. Damaged or missing limbs retain their existing mobility, handling and damage consequences; their health is not averaged into a misleading general-health bar. This is a current-condition indicator, not a prediction of hits remaining against every weapon.

During fighting, one focused role label replaces the normal nearby-label cluster. Each engaged enemy keeps its own outlined bar. Gold marks show preparation and strike direction; short impact words distinguish hits, blocks, dodges and knockdowns. Decorative vegetation clears near squad combatants, and the previous silhouette rendering remains.

Two other active AI melee attacks can occupy a victim's immediate attack window. Additional melee attackers reposition and can take their place as attacks resolve. This does not remove enemies, restrict gunfire or make a surrounded character safe. NPC windup has a 0.45-second minimum; squad preparation retains the faster 0.20-second minimum. Switching to passive cancels the selected survivor's attack/order and applies at least 0.6 seconds of cooldown to prevent cancellation abuse.

## Equipment silhouettes

Bare/ragged torsos narrow by 18%; armor with protection at least 18 broadens by 12%. Species/body proportions, worn layers, weapon designs and existing elite equipment remain. A visual tier never invents armor or changes what can be looted.

## Roadside landmarks

Six persistent sites lie along these links:

| Link | Landmark |
|---|---|
| Dusk Haven to Copper Cross | Rock pass and wayfarer bed |
| Dusk Haven to Saltwatch | Rock pass and wayfarer bed |
| Copper Cross to The Dry Market | Rock pass and wayfarer bed |
| Red Gulch to Iron Gate | Rock pass and wayfarer bed |
| Reed Crossing to Cinder Port | Rock pass and wayfarer bed |
| Shogun's Rest to Dusk Ferry | Rock pass and wayfarer bed |

Each pass has four rock formations. Their collision also blocks projectiles. Central lanes and outer bypasses remain open. Old saved characters and structures near proposed rocks cause those formations to be omitted permanently. Six free beds sit off the routes with two dressings, two rations and two machine parts each. These are finite loot nodes, not renewable supply generators.

Approaching a shelter records it. When injured or exhausted, the HUD can show distance/direction to the nearest known shelter. No waypoint causes movement. These six authored landmarks supplement the existing rivers, fords, terrain regions and eighty persistent encounter groups; they are not a replacement of the entire world map.

## Recovery

Nearby actions offer a pressure dressing while a conscious organic survivor is bleeding and has either two cloth or worn rags. Cloth is preferred; otherwise the rags are destroyed. It stops bleeding and restores no health. The action can be queued and saved like other nearby actions.

Conscious outdoor medics and pilgrims can approach a bleeding or critically unconscious squad member within ten meters. The helper and casualty must both be safe from nearby hostiles; hostile factions, poor reputation, captivity and carried casualties prevent assistance. Organic treatment spends a real dressing. Clockwork treatment spends a real machine part. Helpers cannot regrow limbs or resurrect the dead. Town medics receive a small one-time stock when this feature initializes. Bed rest remains necessary after stabilization.

## Political aftermath

A succession result assigns surviving ordinary guards to physical posts. A hostile replacement of the old government can displace up to two ordinary residents toward the settlement outskirts. Relief-policy governments place surviving medics at a relief post; medics walk there through ordinary simulation. Banners use the current faction color and identify a garrison, relief post or trade court.

This reuses residents and preserves injuries and equipment. It does not recreate dead leaders or duplicate named loot. Existing price-policy, reputation and world-event consequences remain.

## Persistence

The append-only v15 trailer stores initialization, discovered shelters, bed references and omitted rock formations. Game size is 2,148,384 bytes. Version 14's 2,148,348-byte prefix remains readable, as do earlier supported saves. Repeated loads do not add extra shelters or replenish their caches.
