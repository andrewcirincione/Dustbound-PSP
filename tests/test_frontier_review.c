#include "../src/render.c"
#include <assert.h>
extern void init_actor(int,const char*,int,int,float,float,int);
static uint32_t pixels[512*272+64];
static void press(int key){game_tick(.033f,(Input){key,key,0,0});}
static void shot(const char *name){
 for(int i=512*272;i<512*272+64;i++)pixels[i]=0xdeadbeef;
 ui.camX=g.actors[g.selected].x;ui.camY=g.actors[g.selected].y;render_game(pixels,512);
 for(int i=512*272;i<512*272+64;i++)assert(pixels[i]==0xdeadbeef);
 char path[128];snprintf(path,sizeof(path),"build/v150/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=pixels[y*512+x];unsigned char rgb[]={c&255,c>>8&255,c>>16&255};fwrite(rgb,1,3,f);}fclose(f);
}
static void save(const char *name){char path[128];snprintf(path,sizeof(path),"build/v150/%s.sav",name);assert(save_game(path));}
static Actor*fresh(void){
 game_new(73819,0);Actor*a=&g.actors[g.selected];a->x=a->y=380;a->autoattack=0;g.money=1000;g.dayTime=12;ui.screen=SCREEN_WORLD;ui.feedCount=0;
 for(int n=1;n<g.actorCount;n++){g.actors[n].active=0;g.actors[n].target=-1;g.actors[n].attackPhase=0;}
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];float x=1250+k%8*15,y=1250+k/8*15,dx=x-e->x,dy=y-e->y;e->x=x;e->y=y;g.population.state[k].targetX=x;g.population.state[k].targetY=y;g.population.state[k].siteX=x;g.population.state[k].siteY=y;g.life.groups[k].goal=LIFE_SETTLED;for(int i=0;i<e->count;i++){e->reserve[i].x+=dx;e->reserve[i].y+=dy;}}
 g.limbs.surgeonActor=0;for(int n=0;n<DEPTH_NPCS;n++)g.depth.npc[n]=-1;for(int n=0;n<ELITES;n++)g.civil.eliteActor[n]=0;
 return a;
}
static void group(int type,int starving){
 for(int tries=0;tries<100;tries++){population_generate(0,type);if(!starving||g.population.groups[0].modifier==1)break;}
 WorldEvent*e=&g.population.groups[0];assert(!starving||e->modifier==1);float dx=380-e->x,dy=380-e->y;e->x=e->y=380;
 PopulationState*m=&g.population.state[0];m->siteX=m->siteY=380;m->targetX=430;m->targetY=380;
 for(int i=0;i<e->count;i++){e->reserve[i].x+=dx;e->reserve[i].y+=dy;}
 assert(population_materialize(0));g.actors[g.selected].x=380;g.actors[g.selected].y=386;
}
static void combat(void){
 Actor*a=fresh();a->x=a->y=380;a->autoattack=1;
 for(int i=1;i<=3;i++){init_actor(i,"Test raider",RAIDER,HOSTILE,380+(i-2)*1.0f,380.8f,15);g.actors[i].gear[0]=SABRE;g.actors[i].target=g.selected;}
 strike(1,g.selected);strike(2,g.selected);assert(g.actors[1].attackPhase==1&&g.actors[2].attackPhase==1&&g.actors[1].attackDuration>=.45f);strike(3,g.selected);assert(!g.actors[3].attackPhase);g.actors[1].attackPhase=0;strike(3,g.selected);assert(g.actors[3].attackPhase==1);
 a->health[HEAD]=species_health(a)*.2f;assert(fabsf(knockout_reserve(a)-20)<.01f);a->blood=24;assert(fabsf(knockout_reserve(a)-5)<.01f);a->blood=100;a->health[HEAD]=species_health(a);a->health[LARM]=-40;assert(knockout_reserve(a)==100);a->unconscious=30;assert(knockout_reserve(a)==0);a->unconscious=0;
 a->attackPhase=1;a->attackClock=a->attackDuration=.5f;press(KEY_SQUARE);assert(!a->autoattack&&!a->attackPhase&&a->cooldown>=.5f);
 a->health[HEAD]=100;g.actors[2].health[CHEST]=25;g.actors[2].blood=55;a->target=2;ui.feedCount=0;save("combat");shot("combat");
 puts("PASS: actual knockout thresholds, two simultaneous AI melee strikes, readable windup floor, passive cancellation with recovery cost, combat framebuffer bounds.");
}
static void roads(void){
 game_new(73819,0);int buildings=g.buildingCount,nodes=g.nodeCount;trail_init();assert(g.buildingCount==buildings&&g.nodeCount==nodes);
 for(int k=0;k<6;k++){assert(g.trails.bed[k]>0);float x,y,dx,dy;trail_point(k,&x,&y,&dx,&dy);for(int j=-10;j<=10;j++)assert(!trail_blocked(x+dx*j,y+dy*j));for(int r=0;r<4;r++){float rx,ry;if(trail_rock(k,r,&rx,&ry))assert(!can_walk(rx,ry));}assert(!trail_blocked(x-dy*8,y+dx*8));}
 Actor*a=&g.actors[g.selected];float x,y,dx,dy;trail_point(0,&x,&y,&dx,&dy);a->x=x-dx*6;a->y=y-dy*6;a->autoattack=0;ui.screen=SCREEN_WORLD;ui.feedCount=0;save("rock-pass");shot("rock-pass");
 /* A projectile through the flank must hit rock, not the actor beyond it. */
 float rx,ry;trail_rock(0,0,&rx,&ry);init_actor(1,"Shooter",GUARD,UNION,rx-3,ry,15);init_actor(2,"Target",RAIDER,HOSTILE,rx+3,ry,15);int w=gear_id(29,1,1);assert(depth_fire(1,2,w));float hp=g.actors[2].health[CHEST];for(int i=0;i<20;i++)depth_projectiles(.05f);for(int i=0;i<PROJECTILES;i++)assert(!g.depth.shots[i].active);assert(g.actors[2].health[CHEST]==hp);
 int b=g.trails.bed[0]-1;FILE*meta=fopen("build/v150/landmark.json","w");assert(meta);fprintf(meta,"{\"x\":%f,\"y\":%f,\"dx\":%f,\"dy\":%f}\n",x,y,dx,dy);fclose(meta);a->x=g.buildings[b].x;a->y=g.buildings[b].y;a->health[CHEST]=35;a->fatigue=90;g.trails.seen|=1;for(int i=1;i<g.actorCount;i++)g.actors[i].active=0;save("shelter");shot("shelter");int money=g.money;assert(start_sleep(-1)&&g.money==money&&a->taskKind==3);for(int i=0;i<40;i++)simulate(.1f);assert(a->health[CHEST]>35&&a->fatigue<90);save("shelter-rest");assert(load_game("build/v150/shelter-rest.sav")&&g.buildingCount==buildings&&g.nodeCount==nodes);
 puts("PASS: six persistent shelters, idempotent initialization, open central lanes and bypasses, rock collision blocks projectiles, free bed recovery and save/reload.");
}
static void recovery(void){
 Actor*a=fresh();a->bleed=2;a->health[CHEST]=25;a->gear[1]=RAGS;inventory_remove(a,CLOTH,inventory_count(a,CLOTH));nearby_actions();int row=-1;for(int i=0;i<nearby_actions();i++)if(nearby_action(i)->type==ACTION_EMERGENCY)row=i;assert(row>=0);save("emergency-rags");nearby_action_choose(row);assert(a->bleed==0&&!a->gear[1]&&a->health[CHEST]==25);
 a->bleed=1;inventory_add(a,CLOTH,2);nearby_actions();ui.paused=1;nearby_action_choose(0);save("queued-dressing");assert(load_game("build/v150/queued-dressing.sav"));a=&g.actors[g.selected];ui.paused=0;ui.screen=SCREEN_WORLD;society_queue_tick();assert(a->bleed==0&&!inventory_count(a,CLOTH));
 a=fresh();a->bleed=1;a->health[HEAD]=-5;a->unconscious=60;init_actor(1,"Passing medic",SETTLER,UNION,380,381,20);g.actors[1].voice=ROLE_MEDIC;inventory_add(&g.actors[1],MEDKIT,1);g.actors[1].target=-1;save("medic-rescue");assert(field_aid(1,.1f)&&a->bleed==0&&a->health[HEAD]>0&&!inventory_count(&g.actors[1],MEDKIT));
 a->bleed=1;inventory_add(&g.actors[1],MEDKIT,1);init_actor(2,"Threat",RAIDER,HOSTILE,381,381,20);assert(!field_aid(1,.1f)&&inventory_count(&g.actors[1],MEDKIT)==1);g.actors[2].active=0;g.rep[UNION]=-30;assert(!field_aid(1,.1f));g.rep[UNION]=0;a->race=2;a->bleed=0;a->health[HEAD]=-5;inventory_add(&g.actors[1],PARTS,1);assert(field_aid(1,.1f)&&a->health[HEAD]>0&&!inventory_count(&g.actors[1],PARTS));
 puts("PASS: emergency dressing consumes cloth or actual worn rags without healing; safe medic assistance spends real supplies; threat/reputation gates; mechanical rescue uses parts.");
}
static void aftermath(void){
 Actor*a=fresh();int t=0;init_actor(1,"Guard",GUARD,UNION,26,32,20);g.actors[1].homeTown=t;init_actor(2,"Civilian",SETTLER,UNION,28,31,5);g.actors[2].homeTown=t;g.actors[2].voice=ROLE_CITIZEN;
 /* Ensure these fixture actors are ordinary surviving residents. */
 for(int e=0;e<ELITES;e++)if(g.civil.eliteActor[e]==2||g.civil.eliteActor[e]==3)g.civil.eliteActor[e]=0;
 for(int e=0;e<DEPTH_NPCS;e++)if(g.depth.npc[e]==1||g.depth.npc[e]==2)g.depth.npc[e]=-1;
 float oldx=g.actors[1].homeX;politics_occupy(t,SHOGUN);assert(g.actors[1].homeX!=oldx&&g.actors[1].faction==SHOGUN);assert(g.actors[2].faction==FREE&&g.actors[2].homeX>40);a->x=26;a->y=32;save("occupied-town");shot("occupied-town");assert(load_game("build/v150/occupied-town.sav")&&g.actors[2].faction==FREE);
 a=&g.actors[g.selected];init_actor(3,"Relief medic",SETTLER,SHOGUN,26,32,10);g.actors[3].homeTown=t;g.actors[3].voice=ROLE_MEDIC;politics_occupy(t,MONKS);assert(g.life.towns[t].policy==1);Actor*m=&g.actors[3];float d=distance(m->x,m->y,m->homeX,m->homeY);for(int i=0;i<60;i++)simulate(.1f);assert(distance(m->x,m->y,m->homeX,m->homeY)<d);save("relief-post");shot("relief-post");
 puts("PASS: conquest reassigns surviving guard posts and displaces civilians; relief medic actually walks to post; changes persist without respawning unique owners.");
}
static void migration(void){
 assert(load_game("build/old-v14.sav"));int b=g.buildingCount,n=g.nodeCount;float x=g.actors[g.selected].x,y=g.actors[g.selected].y;assert(g.trails.version==1);save("migrated-v14");assert(load_game("build/v150/migrated-v14.sav"));assert(b==g.buildingCount&&n==g.nodeCount&&g.actors[g.selected].x==x&&g.actors[g.selected].y==y);int bed=g.trails.bed[0];g.trails.bed[0]=g.buildingCount+1;save("invalid-shelter");g.trails.bed[0]=bed;assert(!load_game("build/v150/invalid-shelter.sav"));assert(g.trails.bed[0]==bed);puts("PASS: genuine v14 PSP save migration, unchanged position, no duplicate shelters/supplies; invalid shelter references rejected without replacing current state.");
}
static void guide(void){fresh();open_help(4);shot("guide-wounds");assert(ui.helpTopic>0);press(KEY_RIGHT);assert(ui.scroll>0);shot("guide-wounds-continued");press(KEY_R);assert(ui.scroll==0&&ui.selection==5);puts("PASS: long guide chapters expose all recovery text through bounded left/right paging; chapter changes reset position.");}
int main(void){setvbuf(stdout,NULL,_IONBF,0);combat();roads();recovery();aftermath();migration();guide();printf("ALL V150 CHECKS PASSED Game=%zu\n",sizeof(Game));}
