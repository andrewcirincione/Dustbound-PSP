#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include "frontier_fixture.h"
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
 static const int models[]={109,119};
 for(int model=0;model<2;model++)for(int facing=0;facing<2;facing++){int i=models[model];
  arena(1);Actor*a=&g.actors[0];int target=a->target;
  a->gear[0]=a->attackWeapon=WEAPON_ITEMS+i;a->gear[5]=NONE;armor_equip(a,(i-100)%ARMOR_SETS);
  a->xp[STRENGTH]=12*79*79;g.society.weaponXP[0][weapon_class(a->gear[0])]=12*39*39;
  memset(a->pack,0,sizeof(a->pack));inventory_add(a,WEAPON_ITEMS+i,1);a->autoattack=0;
  Actor*b=&g.actors[target];b->gear[0]=NONE;b->gear[2]=NONE;b->cooldown=1000;
  for(int l=0;l<LIMBS;l++)b->health[l]=1000;b->blood=100;b->bleed=0;
  b->x=a->x+(facing?.9f:-.9f);b->y=a->y-.1f;a->facing=facing;
  g.dayTime=12;g.weather=0;g.weatherTimer=g.eventTimer=10000;
  char name[64];snprintf(name,sizeof(name),"greatsword-%03d-%d",i,facing);save(name);
 }
 fresh();int forge=-1;for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].type==FORGE&&g.buildings[b].owner<0&&building_town(b)>=0){forge=b;break;}assert(forge>=0);
 in_room(forge,4,4);fixture_service(ROOM_SMITH_TRADE);
 for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
 save("smith-preview");interior_interact();assert(ui.screen==SCREEN_TRADE);int weaponRow=-1,armorRow=-1;
 for(int row=0;row<shop_count();row++){int it=shop_item(row);if(arsenal_type(it)==32&&weaponRow<0)weaponRow=row;if(itemDefs[it].slot==1&&armorRow<0)armorRow=row;}assert(weaponRow>=0&&armorRow>=0);
 char metadata[512];snprintf(metadata,sizeof(metadata),"%s/smith-preview.json",root);FILE*fp=fopen(metadata,"w");assert(fp);fprintf(fp,"{\"weapon_row\":%d,\"armor_row\":%d,\"screen\":%d}",weaponRow,armorRow,SCREEN_TRADE);fclose(fp);

 fresh();Actor*stance=&g.actors[0];int stage=g.wilderness.building[0]-1;assert(stage>=0);stance->x=g.buildings[stage].x+2;stance->y=g.buildings[stage].y+3;stance->gear[0]=stance->attackWeapon=WEAPON_ITEMS+109;stance->target=stance->attackTarget=-1;stance->autoattack=0;
 for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];e->x=e->y=1200;g.population.state[k].siteX=g.population.state[k].siteY=1200;for(int m=0;m<e->count;m++)e->reserve[m].x=e->reserve[m].y=1200;}
 save("greatsword-stance");
 for(int i=0;i<3;i++){
  fresh();for(int t=0;t<6;t++)population_tick(1);int b=g.wilderness.building[i]-1;assert(b>=0);Actor*a=&g.actors[0];
  a->x=g.buildings[b].x+2;a->y=g.buildings[b].y+3;a->gear[0]=WEAPON_ITEMS+109;a->autoattack=0;
  char name[64];snprintf(name,sizeof(name),"landmark-%d-exterior",i);save(name);
  actor_enter_room(0,b);for(int t=0;t<2;t++)population_tick(1);snprintf(name,sizeof(name),"landmark-%d-interior",i);save(name);
 }
 for(int camp=0;camp<2;camp++){
  fresh();Actor*a=&g.actors[0];a->x=a->y=380;a->gear[0]=WEAPON_ITEMS+109;a->autoattack=0;
  for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
  int k=WAR_POP_GROUPS;population_generate(k,camp?POP_SCAVENGERS:POP_RAIDERS);WorldEvent*e=&g.population.groups[k];float dx=383-e->x,dy=380-e->y;e->x=383;e->y=380;e->count=2;
  g.population.state[k].siteX=e->x;g.population.state[k].siteY=e->y;g.population.state[k].beds=camp?2:0;
  for(int m=0;m<e->count;m++){e->reserve[m].x+=dx;e->reserve[m].y+=dy;e->reserve[m].homeX=e->reserve[m].x;e->reserve[m].homeY=e->reserve[m].y;}
  assert(population_materialize(k));save(camp?"wilderness-camp":"wilderness-bandits");
 }
 puts("Prepared four shoulder-rest greatsword loadouts for equipped and real combat PSP review.");return 0;
}
