CC=cc
CFLAGS=-O2 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc -MMD -MP
CORE=game society civilization limbs population politics content armor world expansion interior playability audio depth depth_combat
OBJS=$(addprefix build/v1000/obj/,$(addsuffix .o,$(CORE)))
all: build/v1000/dust-native
build/v1000/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
build/v1000/dust-native: $(OBJS) build/v1000/obj/render.o build/v1000/obj/native_main.o
	$(CC) $^ -lm -o $@
-include $(wildcard build/v1000/obj/*.d)
