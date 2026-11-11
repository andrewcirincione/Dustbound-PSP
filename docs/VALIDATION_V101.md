# Dust 1.0.1 validation

The shared anatomy change covers all sixteen torso styles, three human body
types, species width differences, both facings, and passive, combat, resting
and crawling poses. Armor uses collar, waist and arm-opening landmarks rather
than its image bounds. Upper arms and forearms are shorter; the brigandine is
narrower and centred. Naginata support placement is behind the primary hand.

Current package qualification: all 41 native suites passed and the PSP build
completed in GitHub Actions. The downloaded executable's SHA-256 is
`5e79af85108f09303f41f4c5eb6fc109e3105977e5882e82fa6374c6ca39b554`.
Its PBP sections, embedded art/music and standard-memory SFO validate.
The runtime is unchanged from that successful build. Build and artifact
provenance are recorded in [BUILD_V101.json](BUILD_V101.json).

Fresh emulator verification is pending. The initial attempt exposed incorrect
memory-stick and resource paths in the harness. Those paths were corrected,
but GitHub has not started the retry and reported an internal startup error.
The recovered package contains `psp-controller.json` with `ok: false` and
`status: pending`; its review images are native compositor captures, not PSP
emulator screenshots. The user approved these pose galleries before publishing.

The automated workflow retains the full verification sequence:

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

The default packager still requires successful emulator telemetry. For this
runner failure, `python scripts/package_v101.py --recovered-build` instead
requires the exact recorded CI executable, unchanged runtime sources and all
41 passing native suites, and explicitly records the pending emulator result.
It does not mark those checks as passed. The save format remains v29. Physical
PSP performance and audible output are not qualified by these checks.
