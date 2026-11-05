# Original frontier artwork

Three original generated RGBA atlases provide eight buildings, eight wilderness elements and eight indoor furnishings. They were created for Dustbound from written western/samurai frontier briefs, without source art from Kenshi or another game. The wilderness atlas received a separate background-removal pass after runtime QA found a white matte in the first generation.

`buildings.png`, `wilderness.png` and `furniture.png` are the source atlases. Each is a four-column, two-row grid. `sprite-00.png` through `sprite-23.png` are cropped, resized and palette-quantized runtime previews. `budget.json` records the exact dimensions and ROM footprint.

Run `python3 scripts/pack_art.py` from the repository root with Pillow installed. It crops cells by alpha, downsamples with Lanczos, quantizes to 256 colors, and emits `src/art_data.h`. The compiled atlas occupies 130,303 bytes; palette lighting caches use 24,576 bytes. Transparent row margins are omitted. Only soft edge texels blend; opaque texels are direct writes. No PNG decoder, image-loading allocation, network request or external art file is needed on PSP.

Characters and equipment remain original procedural, articulated drawings so all six gear slots, body parts and 2,016 material/grade variants can animate without thousands of full sprite sheets. Runtime buildings use cutaway dithering when they cover the selected survivor. Furniture and actors are sorted by floor depth inside rooms.

Source art and generated derivatives are included under the repository license. A screenshot of an asset atlas is not a gameplay capture; the `evidence/v030-*` PSP captures document the actual executable.
