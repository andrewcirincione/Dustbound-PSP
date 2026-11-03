TARGET = dustbound
OBJS = src/psp_main.o src/game.o src/render.o src/content.o src/world.o src/expansion.o src/audio.o
PSPDEV ?= /usr/local/pspdev
PSPSDK := $(PSPDEV)/psp/sdk
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99 -DPLATFORM_PSP -Wno-missing-field-initializers -Wno-misleading-indentation
LIBS = -lm -lpsppower -lpspaudio
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Dustbound 0.2.1
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PIC1 = assets/PIC1.png
PSP_LARGE_MEMORY = 0
BUILD_PRX = 1
PSP_FW_VERSION = 660
include $(PSPSDK)/lib/build.mak

# Keep incremental builds correct after header or launcher artwork changes.
$(OBJS): src/game.h src/combat_pose.h
src/game.o: src/legacy.h
EBOOT.PBP: assets/ICON0.png assets/PIC1.png
