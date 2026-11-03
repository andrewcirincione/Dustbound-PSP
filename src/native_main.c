#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
static uint32_t pixels[480 * 272];
static void ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P6\n480 272\n255\n");
    for (int i = 0; i < 480 * 272; i++) {
        unsigned char c[3] = {pixels[i] & 255, (pixels[i] >> 8) & 255, (pixels[i] >> 16) & 255};
        fwrite(c, 1, 3, f);
    }
    fclose(f);
}
int main(int argc, char **argv) {
    game_new(73819, 0);
    if (argc > 1 && strcmp(argv[1], "--frame") == 0) {
        if (argc > 3)
            ui.screen = atoi(argv[3]);
        render_game(pixels, 480);
        ppm(argv[2]);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--fixtures") == 0) {
        set_save_root(argc > 2 ? argv[2] : ".");
        game_new(73819, 1);
        Actor *a = &g.actors[0];
        a->gear[0] = SABRE;
        a->gear[1] = PLATE;
        a->gear[2] = HELM;
        a->gear[3] = BOOTS;
        a->x = 36;
        a->y = 35;
        g.actors[1].x = 35;
        g.actors[1].y = 36;
        g.actors[2].x = 35;
        g.actors[2].y = 35;
        for (int i = 0; i < g.actorCount; i++)
            if (g.actors[i].kind == RAIDER) {
                g.actors[i].x = 37.3f;
                g.actors[i].y = 35;
                break;
            }
        ui.screen = SCREEN_WORLD;
        save_slot(2);
        game_new(73819, 1);
        a = &g.actors[0];
        a->x = 42;
        a->y = 45;
        a->gear[0] = SABRE;
        a->gear[1] = PLATE;
        a->gear[2] = HELM;
        a->gear[3] = BOOTS;
        g.actors[1].x = 43;
        g.actors[1].y = 46;
        g.actors[2].x = 41;
        g.actors[2].y = 46;
        g.tech = 3;
        inventory_add(a, WOOD, 80);
        inventory_add(a, IRONBAR, 40);
        inventory_add(a, WATER, 10);
        inventory_add(a, GRAIN, 12);
        inventory_add(a, RELIC, 4);
        const float sites[8][2] = {{44, 44}, {40, 44}, {48, 44}, {44, 48},
                                   {40, 48}, {48, 48}, {48, 40}, {44, 40}};
        for (int i = 0; i < 8; i++) {
            a->x = sites[i][0] - 1.9f;
            a->y = sites[i][1];
            build_structure(i);
        }
        a->x = 44;
        a->y = 46;
        for (int i = 0; i < g.actorCount; i++) {
            Actor *enemy = &g.actors[i];
            if (enemy->faction == HOSTILE && distance(enemy->x, enemy->y, 44, 46) < 20) {
                enemy->x = enemy->homeX = enemy->tx = 110;
                enemy->y = enemy->homeY = enemy->ty = 110;
            }
        }
        for (int i = 0; i < g.buildingCount; i++)
            if (g.buildings[i].owner >= 0 && g.buildings[i].type == FARM) {
                g.buildings[i].progress = 90;
                g.buildings[i].stock = 1;
            }
        ui.screen = SCREEN_WORLD;
        save_slot(3);
        return 0;
    }
#ifdef USE_SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window *w = SDL_CreateWindow("Dustbound - native development", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, 960, 544, 0);
    SDL_Renderer *r = SDL_CreateRenderer(w, -1, SDL_RENDERER_SOFTWARE);
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 480, 272);
    int run = 1, old = 0;
    ui.screen = SCREEN_TITLE;
    unsigned last = SDL_GetTicks();
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT)
                run = 0;
        const Uint8 *k = SDL_GetKeyboardState(NULL);
        int held = 0;
        int keys[] = {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,   SDL_SCANCODE_RIGHT,
                      SDL_SCANCODE_X,  SDL_SCANCODE_C,    SDL_SCANCODE_Z,      SDL_SCANCODE_V,
                      SDL_SCANCODE_Q,  SDL_SCANCODE_E,    SDL_SCANCODE_RETURN, SDL_SCANCODE_TAB};
        for (int i = 0; i < 12; i++)
            if (k[keys[i]])
                held |= 1 << i;
        unsigned now = SDL_GetTicks();
        game_tick((now - last) / 1000.0f, (Input){held, held & ~old, 0, 0});
        last = now;
        old = held;
        render_game(pixels, 480);
        SDL_UpdateTexture(t, NULL, pixels, 480 * 4);
        SDL_RenderCopy(r, t, NULL, NULL);
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
    SDL_Quit();
#endif
    return 0;
}
