# Character pose review (draft)

Based on published v1.0.0, commit d115b2e4d3ed0b17a19308c1cab2d50447596af7.
The user supplied an edited character reference and requested varied passive
and combat previews, including two-handed weapons, before a release.

Current changes:

- Smaller head and fitted headwear; shared movement offset for torso and hips.
- Shared arm proportions across every torso, body type, weapon and stance:
  5.4-pixel upper/forearms instead of 6.3, a 10.6-pixel reachable wrist, and
  a 9.2-pixel relaxed hang. Resting/crawling use the same limb materials and
  painted attachment anchors.
- Each generated garment is registered at its painted collar, waist and two
  arm openings. The two image halves meet at one spine and fit onto the shared
  anatomical shoulders. Arms no longer attach to offsets inferred from the
  sprite bounding box. The brigandine chest is narrower and centred above the
  pelvis, with the same socket positions as the other outfits.
- Internal painted joint pivots for upper arms and forearms, preserving opaque
  overlap at elbows and wrists instead of connecting transparent sprite edges.
- Relaxed arms hang beside the torso. Gripping hands rotate with the forearm.
- The complete far arm, gripping hand and held weapon draw before the torso.
  The near arm draws in front. The former far-forearm overlay is removed.
- Intact left/right figures mirror one visual staging, including hand ownership,
  texture parity and leg order. Missing-arm fallback uses the remaining hand.
- The lamellar chest is narrower in standing, grounded and crawling poses.
- Lower melee guards and a closer vertical back mount for stowed weapons.
- Naginata/polearms restore the original support grip behind the main hand.
  The added forward support offset made their arms cross. Swords and clubs
  retain a hilt grip.
- Projected arm segments have equal lengths, permitting a complete fold at the
  shoulder. Unequal lengths previously produced an unbounded elbow when the
  wrist passed inside their minimum reach; the scattergun review exposed it.
- Elbows use a continuous bend branch. Compact authored hand paths replace the
  wide movements that swung arms across the body in the first preview. All
  weapon profiles were checked, rather than only the three reported examples.
- The two painted views retain their side-view hand reach when aiming north or
  south. Multiplying that reach by the target's screen cosine made diagonal
  attacks drag the wrist through the shoulder. Bows raise toward their aim
  before drawing the string, with a curved raising/lowering path.
- Grounded arms share the same painted pivots and armor materials.
- Unarmored figures use the generated bare skin part instead of the wrapped
  sleeve, with the abdomen meeting the upper trousers in a combat stance.

Review reproduction:

1. Build native core with `make -f scripts/native_v1000.mk -j4`.
2. Compile `tests/review_character_poses.c` with native core objects excluding
   `render.o` and `native_main.o`, because the generator includes `render.c`.
3. Run the generator with an existing output parent, then
   `python scripts/character_pose_review.py <frames> <previews> --previous <previous-frames>`.

The sheets cover all 16 torso silhouettes. Catalog captures include 39
weapon/unarmed sets, three attack variations and both facings. The main GIFs
cycle all three attacks with both facings side by side. Comparison captures use
the same variation as the earlier preview. GIFs are slowed pose cycles using
the production compositor's portrait source. The focused comparison shows the
prior passive pose beside the revised passive and fight poses; it deliberately
crops weapon tips to keep the shoulder joins large. These are native
renderer outputs; this change has not been packaged or tested in PPSSPP yet.

Validation includes painted-joint coverage (10,880 elbow cases and 768 wrist
cases), opaque torso occlusion across 16 armor silhouettes and 39 weapon/unarmed
sets, and 468 raster reflection comparisons. Arm-motion observation calls the
production renderer with an empty paint clip: 1,825,200 geometry samples cover
three attacks, 16 directions, three human body types and the broad/slim species
extremes. The maximum elbow movement is .748 world pixels per 1/64 phase sample,
below the .75 regression bound. The animation continuity/timing/cache suite,
attachment suite and 575-screen bounds suite are also required. The revised
previews were visually reviewed and approved for 1.0.1. The release workflow
additionally requires a PSP build and controller checks of the exact executable
before publishing.
