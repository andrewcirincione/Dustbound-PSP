#include "game.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    set_save_root(argc > 1 ? argv[1] : "build/frontier-fixtures");
    int factions[] = {SHOGUN, SCAVENGERS, OUTLAWS};
    for (int k = 0; k < 3; k++) {
        game_new(73819, 0);
        int n = -1;
        for (int j = 0; j < g.actorCount; j++)
            if (g.actors[j].kind == TRADER && g.actors[j].faction == factions[k]) {
                n = j;
                break;
            }
        if (n < 0)
            return 1;
        Actor *p = &g.actors[0], *m = &g.actors[n];
        p->x = m->x;
        p->y = m->y + .5f;
        p->autoattack = 0;
        for (int j = 1; j < g.actorCount; j++)
            if (g.actors[j].faction == HOSTILE || g.actors[j].kind == BEAST)
                g.actors[j].active = 0;
        if (!save_slot(k + 2))
            return 1;
    }
    game_new(73819, 0);
    Actor *p = &g.actors[0];
    memset(p->pack, 0, sizeof(p->pack));
    for (int k = 0; k < 4; k++)
        inventory_add(p, gear_id(84 + k, 3, 2), 1);
    p->gear[1] = gear_id(34, 1, 1);
    p->gear[2] = gear_id(49, 1, 1);
    p->autoattack = 0;
    if (!save_slot(5))
        return 1;
    return 0;
}
