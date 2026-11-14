#include "game.h"
#include "settlement_art_ids.h"
#include "furniture_models.h"
#include <math.h>
#include <string.h>
extern int conscious(const Actor *);
const HomeFurniture homeFurniture[FURN_TYPES] = {
    [FURN_BED]   = {"Bed",   190, 16, ROOM_REST, .58f, .81f},
    [FURN_CHEST] = {"Chest", 145, 20, ROOM_HOME_STASH, .47f, .43f},
    [FURN_TABLE] = {"Table", 125, 18, ROOM_NONE, .68f, .57f},
    [FURN_SHELF] = {"Shelf", 105, 21, ROOM_NONE, .60f, .39f},
    [FURN_STOVE] = {"Stove", 245, ART_IRON_STOVE, ROOM_NONE, .52f, .44f}
};
const float homeBays[HOME_BAYS][2] = {
    {1.35f, 1.55f}, {5.65f, 1.55f}, {1.35f, 3.55f},
    {5.65f, 3.55f}, {1.35f, 5.55f}, {5.65f, 5.55f}
};
int home_state(int building) {
    return building >= 0 && building < g.buildingCount ? g.homes.property[building] : HOME_NONE;
}
void homes_init(void) {
    if (g.homes.version)
        return;
    g.homes.version = 1;
    unsigned char occupied[MAX_BUILDINGS] = {0};
    for (int n = 0; n < g.actorCount; n++) {
        int room = depth_room(&g.actors[n]);
        if (g.actors[n].active && room > 0 && room <= g.buildingCount)
            occupied[room - 1] = 1;
    }
    for (int t = 0; t < TOWNS; t++) {
        if (towns[t].faction == HOSTILE)
            continue;
        int spare = -1, houses = 0;
        for (int i = 0; i < g.buildingCount; i++) {
            const Building *b = &g.buildings[i];
            if (b->type != HOUSE || b->owner >= 0 || g.frontier.variant[i] != BUILD_NORMAL ||
                building_town(i) != t)
                continue;
            int landmark = 0;
            for (int k = 0; k < LANDMARKS; k++)
                landmark |= g.depth.siteBuilding[k] == i;
            if (landmark)
                continue;
            houses++;
            float x, y;
            building_door(i, &x, &y);
            if (!occupied[i] && can_walk(x,y) &&
                (spare < 0 || !g.townLife.chestRefresh[i]))
                spare = i;
        }
        if (spare >= 0 && houses >= 2)
            g.homes.property[spare] = HOME_FOR_SALE;
    }
}
int home_price(int building) {
    int town = building_town(building);
    if (town < 0)
        return 0;
    const TownDesign *plan = town_design(town);
    static const int importance[] = {0, 650, 1600, 6500};
    const Town *center = &towns[town];
    const Building *plot = &g.buildings[building];
    int smith = 0, specialist = 0;
    for (int i = 0; i < g.buildingCount; i++) {
        const Building *service = &g.buildings[i];
        int forge = service->type == FORGE, armor = g.frontier.variant[i] == BUILD_ARMORER;
        if ((forge || armor) && service->owner < 0 &&
            fabsf(service->x - center->x) < 24 && fabsf(service->y - center->y) < 24 &&
            building_town(i) == town && depth_build_ready(i)) {
            smith |= forge;
            specialist |= armor;
        }
    }
    int amenities = plan->beds * 125 + plan->marketTier * 400 + plan->custody * 250 +
                    smith * 1800 + specialist * 950;
    int frontage = (int)fmaxf(0, 15 - distance(plot->x, plot->y, center->x, center->y)) * 50;
    return 5200 + importance[plan->importance] + plan->location * 400 + amenities + frontage;
}
void home_open(int building) {
    ui.homeSite = building;
    ui.homeBay = 0;
    ui.selection = FURN_BED;
    ui.screen = SCREEN_HOME;
}
int home_buy(int building) {
    Actor *a = &g.actors[g.selected];
    if (home_state(building) != HOME_FOR_SALE || !conscious(a) || depth_room(a) ||
        a->prisonTime > 0 || g.depth.actors[g.selected].captive ||
        building_door_distance(building, a->x, a->y) > DOOR_REACH)
        return 0;
    int town = building_town(building);
    if (town < 0)
        return 0;
    int faction = g.townOwner[town];
    if (g.rep[faction] < -25 || wanted_amount(a, faction)) {
        log_msg("The town won't sell you the deed.");
        return 0;
    }
    int price = home_price(building);
    if (g.money < price) {
        log_msg("The deed costs %d silver.", price);
        return 0;
    }
    g.money -= price;
    g.homes.property[building] = HOME_OWNED;
    log_warn("Bought a house in %s.", towns[town].name);
    return 1;
}
int home_furnish(int building, int bay, int type) {
    if (bay < 0 || bay >= HOME_BAYS) return 0;
    placement_init();
    return home_place(building, bay, type, homeBays[bay][0], homeBays[bay][1]);
}

int homes_validate(const Game *save) {
    const Homes *h = &save->homes;
    if(save->plans.version<0||save->plans.version>1)return 0;
    for(int b=0;b<MAX_BUILDINGS;b++){
        if(!isfinite(save->plans.campLastNear[b])||save->plans.campLastNear[b]<0)return 0;
        for(int i=0;i<HOME_PLACES;i++){
            int type=save->plans.furniture[b][i];
            if(type>=FURN_TYPES||!isfinite(save->plans.x[b][i])||!isfinite(save->plans.y[b][i])||
               (type&&(b>=save->buildingCount||h->property[b]!=HOME_OWNED||!save->plans.version||
                save->plans.x[b][i]<.35f||save->plans.x[b][i]>6.65f||save->plans.y[b][i]<.35f||save->plans.y[b][i]>6.65f)))return 0;
        }
    }
    for(int n=0;n<MAX_ACTORS;n++)if(!isfinite(save->plans.corpseSince[n])||save->plans.corpseSince[n]<0)return 0;
    for(int t=0;t<TOWNS;t++)if(save->plans.depotClerk[t]>save->actorCount)return 0;
    for(int c=0;c<CONTRACTS;c++)if(save->plans.dispatchPending[c]>1)return 0;
    if (h->version < 0 || h->version > 1)
        return 0;
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        int state = h->property[i];
        if (state > HOME_OWNED || (state && (!h->version || i >= save->buildingCount ||
            save->buildings[i].type != HOUSE || save->buildings[i].owner >= 0 ||
            save->frontier.variant[i] != BUILD_NORMAL)))
            return 0;
        for (int b = 0; b < HOME_BAYS; b++)
            if (h->furniture[i][b] >= FURN_TYPES || (h->furniture[i][b] && state != HOME_OWNED))
                return 0;
    }
    for (int i = 0; i < HOME_STASH; i++) {
        const Stack *s = &h->stash[i];
        if (s->item < 0 || s->item >= ITEMS || s->qty < 0 || s->qty > 999 ||
            h->stolen[i] >= FACTIONS || (!s->item && (s->qty || h->stolen[i])) ||
            (s->item && (!h->version || !s->qty)))
            return 0;
    }
    return 1;
}

static int room_has_chest(int building) {
    const Building *b = &g.buildings[building];
    if (wilderness_site(building) >= 0) return 1;
    if (ruined_building(building))
        return b->type == SHOP;
    return b->owner < 0 && b->faction > FREE && !home_state(building) &&
           (b->type == SHOP || (b->type == HOUSE &&
            (g.townLife.chestRefresh[building] || (g.seed + building * 17u) % 5 != 0)));
}
static void restock_chest(int building) {
    if (ruined_building(building) || !room_has_chest(building) ||
        g.day < g.townLife.chestRefresh[building])
        return;
    Building *b = &g.buildings[building];
    unsigned mix = (g.seed ^ (unsigned)building * 2654435761u) + g.day * 97u;
    static const int home[] = {CLOTH, WATER, WOOD, GRAIN, OATCAKE, JERKY};
    static const int store[] = {MEDKIT, IRONBAR, CLOTH, RATION, BOLTS, HIDE};
    memset(b->storage, 0, sizeof(b->storage));
    if (b->type == SHOP) {
        for (int i = 0; i < 2; i++) {
            int item = store[(mix % 6 + i * (1 + mix % 5)) % 6];
            if (item == MEDKIT && mix % 19)
                item = portion_item(MEDKIT, 30 + mix % 61);
            b->storage[i] = (Stack){item, i == 0 ? 2 : 1};
        }
    } else {
        b->storage[0] = (Stack){home[mix % 6],1 + (mix >> 6) % 3};
        b->storage[1] = (Stack){home[(mix % 6 + 1 + (mix >> 4) % 5) % 6],1 + (mix >> 8) % 2};
        if ((mix >> 10) % 5 == 0) {
            int spare = (mix >> 13) % 2 ? portion_item(MEDKIT,35 + (mix >> 16) % 40) : BEDROLL_ITEM;
            b->storage[2] = (Stack){spare,1};
        }
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
            !room_has_chest(ui.storageIndex))
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
static int home_hours(int town) {
    int morning = 7 + (town + g.day) % 3;
    return g.dayTime < 7 || g.dayTime >= 21 ||
           (g.dayTime >= morning && g.dayTime < morning + 3) ||
           (g.dayTime >= 16 + town % 3 && g.dayTime < 17 + town % 3);
}
static int home_destination(int n, int town) {
    int home = g.residentHome[n] - 1;
    return home >= 0 && home_hours(town) && g.buildings[home].faction == g.actors[n].faction
           ? home : -1;
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
typedef struct {
    float x, y, until;
    int room, hour;
} TownStop;
static TownStop townStops[MAX_ACTORS];

void town_routines_reset(void) {
    memset(townStops, 0, sizeof(townStops));
}

static int open_stop(int n, int room, float x, float y, float gap) {
    for (int j = 0; j < g.actorCount; j++) {
        const Actor *b = &g.actors[j];
        if (n == j || !b->active || b->dead || b->carriedBy >= 0)
            continue;
        if (depth_room(b) == room && distance(x,y,depth_x(b),depth_y(b)) < gap)
            return 0;
        const TownStop *post = &townStops[j];
        if (post->until > g.time && post->room == room && distance(x,y,post->x,post->y) < gap)
            return 0;
    }
    return 1;
}
static int pick_room_post(int n, int building, unsigned shift, float *x, float *y) {
    static const float places[][2] = {
        {1.25f,3.5f},{2.25f,4.4f},{4.85f,3.25f},{5.7f,3.8f},{4.7f,5.8f},
        {1.8f,5.6f},{2.95f,2.55f},{4.3f,2.55f},{2.45f,1.45f},{5.85f,1.5f},
        {3.45f,4.65f},{1.3f,2.4f},{4.3f,1.25f},{5.65f,5.85f},{2.65f,5.55f}
    };
    float staffX = 3.55f, staffY = 2.3f;
    service_post(building, ROOM_TRADE, &staffX, &staffY);
    unsigned count = sizeof(places) / sizeof(places[0]);
    for (unsigned i = 0; i < count; i++) {
        unsigned spot = (shift + i) % count;
        float px = places[spot][0], py = places[spot][1];
        if (distance(px,py,staffX,staffY) < 1.6f || !interior_walkable(building + 1, px, py) ||
            !open_stop(n, building + 1, px, py, 1.45f))
            continue;
        *x = px;
        *y = py;
        return 1;
    }
    return 0;
}
void home_assign_resident(int n, int building) {
    g.residentHome[n] = building + 1;
    if (g.time > 0 || !home_hours(g.actors[n].homeTown))
        return;
    float x, y;
    if (pick_room_post(n, building, g.seed + (unsigned)n, &x, &y)) {
        actor_enter_room(n, building);
        g.depth.actors[n].rx = x;
        g.depth.actors[n].ry = y;
    }
}
static void pick_street_post(int n, float cx, float cy, float radius, unsigned shift, float *x, float *y) {
    for (int k = 0; k < 16; k++) {
        float angle = ((shift + k * 5) % 16) * .39269908f;
        float px = cx + cosf(angle) * radius, py = cy + sinf(angle) * radius;
        if (can_walk(px, py) && open_stop(n, 0, px, py, 1.65f)) {
            *x = px;
            *y = py;
            return;
        }
    }
    *x = cx;
    *y = cy;
    civil_clear_position(x,y);
}
static TownStop town_stop(int n, int town, int role) {
    Actor *a = &g.actors[n];
    int hour = (int)g.dayTime;
    unsigned shift = (unsigned)(g.time / 35.f + n * .41f) + n * 17u + g.seed;
    TownStop next = {.hour = hour, .until = g.time + 28 + shift % 21};
    int type = role == ROLE_MERCHANT ? SHOP : role == ROLE_INNKEEPER ? BAR : role == ROLE_SMITH ? FORGE : role == ROLE_DEPOT ? STORE : -1;
    if (type >= 0) {
        int business = town_service_building(a, town, type);
        if (business >= 0) {
            int working = 1;
            if (working && (g.time < 35 || shift % 7 != 0)) {
                next.room = business + 1;
                next.x = 3.4f;
                next.y = 2.6f;
                service_post(business, type == FORGE ? ROOM_SMITH_TRADE : type == STORE ? ROOM_STORAGE : ROOM_TRADE, &next.x, &next.y);
                if (open_stop(n,next.room,next.x,next.y,1.15f))
                    return next;
            }
            float x, y;
            building_door(business,&x,&y);
            next.room = 0;
            pick_street_post(n,x,y,1.8f,shift,&next.x,&next.y);
            return next;
        }
    }
    int home = home_destination(n, town);
    if (home >= 0 && pick_room_post(n, home, shift, &next.x, &next.y)) {
        next.room = home + 1;
        return next;
    }
    int mealtime = (hour >= 7 && hour <= 10) || (hour >= 17 && hour <= 22);
    if (hour >= 7 && hour <= 23 && shift % 10 < (mealtime ? 7u : 3u)) {
        int inn = town_service_building(a,town,BAR);
        if (inn >= 0 && pick_room_post(n,inn,shift,&next.x,&next.y)) {
            next.room = inn + 1;
            return next;
        }
    }
    pick_street_post(n,towns[town].x,towns[town].y,4.5f + (shift % 4)*2.05f,shift,&next.x,&next.y);
    return next;
}
int town_npc_routine(int n, float dt) {
    Actor *a = &g.actors[n];
    int town = a->homeTown, room = depth_room(a);
    if (a->squad || a->voice == ROLE_MERCENARY || a->taskKind == TASK_TREAT || actor_in_bed(a) ||
        a->kind == BEAST || town < 0 || town >= TOWNS || !conscious(a) || a->target >= 0 || a->attackPhase ||
        a->carrying >= 0 || a->prisonTime > 0 || g.depth.actors[n].captive || population_owned(n) ||
        a->faction != g.townOwner[town])
        return 0;
    int role = npc_role(a);
    if (a->kind == GUARD) {
        int jail = g.field.jail[town];
        if (jail && !strcmp(a->name,"Jailkeeper")) {
            if (room == jail) {
                room_route_toward(a,4.7f,5.45f,dt*.65f);
                return 1;
            }
            if (!room) {
                float x, y;
                building_door(jail-1,&x,&y);
                if (distance(a->x,a->y,x,y) > .85f)
                    route_toward(a,x,y,dt*.65f);
                return 1;
            }
        }
        if (room && g.buildings[room - 1].type == JAIL)
            for (int j = 0; j < g.actorCount; j++)
                if (g.actors[j].active && !g.actors[j].dead && g.actors[j].prisonTime > 0 &&
                    depth_room(&g.actors[j]) == room) {
                    room_route_toward(a,4.8f,5.1f,dt*.65f);
                    return 1;
                }
        if (!room) return 0;
    } else if (role != ROLE_CITIZEN && role != ROLE_BARBER && role != ROLE_RECRUIT &&
               role != ROLE_INNKEEPER && role != ROLE_MERCHANT && role != ROLE_SMITH && role != ROLE_DEPOT)
        return 0;
    TownStop *post = &townStops[n];
    if (post->until <= g.time || post->hour != (int)g.dayTime)
        *post = town_stop(n,town,role);
    if (room) {
        float x = post->room == room ? post->x : 3.5f;
        float y = post->room == room ? post->y : 6.35f;
        if (distance(depth_x(a),depth_y(a),x,y) > .25f)
            room_route_toward(a,x,y,dt*.65f);
        else if (post->room != room)
            actor_leave_room(n);
        return 1;
    }
    if (!post->room) {
        route_toward(a,post->x,post->y,dt*.56f);
        return 1;
    }
    float x, y;
    building_door(post->room-1,&x,&y);
    if (distance(a->x,a->y,x,y) > .9f)
        route_toward(a,x,y,dt*.72f);
    else if (open_stop(n,post->room,3.5f,6.1f,.78f))
        actor_enter_room(n,post->room-1);
    return 1;
}
void actor_enter_room(int n, int building) {
    Actor *a = &g.actors[n];
    DepthActor *z = &g.depth.actors[n];
    z->room = building + 1;
    g.innFloor[n] = 0;
    z->rx = 3.5f;
    z->ry = 6.1f;
    building_door(building, &a->x, &a->y);
    a->routeLen = a->routePos = 0;
    a->routeClock = 0;
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
    if (a->taskKind != 3 || a->carriedBy >= 0)
        return 0;
    if (bed <= -1000)
        return 1;
    if (bed < 0 || bed >= g.buildingCount)
        return 0;
    int type = g.buildings[bed].type;
    return type == BED || type == BAR || type == HOUSE || type == TOWER;
}
int room_bed_free(const Actor *a, float x, float y, int room, int floor) {
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *other = &g.actors[i];
        if (other != a && other->active && !other->dead && actor_in_bed(other) &&
            depth_room(other) == room && actor_floor(other) == floor &&
            distance(x, y, depth_x(other), depth_y(other)) < .9f)
            return 0;
    }
    return 1;
}
void place_in_bed(Actor *a, int bed, float x, float y, int room) {
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
        g.innFloor[n] = 0;
        a->x = x;
        a->y = y;
    }
    a->taskKind = 3;
    a->taskIndex = bed;
    a->action = REST_DURATION;
    a->autoattack = a->attackPhase = a->sneaking = 0;
    a->movedTimer = 0;
    a->target = g.living.command[n] = -1;
    g.field.playingDead[n] = 0;
}
int rest_at(Actor *a, int bed, float x, float y, int room, int price) {
    if (!conscious(a) || a->attackPhase == 1 || a->attackPhase == 2 || a->carrying >= 0 || a->prisonTime > 0)
        return 0;
    if (!room_bed_free(a, x, y, room, actor_floor(a))) {
        log_msg("That bed is occupied.");
        return 0;
    }
    if (g.money < price) {
        log_msg("A bed costs %d silver.", price);
        return 0;
    }
    if (a->taskKind == 3 && a->taskIndex == bed &&
        distance(depth_x(a), depth_y(a), x, y) < .3f)
        return 1;
    g.money -= price;
    place_in_bed(a, bed, x, y, room);
    ui.screen = SCREEN_WORLD;
    log_msg(room ? "Resting in bed." : "Resting on a bedroll.");
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
        if (b->type == HOUSE && b->owner < 0 && home_state(room - 1) != HOME_OWNED && !lodge_joined(room - 1))
            return 0;
        InteriorProp props[ROOM_PROPS];
        int n = interior_floor_props(room - 1, actor_floor(carrier), props);
        for (int i = 0; i < n; i++) {
            if (props[i].action != ROOM_REST) continue;
            float d = distance(depth_x(carrier), depth_y(carrier), props[i].x, props[i].y);
            if (d < reach && room_bed_free(patient, props[i].x, props[i].y, room, actor_floor(carrier))) {
                bed = room - 1;
                x = props[i].x;
                y = props[i].y;
                reach = d;
            }
        }
        int t = town_near(b->x, b->y, 22);
        cost = b->owner < 0 && t >= 0 && home_state(room - 1) != HOME_OWNED && !lodge_joined(room - 1)
            ? regional_bed_cost(t) : 0;
    } else {
        for (int i = 0; i < g.buildingCount; i++) {
            Building *b = &g.buildings[i];
            if (b->type != BED || portable_bedroll(i) < 0 || !depth_build_ready(i)) continue;
            float d = distance(carrier->x, carrier->y, b->x, b->y);
            if (d < reach && room_bed_free(patient, b->x, b->y, 0, 0)) {
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
                    if (d < reach && room_bed_free(patient, bx, by, 0, 0)) {
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
        g.innFloor[patient - g.actors] = actor_floor(carrier);
        place_in_bed(patient, bed, x, y, room);
        log_msg(room ? "%s laid %s in bed." : "%s laid %s on a bedroll.", carrier->name, patient->name);
    }
    return 1;
}
int building_has_interior(int type) {
    return type <= TOWER || type == FORGE || type == STORE || type == JAIL;
}
int actor_inside(const Actor *a) { return depth_room(a); }
enum { LODGE_FEE = 280 };

static void put_prop(InteriorProp *p, int *count, float x, float y, float w, float d, int art, int action) {
    if (action == ROOM_REST) {
        w *= .86f;
        d *= .86f;
    }
    p[(*count)++] = (InteriorProp){x,y,w,d,art,action};
}
static int room_layout(int building, int t, int style, int floor, InteriorProp *p) {
    const Building *b = &g.buildings[building];
    int variant = g.frontier.variant[building], n = 0;
    const TownDesign *plan = town_design(t);
    int layout = (building + (t + 1) * 5) % 3;
    static const int decor[] = {ART_IRON_STOVE, ART_ARMOR_STAND, ART_SIDE_ALTAR,
                               ART_CARAVAN_BALES, ART_IRON_STOVE, ART_CARAVAN_BALES};
    if (wilderness_site(building) >= 0) {
        int kind = wilderness_site(building) % 8;
        int alternate = building & 1;
        put_prop(p,&n,alternate?1.1f:5.7f,2.0f,.43f,.37f,20,ROOM_CHEST);
        put_prop(p,&n,3.6f,1.1f,.70f,.32f,17,ROOM_RUIN_RECORD);
        if (kind==0 || kind==4) { /* foundry / quarry machinery */
            put_prop(p,&n,1.4f,4.4f,.60f,.57f,19,ROOM_NONE);
            put_prop(p,&n,5.5f,4.5f,.52f,.44f,ART_IRON_STOVE,ROOM_NONE);
            put_prop(p,&n,3.4f,3.0f,.35f,.4f,21,ROOM_NONE);
        } else if (kind==1 || kind==5) { /* watch post / fortified refuge */
            put_prop(p,&n,1.3f,4.6f,.58f,.81f,16,ROOM_REST);
            put_prop(p,&n,5.4f,4.4f,.35f,.35f,ART_PRACTICE_POST,ROOM_NONE);
            put_prop(p,&n,5.4f,1.1f,.45f,.45f,ART_ARMOR_STAND,ROOM_NONE);
        } else if (kind==2 || kind==6) { /* shrine / former clinic */
            put_prop(p,&n,3.3f,2.5f,.55f,.45f,ART_SIDE_ALTAR,ROOM_NONE);
            put_prop(p,&n,1.2f,4.7f,.58f,.81f,16,ROOM_REST);
            put_prop(p,&n,5.5f,4.7f,.58f,.81f,16,ROOM_REST);
        } else { /* caravan stop / warehouse */
            put_prop(p,&n,1.2f,4.7f,.46f,.47f,ART_CARAVAN_BALES,ROOM_NONE);
            put_prop(p,&n,5.5f,4.4f,.46f,.47f,ART_CARAVAN_BALES,ROOM_NONE);
            put_prop(p,&n,1.3f,1.1f,.6f,.4f,21,ROOM_NONE);
        }
        return n;
    }
    if (ruined_building(building)) {
        if (b->type == SHOP)
            put_prop(p,&n,5.75f,2.0f,.43f,.37f,20,ROOM_CHEST);
        if (b->type == FORGE)
            put_prop(p,&n,1.35f,1.55f,.60f,.57f,19,ROOM_NONE);
        if (b->type == BAR || b->type == SHOP)
            put_prop(p,&n,2.45f,1.22f,.89f,.34f,17,ROOM_NONE);
        put_prop(p,&n,1.15f,4.35f,.46f,.47f,ART_CARAVAN_BALES,ROOM_NONE);
        return n;
    }
    if (home_state(building)) return home_layout(building, p);
    if (variant == BUILD_LODGE) {
        put_prop(p,&n,1.1f,1.7f,.62f,.82f,16,ROOM_REST);
        put_prop(p,&n,1.1f,4.25f,.62f,.82f,16,ROOM_REST);
        put_prop(p,&n,5.3f,1.4f,.86f,.37f,17,ROOM_MEMBERSHIP);
        put_prop(p,&n,5.65f,4.25f,.35f,.35f,ART_PRACTICE_POST,ROOM_TRAIN);
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
        if (plan->custody) put_prop(p,&n,5.72f,5.4f,.65f,.33f,17,ROOM_NONE);
        if (plan->custody == 2) put_prop(p,&n,1.1f,5.2f,.5f,.5f,21,ROOM_NONE);
    } else if (b->type == SHOP) {
        int sideCounter = layout == 1 || style == 2;
        if (sideCounter)
            put_prop(p,&n,5.23f,2.67f,.36f,.86f + plan->marketTier*.09f,17,ROOM_TRADE);
        else
            put_prop(p,&n,layout == 2 ? 4.1f : 3.35f,1.62f,
                     .94f + plan->marketTier*.10f,.36f,17,ROOM_TRADE);
        if (room_has_chest(building))
            put_prop(p,&n,sideCounter ? 6.15f : 1.05f,sideCounter ? 1.26f : 3.32f,.43f,.37f,20,ROOM_CHEST);
        put_prop(p,&n,1.1f,1.06f,.50f,.35f,variant == BUILD_ARMORER ? ART_ARMOR_STAND : decor[style],ROOM_NONE);
        if (layout == 2 && t == 3) {
            put_prop(p,&n,1.4f,4.82f,.76f,.55f,18,ROOM_NONE);
            put_prop(p,&n,5.53f,4.91f,.58f,.52f,20,ROOM_NONE);
        } else
            put_prop(p,&n,sideCounter ? 1.28f : 5.55f,4.8f,
                     variant == BUILD_ARMORER ? .50f : .64f, variant == BUILD_ARMORER ? .38f : .55f,
                     variant == BUILD_ARMORER ? ART_ARMOR_STAND : 18,ROOM_NONE);
        if (plan->marketTier)
            put_prop(p,&n,sideCounter ? 3.65f : 5.97f,sideCounter ? .95f : 3.4f,.43f,.42f,21,ROOM_NONE);
        if (plan->marketTier == 2)
            put_prop(p,&n,2.2f,3.7f,.59f,.47f,18,ROOM_NONE);
        if (style == 1 || style == 2)
            put_prop(p,&n,3.5f,5.25f,1.08f,.58f,23,ROOM_NONE);
    } else if (b->type == FORGE) {
        put_prop(p,&n,1.47f,1.47f,.74f,.57f,19,ROOM_FORGE);
        put_prop(p,&n,4.8f,2.18f,.94f,.36f,17,b->owner < 0 ? ROOM_SMITH_TRADE : ROOM_NONE);
        put_prop(p,&n,1.02f,4.7f,.47f,.48f,20,ROOM_NONE);
        put_prop(p,&n,5.6f,4.7f,.50f,.36f,ART_ARMOR_STAND,ROOM_NONE);
        put_prop(p,&n,5.72f,1.0f,.40f,.36f,21,ROOM_NONE);
    } else if (b->type == STORE) {
        for (int j = 0; j < 3 + plan->marketTier; j++) {
            float x = j & 1 ? 5.55f : 1.12f;
            float y = 1.30f + (j / 2) * 1.7f;
            put_prop(p,&n,x,y,.48f+(j%3)*.10f,.44f,20,ROOM_NONE);
        }
        put_prop(p,&n,3.8f,1.1f,.62f,.38f,21,ROOM_NONE);
        put_prop(p,&n,3.4f,2.05f,.80f,.31f,17,ROOM_STORAGE);
    } else if (b->type == BAR) {
        int upstairs = inn_has_upstairs(building);
        if (upstairs)
            put_prop(p,&n,5.94f,5.88f,.46f,.76f,floor ? ART_STAIRS_DOWN : ART_INN_STAIRS,ROOM_STAIRS);
        if (floor) {
            for (int j = 0; j < plan->beds; j++) {
                float x = j % 2 ? 5.61f : 1.39f;
                float y = .95f + (j / 2) * 1.60f;
                put_prop(p,&n,x,y,.53f,.65f,16,ROOM_REST);
            }
            put_prop(p,&n,3.49f,1.08f,.47f,.35f,21,ROOM_NONE);
        } else {
            put_prop(p,&n,layout == 1 ? 4.22f : 3.43f,1.26f,1.06f,.39f,ART_INN_BAR,ROOM_TRADE);
            put_prop(p,&n,2.17f,3.67f,.59f,.48f,18,ROOM_NONE);
            if (upstairs || plan->beds >= 3)
                put_prop(p,&n,4.53f,3.56f,.56f,.45f,18,ROOM_NONE);
            if (upstairs)
                put_prop(p,&n,1.12f,5.35f,.54f,.43f,decor[style],ROOM_NONE);
            else {
                put_prop(p,&n,5.67f,4.96f,.54f,.64f,16,ROOM_REST);
                if (plan->beds >= 3)
                    put_prop(p,&n,1.12f,5.25f,.54f,.64f,16,ROOM_REST);
                else
                    put_prop(p,&n,1.12f,1.01f,.51f,.35f,decor[style],ROOM_NONE);
            }
        }
    } else {
        float bx = layout == 1 ? 5.45f : 1.28f;
        put_prop(p,&n,bx,layout == 2 ? 3.1f : 1.55f,layout == 2 ? .82f : .59f,layout == 2 ? .54f : .81f,16,ROOM_REST);
        put_prop(p,&n,layout == 1 ? 1.42f : 5.48f,layout == 2 ? 2.05f : 4.08f,.66f,.57f,18,ROOM_NONE);
        put_prop(p,&n,layout == 1 ? 1.12f : 5.62f,1.0f,.65f,.37f,decor[style],ROOM_NONE);
        if (room_has_chest(building))
            put_prop(p,&n,layout == 1 ? 5.5f : 1.17f,4.85f,.46f,.42f,20,ROOM_CHEST);
        if (style == 2 || style == 1 || layout == 0)
            put_prop(p,&n,3.58f,3.15f,1.08f,.75f,23,ROOM_NONE);
    }
    if (style == 4 || variant == BUILD_ARMORER)
        put_prop(p,&n,6.35f,5.98f,.22f,.24f,22,ROOM_NONE);
    return n;
}

int inn_has_upstairs(int building) {
    return building >= 0 && building < g.buildingCount && g.buildings[building].type == BAR &&
           !ruined_building(building) && town_design(building_town(building))->beds >= 4;
}

void inn_change_floor(Actor *a, int floor) {
    int n = a - g.actors;
    g.innFloor[n] = floor;
    DepthActor *z = &g.depth.actors[n];
    z->rx = 4.86f - (n == g.selected ? 0 : .3f * (1 + n % 3));
    z->ry = 5.72f;
    if (n == g.selected) {
        g.interiorX = z->rx;
        g.interiorY = z->ry;
    }
    a->routeLen = a->routePos = 0;
    a->routeClock = 0;
    a->target = -1;
    if (a->carrying >= 0) {
        g.innFloor[a->carrying] = floor;
        g.depth.actors[a->carrying].rx = z->rx;
        g.depth.actors[a->carrying].ry = z->ry;
    }
}

int inn_follow_stairs(Actor *a, int floor, float dt) {
    int room = depth_room(a);
    if (actor_floor(a) == floor || !inn_has_upstairs(room - 1))
        return 0;
    if (distance(depth_x(a), depth_y(a), 4.86f, 5.72f) < .65f)
        inn_change_floor(a, floor);
    else
        room_route_toward(a, 4.86f, 5.72f, dt);
    return 1;
}

int interior_floor_props(int building, int floor, InteriorProp *p) {
    int town = building_town(building);
    int style = town < 0 ? 0 : civil_style(town);
    int count = room_layout(building,town,style,floor,p);
    for (int i = 0; i < count; i++) {
        InteriorProp *prop = &p[i];
        if (prop->art == 17 && prop->d > prop->w)
            prop->art = ART_SIDE_COUNTER;
        const FurnitureModel *model = &furnitureModels[furniture_art_index(prop->art)];
        float span = fminf(prop->w + prop->d, model->span / (2*ROOM_MAX_SCALE));
        float share = furniture_is_bed(prop->art) && prop->d > prop->w ? 1-model->xShare : model->xShare;
        prop->w = span*share;
        prop->d = span-prop->w;
    }
    return count;
}

int interior_props(int building, InteriorProp *p) {
    return interior_floor_props(building, 0, p);
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
    if (home_state(building) == HOME_FOR_SALE) {
        home_open(building);
        return 1;
    }
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
        g.innFloor[a->carrying] = 0;
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
    if (actor_floor(a) || (a->prisonTime > 0 && !jail_cell_open(a)))
        return;
    jail_exit(a);
    building_door(building, &a->x, &a->y);
    if (a->carrying >= 0) {
        g.depth.actors[a->carrying].room = 0;
        g.innFloor[a->carrying] = 0;
    }
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
    if (!actor_floor(&g.actors[g.selected]) && distance(g.interiorX, g.interiorY, 3.5f, 6.7f) < 1.1f)
        return -1;
    InteriorProp p[ROOM_PROPS];
    int n = interior_floor_props(g.interiorBuilding - 1, actor_floor(&g.actors[g.selected]), p), best = -2;
    float near = 2.0f;
    for (int i = 0; i < n; i++)
        if (p[i].action) {
            float d = distance(g.interiorX, g.interiorY, p[i].x, p[i].y);
            if (d < near) {
                near = d;
                best = i;
            }
        }
    return best == -2 && home_state(g.interiorBuilding - 1) == HOME_OWNED ? -3 : best;
}
const char *interior_prompt(void) {
    int c = interior_context();
    if (c == -1)
        return "LEAVE BUILDING";
    if (c == -3)
        return "FURNISH HOUSE";
    if (c < 0)
        return ruined_building(g.interiorBuilding - 1) ? "RUINS" : "WALK TO A SERVICE";
    InteriorProp p[ROOM_PROPS];
    interior_floor_props(g.interiorBuilding - 1, actor_floor(&g.actors[g.selected]), p);
    switch (p[c].action) {
    case ROOM_STAIRS:
        return actor_floor(&g.actors[g.selected]) ? "GO DOWNSTAIRS" : "GO UPSTAIRS";
    case ROOM_RUIN_RECORD:
        return "READ WEATHERED RECORD";
    case ROOM_TRADE:
        return "TRADE AT COUNTER";
    case ROOM_REST:
        return "SLEEP IN BED";
    case ROOM_FORGE:
        return "FORGE EQUIPMENT";
    case ROOM_SMITH_TRADE:
        return "BROWSE WEAPONS & ARMOR";
    case ROOM_STORAGE:
        return "DEPOT SERVICES";
    case ROOM_HOME_STASH:
        return "OPEN SHARED STASH";
    case ROOM_CHEST:
        return ruined_building(g.interiorBuilding - 1) ? "SEARCH SALVAGE CHEST" : "SEARCH OWNED CHEST";
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
    if (c == -3) {
        home_open(g.interiorBuilding - 1);
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
    interior_floor_props(id, actor_floor(a), p);
    int town = building_town(id);
    ui.selection = ui.scroll = 0;
    switch (p[c].action) {
    case ROOM_RUIN_RECORD:
        log_msg("%s", wilderness_record(wilderness_site(id)));
        break;
    case ROOM_STAIRS: {
        int floor = actor_floor(a);
        if (a->attackPhase || a->prisonTime > 0)
            break;
        for (int i = 0; i < g.actorCount; i++) {
            Actor *other = &g.actors[i];
            if (i != g.selected && other->squad && conscious(other) && !other->taskKind &&
                assigned_job(other) == JOB_FOLLOW && depth_distance(a, other) < 1.9f)
                inn_change_floor(other, !floor);
        }
        inn_change_floor(a, !floor);
        a->taskKind = 0;
        break;
    }
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
        if (b->type == HOUSE && b->owner < 0 && home_state(id) != HOME_OWNED && !lodge_joined(id)) {
            log_msg(g.frontier.variant[id] == BUILD_LODGE ? "Dustward beds are reserved for members." : "Private bed.");
            break;
        }
        rest_at(a, id, p[c].x, p[c].y, id + 1,
                b->owner < 0 && town >= 0 && home_state(id) != HOME_OWNED && !lodge_joined(id)
                    ? regional_bed_cost(town) : 0);
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
    case ROOM_HOME_STASH:
    case ROOM_STORAGE:
        ui.storageIndex = id;
        ui.storageSide = 0;
        ui.screen = SCREEN_STORAGE;
        break;
    }
}
int jail_room_open(int room) {
    const Actor *selected = &g.actors[g.selected];
    if (selected->prisonTime > 0 && depth_room(selected) == room)
        return jail_cell_open(selected);
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (a->active && !a->dead && a->prisonTime > 0 && depth_room(a) == room && !jail_cell_open(a))
            return 0;
    }
    return 1;
}
static int room_clear(int room, const InteriorProp *p, int count, float x, float y) {
    if (x < .4f || y < .4f || x > 6.6f || y > 6.6f)
        return 0;
    if (g.buildings[room - 1].type == JAIL) {
        if (fabsf(y-CELL_Y) < .24f &&
            (x < CELL_HINGE_X+.14f || x > CELL_LATCH_X-.14f || !jail_room_open(room)))
            return 0;
        if (fabsf(x-CELL_HINGE_X) < .23f && y < CELL_Y &&
            y > CELL_Y-(CELL_LATCH_X-CELL_HINGE_X)-.12f && jail_room_open(room))
            return 0;
    }
    for (int i = 0; i < count; i++)
        if (p[i].art != 23 && fabsf(x-p[i].x) < p[i].w + .18f && fabsf(y-p[i].y) < p[i].d + .18f)
            return 0;
    return 1;
}
int room_floor_walkable(int room, int floor, float x, float y) {
    if (room <= 0 || room > g.buildingCount) return 0;
    InteriorProp p[ROOM_PROPS];
    int count = interior_floor_props(room - 1,floor,p);
    return room_clear(room,p,count,x,y);
}

int interior_walkable(int room, float x, float y) {
    return room_floor_walkable(room, 0, x, y);
}

void interior_clear_position(Actor *a) {
    int room = depth_room(a);
    if (!room || a->carriedBy >= 0 || actor_in_bed(a))
        return;
    InteriorProp p[ROOM_PROPS];
    int count = interior_floor_props(room - 1,actor_floor(a),p);
    float ox = depth_x(a), oy = depth_y(a);
    if (room_clear(room,p,count,ox,oy))
        return;
    int locked = g.buildings[room - 1].type == JAIL && !jail_room_open(room);
    float best = 100, px = ox, py = oy;
    for (float y = .45f; y <= 6.6f; y += .2f)
        for (float x = .45f; x <= 6.6f; x += .2f) {
            if (locked && (oy < CELL_Y) != (y < CELL_Y)) continue;
            float d = (x-ox)*(x-ox)+(y-oy)*(y-oy);
            if (d < best && room_clear(room,p,count,x,y)) {
                best = d;
                px = x;
                py = y;
            }
        }
    int n = a-g.actors;
    g.depth.actors[n].rx = px;
    g.depth.actors[n].ry = py;
    if (g.interiorBuilding == room && g.interiorActor == n) {
        g.interiorX = px;
        g.interiorY = py;
    }
    a->routePos = a->routeLen = 0;
}
static float room_grid_point(int i) { return .5f + .4f * i; }

void room_route_toward(Actor *a, float tx, float ty, float dt) {
    int room = depth_room(a);
    if (!room || !conscious(a)) return;
    float x = depth_x(a), y = depth_y(a);
    float gap = distance(x,y,tx,ty);
    if (gap < .18f) return;
    InteriorProp p[ROOM_PROPS];
    int count = interior_floor_props(room-1,actor_floor(a),p);
    int clear = 1, steps = (int)ceilf(gap/.25f);
    for (int j = 1; j <= steps; j++)
        if (!room_clear(room,p,count,x+(tx-x)*j/steps,y+(ty-y)*j/steps)) {
            clear = 0;
            break;
        }
    if (clear) {
        a->routeLen = 0;
        depth_move_room(a,tx-x,ty-y,dt);
        return;
    }
    a->routeClock -= dt;
    if (a->routeClock <= 0 || distance(tx,ty,a->routeTargetX,a->routeTargetY) > .6f) {
        enum { SIDE = 16, CELLS = SIDE * SIDE };
        short parent[CELLS], queue[CELLS], path[CELLS];
        int sx = (int)fmaxf(0,fminf(SIDE-1,roundf((x-.5f)/.4f)));
        int sy = (int)fmaxf(0,fminf(SIDE-1,roundf((y-.5f)/.4f)));
        int start = sy*SIDE+sx, end = start, used = 1;
        float best = gap;
        for (int j = 0; j < CELLS; j++) parent[j] = -2;
        parent[start] = -1;
        queue[0] = start;
        for (int j = 0; j < used; j++) {
            int cell = queue[j], cx = cell%SIDE, cy = cell/SIDE;
            float d = distance(room_grid_point(cx),room_grid_point(cy),tx,ty);
            if (d < best) { best = d; end = cell; }
            if (d < .31f) break;
            static const int dx[] = {1,-1,0,0}, dy[] = {0,0,1,-1};
            for (int k = 0; k < 4; k++) {
                int nx = cx+dx[k], ny = cy+dy[k], next = ny*SIDE+nx;
                if (nx < 0 || nx >= SIDE || ny < 0 || ny >= SIDE || parent[next] != -2 ||
                    !room_clear(room,p,count,room_grid_point(nx),room_grid_point(ny)) ||
                    !room_clear(room,p,count,room_grid_point(cx)+dx[k]*.2f,room_grid_point(cy)+dy[k]*.2f))
                    continue;
                parent[next] = cell;
                queue[used++] = next;
            }
        }
        int length = 0;
        for (int c = end; c != start; c = parent[c]) path[length++] = c;
        a->routeLen = length > 24 ? 24 : length;
        a->routePos = 0;
        a->routeTargetX = tx;
        a->routeTargetY = ty;
        a->routeClock = .85f;
        for (int j = 0; j < a->routeLen; j++) {
            int c = path[length-j-1];
            a->routeX[j] = c%SIDE;
            a->routeY[j] = c/SIDE;
        }
    }
    while (a->routePos < a->routeLen) {
        int j = a->routePos;
        float nx = room_grid_point(a->routeX[j]), ny = room_grid_point(a->routeY[j]);
        if (distance(x,y,nx,ny) >= .14f) {
            depth_move_room(a,nx-x,ny-y,dt);
            return;
        }
        a->routePos++;
    }
}
void interior_move(float dx, float dy, float dt) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a))
        return;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < .15f)
        return;
    InteriorProp props[ROOM_PROPS];
    int room = g.interiorBuilding, count = interior_floor_props(room - 1, actor_floor(a), props);
    float wx = (dx + dy) * .7071f, wy = (dy - dx) * .7071f, speed = movement_speed(a) * dt;
    for (int i = 0; i < count; i++) {
        const InteriorProp *bed = &props[i];
        if (bed->action != ROOM_REST || distance(g.interiorX,g.interiorY,bed->x,bed->y) >= .3f)
            continue;
        const float offsets[4][2] = {{bed->w+.25f,0},{-bed->w-.25f,0},{0,bed->d+.25f},{0,-bed->d-.25f}};
        float best = -2;
        for (int side = 0; side < 4; side++) {
            float x = bed->x+offsets[side][0], y = bed->y+offsets[side][1];
            float align = side == 0 ? wx : side == 1 ? -wx : side == 2 ? wy : -wy;
            if (align > best && room_clear(room,props,count,x,y)) {
                g.interiorX = x;
                g.interiorY = y;
                best = align;
            }
        }
        break;
    }
    if (length > 1) {
        wx /= length;
        wy /= length;
    }
    float x = g.interiorX + wx * speed, y = g.interiorY + wy * speed;
    if (room_clear(room,props,count,x,g.interiorY))
        g.interiorX = x;
    if (room_clear(room,props,count,g.interiorX,y))
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

#include "placement.inc"
