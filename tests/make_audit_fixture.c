#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint32_t pixels[480 * 272];
static void init(void) {
    game_new(73819, 1);
    character_begin(4);
    character_finish();
    for (int i = 1; i < g.actorCount; i++) {
        g.actors[i].active = 0;
        g.actors[i].squad = 0;
    }
    g.squadCount = 1;
    g.nodeCount = g.buildingCount = 0;
    g.baseExists = 0;
    g.weather = 0;
    g.weatherTimer = g.eventTimer = 9999;
    g.actors[0].x = 44;
    g.actors[0].y = 44;
    g.actors[0].autoattack = 0;
    ui.screen = SCREEN_WORLD;
    g.money = 2500;
    g.tech = 3;
}
static void frame(const char *dir, int screen) {
    char path[512];
    snprintf(path, sizeof(path), "%s/ui-%02d.ppm", dir, screen);
    FILE *f = fopen(path, "wb");
    if (!f)
        exit(2);
    render_game(pixels, 480);
    fprintf(f, "P6\n480 272\n255\n");
    for (int i = 0; i < 480 * 272; i++) {
        unsigned char c[] = {pixels[i] & 255, (pixels[i] >> 8) & 255, (pixels[i] >> 16) & 255};
        fwrite(c, 1, 3, f);
    }
    fclose(f);
}
int main(int argc, char **argv) {
    if (argc < 2)
        return 2;
    set_save_root(argv[1]);
    init();
    Actor *a = &g.actors[0], *b = &g.actors[1];
    b->active = b->squad = 1;
    b->kind = PLAYER;
    b->faction = FREE;
    b->x = 44.8f;
    b->y = 44;
    b->unconscious = 900;
    b->health[CHEST] = -5;
    b->health[LARM] = 12;
    b->bleed = 1.5;
    b->blood = 65;
    b->autoattack = 0;
    b->job = JOB_HOLD;
    b->carriedBy = -1;
    g.squadCount = 2;
    snprintf(b->name, sizeof(b->name), "WOUNDED SCOUT");
    inventory_add_owned(b, gear_id(86, 1, 1), 1, SHOGUN);
    if (!save_slot(2))
        return 2;
    init();
    a = &g.actors[0];
    memset(a->pack, 0, sizeof(a->pack));
    inventory_add(a, MEDKIT, 4);
    inventory_add(a, WOOD, 18);
    inventory_add(a, IRONBAR, 30);
    inventory_add(a, CLOTH, 12);
    inventory_add(a, RELIC, 4);
    inventory_add(a, COAL, 4);
    a->xp[SMITHING] = 12 * 40 * 40;
    a->xp[COOKING] = 12 * 39 * 39;
    g.baseExists = 1;
    g.baseX = 42;
    g.baseY = 44;
    g.raidTimer = 9999;
    g.buildingCount = 4;
    g.buildings[0] = (Building){.x = 42, .y = 44, .type = STORE, .owner = 0};
    g.buildings[0].storage[0] = (Stack){GRAIN, 30};
    g.buildings[0].storage[1] = (Stack){WATER, 10};
    g.buildings[1] = (Building){.x = 48, .y = 44, .type = CAMPFIRE, .owner = 0};
    g.buildings[2] = (Building){.x = 48, .y = 49, .type = FORGE, .owner = 0};
    g.buildings[3] = (Building){.x = 42, .y = 49, .type = BENCH, .owner = 0};
    if (!save_slot(3))
        return 2;
    init();
    a = &g.actors[0];
    b = &g.actors[1];
    b->active = 1;
    b->squad = 0;
    b->kind = RAIDER;
    b->faction = HOSTILE;
    b->dead = 1;
    b->unconscious = 9999;
    b->x = 44.8f;
    b->y = 44;
    b->bountyReward = 0;
    snprintf(b->name, sizeof(b->name), "VULTURE REAVER");
    memset(b->pack, 0, sizeof(b->pack));
    memset(b->stolen, 0, sizeof(b->stolen));
    b->pack[0] = (Stack){gear_id(84, 1, 1), 1};
    b->pack[1] = (Stack){gear_id(78, 0, 1), 1};
    b->pack[2] = (Stack){IRONBAR, 12};
    b->stolen[2] = SHOGUN;
    b->gear[0] = gear_id(86, 1, 1);
    if (!save_slot(4))
        return 2;
    /* Screens are native renders for exhaustive layout review, not emulator evidence. */
    if (argc > 2)
        for (int s = 0; s <= SCREEN_LOG; s++) {
            ui.screen = s;
            ui.selection = 0;
            ui.noticeTime = 0;
            ui.dialogActor = 1;
            ui.lootActor = 1;
            ui.storageIndex = 0;
            ui.tradeTown = 0;
            ui.mapX = a->x;
            ui.mapY = a->y;
            if (s == SCREEN_SKILLS)
                ui.selection = 16;
            if (s == SCREEN_HELP)
                ui.selection = 11;
            if (s == SCREEN_PACK) {
                inventory_add_owned(a, gear_id(78, 0, 1), 1, SHOGUN);
                ui.selection = 6;
            }
            frame(argv[2], s);
        }
    puts("Audit fixture slots 2=rescue, 3=camp logistics, 4=loot generated.");
    return 0;
}
