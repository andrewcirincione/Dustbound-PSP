# Dustbound 0.16.0 validation

This build reconstructs the interrupted animation work from v0.15.0. Earlier-session test claims were not reused. The recovered requirements and checkpoint history are recorded in [RECOVERY_V160.md](RECOVERY_V160.md).

## Native checks

- 956 weapon/unarmed catalog entries; phase endpoints and contact continuity.
- 4,772,352 grip/reach combinations covering both primary hands, three body widths, sixteen aim directions and four phases. All remain finite and within arm reach.
- Actual stroke timing improves with proficiency for 955 weapons and martial skill for unarmed attacks; missing-arm handling penalties remain.
- Movement-driven gait, blocked room movement, attack movement lock, world sprite-cache stance with layered armor, downed beasts and save/load transient state.
- AddressSanitizer/UndefinedBehaviorSanitizer animation suite and v080–v150 regression scripts passed. Includes existing injury, prosthetic, inventory, faction, population and save migration checks, a 12,000-tick full simulation and a ten-game-day population endurance run.

## Visual review

Production-renderer sheets cover relaxed carry, guard, windup, contact, follow-through, recovery, walking and mirrored guard. Review caught long weapons dipping below the ground; relaxed and ending angles were corrected before the final build.

![All native pose families](../evidence/v160/native-all-families.png)

## PSP runtime audit

Validation pending until the final executable finishes the controller-driven PPSSPP audit. Each case loads an ordinary save through the title/load/confirmation screens and records 240 completed world frames at 30 Hz. It does not write emulator memory or inject gameplay state. Prepared saves isolate one opponent and provide ammunition; they are animation scenarios, not campaign balance tests.

| Case | Weapon family |
|---|---|
| 00 | Unarmed |
| 01 | Katana |
| 02 | Wakizashi |
| 03 | Nodachi |
| 04 | Tanto |
| 05 | Naginata |
| 06 | Yari |
| 07 | Nagamaki |
| 08 | Kusarigama |
| 09 | Frontier sabre |
| 10 | Bowie knife |
| 11 | Trail machete |
| 12 | Butcher cleaver |
| 13 | War pick |
| 14 | Boarding axe |
| 15 | Kanabo |
| 16 | Tetsubo |
| 17 | Quarterstaff |
| 18 | Hook blade |
| 19 | Ring sabre |
| 20 | Broken falchion |
| 21 | Horse cutter |
| 22 | Saw sword |
| 23 | Twin sickle |
| 24 | Spiked club |
| 25 | Scout bow |
| 26 | Repeater bow |
| 27 | Heavy arbalest |
| 28 | Hand crossbow |
| 29 | Longbow |
| 30 | Ranger rifle |
| 31 | Dragoon pistol |
| 32 | Scattergun |
| 33 | Greatsword |
| 34 | Railway maul |
| 35 | Crescent glaive |
| 36 | Anchor cleaver |
| 37 | Scimitar |
| 38 | Rapier |

## Reproduction

Run `bash scripts/validate_v160.sh`, build with PSPDEV v20261101, then run `python3 scripts/emulator_v160.py /path/to/PPSSPPHeadless`. The other validation scripts cover earlier gameplay systems. `scripts/verify_release_v160.py` verifies the source tree, compiled EBOOT and evidence hashes before release.

No physical PSP hardware test was performed. These focused tests do not establish full-campaign balance or guarantee absence of all visual defects.
