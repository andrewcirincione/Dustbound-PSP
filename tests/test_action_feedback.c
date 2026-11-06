#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Actor *travel_start(void) {
    game_new(73819, 0);
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
    Actor *a = &g.actors[0];
    a->autoattack = 0;
    a->x = 34;
    a->y = 31;
    ui.screen = SCREEN_WORLD;
    ui.waypoint = 1;
    ui.travelX = 34;
    ui.travelY = 41;
    ui.travelLastX = a->x;
    ui.travelLastY = a->y;
    return a;
}

static void travel_checks(void) {
    for (int state = 0; state < 6; state++) {
        Actor *a = travel_start();
        for (int i = 0; i < 100; i++) {
            a->unconscious = state == 0 ? 20 : 0;
            a->stun = state == 1 ? 20 : 0;
            a->carriedBy = state == 2 ? 1 : -1;
            a->prisonTime = state == 3 ? 20 : 0;
            a->attackPhase = state == 4 ? 1 : 0;
            a->attackClock = 20;
            g.hitstop = state == 5 ? 1 : 0;
            game_tick(.1f, (Input){0});
            assert(ui.waypoint && ui.travelX == 34 && ui.travelY == 41);
            assert(ui.travelStall == 0);
        }
        a->unconscious = a->stun = a->prisonTime = g.hitstop = 0;
        a->carriedBy = -1;
        a->attackPhase = 0;
        a->x = 34;
        a->y = 31;
        for (int i = 0; i < 200 && ui.waypoint; i++)
            game_tick(.1f, (Input){0});
        assert(!ui.waypoint && distance(a->x, a->y, 34, 41) < .7f);
    }
    Actor *a = travel_start();
    g.buildings[0].x = a->x;
    g.buildings[0].y = a->y;
    g.buildings[0].type = HOUSE;
    for (int i = 0; i < 90 && ui.waypoint; i++)
        game_tick(.1f, (Input){0});
    assert(!ui.waypoint && strstr(ui.notice, "Route blocked"));
    a = travel_start();
    a->dead = 1;
    game_tick(.1f, (Input){0});
    assert(!ui.waypoint && strstr(ui.notice, "Traveler lost"));
    a = travel_start();
    a->unconscious = 20;
    game_tick(.1f, (Input){KEY_RIGHT, KEY_RIGHT, 0, 0});
    assert(!ui.waypoint);
    puts("PASS: six travel holds preserve destinations, resume to arrival, and distinguish terrain/death/cancellation.");
}

static void escort_care_checks(void) {
    game_new(73819, 0);
    int sora = g.depth.npc[NPC_SORA];
    Actor *a = &g.actors[0], *b = &g.actors[sora];
    a->x = b->x + .5f;
    a->y = b->y;
    int kits = inventory_count(a, MEDKIT), money = g.money;
    assert(!depth_dialog(sora, 2) && a->carrying < 0);
    assert(depth_dialog(sora, 0) && g.depth.rescued == 1);
    b->bleed = 2;
    float health = b->health[LLEG];
    assert(strstr(depth_option(sora, 0), "again"));
    assert(depth_dialog(sora, 0));
    assert(b->bleed == 0 && b->health[LLEG] > health);
    assert(inventory_count(a, MEDKIT) == kits - 2 && g.money == money);
    assert(g.depth.rescued == 1);
    inventory_remove(a, MEDKIT, inventory_count(a, MEDKIT));
    b->bleed = 2;
    assert(!depth_dialog(sora, 0) && b->bleed == 2);
    assert(g.depth.rescued == 1 && g.money == money);
    puts("PASS: ongoing escort care spends a dressing, stops new bleeding, and never awards arrival early.");
    float speed = movement_speed(a);
    assert(depth_dialog(sora, 2) && a->carrying == sora && b->carriedBy == 0);
    assert(movement_speed(a) < speed);
    assert(save_game("build/escort-carry.sav") && load_game("build/escort-carry.sav"));
    assert(a->carrying == sora && b->carriedBy == 0);
    assert(!depth_dialog(sora, 2));
    ui.screen = SCREEN_WORLD;
    game_tick(.033f, (Input){KEY_CROSS, KEY_CROSS, 0, 0});
    assert(a->carrying < 0 && b->carriedBy < 0 && g.depth.rescued == 1);
    b->x = a->x; b->y = a->y;
    assert(depth_dialog(sora, 2));
    a->x = g.depth.siteX[SITE_STORES];
    a->y = g.depth.siteY[SITE_STORES];
    /* Move the whole carried pair in this prepared arrival case. */
    b->x = a->x; b->y = a->y;
    simulate(.1f); simulate(.1f);
    assert(g.depth.rescued == 2 && g.money == money + 100);
    assert(a->carrying < 0 && b->carriedBy < 0);
    simulate(.1f);
    assert(g.money == money + 100 && !depth_dialog(sora, 2));
    puts("PASS: consenting escort carry adds weight, saves/reloads, permits set-down, and pays once at real arrival.");
}

int main(void) {
    game_new(73819, 0);
    Actor *a = &g.actors[g.selected];
    int sora = g.depth.npc[NPC_SORA];
    a->x = g.depth.siteX[SITE_CULVERT];
    a->y = g.depth.siteY[SITE_CULVERT];
    g.depth.rescued = 2;
    assert(inventory_add(a, IRONBAR, 1));
    int wood = inventory_count(a, WOOD), kits = inventory_count(a, MEDKIT);
    for (int state = 0; state < 4; state++) {
        a->unconscious = state == 0 ? 10 : 0;
        a->stun = state == 1 ? 1 : 0;
        a->carriedBy = state == 2 ? sora : -1;
        a->dead = state == 3;
        const char *expected[] = {"Unconscious", "Stunned", "Set this survivor down", "living ally"};
        assert(!depth_action(SITE_CULVERT, 0));
        assert(strstr(ui.notice, expected[state]));
        assert(!g.depth.route && inventory_count(a, WOOD) == wood);
        assert(!depth_dialog(sora, 0));
        assert(strstr(ui.notice, expected[state]));
        assert(inventory_count(a, MEDKIT) == kits);
    }
    a->dead = 0;
    a->x += 10;
    assert(!depth_action(SITE_CULVERT, 0));
    assert(strstr(ui.notice, "Walk within"));
    assert(!depth_dialog(sora, 0));
    assert(strstr(ui.notice, "talking reach"));
    a->x = g.depth.siteX[SITE_CULVERT];
    assert(depth_action(SITE_CULVERT, 0));
    assert(g.depth.route == ROUTE_BYPASS && inventory_count(a, WOOD) == wood - 3);
    puts("PASS: action feedback distinguishes incapacity from reach without spending supplies.");
    travel_checks();
    escort_care_checks();
    return 0;
}
