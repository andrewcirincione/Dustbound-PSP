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
        g.actors[0].x = biomeIndex ? pos[biomeIndex][0] * 4 - 78 : pos[biomeIndex][0];
        g.actors[0].y = biomeIndex ? pos[biomeIndex][1] * 4 - 81 : pos[biomeIndex][1];
        for (int screen = 0; screen <= SCREEN_PLACE; screen++) {
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
    ui.screen = SCREEN_WORLD;
    const int roomTypes[] = {HOUSE, SHOP, BAR, TOWER, FORGE, BENCH, STORE};
    for (int i = 0; i < 7; i++)
        for (int pose = 0; pose < 4; pose++) {
            g.buildings[0].type = roomTypes[i];
            g.interiorBuilding = 1;
            g.interiorActor = g.selected;
            g.interiorX = 3.5f;
            g.interiorY = 4.8f;
            g.actors[g.selected].taskKind = pose == 3 ? 3 : 0;
            for (unsigned z = 0; z < sizeof(pixels) / sizeof(*pixels); z++)
                pixels[z] = 0xdeadbeef;
            render_game(pixels, 512);
            frames++;
            for (int y = 0; y < 272; y++)
                for (int x = 480; x < 512; x++)
                    if (pixels[y * 512 + x] != 0xdeadbeef)
                        return 1;
            for (int z = 512 * 272; z < 512 * 272 + 32; z++)
                if (pixels[z] != 0xdeadbeef)
                    return 1;
        }
    for(int race=0;race<SPECIES;race++)for(int body=0;body<3;body++)for(int pose=0;pose<3;pose++){
        g.interiorBuilding=0;character_begin(0);ui.createRace=race;ui.createBody=body;
        if(pose){ui.screen=SCREEN_PACK;g.actors[g.selected].race=race;g.actors[g.selected].body=body;g.actors[g.selected].unconscious=pose==2?10:0;}
        for(unsigned z=0;z<sizeof(pixels)/sizeof(*pixels);z++)pixels[z]=0xdeadbeef;
        render_game(pixels,512);frames++;
        for(int y=0;y<272;y++)for(int x=480;x<512;x++)if(pixels[y*512+x]!=0xdeadbeef)return 1;
        for(int z=512*272;z<512*272+32;z++)if(pixels[z]!=0xdeadbeef)return 1;
    }
    printf("PASS: %d screen/biome/guide renders; 480x272 bounds and 512-pixel stride preserved.\n", frames);
    return 0;
}
