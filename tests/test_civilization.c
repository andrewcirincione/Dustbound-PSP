#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern float movement_speed(const Actor*);
extern int add_build(float,float,int,int,int);
static Actor *fresh(void){game_new(73819,0);ui.screen=SCREEN_WORLD;return &g.actors[g.selected];}
static void species_checks(void){
 for(int origin=0;origin<5;origin++)for(int race=0;race<SPECIES;race++){
  fresh();character_begin(origin);ui.createRace=race;character_finish();Actor*a=&g.actors[g.selected];
  assert(a->race==race&&a->origin==origin);for(int s=0;s<SKILLS;s++)assert(level(a,s)==species_start_level(origin,race,s));
  for(int l=0;l<LIMBS;l++)assert(a->health[l]==speciesDefs[race].health);
  float xp=a->xp[MELEE];character_begin(-1);ui.createHair=(ui.createHair+1)%7;character_finish();assert(a->xp[MELEE]==xp&&a->race==race);
 }
 Actor*a=fresh();memset(a->gear,0,sizeof(a->gear));memset(a->pack,0,sizeof(a->pack));float speed[SPECIES];int capacity[SPECIES];
 for(int r=0;r<SPECIES;r++){a->race=r;speed[r]=movement_speed(a);capacity[r]=actor_capacity(a);}
 assert(speed[3]>speed[0]&&speed[0]>speed[4]&&capacity[4]>capacity[0]&&capacity[0]>capacity[3]);
 for(int r=0;r<SPECIES;r++){
  a=fresh();a->race=r;for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);a->health[RARM]-=10;
  inventory_add(a,MEDKIT,5);inventory_add(a,PARTS,5);a->bleed=0;
  assert(r==2?depth_repair(g.selected,g.selected):heal_actor(g.selected,g.selected));
  assert(a->health[RARM]<=species_health(a)&&a->health[RARM]>=species_health(a)-1);
  if(r!=2)assert(!heal_actor(g.selected,g.selected));
  for(int i=0;i<g.actorCount;i++)if(i!=g.selected)g.actors[i].active=0;
  a->x=a->y=400;a->hunger=80;float food=a->hunger;simulate(.1f);
  assert(r==2?a->hunger==100:fabsf(a->hunger-(food-.0045f*speciesDefs[r].food))<.001f);
 }
 puts("PASS: all 25 origin/species combinations, barber preservation, movement/carry/appetite and full species-aware healing/repair.");
}
static void elite_checks(void){
 Actor*a=fresh();assert(V100_ITEMS==2368&&ITEMS>V100_ITEMS&&g.actorCount<=176);assert(g.buildingCount<MAX_BUILDINGS);
 for(int e=0;e<ELITES;e++){
  int n=g.civil.eliteActor[e]-1;assert(n>=0);Actor*b=&g.actors[n];assert(!strcmp(b->name,eliteDefs[e].name)&&civil_elite_index(n)==e);
  assert(b->gear[0]==REGALIA_ITEMS+e*2&&b->gear[1]==REGALIA_ITEMS+e*2+1);assert(level(b,MELEE)>=42);
  for(int k=0;k<2;k++){int it=REGALIA_ITEMS+e*2+k;assert(regalia_owner(it)==e&&gear_family(it)==(k?eliteDefs[e].armorFamily:eliteDefs[e].weaponFamily));assert(strstr(itemDefs[it].desc,b->name));for(int t=0;t<TOWNS;t++)assert(!g.stock[t][it]);}
 }
 for(int r=0;r<4;r++)for(int v=0;v<24;v++){int it=CIV_ARMOR_ITEMS+r*24+v;assert(itemDefs[it].slot==1&&itemDefs[it].armor>0);for(int other=CIV_ARMOR_ITEMS;other<it;other++)assert(strcmp(itemDefs[it].name,itemDefs[other].name));}
 int n=g.civil.eliteActor[12]-1;Actor*b=&g.actors[n];float x=b->x,y=b->y;for(int t=0;t<20;t++)civil_tick(1);assert(distance(x,y,b->x,b->y)>.5f);
 n=g.civil.eliteActor[0]-1;b=&g.actors[n];b->dead=1;a->x=b->x;a->y=b->y;int prosperity=g.prosperity[11],it=b->gear[0];ui.lootActor=n;
 assert(loot_take(PACK,0)&&!b->gear[0]&&inventory_count(a,it)==1);assert(!loot_take(PACK,0));civil_tick(1);assert(g.civil.fallen[0]&&g.prosperity[11]==prosperity-12);
 int count=g.buildingCount;assert(save_game("build/v100-roundtrip.sav"));assert(load_game("build/v100-roundtrip.sav"));assert(g.buildingCount==count&&g.civil.fallen[0]&&!g.actors[n].gear[0]&&inventory_count(&g.actors[g.selected],it)==1);civil_init();assert(!g.actors[n].gear[0]);
 printf("PASS: 18 distinct owners, 36 physical named items, 96 armor variants, traveling elites, no loot refill after death/load; %d actors / %d buildings.\n",g.actorCount,g.buildingCount);
}
static unsigned char reachable[81][81];static short queue[81*81][2];
static void town_checks(void){
 fresh();
 for(int t=1;t<TOWNS;t++){
  memset(reachable,0,sizeof(reachable));int first=0,last=0;
  for(int k=0;k<81;k++)if(can_walk(towns[t].x-20+k*.5f,towns[t].y+20)){queue[last][0]=k;queue[last++][1]=80;reachable[80][k]=1;break;}
  assert(last);
  while(first<last){int x=queue[first][0],y=queue[first++][1];static const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};for(int d=0;d<4;d++){int nx=x+dx[d],ny=y+dy[d];if(nx<0||ny<0||nx>80||ny>80||reachable[ny][nx]||!can_walk(towns[t].x-20+nx*.5f,towns[t].y-20+ny*.5f))continue;reachable[ny][nx]=1;queue[last][0]=nx;queue[last++][1]=ny;}}
  for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];int role=npc_role(a);if(a->homeTown!=t||a->squad||a->prisonTime||g.depth.actors[n].captive||civil_elite_index(n)>=0||depth_named(n)>=0||regional_named(n)||living_owned(n))continue;if(role!=ROLE_MERCHANT&&role!=ROLE_INNKEEPER&&role!=ROLE_MEDIC&&role!=ROLE_SMITH)continue;
   int x=lroundf((a->x-towns[t].x+20)*2),y=lroundf((a->y-towns[t].y+20)*2);if(x<0||x>80||y<0||y>80||!reachable[y][x]||!can_walk(a->x,a->y)){fprintf(stderr,"Inaccessible service %s in %s at %.2f,%.2f\n",a->name,towns[t].name,a->x-towns[t].x,a->y-towns[t].y);assert(0);}
  }
  if(civil_style(t)==1){assert(can_walk(towns[t].x,towns[t].y+12)&&can_walk(towns[t].x,towns[t].y-12));for(int step=-20;step<=20;step++)assert(!can_walk(towns[t].x+12,towns[t].y+step*.5f));}
 }
 puts("PASS: all 15 rebuilt settlements have accessible services; both fortress gate axes remain open.");
}
static void migration_checks(void){
 fresh();assert(load_game("build/old-v9.sav"));assert(g.civil.version==1&&civil_validate(&g));
 Actor*a=&g.actors[g.selected];assert(g.money==4000&&inventory_count(a,RAPIER_ITEMS+7)==1&&a->gear[1]==gear_id(39,0,1));
 int count=g.buildingCount;assert(save_game("build/v100-migrated.sav"));assert(load_game("build/v100-migrated.sav"));assert(g.buildingCount==count);
 /* Applying culture to an older save must preserve a player-built station. */
 fresh();int b=add_build(towns[11].x+5,towns[11].y+4,FORGE,FREE,g.selected);assert(b>=0);Building own=g.buildings[b];memset(&g.civil,0,sizeof(g.civil));civil_init();assert(!memcmp(&own,&g.buildings[b],sizeof(own)));
 printf("PASS: actual v9 -> v10 migration, repeated reload idempotence, player-built structures preserved; Game=%zu bytes.\n",sizeof(Game));
}
static void fixtures(void){
 /* Town views use ordinary saves; the emulator supplies only controller input. */
 int townsForSlot[]={11,1,7,8,14,15};
 for(int slot=0;slot<6;slot++){Actor*a=fresh();int t=townsForSlot[slot];a->x=towns[t].x;a->y=towns[t].y+5;a->autoattack=0;a->sneaking=0;for(int f=0;f<FACTIONS;f++)g.rep[f]=80;
  a->gear[1]=gear_id(t==7||t==15?39:t==14?37:34,2,1);a->gear[2]=gear_id(t==7||t==15?57:t==14?55:50,2,1);
  g.dayTime=12;g.weather=0;ui.camX=a->x;ui.camY=a->y;char path[100];snprintf(path,sizeof(path),"build/v100/DUST_%d.sav",slot);assert(save_game(path));
 }
 Actor*a=fresh();int n=g.civil.eliteActor[12]-1;Actor*b=&g.actors[n];a->x=b->x;a->y=b->y+1;b->unconscious=999;b->bleed=0;a->autoattack=0;for(int k=0;k<PACK;k++)b->pack[k]=(Stack){0};a->gear[0]=gear_id(0,0,0);memset(a->pack,0,sizeof(a->pack));inventory_add(a,RATION,4);inventory_add(a,CIV_ARMOR_ITEMS+95,1);g.dayTime=12;g.weather=0;ui.camX=a->x;ui.camY=a->y;assert(save_game("build/v100/DUST_6.sav"));
}
int main(void){setvbuf(stdout,NULL,_IONBF,0);species_checks();elite_checks();town_checks();migration_checks();fixtures();puts("ALL CIVILIZATION CHECKS PASSED");}
