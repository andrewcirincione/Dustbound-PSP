#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void park_group(int k) {
    WorldEvent *e=&g.population.groups[k];
    PopulationState *m=&g.population.state[k];
    e->x=m->siteX=1160+(k%10)*16;
    e->y=m->siteY=1160+(k/10)*16;
    m->targetX=e->x+10;
    m->targetY=e->y;
    g.life.groups[k].goal=LIFE_SETTLED;
    for(int i=0;i<e->count;i++){
        e->reserve[i].x=e->x;
        e->reserve[i].y=e->y;
    }
}
static Actor *quiet_frontier(unsigned seed) {
    game_new(seed,0);
    Actor *a=&g.actors[g.selected];
    a->x=380;a->y=380;
    g.day=42;
    g.time=41*DAY_SECONDS;
    for(int k=0;k<POP_GROUPS;k++)park_group(k);
    for(int k=0;k<EVENTS;k++)g.living.events[k].status=2;
    return a;
}
static void roll_tick(void) {
    g.population.offlinePulse=4;
    population_tick(1);
    g.time+=1;
}
static void walk_east(Actor *a) { walk_motion(a,.12f,0); }

static void test_clearance(int k) {
    Actor *a=quiet_frontier(73819);
    WorldEvent *e=&g.population.groups[k];
    float oldX=e->x,oldY=e->y;
    int serial=e->serial;
    for(int i=0;i<e->count;i++)e->reserve[i].dead=1;
    e->age=DAY_SECONDS/4;
    g.life.groups[k].goal=LIFE_ROAM;
    walk_east(a);
    roll_tick();
    assert(e->serial==serial&&g.population.state[k].phase==POP_RUINS);
    e->presence=DAY_SECONDS/4;
    a->x=oldX-1;a->y=oldY;
    roll_tick();
    assert(e->serial==serial&&e->spawned&&g.actors[e->members[0]].dead);

    a->x=380;a->y=380;
    walk_east(a);
    roll_tick();
    assert(e->serial!=serial&&!e->spawned);
    assert(e->x>=a->x+61&&e->x<=a->x+92&&fabsf(e->y-a->y)<=18.1f&&distance(e->x,e->y,a->x,a->y)>=68);
    assert(distance(e->x,e->y,oldX,oldY)>105&&can_walk(e->x,e->y));
    assert(save_game("build/encounter-refresh.sav")&&load_game("build/encounter-refresh.sav"));
    assert(population_validate(&g)&&e->serial!=serial&&!e->spawned);
    puts("PASS: recent clear stays empty, a nearby squad preserves old bodies, and a replacement ahead persists on reload.");
}

static void test_passing_traffic(void) {
    unsigned types=0;
    int k=WAR_POP_GROUPS;
    for(unsigned seed=73819;seed<73839;seed++){
        Actor *a=quiet_frontier(seed);
        WorldEvent *e=&g.population.groups[k];
        e->age=DAY_SECONDS/4;
        GroupLife *z=&g.life.groups[k];
        z->goal=LIFE_ROAM;
        z->decision=600;
        FieldSighting *seen=&g.fieldNotes.group[k];
        seen->serial=e->serial;
        seen->time=g.time;
        int serial=e->serial;
        walk_east(a);
        roll_tick();
        assert(e->serial==serial);
        g.time+=DAY_SECONDS/2;
        a->movedTimer=0;
        roll_tick();
        assert(e->serial==serial);
        walk_east(a);
        roll_tick();
        assert(e->serial!=serial&&!e->spawned);
        assert(e->x>=a->x+61&&e->x<=a->x+92&&fabsf(e->y-a->y)<=18.1f&&distance(e->x,e->y,a->x,a->y)>=68);
        assert(town_near(e->x,e->y,28)<0);
        if (g.population.state[k].beds)
            assert(population_object_count(k)>=7);
        assert(population_validate(&g));
        if(e->type==POP_RAIDERS){
            assert(e->count==3||e->count==4);
            for(int i=0;i<e->count;i++)
                assert(level(&e->reserve[i],MELEE)<=7&&e->reserve[i].health[CHEST]<75);
        }
        types|=1u<<e->type;
        int newSerial=e->serial;
        roll_tick();
        assert(e->serial==newSerial);
    }
    unsigned count=0;
    for(unsigned bits=types;bits;bits>>=1)count+=bits&1;
    assert(count>=5&&(types&(1u<<POP_RAIDERS)));
    printf("PASS: quiet day-42 travel renewed %u encounter types over 20 seeds without growing the population.\n",count);
}

static void test_live_scenes(void) {
    unsigned seen=0;
    int camps=0;
    for (unsigned seed=74500;seed<74564;seed++) {
        Actor *a=quiet_frontier(seed);
        int serial[POP_GROUPS];
        for (int k=0;k<POP_GROUPS;k++) {
            WorldEvent *e=&g.population.groups[k];
            serial[k]=e->serial;
            if (k>=WAR_POP_GROUPS) {
                e->age=DAY_SECONDS/4;
                g.life.groups[k].goal=LIFE_ROAM;
                g.life.groups[k].decision=600;
            }
        }
        walk_east(a);
        roll_tick();
        int first=-1,second=-1;
        for (int k=0;k<POP_GROUPS;k++)
            if (g.population.groups[k].serial!=serial[k]) {
                if (first<0) first=k;
                else {
                    assert(second<0);
                    second=k;
                }
            }
        assert(first>=0&&!g.population.groups[first].spawned);
        camps+=g.population.state[first].beds>0;
        if (second<0) continue;
        WorldEvent *hunter=&g.population.groups[first],*prey=&g.population.groups[second];
        assert(g.life.groups[first].targetGroup==second&&g.life.groups[first].targetSerial==prey->serial);
        assert(distance(hunter->x,hunter->y,prey->x,prey->y)>18);
        if (hunter->type==POP_HUNTERS) {
            assert(prey->type==POP_BEASTS);
            seen|=1;
        } else if (hunter->type==POP_PATROL) {
            assert(prey->type==POP_PATROL&&war_rivals(hunter->faction,prey->faction));
            assert(g.life.groups[second].targetSerial==hunter->serial);
            if (!(seen&2)) {
                int before=g.population.conflicts;
                a->x=a->y=100;
                a->movedTimer=0;
                for (int i=0;i<100&&g.population.conflicts==before;i++) roll_tick();
                assert(g.population.conflicts>before);
                assert(g.population.state[first].fights>0&&g.population.state[second].fights>0);
                assert(save_game("build/patrol-skirmish.sav")&&load_game("build/patrol-skirmish.sav"));
                assert(g.population.state[first].fights>0&&g.population.state[second].fights>0);
            }
            seen|=2;
        } else {
            assert(hunter->type==POP_RAIDERS&&(prey->type==POP_MERCHANTS||prey->type==POP_PILGRIMS));
            seen|=4;
        }
    }
    assert(seen==7&&camps>=5);
    printf("PASS: hunts, rival patrol combat, road raids and %d new camps across 64 late-game route rolls.\n",camps);
}

int main(void){
    test_clearance(0);
    test_clearance(WAR_POP_GROUPS);
    test_passing_traffic();
    test_live_scenes();
    return 0;
}
