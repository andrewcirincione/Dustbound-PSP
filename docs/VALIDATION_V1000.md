# Dust 1.0.0 validation

## Feedback coverage

| Request | Shipped behavior | Verification |
|---|---|---|
| Research furniture or freely furnish a home | Beds, chests, tables, shelves and stoves are available for silver inside owned homes, without research. Research remains three forging tiers at a bench. | Native purchase, placement, save and pause tests; PPSSPP catalog and placement |
| Choose construction positions and clear foliage | Paused placement preview for campfires, inventory bedrolls and up to twelve home furniture pieces. Move or remove existing furniture. Walls, doors, props, resources and occupants prevent overlap. Camp footprints clear foliage. | Native collision checks and controller-driven PPSSPP custom camp placement |
| Footsteps quieter; menu navigation louder | Footstep amplitude reduced to one quarter; menu move/pick amplitudes raised. | Audio source inspection; final EBOOT boots and runs with the embedded soundtrack |
| Detached arms and wrong one-handed layers | Narrower shoulder span and torso calibration, anatomical elbow bends, separate bare arms/sleeves and front shoulder overlap. | Production-renderer galleries, attachment and animation suites; actual PSP combat captures |
| Black brigand and greatsword proportions | Outer armor width follows painted silhouettes; revised joint placement and two-handed grips. | All sixteen torso silhouettes and thirty-nine weapon/unarmed families reviewed; all three attack variants exercised |
| Downward staircase on the upper floor | Separate generated timber stairwell opening; ground-floor stairs retain their model. | Native floor-prop assertions; PPSSPP upstairs capture |
| Tents in some camps | Some fixed wayfarer rests and existing population camps use generated tents alongside bedrolls. | World renderer review and population fixtures |
| More smithy stock | Weapon and armor slots doubled, with varied families, quality and settlement-tier selection. | All nine active smithies checked; town market suite |
| Despawn dead bodies | Unclaimed wilderness corpses recycle actor slots after three minutes and outside 32m of any squad member. Protect carried bodies, unpaid bounties, active contracts, unique actors, leaders and permanent town residents. | Cleanup, carrying, leader-protection and population persistence tests |
| Dispatch mission clarity | Acceptance adds a sealed dispatch item; the Journal names the destination. Arrival removes the packet and records delivery; payment still requires the hiring contact. Old unfinished dispatches receive a packet when there is pack space. | New integration and actual old-save migration tests |
| Check Health / Stats tabs | Nearby target inspector has Health, Stats and Bounty tabs. Bounty terms moved from loot/name labels. | PPSSPP tab navigation and native read-only inspection |
| QTY / VAL divider overlap | Headers and item rows sit below the header divider in pack and loot screens. | PPSSPP inventory capture; render suite |
| Activities with enemies nearby | Cooking, trade, placement and rest do not reject a nearby hostile merely for being nearby. Injuries, incapacity and actual reach still apply. | Native hostile-adjacent cooking and camp rest; controller placement |
| Slightly more hostile road traffic and varied loot | Passing-traffic pacing shortened slightly and hostile roll weight raised. Poor mobs may carry sparse food, cloth, water, partial dressings, bolts, raw materials or occasional field equipment. Wielded weapons also vary. | Encounter renewal tests, multi-seed loot distributions and equipment removal |
| Remove nearby quick-buy shortcuts | Road groups use normal Trade; ration/dressing/parts quick purchases are not listed as nearby actions. | Population action enumeration and queued cooking tests |
| Jail hand-in without a keeper | A carried eligible target exposes Environment > Turn in carried bounty inside a paying faction's jail. | Native test with all keepers inactive; PSP hand-in pays 2,560 silver exactly once |
| Loot disappears from the victim | Taking gear removes the actual equipped item. Fixed the previously unlootable underlayer. | Real weapon, torso and underlayer removal plus save/streaming tests |
| Markets never close | Shop services and business routines operate throughout all 24 hours. | Hourly market checks and town-routine suite |
| Separate camp/home pause tab | Four pause tabs: Pause, Squad, World, Camp & Home. Place furniture is disabled outside an owned home. | PPSSPP menu captures and input assertions |
| Hostile outsider factions red | Restricted outsider-hostile factions, and sufficiently hostile standing, render red in Factions. | Render/source checks |
| No prosthetic crafting; plate at smithies | Limb recipes disabled. Equipment forging requires being inside a smithy at its forge. | Crafting and integration suites |
| Remove recipes and upkeep; campfire and bedroll only | General recipe and upkeep routes removed, including the forge shortcut. Camp build list has only Campfire and Bedroll. Existing smithies, research benches and legacy save structures remain usable. | Input-route audit, build count, costs and field-survival tests |
| Camps expire after a day away | Camp proximity tracks all living squad members. A camp clears after one simulated day away; occupied beds are protected. Packed or expired building slots can be reused. | Day-expiry and pack/redeploy regression checks |
| Increase town work and danger pay | Boards usually offer one to three jobs, occasionally none, with cooldown retained. Some dispatches choose longer routes and pay for distance/danger. | Posting and contract suites; physical packet checks |
| Paid shared depots with staff | Three public depots share forty-eight stacks. A clerk or counter opens storage. Quoted per-item fees apply in both directions; insufficient funds fail without moving goods. Home chests retain a separate free shared sixty-four-stack stash. | Native cross-town storage and atomicity; PPSSPP 6-silver deposit and 6-silver withdrawal |
| Generated main-menu background | The existing generated ronin landscape is compiled into an indexed title backdrop. No new geometric world art. | Actual EBOOT title screenshot |

## Test record

All **39 native suites passed**, including the real-time patrol regression. The integration suite and all **575 render cases** also passed AddressSanitizer and UndefinedBehaviorSanitizer. **Thirteen archived expeditions** passed save/load roundtrips.

The maintained native runner is `python scripts/validate_v1000.py`. It builds the shared production core, unpacks archived saves and compiles each `tests/test_*.c` suite. Historical fixtures were updated for finite market stock, timed treatment, real chest inventories, current town geometry, free furniture placement, the six merged weapon skills and the 150 skill cap. Isolated army fixtures explicitly free dormant generic residents; actor exhaustion remains separately tested without those free slots.

The wider run found and fixed a real patrol-employer bug: a jailkeeper could be selected for patrol work, but their business routine kept taking precedence. New patrol jobs select an available ordinary outdoor guard. A controller-driven depot transaction also found a reach mismatch between its clerk and counter; either valid service position now authorizes transfers.

Render verification covers 575 screen/biome/guide combinations at 480 × 272 with a 512-pixel framebuffer stride and guard bytes. Animation tests exercise 39 weapon/unarmed families and three attack variants. Equipment galleries cover 16 painted torso silhouettes in passive, guard, strike and heavy-weapon poses.

Compatibility checks use actual archived expeditions from 0.1 through 0.19.0, including newly generated fixtures written by the original 0.18.8 and 0.19.0 executables. Roundtrips preserve selected survivor position, money, inventory, equipment, XP and stolen ownership. The 0.19.0 property fixture additionally checks furniture migration, the shared house stash, partial dressing stock and unfinished dispatches.

PSP testing uses official PPSSPP 1.20.4 and the compiled PSP EBOOT, with ordinary prepared saves and emulated button presses. Debug capture/telemetry are enabled only by a QA marker that is excluded from the release package. The final test payload hash is recorded in `evidence/v1000/build-manifest.json`. Native simulation timing is not physical PSP performance. Hardware PSP performance and audible output have not been measured.

Screenshots in `evidence/v1000/` are captured from PPSSPP. Equipment contact sheets use the production native renderer and are explicitly labeled as review galleries.

## Reproduction

1. Install the official PSP toolchain and build with `make` (`PSPDEV` and its `bin` directory on `PATH`).
2. Run `python scripts/validate_pbp.py EBOOT.PBP --assets assets` to check the launcher, executable and embedded music sections.
3. Run `python scripts/validate_v1000.py` for native regressions. `scripts/native_v1000.mk` can build the native core independently.
4. Build `tests/make_v1000_fixture.c` against the native core and run it to create ordinary QA saves. `scripts/emulator_v1000.py` drives PPSSPP and its debugger in the same process namespace; its second argument accepts a JSON sequence of controller actions and expected state.
5. Package with `python scripts/package_v1000.py`. The script checks the EBOOT against the final emulator-tested SHA, rejects QA saves/markers in the ZIP, and writes deterministic archives and SHA-256 sums.

This release retains the established settlement layouts, regional architecture, gear assets, injuries, slavery/custody, faction wars and home ownership. It does not replace existing painted art with primitives.
