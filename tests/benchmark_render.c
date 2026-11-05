#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
static uint32_t pixels[512 * 272];
static double stamp(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}
static int cmp(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
int main(int argc, char **argv) {
    (void)argv;
    int scrolling = argc > 1;
    const float pos[][2] = {{28, 27}, {280, 201}, {42, 45}, {302, 310}, {54, 322}};
    const char *names[] = {"Dusk Haven", "Shoguns Rest", "Outpost", "Snow", "Pine"};
    puts("scene,median_ms,p95_ms,worst_ms,frames,game_bytes,ui_bytes");
    for (int s = 0; s < 5; s++) {
        game_new(73819, 1);
        g.actors[0].x = (s == 0 || s == 2) ? pos[s][0] : pos[s][0] * 4 - 78;
        g.actors[0].y = (s == 0 || s == 2) ? pos[s][1] : pos[s][1] * 4 - 81;
        ui.screen = SCREEN_WORLD;
        double samples[240];
        for (int i = -20; i < 240; i++) {
            g.time += 1.f / 30;
            if (scrolling)
                g.actors[0].x += .02f;
            double a = stamp();
            render_game(pixels, 512);
            double b = stamp();
            if (i >= 0)
                samples[i] = (b - a) * 1000;
        }
        qsort(samples, 240, sizeof(double), cmp);
        printf("%s,%.3f,%.3f,%.3f,240,%zu,%zu\n", names[s], samples[120], samples[228], samples[239],
               sizeof(Game), sizeof(UI));
    }
    return 0;
}
