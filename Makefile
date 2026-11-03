TARGET = dustbound
OBJS = src/psp_main.o src/game.o src/render.o
PSPDEV ?= /usr/local/pspdev
PSPSDK := $(PSPDEV)/psp/sdk
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99 -ffast-math
LIBS = -lm -lpsppower
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Dustbound 0.1.0
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PIC1 = assets/PIC1.png
PSP_LARGE_MEMORY = 0
BUILD_PRX = 1
PSP_FW_VERSION = 660
include $(PSPSDK)/lib/build.mak
