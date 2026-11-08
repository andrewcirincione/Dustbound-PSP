# 0.16.0 reconstruction checkpoint

Baseline: v0.15.0, d6c2c73e16e94945d3105cd71b51d23511dbc692.
The previous session's 0.16.0 source and executable were not recovered. This branch reconstructs its requirements; previous claims of passing tests are not evidence for this build.

Requested scope: audit all weapon families and unarmed idle/combat motion; coherent, fluid, weighty animations; proficiency must accelerate the actual swing; compile and publish the PSP executable.

Recovered findings to verify and fix:
- Fixed 0.24-second strike despite proficiency-dependent preparation/recovery.
- Separate relaxed idle and combat guard; unarmed guard; distinct bow draw/release instead of firearm recoil.
- Continuous phase transitions and follow-through; reachable rigid weapon grips.
- Walking phase driven by actual displacement, including blocked movement.
- Planted rear foot and coherent hit reactions.
- Downed beasts must not remain standing.
- Combat identification must not cover raised weapons.
- Exercise the actual world renderer as well as isolated pose previews.

Plan: push each implementation checkpoint; run native and rendered regression coverage; compile with pinned PSPDEV; inspect emulated output where available; publish only after validation and verify release assets. Record any validation limitations explicitly.
