#include "game.h"
#include <stdint.h>
#include <stdio.h>
static uint32_t pixels[512 * 272 + 32];
int main(void) {
    game_new(73819, 0);
    character_begin(0);
    int frames = 0;
    for (int biomeIndex = 0; biomeIndex < 9; biomeIndex++) {
        const float pos[9][2] = {{26, 29},  {155, 42},  {140, 294}, {65, 215}, {322, 36},
                                 {235, 71}, {302, 310}, {54, 322},  {165, 150}};
        g.actors[0].x = pos[biomeIndex][0];
        g.actors[0].y = pos[biomeIndex][1];
        for (int screen = 0; screen <= SCREEN_LOOT; screen++) {
            ui.screen = screen;
            ui.selection = 0;
            ui.dialogActor = 1;
            ui.lootActor = 1;
            ui.storageIndex = 0;
            ui.tradeTown = 0;
            for (unsigned i = 0; i < sizeof(pixels) / sizeof(*pixels); i++)
                pixels[i] = 0xdeadbeef;
            render_game(pixels, 512);
            frames++;
            for (int y = 0; y < 272; y++)
                for (int x = 480; x < 512; x++)
                    if (pixels[y * 512 + x] != 0xdeadbeef) {
                        fprintf(stderr, "Stride overrun screen %d biome %d\n", screen, biomeIndex);
                        return 1;
                    }
            for (int i = 512 * 272; i < 512 * 272 + 32; i++)
                if (pixels[i] != 0xdeadbeef) {
                    fprintf(stderr, "Framebuffer overrun\n");
                    return 1;
                }
        }
    }
    for (int page = 0; page < GUIDE_PAGES; page++) {
        ui.screen = SCREEN_HELP;
        ui.selection = page;
        render_game(pixels, 512);
        frames++;
    }
    printf("PASS: %d screen/biome/guide renders; 480x272 bounds and 512-pixel stride preserved.\n", frames);
    return 0;
}
