#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include "frontier_fixture.h"
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
 static const int models[]={100,102,109,119};
 for(int model=0;model<4;model++)for(int facing=0;facing<2;facing++){int i=models[model];
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
 puts("Prepared eight raised-shoulder greatsword loadouts for equipped and real combat PSP review.");return 0;
}
