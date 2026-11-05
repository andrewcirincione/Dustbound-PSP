#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    game_new(73819, 0);
    Actor a = g.actors[0];
    memset(a.gear, 0, sizeof(a.gear));
    memset(a.pack, 0, sizeof(a.pack));
    a.carrying = -1;
    a.xp[STRENGTH] = 12 * 90 * 90;
    int variants = 0;
    FILE *f = fopen("evidence/v051-clothing-balance.csv", "w");
    assert(f);
    fputs("family,name,slot,armor,weight,move_percent,stealth,capacity,weather,effect,value\n", f);
    for (int family = 32; family < 76; family++) {
        for (int m = 0; m < MATERIALS; m++)
            for (int q = 0; q < QUALITIES; q++) {
                int item = gear_id(family, m, q);
                const ItemDef *d = &itemDefs[item];
                assert(d->armor > 0 && d->armor <= 60);
                assert(d->weight >= 1 && d->weight <= 20);
                assert(d->speed >= -12 && d->speed <= 10);
                assert(d->effect == 4 || (d->effect >= 6 && d->effect <= 11));
                if (q) {
                    const ItemDef *previous = &itemDefs[item - 1];
                    assert(d->armor > previous->armor && d->price > previous->price);
                    assert(d->weight == previous->weight);
                    assert(d->speed >= previous->speed && d->stealth >= previous->stealth);
                    assert(d->effect == previous->effect);
                }
                memset(a.gear, 0, sizeof(a.gear));
                float bare = movement_speed(&a);
                a.gear[d->slot] = item;
                assert(fabsf(movement_speed(&a) / bare - (1 + d->speed * .005f)) < .001f);
                assert(gear_bonus(&a, 2) == d->stealth && gear_bonus(&a, 3) == d->capacity);
                assert(gear_bonus(&a, 4) == d->protection);
                int effect = d->effect == 6    ? gear_bonus(&a, 2)
                             : d->effect == 7  ? gear_bonus(&a, 1)
                             : d->effect == 8  ? gear_bonus(&a, 5)
                             : d->effect == 9  ? gear_bonus(&a, 4)
                             : d->effect == 10 ? gear_bonus(&a, 3)
                             : d->effect == 11 ? gear_bonus(&a, 6)
                                               : d->potency;
                assert(item_effect_value(item) == effect);
                variants++;
            }
        const ItemDef *d = &itemDefs[gear_id(family, 0, 0)];
        fprintf(f, "%d,%s,%d,%d,%d,%.1f,%d,%d,%d,%s,%d\n", family, familyNames[family], d->slot, d->armor,
                d->weight, d->speed * .5f, d->stealth, d->capacity, d->protection, effectNames[d->effect],
                item_effect_value(gear_id(family, 0, 0)));
    }
    fclose(f);
    const int heavy[] = {38, 50, 62, 70}, light[] = {39, 48, 61, 73};
    for (int slot = 0; slot < 4; slot++)
        for (int m = 0; m < MATERIALS; m++)
            for (int q = 0; q < QUALITIES; q++) {
                const ItemDef *h = &itemDefs[gear_id(heavy[slot], m, q)];
                const ItemDef *l = &itemDefs[gear_id(light[slot], m, q)];
                assert(h->armor > l->armor * 2 && h->weight > l->weight);
                assert(h->speed < l->speed && h->stealth < l->stealth);
            }
    // Carrying an unequipped item cannot grant its passive bonuses.
    memset(a.gear, 0, sizeof(a.gear));
    a.pack[0] = (Stack){gear_id(65, 4, 3), 1};
    for (int stat = 0; stat < 7; stat++)
        assert(!gear_bonus(&a, stat));
    printf("PASS: %d clothing variants preserve quality progression, live bonuses and 96 heavy/light "
           "tradeoffs.\n",
           variants);
    return 0;
}
