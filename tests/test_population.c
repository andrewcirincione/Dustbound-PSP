#include "../src/render.c"
#include <assert.h>
#include <time.h>
static uint32_t canvas[512*272+64];
static Actor *fresh(void){game_new(73819,0);assert(population_validate(&g));Actor*a=&g.actors[g.selected];a->x=a->y=400;a->autoattack=0;a->target=-1;g.money=1000;g.dayTime=12;for(int f=0;f<FACTIONS;f++)g.rep[f]=80;for(int n=0;n<g.actorCount;n++)if(n!=g.selected){g.actors[n].active=0;g.actors[n].target=-1;g.actors[n].attackPhase=0;}return a;}
static void place(int k,float x,float y){WorldEvent*e=&g.population.groups[k];PopulationState*m=&g.population.state[k];float dx=x-e->x,dy=y-e->y;e->x=x;e->y=y;m->siteX+=dx;m->siteY+=dy;m->targetX=x+20;m->targetY=y;for(int i=0;i<e->count;i++){Actor*a=&e->reserve[i];a->x+=dx;a->y+=dy;a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;}}
static void isolate(void){for(int k=0;k<POP_GROUPS;k++)place(k,1250+(k%8)*12,1250+(k/8)*12);}
static void clear_area(float *x,float*y){for(int yy=380;yy<460;yy+=4)for(int xx=380;xx<460;xx+=4){int clear=1;for(int j=-6;j<=6;j++)for(int i=-6;i<=6;i++)if(!can_walk(xx+i,yy+j))clear=0;if(clear){*x=xx;*y=yy;return;}}assert(!"no clear fixture ground");}
static void prepared(int k,int type){population_generate(k,type);float x,y;clear_area(&x,&y);place(k,x,y);WorldEvent*e=&g.population.groups[k];PopulationState*m=&g.population.state[k];
 for(int j=0;j<5;j++){m->offsetX[j]=j==0?0:j==4?4:(j-2)*3;m->offsetY[j]=j==0?0:j==4?-3:3;}
 for(int i=0;i<e->count;i++){e->reserve[i].x=x+(i%4-1)*1.8f;e->reserve[i].y=y+(i/4)*1.8f;e->reserve[i].homeX=e->reserve[i].tx=e->reserve[i].x;e->reserve[i].homeY=e->reserve[i].ty=e->reserve[i].y;}
}
static void capture(const char*name){ui.camX=g.actors[g.selected].x;ui.camY=g.actors[g.selected].y;for(int i=512*272;i<512*272+64;i++)canvas[i]=0xdeadbeef;render_game(canvas,512);for(int i=512*272;i<512*272+64;i++)assert(canvas[i]==0xdeadbeef);char path[180];snprintf(path,sizeof(path),"build/v120/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=canvas[y*512+x];unsigned char rgb[]={c&255,c>>8&255,c>>16&255};fwrite(rgb,1,3,f);}fclose(f);}
static void distribution(void){
 int total=0,wearers=0,types[POP_TYPES]={0},biomes[16]={0};
 for(int seed=0;seed<6;seed++){game_new(73819+seed*923,0);assert(population_validate(&g));int units=0,worn=0;for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];types[e->type]++;biomes[biome(e->x,e->y)]++;assert(can_walk(e->x,e->y));assert(!e->spawned&&e->stored);for(int j=0;j<e->count;j++){Actor*a=&e->reserve[j];units++;worn+=a->prostheticMask!=0;if(a->kind==BEAST)assert(!a->prostheticMask);}}
  printf("WORLD %d: %d generated NPCs, %d prosthetic wearers, %d preserved wilderness NPCs\n",seed,units,worn,g.population.imported);total+=units;wearers+=worn;
 }
 for(int t=0;t<POP_TYPES;t++)assert(types[t]);int regions=0;for(int b=0;b<16;b++)regions+=biomes[b]>0;assert(regions>=6&&wearers*100<total*9&&wearers*100>total*1);
 Actor base=g.actors[g.selected];base.lostMask=base.prostheticMask=base.squad=base.dead=0;base.kind=SETTLER;base.race=0;for(int l=0;l<LIMBS;l++)base.health[l]=100;
 for(int tier=0;tier<=3;tier++){int fitted=0,relic=0;for(int seed=1;seed<=20000;seed++){Actor a=base;population_roll_limbs(&a,(unsigned)seed*19349663u,tier);fitted+=a.prostheticMask!=0;for(int l=LARM;l<=RLEG;l++)relic+=limb_item_grade(limb_installed(&a,l))==4;}printf("RARITY tier %d: %d/20000 wearers; %d Relic parts\n",tier,fitted,relic);assert(fitted>(tier==3?1600:tier==2?1000:400)&&fitted<(tier==3?2400:tier==2?1800:1200));assert(relic<30);if(tier<3)assert(!relic);}
 puts("PASS: persistent groups cover all 16 situations and several regions; ordinary prosthetics are uncommon and beasts have none.");
}
static void roaming_bandits(void) {
    int cloth = 0, food = 0, dressings = 0, empty = 0;
    for (unsigned seed = 73819; seed <= 73823; seed++) {
        game_new(seed,0);
        for (int k = WAR_POP_GROUPS; k < POP_GROUPS; k++) {
            WorldEvent *e = &g.population.groups[k];
            assert(e->type == POP_RAIDERS && e->tier == 0 && (e->count == 3 || e->count == 4));
            assert(g.population.state[k].phase == POP_TRAVEL && !population_object_count(k));
            for (int j = 0; j < e->count; j++) {
                Actor *a = &e->reserve[j];
                assert(a->kind == RAIDER && a->faction == HOSTILE && level(a,MELEE) <= 7);
                assert(a->health[CHEST] == species_health(a) && a->health[CHEST] < 75);
                int supplies = 0;
                for (int slot = 0; slot < PACK; slot++) {
                    if (!a->pack[slot].item) continue;
                    int item = portion_base(a->pack[slot].item);
                    assert(a->pack[slot].qty == 1);
                    if (item == CLOTH) cloth++;
                    else if (item == JERKY || item == OATCAKE) food++;
                    else {
                        assert(item == MEDKIT && portion_percent(a->pack[slot].item) < 100);
                        dressings++;
                    }
                    supplies++;
                }
                assert(supplies <= 1);
                empty += !supplies;
            }
        }
    }
    assert(cloth && food && dressings && empty);
    int serial = g.population.groups[WAR_POP_GROUPS].serial;
    assert(save_game("build/v120/roaming-bandits.sav") && load_game("build/v120/roaming-bandits.sav"));
    assert(g.population.groups[WAR_POP_GROUPS].serial == serial && population_validate(&g));
    puts("PASS: eight small roaming gangs have weak fighters, sparse supplies and persistent saves.");
}
static void persistence(void){
 Actor*a=fresh();isolate();prepared(0,POP_SCAVENGERS);WorldEvent*e=&g.population.groups[0];a->x=e->x;a->y=e->y;assert(population_materialize(0));int n=e->members[0];Actor*b=&g.actors[n];strcpy(b->name,"STATE SURVIVOR");b->health[LARM]=39;b->stolen[0]=OUTLAWS;limb_set(b,RLEG,limb_item(3,1),OUTLAWS);b->health[RLEG]=91;g.depth.actors[n].trauma[LARM]=4;g.society.weaponXP[n][0]=840;g.population.state[0].cacheQty=0;
 Actor snapshot=*b;a->x=a->y=100;assert(population_store(0));assert(!e->spawned&&e->members[0]==-1);assert(save_game("build/v120/stored.sav")&&load_game("build/v120/stored.sav"));e=&g.population.groups[0];a=&g.actors[g.selected];assert(!e->spawned&&e->reserve[0].health[LARM]==39&&e->reserve[0].stolen[0]==OUTLAWS&&limb_installed(&e->reserve[0],RLEG)==limb_item(3,1)&&!g.population.state[0].cacheQty);a->x=e->x;a->y=e->y;assert(population_materialize(0));n=e->members[0];b=&g.actors[n];assert(!strcmp(b->name,snapshot.name)&&b->health[RLEG]==91&&limb_owner(b,RLEG)==OUTLAWS&&g.society.weaponXP[n][0]==840&&g.depth.actors[n].trauma[LARM]==4);
 b->unconscious=300;a->x=b->x;a->y=b->y+1;ui.lootActor=n;int old=inventory_count(a,limb_item(3,1));assert(loot_take(PACK+GEARS+3,0));assert(inventory_count(a,limb_item(3,1))==old+1&&limb_missing(b,RLEG));assert(!loot_take(PACK+GEARS+3,0));
 a->x=a->y=100;assert(population_store(0));assert(save_game("build/v120/looted.sav")&&load_game("build/v120/looted.sav"));e=&g.population.groups[0];assert(limb_missing(&e->reserve[0],RLEG));population_init();assert(limb_missing(&e->reserve[0],RLEG));assert(population_materialize(0));n=e->members[0];assert(limb_missing(&g.actors[n],RLEG));
 g.actors[n].dead=1;g.actors[n].unconscious=0;assert(population_store(0));assert(population_materialize(0));assert(g.actors[e->members[0]].dead);
 /* Nearby members of a split squad, carried NPCs and helpers pin their actual actors. */
 int ally=1;g.actors[ally]=g.actors[g.selected];g.actors[ally].squad=1;g.actors[ally].active=1;g.actors[ally].x=e->x;g.actors[ally].y=e->y;g.squadCount=2;assert(!population_store(0));g.actors[ally].x=g.actors[ally].y=100;g.actors[e->members[0]].carriedBy=g.selected;assert(!population_store(0));g.actors[e->members[0]].carriedBy=-1;
 int follow=e->members[1];g.society.freed[follow]=1;population_tick(1);assert(!population_owned(follow)&&g.actors[follow].active&&g.population.state[0].detached&2);assert(population_validate(&g));
 puts("PASS: full actor, damage, loot, stolen tags, fitted parts, class XP, trauma, corpses and finite supplies survive streaming/save reload; no limb duplication; split squads/carried actors pin; freed followers detach without joining the squad.");
}
static void camp_actions(void){
 Actor*a=fresh();isolate();prepared(0,POP_MERCHANTS);WorldEvent*e=&g.population.groups[0];PopulationState*m=&g.population.state[0];assert(population_materialize(0));int bed=m->beds;float x,y;assert(population_object(0,bed,&x,&y)==2);a->x=x;a->y=y;a->health[LARM]=40;a->fatigue=80;a->bleed=0;a->hunger=100;
 int count=nearby_actions(),found=-1;for(int r=0;r<count;r++){const NearbyAction*act=nearby_action(r);if(act->type==ACTION_POPULATION&&act->kind==10+bed)found=r;}assert(found>=0);nearby_action_choose(found);assert(a->taskKind==3&&population_sleep_valid(a));for(int i=0;i<60;i++)game_tick(1.f/30,(Input){0});assert(a->health[LARM]>40&&a->fatigue<80);
 a->taskKind=0;population_object(0,0,&a->x,&a->y);int money=g.money,rations=inventory_count(a,RATION),stock=m->stock[0];assert(population_action(0,30)&&g.money==money-40&&inventory_count(a,RATION)==rations+1&&m->stock[0]==stock-1);
 inventory_add(a,MEAT,2);int meat=inventory_count(a,MEAT);rations=inventory_count(a,RATION);assert(population_action(0,1)&&inventory_count(a,MEAT)==meat-2&&inventory_count(a,RATION)==rations+1);
 a->x=e->x+20;a->y=e->y;money=g.money;assert(!population_action(0,30)&&g.money==money);a->x=x;a->y=y;g.actors[e->members[0]].faction=HOSTILE;g.actors[e->members[0]].x=x+1;g.actors[e->members[0]].y=y;assert(!population_action(0,10+bed));g.actors[e->members[0]].faction=SCAVENGERS;
 /* Object reach follows the physical bed even when terrain displaced it from the camp center. */
 m->offsetX[bed]=12;population_object(0,bed,&a->x,&a->y);count=nearby_actions();found=-1;for(int r=0;r<count;r++){const NearbyAction*act=nearby_action(r);if(act->type==ACTION_POPULATION&&act->kind==10+bed)found=r;}assert(found>=0);m->offsetX[bed]=(bed-2)*3;
 a->x=e->x;a->y=e->y;Actor before=*a;for(int q=0;q<PACK;q++)a->pack[q]=(Stack){WOOD,999};a->pack[0]=(Stack){MEAT,3};money=g.money;stock=m->stock[0];assert(!population_action(0,1)&&inventory_count(a,MEAT)==3);assert(!population_action(0,30)&&g.money==money&&m->stock[0]==stock);*a=before;
 a->x=e->x;a->y=e->y;ui.paused=1;count=nearby_actions();found=-1;for(int r=0;r<count;r++){const NearbyAction*act=nearby_action(r);if(act->type==ACTION_POPULATION&&act->kind==30)found=r;}assert(found>=0);nearby_action_choose(found);assert(g.society.queueCount[g.selected]==1);assert(save_game("build/v120/queued-camp.sav")&&load_game("build/v120/queued-camp.sav"));assert(g.society.queueCount[g.selected]==1);
 puts("PASS: actual nearby-action bed rest heals and reduces fatigue; finite trader stock, exact payment, cooking deductions, safe/reach gates and paused camp actions save correctly.");
}
static void offline(void){
 Actor*a=fresh();isolate();prepared(0,POP_PATROL);WorldEvent*e=&g.population.groups[0];PopulationState*m=&g.population.state[0];a->x=a->y=100;float before=e->x,memberx=e->reserve[0].x;for(int i=0;i<12;i++)population_tick(1);assert(fabsf(e->x-before)>1&&fabsf(e->reserve[0].x-memberx)>1&&!e->spawned);
 prepared(0,POP_SCAVENGERS);e=&g.population.groups[0];m=&g.population.state[0];e->age=0;g.dayTime=12;for(int n=0;n<g.nodeCount;n++)if(distance(e->x,e->y,g.nodes[n].x,g.nodes[n].y)<40)g.nodes[n].remaining=0;int node=g.nodeCount++;g.nodes[node]=(Node){.x=e->x+8,.y=e->y,.type=NODE_ORE,.remaining=8};int ore=0;for(int j=0;j<e->count;j++)ore+=inventory_count(&e->reserve[j],ORE);for(int i=0;i<50;i++)population_tick(1);int after=0;for(int j=0;j<e->count;j++)after+=inventory_count(&e->reserve[j],ORE);assert(m->harvests>=1&&after>ore&&g.nodes[node].remaining<8);
 prepared(0,POP_RAIDERS);prepared(1,POP_PATROL);e=&g.population.groups[0];WorldEvent*f=&g.population.groups[1];place(1,e->x+4,e->y);g.population.state[0].phase=g.population.state[1].phase=POP_FIGHT;g.population.state[0].clock=g.population.state[1].clock=1000;float health=0;for(int i=0;i<f->count;i++)health+=f->reserve[i].health[CHEST]+f->reserve[i].health[HEAD];int money=g.money,kills=g.kills;for(int i=0;i<30;i++)population_tick(1);float hurt=0;for(int i=0;i<f->count;i++)hurt+=f->reserve[i].health[CHEST]+f->reserve[i].health[HEAD];assert(hurt<health&&g.population.conflicts>0&&g.money==money&&g.kills==kills);assert(save_game("build/v120/offline-battle.sav")&&load_game("build/v120/offline-battle.sav"));assert(g.population.conflicts>0&&population_validate(&g));
 puts("PASS: off-screen groups actually travel, gather finite world resources and fight other factions; injuries persist without awarding player money, kills or participation rewards.");
}
static void limits(void){
 Actor*a=fresh();isolate();prepared(0,POP_ARMY);for(int n=0;n<MAX_ACTORS;n++)g.living.owned[n]=0;g.actorCount=MAX_ACTORS;assert(!population_materialize(0)&&!g.population.groups[0].spawned);assert(g.population.groups[0].count>=12);
 a=fresh();int money=g.money;assert(load_game("build/old-v11.sav")&&g.population.version==2&&g.limbs.version==1);assert(g.money==20000);assert(save_game("build/v120/migrated.sav")&&load_game("build/v120/migrated.sav"));assert(population_validate(&g));int version=g.population.version;g.population.version=3;assert(!population_validate(&g));g.population.version=version;int t=g.population.groups[0].type;g.population.groups[0].type=100;assert(!population_validate(&g));g.population.groups[0].type=t;(void)money;
 /* Ordinary groups renew after a cooldown only when the original remains are out of range. */
 a=fresh();isolate();prepared(0,POP_BATTLEFIELD);WorldEvent*e=&g.population.groups[0];a->x=a->y=100;for(int i=0;i<e->count;i++)e->reserve[i].dead=1;e->age=9000;e->presence=0;int serial=e->serial;g.population.offlinePulse=4;population_tick(1);assert(e->serial==serial);e->presence=DAY_SECONDS/4;for(int i=0;i<25&&e->serial==serial;i++)population_tick(1);assert(e->serial!=serial);
 /* Physical convoy captives use the existing rescue timer and preserve independent follow/flee decisions. */
 a=fresh();isolate();prepared(0,POP_PRISONERS);assert(population_materialize(0));e=&g.population.groups[0];int prisoner=e->members[e->count-1];Actor*b=&g.actors[prisoner];a->x=b->x;a->y=b->y;for(int i=0;i<e->count-2;i++){g.actors[e->members[i]].x=e->x+100;g.actors[e->members[i]].y=e->y+100;}
 a->xp[LOCKPICK]=100000;assert(society_rescue_start(prisoner));for(int i=0;i<500&&g.depth.actors[prisoner].captive;i++)society_tick(.1f);assert(!g.depth.actors[prisoner].captive&&!b->squad&&g.society.freed[prisoner]);population_tick(1);assert(!population_owned(prisoner));assert(save_game("build/v120/rescued-convoy.sav")&&load_game("build/v120/rescued-convoy.sav"));
 puts("PASS: actor exhaustion defers whole groups without recycling protected NPCs; actual v11 migration and v12 reload pass; malformed population data is rejected.");
}
static void fixtures(void){
 for(int type=0;type<POP_TYPES;type++){
  Actor*a=fresh();isolate();prepared(0,type);WorldEvent*e=&g.population.groups[0];PopulationState*m=&g.population.state[0];a->x=e->x+1;a->y=e->y+3;assert(population_materialize(0));
  a->autoattack=0;memset(a->pack,0,sizeof(a->pack));inventory_add(a,MEAT,6);inventory_add(a,MEDKIT,3);inventory_add(a,PARTS,3);inventory_add(a,RATION,1);
  if(type==POP_MERCHANTS){population_object(0,m->beds,&a->x,&a->y);a->health[LARM]=40;a->fatigue=80;}
  char path[120];snprintf(path,sizeof(path),"build/v120/encounter-%02d.sav",type);assert(save_game(path));char name[60];snprintf(name,sizeof(name),"encounter-%02d",type);ui.screen=SCREEN_WORLD;capture(name);ui.screen=SCREEN_ACTIONS;ui.selection=0;snprintf(name,sizeof(name),"actions-%02d",type);capture(name);
 }
 Actor*a=fresh();isolate();int owner=-1,part=-1,source=-1;for(int k=0;k<POP_GROUPS&&source<0;k++)for(int i=0;i<g.population.groups[k].count;i++)for(int l=LARM;l<=RLEG;l++)if(limb_installed(&g.population.groups[k].reserve[i],l)){source=k;owner=i;part=l;break;}
 assert(source>=0);/* Keep the real rolled actor and its original gear; make an ordinary prepared fallen-owner save. */
 GroupLife gl=g.life.groups[source];g.life.groups[source]=g.life.groups[0];g.life.groups[0]=gl;WorldEvent snapshot=g.population.groups[source];PopulationState sm=g.population.state[source];PopulationSocial social[POP_MEMBERS];memcpy(social,g.population.social[source],sizeof(social));g.population.groups[source]=g.population.groups[0];g.population.state[source]=g.population.state[0];memcpy(g.population.social[source],g.population.social[0],sizeof(social));g.population.groups[0]=snapshot;g.population.state[0]=sm;memcpy(g.population.social[0],social,sizeof(social));float x,y;clear_area(&x,&y);place(0,x,y);assert(population_materialize(0));WorldEvent*e=&g.population.groups[0];int n=e->members[owner];Actor*b=&g.actors[n];b->unconscious=1000;b->bleed=0;a->x=b->x;a->y=b->y+1;for(int i=0;i<e->count;i++)if(i!=owner){g.actors[e->members[i]].x=e->x+22+i;g.actors[e->members[i]].y=e->y+15;}
 assert(save_game("build/v120/random-prosthetic.sav"));printf("ROLLED LIMB FIXTURE: group type %d owner %s limb %d item %d\n",e->type,b->name,part,limb_installed(b,part));ui.screen=SCREEN_LOOT;ui.lootActor=n;ui.selection=PACK+GEARS+part-LARM;capture("random-prosthetic");
 a=fresh();isolate();prepared(0,POP_MERCHANTS);assert(population_materialize(0));e=&g.population.groups[0];a->x=e->x+3;a->y=e->y+3;ui.screen=SCREEN_WORLD;clock_t start=clock();for(int i=0;i<1000;i++)simulate(.033f);printf("BENCH: 1000 population-enabled ticks %.3fs native CPU; not physical PSP performance.\n",(double)(clock()-start)/CLOCKS_PER_SEC);assert(population_validate(&g));
 puts("PASS: all sixteen world/action layouts stay within 480x272; ordinary prepared saves and a naturally rolled prosthetic-owner fixture are ready for PSP controller review.");
}
int main(void){setvbuf(stdout,NULL,_IONBF,0);distribution();roaming_bandits();persistence();camp_actions();offline();limits();fixtures();printf("ALL POPULATION CHECKS PASSED; Game=%zu Population=%zu Actor=%zu\n",sizeof(Game),sizeof(Population),sizeof(Actor));}
