#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    game_new(73819, 0);
    Actor *a = &g.actors[0];
    a->autoattack = 0;
    ui.screen = SCREEN_WORLD;
    ui.waypoint = 1;
    ui.travelX = a->x + 2;
    ui.travelY = a->y + 10;
    float x = a->x, y = a->y;
    for (int i = 0; i < 100; i++)
        game_tick(.05f, (Input){0});
    assert(a->x == x && a->y == y && ui.waypoint);
    assert(ui.travelX == x + 2 && ui.travelY == y + 10);

    ui.feedCount = ui.logCount = 0;
    for (int i = 0; i < 8; i++)
        log_msg("Routine message %d for the frontier.", i);
    log_warn("Urgent injury.");
    int priority, offset;
    assert(!strcmp(log_current(&priority, &offset), "Urgent injury.") && priority == 2);
    for (int i = 0; i < 60; i++)
        log_tick(.05f);
    assert(ui.feedCount < 7 && ui.logCount == 9);
    puts("PASS: map pins remain manual; urgent alerts surface immediately and crowded ticker drains faster.");
    return 0;
}
