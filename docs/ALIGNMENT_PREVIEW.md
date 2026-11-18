# Boot and greatsword alignment — preview stage

The user requested in-game previews before regression testing or publication.
This branch is a review checkpoint, based on the published Dust 1.2.2 code.
The public release and main branch have not been changed by this preview.

## Changes

- Right-facing boots move 0.7 render units toward the leg. Both endpoints use
  the same solved ankle position, so the adjustment follows each step. Left
  facings, bare feet, and exposed mechanical feet retain their existing offsets.
- Greatsword carry grip height changes from -19.6 to -23.2. The shallow -2.80
  radian carry angle stays the same. Raising the wrist bends the elbow through
  the existing arm solver and moves the blade above the shoulder instead of
  increasing its angle. Combat guard and attack keyframes are unchanged.

## Preview method

`tests/preview_alignment.c` prepares declared save scenes with four outfits
(Cinder Scout, Quarry Jack, Charter Marshal, Sunken Crown) and four greatswords
(Breaker-Yard Slab, Ceramic Bulwark, Atlas Meridian, Shearblade). It uses the
production renderer for iteration and writes ordinary saves for the PSP game.
No production gameplay or capture-only pose overrides are added.

`scripts/preview_alignment_psp.py` boots the PSP executable in official PPSSPP
1.20.4, loads the prepared saves through the game's menus, and captures the
real 480×272 framebuffer in both facings. The updated build also captures
walking in both directions. Scenes are arranged on an isolated road for
unobstructed equipment inspection; these are prepared scenes, not a campaign.

`scripts/arrange_alignment_previews.py` labels matched before/after frames and
enlarges their crops with nearest-neighbor sampling. The original frames and
capture records are preserved in `evidence/alignment-preview/before` and
`evidence/alignment-preview/after`.

The completed preview capture set contains 16 baseline frames and 24 updated
frames: world and inventory views of all four loadouts in both facings, plus
eight updated walking frames. These are visual-review captures, not a full
regression qualification.

Baseline source: `7479dea2989d5ca7aad8a430e23cbd8edf5a1f43`.

Baseline EBOOT SHA-256 (identical to the published 1.2.2 executable):
`a2782c183a2adc9107a1215a2c677d0dd59fe621c38e5e9fb8f4c0bcbfac2a96`.

Preview EBOOT SHA-256:
`47e6f2c208754b1ed553403732351f7d293f1b141eae6ef20ef78b8d3c0fd3aa`.

The executable retains the 1.2.2 title while it is a preview. Full regression
tests and release publication are pending visual review. No physical PSP
hardware performance claim is made.
