#include "frontier_fixture.h"
#include "save_fixture.h"
#include "combat_pose.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static int checks;
#define CHECK(x)                                                                                             \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(x)) {                                                                                          \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)
int main(void) {
    game_new(73819, 0);
    Actor *a = &g.actors[0];
    int shop = -1, bar = -1;
    for (int i = 0; i < g.buildingCount; i++) {
        if (shop < 0 && g.buildings[i].type == SHOP)
            shop = i;
        if (bar < 0 && g.buildings[i].type == BAR)
            bar = i;
    }
    CHECK(shop >= 0 && bar >= 0);
    CHECK(!interior_enter(shop));
    fixture_door(a, shop);
    a->target = -1;
    int haveDoor = 0;
    for (int k = 0, count = nearby_actions(); k < count; k++)
        haveDoor |= nearby_action(k)->kind == 3 && nearby_action(k)->target == shop;
    CHECK(haveDoor);
    interact_context(3, shop);
    CHECK(g.interiorBuilding == shop + 1);
    CHECK(interior_context() == -1);
    fixture_service(ROOM_TRADE);
    interior_interact();
    CHECK(ui.screen == SCREEN_TRADE && ui.tradeTown == 0);
    ui.screen = SCREEN_WORLD;
    int money = g.money;
    CHECK(buy_item(0, RATION));
    CHECK(g.money < money);
    float oldx = g.interiorX, oldy = g.interiorY;
    for (int i = 0; i < 200; i++)
        interior_move(0, -1, .1f);
    CHECK(g.interiorY > .4f && g.interiorX > .4f);
    CHECK(g.interiorX != oldx || g.interiorY != oldy);
    fixture_service(ROOM_TRADE);
    InteriorProp props[ROOM_PROPS];
    interior_props(shop, props);
    int counter = interior_context();
    for (int step = 0; step < 35; step++)
        interior_move(1, -1, .04f);
    CHECK(fabsf(g.interiorY - props[counter].y) >= props[counter].d);
    g.interiorX = 3.5f;
    g.interiorY = 4.8f;
    CHECK(save_game("build/interior-test.sav"));
    g.interiorBuilding = 0;
    CHECK(load_game("build/interior-test.sav"));
    CHECK(g.interiorBuilding == shop + 1 && fabsf(g.interiorY - 4.8f) < .001f);
    /* A checked legacy v2 prefix must migrate, with no fabricated indoor state. */
    save_prior_fixture("build/v2-test.sav", 2);
    CHECK(load_game("build/v2-test.sav"));
    CHECK(g.interiorBuilding == 0);
    a = &g.actors[0];
    fixture_door(a, bar);
    a->target = -1;
    CHECK(interior_enter(bar));
    fixture_service(ROOM_REST);
    a->health[LARM] = 40;
    a->bleed = 0;
    dress_wounds(a);
    money = g.money;
    interior_interact();
    CHECK(a->taskKind == 3 && g.money == money - 25);
    simulate(1);
    CHECK(a->health[LARM] > 40);
    a->bleed = 1;
    float blood = a->blood;
    simulate(1);
    CHECK(a->blood < blood); /* indoors cannot freeze injuries */
    interior_leave();
    CHECK(!g.interiorBuilding && can_walk(a->x, a->y));
    CHECK(!interior_enter(-1) && !interior_enter(g.buildingCount));
    game_new(73819, 1);
    a = &g.actors[0];
    fixture_door(a, bar);
    a->target = -1;
    CHECK(interior_enter(bar));
    a->dead = 1;
    ui.screen = SCREEN_SQUAD;
    game_tick(.033f, (Input){0, KEY_SQUARE, 0, 0});
    CHECK(!g.interiorBuilding && g.selected != 0 && g.squadCount == 2);
    CHECK(save_game("build/indoor-death.sav") && load_game("build/indoor-death.sav"));
    /* Independent rigid-body invariant: both wrists lie on the weapon's own
       axis at all directions and phases, including crouched follow-through. */
    for (int family = 0; family < 32; family++)
        for (int phase = 0; phase < 4; phase++)
            for (int dir = 0; dir < 8; dir++)
                for (int t = 0; t <= 12; t++) {
                    CombatPose p = combat_pose(gear_id(family, 1, 1), phase, t / 12.f, 0);
                    float an = dir * .785398163f + p.blade;
                    WeaponGrip grip =
                        weapon_grip(20 + p.shift * (1 + p.twist), 40 + p.handY * 1.25f + p.crouch, an, 3);
                    float x0, y0, x1, y1;
                    weapon_hand(grip, gear_id(family, 1, 1), 0, &x0, &y0);
                    weapon_hand(grip, gear_id(family, 1, 1), 1, &x1, &y1);
                    float support = family == 29 || family == 31   ? 7
                                    : family == 25 || family == 26 ? 5
                                    : family == 24                 ? 6
                                    : family == 28                 ? 11.5f
                                    : family == 14 || family == 15 ? 3.2f
                                                                   : 4;
                    CHECK(fabsf(hypotf(x1 - x0, y1 - y0) - support * 3) < .001f);
                    CHECK(fabsf((x1 - x0) * sinf(an) - (y1 - y0) * cosf(an)) < .001f);
                }
    printf("PASS: %d interior, save migration and rigid grip checks.\n", checks);
    return 0;
}
