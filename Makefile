TARGET = dustbound
OBJS = src/psp_main.o src/game.o src/render.o src/content.o src/armor.o src/arsenal.o src/world.o src/expansion.o src/interior.o src/playability.o src/audio.o src/depth.o src/depth_combat.o src/society.o src/civilization.o src/limbs.o src/population.o src/politics.o
PSPDEV ?= /usr/local/pspdev
PSPSDK := $(PSPDEV)/psp/sdk
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99 -DPLATFORM_PSP -Wno-missing-field-initializers -Wno-misleading-indentation
LIBS = -lm -lpsppower -lpspaudio
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Dust 1.0.3
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PIC1 = assets/PIC1.png
PSP_EBOOT_PSAR = assets/DustAmbient.mp3
PSP_LARGE_MEMORY = 0
BUILD_PRX = 1
PSP_FW_VERSION = 660
include $(PSPSDK)/lib/build.mak

# Keep incremental builds correct after header or launcher artwork changes.
$(OBJS): src/game.h src/combat_pose.h src/combat_moves.h src/depth.h src/depth_types.h
src/game.o: src/legacy.h
EBOOT.PBP: assets/ICON0.png assets/PIC1.png $(PSP_EBOOT_PSAR)

PARAM.SFO: Makefile

src/render.o: src/menu_art.h src/art_data.h src/settlement_art.h src/structure_art.h src/furniture_art.h src/furniture_models.h src/settlement_art_ids.h src/painted_actor.h src/actor_attachments.h src/depth_render.h
src/render.o: src/armor_art_data.h src/armor_sets_data.h
src/armor.o: src/armor_sets_data.h
src/interior.o: src/settlement_art_ids.h src/furniture_models.h src/placement.inc

src/world.o: src/regional.h

src/world.o: src/living.h

$(OBJS): src/society.h src/society_types.h

$(OBJS): src/civilization.h src/weapon_design.h src/limbs.h

$(OBJS): src/population.h src/population_types.h

$(OBJS): src/life.h src/life_types.h
src/population.o: src/population_behavior.h src/settlement_art_ids.h

src/audio.o: src/vendor/minimp3.h

src/world.o: src/frontier_towns.h src/structure_models.h src/settlement_art_ids.h

src/arsenal.o: src/arsenal_data.h src/arsenal_grips_data.h
src/render.o: src/arsenal_art_data.h
