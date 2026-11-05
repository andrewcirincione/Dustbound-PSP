/* Prepared catalog saves only; the shopping run starts a normal new game. */
#include "game.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    set_save_root(argc > 1 ? argv[1] : "build/attachment-fixtures");
    for (int group = 0; group < 7; group++) {
        if (group == 4)
            set_save_root("build/clothing-fixtures");
        game_new(73819, 0);
        Actor *a = &g.actors[0];
        a->gear[0] = gear_id(0, 1, 1);
        a->gear[1] = gear_id(34, 1, 1);
        a->gear[2] = gear_id(49, 0, 1);
        a->gear[3] = gear_id(60, 0, 1);
        a->gear[4] = gear_id(68, 0, 1);
        a->gear[5] = gear_id(82, 0, 1);
        if (group >= 4)
            a->gear[0] = a->gear[2] = a->gear[5] = NONE;
        a->x = 27;
        a->y = 30;
        a->autoattack = 0;
        a->xp[STRENGTH] = 12 * 90 * 90;
        memset(a->pack, 0, sizeof(a->pack));
        for (int i = 0; i < (group == 6 ? 12 : group == 3 ? 8 : group == 2 ? 4 : 16); i++) {
            int f = group >= 4   ? 32 + (group - 4) * 16 + i
                    : group == 3 ? 76 + i
                                 : weapon_family_index(group * 16 + i);
            a->pack[i] = (Stack){gear_id(f, 3, 2), 1};
        }
        for (int i = 1; i < g.actorCount; i++)
            if (g.actors[i].faction == HOSTILE || g.actors[i].kind == BEAST)
                g.actors[i].active = 0;
        if (!save_slot(group >= 4 ? group - 2 : group + 2))
            return 1;
    }
    game_new(73819, 0);
    ui.tradeTown = 0;
    ui.shopFilter = 4;
    FILE *f = fopen("build/attachment-fixtures/market.json", "w");
    if (!f)
        return 1;
    fprintf(f, "{\"stock\":[");
    for (int i = 0; i < shop_count(); i++) {
        int item = shop_item(i);
        fprintf(f, "%s{\"item\":%d,\"price\":%d}", i ? "," : "", item, item_price(0, item, 0));
    }
    for (int i = 0; i < g.actorCount; i++)
        if (npc_role(&g.actors[i]) == ROLE_MARSHAL && town_near(g.actors[i].x, g.actors[i].y, 14) == 0) {
            fprintf(f, "],\"marshal\":[%.2f,%.2f]}\n", g.actors[i].x, g.actors[i].y);
            break;
        }
    fclose(f);
    return 0;
}
