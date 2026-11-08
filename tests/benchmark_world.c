/* Compare native or PSP simulation and render work on the same prepared scenes.
   On PSP, substitute this for psp_main.c in a scratch build. PPSSPP reports
   emulated CPU time. Both clocks exclude presentation and frame waits. */
#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PLATFORM_PSP
#include <pspkernel.h>
#include <psppower.h>
PSP_MODULE_INFO("Dustbound benchmark", 0, 0, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12 * 1024);
static uint64_t timestamp(void) { return sceKernelGetSystemTimeWide(); }
#else
#include <time.h>
static uint64_t timestamp(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
}
#endif

enum { WARMUP = 20, SAMPLES = 160 };
static uint32_t pixels[512 * 272 + 16];
static uint32_t tick_us[SAMPLES], draw_us[SAMPLES], total_us[SAMPLES];

static int order_samples(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static uint32_t pixel_digest(uint32_t hash) {
    for (int y = 0; y < 272; y++)
        for (int x = 0; x < 480; x++)
            hash = (hash ^ pixels[y * 512 + x]) * 16777619u;
    return hash;
}
static uint32_t state_digest(void) {
    memset(g.society.weaponXP, 0, sizeof(g.society.weaponXP));
    for (int i = 0; i < POP_GROUPS; i++)
        for (int j = 0; j < POP_MEMBERS; j++)
            memset(g.population.social[i][j].weaponXP, 0, sizeof(g.population.social[i][j].weaponXP));
    const unsigned char *p = (const unsigned char *)&g;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < sizeof(g); i++) hash = (hash ^ p[i]) * 16777619u;
    return hash;
}
extern void init_actor(int n, const char *name, int kind, int faction, float x, float y, int skill);
static void set_scene(int scene) {
    game_new(73819, 1);
    ui.screen = SCREEN_WORLD;
    g.dayTime = 12;
    Actor *a = &g.actors[0];
    a->autoattack = 0;
    if (scene == 0) { a->x = 27; a->y = 29; }
    if (scene == 1 || scene == 2) { a->x = 160; a->y = 352; }
    if (scene == 2) {
        const int families[] = {0, 1, 84, 5, 14, 9, 29, 28};
        for (int i = 1; i < g.actorCount; i++) g.actors[i].active = 0;
        for (int i = 1; i <= 20; i++) {
            init_actor(i, "Sparring fighter", i % 2 ? RAIDER : GUARD, i % 2 ? HOSTILE : UNION,
                       a->x + (i % 5 - 2) * 1.02f, a->y + (i / 5 - 2) * 1.09f, 26);
            g.actors[i].gear[0] = gear_id(families[i % 8], 1, 1);
            inventory_add(&g.actors[i], BOLTS, 90);
            inventory_add(&g.actors[i], CARTRIDGE, 90);
        }
        a->gear[0] = gear_id(84, 1, 1);
        a->autoattack = 1;
        a->xp[STRENGTH] = 12 * 45 * 45;
    }
    if (scene == 3) {
        for (int i = 0; i < g.buildingCount; i++) {
            if (g.buildings[i].type != BAR) continue;
            a->x = g.buildings[i].x;
            a->y = g.buildings[i].y;
            depth_enter_room(i);
            break;
        }
    }
}
int main(void) {
#ifdef PLATFORM_PSP
    scePowerSetClockFrequency(333, 333, 166);
#endif
    FILE *out = fopen("perf.csv", "w");
    if (!out) return 1;
    fprintf(out, "scene,tick_median_us,render_median_us,render_p95_us,total_median_us,total_p95_us,total_worst_us,pixel_digest,state_digest\n");
    const char *names[] = {"Town", "Scrolling frontier", "Crowded fight", "Interior"};
    for (int scene = 0; scene < 4; scene++) {
        set_scene(scene);
        uint32_t hash = 2166136261u;
        for (int i = -WARMUP; i < SAMPLES; i++) {
            if (scene == 1) {
                g.actors[0].x += .067f;
                g.actors[0].y -= .043f;
            }
            uint64_t start = timestamp();
            game_tick(1.f / 30, (Input){0});
            uint64_t middle = timestamp();
            render_game(pixels, 512);
            uint64_t end = timestamp();
            if (i >= 0) {
                tick_us[i] = middle - start;
                draw_us[i] = end - middle;
                total_us[i] = end - start;
                hash = pixel_digest(hash);
            }
        }
        qsort(tick_us, SAMPLES, sizeof(tick_us[0]), order_samples);
        qsort(draw_us, SAMPLES, sizeof(draw_us[0]), order_samples);
        qsort(total_us, SAMPLES, sizeof(total_us[0]), order_samples);
        fprintf(out, "%s,%u,%u,%u,%u,%u,%u,%08x,%08x\n", names[scene],
                tick_us[SAMPLES / 2], draw_us[SAMPLES / 2], draw_us[SAMPLES * 95 / 100],
                total_us[SAMPLES / 2], total_us[SAMPLES * 95 / 100], total_us[SAMPLES - 1],
                hash, state_digest());
        fflush(out);
    }
    fclose(out);
#ifdef PLATFORM_PSP
    sceKernelExitGame();
#endif
    return 0;
}
