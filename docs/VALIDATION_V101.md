# Dust 1.0.1 validation

The shared anatomy change covers all sixteen torso styles, three human body
types, species width differences, both facings, and passive, combat, resting
and crawling poses. Armor uses collar, waist and arm-opening landmarks rather
than its image bounds. Upper arms and forearms are shorter; the brigandine is
narrower and centred. Naginata support placement is behind the primary hand.

The release workflow is a required sequence:

1. Run all 41 current native suites, including 575 screen renders, archived-save
   compatibility, painted joint coverage, torso occlusion and reflected poses.
   Arm-motion observation covers 39 weapon/unarmed sets, three attacks, sixteen
   directions and five body/species combinations. A prior rigid-grip fixture
   now expects the intentionally shorter 3.2-pixel kanabo/tetsubo hand spacing.
2. Build the PSP executable with verified PSPDEV v20261101, then validate the
   executable, icon, wallpaper, embedded DustAmbient.mp3 and standard-memory SFO.
3. In official PPSSPP 1.20.4, boot that EBOOT and load ordinary prepared saves.
   Exercise sabre, greatsword, naginata, unarmed and tetsubo attacks and passive
   poses; walk and open inventory using emulated controller input. Capture the
   actual completed PSP framebuffer and require version 1.0.1 telemetry.
4. Rebuild the enlarged review galleries with the production native compositor.
   Cover all sixteen torso silhouettes and thirty-nine weapon/unarmed sets,
   with three attack variations and both facings. These are labeled renderer
   galleries, distinct from the PPSSPP screenshots.
5. Package only the EBOOT whose hash matches the successful emulator run.
   Verify the installed ZIP contains those exact bytes and no QA saves or
   markers. Record runtime source hashes, commit, native results and checksums.
   Verify GitHub's uploaded asset digests before publishing the draft release.

The Evidence ZIP records the results of this sequence. The native compositor
galleries were visually reviewed before release authorization. The save format
remains v29. Physical PSP performance and audible output are not qualified by
these checks.
