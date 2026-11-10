# Dustbound — PSP frontier survival RPG

## 0.18.3 — Walls and streets

An original isometric western–samurai survival RPG for PSP homebrew, inspired by Kenshi. Build a squad, survive injuries and hunger, trade, steal, craft equipment and establish a camp among independent factions and wandering groups.

Dust.mp3 loops across the title screen, gameplay and menus. The full soundtrack is built into the EBOOT and decoded ahead of playback.

[Three Banners design](docs/FRONTIER_WAR.md) · [Field guide](docs/FIELD_GUIDE.md)

Town work appears in small batches. Ask around for the person hiring, and use the Journal for the current objective and turn-in. Shop and household chests contain owned goods. Residents come and go, chests replenish after a few days, and repeated theft prompts temporary pocket searches.

Fight stance closes to weapon range and repeats at the weapon's cooldown. Analog movement takes priority. Passive stance stows the weapon. Leg injuries can cause a limp or a crawl. Bed rest places the survivor on the mattress, and serious wounds take longer to dress and heal.

All nineteen settlements have their own street plans, amenities and interiors, with painted buildings and furniture tied to local materials and culture. Brass Junction has an armorer and a six-bed inn. Vermilion Keep has a stockade, while Copper Cross uses watch cages. Guests circulate between the inn, streets and homes. Dustward Clan lodges share beds and training access for members. The Compact, Shogunate and Ash Monastery send armies against one another. Captured cities can have guarded labor yards, and nearby residents pass on news of actual raids and victories.

Painted buildings and furniture use measured foundations. Walls follow both isometric axes, and jail gates swing open when unlocked. Nearby names and speech share space with building labels. Eight more small bandit gangs roam the roads with weak fighters and occasional supplies. Household chests hold more, and retreat is reserved for severe injuries or a rout.

![Nearby roles and military equipment](evidence/v150-psp-army.png)

There is no map or group auto-travel. Cross on the map places a pin on both maps. Follow its guide line on the minimap until you reach it. Squad followers, work jobs and NPC movement remain part of the simulation.

Starving bandits carry poor weapons and wear incomplete clothing. Soldiers wear coordinated armor and move in three-wide formations. Elite duelists wear complete superior equipment. Nearby labels show a broad role in faction colors, with a red **!** for hostility. Enemy combat bars show knockout reserve from head, chest and blood. Nearby characters do not expose skill values or encounter tiers. Start > World > Factions shows the color key, reputation and wanted standing.

New expeditions start in one of five settlements with different equipment, companions, and difficulty. The atlas and local map share the walking view's orientation. Nearby Actions lists self-treatment and shows why unavailable actions are disabled. First aid stops bleeding before dressing individual wounds. Food and dressings can be used in portions, and wounds recover after treatment. A bed speeds up recovery, and portable bedrolls can be deployed and packed away. Toughness determines the threshold for a recovery coma. Jail staff treat prisoners, and rare travelers can help injured strangers.

The thin control strip shows common menu shortcuts. Squad management separates direct assignments from job priorities. Follow tracks the controlled member, and the active character's assigned work waits for five idle seconds in Passive stance. The scrolling news bar keeps urgent injury alerts and gives the last message more time on screen.

The world activity ledger records where and when you saw a group. The Journal tracks the caravan on an accepted escort. It waits for you to meet it and stops if separated. Councils show named claimants and the price policy each would implement. Successful autosaves are silent.

Fighting now has clearer windups, outlined knockout bars and staggered AI melee attacks. Passive stance cancels your attack so you can retreat. Six roadside rock passes offer cover and alternate routes; free wayfarer beds provide finite supplies and recovery stops. Emergency dressings can consume cloth or worn rags, and friendly medics can approach and stabilize you when safe. Government changes reposition surviving guards, civilians and relief medics.

Weapon handling now changes the actual strike speed. Relaxed carry and combat guards blend smoothly; fists, bows, firearms, thrusting weapons and heavy swings have distinct motion. Rigid grips prevent overstretched arms, walking follows distance, and combat labels leave raised weapons visible.

For a minor bounty the watch approaches and offers payment or jail. A bounty of 100 silver or more triggers pursuit on sight. Guards carry subdued or surrendered survivors to cells, treat wounds and seize stolen goods. Work camps issue slave garb and hold confiscated belongings in a chest. Road traders carry their own stock, and treating faction members, trading or completing work can improve standing.

## The wider game

- Five playable species, distinct settlement cultures, eighteen named leaders and elites, and personal equipment.
- Ninety-one persistent wilderness groups across sixteen encounter types, from refugee camps with usable beds to armies, convoys and ambushes.
- Hunters pursue real prey; survivors use medicine and carry casualties; camps seek finite supplies; refugees can settle; armies reinforce, supply and assault settlements.
- Six body regions, permanent limb loss, crawling, two-handed weapon penalties, five prosthetic grades, shops and a surgeon.
- Weapons with different shapes, reach and handling; layered clothing, backpacks, crafting requirements and direct equipment comparisons.
- Faction reputation, crimes, imprisonment, rescue, recruiting, hired guards, job queues, outpost work and succession after ruler deaths.

[Living world](docs/CONTENT_V130.md) · [Encounters](docs/CONTENT_V120.md) · [Limbs](docs/CONTENT_V110.md) · [Species and equipment](docs/CONTENT_V100.md)

## Install

Extract `Dustbound-0.18.3-PSP.zip` to the memory-stick root, or replace `PSP/GAME/DUSTBOUND/EBOOT.PBP`. A PSP capable of running homebrew, or PPSSPP, is required. Keep your saves before upgrading. Save format v25 reads earlier expeditions. Existing equipment, training and town control carry over.

## Controls

| Input | Action |
|---|---|
| Analog stick | Walk; interrupt work |
| Cross | Nearby actions; confirm menu choice |
| Circle | Sneak in the world, back in menus |
| Triangle | Inventory |
| Square | Fight or Passive stance in the world |
| Select | Atlas in the world; contextual guide in menus |
| Start | Pause menu |
| D-pad | Information shortcuts / menu selection; never walking |
| R in the world | Pause/resume simulation and queued actions |
| L + Square | Choose an opponent |
| Cross in the atlas | Place a manual reference pin |

The pause menu has Pause, Squad and World tabs. Factions and World activity are in World. Menus stop simulation. Combat and assigned jobs retain their own behavior; map pins never issue movement.

## Build and verify

With PSPDEV available, run `make -j2`, then `python3 scripts/validate_pbp.py EBOOT.PBP --assets assets`. The PSP build uses PSPDEV v20261101.

Run the regression gates from `scripts/validate_v080.sh` through `scripts/validate_v160.sh` listed in `.github/workflows/build.yml`. Run `bash scripts/validate_v170.sh path/to/v16.sav` to verify war behavior and older saves. Pass that same old save to `python3 scripts/emulator_v170.py /path/to/PPSSPPHeadless path/to/v16.sav` to check the compiled EBOOT using controller input (Python dependencies: Pillow and websockets).

This remains a playable alpha. PPSSPP testing does not qualify physical PSP performance, and the focused controller scenarios are not a naturally played full campaign. Offline battles use a simplified model. See the current validation record for exact scope.

MIT license for original source and included artwork. PSPDEV and PPSSPP are separate projects and are not bundled with the release.
