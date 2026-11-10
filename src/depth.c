#include "game.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor *);
extern int add_actor(const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern int hostile(const Actor *, const Actor *);
#define D (g.depth)
const char *landmarkNames[LANDMARKS] = {"SALT ROAD WRECK", "DRY CULVERT",  "OLD TOLL CAMP",
                                        "CHAIN YARD",      "ASH SHRINE",   "OLD FOUNDRY",
                                        "WATCH BEACON",    "COPPER STORES"};
const char *orderNames[4] = {"Balanced", "Defensive", "Keep range", "Hold fire"};
static int member(const Actor *a) {
    return (uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + MAX_ACTORS)
               ? (int)(a - g.actors)
               : -1;
}
int depth_named(int actor) {
    for (int i = 0; i < DEPTH_NPCS; i++)
        if (D.npc[i] == actor && actor >= 0)
            return i;
    return -1;
}
void depth_open(int screen) {
    ui.screen = screen == SCREEN_ORDERS ? SCREEN_SQUAD : screen == SCREEN_CARE ? SCREEN_HEALTH : screen;
    ui.selection = ui.scroll = 0;
}
static void clear_spot(float *x, float *y) {
    float ox = *x, oy = *y;
    for (int r = 0; r < 10; r++)
        for (int k = 0; k < 16; k++) {
            float a = k * .392699f, xx = ox + cosf(a) * r, yy = oy + sinf(a) * r;
            if (can_walk(xx, yy) && can_walk(xx + .7f, yy) && can_walk(xx, yy + .7f)) {
                *x = xx;
                *y = yy;
                return;
            }
        }
}
void depth_init(void) {
    if (D.version == 1)
        return;
    memset(&D, 0, sizeof(D));
    D.version = 1;
    D.lastDay = g.day;
    D.retaliation = 360;
    for (int i = 0; i < MAX_ACTORS; i++) {
        D.actors[i].upkeep = 100;
        for (int j = 0; j < 4; j++)
            D.actors[i].queue[j] = JOB_HOLD;
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        D.buildings[i].condition = 100;
        D.buildings[i].fuel = 20;
    }
    for (int i = 0; i < DEPTH_NPCS; i++)
        D.npc[i] = -1;
    for (int i = 0; i < 3; i++)
        D.groupLeader[i] = -1;
    D.groupLeader[0] = g.selected;
    const float coords[LANDMARKS][2] = {{100, 43.4f}, {144, 19.8f}, {162, 47.6f}, {173, 57.6f},
                                        {72, 14.6f},  {258, 289},   {513, 73},    {225, 53}};
    for (int i = 0; i < LANDMARKS; i++) {
        D.siteX[i] = coords[i][0];
        D.siteY[i] = coords[i][1];
        clear_spot(&D.siteX[i], &D.siteY[i]);
        D.siteBuilding[i] = -1;
        if (g.buildingCount < MAX_BUILDINGS) {
            int type = i == SITE_SHRINE    ? BED
                       : i == SITE_FOUNDRY ? FORGE
                       : i == SITE_CAGES   ? WALL
                       : i == SITE_STORES  ? STORE
                                           : CAMPFIRE;
            D.siteBuilding[i] = add_build(D.siteX[i] - 2.5f, D.siteY[i] - 1.8f, type,
                                          i == SITE_CAMP || i == SITE_CAGES ? OUTLAWS : FREE, -1);
        }
    }

}
int depth_validate(const Game *s) {
    const FrontierDepth *d = &s->depth;
    if (!d->version)
        return 1; /* legacy prefix */
    if (d->version != 1 || d->route < 0 || d->route > ROUTE_FREED || d->rescued < 0 || d->rescued > 2 ||
        d->reported < 0 || d->reported > 1 || d->visited < 0 || d->visited >= 256)
        return 0;
    if (!isfinite(d->retaliation) || !isfinite(d->convoyClock))
        return 0;
    for (int i = 0; i < DEPTH_NPCS; i++)
        if (d->npc[i] < -1 || d->npc[i] >= s->actorCount)
            return 0;
    for (int i = 0; i < LANDMARKS; i++)
        if (!isfinite(d->siteX[i]) || !isfinite(d->siteY[i]) || d->siteX[i] < 0 || d->siteY[i] < 0 ||
            d->siteX[i] > WORLD || d->siteY[i] > WORLD || d->siteBuilding[i] < -1 ||
            d->siteBuilding[i] >= s->buildingCount)
            return 0;
    for (int i = 0; i < 3; i++)
        if (d->groupLeader[i] < -1 || d->groupLeader[i] >= s->actorCount || !isfinite(d->groupX[i]) ||
            !isfinite(d->groupY[i]) || d->groupX[i] < 0 || d->groupY[i] < 0 || d->groupX[i] > WORLD ||
            d->groupY[i] > WORLD)
            return 0;
    for (int i = 0; i < MAX_ACTORS; i++) {
        const DepthActor *a = &d->actors[i];
        if (a->order < 0 || a->order > 3 || a->group < 0 || a->group > 2 || a->room < 0 ||
            a->room > s->buildingCount || a->layer < 0 || a->layer >= ITEMS || a->confiscated < 0 ||
            a->confiscated >= ITEMS || a->captive < 0 || a->captive > 3 || !isfinite(a->upkeep) ||
            a->upkeep < 0 || a->upkeep > 100 || !isfinite(a->escape) || !isfinite(a->recognized) ||
            !isfinite(a->trespass) || !isfinite(a->rx) || !isfinite(a->ry))
            return 0;
        for (int j = 0; j < 4; j++)
            if (a->queue[j] < 0 || a->queue[j] > JOB_HAUL)
                return 0;
        for (int j = 0; j < LIMBS; j++)
            if (!isfinite(a->trauma[j]) || a->trauma[j] < 0 || a->trauma[j] > 80)
                return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (!isfinite(d->buildings[i].labor) || d->buildings[i].labor < 0 ||
            !isfinite(d->buildings[i].fuel) || d->buildings[i].fuel < 0 || d->buildings[i].fuel > 1000 ||
            !isfinite(d->buildings[i].condition) || d->buildings[i].condition < 0 ||
            d->buildings[i].condition > 100)
            return 0;
    for (int i = 0; i < PROJECTILES; i++) {
        const Projectile *p = &d->shots[i];
        if (p->active &&
            (p->source < 0 || p->source >= s->actorCount || p->weapon < 0 || p->weapon >= ITEMS ||
             p->room < 0 || p->room > s->buildingCount || !isfinite(p->x) || !isfinite(p->y) ||
             !isfinite(p->vx) || !isfinite(p->vy) || !isfinite(p->left) || p->left < 0 || p->left > 100))
            return 0;
    }
    return 1;
}
int depth_trade_open(int town) {
    ui.roadTrader = 0;
    ui.tradeSmith = 0;
    if (town < 0)
        return 0;
    if (g.dayTime < 6 || g.dayTime >= 22) {
        log_msg("Markets open at 06:00. Find an inn until dawn.");
        return 0;
    }
    return 1;
}
int depth_camp_count(void) {
    int n = 0;
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].owner >= 0)
            n++;
    return n;
}
int depth_camp_build(int row) {
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].owner >= 0 && row-- == 0)
            return i;
    return -1;
}
int depth_build_ready(int b) {
    return b >= 0 && b < g.buildingCount && D.buildings[b].labor <= 0 && D.buildings[b].condition > 0;
}
int depth_build_action(int b) {
    Actor *a = &g.actors[g.selected];
    if (b < 0 || b >= g.buildingCount || !conscious(a) ||
        distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 2.5f)
        return 0;
    if (D.buildings[b].labor > 0) {
        a->taskKind = 4;
        a->taskIndex = b;
        a->action = D.buildings[b].labor;
        depth_open(SCREEN_WORLD);
        log_msg("Construction started. Stay nearby; movement cancels.");
        return 1;
    }
    if (D.buildings[b].condition < 100) {
        if (!inventory_remove(a, WOOD, 1)) {
            log_msg("One timber repairs this structure.");
            return 0;
        }
        D.buildings[b].condition = fminf(100, D.buildings[b].condition + 35);
        gain(a, ENGINEERING, 12);
        log_msg("Structure repaired.");
        return 1;
    }
    return depth_fuel(b);
}
int depth_fuel(int b) {
    Actor *a = &g.actors[g.selected];
    if (b < 0 || b >= g.buildingCount || !depth_build_ready(b) || g.buildings[b].owner < 0 ||
        (g.buildings[b].type != FORGE && g.buildings[b].type != CAMPFIRE))
        return 0;
    if (actor_inside(a) ? actor_inside(a) != b + 1 :
        distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 1.7f &&
        (g.buildings[b].type != FORGE || building_door_distance(b, a->x, a->y) > DOOR_REACH)) {
        log_msg("Stand beside the fire or forge.");
        return 0;
    }
    if (D.buildings[b].fuel > 990) {
        log_msg("Fuel store is full.");
        return 0;
    }
    int fuel = 0;
    for (int i = 0; i < PACK; i++)
        if (!a->stolen[i] && (a->pack[i].item == COAL || a->pack[i].item == WOOD)) {
            fuel = a->pack[i].item;
            inventory_remove_slot(a, i, 1);
            break;
        }
    if (!fuel) {
        log_msg("Bring clean coal or timber for fuel.");
        return 0;
    }
    D.buildings[b].fuel += fuel == COAL ? 10 : 4;
    log_msg("Fuel loaded: %.0f production charges.", D.buildings[b].fuel);
    return 1;
}
int depth_use_fuel(int b, float amount) {
    if (!depth_build_ready(b)) {
        log_msg("Finish construction or repair the station.");
        return 0;
    }
    if (g.buildings[b].owner < 0)
        return 1;
    if (D.buildings[b].fuel < amount) {
        log_msg("Station needs fuel. Camp ledger: add coal or timber.");
        return 0;
    }
    D.buildings[b].fuel -= amount;
    D.buildings[b].condition = fmaxf(1, D.buildings[b].condition - .1f);
    return 1;
}
int depth_organic(const Actor *a, int limb) {
    return a->race != 2 && !(a->prostheticMask & (1 << limb));
}
int depth_can_heal(const Actor *a, int limb) {
    return depth_organic(a, limb) && (!(a->lostMask & (1 << limb)) || (a->prostheticMask & (1 << limb)));
}
void depth_wound(Actor *a, int limb, float damage) {
    int n = member(a);
    if (n < 0)
        return;
    a->treatedMask &= ~(1 << limb);
    a->woundFlags &= ~(1u << (WOUND_REPAIR_SHIFT + limb));
    float cap = fminf(80, limb_max_health(a, limb) * .74f);
    D.actors[n].trauma[limb] = fminf(cap, D.actors[n].trauma[limb] + damage * .18f);
    D.actors[n].upkeep = fmaxf(20, D.actors[n].upkeep - damage * .025f);
}
int depth_needs_repair(const Actor *a) {
    int n = member(a);
    if (n < 0 || a->dead)
        return 0;
    for (int j = 0; j < LIMBS; j++)
        if (!limb_missing(a, j) && !depth_organic(a, j) &&
            !(a->woundFlags & (1u << (WOUND_REPAIR_SHIFT + j))) &&
            (a->health[j] < limb_max_health(a, j) - 1 || D.actors[n].trauma[j] > 0))
            return 1;
    return 0;
}
int depth_repair(int mi, int pi) { return begin_treatment(mi, pi, TREAT_REPAIR, -1); }
int depth_layer(int slot) {
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &D.actors[g.selected];
    if (slot < 0 || slot >= PACK)
        return 0;
    int item = a->pack[slot].item;
    if (!item || itemDefs[item].slot != 1 || itemDefs[item].weight > 5) {
        log_msg("Underlayer needs light body clothing (weight 5 or less).");
        return 0;
    }
    Actor backup = *a;
    int owner = a->stolen[slot];
    inventory_remove_slot(a, slot, 1);
    if (z->layer && !inventory_add_owned(a, z->layer, 1, z->layerOwner)) {
        *a = backup;
        log_msg("Pack full. Layer swap cancelled.");
        return 0;
    }
    z->layer = item;
    z->layerOwner = owner;
    log_msg("Underlayer fitted. Inspect coverage in Armor Layers.");
    return 1;
}
int depth_coverage(int item, int limb) {
    if (item <= 0 || item >= ITEMS)
        return 0;
    int slot = itemDefs[item].slot;
    if (slot == 2)
        return limb == HEAD ? 85 : 0;
    if (slot == 1)
        return limb == CHEST ? 95 : limb == LARM || limb == RARM ? 65 : 0;
    if (slot == 4)
        return limb >= LLEG ? 90 : 0;
    if (slot == 3)
        return limb >= LLEG ? 35 : 0;
    return 0;
}
float depth_armor(const Actor *a, int limb, int weapon, int roll) {
    int n = member(a), family = weapon_family_index(weapon);
    int blunt = !weapon || weapon == STAFF || (family >= 14 && family <= 16) || family == 33;
    float armor = 0, condition = n >= 0 ? .65f + D.actors[n].upkeep * .0035f : 1;
    for (int j = 1; j < GEARS; j++)
        if (roll < depth_coverage(a->gear[j], limb))
            armor += itemDefs[a->gear[j]].armor * (blunt ? .72f : 1);
    if (n >= 0 && roll < depth_coverage(D.actors[n].layer, limb))
        armor += itemDefs[D.actors[n].layer].armor * (blunt ? .8f : .55f);
    return armor * condition;
}
int depth_capture_action(int n) {
    if (n < 0 || n >= g.actorCount)
        return 0;
    Actor *a = &g.actors[g.selected], *b = &g.actors[n];
    DepthActor *z = &D.actors[n];
    if (!conscious(a) || b->dead || depth_distance(a, b) > TALK_REACH)
        return 0;
    if (n == g.selected && slave_in_camp(b)) return slave_pick_start(n);
    return (z->captive || b->prisonTime > 0) && society_rescue_start(n);
}
static void captivity(float dt) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active || a->dead || fatal_injury(a))
            continue;
        if (g.field.laborCamp[i] || g.field.intake[i]) continue;
        if (z->captive == 2) { /* physical capture, no teleport */
            if (a->carriedBy < 0) {
                z->captive = 0;
                continue;
            }
            Actor *captor = &g.actors[a->carriedBy];
            route_toward(captor, D.siteX[3], D.siteY[3], dt);
            if (distance(captor->x, captor->y, D.siteX[3], D.siteY[3]) < 1.6f) {
                captor->carrying = -1;
                a->carriedBy = -1;
                z->captive = 1;
                z->confiscated = a->gear[0];
                z->confiscatedOwner = a->gearStolen[0];
                a->gear[0] = 0;
                a->gearStolen[0] = 0;
                log_msg("%s is shackled at the chain yard.", a->name);
            }
        } else if (z->captive == 1) {
            a->attackPhase = 0;
            a->target = -1;
            a->taskKind = 0;
            if (a->bleed > .01f || (a->woundFlags & WOUND_CUTS))
                dress_wounds(a);
            a->hunger = fmaxf(25, a->hunger);
            if (a->race == 2 && a->unconscious > 0)
                injury_repair(a, z);
        } else if (!g.field.version && a->squad && a->unconscious > 0 && a->carriedBy < 0 && !actor_inside(a) &&
                   distance(a->x, a->y, D.siteX[3], D.siteY[3]) < 24) {
            for (int j = 0; j < g.actorCount; j++) {
                Actor *b = &g.actors[j];
                if (j != i && !b->squad && b->faction == HOSTILE && conscious(b) && b->carrying < 0 &&
                    depth_distance(a, b) < TALK_REACH) {
                    a->carriedBy = j;
                    b->carrying = i;
                    z->captive = 2;
                    a->bleed = 0;
                    log_msg("%s is being carried to the chain yard. Rescue them!", a->name);
                    break;
                }
            }
        }
    }
}
static int stock_has(int type, int item, const Actor *a) {
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type != type || b->owner < 0 || !depth_build_ready(i) ||
            distance(a->x, a->y, b->x, b->y) > 18)
            continue;
        if (!item)
            return 1;
        for (int j = 0; j < 16; j++)
            if (b->storage[j].item == item && b->storage[j].qty > 0)
                return b->storage[j].qty;
    }
    return 0;
}
static int ready_job(const Actor *a, int job) {
    if (job == JOB_HOLD || job == JOB_FOLLOW)
        return 1;
    if (job == JOB_MEDIC) {
        if (!inventory_count(a, MEDKIT))
            return 0;
        for (int j = 0; j < g.actorCount; j++)
            if (g.actors[j].squad && needs_aid(&g.actors[j]) && wound_needs_dressing(&g.actors[j]) &&
                depth_distance(a, &g.actors[j]) < 18)
                return 1;
        return 0;
    }
    if (job == JOB_MINE) {
        for (int j = 0; j < g.nodeCount; j++)
            if (g.nodes[j].remaining && g.nodes[j].type <= NODE_COPPER &&
                distance(a->x, a->y, g.nodes[j].x, g.nodes[j].y) < 12) {
                Actor test = *a;
                if (inventory_add(&test, g.nodes[j].type == NODE_ORE ? ORE : COPPER, 1))
                    return 1;
            }
        return 0;
    }
    if (job == JOB_FARM)
        return stock_has(FARM, 0, a);
    if (job == JOB_HAUL) {
        if (!stock_has(STORE, 0, a))
            return 0;
        return inventory_count(a, ORE) || inventory_count(a, COPPER) || inventory_count(a, WOOD) ||
               inventory_count(a, GRAIN) || inventory_count(a, IRONBAR);
    }
    int station = job == JOB_SMELT ? FORGE : CAMPFIRE;
    if (!stock_has(station, 0, a))
        return 0;
    if (stock_has(STORE, 0, a) &&
        inventory_count(a, job == JOB_SMELT ? IRONBAR : RATION) > (job == JOB_SMELT ? 0 : 2))
        return 1;
    int input = job == JOB_SMELT ? ORE : GRAIN;
    if (inventory_count(a, input) + stock_has(STORE, input, a) < 3 &&
        !(job == JOB_COOK && inventory_count(a, MEAT) + stock_has(STORE, MEAT, a) >= 2))
        return 0;
    return job == JOB_SMELT || inventory_count(a, WATER) + stock_has(STORE, WATER, a) > 0;
}
int assigned_job(const Actor *a) {
    int n = member(a);
    if (n < 0 || !D.actors[n].queued)
        return a->job;
    return ui.priorityJob[n] ? ui.priorityJob[n] - 1 : JOB_HOLD;
}
void depth_queue(Actor *a) {
    int n = member(a);
    if (n < 0 || !automatic_work(a) || !D.actors[n].queued || D.actors[n].paused || a->taskKind || a->target >= 0)
        return;
    ui.priorityJob[n] = JOB_HOLD + 1;
    for (int j = 0; j < 4; j++)
        if (ready_job(a, D.actors[n].queue[j])) {
            ui.priorityJob[n] = D.actors[n].queue[j] + 1;
            break;
        }
}
int depth_group_job(Actor *a, float dt) {
    int n = member(a);
    if (n < 0 || !a->squad)
        return 0;
    DepthActor *z = &D.actors[n];
    if (z->paused)
        return 1;
    if (assigned_job(a) != JOB_FOLLOW)
        return 0;
    Actor *lead = &g.actors[g.selected];
    if (a == lead || !conscious(lead))
        return 1;
    int room = depth_room(a), leadRoom = depth_room(lead);
    if (room && room == leadRoom) {
        if (!inn_follow_stairs(a, actor_floor(lead), dt) && depth_distance(a, lead) > 2.2f)
            room_route_toward(a, depth_x(lead), depth_y(lead), dt);
    } else if (room && inn_has_upstairs(room - 1)) {
        if (!inn_follow_stairs(a, 0, dt)) {
            if (distance(depth_x(a), depth_y(a), 3.5f, 6.35f) < .6f)
                actor_leave_room(n);
            else
                room_route_toward(a, 3.5f, 6.35f, dt);
        }
    } else if (!room && leadRoom) {
        float x, y;
        building_door(leadRoom - 1, &x, &y);
        if (distance(a->x, a->y, x, y) > 1.7f)
            route_toward(a, x, y, dt);
        else {
            z->room = leadRoom;
            z->rx = 2.6f + (n % 3) * .65f;
            z->ry = 5.8f;
        }
    } else if (!room && distance(a->x, a->y, lead->x, lead->y) > 2.4f + z->group * .35f) {
        route_toward(a, lead->x + (n % 3 - 1) * .75f, lead->y + .8f + z->group * .45f, dt);
    }
    return 1;
}
void depth_tick(float dt) {
    if (!D.version)
        return;
    captivity(dt);
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active || a->dead)
            continue;
        z->recognized = fmaxf(0, z->recognized - dt);
        if (a->race == 2) {
            a->bleed = 0;
            a->blood = 100;
            a->hunger = 100;
        }
        if (a->squad)
            depth_queue(a);
        if (a->taskKind == 4) {
            int b = a->taskIndex;
            if (!conscious(a) || b < 0 || b >= g.buildingCount ||
                distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 2.5f) {
                a->taskKind = 0;
                continue;
            }
            float power = 1 + level(a, ENGINEERING) * .025f;
            D.buildings[b].labor = fmaxf(0, D.buildings[b].labor - dt * power);
            a->action = D.buildings[b].labor + 1;
            gain(a, ENGINEERING, dt * 3);
            if (!D.buildings[b].labor) {
                a->taskKind = 0;
                log_msg("Construction complete: %s.", context_name());
            }
        }
    }
    depth_projectiles(dt);
}
void depth_after_tick(float dt) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active)
            continue;
        for (int j = 0; j < LIMBS; j++)
            a->health[j] = fminf(a->health[j], limb_max_health(a,j) - z->trauma[j]);
        int n = depth_named(i);
        if (z->captive == 1 && n != NPC_RELL && !g.war.prisonTown[i] && !g.field.laborCamp[i]) {
            a->x = D.siteX[3] + (n == NPC_TAMSIN ? 1.2f : 0);
            a->y = D.siteY[3];
        }
    }
    depth_room_tick(dt);
}
int depth_input(float dt, Input in) {
    int p = in.pressed;
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &D.actors[g.selected];
    if (ui.screen >= SCREEN_ORDERS && ui.screen <= SCREEN_LAYERS && ui.screen != SCREEN_ENCOUNTER) {
        if (p & KEY_SELECT) {
            open_help(ui.screen == SCREEN_ORDERS                                    ? 21
                      : ui.screen == SCREEN_CAMP                                      ? 22
                      : ui.screen == SCREEN_CARE                                      ? 23
                                                                                      : 24);
            return 1;
        }
        if (p & (KEY_CIRCLE | KEY_START)) {
            depth_open(SCREEN_WORLD);
            return 1;
        }
        int count = ui.screen == SCREEN_ORDERS   ? 9
                    : ui.screen == SCREEN_CAMP     ? depth_camp_count()
                    : ui.screen == SCREEN_LAYERS   ? PACK
                    : ui.screen == SCREEN_CARE     ? LIMBS
                                                   : 2;
        if (count < 1)
            count = 1;
        if (p & KEY_DOWN)
            ui.selection = (ui.selection + 1) % count;
        if (p & KEY_UP)
            ui.selection = (ui.selection + count - 1) % count;
        if (ui.screen == SCREEN_ORDERS) {
            int delta = p & KEY_LEFT ? -1 : 1;
            if (p & (KEY_CROSS | KEY_LEFT | KEY_RIGHT))
                switch (ui.selection) {
                case 0:
                    z->order = (z->order + delta + 4) % 4;
                    a->autoattack = z->order != ORDER_HOLD_FIRE;
                    break;
                case 1:
                    z->group = (z->group + delta + 3) % 3;
                    break;
                case 2:
                    D.groupLeader[z->group] = g.selected;
                    log_msg("%s leads group %d.", a->name, z->group + 1);
                    break;
                case 3:
                    z->paused = !z->paused;
                    a->taskKind = 0;
                    log_msg(z->paused ? "Jobs paused. Manual movement is safe." : "Jobs resumed.");
                    break;
                case 8:
                    z->queued = !z->queued;
                    break;
                default:
                    z->queue[ui.selection - 4] = (z->queue[ui.selection - 4] + delta + 8) % 8;
                    break;
                }
        } else if (ui.screen == SCREEN_CAMP) {
            if (p & KEY_CROSS)
                depth_build_action(depth_camp_build(ui.selection));
            if (p & KEY_SQUARE)
                depth_fuel(depth_camp_build(ui.selection));
        } else if (ui.screen == SCREEN_CARE) {
            if (p & KEY_CROSS) {
                if (z->captive)
                    depth_capture_action(g.selected);
                else
                    heal_actor(g.selected, g.selected);
            }
            if (p & KEY_SQUARE)
                depth_repair(g.selected, g.selected);
            if (p & KEY_TRIANGLE) {
                if (z->upkeep >= 100)
                    log_msg("Equipment is already maintained.");
                else if (inventory_remove(a, CLOTH, 1)) {
                    z->upkeep = fminf(100, z->upkeep + 40);
                    log_msg("Clothing and armor maintained.");
                } else
                    log_msg("Maintenance needs one cloth.");
            }
        } else if (ui.screen == SCREEN_LAYERS) {
            if (p & KEY_CROSS)
                depth_layer(ui.selection);
            if ((p & KEY_SQUARE) && z->layer) {
                if (inventory_add_owned(a, z->layer, 1, z->layerOwner)) {
                    z->layer = 0;
                    z->layerOwner = 0;
                } else
                    log_msg("Pack full. Cannot remove layer.");
            }
        }
        return 1;
    }
    if (ui.screen == SCREEN_WORLD && z->captive) {
        if ((p & KEY_CIRCLE) && slave_in_camp(a) && a->taskKind == TASK_CELL_LOCK) {
            a->taskKind = 0;
            return 1;
        }
        if (p & KEY_CROSS) {
            if (slave_in_camp(a)) slave_pick_start(g.selected);
            else depth_open(SCREEN_CARE);
            return 1;
        }
        if (!(p & (KEY_L | KEY_R | KEY_START | KEY_SELECT | KEY_TRIANGLE))) {
            if (!ui.paused)
                simulate(dt * ui.speed);
            return 1;
        }
    }
    if (ui.screen == SCREEN_WORLD && fabsf(in.ax) + fabsf(in.ay) > .15f) {
        if (z->queued && !z->paused) {
            z->paused = 1;
            log_msg("Job queue paused by movement. Resume in Orders.");
        }
        D.groupTravel[z->group] = 0;
    }
    return 0;
}
