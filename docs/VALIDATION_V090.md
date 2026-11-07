# 0.9.0 validation

Qualified on 2026-11-07. The local PSP executable was compiled with pinned PSPDEV v20261101 and exercised in PPSSPP 1.20.4 through emulated controller inputs. Prepared ordinary saves make the specific situations reproducible. QA telemetry and completed-frame capture are read-only; the playthrough does not write emulator memory.

- EBOOT: 1,559,098 bytes; SHA-256 `eb76a319a4f5f584471ddd7332a99a705bd3c97c493d89acb4459845d26ecfb3`.
- Game state: 618,188 bytes; save version 9. The genuine 0.8.1 fixture is committed compressed at `tests/fixtures/v081-migration.sav.gz`; its uncompressed SHA-256 is `a3a48b110ff34414e8c8beb684b28adf7cba1ff50f7f23f7a38256636d273214`.
- Publication rebuilds the executable independently, checks package structure, and requires the exact emulated EBOOT hash before uploading.

## Native checks

| Suite | Scope and result |
|---|---|
| `validate_v080.sh` | PASS: 61/61 core and 50/50 expansion checks; renewable event combinations, army streaming, rewards and legacy migration |
| `validate_v081.sh` | PASS: town selection, discovery, paging, zoom, exact travel, D-pad separation, and exclusion of incidents from both atlas lists |
| `validate_v090.sh` society | PASS: new weapon variants, regional profit/no same-town arbitrage, offscreen production; faction warrants, gates, uniforms, recognition; freight atomicity/deposit/one-shot payment; service wages, warning cooldown, patrol completion, offscreen employer movement and failure; paused lock/open queue; physical capture, binding, eating, escape and v9 persistence; actual v8 migration |
| Other prisoners / slaves | PASS: timed sequential rescues while paused, save/load mid-rescue, guard detection and faction consequence, distance/room/dead-target checks, independent flight/following without squad membership, dismissal to refuge, explicit recruitment and full-roster behavior |
| Rendering | PASS: AddressSanitizer/UndefinedBehaviorSanitizer, 377 screen/biome/guide cases, 480×272 bounds and 512-pixel stride |
| Progression | PASS: strikes, blocked XP, hit defense/toughness, unarmed and staggered evasion, burden, opponent scaling, armor-independent toughness training, separate persistent class XP and handling effects |

The final native simulation sample ran 1,000 full-world ticks with 173 actors in 0.081 seconds of host CPU time. This is neither PSP frame rate nor a full campaign endurance test. Native output is recorded in `evidence/v090-native-checks.txt`.

## PSP controller playthrough

`scripts/emulator_v090.py` captures 31 actual PSP frames and checks inventory hover without equipping, real outfit changes, both skill pages, town-only atlas paging/local view, merchant trade/tips/work, accepting an escort, paused lockpick → open execution, employer distance warning with wages stopped, local warrants, cannibal capture and eating damage, weapon-class XP increasing during real combat, two queued prisoner/slave rescues, independent followers and explicit recruitment. Its completion record and matching EBOOT hash are in `evidence/v090-controller-checks.json`.

Screenshots are `evidence/v090-psp-*.png`. The feedback gallery groups unmodified captures; it does not substitute mockups for emulator output. The terminal failure image used during script debugging is excluded from the evidence set.

## Limits

No physical PSP test, complete natural campaign, sustained memory-stick test or hardware frame-pacing claim is made. Eight renewable incident mechanics combine with different actors and conditions; they are not infinitely many distinct authored systems. The economy is an aggregate town model. Numerical balance is adapted from Kenshi rather than identical. See [stat audit](STATS_AUDIT_V090.md) and [design critique](DESIGN_REVIEW_V090.md).
