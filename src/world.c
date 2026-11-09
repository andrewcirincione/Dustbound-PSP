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
    if (a->kind == RAIDER && level(a, MELEE) <= 12)
        return;
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
        if (g.actors[n].squad)
            g.actors[n].gear[4] = gear_id(68, 0, 0);
    restock();
}
static Actor *road_vendor(void) {
    int n = ui.roadTrader - 1;
    if (!population_vendor(n) || !conscious(&g.actors[n]) ||
        depth_distance(&g.actors[g.selected], &g.actors[n]) > TALK_REACH)
        return NULL;
    return &g.actors[n];
}
static float sale_chance(const Actor *a, int slot, int faction) {
    int owner = a->stolen[slot];
    if (!owner || (faction == OUTLAWS && owner != faction))
        return 1;
    int value = itemDefs[a->pack[slot].item].price;
    return limit(.16f + level(a, STEALTH) * .003f + level(a, THEFT) * .005f +
                 (faction != owner ? .22f : 0) - fminf(.12f, value * .00012f), .05f, .96f);
}
float stolen_sale_chance(int town, int slot) {
    if (slot < 0 || slot >= PACK)
        return 0;
    const Actor *m = town == -1 ? road_vendor() : NULL;
    int faction = m ? m->faction : town >= 0 && town < TOWNS ? g.townOwner[town] : FREE;
    return sale_chance(&g.actors[g.selected], slot, faction);
}
int stolen_sale(int town, int slot) {
    Actor *a = &g.actors[g.selected];
    if (!a->stolen[slot])
        return 1;
    float chance = stolen_sale_chance(town, slot);
    gain(a, THEFT, 22);
    if (chance >= 1 || wrand() % 1000 < chance * 1000)
        return 1;
    const Actor *m = town == -1 ? road_vendor() : NULL;
    int faction = m ? m->faction : town >= 0 && town < TOWNS ? g.townOwner[town] : FREE;
    if (faction > FREE && faction < WILDLIFE) {
        crime_charge(a, faction, 40, CRIME_STOLEN_SALE);
        g.rep[faction] -= 8;
        log_warn("The trader recognized the stolen goods and reported you. Bounty +40.");
    } else
        log_warn("The trader recognized the stolen goods and refused the sale.");
    return 0;
}
int item_price(int town, int it, int selling) {
    if (it <= 0 || it >= ITEMS)
        return 0;
    if (town == -1 && road_vendor())
        return (int)fmaxf(1, itemDefs[it].price * (selling ? .65f : 1.12f));
    if (town < 0 || town >= TOWNS)
        return 0;
    float mod = 1 + regional_price(town,it) + (town==1&&g.depth.route==ROUTE_TRUCE?.12f:0) + (60 - g.prosperity[town]) * .004f + limit(g.rep[g.townOwner[town]]*.001f,-.15f,.1f)*(selling?.5f:-1);
    mod = limit(mod, .65f, 1.6f);
    return (int)fmaxf(1, itemDefs[it].price * (selling ? .75f : 1) * mod * market_factor(town,it) * politics_price(town,it));
}
int *market_stock(int town, int item) {
    return item < TOWN_STOCK_ITEMS ? &g.stock[town][item] : &g.field.stock[town][item - TOWN_STOCK_ITEMS];
}
int market_quantity(int town, int item) {
    if (town >= 0 && town < TOWNS)
        return *market_stock(town, item);
    Actor *trader = road_vendor();
    return trader ? inventory_count(trader, item) : 0;
}
static int shop_visible(int item) {
    int slot = itemDefs[item].slot;
    if (ui.tradeSmith && (slot < 0 || slot == 5))
        return 0;
    switch (ui.shopFilter) {
    case 1: return slot < 0;
    case 2: return slot == 0;
    case 3: return slot >= 1 && slot != 5;
    case 4: return slot == 5;
    case 5: return limb_item_grade(item) >= 0 || item == PARTS;
    }
    return 1;
}
static int shop_list(int index) {
    int town = ui.tradeTown, count = 0;
    if (town >= 0 && town < TOWNS) {
        /* Regional designs lead the weapon tab. */
        for (int step = 1; step < ITEMS; step++) {
            int it = ui.shopFilter == 2 ? BASE_ITEMS + step - 1 : step;
            if (it >= ITEMS) it -= ITEMS - 1;
            if (*market_stock(town, it) > 0 && shop_visible(it) && count++ == index)
                return it;
        }
    } else {
        Actor *m = road_vendor();
        if (m)
            for (int i = 0; i < PACK; i++) {
                int it = m->pack[i].item, seen = 0;
                if (!it || !shop_visible(it)) continue;
                for (int j = 0; j < i && !seen; j++)
                    seen = m->pack[j].item == it;
                if (!seen && count++ == index) return it;
            }
    }
    return index < 0 ? count : NONE;
}
int shop_item(int index) { return index < 0 ? NONE : shop_list(index); }
int shop_count(void) { return shop_list(-1); }
int road_trade(int item, int slot) {
    Actor *m = road_vendor(), *a = &g.actors[g.selected];
    if (!m || !conscious(a)) {
        log_msg("The trader has moved on.");
        return 0;
    }
    int selling = slot >= 0;
    Actor *from = selling ? a : m, *to = selling ? m : a;
    if (!selling)
        for (int j = 0; j < PACK; j++)
            if (m->pack[j].item == item && m->pack[j].qty) { slot = j; break; }
    if (slot < 0 || slot >= PACK || !from->pack[slot].item)
        return 0;
    item = from->pack[slot].item;
    int price = item_price(-1, item, selling);
    if (!selling && g.money < price) {
        log_msg("Not enough silver.");
        return 0;
    }
    Actor check = *to;
    if (!inventory_add(&check, item, 1)) {
        log_msg(selling ? "The trader has no room for that." : "Pack full.");
        return 0;
    }
    if (selling && !stolen_sale(-1, slot))
        return 0;
    inventory_add(to, item, 1);
    inventory_remove_slot(from, slot, 1);
    g.money += selling ? price : -price;
    trade_goodwill(m->faction, price);
    log_msg("%s %s for %d silver.", selling ? "Sold" : "Bought", itemDefs[item].name, price);
    return 1;
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
}
static void thin_ore(void) {
    for (int i = 0; i < g.nodeCount; i++) {
        Node *node = &g.nodes[i];
        if (node->type > NODE_COPPER)
            continue;
        int crowded = town_near(node->x, node->y, 16) >= 0;
        for (int j = 0; !crowded && j < i; j++) {
            const Node *other = &g.nodes[j];
            if (other->type <= NODE_COPPER && other->regen >= 0 &&
                fabsf(node->x - other->x) < 12 && fabsf(node->y - other->y) < 12)
                crowded = distance(node->x, node->y, other->x, other->y) < 12;
        }
        if (crowded || (i % 4 == 3)) {
            node->remaining = 0;
            node->regen = -1;
        }
    }
    /* Give each start an accessible seam beyond the buildings. */
    for (int i = 0; i < START_SCENARIOS; i++) {
        int t = startScenarios[i].town;
        float x = towns[t].x, y = towns[t].y;
        if (nearest_ore(x, y, 27, NODE_ORE) >= 0)
            continue;
        for (int j = 0; j < 16; j++) {
            float angle = j * .392699f + .40f;
            float ox = x + 21 * cosf(angle), oy = y + 21 * sinf(angle);
            int nearby = 0;
            for (int n = 0; n < g.nodeCount; n++)
                if (distance(ox, oy, g.nodes[n].x, g.nodes[n].y) < 1.3f) {
                    nearby = 1;
                    break;
                }
            if (!nearby && town_near(ox, oy, 16) < 0 && can_walk(ox, oy) &&
                can_walk(ox + .8f, oy) && can_walk(ox, oy + .8f)) {
                add_node(ox, oy, NODE_ORE, FREE, NONE, 0);
                break;
            }
        }
    }
}
/* Open approaches keep portable beds, lockups and camp yards out of other buildings. */
static int clear_structure_site(float x, float y, float spacing) {
    if (!can_walk(x, y) || !can_walk(x + 1.45f, y + .65f))
        return 0;
    for (int i = 0; i < g.buildingCount; i++) {
        const Building *b = &g.buildings[i];
        if (g.field.rolls[i] >= 0 && fabsf(x - b->x) < spacing &&
            fabsf(y - b->y) < spacing && distance(x, y, b->x, b->y) < spacing)
            return 0;
    }
    return 1;
}

int portable_bedroll(int building) {
    return building >= 0 && building < g.buildingCount ? g.field.rolls[building] : 0;
}

int deploy_bedroll(Actor *a, int slot) {
    if (!conscious(a) || actor_inside(a) || a->prisonTime > 0 || a->carrying >= 0 ||
        g.depth.actors[a - g.actors].captive || a->attackPhase == 1 || a->attackPhase == 2) {
        log_msg("Find clear ground to lay out the bedroll.");
        return 0;
    }
    for (int i = 0; i < 8; i++) {
        float angle = i * .785398f;
        float x = a->x + 1.4f * cosf(angle), y = a->y + 1.4f * sinf(angle);
        if (!clear_structure_site(x, y, 1.7f))
            continue;
        int b = 0;
        while (b < g.buildingCount && g.field.rolls[b] >= 0)
            b++;
        if (b == g.buildingCount)
            b = add_build(x, y, BED, FREE, a - g.actors);
        if (b < 0)
            break;
        int owner = a->stolen[slot];
        if (!inventory_remove_slot(a, slot, 1))
            return 0;
        g.buildings[b] = (Building){.x = x, .y = y, .type = BED, .owner = a - g.actors};
        g.depth.buildings[b] = (DepthBuilding){.condition = 100};
        g.field.rolls[b] = owner + 1;
        a->taskKind = 0;
        depth_open(SCREEN_WORLD);
        log_msg("Bedroll laid out.");
        return 1;
    }
    log_msg("There is no room to lay out the bedroll here.");
    return 0;
}

void pack_bedroll(int b) {
    Actor *a = &g.actors[g.selected];
    if (portable_bedroll(b) <= 0)
        return;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].active && !g.actors[i].dead && actor_in_bed(&g.actors[i]) &&
            g.actors[i].taskIndex == b) {
            log_msg("Someone is resting on the bedroll.");
            return;
        }
    if (!inventory_add_owned(a, BEDROLL_ITEM, 1, g.field.rolls[b] - 1)) {
        log_msg("Pack full.");
        return;
    }
    g.field.rolls[b] = -1;
    g.buildings[b].owner = -1;
    g.depth.buildings[b].condition = 0;
    log_msg("Packed up the bedroll.");
}

static const int campTowns[SLAVE_CAMPS] = {8, 17, 16};
static const int campFactions[SLAVE_CAMPS] = {OUTLAWS, SHOGUN, UNION};
int slave_camp_at(int building) {
    if (building >= 0)
        for (int i = 0; i < SLAVE_CAMPS; i++)
            if (g.field.camp[i] == building + 1)
                return i;
    return -1;
}
int slave_in_camp(const Actor *a) {
    int n = (int)(a - g.actors);
    return g.field.laborCamp[n] && g.depth.actors[n].captive == 1;
}

static int place_custody_site(int town, int type, int faction, float radius) {
    for (int ring = 0; ring < 4; ring++)
        for (int i = 0; i < 16; i++) {
            float angle = (i + town * 3) * .392699f;
            float x = towns[town].x + cosf(angle) * (radius + ring * 3),
                  y = towns[town].y + sinf(angle) * (radius + ring * 3);
            if ((type == SLAVE_YARD && town_near(x, y, 18) >= 0) ||
                !clear_structure_site(x, y, type == JAIL ? 3.6f : 5))
                continue;
            int b = add_build(x, y, type, faction, -1);
            if (b >= 0)
                g.depth.buildings[b].condition = 100;
            return b;
        }
    return -1;
}

static void cell_place(int n, int b) {
    Actor *a = &g.actors[n];
    DepthActor *z = &g.depth.actors[n];
    a->x = g.buildings[b].x + 1.45f;
    a->y = g.buildings[b].y + .65f;
    z->rx = 2.6f + .55f * (n % 3);
    z->ry = 2.3f;
    z->room = b + 1;
    if (n == g.selected) {
        g.interiorBuilding = b + 1;
        g.interiorActor = n;
        g.interiorX = z->rx;
        g.interiorY = z->ry;
        z->room = 0;
    }
}

static int town_lockup(int town) {
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type == TOWER && b->owner < 0 && town_near(b->x, b->y, 15) == town &&
            can_walk(b->x + 1.45f, b->y + .65f)) {
            b->type = JAIL;
            return i;
        }
    }
    return place_custody_site(town, JAIL, g.townOwner[town], 11);
}

void field_init(void) {
    if (g.field.version)
        return;
    g.field.version = 1;
    thin_ore();
    for (int t = 0; t < TOWNS; t++) {
        g.field.stock[t][0] = 2;
        if (g.townOwner[t] != HOSTILE && t != 14)
            g.field.jail[t] = town_lockup(t) + 1;
    }
    for (int c = 0; c < SLAVE_CAMPS; c++) {
        int b = place_custody_site(campTowns[c], SLAVE_YARD, campFactions[c], 25);
        g.field.camp[c] = b + 1;
        if (b >= 0) {
            Building *camp = &g.buildings[b];
            int guard = 0;
            while (guard < g.actorCount && g.living.owned[guard] >= 0)
                guard++;
            if (guard < g.actorCount) {
                init_actor(guard, "Camp overseer", GUARD, camp->faction, camp->x + 2, camp->y + 1, 24);
                g.living.owned[guard] = g.living.anger[guard] = 0;
                g.living.command[guard] = -1;
            } else {
                guard = add_actor("Camp overseer", GUARD, camp->faction, camp->x + 2, camp->y + 1, 24);
            }
            if (guard >= 0)
                g.actors[guard].homeTown = -1;
        }
    }
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (a->squad && !a->dead && a->prisonTime > 0 && a->prisonTown >= 0 && g.field.jail[a->prisonTown])
            cell_place(n, g.field.jail[a->prisonTown] - 1);
    }
}

int field_validate(const Game *save) {
    if (save->field.version < 0 || save->field.version > 1)
        return 0;
    for (int t = 0; t < TOWNS; t++) {
        int jail = save->field.jail[t];
        if (jail < 0 || jail > save->buildingCount || (jail && save->buildings[jail - 1].type != JAIL))
            return 0;
        for (int j = 0; j < ITEMS - TOWN_STOCK_ITEMS; j++)
            if (save->field.stock[t][j] < 0)
                return 0;
    }
    for (int b = 0; b < save->buildingCount; b++)
        if (save->field.rolls[b] < -1 || save->field.rolls[b] > FACTIONS ||
            (save->field.rolls[b] && save->buildings[b].type != BED))
            return 0;
    for (int n = 0; n < save->actorCount; n++) {
        int b = save->field.intake[n];
        if (b < 0 || b > save->buildingCount ||
            (b && save->buildings[b - 1].type != JAIL && save->buildings[b - 1].type != SLAVE_YARD) ||
            save->field.laborCamp[n] > SLAVE_CAMPS ||
            (save->field.laborCamp[n] && !save->field.camp[save->field.laborCamp[n] - 1]) ||
            save->field.playingDead[n] > 1 ||
            !isfinite(save->field.cellOpen[n]) || save->field.cellOpen[n] < 0)
            return 0;
    }
    for (int c = 0; c < SLAVE_CAMPS; c++) {
        int b = save->field.camp[c];
        if (b < 0 || b > save->buildingCount || (b && save->buildings[b - 1].type != SLAVE_YARD))
            return 0;
        for (int s = 0; s < SEIZED_STACKS; s++) {
            const Stack *v = &save->field.seized[c][s];
            if (v->item < 0 || v->item >= ITEMS || v->qty < 0 || v->qty > 999 ||
                save->field.seizedOwner[c][s] >= FACTIONS)
                return 0;
        }
    }
    return 1;
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
    faction_goodwill(g.townOwner[c->town], 8);
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
static const char *faction_conversation(const Actor *a) {
    static const char *lines[FACTIONS][3] = {
        {"I work for myself. So do most people out here.", "We stick together until the road splits.",
         "A warm fire and someone who keeps watch. That's as close as I get to a flag."},
        {"The pumps kept us alive. We take turns keeping them running.", "Our wardens used to pull shifts at the wells like everybody else.",
         "Pay the road levy and we'll keep the wells open. That's the bargain."},
        {"My mother stripped iron off dead trains. I sell what her hands taught me to find.",
         "Around here, a promise is worth about as much as the scrap you bring home.",
         "We share a wagon when the road gets rough. After that, everyone keeps their own books."},
        {"We take what the wastes give us. Sometimes the wastes give us travelers.",
         "You want our history? Start with the bones outside the gate.", "We feed our own first."},
        {"My grandfather swore his sword to the old commander. The debt passed to me.",
         "The rice goes to the keep before it comes to our tables. That's how the garrison eats.",
         "A soldier's children learn the names of the dead before the names of the roads."},
        {"The fever took my brothers. The hospice took me in.", "We grow the herbs behind the abbey. It takes all summer to fill a shelf.",
         "Our elders argue over who gets the last medicine. I don't envy them."},
        {"I moved to the Mesa because nobody here asked who my father was.",
         "Don't ask where a crate came from. Pay what you agreed and you'll get along here.",
         "We used to run supplies past the watch. Now the watch buys from us."},
        {"", "", ""}
    };
    return lines[a->faction][(g.seed + (unsigned)(a - g.actors) * 17 + g.day) % 3];
}
const char *dialogue_line(int n, int page) {
    if (page == DIALOG_SUMMONS || page == DIALOG_BOUNTY) {
        static char line[200];
        char offenses[116];
        const Actor *suspect = &g.actors[g.selected];
        int faction = g.actors[n].faction;
        crime_description(suspect, faction, offenses, sizeof(offenses));
        snprintf(line, sizeof(line), page == DIALOG_SUMMONS
                 ? "Hold up, %s. You're wanted for %s. Pay %d silver or come with me."
                 : "%s has you down for %s. The outstanding fine is %d silver.",
                 page == DIALOG_SUMMONS ? suspect->name : factionNames[faction],
                 offenses, wanted_amount(suspect, faction));
        return line;
    }
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
        return "Pay bail, wait out the sentence, or pick the lock. The watch can lock it again.";
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
        for (int k = 0; k < POP_GROUPS; k++) {
            const WorldEvent *e = &g.population.groups[k];
            if ((e->type == POP_RAIDERS || e->type == POP_AMBUSH) && e->count > 0 &&
                distance(a->x, a->y, e->x, e->y) < 65)
                return "I saw bandits on the road nearby. They looked hungry enough to fight for scraps.";
        }
        return npc_rumor(a);
    }
    if (page == 2)
        return town_custom(town_near(a->x,a->y,14));
    if (page == 3)
        return faction_conversation(a);
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
        return "I've got arms and armor for sale. If you want to work the forge yourself, bring your own materials.";
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
static int sight_axis(float from, float delta, float center, float half, float *lo, float *hi) {
    if (fabsf(delta) < .001f)
        return fabsf(from - center) < half;
    float a = (center - half - from) / delta, b = (center + half - from) / delta;
    *lo = fmaxf(*lo, fminf(a, b));
    *hi = fminf(*hi, fmaxf(a, b));
    return *lo < *hi;
}
float npc_sight_range(const Actor *a) {
    float reach = actor_inside(a) ? 8 : a->kind == BEAST ? 4.5f :
        weapon_ranged(a->gear[0]) ? itemDefs[a->gear[0]].range + 1 : 6;
    return !actor_inside(a) && (g.weather == 1 || g.weather == 3) ? reach * .75f : reach;
}
int npc_sees(const Actor *observer, const Actor *a, float reach) {
    if (!conscious(observer) || actor_inside(observer) != actor_inside(a))
        return 0;
    float dx = depth_x(a) - depth_x(observer), dy = depth_y(a) - depth_y(observer);
    float d = sqrtf(dx * dx + dy * dy);
    int room = actor_inside(a);
    int shop_watch = room && g.buildings[room - 1].type == SHOP && npc_role(observer) == ROLE_MERCHANT;
    if (a->sneaking) {
        if (shop_watch && d < 2.65f)
            return 1;
        reach *= fmaxf(.14f, (shop_watch ? .90f : .66f) - level(a, STEALTH) * .004f - gear_bonus(a, 2) * .004f);
        if (d > 1.1f && (observer->facing ? dx - dy < -.45f : dx - dy > .45f))
            reach *= shop_watch ? .72f : .38f;
    }
    if (d > reach)
        return 0;
    if (!actor_inside(a))
        for (int j = 0; j < g.buildingCount; j++) {
            const Building *b = &g.buildings[j];
            if (!building_has_interior(b->type) || fabsf(b->x - observer->x) > reach + 1.5f ||
                fabsf(b->y - observer->y) > reach + 1.3f)
                continue;
            float lo = .10f, hi = .92f;
            if (sight_axis(observer->x, dx, b->x, 1.45f, &lo, &hi) &&
                sight_axis(observer->y, dy, b->y, 1.28f, &lo, &hi))
                return 0;
        }
    return 1;
}
int theft_witnesses(int n) {
    int count=0; Actor *a=&g.actors[g.selected];
    for(int i=0;i<g.actorCount;i++) {
        Actor *b=&g.actors[i];
        if(i!=n && !b->squad && b->kind!=BEAST && b->active && conscious(b) && npc_sees(b, a, 7)) count++;
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
        crime_charge(a,b->faction,80,CRIME_PICKPOCKET);g.rep[b->faction]-=18;b->speech=19;b->speechTime=5;
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
        float d = distance(a->x, a->y, guard->x, guard->y);
        if (d < reach) {
            found = i;
            reach = d;
        }
    }
    return found;
}
int jail_bail_cost(const Actor *a) { return wanted_amount(a, jail_faction(a)); }
static void jail_release(Actor *a) {
    g.field.cellOpen[a - g.actors] = 0;
    a->prisonTime = 0;
    a->prisonTown = -1;
    a->taskKind = 0;
    a->target = -1;
}
int jail_cell_open(const Actor *a) {
    return a->prisonTime > 0 && g.field.cellOpen[a - g.actors] > 0;
}
void jail_exit(Actor *a) {
    if (!jail_cell_open(a))
        return;
    int f = jail_faction(a);
    crime_charge(a, f, (int)fmaxf(60, SERIOUS_BOUNTY - wanted_amount(a, f)), CRIME_ESCAPE);
    jail_release(a);
    log_warn("%s escaped from jail. The watch is looking for them.", a->name);
}
int jail_bail(int n) {
    Actor *a = &g.actors[n];
    if (!conscious(a) || a->prisonTime <= 0 || a->taskKind == TASK_JAIL_CARE)
        return 0;
    if (jail_bail_cost(a) && !crime_pay(a, jail_faction(a)))
        return 0;
    jail_release(a);
    log_msg("%s was released on bail.", a->name);
    return 1;
}
float jail_pick_duration(const Actor *a, int tools) {
    return (tools ? 8.0f : 13.0f) - level(a, LOCKPICK) * (tools ? .045f : .065f);
}
int jail_pick_start(int n) {
    Actor *a = &g.actors[n];
    if (!conscious(a) || a->prisonTime <= 0 || a->taskKind == TASK_JAIL_CARE || jail_cell_open(a))
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
    float *open = &g.field.cellOpen[a - g.actors];
    if (*open > 0 && depth_y(a) < 3.7f && jail_guard(a, 6) >= 0) {
        *open = fmaxf(0, *open - dt);
        if (!*open)
            log_warn("The watch has locked the cell again.");
    }
    if (a->taskKind == TASK_CELL_LOCK) {
        if (!conscious(a)) {
            a->taskKind = 0;
        } else if ((a->action -= dt) <= 0) {
            int tools = a->taskIndex && inventory_count(a, TOOLS);
            float chance = 5 + level(a, LOCKPICK) * .90f + (tools ? 12 : 0);
            a->taskKind = 0;
            gain(a, LOCKPICK, 42);
            if (wrand() % 100 < fminf(97, chance)) {
                *open = 5;
                log_warn("Cell unlocked. Get to the exit.");
                if (!actor_inside(a)) {
                    jail_exit(a);
                    return;
                }
            } else
                log_msg("The cell lock held.");
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
/* The carrier owns movement until the prisoner reaches an actual intake site. */
static void begin_intake(int n, int guard, int building) {
    Actor *a = &g.actors[n], *b = &g.actors[guard];
    if (a->carrying >= 0) put_down(a);
    b->carrying = n;
    a->carriedBy = guard;
    a->x = b->x;
    a->y = b->y;
    g.field.intake[n] = building + 1;
    g.field.playingDead[n] = 0;
    g.society.queueCount[n] = 0;
    society_rescue_cancel(n);
    a->taskKind = a->attackPhase = a->autoattack = b->attackPhase = 0;
    a->target = b->target = g.living.command[n] = -1;
    log_warn("%s is carrying %s to %s.", b->name, a->name,
             g.buildings[building].type == JAIL ? "jail" : "a work camp");
    jail_medical(a, 0);
}
int watch_surrender(int guard) {
    Actor *a = &g.actors[g.selected], *b = &g.actors[guard];
    int town = watch_town(b);
    if (town < 0 || !conscious(a) || !conscious(b) || b->carrying >= 0 || depth_distance(a, b) >= 2.1f ||
        !wanted_amount(a, b->faction))
        return 0;
    begin_intake(g.selected, guard, g.field.jail[town] - 1);
    return 1;
}
static int camp_has_space(int c, const Actor *a) {
    int need = g.depth.actors[a - g.actors].layer != NONE;
    for (int i = 0; i < PACK; i++) need += a->pack[i].item != NONE;
    for (int i = 0; i < GEARS; i++) need += a->gear[i] != NONE;
    for (int s = 0; s < SEIZED_STACKS; s++) need -= !g.field.seized[c][s].item;
    return need <= 0;
}
static void intake_slave(int n, int camp, int faction) {
    Actor *a = &g.actors[n];
    DepthActor *z = &g.depth.actors[n];
    int slot = 0;
    for (int i = 0; i < PACK + GEARS + 1; i++) {
        int gear = i - PACK;
        Stack stack = gear < 0 ? a->pack[i] :
            (Stack){gear < GEARS ? a->gear[gear] : z->layer, 1};
        int owner = gear < 0 ? a->stolen[i] :
            gear < GEARS ? a->gearStolen[gear] : z->layerOwner;
        if (!stack.item)
            continue;
        while (g.field.seized[camp][slot].item)
            slot++;
        g.field.seized[camp][slot] = stack;
        g.field.seizedOwner[camp][slot++] = owner;
    }
    memset(a->pack, 0, sizeof(a->pack));
    memset(a->stolen, 0, sizeof(a->stolen));
    memset(a->gear, 0, sizeof(a->gear));
    memset(a->gearStolen, 0, sizeof(a->gearStolen));
    z->layer = z->layerOwner = z->queued = 0;
    a->gear[1] = SLAVE_GARB;
    a->job = JOB_HOLD;
    a->action = 0;
    z->captive = 1;
    g.field.laborCamp[n] = camp + 1;
    g.field.escaped[n] &= ~(1u << faction);
    g.society.captorFaction[n] = faction;
    log_warn("%s was shackled. The overseer put their belongings in the camp chest.", a->name);
}
static void intake_jail(int n, int building, Actor *guard) {
    Actor *a = &g.actors[n];
    int seized = 0;
    for (int i = 0; i < PACK; i++)
        if (a->stolen[i]) {
            seized += a->pack[i].qty;
            a->pack[i] = (Stack){0};
            a->stolen[i] = 0;
        }
    for (int i = 0; i < GEARS; i++)
        if (a->gearStolen[i]) {
            seized++;
            a->gear[i] = a->gearStolen[i] = 0;
        }
    DepthActor *z = &g.depth.actors[n];
    if (z->layerOwner) {
        seized++;
        z->layer = z->layerOwner = 0;
    }
    a->prisonTown = guard->homeTown;
    a->prisonTime = 90 + wanted_amount(a, guard->faction) * .25f;
    cell_place(n, building);
    DepthActor *watch = &g.depth.actors[guard - g.actors];
    watch->room = building + 1;
    watch->rx = 3.5f;
    watch->ry = 6.15f;
    log_warn("%s locked in a cell at %s.", a->name, towns[a->prisonTown].name);
    if (seized) log_msg("The watch confiscated %d stolen goods.", seized);
    jail_medical(a, 0);
}
int slave_pick_start(int n) {
    Actor *a = &g.actors[n];
    if (!conscious(a) || !slave_in_camp(a) || a->taskKind == TASK_JAIL_CARE)
        return 0;
    if (a->taskKind != TASK_CELL_LOCK) {
        a->taskKind = TASK_CELL_LOCK;
        a->action = jail_pick_duration(a, inventory_count(a, TOOLS) > 0);
        log_msg("Picking the shackles.");
    }
    if (n == g.selected) depth_open(SCREEN_WORLD);
    return 1;
}
static void labor_tick(int n, float dt) {
    Actor *a = &g.actors[n];
    int camp = g.field.laborCamp[n] - 1;
    const Building *b = &g.buildings[g.field.camp[camp] - 1];
    jail_medical(a, dt);
    a->hunger = fmaxf(a->hunger, 28);
    if (!conscious(a) || a->taskKind == TASK_JAIL_CARE)
        return;
    if (a->taskKind == TASK_CELL_LOCK) {
        if ((a->action -= dt) <= 0) {
            a->taskKind = 0;
            gain(a, LOCKPICK, 30);
            if (wrand() % 100 < fminf(96, 32 + level(a, LOCKPICK) * .7f)) {
                g.depth.actors[n].captive = 0;
                g.field.laborCamp[n] = 0;
                g.field.escaped[n] |= 1u << b->faction;
                log_warn("%s slipped the shackles.", a->name);
            } else {
                log_msg("The shackles held.");
            }
        }
        return;
    }
    float angle = (floorf(g.time / 13) + n) * 1.570796f;
    route_toward(a, b->x + cosf(angle) * 3, b->y + sinf(angle) * 3, dt * .65f);
    gain(a, LABOUR, dt * .9f);
    gain(a, STRENGTH, dt * .35f);
}
static int nearby_slave_camp(const Actor *guard, const Actor *patient) {
    if (guard->kind != GUARD && guard->kind != RAIDER)
        return -1;
    for (int c = 0; c < SLAVE_CAMPS; c++) {
        int b = g.field.camp[c] - 1;
        if (b >= 0 && g.buildings[b].faction == guard->faction &&
            distance(patient->x, patient->y, g.buildings[b].x, g.buildings[b].y) < 38 &&
            camp_has_space(c, patient))
            return b;
    }
    return -1;
}
void custody_tick(float dt) {
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (!a->active || !a->squad)
            continue;
        if (a->prisonTime <= 0)
            g.field.cellOpen[n] = fmaxf(0, g.field.cellOpen[n] - dt);
        int b = g.field.intake[n] - 1;
        if (b >= 0) {
            int guard = a->carriedBy;
            Actor *carrier = guard >= 0 ? &g.actors[guard] : NULL;
            if (a->dead || fatal_injury(a) || !carrier || !conscious(carrier) ||
                carrier->carrying != n) {
                if (carrier && carrier->carrying == n) carrier->carrying = -1;
                a->carriedBy = -1;
                if (a->taskKind == TASK_JAIL_CARE) a->taskKind = 0;
                g.field.intake[n] = 0;
                continue;
            }
            jail_medical(a, dt);
            if (actor_inside(carrier)) {
                float dx = 3.5f - depth_x(carrier), dy = 6.3f - depth_y(carrier);
                if (dx * dx + dy * dy > .32f) {
                    depth_move_room(carrier, dx, dy, dt);
                    continue;
                }
                actor_leave_room(guard);
                actor_leave_room(n);
            }
            Building *dest = &g.buildings[b];
            int camp = slave_camp_at(b);
            float x = dest->x + 1.45f, y = dest->y + .65f;
            if (distance(carrier->x, carrier->y, x, y) > .8f) {
                route_toward(carrier, x, y, dt * 1.2f);
                continue;
            }
            carrier->carrying = -1;
            a->carriedBy = -1;
            a->x = carrier->x;
            a->y = carrier->y;
            g.field.intake[n] = 0;
            if (camp < 0)
                intake_jail(n, b, carrier);
            else if (camp_has_space(camp, a))
                intake_slave(n, camp, dest->faction);
            continue;
        }
        if (a->dead || fatal_injury(a))
            continue;
        if (slave_in_camp(a)) {
            labor_tick(n, dt);
            continue;
        }
        if (a->unconscious <= 0 || a->carriedBy >= 0 || a->prisonTime > 0 ||
            g.depth.actors[n].captive)
            continue;
        for (int j = 0; j < g.actorCount; j++) {
            Actor *guard = &g.actors[j];
            if (guard->squad || !conscious(guard) || guard->carrying >= 0 ||
                fabsf(guard->x - a->x) > 2.3f || fabsf(guard->y - a->y) > 2.3f ||
                depth_distance(a, guard) > 2.3f)
                continue;
            int town = watch_town(guard);
            int jail = town >= 0 && town < TOWNS ? g.field.jail[town] - 1 : -1;
            if (jail >= 0 && wanted_amount(a, guard->faction) >= SERIOUS_BOUNTY) {
                begin_intake(n, j, jail);
                break;
            }
            int camp = actor_inside(a) ? -1 : nearby_slave_camp(guard, a);
            if (camp >= 0 && hostile(guard, a)) {
                begin_intake(n, j, camp);
                break;
            }
        }
    }
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
        int sheltered = actor_inside(a) || a->taskKind == 3 || town_near(a->x, a->y, 8) >= 0;
        if ((g.weather == 3 || g.weather == 4) && !sheltered)
            a->exposure = fminf(100, a->exposure + dt * fmaxf(0, .20f - gear_bonus(a, 4) * .002f));
        else
            a->exposure = fmaxf(0, a->exposure - dt * .45f);
        if (a->exposure > 65) {
            a->treatedMask &= ~(1 << CHEST);
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
        (!recovery_coma(a) && a->bleed < .15f && a->health[HEAD] > 0 && a->health[CHEST] > 0) || !aid_safe_at(a, a->x, a->y))
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
void recovery_hint(char *out, int size) {
    Actor *a = &g.actors[g.selected];
    out[0] = 0;
    if (a->dead) return;
    if (a->carriedBy >= 0)
        snprintf(out, size, "BEING CARRIED BY %s", g.actors[a->carriedBy].name);
    else if (a->taskKind == TASK_JAIL_CARE)
        snprintf(out, size, "RECEIVING MEDICAL CARE");
    else if (jail_cell_open(a))
        snprintf(out, size, "CELL UNLOCKED");
    else if (a->prisonTime > 0)
        snprintf(out, size, "IN CUSTODY / %ds LEFT", (int)ceilf(a->prisonTime));
    else if (slave_in_camp(a))
        snprintf(out, size, a->taskKind == TASK_CELL_LOCK ? "PICKING SHACKLES" : "SHACKLED / FORCED LABOUR");
    else if (a->bleed > .01f)
        snprintf(out, size, "BLEEDING / %.0f BLOOD", a->blood);
    else if (recovery_coma(a))
        snprintf(out, size, "RECOVERY COMA");
    else if (a->unconscious > 0)
        snprintf(out, size, "RECOVERING CONSCIOUSNESS");
    else if (playing_dead(a))
        snprintf(out, size, "PLAYING DEAD");
    else if (g.field.escaped[g.selected])
        snprintf(out, size, "ESCAPED SLAVE");
}
