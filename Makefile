TARGET = dustbound
OBJS = src/psp_main.o src/game.o src/render.o src/content.o src/world.o src/expansion.o src/interior.o src/playability.o src/audio.o src/depth.o src/depth_combat.o src/society.o src/civilization.o src/limbs.o src/population.o src/politics.o
PSPDEV ?= /usr/local/pspdev
PSPSDK := $(PSPDEV)/psp/sdk
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99 -DPLATFORM_PSP -Wno-missing-field-initializers -Wno-misleading-indentation
LIBS = -lm -lpsppower -lpspaudio
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Dustbound 0.17.4
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PIC1 = assets/PIC1.png
PSP_LARGE_MEMORY = 0
BUILD_PRX = 1
PSP_FW_VERSION = 660
include $(PSPSDK)/lib/build.mak

# Keep incremental builds correct after header or launcher artwork changes.
$(OBJS): src/game.h src/combat_pose.h src/combat_moves.h src/depth.h src/depth_types.h
src/game.o: src/legacy.h
EBOOT.PBP: assets/ICON0.png assets/PIC1.png

PARAM.SFO: Makefile

src/render.o: src/art_data.h src/painted_actor.h src/depth_render.h

src/world.o: src/regional.h

src/world.o: src/living.h

$(OBJS): src/society.h src/society_types.h

$(OBJS): src/civilization.h src/weapon_design.h src/limbs.h

$(OBJS): src/population.h src/population_types.h

$(OBJS): src/life.h src/life_types.h
src/population.o: src/population_behavior.h
