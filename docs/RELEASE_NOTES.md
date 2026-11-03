# Dustbound 0.3.0 — The weathered frontier

A new painted frontier, walkable interiors and corrected sword follow-through for the PSP sandbox.

- Replaced flat building and wilderness art with original painted sprites; added indoor furnishings, textured ground, facade cutaways and warmer, weathered UI materials.
- Both hands share the weapon's rigid grip during every attack phase. Fingers draw over the handle; torso twist and crouching cannot detach the second hand.
- Every in-game controller prompt uses a PSP symbol or keycap. The written guide uses matching pictograms.
- Walk into houses, shops, inns, towers, smithies, workshops and depots. Furniture blocks movement; counters trade, beds heal, and workstations open forging, research or storage. Save and reload inside a room.
- Indexed row spans, cached lighting, scrolling terrain reuse and RAM framebuffer staging reduce rendering work. All 24 compiled sprites occupy 130,303 bytes, plus 24,576 bytes for lighting palettes.

Validated with 111 existing simulation checks, 25 new interior/save regressions, 13,312 grip poses, 272 render cases, sanitizers and 2,016 distinct gear renders at both scales. The exact release EBOOT passed normal, combat/outpost and interior playthroughs in PPSSPP. The independent GitHub build matches the playtested EBOOT checksum. The evidence archive contains native-resolution PSP screenshots and a combat recording.

Extract `Dustbound-0.3.0-PSP.zip` to the memory stick root on a PSP configured for homebrew. Version 3 saves preserve interiors and load version 1/2 expeditions; keep backups before upgrading. QA markers and test saves are excluded.

Still a playable alpha. Interiors are single rooms with services; multistorey layouts, indoor NPC schedules/combat and physical PSP-3000 qualification remain unfinished. The 30 Hz display target and improved host rendering measurements are not a hardware frame-rate guarantee.
