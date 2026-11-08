#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
extern int hostile(const Actor*,const Actor*);
static uint32_t pixels[480*272];
static void shot(const char*n){char path[200];snprintf(path,sizeof(path),"evidence/v080/%s.ppm",n);render_game(pixels,480);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<480*272;i++){unsigned char b[3]={pixels[i]&255,(pixels[i]>>8)&255,(pixels[i]>>16)&255};fwrite(b,1,3,f);}fclose(f);}
static uint32_t hash(const void*p,size_t n){uint32_t h=2166136261u;const unsigned char*b=p;while(n--)h=(h^*b++)*16777619u;return h;}
static void v7fixture(void){/* Serialize the exact released v7 layout from the unchanged fields. */
    unsigned char raw[435592];memcpy(raw,&g,144);
    for(int t=0;t<TOWNS;t++)memcpy(raw+144+t*CATALOG_ITEMS*4,g.stock[t],CATALOG_ITEMS*4);
    size_t tail=144+TOWNS*CATALOG_ITEMS*4;
    memcpy(raw+tail,(char*)&g+offsetof(Game,lastRestock),sizeof(raw)-tail);
    struct {char magic[8];unsigned version,size,hash;}h={{'D','U','S','T','S','A','V','E'},7,sizeof(raw),hash(raw,sizeof(raw))};
    FILE*f=fopen("build/v7-living.sav","wb");fwrite(&h,sizeof(h),1,f);fwrite(raw,sizeof(raw),1,f);fclose(f);
}
int main(void){
    game_new(73819,0);assert(g.living.version==1&&living_validate(&g));
    for(int it=CATALOG_ITEMS;it<V100_ITEMS;it++){assert(gear_family(it)>=0&&gear_family(it)<FAMILIES);assert(itemDefs[it].price>0);}
    g.money=777;g.stock[15][CATALOG_ITEMS-1]=9;g.regions.outcome[0]=1;v7fixture();
    assert(load_game("build/v7-living.sav"));assert(g.money==777&&g.stock[15][CATALOG_ITEMS-1]==9&&g.regions.outcome[0]==1&&g.living.version==1);
    int ser=g.living.serial;assert(save_game("build/v8-living.sav"));assert(load_game("build/v8-living.sav"));assert(g.living.serial==ser);
    int seen[32]={0};for(int i=0;i<1200;i++){living_generate(i%EVENTS);WorldEvent*e=&g.living.events[i%EVENTS];seen[e->type*4+e->modifier]++;assert(living_validate(&g));}
    for(int i=0;i<32;i++)assert(seen[i]);
    ser=g.living.serial;for(int i=0;i<5000;i++)living_tick(1);assert(g.living.serial>ser+8);assert(g.actorCount<=MAX_ACTORS);
    game_new(73819,0);Actor*a=&g.actors[g.selected];for(int i=1;i<g.actorCount;i++)g.actors[i].active=0;
    a->x=400;a->y=400;ui.camX=a->x;ui.camY=a->y;ui.noticeTime=0;
    Actor*b=&g.actors[1];b->active=1;b->kind=GUARD;b->faction=SCAVENGERS;b->squad=0;b->dead=0;b->unconscious=0;b->x=b->homeX=b->tx=403;b->y=b->homeY=b->ty=400;
    assert(combat_candidates()==1&&combat_candidate(0)==1&&actor_relation(1)==0);
    int money=g.money,bounty=a->bounty;command_attack(1);assert(g.living.command[g.selected]==1&&hostile(a,b)&&a->bounty==bounty+100&&g.money==money);
    find_context();assert(ui.contextKind!=0||ui.contextIndex!=1);
    ui.screen=SCREEN_WORLD;shot("target");
    g.living.anger[1]=0;g.rep[b->faction]=0;a->bounty=0;g.living.command[g.selected]=-1;a->target=-1;a->autoattack=0;assert(save_game("build/DUST_2.sav"));
    ui.screen=SCREEN_ENGAGE;ui.selection=0;ui.attackFocus=1;shot("opponents");
    b->active=0;WorldEvent*e=&g.living.events[0];e->type=6;e->count=16;e->modifier=2;e->faction=SHOGUN;e->x=410;e->y=400;e->tier=3;e->town=8;
    a->x=407;a->y=399;ui.camX=a->x;ui.camY=a->y;ui.noticeTime=0;
    living_tick(1);assert(e->spawned);for(int i=0;i<16;i++){assert(e->members[i]>=0&&g.living.owned[e->members[i]]==1);}
    ui.screen=SCREEN_WORLD;shot("army");memset(g.discovered,1,sizeof(g.discovered));memset(g.seen,255,sizeof(g.seen));assert(save_game("build/DUST_3.sav"));
    int original=e->members[0];g.actors[original].health[LARM]=43;g.actors[original].pack[0].qty=1;
    a->x=20;a->y=20;living_tick(1);assert(e->stored&&!e->spawned);
    a->x=e->x-3;a->y=e->y-1;living_tick(1);assert(e->spawned&&g.actors[e->members[0]].health[LARM]==43&&g.actors[e->members[0]].pack[0].qty==1);
    int leader=e->members[0];assert(g.actors[leader].gear[0]>=CATALOG_ITEMS+16);ui.screen=SCREEN_LOOT;ui.lootActor=leader;ui.selection=PACK;shot("signature-loot");
    ui.screen=SCREEN_JOURNAL;ui.journalTab=2;ui.selection=0;shot("events");
    memset(g.discovered,1,sizeof(g.discovered));memset(g.seen,255,sizeof(g.seen));ui.screen=SCREEN_MAP;ui.mapTown=8;ui.mapX=towns[8].x;ui.mapY=towns[8].y;ui.mapZoom=1;shot("map-local");ui.mapZoom=0;shot("map-world");
    /* One-shot delivery, full-pack cache rollback, guard retaliation, suspended job safety. */
    e=&g.living.events[1];e->type=2;e->status=0;e->tier=1;e->modifier=1;e->x=a->x;e->y=a->y;e->town=0;
    inventory_add(a,RATION,4);money=g.money;living_action(1);assert(e->status==1&&g.money>money);money=g.money;living_action(1);assert(g.money==money);
    e=&g.living.events[2];e->type=3;e->status=0;e->x=a->x;e->y=a->y;for(int i=0;i<PACK;i++)a->pack[i]=(Stack){WOOD,999};living_action(2);assert(!e->status);
    /* Resolve army and recycle it far from the player; no abandoned references. */
    e=&g.living.events[0];int owner=g.townOwner[e->town];e->x=towns[e->town].x;e->y=towns[e->town].y;
    a->x=e->x-2;a->y=e->y;g.actors[e->members[0]].x=e->x;g.actors[e->members[0]].y=e->y;
    b=&g.actors[1];b->active=1;b->kind=GUARD;b->faction=SCAVENGERS;b->homeTown=e->town;b->x=e->x;b->y=e->y;living_tick(1);assert(!e->status&&g.townOwner[e->town]==owner);b->active=0;
    for(int i=0;i<16;i++){g.actors[e->members[i]].unconscious=90;g.actors[e->members[i]].defeatCredited=1;}
    living_tick(1);assert(e->status==1);a->x=40;a->y=40;e->age=121;living_tick(1);assert(e->status==0&&!e->spawned);assert(living_validate(&g));
    printf("PASS: 1,200 event generations, all 32 mechanic/condition combinations, runtime renewal, 16-member formation, reward/loot gates, target retaliation, v7 migration and v8 persistence.\n");
}
