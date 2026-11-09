#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int add_actor(const char *, int, int, float, float, int);
extern void init_actor(int, const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern void add_node(float, float, int, int, int, int);
extern int conscious(const Actor *);
extern int hostile(const Actor *,const Actor *);
extern int nearby_station(int, float);
extern void restock(void);
static unsigned wrand(void) {
    unsigned x = g.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g.rng = x ? x : 17;
}
static unsigned whash(int x, int y) {
    unsigned v = (unsigned)x * 374761393u + (unsigned)y * 668265263u + 73819;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}
static float limit(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static const float riverFords[]={330,620,1000,1260};
static const float brookFords[]={305,530,695};
static float river_x(float y){return 4*(194+12*sinf((y+81)*.00625f))-78;}
static float brook_y(float x){return 4*(139+9*sinf((x+78)*.00625f))-81;}
void army_ford_waypoint(float x,float y,float *tx,float *ty){
    float from=x-river_x(y),to=*tx-river_x(*ty);
    if(from*to<0 || (fabsf(from)<12 && fabsf(to)>12)){
        float best=1e9f,fx=*tx,fy=*ty;
        for(unsigned i=0;i<sizeof(riverFords)/sizeof(riverFords[0]);i++){
            float wy=riverFords[i],wx=river_x(wy)+(to>0?15:-15);
            float cost=distance(x,y,wx,wy)+distance(wx,wy,*tx,*ty);
            if(cost<best){best=cost;fx=wx;fy=wy;}
        }
        *tx=fx;*ty=fy;
    }
    if(x>750||*tx>752)return;
    from=y-brook_y(x);to=*ty-brook_y(*tx);
    if(from*to>=0 && (fabsf(from)>=9 || fabsf(to)<=9))return;
    float best=1e9f,fx=*tx,fy=*ty;
    for(unsigned i=0;i<sizeof(brookFords)/sizeof(brookFords[0]);i++){
        float wx=brookFords[i],wy=brook_y(wx)+(to>0?12:-12);
        float cost=distance(x,y,wx,wy)+distance(wx,wy,*tx,*ty);
        if(cost<best){best=cost;fx=wx;fy=wy;}
    }
    *tx=fx;*ty=fy;
}
int biome(float x, float y) {
    if (world_is_expanded()) {
        x = (x + 78) / 4;
        y = (y + 81) / 4;
    }
    if (x < 128 && y < 128) {
        if (x > 74 && y > 62)
            return 3;
        if (y > 69)
            return 2;
        if (x > 58 && y < 49)
            return 1;
        if (distance(x, y, 61, 62) < 16)
            return 4;
        return 0;
    }
    if (y > 285 && x > 240)
        return 6;
    if (y > 280 && x < 100)
        return 7;
    if (x > 280 && y < 175)
        return y < 80 ? 4 : 8;
    if (y > 180 && x < 110)
        return 3;
    if (y > 250 && x < 230)
        return 2;
    if (x > 205)
        return 5;
    if (y > 110 && y < 215)
        return 8;
    return 1;
}
int terrain_road(float x, float y) {
    if(world_is_expanded()){
        if(x>625&&x<770)for(unsigned i=0;i<sizeof(riverFords)/sizeof(riverFords[0]);i++)
            if(fabsf(y-riverFords[i])<1.65f&&fabsf(x-river_x(y))<18)return 1;
        if(y>430&&y<535)for(unsigned i=0;i<sizeof(brookFords)/sizeof(brookFords[0]);i++)
            if(fabsf(x-brookFords[i])<1.65f&&fabsf(y-brook_y(x))<15)return 1;
    }
    int town=town_near(x,y,16);
    if(town>0){
        float dx=x-towns[town].x,dy=y-towns[town].y;int style=civil_style(town);
        if(town==OLD_TOWNS&&fabsf(dx)<10&&(fabsf(dy-2)<1.1f||fabsf(dy+4)<.75f))return 1;
        if(style==1 && ((fabsf(dx)<2.2f && fabsf(dy)<14) || (fabsf(dy-5)<1.2f && fabsf(dx)<10)))return 1;
        if(style==2 && fabsf(dx)<6 && fabsf(dy)<6 && (fabsf(dx)>3.8f || fabsf(dy)>3.8f))return 1;
        if(style==4 && fabsf(dx)<10 && fabsf(dy)<10 && (fabsf(dx-1)<1.1f || fabsf(dy-2)<1.1f))return 1;
        if(style==3 && fabsf(dx)<9 && fabsf(dy-dx*.45f)<.7f)return 1;
    }
    /* A sparse road graph: each town joins the nearest earlier settlement. */
    static int neighbors[TOWNS], ready;
    if (!ready) {
        for (int t = 1; t < TOWNS; t++) {
            int near = 0;
            float best = 1e10f;
            for (int j = 0; j < t; j++) {
                float dx = towns[t].x - towns[j].x, dy = towns[t].y - towns[j].y, d = dx * dx + dy * dy;
                if (d < best) {
                    best = d;
                    near = j;
                }
            }
            neighbors[t] = near;
        }
        ready = 1;
    }
    for (int t = 1; t < TOWNS; t++) {
        int near = neighbors[t];
        float ax = towns[t].x, ay = towns[t].y, dx = towns[near].x - ax, dy = towns[near].y - ay;
        float u = ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy);
        if (u >= 0 && u <= 1) {
            float xx = x - ax - u * dx, yy = y - ay - u * dy;
            if (xx * xx + yy * yy < 1.7f)
                return 1;
        }
    }
    return 0;
}
int terrain_height(float x, float y) {
    if (town_near(x, y, 12) >= 0)
        return 0;
    int b = biome(x, y);
    if (b == 0 || b == 2 || b == 3)
        return 0;
    int ix = (int)x / 24, iy = (int)y / 24;
    float u = (x - ix * 24) / 24, v = (y - iy * 24) / 24;
    float a = (whash(ix, iy) % 13) * (1 - u) + (whash(ix + 1, iy) % 13) * u;
    float c = (whash(ix, iy + 1) % 13) * (1 - u) + (whash(ix + 1, iy + 1) % 13) * u;
    return (int)((a * (1 - v) + c * v) * (b == 5 || b == 8 ? 1.6f : .7f));
}
int terrain_water(float x, float y) {
    float wx = x, wy = y;
    if (world_is_expanded()) {
        x = (x + 78) / 4;
        y = (y + 81) / 4;
    }
    if (x < 128 && y < 128)
        return 0;
    float r = 194 + 12 * sinf(y * .025f), s = 139 + 9 * sinf(x * .025f);
    int wet = fabsf(x - r) < 2.2f || (x < 210 && fabsf(y - s) < 1.6f) || distance(x, y, 107, 242) < 12;
    if (!wet)
        return 0;
    if (town_near(wx, wy, 12) >= 0 || terrain_road(wx, wy) || terrain_road(wx + 1, wy) ||
        terrain_road(wx - 1, wy))
        return 0;
    return 1;
}
void route_toward(Actor *a, float x, float y, float dt) {
    if (!conscious(a) || a->prisonTime > 0)
        return;
    float dx = x - a->x, dy = y - a->y, d = sqrtf(dx * dx + dy * dy);
    if (d < .3f)
        return;
    int clear = 1;
    for (int j = 1; j <= 4; j++) {
        float k = fminf(d, 3) * j * .25f / d;
        if (!can_walk(a->x + dx * k, a->y + dy * k)) {
            clear = 0;
            break;
        }
    }
    if (clear) {
        a->routeLen = 0;
        move_actor(a, dx, dy, dt);
        return;
    }
    a->routeClock -= dt;
    if (a->routePos >= a->routeLen || a->routeClock <= 0 ||
        distance(x, y, a->routeTargetX, a->routeTargetY) > 2) {
        enum { N = 33, C = N * N };
        static short parent[C], queue[C], back[C];
        int ox = (int)a->x - N / 2, oy = (int)a->y - N / 2, st = N / 2 * N + N / 2;
        for (int i = 0; i < C; i++)
            parent[i] = -2;
        int head = 0, tail = 0, goal = st;
        float best = 1e10f;
        queue[tail++] = st;
        parent[st] = -1;
        while (head < tail) {
            int n = queue[head++], xx = n % N, yy = n / N;
            float ex = ox + xx + .5f - x, ey = oy + yy + .5f - y, score = ex * ex + ey * ey;
            if (score < best) {
                best = score;
                goal = n;
            }
            if (score < .7f)
                break;
            const int dd[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (int j = 0; j < 4; j++) {
                int nx = xx + dd[j][0], ny = yy + dd[j][1], z = ny * N + nx;
                if (nx < 0 || ny < 0 || nx >= N || ny >= N || parent[z] != -2)
                    continue;
                if (!can_walk(ox + nx + .5f, oy + ny + .5f))
                    continue;
                parent[z] = n;
                queue[tail++] = z;
            }
        }
        int count = 0;
        for (int z = goal; z != st && z >= 0 && count < C; z = parent[z])
            back[count++] = z;
        a->routeLen = count > 24 ? 24 : count;
        a->routePos = 0;
        for (int i = 0; i < a->routeLen; i++) {
            int z = back[count - 1 - i];
            a->routeX[i] = ox + z % N;
            a->routeY[i] = oy + z / N;
        }
        a->routeClock = 7;
        a->routeTargetX = x;
        a->routeTargetY = y;
    }
    if (a->routePos < a->routeLen) {
        float xx = a->routeX[a->routePos] + .5f, yy = a->routeY[a->routePos] + .5f;
        if (distance(a->x, a->y, xx, yy) < .35f)
            a->routePos++;
        else
            move_actor(a, xx - a->x, yy - a->y, dt);
    }
}
static void outfit(Actor *a, int n) {
    if (a->kind == BEAST)
        return;
    int f = a->faction, m = (n + f) % MATERIALS, q = a->kind == GUARD ? 2 : n % 2;
    a->gear[0] = gear_id(a->kind == TRADER   ? 3
                         : a->kind == RAIDER ? (n % 24)
                         : f == SHOGUN       ? (n % 7)
                         : f == MONKS        ? 16
                                             : n % 32,
                         m, q);
    a->gear[1] = gear_id(32 + (n + f * 3) % 16, m, q);
    a->gear[2] = gear_id(48 + (n + f) % 12, m, q);
    a->gear[3] = gear_id(60 + n % 8, m, q);
    a->gear[4] = gear_id(68 + n % 8, m, q);
    if (n % 3 == 0)
        a->gear[5] = gear_id(76 + n % 8, m, q);
    if (weapon_ranged(a->gear[0]))
        inventory_add(a, gear_family(a->gear[0]) >= 29 ? CARTRIDGE : BOLTS, 24);
}
void init_expansion(void) {
    for (int t = 0; t < TOWNS; t++) {
        g.townOwner[t] = towns[t].faction;
        g.prosperity[t] = 60;
    }
    g.weatherTimer = 160;
    g.eventTimer = 220;
    g.difficulty = 1;
    snprintf(g.worldNews, sizeof(g.worldNews), "The old road is open. Work waits in the frontier towns.");
    for (int t = 0; t < OLD_TOWNS; t++) {
        float x = towns[t].x, y = towns[t].y;
        int f = towns[t].faction;
        if (t >= 5) {
            add_build(x - 4, y - 3, HOUSE, f, -1);
            add_build(x, y - 5, TOWER, f, -1);
            add_build(x + 4, y - 3, SHOP, f, -1);
            add_build(x - 4, y + 3, BAR, f, -1);
            add_build(x + 4, y + 3, HOUSE, f, -1);
            add_build(x + 7, y, WINDMILL, f, -1);
            add_build(x, y + 1, WELL, f, -1);
            add_actor("Gate warden", GUARD, f, x + 2, y - 1, 14 + t);
            add_actor("Road marshal", GUARD, f, x - 1, y + 4, 13 + t);
            add_actor(f == HOSTILE ? "Vulture quarter" : "Market keeper", TRADER, f, x + 2, y - 1.5f, 8 + t);
            static const char *recruits[] = {"KASUMI", "FLINT",  "HANA", "COLE", "REN", "SABLE",
                                             "JIN",    "MORROW", "YUKI", "ADA",  "ROOK"};
            add_actor(recruits[t - 5], RECRUIT, f, x - 2, y + 3.5f, 5 + t / 2);
            add_actor("Inn keeper", SETTLER, f, x - 1, y + 6, 4);
            add_actor("Caravan scout", CARAVAN, f, x + 5, y + 6, 12);
        }
        if (f != HOSTILE) {
            add_build(x - 7, y + 5, BED, f, -1);
            add_build(x + 8, y + 5, FORGE, f, -1);
            add_build(x + 8, y + 9, BENCH, f, -1);
            if (t < 5)
                add_actor("Inn keeper", SETTLER, f, x - 1, y + 6, 5);
        }
        add_node(x + 5, y + 6, NODE_LOCKED, f, gear_id(32 + t % 16, t % 6, (t / 4) % 4), 1);
    }
    for (int n = 1; n < g.actorCount; n++)
        if (!g.actors[n].squad)
            outfit(&g.actors[n], n);
    for (int n = 1; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (a->squad || a->faction == HOSTILE)
            continue;
        int t = town_near(a->x, a->y, 14);
        if (t < 0)
            continue;
        if (!strcmp(a->name, "Gate guard") || !strcmp(a->name, "Gate warden")) {
            a->kind = SETTLER;
            a->voice = ROLE_MEDIC;
            snprintf(a->name, 16, "Town medic");
        } else if (a->kind == CARAVAN) {
            a->kind = SETTLER;
            a->voice = ROLE_SMITH;
            snprintf(a->name, 16, "Frontier smith");
            a->x = a->homeX = towns[t].x + 7;
            a->y = a->homeY = towns[t].y + 7;
        } else if (!strcmp(a->name, "Wayfarer")) {
            a->voice = ROLE_BARBER;
            snprintf(a->name, 16, "Trail barber");
        }
    }

    for (int i = 0; i < 370; i++) {
        float x = 24 + wrand() % 350, y = 25 + wrand() % 350;
        if (town_near(x, y, 11) >= 0 || !can_walk(x, y))
            continue;
        int type = i % 11 == 0 ? NODE_RUIN : i % 3;
        add_node(x, y, type, 0,
                 type == NODE_RUIN ? (i % 2 ? RELIC : gear_id(i % FAMILIES, (i / 7) % 6, i % 4)) : 0,
                 type == NODE_RUIN ? 1 : 0);
    }
    for (int i = 0; i < 44 && g.actorCount < MAX_ACTORS - 8; i++) {
        float x = 24 + wrand() % 348, y = 25 + wrand() % 348;
        if (i < 30) {
            int t = 1 + i / 2, near = 0;
            float best = 1e9f;
            for (int j = 0; j < t; j++) {
                float d = distance(towns[t].x, towns[t].y, towns[j].x, towns[j].y);
                if (d < best) {
                    best = d;
                    near = j;
                }
            }
            float u = i % 2 ? .7f : .35f;
            x = towns[t].x * (1 - u) + towns[near].x * u;
            y = towns[t].y * (1 - u) + towns[near].y * u + (i % 2 ? 1.0f : -1.0f);
        }
        if (town_near(x, y, 15) >= 0 || !can_walk(x, y))
            continue;
        int b = biome(x, y), beast = i % 4 == 0;
        static const char *foes[] = {"Dust jackal",   "Salt raider", "Red cutthroat", "Ash revenant",
                                     "Iron deserter", "Bone hound",  "Pine stalker",  "Dune prowler"};
        int n = add_actor(foes[(i + b) % 8], beast ? BEAST : RAIDER, beast ? WILDLIFE : HOSTILE, x, y,
                          6 + b * 2 + i % 12);
        if (n >= 0) {
            outfit(&g.actors[n], i);
            if (!beast && i % 9 == 0) {
                g.actors[n].bountyReward = 250 + i * 10;
                inventory_add(&g.actors[n], RELIC, 1);
            }
        }
    }
    for (int n = 0; n < g.actorCount; n++)
        if (g.actors[n].squad) {
            g.actors[n].gear[4] = gear_id(68, 0, 0);
            g.actors[n].gear[5] = gear_id(76, 0, 0);
        }
    restock();
}
int item_price(int town, int it, int selling) {
    if (it <= 0 || it >= ITEMS || town < 0 || town >= TOWNS)
        return 0;
    float mod = 1 + regional_price(town,it) + (town==1&&g.depth.route==ROUTE_TRUCE?.12f:0) + (60 - g.prosperity[town]) * .004f + limit(g.rep[g.townOwner[town]]*.001f,-.15f,.1f)*(selling?.5f:-1);
    mod = limit(mod, .65f, 1.6f);
    return (int)fmaxf(1, itemDefs[it].price * (selling ? .75f : 1) * mod * market_factor(town,it) * politics_price(town,it));
}
int shop_item(int index) {
    if (ui.tradeTown < 0 || ui.tradeTown >= TOWNS)
        return NONE;
    /* Regional designs lead the weapon tab; shared starter goods go last. */
    for (int step = 1; step < ITEMS; step++) {
        int i = ui.shopFilter == 2 ? BASE_ITEMS + step - 1 : step;
        if (i >= ITEMS)
            i = i - ITEMS + 1;
        if (g.stock[ui.tradeTown][i] > 0) {
            int slot = itemDefs[i].slot;
            if (ui.shopFilter == 1 && slot >= 0)
                continue;
            if (ui.shopFilter == 2 && slot != 0)
                continue;
            if (ui.shopFilter == 3 && (slot < 1 || slot == 5))
                continue;
            if (ui.shopFilter == 5 && limb_item_grade(i)<0 && i!=PARTS) continue;
            if (ui.shopFilter == 4 && slot != 5)
                continue;
            if (index-- == 0)
                return i;
        }
    }
    return NONE;
}
int shop_count(void) {
    int n = 0;
    if (ui.tradeTown < 0 || ui.tradeTown >= TOWNS)
        return 0;
    for (int i = 1; i < ITEMS; i++)
        if (g.stock[ui.tradeTown][i] > 0) {
            int s = itemDefs[i].slot;
            if (ui.shopFilter == 1 && s >= 0)
                continue;
            if (ui.shopFilter == 2 && s != 0)
                continue;
            if (ui.shopFilter == 3 && (s < 1 || s == 5))
                continue;
            if (ui.shopFilter == 5 && limb_item_grade(i)<0 && i!=PARTS) continue;
            if (ui.shopFilter == 4 && s != 5)
                continue;
            n++;
        }
    return n;
}
int nearby_workstation(int type) { return nearby_station(type, 1.7f); }
int forge_selection_item(void){
    int f=ui.forgeFamily;
    return f==88?SCIMITAR_ITEMS+ui.forgeMaterial*4+ui.forgeQuality:
           f==89?RAPIER_ITEMS+ui.forgeMaterial*4+ui.forgeQuality:
           f>=90&&f<94?CIV_ARMOR_ITEMS+(f-90)*24+ui.forgeMaterial*4+ui.forgeQuality:
           gear_id(f,ui.forgeMaterial,ui.forgeQuality);
}
const char *forge_selection_name(void){
    static const char *extra[]={"Scimitar","Rapier","Court brigandine","Reed carapace","Basalt plate","Censor vestments"};
    return ui.forgeFamily>=88&&ui.forgeFamily<94?extra[ui.forgeFamily-88]:familyNames[ui.forgeFamily];
}
int forge_requirements(int it,CraftRequirements*out){
    if(it<BASE_ITEMS||it>=ITEMS||regalia_owner(it)>=0)return 0;
    ItemDef*d=&itemDefs[it];memset(out,0,sizeof(*out));out->output=it;out->qty=1;out->station=FORGE;
    out->tech=d->material>=4?3:d->material>=2?2:1;out->smithing=d->quality*12;
    int items[]={IRONBAR,CLOTH,WOOD,RELIC,COAL},needs[]={2+d->material+d->quality*2,d->slot?2+d->quality:0,d->slot==0,d->material==5,d->quality>=2};
    for(int i=0;i<5;i++)if(needs[i]){out->item[out->count]=items[i];out->need[out->count++]=needs[i];}return 1;
}
int forge_equipment(int it) {
    Actor *a = &g.actors[g.selected];
    CraftRequirements requirements;
    if (!forge_requirements(it,&requirements))
        return 0;
    ItemDef *d = &itemDefs[it];
    if (nearby_workstation(FORGE) < 0) {
        log_msg("Walk to the forge or its indoor workbench.");
        return 0;
    }
    int tech = requirements.tech, skill = requirements.smithing;
    if (g.tech < tech || level(a, SMITHING) < skill) {
        log_msg("Requires research %d and smithing %d.", tech, skill);
        return 0;
    }
    for(int i=0;i<requirements.count;i++)if(inventory_count(a,requirements.item[i])<requirements.need[i]){
        log_msg("Missing %d %s.",requirements.need[i]-inventory_count(a,requirements.item[i]),itemDefs[requirements.item[i]].name);return 0;
    }
    Actor backup = *a;int owner=0;
    for(int i=0;i<requirements.count;i++){
        if(!owner)owner=inventory_input_owner(a,requirements.item[i],requirements.need[i]);
        inventory_remove(a,requirements.item[i],requirements.need[i]);
    }
    if (!inventory_add_owned(a, it, 1, owner)) {
        *a = backup;
        log_msg("Pack full. Forging cancelled.");
        return 0;
    }
    if(!depth_use_fuel(nearby_workstation(FORGE),1)){*a=backup;return 0;}
    gain(a, SMITHING, 45 + d->quality * 20);
    log_msg("Forged %s.", d->name);
    return 1;
}
int storage_transfer(int b, int from, int slot) {
    if (b < 0 || b >= g.buildingCount || g.buildings[b].type != STORE)
        return 0;
    Actor *a = &g.actors[g.selected];
    Building *z = &g.buildings[b];
    if (actor_inside(a) ? nearby_station(STORE, 1.7f) != b : distance(a->x, a->y, z->x, z->y) > 1.7f) {
        log_msg("Walk to the depot before transferring goods.");
        return 0;
    }
    if (from) {
        if (slot < 0 || slot >= 16 || !z->storage[slot].item)
            return 0;
        int it = z->storage[slot].item;
        if (!inventory_add(a, it, 1))
            return 0;
        if (--z->storage[slot].qty == 0)
            z->storage[slot].item = 0;
        log_msg("Withdrew %s.", itemDefs[it].name);
        return 1;
    }
    if (slot < 0 || slot >= PACK || !a->pack[slot].item)
        return 0;
    int dest = -1, it = a->pack[slot].item;
    for (int i = 0; i < 16; i++)
        if (z->storage[i].item == it && z->storage[i].qty < 999) {
            dest = i;
            break;
        }
    if (dest < 0)
        for (int i = 0; i < 16; i++)
            if (!z->storage[i].item) {
                dest = i;
                break;
            }
    if (dest < 0) {
        log_msg("Depot has no free stack.");
        return 0;
    }
    if (a->stolen[slot]) {
        log_msg("Fence stolen goods before using a public depot.");
        return 0;
    }
    z->storage[dest].item = it;
    z->storage[dest].qty++;
    inventory_remove_slot(a, slot, 1);
    log_msg("Stored %s.", itemDefs[it].name);
    return 1;
}
int start_sleep(int town) {
    Actor *a = &g.actors[g.selected];
    int bed = nearby_station(BED, 1.7f);
    if (bed < 0) {
        log_msg("Enter the inn and choose Sleep in bed beside a bed. 25 silver.");
        return 0;
    }
    Building *b = &g.buildings[bed];
    int price = b->owner < 0 && !trail_bed(bed) && town >= 0 ? regional_bed_cost(town) : 0;
    return rest_at(a, bed, b->x, b->y, 0, price);
}
int doctor_heal(int town) {
    Actor *a = &g.actors[g.selected];
    if (town < 0 || town_near(a->x, a->y, 14) != town || g.money < 80) {
        log_msg("Town treatment costs 80 silver.");
        return 0;
    }
    if (a->race == 2) {
        log_msg("Mechanical bodies need spare parts.");
        return 0;
    }
    return begin_treatment(g.selected, g.selected, TREAT_CLINIC, -1);
}
float settlement_time(void) {
    return (g.day - 1.f + g.dayTime / 24.f) * DAY_SECONDS;
}
int nearest_ore(float x, float y, float reach, int type) {
    int best = -1;
    for (int i = 0; i < g.nodeCount; i++) {
        const Node *n = &g.nodes[i];
        if (n->type > NODE_COPPER || (type >= 0 && n->type != type) || !n->remaining ||
            fabsf(x - n->x) >= reach || fabsf(y - n->y) >= reach)
            continue;
        float d = distance(x, y, n->x, n->y);
        if (d < reach) {
            reach = d;
            best = i;
        }
    }
    return best;
}
void town_life_init(void) {
    if (g.townLife.layout)
        return;
    g.townLife.layout = 1;
    /* Old expeditions retain the people and terrain, with ordinary interactions. */
    for (int i = 0; i < DEPTH_NPCS; i++)
        g.depth.npc[i] = -1;
    /* Existing scrap villages had a house at the shop's exact location. */
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type != HOUSE || b->owner >= 0)
            continue;
        int t = town_near(b->x, b->y, 15);
        if (t < 0 || civil_style(t) != 3)
            continue;
        for (int j = 0; j < g.buildingCount; j++) {
            const Building *shop = &g.buildings[j];
            if (shop->type == SHOP && shop->owner < 0 &&
                distance(b->x, b->y, shop->x, shop->y) < 1) {
                b->x = towns[t].x + 10;
                b->y = towns[t].y - 1;
                break;
            }
        }
    }
    /* Starter towns have a real seam close enough to find on a first outing. */
    for (int i = 0; i < START_SCENARIOS; i++) {
        int t = startScenarios[i].town;
        float x = towns[t].x, y = towns[t].y;
        if (nearest_ore(x, y, 17, NODE_ORE) >= 0)
            continue;
        for (int j = 0; j < 16; j++) {
            float angle = j * .392699f + .40f;
            float ox = x + 13 * cosf(angle), oy = y + 13 * sinf(angle);
            int nearby = 0;
            for (int n = 0; n < g.nodeCount; n++)
                if (distance(ox, oy, g.nodes[n].x, g.nodes[n].y) < 1.3f) {
                    nearby = 1;
                    break;
                }
            if (!nearby && can_walk(ox, oy) && can_walk(ox + .8f, oy) && can_walk(ox, oy + .8f)) {
                add_node(ox, oy, NODE_ORE, FREE, NONE, 0);
                break;
            }
        }
    }
}
const char *contract_title(int kind) {
    static const char *titles[] = {
        "Town dispatch", "Iron for the smithy", "Clear the roads", "Salvage for repairs",
        "Freight delivery", "Caravan escort", "Watch patrol", "Named bounty"
    };
    return kind >= 0 && kind < CONTRACT_TYPES ? titles[kind] : "Work";
}
#define HUNT_REACH 96.f
static int quarry(const Actor *a) {
    return !a->squad && (a->kind == BEAST || (a->kind == RAIDER && a->faction == HOSTILE));
}
static int hunt_report(int town, float *x, float *y) {
    float closest = HUNT_REACH;
    int count = 0;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (!quarry(a) || !conscious(a) || a->defeatCredited || actor_inside(a) ||
            g.depth.actors[i].captive || population_hidden(i))
            continue;
        float d = distance(a->x, a->y, towns[town].x, towns[town].y);
        if (d >= HUNT_REACH)
            continue;
        count++;
        if (d < closest) {
            closest = d;
            *x = a->x;
            *y = a->y;
        }
    }
    for (int i = 0; i < POP_GROUPS && g.population.version; i++) {
        const WorldEvent *e = &g.population.groups[i];
        if (e->spawned || (e->type != POP_RAIDERS && e->type != POP_BEASTS))
            continue;
        float d = distance(e->x, e->y, towns[town].x, towns[town].y);
        if (d >= HUNT_REACH)
            continue;
        int standing = 0;
        for (int j = 0; j < e->count; j++)
            if (!(g.population.state[i].detached & (1u << j)) && !e->reserve[j].dead &&
                e->reserve[j].unconscious <= 0 && !e->reserve[j].defeatCredited)
                standing++;
        count += standing;
        if (standing && d < closest) {
            closest = d;
            *x = e->x;
            *y = e->y;
        }
    }
    return count >= 3;
}
static int posting_cycle(int town) { return 1 + (g.day + town % 2) / 2; }
int contract_offer(int town, int row) {
    if (town < 0 || town >= TOWNS || row < 0)
        return -1;
    int cycle = posting_cycle(town);
    if (g.townLife.boardCycle[town] != cycle && settlement_time() < g.townLife.boardAfter[town])
        return -1;
    unsigned mix = (g.seed ^ (town + 1u) * 2654435761u) + cycle * 2246822519u;
    mix ^= mix >> 16;
    int count = mix % 3;
    unsigned taken = g.townLife.boardCycle[town] == cycle ? g.townLife.boardTaken[town] : 0;
    for (int j = 0; j < count; j++) {
        int kind = ((mix >> 8) + j * 3) % CONTRACT_TYPES;
        if (taken & (1u << kind))
            continue;
        int already = 0;
        for (int i = 0; i < CONTRACTS; i++)
            if (g.contracts[i].active == 1 && g.contracts[i].town == town &&
                g.contracts[i].type == kind)
                already = 1;
        float x, y;
        if (already || (kind == CONTRACT_HUNT && !hunt_report(town, &x, &y)) ||
            (kind >= CONTRACT_ESCORT && !society_job_available(town, kind)))
            continue;
        if (row-- == 0)
            return kind;
    }
    return -1;
}
int journal_slot(int row) {
    for (int i = 0; i < CONTRACTS; i++)
        if (g.contracts[i].active && row-- == 0)
            return i;
    return -1;
}
int journal_count(void) {
    int count = 0;
    for (int i = 0; i < CONTRACTS; i++)
        count += g.contracts[i].active != 0;
    return count;
}
int work_dialogue_count(int town) {
    int rows = 0;
    while (contract_offer(town, rows) >= 0)
        rows++;
    return rows + 1;
}
int work_contact(int town) {
    if (town < 0 || town >= TOWNS)
        return -1;
    int best = -1;
    unsigned best_score = 0;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (a->squad || a->homeTown != town || a->faction != g.townOwner[town] || !conscious(a) ||
            a->prisonTime > 0 || a->target >= 0 || population_owned(i) || g.depth.actors[i].captive ||
            distance(a->x, a->y, towns[town].x, towns[town].y) > 16)
            continue;
        int role = npc_role(a);
        if (role != ROLE_MERCHANT && role != ROLE_MARSHAL && role != ROLE_INNKEEPER && role != ROLE_SMITH)
            continue;
        unsigned score = (g.seed ^ ((i + 1u) * 2654435761u)) + posting_cycle(town) * 97u;
        if (best < 0 || score > best_score) {
            best = i;
            best_score = score;
        }
    }
    return best;
}
const char *work_hint(int town, int speaker) {
    static char text[120];
    int contact = work_contact(town);
    if (contract_offer(town, 0) < 0 || contact < 0)
        return "I don't know of any paying work here today.";
    if (contact == speaker)
        return "I've got work, if you're looking to earn some silver.";
    const Actor *a = &g.actors[contact];
    int role = npc_role(a);
    const char *where = role == ROLE_MERCHANT ? "the trading post" : role == ROLE_INNKEEPER ? "the inn" :
                        role == ROLE_SMITH ? "the smithy" : "the watch post";
    snprintf(text, sizeof(text), "Ask %s by %s. They have some work.", a->name, where);
    return text;
}
int contract_destination(int town, int faction) {
    float best = WORLD * 2.f;
    int target = -1;
    for (int t = 0; t < TOWNS; t++) {
        int owner = g.townOwner[t];
        if (t == town || t == 7 || t == 11 || t == 14 || owner == HOSTILE ||
            g.rep[owner] < -25 || war_rivals(faction, owner))
            continue;
        float d = distance(towns[town].x, towns[town].y, towns[t].x, towns[t].y);
        if (d < best) {
            best = d;
            target = t;
        }
    }
    return target < 0 ? (town + 1) % TOWNS : target;
}
static void take_posting(int town, int kind) {
    int cycle = posting_cycle(town);
    if (g.townLife.boardCycle[town] != cycle) {
        g.townLife.boardCycle[town] = cycle;
        g.townLife.boardTaken[town] = 0;
    }
    g.townLife.boardTaken[town] |= 1u << kind;
    g.townLife.boardAfter[town] = settlement_time() + DAY_SECONDS * .7f;
}
static int contract_carrier(const Actor *a, int town) {
    return a->active && a->squad && !a->dead &&
           (town < 0 || town_near(a->x, a->y, 14) == town);
}
int contract_item_count(int item, int town) {
    int count = 0;
    for (int i = 0; i < g.actorCount; i++)
        if (contract_carrier(&g.actors[i], town))
            count += inventory_count(&g.actors[i], item);
    return count;
}
int contract_use_items(int item, int qty, int town) {
    if (contract_item_count(item, town) < qty)
        return 0;
    for (int i = 0; qty && i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (contract_carrier(a, town)) {
            int take = (int)fminf(qty, inventory_count(a, item));
            if (take && inventory_remove(a, item, take))
                qty -= take;
        }
    }
    return 1;
}
int contract_turn_in(int slot) {
    const Contract *c = &g.contracts[slot];
    return c->type == CONTRACT_FREIGHT ? c->target : c->town;
}
int contract_ready(int slot) {
    const Contract *c = &g.contracts[slot];
    if (c->active != 1)
        return 0;
    if (c->type == CONTRACT_BOUNTY) {
        int who = g.society.jobActor[slot] - 1;
        return who >= 0 && who < g.actorCount && g.actors[g.selected].carrying == who &&
            g.actors[who].bountyReward > 0 && (g.actors[who].dead || g.actors[who].unconscious > 0);
    }
    return c->item ? contract_item_count(c->item, -1) >= c->needed : c->progress >= c->needed;
}
static void contract_update(Contract *c, int progress) {
    if (c->progress == progress)
        return;
    c->progress = progress;
    if (progress >= c->needed)
        log_level("%s ready. Claim at %s.", contract_title(c->type),
                  towns[contract_turn_in((int)(c - g.contracts))].name);
    else
        log_level("%s: %d/%d.", contract_title(c->type), progress, c->needed);
}
void contract_tick(void) {
    for (int i = 0; i < CONTRACTS; i++) {
        Contract *c = &g.contracts[i];
        if (c->active != 1)
            continue;
        if (c->item)
            contract_update(c, (int)fminf(c->needed, contract_item_count(c->item, -1)));
        else if (c->type == CONTRACT_DISPATCH && c->progress < c->needed) {
            const Actor *a = &g.actors[g.selected];
            if (conscious(a) && town_near(a->x, a->y, 12) == c->target)
                contract_update(c, c->needed);
        } else if (c->type == CONTRACT_BOUNTY)
            contract_update(c, contract_ready(i));
    }
}
void contract_defeat(Actor *victim) {
    if (!quarry(victim))
        return;
    for (int i = 0; i < CONTRACTS; i++) {
        Contract *c = &g.contracts[i];
        if (c->active == 1 && c->type == CONTRACT_HUNT && c->progress < c->needed &&
            (c->baseline >= 0 || distance(victim->x, victim->y, towns[c->town].x, towns[c->town].y) < HUNT_REACH))
            contract_update(c, c->progress + 1);
    }
}
int contract_location(int slot, float *x, float *y) {
    if (slot < 0 || slot >= CONTRACTS)
        return -1;
    const Contract *c = &g.contracts[slot];
    if (c->active != 1)
        return -1;
    int town = contract_turn_in(slot);
    if (!contract_ready(slot)) {
        if (c->type == CONTRACT_DISPATCH || c->type == CONTRACT_ESCORT)
            town = c->target;
        if (c->type == CONTRACT_HUNT && g.townLife.huntX[slot] > 0) {
            *x = g.townLife.huntX[slot];
            *y = g.townLife.huntY[slot];
            return town;
        }
        if (c->type == CONTRACT_BOUNTY) {
            int who = g.society.jobActor[slot] - 1;
            if (who >= 0 && who < g.actorCount && !g.actors[who].dead && g.actors[who].unconscious <= 0) {
                *x = g.actors[who].homeX;
                *y = g.actors[who].homeY;
                return town;
            }
        }
        if (c->type == CONTRACT_ORE) {
            int node = nearest_ore(towns[town].x, towns[town].y, 40, NODE_ORE);
            if (node >= 0) {
                *x = g.nodes[node].x;
                *y = g.nodes[node].y;
                return town;
            }
        }
    }
    *x = towns[town].x;
    *y = towns[town].y;
    return town;
}
int accept_contract(int town, int kind) {
    if (town < 0 || town >= TOWNS || kind < 0 || kind >= CONTRACT_TYPES)
        return 0;
    if (kind >= CONTRACT_FREIGHT) {
        if (!society_contract(town, kind))
            return 0;
        take_posting(town, kind);
        return 1;
    }
    int n = -1;
    for (int i = 0; i < CONTRACTS; i++) {
        const Contract *c = &g.contracts[i];
        if (c->active == 1 && c->town == town && c->type == kind) {
            log_msg("That contract is already in your journal.");
            return 0;
        }
        if (n < 0 && c->active != 1)
            n = i;
    }
    if (n < 0) {
        log_msg("Journal full. Finish an active contract.");
        return 0;
    }
    float x = 0, y = 0;
    if (kind == CONTRACT_HUNT && !hunt_report(town, &x, &y)) {
        log_msg("The roads are quiet. Check the board later.");
        return 0;
    }
    Contract c = {0};
    c.active = 1;
    c.type = kind;
    c.town = town;
    c.target = contract_destination(town, g.townOwner[town]);
    c.item = kind == CONTRACT_ORE ? ORE : kind == CONTRACT_RELIC ? RELIC : NONE;
    c.needed = kind == CONTRACT_ORE ? 8 : kind == CONTRACT_HUNT ? 3 : 1;
    c.reward = kind == CONTRACT_DISPATCH ? 220 : kind == CONTRACT_ORE ? 240 : kind == CONTRACT_HUNT ? 420 : 550;
    c.baseline = -1; /* Earlier hunt postings allowed encounters anywhere. */
    g.contracts[n] = c;
    g.townLife.huntX[n] = x;
    g.townLife.huntY[n] = y;
    take_posting(town, kind);
    log_level("Accepted %s at %s. See Journal.", contract_title(kind), towns[town].name);
    contract_tick();
    return 1;
}
void contract_description(int n, char *out, int size) {
    if (n < 0 || n >= CONTRACTS || !g.contracts[n].active) {
        snprintf(out, size, "Ask a merchant, innkeeper or marshal for current work.");
        return;
    }
    const Contract *c = &g.contracts[n];
    if (c->active == 2) {
        snprintf(out, size, "Paid and closed at %s.", towns[contract_turn_in(n)].name);
        return;
    }
    if (c->type >= CONTRACT_FREIGHT) {
        society_contract_text(n, out, size);
        return;
    }
    if (contract_ready(n)) {
        snprintf(out, size, "%s complete (%d/%d). Return to %s and claim %d silver here in Journal.",
                 contract_title(c->type), c->needed, c->needed, towns[c->town].name, c->reward);
        return;
    }
    switch (c->type) {
    case CONTRACT_DISPATCH:
        snprintf(out, size, "The watch needs to share its road reports. Deliver the dispatch to %s (0/1), then return to %s.",
                 towns[c->target].name, towns[c->town].name);
        break;
    case CONTRACT_HUNT:
        snprintf(out, size, "Raiding parties and wild packs have stopped traffic. Defeat %d raiders or predators%s (%d/%d).",
                 c->needed, c->baseline < 0 ? " near town" : "", c->progress, c->needed);
        break;
    default:
        snprintf(out, size, "%s Bring %d %s to %s (%d/%d in squad).",
                 c->type == CONTRACT_ORE ? "The smith is short of iron for repairs." : "An old relic could help repair the town pump.",
                 c->needed, itemDefs[c->item].name, towns[c->town].name,
                 contract_item_count(c->item, -1), c->needed);
        break;
    }
}
int complete_contract(int n) {
    if (n < 0 || n >= CONTRACTS || g.contracts[n].active != 1)
        return 0;
    Contract *c = &g.contracts[n];
    Actor *a = &g.actors[g.selected];
    if (!conscious(a)) return 0;
    if (c->type >= CONTRACT_FREIGHT)
        return society_complete(n);
    if (town_near(a->x, a->y, 14) != c->town) {
        log_msg("Claim at %s.", towns[c->town].name);
        return 0;
    }
    if (c->item) {
        if (!contract_use_items(c->item, c->needed, c->town)) {
            log_msg("Bring the required supplies and their carriers to %s.", towns[c->town].name);
            return 0;
        }
    } else if (c->progress < c->needed) {
        log_msg("Contract objective is incomplete.");
        return 0;
    }
    c->completed = 1;
    c->active = 2;
    g.money += c->reward;
    g.contractsDone++;
    g.rep[g.townOwner[c->town]] += 8;
    g.prosperity[c->town] = (int)limit(g.prosperity[c->town] + 5, 0, 100);
    gain(a, PERCEPTION, 70);
    log_warn("%s paid %d silver for %s.", towns[c->town].name, c->reward, contract_title(c->type));
    return 1;
}
int dialogue_dismissive(int actor) {
    if (actor < 0 || actor >= g.actorCount)
        return 0;
    const Actor *a = &g.actors[actor];
    if (a->kind != SETTLER || regional_named(actor) || civil_elite_index(actor) >= 0 ||
        politics_leader_title(actor))
        return 0;
    int room = actor_inside(a);
    if (room && g.buildings[room - 1].type == HOUSE && (g.dayTime >= 19 || g.dayTime < 6))
        return 1;
    return npc_role(a) == ROLE_CITIZEN && (g.seed + actor * 17u + g.day) % 5 < 2;
}
const char *dialogue_line(int n, int page) {
    if (page == DIALOG_INSPECTION)
        return "We've had thefts in town. Mind if I look through your pockets?";
    if (page == DIALOG_DISMISS) {
        static const char *lines[] = {"I'm busy. Try someone else.", "Can I have a minute to myself?",
                                     "Keep walking, stranger.", "I haven't got anything to say."};
        return lines[(g.seed + (unsigned)n) % 4];
    }
    if(!page&&population_owned(n))return population_dialogue(n);
    if (n < 0 || n >= g.actorCount)
        return "The road is quiet.";
    Actor *a = &g.actors[n];
    if (a->prisonTime > 0)
        return "The guards treat injuries in custody. Pay bail, serve the sentence, or pick the lock. "
               "Tools help. A failed escape adds time and bounty.";
    if (page == DIALOG_WORK_HINT)
        return work_hint(town_near(g.actors[g.selected].x, g.actors[g.selected].y, 14), n);
    if (page == DIALOG_WORK)
        return contract_offer(town_near(g.actors[g.selected].x, g.actors[g.selected].y, 14), 0) < 0
            ? "Nothing posted today. Check back after the next jobs go up."
            : "Here's what the town needs. The Journal keeps track after you take a posting.";
    if (page == 4)return trade_tip(town_near(a->x,a->y,14));
    if (page == 5){
        const char *rumor=war_rumor(a);
        int town=town_near(a->x,a->y,14);
        if (rumor)
            return rumor;
        if (town >= 0 && contract_offer(town, 0) >= 0)
            return work_hint(town, n);
        return town >= 0 ? town_custom(town) : npc_rumor(a);
    }
    if (page == 2)
        return town_custom(town_near(a->x,a->y,14));
    if (page == 3)
        return faction_identity(a->faction);
    if(limb_surgeon(n))return "I remove arms or legs for 400 silver. Removal is permanent. Buy a replacement first; fitting costs two machine parts. Black Mesa sells relic limbs.";
    if(politics_leader_title(n)){
        static char line[160];
        snprintf(line,sizeof(line),"The council seated me at %s. %s",towns[a->homeTown].name,politics_policy_text(a->faction));
        return line;
    }
    int elite=civil_elite_index(n);
    if(elite>=0)return eliteDefs[elite].story;
    if (npc_role(a) == ROLE_MERCENARY) {
        static char terms[190];
        int s = merc_slot(a);
        if (s < 0)
            return "I sell protection, not allegiance. One guard for one day. I keep my own gear and fight "
                   "independently. Escort or camp duty?";
        snprintf(terms, sizeof(terms),
                 "ON CONTRACT: %s. %.1f minutes remain. I fight independently and wait outside interiors. "
                 "Dismissal gives no refund.",
                 g.mercMode[s] ? "CAMP WATCH" : "ESCORT", (g.mercUntil[s] - g.time) / 60.f);
        return terms;
    }
    unsigned voice = (g.seed + n * 17u + g.day / 3) % 3;
    switch (npc_role(a)) {
    case ROLE_MERCHANT: {
        static const char *lines[] = {"I've got food, dressings and whatever came in on the last wagon.",
            "Traveling far? The pack you choose matters once you start carrying iron.",
            "Lay your salvage here. I'll tell you what I can pay for it."};
        return lines[voice];
    }
    case ROLE_INNKEEPER:
        return "A bed costs 25 silver. Step inside the inn to rest. Patch bleeding wounds before you sleep.";
    case ROLE_MEDIC:
        return "Eighty silver for wound care. I can stop bleeding and restore health, but lost limbs need "
               "mechanical replacements.";
    case ROLE_SMITH:
        return "The forge is here. Bring iron, cloth and fuel. Your smithing skill and research determine "
               "what you can make.";
    case ROLE_BARBER:
        return "Sit down and tell me how you want to look. I can change hair, beard and appearance without "
               "changing your skills.";
    case ROLE_RECRUIT:
        return "Three hundred silver buys my supplies and my sword. We share the road, the work and the "
               "danger.";
    case ROLE_MARSHAL: {
        static const char *lines[] = {"Something the watch can do for you?",
            "Keep your quarrels outside the gate. We've got enough to do.",
            "State your business. I'll hear you out."};
        return lines[voice];
    }
    case ROLE_SCOUT:
        return "Mark a destination on your atlas. Watch for raiders between towns, and keep enough food for "
               "the return trip.";
    }
    static const char *factionLines[FACTIONS] = {
        "No lord owns the road. Find honest work if you can, and keep one hand near your blade.",
        "The Compact keeps the wells open. Do your share, pay your debts, and our gates will shelter you.",
        "Everything has a second life. Iron, coats, people. Have a look at the stock; the caravans bring "
        "different pieces every dawn.",
        "This is Vulture ground. The weak pay in silver. The foolish pay in blood.",
        "A sword is a promise. The Shogunate remembers those who keep it, and those who break it. Mind your "
        "conduct in our streets.",
        "Sit in the shade, traveler. The ash takes everyone in time. Until then, there is work, medicine, "
        "and a little kindness.",
        "Nobody asks where a thing came from in Black Mesa. Keep your voice low and your business "
        "profitable.",
        "A low growl rises from the dust."};
    return factionLines[a->faction % FACTIONS];
}
const char *ambient_line(int n) {
    if (n >= 0 && n < g.actorCount) {
        const char *rumor=war_ambient(g.actors[n].speech,g.actors[n].faction);
        if(rumor)return rumor;
    }
    static const char *lines[24] = {"Storm's coming. Cover your eyes.",
                                    "Fresh iron. Fair prices.",
                                    "Keep your blade sheathed here.",
                                    "The old wells are running dry.",
                                    "Need another sword on the road?",
                                    "Nobody crosses the scar alone.",
                                    "I count the mileposts to stay awake.",
                                    "A bowl of rice beats a grave.",
                                    "The Shogun taxes even the dead.",
                                    "Check your straps before dusk.",
                                    "Black Mesa never asks questions.",
                                    "My brother never came home.",
                                    "Work on the board, if you need it.",
                                    "These plates stopped an arrow once.",
                                    "Rain smells like rust again.",
                                    "No credit. Not after last winter.",
                                    "Keep moving, drifter.",
                                    "The monks still take the wounded.",
                                    "A sharp blade won't fill your belly.",
                                    "Someone's been opening our crates.",
                                    "They say the north is white as bone.",
                                    "Don't sleep in the open tonight.",
                                    "One more caravan, then I'm done.",
                                    "The frontier remembers."};
    if (n < 0 || n >= g.actorCount)
        return "";
    return lines[g.actors[n].speech % 24];
}
void dialogue_open(int n) {
    ui.dialogActor = n;
    ui.dialogPage = dialogue_dismissive(n) ? DIALOG_DISMISS : 0;
    ui.selection = 0;
    ui.screen = SCREEN_DIALOG;
}
float pickpocket_chance(const Actor *a) {
    return fmaxf(0, fminf(.9f, .25f + level(a, THEFT) * .007f + level(a, STEALTH) * .003f + gear_bonus(a, 2) * .003f));
}
int pickpocket_item(int n) {
    if (n < 0 || n >= g.actorCount) return NONE;
    for (int i = 0; i < PACK; i++)
        if (g.actors[n].pack[i].item) return g.actors[n].pack[i].item;
    return NONE;
}
int theft_behind(int n) {
    if(n<0 || n>=g.actorCount) return 0;
    Actor *a=&g.actors[g.selected],*b=&g.actors[n];
    float side=(depth_x(a)-depth_x(b))-(depth_y(a)-depth_y(b));
    return b->facing ? side < -.25f : side > .25f;
}
static int theft_sight(const Actor *observer,const Actor *a) {
    if(actor_inside(observer)!=actor_inside(a)) return 0;
    float d=depth_distance(observer,a);
    if(d>7) return 0;
    float dx=depth_x(a)-depth_x(observer),dy=depth_y(a)-depth_y(observer);
    if(d>2 && (observer->facing ? dx-dy<-.5f : dx-dy>.5f)) return 0;
    /* Exterior walls occlude witnesses; people in the same room can see. */
    if(!actor_inside(a)) for(int k=1;k<12;k++) {
        float x=observer->x+dx*k/12.f,y=observer->y+dy*k/12.f;
        for(int j=0;j<g.buildingCount;j++) {
            Building *z=&g.buildings[j];
            if(building_has_interior(z->type) && fabsf(x-z->x)<1.5f && fabsf(y-z->y)<1.3f) return 0;
        }
    }
    return 1;
}
int theft_witnesses(int n) {
    int count=0; Actor *a=&g.actors[g.selected];
    for(int i=0;i<g.actorCount;i++) {
        Actor *b=&g.actors[i];
        if(i!=n && !b->squad && b->kind!=BEAST && b->active && conscious(b) && theft_sight(b,a)) count++;
    }
    return count;
}
float theft_chance(int n,int slot) {
    if(n<0 || n>=g.actorCount || slot<0 || slot>=PACK) return 0;
    int it=g.actors[n].pack[slot].item;
    if(!it) return 0;
    float penalty=fminf(.25f,itemDefs[it].weight*.018f+itemDefs[it].price/6000.f);
    return limit(pickpocket_chance(&g.actors[g.selected]) + (theft_behind(n)?.16f:-.14f)
                 - fminf(.6f,.15f*theft_witnesses(n))-penalty,.02f,.95f);
}
int pickpocket_slot(int n,int source) {
    Actor *a=&g.actors[g.selected];
    if(n<0 || n>=g.actorCount || source<0 || source>=PACK || !a->sneaking || !conscious(a)) return 0;
    Actor *b=&g.actors[n];
    if(!b->active || b->squad || b->dead || b->unconscious>0 || b->kind==BEAST || depth_distance(a,b)>TALK_REACH) return 0;
    int it=b->pack[source].item,room=0;
    if(!it) {log_msg("Their pockets are empty.");return 0;}
    for(int i=0;i<PACK;i++) if(!a->pack[i].item || (a->pack[i].item==it && a->stolen[i]==b->faction && a->pack[i].qty<999)) room=1;
    if(!room) {log_msg("Your pack is full.");return 0;}
    float chance=theft_chance(n,source);
    gain(a,THEFT,22);
    if((wrand()%1000)/1000.f>=chance) {
        town_theft(town_near(a->x, a->y, 17));
        crime_add(a,b->faction,80);g.rep[b->faction]-=18;b->speech=19;b->speechTime=5;
        log_warn("Caught stealing! Bounty +80; faction standing -18.");return 0;
    }
    if(!inventory_add_owned(a,it,1,b->faction)) return 0;
    inventory_remove_slot(b,source,1);
    town_theft(town_near(a->x, a->y, 17));
    log_msg("Lifted %s. Find a fence before selling it back.",itemDefs[it].name);return 1;
}
int pickpocket(int n) {
    if(n<0 || n>=g.actorCount) return 0;
    for(int i=0;i<PACK;i++) if(g.actors[n].pack[i].item) return pickpocket_slot(n,i);
    log_msg("Their pockets are empty.");return 0;
}
static int worker_building(const Actor *a, int type) {
    int found = -1;
    float best = 15;
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        float d = distance(a->x, a->y, b->x, b->y);
        if (b->type == type && depth_build_ready(i) && (b->owner >= 0 || g.rep[b->faction] >= -25) && d < best) {
            found = i;
            best = d;
        }
    }
    return found;
}
static int depot_has(int depot, int item) {
    if (depot < 0)
        return 0;
    int n = 0;
    for (int i = 0; i < 16; i++)
        if (g.buildings[depot].storage[i].item == item)
            n += g.buildings[depot].storage[i].qty;
    return n;
}
static int worker_walk(Actor *a, int building, float dt) {
    Building *b = &g.buildings[building];
    if (distance(a->x, a->y, b->x, b->y) <= 1.5f)
        return 0;
    route_toward(a, b->x, b->y, dt);
    return 1;
}
static void worker_production(Actor *a, float dt) {
    int job = assigned_job(a);
    if (!automatic_work(a) || g.depth.actors[a-g.actors].paused || society_rescue_busy((int)(a-g.actors)) || g.depth.actors[a-g.actors].captive || actor_inside(a) || job < JOB_SMELT || a->taskKind || !conscious(a) || a->prisonTime > 0 ||
        a->carriedBy >= 0 || a->target >= 0)
        return;
    int depot = worker_building(a, STORE),
        station = worker_building(a, job == JOB_SMELT ? FORGE : CAMPFIRE);
    int old = g.selected;
    int output = job == JOB_SMELT ? IRONBAR : RATION;
    /* Finish one trip before seeking another task: unload production and raw haul cargo. */
    for (int i = 0; i < PACK; i++) {
        int it = a->pack[i].item;
        int haul =
            job == JOB_HAUL && (it == ORE || it == COPPER || it == WOOD || it == GRAIN || it == IRONBAR);
        if (depot >= 0 && !a->stolen[i] &&
            (haul || (job != JOB_HAUL && it == output && a->pack[i].qty > (output == RATION ? 2 : 0)))) {
            work_notice(a, "hauling supplies to the depot", NULL);
            g.selected = (int)(a - g.actors);
            if (!worker_walk(a, depot, dt)) {
                a->action -= dt;
                if (a->action <= 0) {
                    storage_transfer(depot, 0, i);
                    a->action = .5f;
                }
            }
            g.selected = old;
            return;
        }
    }
    if (job == JOB_HAUL || station < 0) {
        g.selected = old;
        return;
    }
    int ingredient = ORE, need = 3;
    if (job == JOB_COOK) {
        ingredient = GRAIN;
        if (inventory_count(a, GRAIN) < 3 && inventory_count(a, GRAIN) + depot_has(depot, GRAIN) < 3 &&
            inventory_count(a, MEAT) + depot_has(depot, MEAT) >= 2) {
            ingredient = MEAT;
            need = 2;
        }
    }
    int missing = inventory_count(a, ingredient) < need              ? ingredient
                  : job == JOB_COOK && !inventory_count(a, WATER) ? WATER
                                                                     : 0;
    if (missing) {
        if (depot >= 0 && depot_has(depot, missing))
            work_notice(a, "fetching", itemDefs[missing].name);
        g.selected = (int)(a - g.actors);
        if (depot >= 0 && depot_has(depot, missing) && !worker_walk(a, depot, dt)) {
            for (int j = 0; j < 16; j++)
                if (g.buildings[depot].storage[j].item == missing) {
                    storage_transfer(depot, 1, j);
                    break;
                }
        }
        g.selected = old;
        return;
    }
    work_notice(a, job == JOB_SMELT ? "working at the forge" : "going to cook rations", NULL);
    g.selected = (int)(a - g.actors);
    if(g.buildings[station].owner>=0&&g.depth.buildings[station].fuel<1) {
        int fuel=inventory_count(a,COAL)?COAL:inventory_count(a,WOOD)?WOOD:0;
        if(fuel){if(!worker_walk(a,station,dt))depth_fuel(station);}
        else if(depot>=0&&!worker_walk(a,depot,dt)){for(int j=0;j<16;j++)if(g.buildings[depot].storage[j].item==COAL||g.buildings[depot].storage[j].item==WOOD){storage_transfer(depot,1,j);break;}}
        g.selected=old;return;
    }
    if (!worker_walk(a, station, dt)) {
        a->action -= dt;
        if (a->action <= 0) {
            craft_item(job == JOB_SMELT ? 0 : 1);
            a->action = 5;
        }
    }
    g.selected = old;
}
static int jail_faction(const Actor *a) {
    return a->prisonTown >= 0 && a->prisonTown < TOWNS ? g.townOwner[a->prisonTown] : FREE;
}
int jail_authority(const Actor *guard, const Actor *patient) {
    return patient->prisonTime > 0 && patient->prisonTown >= 0 && guard->kind == GUARD &&
           guard->homeTown == patient->prisonTown && guard->faction == jail_faction(patient);
}
static int jail_guard(const Actor *a, float reach) {
    int found = -1;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *guard = &g.actors[i];
        if (!jail_authority(guard, a) || !conscious(guard))
            continue;
        float d = depth_distance(a, guard);
        if (d < reach) {
            found = i;
            reach = d;
        }
    }
    return found;
}
int jail_bail_cost(const Actor *a) { return wanted_amount(a, jail_faction(a)); }
static void jail_release(Actor *a) {
    a->prisonTime = 0;
    a->prisonTown = -1;
    a->taskKind = 0;
    a->target = -1;
}
int jail_bail(int n) {
    Actor *a = &g.actors[n];
    if (!conscious(a) || a->prisonTime <= 0 || a->taskKind == TASK_JAIL_CARE)
        return 0;
    int cost = jail_bail_cost(a), faction = jail_faction(a);
    if (g.money < cost) {
        log_msg("Insufficient silver for bail.");
        return 0;
    }
    g.money -= cost;
    crime_clear(a, faction);
    if (faction != FREE)
        g.rep[faction] = 0;
    jail_release(a);
    log_msg("%s was released on bail.", a->name);
    return 1;
}
float jail_pick_duration(const Actor *a, int tools) {
    return (tools ? 8.0f : 13.0f) - level(a, LOCKPICK) * (tools ? .045f : .065f);
}
int jail_pick_start(int n) {
    Actor *a = &g.actors[n];
    if (!conscious(a) || a->prisonTime <= 0 || a->taskKind == TASK_JAIL_CARE)
        return 0;
    if (a->taskKind == TASK_CELL_LOCK)
        return 1;
    int tools = inventory_count(a, TOOLS) > 0;
    a->taskKind = TASK_CELL_LOCK;
    a->taskIndex = tools;
    a->action = jail_pick_duration(a, tools);
    log_msg(tools ? "Picking the cell lock. Circle cancels." : "Working a wire into the cell lock. Circle cancels.");
    return 1;
}
static void jail_medical(Actor *a, float dt) {
    if (fatal_injury(a))
        return;
    if (a->taskKind != TASK_JAIL_CARE && (wound_needs_dressing(a) || depth_needs_repair(a))) {
        a->taskKind = TASK_JAIL_CARE;
        a->action = 2.8f + treatment_burden(a) * .65f;
        if (a->squad) {
            int guard = jail_guard(a, 22);
            log_warn("%s begins treating %s in custody.", guard >= 0 ? g.actors[guard].name : "Jail staff", a->name);
        }
    }
    if (a->taskKind != TASK_JAIL_CARE)
        return;
    a->action -= dt;
    if (a->action > 0)
        return;
    dress_wounds(a);
    injury_repair(a, &g.depth.actors[a - g.actors]);
    a->taskKind = 0;
    if (a->squad)
        log_msg("%s is under care. Wounds recover while serving the sentence.", a->name);
}
static void jail_tick(Actor *a, float dt) {
    if (fatal_injury(a))
        return;
    jail_medical(a, dt);
    if (a->taskKind == TASK_CELL_LOCK) {
        if (!conscious(a)) {
            a->taskKind = 0;
        } else {
            a->action -= dt;
            if (a->action <= 0) {
                int faction = jail_faction(a), guard = jail_guard(a, 4);
                int tools = a->taskIndex && inventory_count(a, TOOLS);
                float chance = 22 + level(a, LOCKPICK) * .76f + (tools ? 21 : 0) - (guard >= 0 ? 18 : 0);
                a->taskKind = 0;
                gain(a, LOCKPICK, 34);
                if (wrand() % 100 < fminf(97, chance)) {
                    int n = (int)(a - g.actors);
                    jail_release(a);
                    if (faction != FREE) {
                        crime_add(a, faction, 100);
                        g.rep[faction] -= 18;
                        g.society.recognized[n][faction] = 1800;
                    }
                    log_warn("%s picked the cell lock. Get clear of the guards.", a->name);
                    return;
                }
                a->prisonTime += 20;
                crime_add(a, faction, 20);
                log_warn("The cell lock held. Sentence and bounty increased.");
            }
        }
    }
    a->prisonTime -= dt;
    if (a->prisonTime > 0)
        return;
    if (a->taskKind == TASK_JAIL_CARE) {
        a->prisonTime = .01f;
        return;
    }
    crime_clear(a, jail_faction(a));
    jail_release(a);
    if (a->squad)
        log_msg("%s served their sentence.", a->name);
}
static void jail_arrest(Actor *a, Actor *guard, float dt) {
    int t = guard->homeTown, n = (int)(a - g.actors);
    a->prisonTown = t;
    a->prisonTime = 90 + wanted_amount(a, guard->faction) * .25f;
    a->x = towns[t].x - 6;
    a->y = towns[t].y + 5;
    for (int j = 0; j < 15 && !can_walk(a->x, a->y); j++) {
        a->x = towns[t].x - 8 + j % 5;
        a->y = towns[t].y + 5 + j / 5;
    }
    g.society.queueCount[n] = 0;
    society_rescue_cancel(n);
    a->taskKind = a->attackPhase = a->autoattack = 0;
    a->target = g.living.command[n] = -1;
    if (guard->target == n || guard->attackTarget == n)
        guard->attackPhase = 0;
    for (int s = 0; s < PACK; s++)
        if (a->stolen[s]) {
            a->pack[s] = (Stack){0, 0};
            a->stolen[s] = 0;
        }
    log_warn("%s taken into custody at %s.", a->name, towns[t].name);
    jail_medical(a, dt);
}
void tick_expansion(float dt) {
    g.weatherTimer -= dt;
    g.eventTimer -= dt;
    g.season = (g.day / 7) % 4;
    if (g.weatherTimer <= 0) {
        g.weatherTimer = 160 + wrand() % 160;
        int b = biome(g.actors[g.selected].x, g.actors[g.selected].y);
        int r = wrand() % 5;
        g.weather = r < 2 ? 0 : b == 6 ? 4 : b == 3 ? 3 : r == 2 ? 2 : 1;
        snprintf(g.worldNews, sizeof(g.worldNews), "%s over the borderlands. Watch your supplies.",
                 weatherNames[g.weather]);
    }
    if (g.eventTimer <= 0) {
        g.eventTimer = 220;
        int t = wrand() % TOWNS, k = wrand() % 4;
        g.townState[t] = k;
        g.prosperity[t] = (int)limit(g.prosperity[t] + (k == 0   ? 12
                                                        : k == 1 ? -12
                                                        : k == 2 ? 5
                                                                 : -18),
                                     15, 95);
        static const char *news[] = {"A caravan arrived in", "Drought is tightening prices in",
                                     "Farmers report rain near", "Border raiders struck"};
        snprintf(g.worldNews, sizeof(g.worldNews), "%s %s.", news[k], towns[t].name);
        if (k == 3) {
            g.factionWar[g.townOwner[t]] = 1;
            if (g.prosperity[t] < 25 && g.day > 2 && t > 0 && t < OLD_TOWNS && !(t==2 && g.regions.outcome[0]) && !(t==12 && g.regions.outcome[2]))
                politics_occupy(t,g.townOwner[t] == HOSTILE ? towns[t].faction : HOSTILE);
        }
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead)
            continue;
        a->speechTime = fmaxf(0, a->speechTime - dt);
        a->speechCooldown -= dt;
        if (a->speechCooldown <= 0 && !a->squad && a->kind != BEAST) {
            a->speechCooldown = 36 + wrand() % 70;
            const Actor *listener = &g.actors[g.selected];
            if (fabsf(a->x - listener->x) < 7 && fabsf(a->y - listener->y) < 7 &&
                conscious(a) && a->target < 0 && !g.depth.actors[i].captive && a->prisonTime <= 0 &&
                !population_hidden(i) && depth_distance(a, listener) < 7) {
                int rumor = war_chatter(i);
                if (rumor) {
                    a->speech = rumor;
                    a->speechTime = 6;
                    ui.chatterNext = g.time + 10;
                } else if (g.time >= ui.chatterNext && !hostile(a, listener) &&
                           !dialogue_dismissive(i) && (wrand() % 4 == 0)) {
                    int line = (i * 7 + g.day + (int)(g.time / 20)) % 24;
                    if (!(ui.chatterSeen & (1u << line))) {
                        a->speech = line;
                        a->speechTime = 4;
                        ui.chatterSeen |= 1u << line;
                        ui.chatterNext = g.time + 18;
                        if (ui.chatterSeen == (1u << 24) - 1)
                            ui.chatterSeen = 0;
                    }
                }
            }
        }
        if (a->prisonTime > 0) {
            jail_tick(a, dt);
            continue;
        }
        if (!a->squad)
            continue;
        if (!actor_inside(a) && a->unconscious > 0 && a->bounty >= 80 &&
            a->carriedBy < 0 && !g.depth.actors[i].captive && !fatal_injury(a)) {
            for (int j = 0; j < g.actorCount; j++) {
                Actor *b = &g.actors[j];
                int t = b->homeTown;
                if (b->kind != GUARD || b->faction == HOSTILE || t < 0 || t == 14 ||
                    g.townOwner[t] != b->faction || !conscious(b) ||
                    wanted_amount(a, b->faction) < 80 || depth_distance(a, b) >= 4)
                    continue;
                jail_arrest(a, b, dt);
                break;
            }
            if (a->prisonTime > 0)
                continue;
        }
        int sheltered = actor_inside(a) || a->taskKind == 3 || town_near(a->x, a->y, 8) >= 0;
        if ((g.weather == 3 || g.weather == 4) && !sheltered)
            a->exposure = fminf(100, a->exposure + dt * fmaxf(0, .20f - gear_bonus(a, 4) * .002f));
        else
            a->exposure = fmaxf(0, a->exposure - dt * .45f);
        if (a->exposure > 65) {
            a->health[CHEST] -= dt * .12f;
            if (a->health[CHEST] <= 0)
                injury_knockout(a, 0);
        }
        worker_production(a, dt);
    }
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == FARM && g.weather == 2)
            g.buildings[i].stock = 1;

}

#include "regional.h"

#include "living.h"

/* Six fixed roadside landmarks. Geometry leaves a direct lane and outer bypasses. */
void trail_point(int site,float*x,float*y,float*dx,float*dy){
 static const int ends[6][2]={{0,1},{0,2},{1,3},{5,6},{9,10},{11,13}};
 int a=ends[site][0],b=ends[site][1];float vx=towns[b].x-towns[a].x,vy=towns[b].y-towns[a].y,d=fmaxf(1,sqrtf(vx*vx+vy*vy));
 *dx=vx/d;*dy=vy/d;*x=towns[a].x+vx*.46f;*y=towns[a].y+vy*.46f;
}
int trail_rock(int site,int rock,float*x,float*y){float cx,cy,dx,dy;trail_point(site,&cx,&cy,&dx,&dy);float u=rock<2?-4:4,v=rock%2?-3.8f:3.8f;*x=cx+dx*u-dy*v;*y=cy+dy*u+dx*v;return !(g.trails.omit&(1<<(site*4+rock)));}
int trail_blocked(float x,float y){
 if(!world_is_expanded())return 0;
 for(int k=0;k<6;k++){float cx,cy,dx,dy;trail_point(k,&cx,&cy,&dx,&dy);if(fabsf(x-cx)>12||fabsf(y-cy)>12)continue;
  for(int r=0;r<4;r++){float rx,ry;if(!trail_rock(k,r,&rx,&ry))continue;if(distance(x,y,rx,ry)<1.65f)return 1;}
 }return 0;
}
int trail_bed(int b){for(int k=0;k<6;k++)if(g.trails.bed[k]==b+1)return 1;return 0;}
void trail_init(void){
 if(g.trails.version)return;g.trails.version=1;
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].active&&!g.actors[n].squad&&npc_role(&g.actors[n])==ROLE_MEDIC){inventory_add(&g.actors[n],MEDKIT,3);inventory_add(&g.actors[n],PARTS,2);}
 for(int k=0;k<6;k++)for(int r=0;r<4;r++){float x,y;trail_rock(k,r,&x,&y);for(int n=0;n<g.actorCount;n++)if(g.actors[n].active&&distance(x,y,g.actors[n].x,g.actors[n].y)<2.5f)g.trails.omit|=1<<(k*4+r);for(int b=0;b<g.buildingCount;b++)if(distance(x,y,g.buildings[b].x,g.buildings[b].y)<4.5f)g.trails.omit|=1<<(k*4+r);}

 for(int k=0;k<6;k++){float x,y,dx,dy;trail_point(k,&x,&y,&dx,&dy);x-=dy*10;y+=dx*10;
  int found=0;for(int r=0;r<12;r++){float xx=x+dx*r,yy=y+dy*r;if(can_walk(xx,yy)&&town_near(xx,yy,22)<0){x=xx;y=yy;found=1;break;}}if(!found)continue;
  int b=add_build(x,y,BED,FREE,-1);if(b<0)continue;g.trails.bed[k]=b+1;g.depth.buildings[b].labor=0;g.depth.buildings[b].condition=100;
  add_node(x+1.5f,y+.5f,NODE_LOOT,0,MEDKIT,2);add_node(x-1.5f,y+.5f,NODE_LOOT,0,RATION,2);add_node(x,y+1.5f,NODE_LOOT,0,PARTS,2);
 }
}
static int aid_safe_at(const Actor *patient, float x, float y) {
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *other = &g.actors[i];
        if (fabsf(other->x - x) < 9 && fabsf(other->y - y) < 9 &&
            conscious(other) && !actor_inside(other) && hostile(patient, other) &&
            distance(x, y, other->x, other->y) < 9)
            return 0;
    }
    return 1;
}
static int aid_patient(const Actor *a) {
    int n = (int)(a - g.actors);
    return a->active && a->squad && !a->dead && !fatal_injury(a) && !actor_inside(a) &&
        a->prisonTime <= 0 && !g.depth.actors[n].captive &&
        (a->unconscious > 0 || a->bleed > .05f || a->blood < 30);
}
static int aid_type(const Actor *medic, const Actor *patient) {
    if (patient->race == 2)
        return inventory_count(medic, PARTS) && depth_needs_repair(patient) ? TREAT_REPAIR : -1;
    if (inventory_count(medic, MEDKIT) && wound_needs_dressing(patient))
        return TREAT_DRESS;
    if (patient->bleed > .01f && inventory_count(medic, CLOTH) >= 2)
        return TREAT_PRESSURE;
    return -1;
}
static int aid_volunteer(int n) {
    Actor *m = &g.actors[n];
    if (m->squad || !conscious(m) || actor_inside(m) || m->target >= 0 || m->carrying >= 0 ||
        m->prisonTime > 0 || g.depth.actors[n].captive || population_hidden(n) ||
        m->faction == HOSTILE || m->faction == WILDLIFE || g.rep[m->faction] < -25)
        return 0;
    int owner = population_owned(n);
    return npc_role(m) == ROLE_MEDIC || (owner && g.population.groups[owner - 1].type == POP_PILGRIMS) ||
        ((m->kind == SETTLER || m->kind == RECRUIT || m->kind == TRADER || m->kind == CARAVAN) &&
         (g.seed + (unsigned)n * 17) % 4 == 0);
}
int field_aid(int n, float dt) {
    if (g.roadsideAid.actor == n + 1)
        return 1;
    if (!aid_volunteer(n))
        return 0;
    Actor *m = &g.actors[n];
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!aid_patient(a) || a->carriedBy >= 0 || fabsf(a->x - m->x) >= 16 || fabsf(a->y - m->y) >= 16 ||
            hostile(m, a) || (a->unconscious <= 0 && npc_role(m) != ROLE_MEDIC && a->blood > 35))
            continue;
        int type = aid_type(m, a);
        float reach = depth_distance(m, a);
        if (type < 0 || reach > 16 || !aid_safe_at(a, a->x, a->y) || !aid_safe_at(m, m->x, m->y))
            continue;
        if (reach > TALK_REACH * .92f)
            route_toward(m, a->x, a->y, dt);
        else
            begin_treatment(n, i, type, -1);
        return 1;
    }
    return 0;
}
static int recovery_bed_location(int bed, float *x, float *y) {
    if (bed > 0 && bed <= g.buildingCount && g.buildings[bed - 1].type == BED && depth_build_ready(bed - 1)) {
        *x = g.buildings[bed - 1].x;
        *y = g.buildings[bed - 1].y;
        return 1;
    }
    if (bed <= -1000) {
        int raw = -1000 - bed;
        return population_object(raw / 4, raw % 4 + 1, x, y) == 2;
    }
    return 0;
}
static int recovery_bed_free(const Actor *patient, int bed, float *x, float *y) {
    if (!recovery_bed_location(bed, x, y) || !can_walk(*x, *y) || !aid_safe_at(patient, *x, *y))
        return 0;
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (a != patient && a->active && !a->dead && !actor_inside(a) &&
            a->taskKind == 3 && distance(*x, *y, a->x, a->y) < 1.4f)
            return 0;
    }
    return 1;
}
static int nearest_recovery_bed(const Actor *a) {
    int best = 0;
    float reach = 26, x, y;
    for (int b = 1; b <= g.buildingCount; b++) {
        if (!recovery_bed_location(b, &x, &y))
            continue;
        float d = distance(a->x, a->y, x, y);
        if (d < reach && recovery_bed_free(a, b, &x, &y)) {
            reach = d;
            best = b;
        }
    }
    for (int k = 0; k < POP_GROUPS; k++) {
        const PopulationState *site = &g.population.state[k];
        if (fabsf(site->siteX - a->x) > 32 || fabsf(site->siteY - a->y) > 32)
            continue;
        for (int j = 0; j < site->beds; j++) {
            int bed = -1000 - k * 4 - j;
            if (!recovery_bed_location(bed, &x, &y))
                continue;
            float d = distance(a->x, a->y, x, y);
            if (d < reach && recovery_bed_free(a, bed, &x, &y)) {
                reach = d;
                best = bed;
            }
        }
    }
    return best;
}
void roadside_aid_finished(int mi, int pi) {
    Actor *m = &g.actors[mi], *a = &g.actors[pi];
    if (g.roadsideAid.actor && g.roadsideAid.actor != mi + 1)
        return;
    if (g.roadsideAid.actor)
        g.roadsideAid.patient = 0;
    if (a->unconscious <= 0 || a->carriedBy >= 0 || m->carrying >= 0 ||
        !aid_patient(a) || actor_inside(m) || wrand() % 3 != 0)
        return;
    int bed = nearest_recovery_bed(a);
    if (!bed)
        return;
    g.roadsideAid.actor = mi + 1;
    g.roadsideAid.patient = pi + 1;
    g.roadsideAid.bed = bed;
    g.roadsideAid.until = g.time + 32;
    m->carrying = pi;
    a->carriedBy = mi;
    log_warn("%s is carrying %s to a nearby bed.", m->name, a->name);
}
static void roadside_put_down(int mi, Actor *a) {
    Actor *m = &g.actors[mi];
    if (a && a->carriedBy == mi) {
        a->x = m->x;
        a->y = m->y;
        a->carriedBy = -1;
    }
    if (a && m->carrying == a - g.actors)
        m->carrying = -1;
    g.roadsideAid.patient = g.roadsideAid.bed = 0;
    if (!g.roadsideAid.generated)
        g.roadsideAid.actor = 0;
}
static void roadside_move(int mi, float dt) {
    Actor *m = &g.actors[mi];
    int pi = g.roadsideAid.patient - 1;
    Actor *a = pi >= 0 ? &g.actors[pi] : NULL;
    if (g.roadsideAid.bed) {
        float x, y;
        if (!a || a->dead || fatal_injury(a) || !conscious(m) || a->carriedBy != mi ||
            g.time >= g.roadsideAid.until || !aid_safe_at(m, m->x, m->y) ||
            !recovery_bed_free(a, g.roadsideAid.bed, &x, &y)) {
            roadside_put_down(mi, a);
            return;
        }
        if (distance(m->x, m->y, x, y) > .8f)
            route_toward(m, x, y, dt);
        else {
            int bed = g.roadsideAid.bed;
            roadside_put_down(mi, a);
            a->x = x;
            a->y = y;
            a->taskKind = 3;
            a->taskIndex = bed > 0 ? bed - 1 : bed;
            a->action = 120;
            log_warn("%s laid %s in a bed to recover.", m->name, a->name);
        }
        return;
    }
    if (a) {
        if (!aid_patient(a) || a->carriedBy >= 0 || !conscious(m) || g.time >= g.roadsideAid.until) {
            roadside_put_down(mi, a);
        } else if (m->taskKind != TASK_TREAT && aid_safe_at(a, a->x, a->y) && aid_safe_at(m, m->x, m->y)) {
            int type = aid_type(m, a);
            if (type < 0)
                roadside_put_down(mi, a);
            else if (depth_distance(m, a) > TALK_REACH * .92f)
                route_toward(m, a->x, a->y, dt);
            else
                begin_treatment(mi, pi, type, -1);
        }
        return;
    }
    if (!g.roadsideAid.generated) {
        g.roadsideAid.actor = 0;
        return;
    }
    if (m->squad) {
        g.roadsideAid.actor = g.roadsideAid.generated = 0;
        return;
    }
    int nearby = 0;
    for (int n = 0; n < g.actorCount; n++)
        if (g.actors[n].squad && !g.actors[n].dead &&
            distance(m->x, m->y, g.actors[n].x, g.actors[n].y) < 28) {
            nearby = 1;
            break;
        }
    if (!nearby) {
        m->active = 0;
        g.living.owned[mi] = -1;
        g.roadsideAid.actor = g.roadsideAid.generated = 0;
    } else if (conscious(m))
        route_toward(m, m->homeX, m->homeY, dt * .8f);
}
void roadside_aid_tick(float dt) {
    if (g.roadsideAid.actor) {
        roadside_move(g.roadsideAid.actor - 1, dt);
        return;
    }
    Actor *a = &g.actors[g.selected];
    if (g.roadsideAid.day == g.day || !aid_patient(a) || a->unconscious <= 0 ||
        a->carriedBy >= 0 || (a->woundFlags & WOUND_AID_CHECKED) || town_near(a->x, a->y, 12) >= 0 ||
        (!recovery_coma(a) && a->bleed < .15f) || !aid_safe_at(a, a->x, a->y))
        return;
    for (int n = 0; n < g.actorCount; n++) {
        Actor *m = &g.actors[n];
        if (fabsf(m->x - a->x) < 16 && fabsf(m->y - a->y) < 16 && aid_volunteer(n) &&
            !hostile(m, a) && depth_distance(m, a) < 16 && aid_type(m, a) >= 0)
            return;
    }
    a->woundFlags |= WOUND_AID_CHECKED;
    if (wrand() % 5 != 0)
        return;
    float phase = (wrand() % 16) * .3926991f;
    for (int k = 0; k < 16; k++) {
        float angle = phase + k * .3926991f, dx = cosf(angle), dy = sinf(angle);
        float x = a->x + dx * 16, y = a->y + dy * 16;
        float hx = a->x + dx * 36, hy = a->y + dy * 36;
        if ((fabsf(view_horizontal(x - a->x, y - a->y)) < 13 &&
             fabsf(view_vertical(x - a->x, y - a->y)) < 17) ||
            !can_walk(x, y) || !can_walk(hx, hy))
            continue;
        int clear = 1;
        for (int d = 2; d < 16 && clear; d += 2)
            clear = can_walk(a->x + dx * d, a->y + dy * d);
        if (!clear)
            continue;
        int n = -1;
        for (int j = 0; j < g.actorCount; j++)
            if (g.living.owned[j] < 0) {
                n = j;
                break;
            }
        static const char *names[] = {"Mira", "Tern", "Nadi", "Edda"};
        const char *name = names[wrand() % 4];
        if (n >= 0)
            init_actor(n, name, SETTLER, FREE, x, y, 12);
        else
            n = add_actor(name, SETTLER, FREE, x, y, 12);
        if (n < 0)
            return;
        g.living.owned[n] = 0;
        Actor *m = &g.actors[n];
        m->homeX = hx;
        m->homeY = hy;
        m->voice = ROLE_MEDIC;
        if (!inventory_count(m, a->race == 2 ? PARTS : MEDKIT))
            inventory_add(m, a->race == 2 ? PARTS : MEDKIT, 1);
        g.roadsideAid.actor = n + 1;
        g.roadsideAid.patient = g.selected + 1;
        g.roadsideAid.generated = 1;
        g.roadsideAid.until = g.time + 28;
        g.roadsideAid.day = g.day;
        return;
    }
}
void recovery_hint(char*out,int size){
 Actor*a=&g.actors[g.selected];out[0]=0;if(a->dead)return;
 if(a->taskKind==TASK_JAIL_CARE){snprintf(out,size,"JAIL CARE: staff are treating your wounds.");return;}
 if(a->prisonTime>0){snprintf(out,size,"IN CUSTODY: %ds left. Wounds recover under guard care.",(int)ceilf(a->prisonTime));return;}
 if(a->carriedBy>=0){snprintf(out,size,"BEING CARRIED by %s.",g.actors[a->carriedBy].name);return;}
 if(recovery_coma(a)){
     const char *hint = a->race == 2 ? "RECOVERY COMA: core restarting. Spare parts speed repair." :
         !(a->woundFlags & WOUND_CUTS) ? "RECOVERY COMA: vital injuries and blood are recovering." :
         g.squadCount > 1 ? "RECOVERY COMA: switch allies with L to dress the wounds." :
         "RECOVERY COMA: cuts must clot or be dressed before you can wake.";
     snprintf(out, size, "%s", hint);
     return;
 }
 if(a->bleed>.01f){snprintf(out,size,a->unconscious>0?"BLEEDING: switch to an ally for first aid.":"BLEEDING: Nearby Actions offers first aid.");return;}
 if(a->unconscious>0){snprintf(out,size,"KNOCKED OUT: recovering consciousness. Allies can carry you.");return;}
 if(a->health[HEAD]>60&&a->health[CHEST]>60&&a->fatigue<70)return;
 if(a->treatedMask){snprintf(out,size,"RECOVER: dressed wounds heal slowly on the trail. A bed is faster.");return;}
 int b=-1;float near=1e9f;for(int k=0;k<6;k++)if(g.trails.bed[k]&&(g.trails.seen&(1<<k))){int n=g.trails.bed[k]-1;float d=distance(a->x,a->y,g.buildings[n].x,g.buildings[n].y);if(d<near){near=d;b=n;}}
 if(b>=0){float dx=g.buildings[b].x-a->x,dy=g.buildings[b].y-a->y;snprintf(out,size,"RECOVER: known roadside shelter %.0fm %s",near,fabsf(dx)>fabsf(dy)?(dx<0?"W":"E"):(dy<0?"N":"S"));}
 else snprintf(out,size,"RECOVER: bandage, find a safe bed, then rest. Medics help nearby.");
}
