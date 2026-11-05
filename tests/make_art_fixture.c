/* Prepared saves for controller-driven visual QA; never shipped in installers. */
#include "game.h"
#include <string.h>
int main(int argc, char **argv) {
    set_save_root(argc > 1 ? argv[1] : "build/art-fixtures");
    for (int group = 0; group < 2; group++) {
        game_new(73819, 0);
        Actor *a = &g.actors[0];
        a->gear[0] = gear_id(0, 1, 1);
        a->gear[1] = gear_id(34, 1, 1);
        a->gear[2] = gear_id(49, 0, 1);
        a->gear[3] = gear_id(60, 0, 1);
        a->gear[4] = gear_id(68, 0, 1);
        a->gear[5] = gear_id(82, 0, 1);
        a->x = 27;
        a->y = 30;
        a->autoattack = 0;
        a->xp[STRENGTH] = 12 * 90 * 90;
        memset(a->pack, 0, sizeof(a->pack));
        for (int i = 0; i < 16; i++)
            a->pack[i] = (Stack){gear_id(group * 16 + i, 3, 2), 1};
        for (int i = 1; i < g.actorCount; i++)
            if (g.actors[i].faction == HOSTILE || g.actors[i].kind == BEAST)
                g.actors[i].active = 0;
        if (!save_slot(group + 1))
            return 1;
    }
    return 0;
}
