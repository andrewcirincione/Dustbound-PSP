#ifndef FRONTIER_FIXTURE_H
#define FRONTIER_FIXTURE_H
#include "game.h"
#include <assert.h>
#include <math.h>

static inline void fixture_door(Actor *a, int id) {
    assert(id >= 0 && id < g.buildingCount);
    building_door(id, &a->x, &a->y);
}
static inline void fixture_service(int action) {
    assert(g.interiorBuilding);
    InteriorProp props[ROOM_PROPS];
    int n = interior_floor_props(g.interiorBuilding - 1, actor_floor(&g.actors[g.selected]), props);
    for (int i = 0; i < n; i++)
        if (props[i].action == action) {
            const float sides[][2] = {{0,1},{0,-1},{-1,0},{1,0}};
            for (int j = 0; j < 4; j++) {
                float x = props[i].x + (props[i].w + .22f) * sides[j][0];
                float y = props[i].y + (props[i].d + .22f) * sides[j][1];
                if (!room_floor_walkable(g.interiorBuilding, actor_floor(&g.actors[g.selected]), x, y))
                    continue;
                g.interiorX = x;
                g.interiorY = y;
                if (interior_context() == i)
                    return;
            }
        }
    assert(!"Service is missing or out of reach");
}
static inline int fixture_action(int kind) {
    int count = nearby_actions();
    for (int i = 0; i < count; i++)
        if (nearby_action(i)->type == kind && !nearby_action_reason(i))
            return i;
    return -1;
}
static inline int fixture_dressings(const Actor *a) {
    int total = 0;
    for (int i = 0; i < PACK; i++)
        if (portion_base(a->pack[i].item) == MEDKIT)
            total += a->pack[i].qty * portion_percent(a->pack[i].item);
    return total;
}
#endif
