#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern void simulate(float);
extern int hostile(const Actor *, const Actor *);
int main(void) {
    game_new(73819, 0);
    ui.screen = SCREEN_WORLD;
    int guards[4], count = 0;
    for (int i = 0; i < g.actorCount; i++)
        if (npc_role(&g.actors[i]) == ROLE_MERCENARY)
            guards[count++] = i;
    printf("World actors %d, contract guards %d\n", g.actorCount, count);
    assert(count == 4);
    for (int i = 0; i < g.actorCount; i++)
        if (npc_role(&g.actors[i]) == ROLE_MEDIC) {
            Actor *medic = &g.actors[i];
            assert(can_walk(medic->x, medic->y));
            for (int j = 0; j < g.actorCount; j++)
                if (npc_role(&g.actors[j]) == ROLE_MERCHANT)
                    assert(distance(medic->x, medic->y, g.actors[j].x, g.actors[j].y) > TALK_REACH);
        }
    for (int f = 84; f < 88; f++) {
        int it = gear_id(f, 1, 1);
        assert(itemDefs[it].slot == 0 && !weapon_ranged(it));
    }
    Actor *p = &g.actors[0], *m = &g.actors[guards[0]];
    int it = gear_id(84, 1, 1);
    float slow = weapon_handling(p, it);
    p->xp[STRENGTH] = 12 * 80 * 80;
    assert(weapon_handling(p, it) < slow);
    p->health[LARM] = 0;
    assert(weapon_handling(p, it) > 1);
    p->health[LARM] = 100;
    int money = g.money, squad = g.squadCount;
    assert(!merc_hire(guards[0], 0) && g.money == money); /* cannot hire remotely */
    p->x = m->x;
    p->y = m->y;
    assert(!merc_hire(guards[0], 1) && g.money == money); /* no camp */
    assert(merc_hire(guards[0], 0) && g.money == money - 240 && !m->squad && g.squadCount == squad);
    int slot = merc_slot(m);
    assert(slot >= 0 && dialogue_action(guards[0], 0) == 11);
    assert(!merc_hire(guards[0], 0) && g.money == money - 240);
    for (int i = 1; i < g.actorCount; i++)
        if (i != guards[0])
            g.actors[i].active = 0;
    p->x += 7;
    float before = distance(p->x, p->y, m->x, m->y);
    for (int i = 0; i < 150; i++)
        simulate(1.f / 30);
    assert(distance(p->x, p->y, m->x, m->y) < before);
    p->x = m->x;
    p->y = m->y;
    p->bleed = 3;
    m->target = -1;
    frontier_tick(.1f);
    assert(p->bleed == 0); /* hired aid is useful after a fight */
    assert(save_game("build/v050-contract.sav"));
    float until = g.mercUntil[slot];
    g.mercUntil[slot] = 0;
    assert(load_game("build/v050-contract.sav"));
    m = &g.actors[guards[0]];
    assert(merc_slot(m) == slot && g.mercUntil[slot] == until);
    g.time = until + .1f;
    frontier_tick(.1f);
    assert(merc_slot(m) < 0 && !g.mercActor[slot]);
    p = &g.actors[g.selected];
    p->x = m->x;
    p->y = m->y;
    g.baseExists = 1;
    g.baseX = p->x + 4;
    g.baseY = p->y;
    g.money = 180;
    assert(merc_hire(guards[0], 1) && !g.money);
    merc_dismiss(guards[0]);
    assert(merc_slot(m) < 0);
    assert(!merc_hire(guards[0], 0));
    game_new(73819, 0);
    assert(g.stock[0][gear_id(84, 1, 1)] > 0);
    assert(!g.stock[0][gear_id(85, 1, 1)]);
    assert(g.stock[1][gear_id(85, 1, 1)] > 0);
    ui.tradeTown = 1;
    ui.shopFilter = 2;
    assert(shop_item(0) >= BASE_ITEMS); /* show regional gear before shared starter stock */
    for (int i = 0; i < shop_count(); i++) {
        int item = shop_item(i);
        assert(item > 0 && item < ITEMS && itemDefs[item].slot == 0 && g.stock[1][item] > 0);
        for (int j = 0; j < i; j++)
            assert(shop_item(j) != item);
    }
    assert(load_game("build/v041-outpost.sav"));
    assert(g.baseExists && g.identityVersion == 2);
    int n = g.actorCount;
    assert(save_game("build/v050-migrated.sav") && load_game("build/v050-migrated.sav") && g.actorCount == n);
    SaveInfo info;
    set_save_root("build");
    assert(save_slot(1) && save_inspect(1, &info) && info.valid && info.version >= 6);
    puts("PASS: frontier roles, 4 new families, regional stock, strength/injury penalties, "
         "hire/range/payment/follow/expiry/dismissal and genuine v4 migration.");
}
