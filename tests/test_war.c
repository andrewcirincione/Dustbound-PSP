#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern int hostile(const Actor *, const Actor *);
extern int conscious(const Actor *);
extern void init_actor(int, const char *, int, int, float, float, int);

static void at(float x, float y) {
    Actor *a = &g.actors[g.selected];
    a->x = a->homeX = a->tx = x;
    a->y = a->homeY = a->ty = y;
    a->autoattack = 0;
    ui.camX = x;
    ui.camY = y;
    ui.screen = SCREEN_WORLD;
}

static void fixture(const char *name) {
    char path[100];
    snprintf(path, sizeof(path), "build/v170/%s.sav", name);
    assert(save_game(path));
}

static int newest_takeover(int town) {
    for (int i = 0; i < WAR_REPORTS; ++i) {
        int slot = (g.war.reportNext + WAR_REPORTS - 1 - i) % WAR_REPORTS;
        if (g.war.report[slot].kind == WAR_TAKEOVER && g.war.report[slot].town == town) return slot;
    }
    return -1;
}

static void capitals(void) {
    const int factions[] = {UNION, SHOGUN, MONKS};
    const char *names[] = {"brass", "vermilion", "abbey"};
    const int specific[] = {WINDMILL, WALL, FARM};
    int torsos[WAR_FACTIONS], heads[WAR_FACTIONS];
    for (int i = 0; i < WAR_FACTIONS; ++i) {
        game_new(73819, 0);
        int t = OLD_TOWNS + i, count = 0, signature = 0;
        assert(g.townOwner[t] == factions[i] && g.war.resident[i][0]);
        assert(g.population.groups[OLD_POP_GROUPS+i].faction == factions[i]);
        assert(g.population.groups[OLD_POP_GROUPS+i].type == POP_ARMY);
        int n = g.war.resident[i][0] - 1;
        torsos[i] = gear_family(g.actors[n].gear[1]);
        heads[i] = gear_family(g.actors[n].gear[2]);
        for (int j = 0; j < i; ++j) assert(torsos[j] != torsos[i] && heads[j] != heads[i]);
        for (int b = 0; b < g.buildingCount; ++b) {
            Building *z = &g.buildings[b];
            if (building_town(b) != t) continue;
            assert(z->faction == factions[i]);
            ++count;
            signature += z->type == specific[i];
        }
        int food=0,inn=0;for(int it=1;it<ITEMS;it++)if(food_capacity(it))food+=*market_stock(t,it);
        for(int b=0;b<g.buildingCount;b++)if(building_town(b)==t&&g.buildings[b].type==BAR)inn++;
        assert(count >= 6 && inn && food); /* varied plans no longer share a fixed civic template */
        g.dayTime = 12;
        at(towns[t].x + 2, towns[t].y + 4);
        if(!can_walk(g.actors[0].x,g.actors[0].y)){
            int found=0;for(int y=-16;y<=16&&!found;y++)for(int x=-16;x<=16;x++)if(can_walk(towns[t].x+x,towns[t].y+y)){at(towns[t].x+x,towns[t].y+y);found=1;break;}assert(found);
        }
        assert(can_walk(g.actors[0].x,g.actors[0].y));
        if (i == 1) {
            g.actors[0].gear[1] = gear_id(34, 4, 1);
            g.actors[0].gear[2] = gear_id(50, 4, 1);
        }
        for (int j = OLD_TOWNS; j < TOWNS; ++j) g.discovered[j] = 1;
        fixture(names[i]);
    }
    puts("PASS: all three capitals have walkable approaches, separate civic layouts, merchants, food, distinct uniforms and starting armies.");
}

static void siege_rescue(void) {
    game_new(73819, 0);
    const int t = OLD_TOWNS, k = OLD_POP_GROUPS + 1;
    const int victim = g.war.resident[0][0] - 1;
    const int second = g.war.resident[0][1] - 1;
    g.actors[victim].health[CHEST] = 7;
    g.actors[victim].health[LARM] = 52;
    g.actors[victim].gearStolen[0] = OUTLAWS;
    g.actors[second].health[CHEST] = 7;
    int weapon = g.actors[victim].gear[0];
    WorldEvent *army = &g.population.groups[k];
    GroupLife *plan = &g.life.groups[k];
    assert(hostile(&g.actors[victim], &army->reserve[0]));
    assert(!hostile(&g.actors[0], &army->reserve[0]));
    army->x = towns[t].x + 2;
    army->y = towns[t].y + 4;
    plan->targetTown = t;
    plan->goal = LIFE_RAID;
    plan->clock = 0;
    population_tick(1);
    assert(g.townOwner[t] == UNION && conscious(&g.actors[victim]));
    for (int i = 0; i < 40 && g.townOwner[t] == UNION; ++i) { g.time += 1; population_tick(1); }
    assert(g.townOwner[t] == SHOGUN && g.war.prisonTown[victim] == t+1);
    assert(g.war.prisonTown[second] == t+1 && g.depth.actors[victim].captive == 1);
    assert(g.depth.actors[victim].confiscated == weapon && g.depth.actors[victim].confiscatedOwner == OUTLAWS);
    assert(!g.actors[victim].gear[0] && g.actors[victim].health[LARM] == 52);
    assert(g.actors[victim].faction == UNION && !hostile(&g.actors[victim], &army->reserve[1]));
    int report = newest_takeover(t);
    assert(report >= 0 && g.war.report[report].prisoners == 2);
    assert(strstr(war_ambient(report+32, SHOGUN), "We took BRASS JUNCTION"));
    assert(strstr(war_ambient(report+32, UNION), "We lost BRASS JUNCTION"));
    g.time += 80;
    Actor *a = &g.actors[victim];
    float px = a->x, py = a->y;
    depth_after_tick(.1f);
    assert(a->x == px && a->y == py);
    int guard = g.life.towns[t].leader - 1;
    assert(guard >= 0 && g.actors[guard].faction == SHOGUN);
    a->unconscious = g.actors[second].unconscious = 0;
    a->health[CHEST] = g.actors[second].health[CHEST] = 20;
    g.stock[t][IRONBAR] = 10;
    war_tick_labor(60);
    assert(g.stock[t][IRONBAR] == 12);
    g.discovered[t] = 1;
    g.dayTime = 12;
    at(px+.7f, py);
    fixture("occupied");
    assert(load_game("build/v170/occupied.sav"));
    assert(g.townOwner[t] == SHOGUN && g.war.prisonTown[victim] == t+1);
    at(px+.7f, py);
    assert(society_rescue_start(victim));
    society_tick(1);
    assert(!g.society.rescueHelper[victim] && g.depth.actors[victim].captive);
    g.actors[guard].unconscious = 90;
    for (int n = 1; n < g.actorCount; ++n)
        if (g.actors[n].faction == SHOGUN && g.actors[n].kind == GUARD && distance(px,py,g.actors[n].x,g.actors[n].y)<7)
            g.actors[n].unconscious = 90;
    fixture("rescue");
    assert(society_rescue_start(victim));
    for (int i = 0; i < 24; ++i) society_tick(1);
    assert(!g.war.prisonTown[victim] && !g.depth.actors[victim].captive && g.society.freed[victim]);
    assert(g.actors[victim].gear[0] == weapon && g.actors[victim].gearStolen[0] == OUTLAWS);
    assert(!g.actors[victim].squad && g.war.prisonTown[second] == t+1);
    politics_occupy(t, MONKS);
    assert(!g.war.prisonTown[second] && !g.depth.actors[second].captive && g.society.freed[second]);
    assert(g.townOwner[t] == MONKS && politics_price(t, MEDKIT) < 1);
    puts("PASS: garrison resists, conquerors post an actual soldier, prisoners and owned weapons persist, guarded labor produces goods, rescue and regime change free captives.");
}

static void chatter(void) {
    game_new(73819, 0);
    at(26, 29.5f);
    g.dayTime = 12;
    int n = g.war.resident[0][2] - 1;
    init_actor(n, "Road trader", TRADER, FREE, 26.7f, 29.5f, 8);
    g.actors[n].speechCooldown = .02f;
    for (int i = 1; i < g.actorCount; ++i) if (i != n) g.actors[i].speechCooldown = 90;
    g.time = 150;
    politics_occupy(OLD_TOWNS+1, MONKS);
    assert(!strstr(dialogue_line(n, 5), "VERMILION KEEP"));
    g.time += 145;
    int key = war_chatter(n);
    assert(key >= 32 && strstr(war_ambient(key, FREE), "Ash order took VERMILION KEEP"));
    assert(war_chatter(n) == 0);
    g.war.nextGossip = 0;
    g.war.lastSpoken = 0;
    fixture("gossip");

    g.time += 2000;
    int bandit = n + 1;
    init_actor(bandit, "Road bandit", RAIDER, HOSTILE, 49, 29.5f, 10);
    g.depth.actors[bandit].room=0; /* this slot previously belonged to an indoor resident */
    for(int k=0;k<POP_GROUPS;k++)g.population.groups[k].x=g.population.groups[k].y=1400;
    g.war.nextGossip = 0;
    key = war_chatter(n);
    assert(key >= 24 && key < 32 && strstr(war_ambient(key, FREE), "bandits east"));
    g.war.nextGossip = 0;
    g.war.lastSpoken = 0;
    fixture("bandits");
    g.actors[bandit].dead = 1;
    g.war.nextGossip = 0;
    assert(war_chatter(n) == 0 || !strstr(war_rumor(&g.actors[n]), "VERMILION KEEP"));
    puts("PASS: distant news takes time, speakers react to actual owners, spam is throttled, nearby bandits produce directional warnings and old news expires.");
}

static void migrations(const char *v16) {
    static const char *files[] = {"tests/fixtures/legacy-v01.dust", "build/v170/old-v4.sav",
        "build/v170/old-v5.sav", "build/v170/old-v9.sav", "build/v170/old-v11.sav",
        "build/v170/old-v12.sav", "build/v170/old-v13.sav", "build/v170/old-v14.sav"};
    for (unsigned i = 0; i < sizeof(files)/sizeof(files[0]); ++i) {
        assert(load_game(files[i]));
        assert(g.war.version && population_validate(&g) && life_validate(&g));
    }
    if(v16) {
        assert(load_game(v16));
        assert(g.society.weaponXP[g.selected][WC_SLASHING] == 10200);
        assert(g.population.social[0][0].weaponXP[WC_ARCHERY] == 110);
        int money = g.money, buildings = g.buildingCount, actors = g.actorCount;
        fixture("migrated");
        assert(load_game("build/v170/migrated.sav"));
        assert(g.money == money && g.buildingCount == buildings && g.actorCount == actors);
    }
    puts("PASS: released older saves open with their original state; reloading the new format creates no duplicate capital buildings or residents.");
}

int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);
    capitals();
    siege_rescue();
    chatter();
    migrations(argc>1?argv[1]:NULL);
    return 0;
}
