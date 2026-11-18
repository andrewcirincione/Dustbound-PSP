# Boot alignment and single-handed shoulder carry — preview stage

The user requested in-game previews before regression testing or publication.
They rejected the first raised two-handed carry and specified a front elbow
at about 90 degrees, a blade resting on the shoulder at 45 degrees, and a
relaxed rear arm. This checkpoint implements that revised passive pose.

## Current changes

- Right-facing boots move 0.7 render units toward the leg. Both endpoints use
  the same solved ankle position, so the adjustment follows each step.
- Passive greatswords slope back at 45 degrees and are held by the near/front
  hand. The rear arm hangs and swings naturally while walking.
- The wrist is placed sqrt(2) arm-segment lengths from its shoulder socket,
  producing a 90-degree elbow through the normal equal-length arm solver.
  Its position accounts for the blade's painted lower edge at the shoulder
  bearing, so the blade's thickness is supported above the shoulder cap.
- The carry blends into the regular two-handed combat stance. Existing guard,
  attack and block poses remain the combat targets.

## Preview method

`tests/preview_alignment.c` prepares declared save scenes with four outfits
(Cinder Scout, Quarry Jack, Charter Marshal, Sunken Crown) and four greatswords
(Breaker-Yard Slab, Ceramic Bulwark, Atlas Meridian, Shearblade). It uses the
production renderer for iteration and writes ordinary saves for the PSP game.
No capture-only pose overrides are added to production gameplay.

`scripts/preview_alignment_psp.py` boots the executable in official PPSSPP
1.20.4 and loads the prepared scenes through the game's menus. The revised
`shoulder-carry` set captures both facings, inventory views and walking using
the real 480×272 PSP framebuffer. Scenes are arranged on an isolated road for
equipment inspection; these are prepared scenes, not a campaign.

`scripts/arrange_shoulder_previews.py` arranges matched previous/revised frames
and both facings of the four loadouts. Crops use nearest-neighbor enlargement;
the originals remain in `evidence/alignment-preview/shoulder-carry`.

The earlier rejected pose's captures remain in
`evidence/alignment-preview/after`. Its source checkpoint is
`389c3c46b2dc1712ee0ef448d081336c33f212e2`; its EBOOT SHA-256 is
`47e6f2c208754b1ed553403732351f7d293f1b141eae6ef20ef78b8d3c0fd3aa`.
The new comparison sheets are in `evidence/shoulder-carry-preview`.

Revised preview EBOOT SHA-256:
`74c20eda14092759b10e0bf58dfbd8368f81aa278601f356afddaaa2ffb23ed5`.

The executable retains the 1.2.2 title while it is a preview. Main and the
public release are unchanged. Full regression tests and publication remain
pending user visual review. Physical PSP hardware was not tested.
