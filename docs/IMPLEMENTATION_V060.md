# 0.6.0 implementation status

The unpublished candidate was lost during a workspace reset following an interrupted upload. Released v0.5.2 is intact. Earlier tests do not qualify a reconstructed executable.

Remaining acceptance scope:

- [ ] Authored exploration and multiple quest solutions.
- [ ] Persistent town, faction, road and trade consequences.
- [ ] Captivity, escape, recovery and robotic repair.
- [ ] Tactical combat, projectiles and indoor combat.
- [ ] Named dialogue, schedules and contextual NPC behavior.
- [ ] Construction, production, fuel, job priorities and expedition groups.
- [ ] Armor layers, coverage and meaningful gear comparisons.
- [ ] Objectives, preparation advice and contextual guides.
- [ ] Presentation review, campaign/economy tests and PSP profiling.
- [ ] Fresh native/sanitizer checks and controller playtests on the exact EBOOT.
- [ ] Verified GitHub build, source, installer, evidence and release.

Physical PSP-3000 qualification remains outstanding.

## Publishing connection

On 2026-11-06, Dustbound-PSP was added to the existing ChatGPT Codex Connector installation used for Rift-Ascent. This status update was published directly through the GitHub connector, verifying repository write access. Future source updates can use this connection instead of browser uploads.

The implementation and validation checklist above remains outstanding; this access repair does not constitute a new game build.

## Rebuild checkpoint — 2026-11-06

Reconstructed the Copper Road expedition, named residents and branching consequences, six new gameplay panels, v6 save migration, construction/fuel, job queues and groups, trauma/robot repair, underlayers/coverage, captivity, room combat and moving projectiles. New native tests cover the branching route, transactional failure paths, save migration, seven simulated days and the whole catalog buy/resell spread. Regression work and controller playtests are in progress. This checkpoint is not release qualification.
