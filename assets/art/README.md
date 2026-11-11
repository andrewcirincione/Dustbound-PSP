# Original painted frontier artwork

The runtime uses thirteen original painted source atlases: buildings-v2, wilderness, furniture, props, torsos, heads, limbs, boots-backs, beasts, materials, weapons-blades, weapons-frontier and weapons-heavy. Their western/samurai briefs were created for Dust; no Kenshi game assets are included. The launcher-v041 illustration supplies the PSP launcher icon. A separate dust-storm illustration is used for the PSP wallpaper. The ruined-villages atlas supplies painted ruins for the trading post, inn, smithy and cottage.

Version 0.4.1 replaces all 32 geometric weapon families with painted sprites. Legacy items use the matching family art. Six material treatments and four quality grades preserve wood, leather, wrapping and rust while changing exposed metal. Warm source colours remain muted. Painted sprites, articulated body parts and rigid weapon pivots share the existing attack poses.

Run `python3 scripts/pack_art.py` with Pillow installed. It crops cells using alpha, calibrates building foundations, resizes and palette-quantizes the runtime sprites, and emits `src/art_data.h`. Weapon cells have explicit source-space handle anchors and scale metadata; do not replace these with the alpha bounding-box centre. Source atlases remain unchanged. The same compiler produces `assets/ICON0.png` from the launcher illustration. The finished wallpaper is checked in as `assets/PIC1.png`.

The 144 indexed sprites and 32 projected shadow masks occupy 554,886 bytes, plus small sprite/pivot descriptors. Lighting palettes occupy 147,456 bytes. No PNG decoder, network request or external art file is needed on PSP. Actor and menu caches have fixed capacities; see `docs/VALIDATION.md` for the complete memory budget.

The two weapon atlas prompts are recorded in `weapon-prompts.txt`. The built-in image generator produced the new source artwork; crop, resize, quantization and pivot calibration are reproducible asset-compilation steps. Runtime sprite previews are `sprite-00.png` through `sprite-143.png`.

Source art and derivatives are included under the repository license. Atlas images, launcher illustration and native review grids are not gameplay screenshots. `evidence/v041-*` playthrough captures document the actual EBOOT; the intentionally retained `iteration1` capture shows the preview clipping defect found before correction.

Version 0.5.0 adds four painted heavy-weapon sprites in `weapons-heavy.png`. Their generation brief is recorded in `heavy-weapon-prompt.txt`; the packer calibrates separate handle pivots and the runtime shares the same articulated grip. The existing body art and environment atlases were reviewed and retained. The new assets add 11,487 compiled sprite bytes and 4,096 palette bytes; fixed actor/portrait cache dimensions are unchanged.

Version 1.0.0 adds generated `stairs-down.png`: a transparent isometric timber floor opening with descending stairs, painted in the same worn frontier style as the inn furniture, viewed from above at the existing 2:1 floor perspective, with no roof or wall backdrop. The source is compiled as furniture model 19 by `scripts/pack_furniture_art.py`. The title menu compiles the existing generated ronin landscape in `assets/PIC1.png` through `scripts/pack_menu_art.py`; no generated landscape is redrawn with runtime shapes.
