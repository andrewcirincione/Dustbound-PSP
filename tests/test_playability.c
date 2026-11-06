#include "game.h"
#include "fixtures/save_v4_layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static int checks;
#define CHECK(c)                                                                                             \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c);                                                  \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)
static void fresh(void) {
    game_new(73819, 0);
    ui.screen = SCREEN_WORLD;
    g.actors[0].autoattack = 0;
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
}
static void step(float secs) {
    for (int i = 0; i < (int)(secs * 30); i++)
        simulate(1.f / 30);
}
static uint32_t hash(const void *p, size_t n) {
    const unsigned char *b = p;
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ *b++) * 16777619u;
    return h;
}
static int has_action(int i, int action) {
    for (int r = 0; r < dialogue_count(i); r++)
        if (dialogue_action(i, r) == action)
            return 1;
    return 0;
}
int main(void) {
    for (int origin = 0; origin < 5; origin++) {
        game_new(73819, 0);
        character_begin(origin);
        character_finish();
        Actor *starter = &g.actors[0];
        CHECK(encumbrance(starter) <= 30 + level(starter, STRENGTH) * 2 + gear_bonus(starter, 3));
    }
    fresh();
    Actor *a = &g.actors[0];
    g.buildingCount = 0;
    g.nodeCount = 1;
    g.nodes[0] = (Node){.x = 100, .y = 100, .type = NODE_ORE, .remaining = 20};
    a->x = 102.5f;
    a->y = 100;
    find_context();
    CHECK(ui.contextKind == 1 && ui.contextIndex == 0 && context_distance() > context_reach());
    int ore = inventory_count(a, ORE);
    interact();
    step(4);
    CHECK(!a->taskKind && inventory_count(a, ORE) == ore);
    a->x = 100.8f;
    interact();
    CHECK(a->taskKind == 1);
    step(3);
    CHECK(inventory_count(a, ORE) > ore);
    ore = inventory_count(a, ORE);
    interact();
    a->x = 103;
    step(4);
    CHECK(!a->taskKind && inventory_count(a, ORE) == ore);
    a->x = 102;
    a->job = JOB_MINE;
    step(12);
    CHECK(inventory_count(a, ORE) > ore && distance(a->x, a->y, 100, 100) <= WORK_REACH);
    g.nodes[0].locked = 1;
    g.nodes[0].type = NODE_LOCKED;
    a->job = JOB_HOLD;
    a->taskKind = 0;
    a->x = 102;
    interact();
    CHECK(!a->taskKind && g.nodes[0].locked);
    a->x = 100.5f;
    interact();
    CHECK(a->taskKind == 2);
    a->x = 104;
    step(6);
    CHECK(g.nodes[0].locked && !a->taskKind);
    fresh();
    int roles[10] = {0};
    for (int i = 1; i < g.actorCount; i++) {
        Actor *n = &g.actors[i];
        if (n->faction == HOSTILE || n->kind == BEAST || depth_named(i)>=0)
            continue;
        int r = npc_role(n);
        roles[r]++;
        CHECK(dialogue_count(i) >= 2 && dialogue_count(i) <= 5);
        CHECK(has_action(i, 0) == (r == ROLE_MERCHANT));
        CHECK(has_action(i, 2) == (r == ROLE_INNKEEPER));
        CHECK(has_action(i, 3) == (r == ROLE_MEDIC));
        CHECK(has_action(i, 7) == (r == ROLE_SMITH));
        CHECK(has_action(i, 5) == (r == ROLE_BARBER));
        CHECK(has_action(i, 1) == (r == ROLE_MARSHAL));
    }
    CHECK(roles[ROLE_MERCHANT] && roles[ROLE_MEDIC] && roles[ROLE_RECRUIT] && roles[ROLE_SMITH] &&
          roles[ROLE_INNKEEPER]);
    int recruit = 1;
    while (npc_role(&g.actors[recruit]) != ROLE_RECRUIT)
        recruit++;
    dialogue_open(recruit);
    int money = g.money;
    game_tick(.033f, (Input){KEY_CROSS, KEY_CROSS, 0, 0});
    CHECK(g.actors[recruit].squad && g.money == money - 300 && ui.screen == SCREEN_WORLD);
    fresh();
    a = &g.actors[0];
    Actor *b = &g.actors[1];
    *b = *a;
    b->x = a->x + 1;
    b->active = 1;
    b->squad = 1;
    g.squadCount = 2;
    memset(a->pack, 0, sizeof(a->pack));
    memset(b->pack, 0, sizeof(b->pack));
    a->hunger = 30;
    b->hunger = 100;
    inventory_add(b, RATION, 2);
    step(.1f);
    CHECK(a->hunger > 70 && inventory_count(b, RATION) == 1);
    a->hunger = 30;
    b->x = a->x + 4;
    step(.1f);
    CHECK(a->hunger < 40 && inventory_count(b, RATION) == 1);
    b->x = a->x + 1;
    b->gear[5] = 0;
    step(.1f);
    CHECK(a->hunger < 40 && inventory_count(b, RATION) == 1);
    b->gear[5] = gear_id(76, 0, 0);
    g.interiorBuilding = 1;
    g.interiorActor = 0;
    step(.1f);
    CHECK(a->hunger < 40 && inventory_count(b, RATION) == 1);
    g.interiorBuilding = 0;
    b->active = 0;
    g.buildingCount = 1;
    g.buildings[0] = (Building){.x = a->x + 1, .y = a->y, .type = STORE, .owner = 0};
    g.buildings[0].storage[0] = (Stack){RATION, 1};
    step(.1f);
    CHECK(a->hunger > 70 && !g.buildings[0].storage[0].qty);
    a->hunger = 10;
    a->bleed = 0;
    a->health[LARM] = 40;
    a->taskKind = 3;
    a->action = 100;
    step(1);
    CHECK(a->health[LARM] == 40);
    a->hunger = 50;
    step(1);
    CHECK(a->health[LARM] > 40);
    b->active = 1;
    inventory_add(a, ORE, 1);
    int slot = 0;
    while (a->pack[slot].item != ORE)
        slot++;
    a->stolen[slot] = UNION;
    ui.screen = SCREEN_PACK;
    ui.selection = slot;
    game_tick(.033f, (Input){KEY_SQUARE, KEY_SQUARE, 0, 0});
    slot = 0;
    while (slot < PACK && b->pack[slot].item != ORE)
        slot++;
    CHECK(slot < PACK && b->stolen[slot] == UNION);
    fresh();
    float closest = 1e9f;
    for (int t = 0; t < TOWNS; t++)
        for (int j = 0; j < t; j++) {
            float d = distance(towns[t].x, towns[t].y, towns[j].x, towns[j].y);
            if (d < closest)
                closest = d;
        }
    CHECK(closest > 100);
    for (int i = 1; i < g.actorCount; i++)
        if (g.actors[i].kind == RAIDER || g.actors[i].kind == BEAST)
            for (int t = 0; t < TOWNS; t++)
                if (towns[t].faction != HOSTILE)
                    CHECK(distance(g.actors[i].x, g.actors[i].y, towns[t].x, towns[t].y) >= 32);
    int wilderness = 0;
    for (int i = 1; i < g.actorCount; i++)
        if ((g.actors[i].kind == RAIDER || g.actors[i].kind == BEAST) &&
            town_near(g.actors[i].x, g.actors[i].y, 30) < 0)
            wilderness++;
    CHECK(wilderness >= 25);
    printf("Closest towns %.1fm; wilderness enemies %d.\n", closest, wilderness);
    /* Route traverses the real terrain, with combat disabled to isolate routing. */
    a = &g.actors[0];
    a->x = 34;
    a->y = 31;
    ui.waypoint = 1;
    ui.mapX = towns[1].x + 2;
    ui.mapY = towns[1].y + 7;
    ui.travelX = ui.mapX;
    ui.travelY = ui.mapY;
    game_tick(.033f, (Input){KEY_SELECT, KEY_SELECT, 0, 0});
    CHECK(ui.screen == SCREEN_MAP && ui.mapX == a->x && ui.travelX == towns[1].x + 2);
    game_tick(.033f, (Input){KEY_R, KEY_R, 0, 0});
    game_tick(.033f, (Input){KEY_CIRCLE, KEY_CIRCLE, 0, 0});
    CHECK(ui.waypoint && ui.travelX == towns[1].x + 2 && ui.travelY == towns[1].y + 7);
    int frames = 0;
    for (; frames < 30 * 240 && ui.waypoint; frames++)
        game_tick(1.f / 30, (Input){0});
    printf("Dusk to Copper: %.1fs, remaining %.2fm.\n", frames / 30.f,
           distance(a->x, a->y, ui.travelX, ui.travelY));
    CHECK(!ui.waypoint && frames > 30 * 40);
    CHECK(g.discovered[1]);
    ui.screen = SCREEN_WORLD;
    game_tick(.033f, (Input){KEY_SELECT, KEY_SELECT, 0, 0});
    CHECK(ui.mapZoom == 1 && ui.screen == SCREEN_MAP);
    game_tick(.033f, (Input){KEY_TRIANGLE, KEY_TRIANGLE, 0, 0});
    CHECK(!ui.mapZoom);
    /* Real pre-change v3 fixture: coordinates, fog, gear and specialists migrate. */
    FILE *f = fopen("build/v030-frontier.sav", "rb");
    CHECK(f != NULL);
    PreV5Game *old = malloc(sizeof(PreV5Game));
    CHECK(old != NULL);
    fseek(f, 20, SEEK_SET);
    CHECK(fread(old, sizeof(PreV5Game), 1, f) == 1);
    fclose(f);
    CHECK(load_game("build/v030-frontier.sav"));
    a = &g.actors[g.selected];
    CHECK(distance(a->x, a->y, towns[11].x, towns[11].y) < 10 && g.money == old->money);
    CHECK(!memcmp(a->gear, old->actors[old->selected].gear, sizeof(a->gear)));
    int fog = 0;
    for (unsigned i = 0; i < sizeof(g.seen); i++)
        if (g.seen[i])
            fog++;
    CHECK(fog > 1000);
    int medics = 0;
    for (int i = 0; i < g.actorCount; i++)
        medics += npc_role(&g.actors[i]) == ROLE_MEDIC;
    CHECK(medics > 0);
    float x = a->x, y = a->y;
    CHECK(save_game("build/v4-roundtrip.sav") && load_game("build/v4-roundtrip.sav"));
    CHECK(g.actors[g.selected].x == x && g.actors[g.selected].y == y);
    /* v2 used exactly the same exterior payload as v3, without indoor fields. */
    f = fopen("build/real-v2-prefix.sav", "wb");
    size_t n = offsetof(PreV5Game, interiorBuilding);
    uint32_t h[] = {2, n, hash(old, n)};
    fwrite("DUSTSAVE", 8, 1, f);
    fwrite(h, sizeof(h), 1, f);
    fwrite(old, n, 1, f);
    fclose(f);
    free(old);
    CHECK(load_game("build/real-v2-prefix.sav") && !g.interiorBuilding && g.actors[g.selected].x == x &&
          g.actors[g.selected].y == y);
    printf("PASS: %d playability checks.\n", checks);
    return 0;
}
