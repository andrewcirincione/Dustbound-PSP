#include "frontier_fixture.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void near(float a, float b) { assert(fabsf(a - b) < .001f); }
int main(void) {
    game_new(73819, 0);
    unsigned stockedFamilies = 0;
    for (int t = 0; t < TOWNS; t++) {
        ui.tradeTown = t;
        ui.shopFilter = 4;
        assert(shop_count() <= 1);
        for (int i = 0; i < shop_count(); i++) {
            int item = shop_item(i);
            assert(item && itemDefs[item].slot == 5 && market_quantity(t, item) == 1);
            stockedFamilies |= 1u << (gear_family(item) - 76);
            for (int j = 0; j < i; j++)
                assert(shop_item(j) != item);
        }
        assert(shop_item(shop_count()) == NONE);
    }
    assert(__builtin_popcount(stockedFamilies) >= 4);
    Actor *a = &g.actors[0];
    Actor original = *a;
    int swaps = 0;
    for (int f = 76; f <= 83; f++)
        for (int m = 0; m < MATERIALS; m++)
            for (int q = 0; q < QUALITIES; q++) {
                *a = original;
                memset(a->pack, 0, sizeof(a->pack));
                memset(a->stolen, 0, sizeof(a->stolen));
                int item = gear_id(f, m, q);
                a->pack[0] = (Stack){item, 2};
                a->pack[1] = (Stack){ORE, 20};
                a->pack[2] = (Stack){MEDKIT, 4};
                a->stolen[0] = 2;
                a->carrying = 1;
                float expected = backpack_load_preview(a, item);
                use_item(0);
                assert(a->gear[5] == item && a->gearStolen[5] == 2);
                near(expected, encumbrance(a));
                swaps++;
            }
    *a = original;
    memset(a->pack, 0, sizeof(a->pack));
    a->pack[0] = (Stack){ORE, 40};
    a->gear[5] = gear_id(76, 0, 0);
    float travel = encumbrance(a), travel_speed = movement_speed(a);
    a->gear[5] = gear_id(79, 0, 0);
    assert(encumbrance(a) < travel && movement_speed(a) > travel_speed);
    int bag = a->gear[5];
    a->gear[5] = NONE;
    a->pack[1] = (Stack){bag, 1};
    float raw = 0;
    for (int i = 0; i < PACK; i++)
        raw += itemDefs[a->pack[i].item].weight * a->pack[i].qty;
    for (int i = 0; i < GEARS; i++)
        raw += itemDefs[a->gear[i]].weight;
    near(raw, encumbrance(a)); // A carried bag gives no worn bonus.
    *a = original;
    memset(a->gear, 0, sizeof(a->gear));
    for (int j = 0; j < LIMBS; j++)
        a->health[j] = 30;
    inventory_add(a, MEDKIT, 3);
    Actor hurt = *a;
    assert(heal_actor(0, 0));
    float ordinary = a->action;
    assert(a->health[HEAD] == hurt.health[HEAD]);
    *a = hurt;
    a->gear[5] = gear_id(78, 0, 0);
    assert(heal_actor(0, 0) && a->action < ordinary && a->health[HEAD] == hurt.health[HEAD]);

    game_new(73819, 0);
    g.identityVersion = 1;
    int sample = gear_id(79, 0, 0), unrelated = gear_id(84, 1, 1), actors = g.actorCount;
    g.stock[0][sample] = 0;
    g.stock[0][unrelated] = 19;
    assert(save_game("build/v051-market-upgrade.sav"));
    assert(load_game("build/v051-market-upgrade.sav"));
    assert(g.identityVersion == 2 && g.stock[0][sample] == 2 && g.stock[0][unrelated] == 19);
    assert(g.actorCount == actors);
    g.stock[0][sample] = 0;
    assert(save_game("build/v051-market-upgrade.sav") && load_game("build/v051-market-upgrade.sav"));
    assert(!g.stock[0][sample]); // Reloading cannot replenish a sold-out pack.

    assert(accept_contract(0, 0));
    ui.screen = SCREEN_JOURNAL;
    ui.selection = 0;
    ui.waypoint = 1;
    ui.travelX = 100;
    ui.travelY = 110;
    game_tick(.033f, (Input){.pressed = KEY_SQUARE});
    assert(ui.screen == SCREEN_MAP && ui.mapTown == g.contracts[0].target);
    near(ui.mapX, towns[g.contracts[0].target].x);
    assert(ui.waypoint && ui.travelX == 100 && ui.travelY == 110);
    g.contracts[0].progress = 1;
    ui.screen = SCREEN_JOURNAL;
    game_tick(.033f, (Input){.pressed = KEY_SQUARE});
    assert(ui.mapTown == 0);
    assert(accept_contract(0, 1));
    memset(g.actors[0].pack, 0, sizeof(g.actors[0].pack));
    inventory_add(&g.actors[0], ORE, 3);
    char desc[180];
    contract_description(1, desc, sizeof(desc));
    assert(strstr(desc, "3/8 in squad"));
    assert(load_game("build/v050-expedition.sav"));
    assert(g.identityVersion == 2 && g.money == 85 && g.squadCount == 2);
    assert(g.actors[g.selected].gear[1] == LEATHER);
    printf("PASS: %d real backpack swaps match load preview; 16 stocked markets; cargo/speed/medical "
           "effects; one-time and genuine v5 migration; journal target/return tracking preserves travel.\n",
           swaps);
    return 0;
}
