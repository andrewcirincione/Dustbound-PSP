# Original painted frontier artwork

The runtime uses twelve original painted source atlases: buildings-v2, wilderness, furniture, props, torsos, heads, limbs, boots-backs, beasts, materials, weapons-blades and weapons-frontier. Their western/samurai briefs were created for Dustbound; no Kenshi game assets are included. The separate launcher-v041 illustration supplies the PSP launcher icon and background.

Version 0.4.1 replaces all 32 geometric weapon families with painted sprites. Legacy items use the matching family art. Six material treatments and four quality grades preserve wood, leather, wrapping and rust while changing exposed metal. Warm source colours remain muted. Painted sprites, articulated body parts and rigid weapon pivots share the existing attack poses.

Run `python3 scripts/pack_art.py` with Pillow installed. It crops cells using alpha, calibrates building foundations, resizes and palette-quantizes the runtime sprites, and emits `src/art_data.h`. Weapon cells have explicit source-space handle anchors and scale metadata; do not replace these with the alpha bounding-box centre. Source atlases remain unchanged. The same compiler produces `assets/ICON0.png` and `assets/PIC1.png` from the launcher illustration.

The 140 indexed sprites and 32 projected shadow masks occupy 543,399 bytes, plus small sprite/pivot descriptors. Lighting palettes occupy 143,360 bytes. No PNG decoder, network request or external art file is needed on PSP. Actor and menu caches have fixed capacities; see `docs/VALIDATION.md` for the complete memory budget.

The two weapon atlas prompts are recorded in `weapon-prompts.txt`. The built-in image generator produced the new source artwork; crop, resize, quantization and pivot calibration are reproducible asset-compilation steps. Runtime sprite previews are `sprite-00.png` through `sprite-139.png`.

Source art and derivatives are included under the repository license. Atlas images, launcher illustration and native review grids are not gameplay screenshots. `evidence/v041-*` playthrough captures document the actual EBOOT; the intentionally retained `iteration1` capture shows the preview clipping defect found before correction.
