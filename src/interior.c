#include "game.h"
#include <math.h>
#include <string.h>
extern int conscious(const Actor *);
static int private_chest(int building) {
    const Building *b = &g.buildings[building];
    return b->owner < 0 && b->faction > FREE &&
           (b->type == SHOP || (b->type == HOUSE && (g.seed + building * 17u) % 3 != 0));
}
static void restock_chest(int building) {
    if (!private_chest(building) || g.day < g.townLife.chestRefresh[building])
        return;
    Building *b = &g.buildings[building];
    unsigned mix = (g.seed ^ (unsigned)building * 2654435761u) + g.day * 97u;
    static const int home[] = {RATION, CLOTH, WATER, WOOD, GRAIN};
    static const int store[] = {MEDKIT, IRONBAR, CLOTH, RATION, BOLTS, HIDE};
    memset(b->storage, 0, sizeof(b->storage));
    if (b->type == SHOP) {
        for (int i = 0; i < 2; i++) {
            int item = store[(mix % 6 + i * (1 + mix % 5)) % 6];
            if (item == MEDKIT && mix % 19)
                item = portion_item(MEDKIT, 30 + mix % 61);
            b->storage[i] = (Stack){item, i == 0 ? 2 : 1};
        }
    } else if (mix % 5 != 0) {
        b->storage[0] = (Stack){home[mix % 5], 1 + mix % 2};
    }
    g.townLife.chestRefresh[building] = g.day + 2 + mix % 3;
}
static Stack *chest_stack(int row, int *slot) {
    Stack *stacks;
    int count;
    if (ui.chestKind == CHEST_PROPERTY) {
        if (ui.storageIndex < 0 || ui.storageIndex >= SLAVE_CAMPS)
            return NULL;
        stacks = g.field.seized[ui.storageIndex];
        count = SEIZED_STACKS;
    } else {
        if (!g.interiorBuilding || g.interiorBuilding != ui.storageIndex + 1 ||
            !private_chest(ui.storageIndex))
            return NULL;
        stacks = g.buildings[ui.storageIndex].storage;
        count = 16;
    }
    for (int i = 0; i < count; i++)
        if (stacks[i].item && stacks[i].qty && row-- == 0) {
            if (slot) *slot = i;
            return &stacks[i];
        }
    return NULL;
}
const Stack *interior_chest_item(int row) {
    if (ui.chestKind != CHEST_FIELD)
        return chest_stack(row, NULL);
    const Node *node = &g.nodes[ui.storageIndex];
    static Stack contents;
    contents = (Stack){node->item, node->qty};
    return row == 0 && !node->locked && node->remaining && node->item && node->qty ? &contents : NULL;
}
int chest_owner(void) {
    if (ui.chestKind == CHEST_PROPERTY)
        return FREE;
    if (ui.chestKind == CHEST_FIELD)
        return g.nodes[ui.storageIndex].owner;
    return g.buildings[ui.storageIndex].faction;
}
int interior_chest_count(void) {
    if (ui.chestKind == CHEST_FIELD)
        return interior_chest_item(0) != NULL;
    int count = 0;
    while (chest_stack(count, NULL))
        count++;
    return count;
}
void interior_chest_take(int row) {
    Actor *a = &g.actors[g.selected];
    const Stack *contents = interior_chest_item(row);
    if (!contents || !conscious(a))
        return;
    int item = contents->item, owner = chest_owner(), slot = 0;
    Stack *stack = ui.chestKind == CHEST_FIELD ? NULL : chest_stack(row, &slot);
    if (ui.chestKind == CHEST_PROPERTY)
        owner = g.field.seizedOwner[ui.storageIndex][slot];
    if (!inventory_add_owned(a, item, 1, owner)) {
        log_msg("Pack full.");
        return;
    }
    if (stack) {
        if (!--stack->qty) {
            stack->item = NONE;
            if (ui.chestKind == CHEST_PROPERTY)
                g.field.seizedOwner[ui.storageIndex][slot] = 0;
        }
    } else {
        g.nodes[ui.storageIndex].qty--;
    }
    log_msg("Took %s from the chest.", itemDefs[item].name);
    if (owner && ui.chestKind != CHEST_PROPERTY)
        witnessed_theft(a, owner, 6, 18);
}
static int home_destination(int n, int town) {
    const Actor *a = &g.actors[n];
    int night = g.dayTime < 7 || g.dayTime >= 19;
    unsigned shift = g.seed + n * 17u + g.day;
    if (night ? shift % 5 == 0 : (shift + (int)g.dayTime / 3) % 5 != 0)
        return -1;
    int chosen = -1;
    unsigned best = 0;
    for (int j = 0; j < g.buildingCount; j++) {
        const Building *home = &g.buildings[j];
        if (home->type != HOUSE || home->owner >= 0 || home->faction != a->faction ||
            building_town(j) != town)
            continue;
        unsigned score = (shift ^ (j * 65537u)) * 2654435761u;
        if (chosen < 0 || score > best) {
            best = score;
            chosen = j;
        }
    }
    return chosen;
}
int home_occupant(int building) {
    for (int i = 0; i < g.actorCount; i++)
        if (!g.actors[i].squad && conscious(&g.actors[i]) && depth_room(&g.actors[i]) == building + 1)
            return i;
    return -1;
}
static int town_service_building(const Actor *a, int town, int type) {
    int found = -1;
    float nearest = 17;
    for (int b = 0; b < g.buildingCount; b++) {
        const Building *v = &g.buildings[b];
        if (v->type != type || v->owner >= 0 || v->faction != a->faction ||
            building_town(b) != town || !depth_build_ready(b))
            continue;
        float d = distance(a->homeX, a->homeY, v->x, v->y);
        if (d < nearest) {
            nearest = d;
            found = b;
        }
    }
    return found;
}
static void service_post(int building, int service, float *x, float *y) {
    InteriorProp props[ROOM_PROPS];
    int count = interior_props(building, props);
    for (int i = 0; i < count; i++) {
        const InteriorProp *p = &props[i];
        if (p->action != service)
            continue;
        float px = p->w < p->d ? p->x - p->w - .52f : p->x;
        float py = p->w < p->d ? p->y : p->y + p->d + .52f;
        if (interior_walkable(building + 1, px, py)) {
            *x = px;
            *y = py;
        }
        break;
    }
}
int town_npc_routine(int n, float dt) {
    Actor *a = &g.actors[n];
    int town = a->homeTown, room = depth_room(a);
    if (a->squad || town < 0 || town >= TOWNS || !conscious(a) || a->target >= 0 || a->attackPhase ||
        a->carrying >= 0 || a->prisonTime > 0 || g.depth.actors[n].captive || population_owned(n) ||
        a->faction != g.townOwner[town])
        return 0;

    int role = npc_role(a), goal = -1, type = -1;
    float hour = g.dayTime, postX = 3.4f, postY = 3.15f;
    if (role == ROLE_MERCHANT && hour >= 6 && hour < 22)
        type = SHOP;
    else if (role == ROLE_INNKEEPER && hour >= 6)
        type = BAR;
    else if (role == ROLE_SMITH && hour >= 7 && hour < 19)
        type = FORGE;
    else if ((role == ROLE_RECRUIT || role == ROLE_BARBER || role == ROLE_CITIZEN) &&
             ((hour >= 7 && hour < 11) || (hour >= 17 && hour < 22.5f))) {
        type = BAR;
        postX = role == ROLE_RECRUIT ? 2.05f : 4.55f;
        postY = role == ROLE_CITIZEN ? 4.7f : 3.9f;
    } else if (a->kind == SETTLER && (role == ROLE_CITIZEN || role == ROLE_BARBER || hour < 6)) {
        goal = home_destination(n, town);
        postX = 2.7f;
    }
    if (role == ROLE_INNKEEPER) {
        postX = 4;
        postY = 2.12f;
    }
    if (type >= 0)
        goal = room && g.buildings[room - 1].type == type ? room - 1 : town_service_building(a, town, type);
    if (room && g.buildings[room - 1].type == JAIL && watch_town(a) == town)
        for (int i = 0; i < g.actorCount; i++)
            if (g.actors[i].active && !g.actors[i].dead && g.actors[i].prisonTime > 0 &&
                depth_room(&g.actors[i]) == room) {
                goal = room - 1;
                postX = 4.8f;
                postY = 5.1f;
                break;
            }
    if (room) {
        if (goal == room - 1 && (role == ROLE_MERCHANT || role == ROLE_INNKEEPER || role == ROLE_SMITH))
            service_post(goal, role == ROLE_SMITH ? ROOM_SMITH_TRADE : ROOM_TRADE, &postX, &postY);
        float x = goal == room - 1 ? postX : 3.5f;
        float y = goal == room - 1 ? postY : 6.35f;
        float dx = x - depth_x(a), dy = y - depth_y(a);
        if (dx * dx + dy * dy > .12f)
            depth_move_room(a, dx, dy, dt * .65f);
        else if (goal != room - 1)
            actor_leave_room(n);
        else if (type == SHOP)
            a->facing = depth_x(a) < 3.9f ? 0 : 1;
        return 1;
    }
    if (goal < 0)
        return 0;
    float x, y;
    building_door(goal, &x, &y);
    if (distance(a->x, a->y, x, y) > .8f) {
        route_toward(a, x, y, dt * .65f);
        return 1;
    }
    actor_enter_room(n, goal);
    return 1;
}
void actor_enter_room(int n, int building) {
    Actor *a = &g.actors[n];
    DepthActor *z = &g.depth.actors[n];
    z->room = building + 1;
    z->rx = 3.5f;
    z->ry = 6.1f;
    building_door(building, &a->x, &a->y);
    a->routeLen = a->routePos = 0;
    if (n == g.selected) {
        g.interiorBuilding = z->room;
        g.interiorActor = n;
        g.interiorX = z->rx;
        g.interiorY = z->ry;
        z->room = 0;
    }
}
int actor_in_bed(const Actor *a) {
    int bed = a->taskIndex;
    if (a->taskKind != 3)
        return 0;
    if (bed <= -1000)
        return 1;
    if (bed < 0 || bed >= g.buildingCount)
        return 0;
    int type = g.buildings[bed].type;
    return type == BED || type == BAR || type == HOUSE || type == TOWER;
}
static int bed_unoccupied(const Actor *a, float x, float y, int room) {
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *other = &g.actors[i];
        if (other != a && other->active && !other->dead && actor_in_bed(other) &&
            depth_room(other) == room && distance(x, y, depth_x(other), depth_y(other)) < .9f)
            return 0;
    }
    return 1;
}
static void bed_position(Actor *a, int bed, float x, float y, int room) {
    int n = a - g.actors;
    DepthActor *z = &g.depth.actors[n];
    z->room = room;
    if (room) {
        z->rx = x;
        z->ry = y;
        if (n == g.selected) {
            g.interiorX = x;
            g.interiorY = y;
        }
    } else {
        a->x = x;
        a->y = y;
    }
    a->taskKind = 3;
    a->taskIndex = bed;
    a->action = REST_DURATION;
    a->autoattack = a->attackPhase = 0;
    a->target = g.living.command[n] = -1;
    g.field.playingDead[n] = 0;
}
int rest_at(Actor *a, int bed, float x, float y, int room, int price) {
    if (!conscious(a) || a->attackPhase == 1 || a->attackPhase == 2 || a->carrying >= 0 || a->prisonTime > 0)
        return 0;
    if (!bed_unoccupied(a, x, y, room)) {
        log_msg("That bed is occupied.");
        return 0;
    }
    if (g.money < price) {
        log_msg("A bed costs %d silver.", price);
        return 0;
    }
    if (a->taskKind == 3 && a->taskIndex == bed)
        return 1;
    g.money -= price;
    bed_position(a, bed, x, y, room);
    ui.screen = SCREEN_WORLD;
    log_msg("Resting in bed.");
    return 1;
}
int lay_carried_in_bed(Actor *carrier, int place) {
    if (carrier->carrying < 0 || carrier->carrying >= g.actorCount || !conscious(carrier))
        return 0;
    Actor *patient = &g.actors[carrier->carrying];
    if (patient->dead || patient->prisonTime > 0 || g.depth.actors[carrier->carrying].captive)
        return 0;
    int room = depth_room(carrier), bed = -1, cost = 0;
    float x = 0, y = 0, reach = 1.95f;
    if (room) {
        Building *b = &g.buildings[room - 1];
        if (b->type == HOUSE && b->owner < 0 && !lodge_joined(room - 1))
            return 0;
        InteriorProp props[ROOM_PROPS];
        int n = interior_props(room - 1, props);
        for (int i = 0; i < n; i++) {
            if (props[i].action != ROOM_REST) continue;
            float d = distance(depth_x(carrier), depth_y(carrier), props[i].x, props[i].y);
            if (d < reach && bed_unoccupied(patient, props[i].x, props[i].y, room)) {
                bed = room - 1;
                x = props[i].x;
                y = props[i].y;
                reach = d;
            }
        }
        int t = town_near(b->x, b->y, 22);
        cost = b->owner < 0 && t >= 0 ? regional_bed_cost(t) : 0;
    } else {
        for (int i = 0; i < g.buildingCount; i++) {
            Building *b = &g.buildings[i];
            if (b->type != BED || portable_bedroll(i) < 0 || !depth_build_ready(i)) continue;
            float d = distance(carrier->x, carrier->y, b->x, b->y);
            if (d < reach && bed_unoccupied(patient, b->x, b->y, 0)) {
                bed = i;
                x = b->x;
                y = b->y;
                reach = d;
                int t = town_near(x, y, 22);
                cost = b->owner < 0 && t >= 0 && !trail_bed(i) ? regional_bed_cost(t) : 0;
            }
        }
        for (int k = 0; k < POP_GROUPS; k++)
            if (distance(carrier->x, carrier->y, g.population.groups[k].x, g.population.groups[k].y) < 30)
                for (int j = 1; j < population_object_count(k) && j < 5; j++) {
                    float bx, by;
                    if (population_object(k, j, &bx, &by) != 2) continue;
                    float d = distance(carrier->x, carrier->y, bx, by);
                    if (d < reach && bed_unoccupied(patient, bx, by, 0)) {
                        bed = -1000 - k * 4 - (j - 1);
                        x = bx; y = by; reach = d; cost = 0;
                    }
                }
    }
    if (bed == -1 || g.money < cost)
        return 0;
    if (place) {
        g.money -= cost;
        carrier->carrying = patient->carriedBy = -1;
        patient->x = carrier->x;
        patient->y = carrier->y;
        bed_position(patient, bed, x, y, room);
        log_msg("%s laid %s in bed.", carrier->name, patient->name);
    }
    return 1;
}
int building_has_interior(int type) {
    return type <= TOWER || type == FORGE || type == STORE || type == JAIL;
}
int actor_inside(const Actor *a) { return depth_room(a); }
enum { LODGE_FEE = 280 };

static void put_prop(InteriorProp *p, int *count, float x, float y, float w, float d, int art, int action) {
    p[(*count)++] = (InteriorProp){x,y,w,d,art,action};
}
int interior_props(int building, InteriorProp *p) {
    const Building *b = &g.buildings[building];
    int t = building_town(building), style = t < 0 ? 0 : civil_style(t);
    int variant = g.frontier.variant[building], n = 0;
    const TownDesign *plan = town_design(t);
    int layout = (building + (t + 1) * 5) % 3;
    static const int decor[] = {ROOM_STOVE_ART, ROOM_ARMOR_ART, ROOM_SHRINE_ART,
                               ROOM_BALES_ART, ROOM_HEARTH_ART, ROOM_BALES_ART};
    if (variant == BUILD_LODGE) {
        put_prop(p,&n,1.1f,1.7f,.62f,.82f,16,ROOM_REST);
        put_prop(p,&n,1.1f,4.25f,.62f,.82f,16,ROOM_REST);
        put_prop(p,&n,5.3f,1.4f,1.0f,.45f,17,ROOM_MEMBERSHIP);
        put_prop(p,&n,5.65f,4.25f,.35f,.35f,24,ROOM_TRAIN);
        put_prop(p,&n,3.0f,3.0f,1.1f,.7f,23,ROOM_NONE);
        return n;
    }
    if (b->type == JAIL) {
        int beds = plan->custody == 2 ? 4 : plan->custody == 1 ? 2 : 1;
        for (int j = 0; j < beds; j++) {
            float x = j & 1 ? 5.5f : 1.1f;
            float y = j >= 2 ? 2.65f : 1.02f;
            put_prop(p,&n,x,y,.57f,.64f,16,ROOM_NONE);
        }
        if (plan->custody) put_prop(p,&n,5.72f,5.4f,.72f,.38f,17,ROOM_NONE);
        if (plan->custody == 2) put_prop(p,&n,1.1f,5.2f,.5f,.5f,21,ROOM_NONE);
    } else if (b->type == SHOP) {
        int sideCounter = layout == 1 || style == 2;
        if (sideCounter)
            put_prop(p,&n,5.23f,2.67f,.46f,1.24f + plan->marketTier*.11f,17,ROOM_TRADE);
        else
            put_prop(p,&n,layout == 2 ? 4.1f : 3.35f,1.62f,
                     1.28f + plan->marketTier*.15f,.48f,17,ROOM_TRADE);
        if (private_chest(building))
            put_prop(p,&n,sideCounter ? 6.15f : 1.05f,sideCounter ? 1.26f : 3.32f,.43f,.37f,20,ROOM_CHEST);
        put_prop(p,&n,1.1f,1.06f,.62f,.40f,variant == BUILD_ARMORER ? ROOM_ARMOR_ART : decor[style],ROOM_NONE);
        if (layout == 2 && t == 3) {
            put_prop(p,&n,1.4f,4.82f,.76f,.55f,18,ROOM_NONE);
            put_prop(p,&n,5.53f,4.91f,.58f,.52f,20,ROOM_NONE);
        } else
            put_prop(p,&n,sideCounter ? 1.28f : 5.55f,4.8f,.64f,.55f,variant == BUILD_ARMORER ? ROOM_ARMOR_ART : 18,ROOM_NONE);
        if (plan->marketTier)
            put_prop(p,&n,sideCounter ? 3.65f : 5.97f,sideCounter ? .95f : 3.4f,.43f,.42f,21,ROOM_NONE);
        if (plan->marketTier == 2)
            put_prop(p,&n,2.2f,3.7f,.59f,.47f,18,ROOM_NONE);
        if (style == 1 || style == 2)
            put_prop(p,&n,3.5f,5.25f,1.08f,.58f,23,ROOM_NONE);
    } else if (b->type == FORGE) {
        put_prop(p,&n,1.47f,1.47f,.9f,.7f,19,ROOM_FORGE);
        put_prop(p,&n,4.8f,2.18f,1.12f,.47f,17,b->owner < 0 ? ROOM_SMITH_TRADE : ROOM_NONE);
        put_prop(p,&n,1.02f,4.7f,.47f,.48f,20,ROOM_NONE);
        put_prop(p,&n,5.6f,4.7f,.58f,.48f,ROOM_ARMOR_ART,ROOM_NONE);
        put_prop(p,&n,5.72f,1.0f,.40f,.36f,21,ROOM_NONE);
    } else if (b->type == STORE) {
        for (int j = 0; j < 3 + plan->marketTier; j++) {
            float x = j & 1 ? 5.55f : 1.12f;
            float y = 1.30f + (j / 2) * 1.7f;
            put_prop(p,&n,x,y,.48f+(j%3)*.10f,.44f,20,ROOM_STORAGE);
        }
        put_prop(p,&n,3.8f,1.1f,.95f,.45f,21,ROOM_NONE);
    } else if (b->type == BAR) {
        int beds = t < 0 ? 3 : plan->beds;
        int alcove = beds >= 4 && (layout == 1 || style == 2);
        int sideBar = alcove || (t >= 0 && (plan->turn == 2 || plan->plan == 4) && beds <= 3);
        for (int j = 0; j < beds; j++) {
            float x, y, w = .60f, d = .67f;
            if (alcove) {
                x = 1.14f + (j % 3) * 1.88f;
                y = 1.05f + (j / 3) * 1.98f;
            } else {
                x = sideBar ? 1.12f + j * 1.66f : ((j + layout) & 1) ? 5.80f : 1.12f;
                y = sideBar ? 1.20f : 1.11f + (j / 2) * 1.77f;
                if (beds == 1) { w = .76f; d = .56f; }
            }
            put_prop(p,&n,x,y,w,d,16,ROOM_REST);
        }
        if (sideBar) {
            put_prop(p,&n,5.65f,5.15f,.47f,1.05f,17,ROOM_TRADE);
            put_prop(p,&n,1.35f,5.2f,.63f,.49f,decor[style],ROOM_NONE);
            put_prop(p,&n,3.59f,5.34f,.76f,.57f,23,ROOM_NONE);
        } else {
            put_prop(p,&n,3.55f,1.05f,1.11f,.43f,17,ROOM_TRADE);
            if (beds > 2)
                put_prop(p,&n,layout == 1 ? 2.75f : 4.25f,3.7f,.60f,.55f,18,ROOM_NONE);
            if (style == 2 || beds >= 4)
                put_prop(p,&n,3.58f,5.15f,1.10f,.68f,23,ROOM_NONE);
            if (beds < 4)
                put_prop(p,&n,style == 3 ? 5.73f : 1.1f,5.35f,.50f,.43f,decor[style],ROOM_NONE);
        }
    } else {
        float bx = layout == 1 ? 5.45f : 1.28f;
        put_prop(p,&n,bx,layout == 2 ? 3.1f : 1.55f,layout == 2 ? .82f : .59f,layout == 2 ? .54f : .81f,16,ROOM_REST);
        put_prop(p,&n,layout == 1 ? 1.42f : 5.48f,layout == 2 ? 2.05f : 4.08f,.66f,.57f,18,ROOM_NONE);
        put_prop(p,&n,layout == 1 ? 1.12f : 5.62f,1.0f,.65f,.37f,decor[style],ROOM_NONE);
        if (private_chest(building))
            put_prop(p,&n,layout == 1 ? 5.5f : 1.17f,4.85f,.46f,.42f,20,ROOM_CHEST);
        if (style == 2 || style == 1 || layout == 0)
            put_prop(p,&n,3.58f,3.15f,1.08f,.75f,23,ROOM_NONE);
    }
    if (style == 4 || variant == BUILD_ARMORER)
        put_prop(p,&n,6.35f,5.98f,.22f,.24f,22,ROOM_NONE);
    return n;
}

int lodge_joined(int building) {
    return g.frontier.variant[building] == BUILD_LODGE && g.frontier.clanMember;
}
int interior_enter(int building) {
    Actor *a = &g.actors[g.selected];
    if (building < 0 || building >= g.buildingCount || !building_has_interior(g.buildings[building].type) ||
        !conscious(a) || a->prisonTime > 0)
        return 0;
    if (building_door_distance(building, a->x, a->y) > DOOR_REACH)
        return 0;
    if (a->attackPhase == 1 || a->attackPhase == 2)
        return 0;
    if(!depth_build_ready(building)) {log_msg("Finish construction before entering.");return 0;}
    actor_enter_room(g.selected, building);
    a->taskKind = 0;
    a->attackPhase = 0;
    a->target = -1;
    a->movedTimer = 0;
    g.depth.actors[g.selected].trespass = 0;
    g.field.playingDead[g.selected] = 0;
    depth_enter_room(building);
    if (a->carrying >= 0) {
        DepthActor *carried = &g.depth.actors[a->carrying];
        carried->room = building + 1;
        carried->rx = g.interiorX;
        carried->ry = g.interiorY;
    }
    return 1;
}
void interior_leave(void) {
    if (!g.interiorBuilding)
        return;
    Actor *a = &g.actors[g.interiorActor];
    int building = g.interiorBuilding - 1;
    if (a->prisonTime > 0 && !jail_cell_open(a))
        return;
    jail_exit(a);
    building_door(building, &a->x, &a->y);
    if (a->carrying >= 0) g.depth.actors[a->carrying].room = 0;
    a->taskKind = 0;
    a->target = -1;
    g.depth.actors[g.interiorActor].trespass = 0;
    depth_leave_room(g.interiorBuilding-1);
    g.interiorBuilding = 0;
    find_context();
}
int interior_context(void) {
    if (!g.interiorBuilding)
        return -2;
    if (distance(g.interiorX, g.interiorY, 3.5f, 6.7f) < 1.1f)
        return -1;
    InteriorProp p[ROOM_PROPS];
    int n = interior_props(g.interiorBuilding - 1, p), best = -2;
    float near = 2.0f;
    for (int i = 0; i < n; i++)
        if (p[i].action) {
            float d = distance(g.interiorX, g.interiorY, p[i].x, p[i].y);
            if (d < near) {
                near = d;
                best = i;
            }
        }
    return best;
}
const char *interior_prompt(void) {
    int c = interior_context();
    if (c == -1)
        return "LEAVE BUILDING";
    if (c < 0)
        return "WALK TO A SERVICE";
    InteriorProp p[ROOM_PROPS];
    interior_props(g.interiorBuilding - 1, p);
    switch (p[c].action) {
    case ROOM_TRADE:
        return "TRADE AT COUNTER";
    case ROOM_REST:
        return "SLEEP IN BED";
    case ROOM_FORGE:
        return "FORGE EQUIPMENT";
    case ROOM_SMITH_TRADE:
        return "BROWSE WEAPONS & ARMOR";
    case ROOM_STORAGE:
        return "OPEN DEPOT";
    case ROOM_CHEST:
        return "SEARCH OWNED CHEST";
    case ROOM_MEMBERSHIP:
        return g.frontier.clanMember ? "Dustward Clan membership" : "Join Dustward Clan (280 silver)";
    case ROOM_TRAIN:
        return "Train with the practice dummy";
    }
    return "EXPLORE";
}
void interior_interact(void) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a)) {
        log_msg("This survivor cannot act.");
        return;
    }
    int c = interior_context();
    if (c == -1) {
        interior_leave();
        return;
    }
    /* Explicit furniture selection must never be intercepted by a nearby NPC. */
    if (c < 0) {
        log_msg("Walk closer to a counter, bed or workstation.");
        return;
    }
    int id = g.interiorBuilding - 1;
    Building *b = &g.buildings[id];
    InteriorProp p[ROOM_PROPS];
    interior_props(id, p);
    int town = building_town(id);
    ui.selection = ui.scroll = 0;
    switch (p[c].action) {
    case ROOM_TRADE:
    case ROOM_SMITH_TRADE:
        if (town < 0) {
            log_msg("No merchant works here.");
            break;
        }
        if(!depth_trade_open(town))break;
        ui.tradeTown = town;
        ui.roadTrader = 0;
        ui.tradeSmith = p[c].action == ROOM_SMITH_TRADE ? TRADE_SMITH :
                        b->type == BAR ? TRADE_INN :
                        g.frontier.variant[id] == BUILD_ARMORER ? TRADE_ARMORER : TRADE_GENERAL;
        ui.sell = ui.shopFilter = 0;
        ui.screen = SCREEN_TRADE;
        break;
    case ROOM_REST:
        if (b->type == HOUSE && b->owner < 0 && !lodge_joined(id)) {
            log_msg(g.frontier.variant[id] == BUILD_LODGE ? "Dustward beds are reserved for members." : "Private bed.");
            break;
        }
        rest_at(a, id, p[c].x, p[c].y, id + 1,
                b->owner < 0 && town >= 0 && !lodge_joined(id) ? regional_bed_cost(town) : 0);
        break;
    case ROOM_CHEST:
        restock_chest(id);
        ui.storageIndex = id;
        ui.chestKind = CHEST_ROOM;
        ui.screen = SCREEN_CHEST;
        break;
    case ROOM_FORGE:
        ui.screen = SCREEN_FORGE;
        break;
    case ROOM_MEMBERSHIP:
        if (g.frontier.clanMember)
            log_msg("Dustward beds and dummies are open to your squad.");
        else if (g.money < LODGE_FEE)
            log_msg("Dustward membership costs %d silver.", LODGE_FEE);
        else {
            g.money -= LODGE_FEE;
            g.frontier.clanMember = 1;
            log_warn("Joined the Dustward Clan.");
        }
        break;
    case ROOM_TRAIN:
        if (!lodge_joined(id)) {
            log_msg("Only Dustward members can use the dummy.");
            break;
        }
        if (a->gear[0] ? weapon_class_level(a, weapon_class(a->gear[0])) >= 20 : level(a, MARTIAL) >= 20) {
            log_msg("You've outgrown this dummy.");
            break;
        }
        a->taskKind = TASK_TRAIN;
        a->taskIndex = id;
        a->action = 4;
        a->autoattack = 0;
        a->target = -1;
        break;
    case ROOM_STORAGE:
        ui.storageIndex = id;
        ui.storageSide = 0;
        ui.screen = SCREEN_STORAGE;
        break;
    }
}
int interior_walkable(int room, float x, float y) {
    if (room <= 0 || room > g.buildingCount || x < .4f || y < .4f || x > 6.6f || y > 6.6f)
        return 0;
    if (g.buildings[room - 1].type == JAIL &&
        fabsf(y - 3.65f) < .2f && fabsf(x - 3.5f) > .85f)
        return 0;
    InteriorProp p[ROOM_PROPS];
    int n = interior_props(room - 1, p);
    for (int i = 0; i < n; i++)
        if (p[i].art != 23 && fabsf(x - p[i].x) < p[i].w + .18f && fabsf(y - p[i].y) < p[i].d + .18f)
            return 0;
    return 1;
}
void interior_move(float dx, float dy, float dt) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a))
        return;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < .15f)
        return;
    InteriorProp props[ROOM_PROPS];
    int count = interior_props(g.interiorBuilding - 1, props);
    for (int i = 0; i < count; i++)
        if (props[i].action == ROOM_REST &&
            distance(g.interiorX, g.interiorY, props[i].x, props[i].y) < .3f)
            g.interiorX = props[i].x + props[i].w + .25f;
    float wx = (dx + dy) * .7071f, wy = (dy - dx) * .7071f, speed = movement_speed(a) * dt;
    if (length > 1) {
        wx /= length;
        wy /= length;
    }
    float x = g.interiorX + wx * speed, y = g.interiorY + wy * speed;
    if (interior_walkable(g.interiorBuilding, x, g.interiorY))
        g.interiorX = x;
    if (interior_walkable(g.interiorBuilding, g.interiorX, y))
        g.interiorY = y;
    a->taskKind = 0;
    walk_motion(a, g.interiorX - (x - wx * speed), g.interiorY - (y - wy * speed));
}

void training_tick(Actor *a, float dt) {
    int id = a->taskIndex;
    if (id < 0 || id >= g.buildingCount || depth_room(a) != id + 1 || !lodge_joined(id) || !conscious(a)) {
        a->taskKind = 0;
        return;
    }
    if ((a->action -= dt) > 0) return;
    int weapon = a->gear[0], skill = weapon ? weapon_class(a->gear[0]) : MARTIAL;
    float current = weapon ? g.society.weaponXP[a - g.actors][skill] : a->xp[MARTIAL];
    float needed = 12 * 19 * 19 - current;
    if (needed <= 0) {
        a->taskKind = 0;
        a->action = 0;
        return;
    }
    float points = fminf(needed, 78);
    if (weapon) weapon_class_gain(a, weapon, points);
    else gain(a, MARTIAL, points);
    a->action = 4;
}
