#include "game.h"
#include "arsenal_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
static int options(int town,int venue,int*weapons,int*armor){int n=0;*weapons=*armor=0;for(int it=1;it<ITEMS;it++)if(*market_stock(town,it)>0&&*market_venue(town,it)==venue){n++;*weapons+=itemDefs[it].slot==0;*armor+=itemDefs[it].slot>=1&&itemDefs[it].slot<=4;}return n;}
int main(void){
 game_new(73819,0);assert(g.arsenal.version==1);int covered[38]={0},legends=0;
 for(int i=0;i<ARSENAL_WEAPONS;i++){
  int it=WEAPON_ITEMS+i;const ArsenalDef*z=&arsenalDefs[i];const ItemDef*d=&itemDefs[it];
  assert(arsenal_index(it)==i&&arsenal_type(it)==z->type&&d->slot==0&&d->power>0&&d->cooldown>.2f&&d->weight>0);covered[z->type]++;legends+=z->tier==4;
  assert(armor_item_set(it)<0);assert(weapon_rapier(it)==(z->type==37));assert(weapon_ranged(it)==(z->type>=24&&z->type<=31));
  for(int j=0;j<i;j++)assert(strcmp(z->name,arsenalDefs[j].name));
 }
 for(int t=0;t<38;t++)assert(t==32?covered[t]==23:covered[t]>=2&&covered[t]<=3);assert(legends==12);
 for(int type=0;type<38;type++){
  int lastPower=0,seen=0;
  for(int tier=0;tier<=4;tier++){
   Actor a=g.actors[0];a.squad=0;a.kind=RAIDER;a.faction=HOSTILE;
   a.gear[0]=type==36?SCIMITAR_ITEMS:type==37?RAPIER_ITEMS:gear_id(weapon_family_index(type),0,0);
   arsenal_outfit(&a,1234,tier);int idx=arsenal_index(a.gear[0]);
   if(idx>=0){assert(arsenalDefs[idx].type==type&&arsenalDefs[idx].tier<=tier);assert(itemDefs[a.gear[0]].power>=lastPower);lastPower=itemDefs[a.gear[0]].power;seen++;}
  }assert(seen>=4);
 }
 int rare=0;for(int day=1;day<=18;day++){
  g.day=day;for(int t=0;t<TOWNS;t++){
   restock_town(t);if(!town_design(t)->smith)continue;int w,a,budget=15+10*town_design(t)->importance;
   assert(options(t,TRADE_SMITH,&w,&a)==budget);assert(w==budget*3/5&&a==budget-w);assert(w>=8&&a>=6);
   assert(g.arsenal.stockDay[t]==day);
   int greats=0;for(int j=100;j<120;j++)greats+=*market_stock(t,WEAPON_ITEMS+j)>0&&*market_venue(t,WEAPON_ITEMS+j)==TRADE_SMITH;assert(greats>=(town_design(t)->importance>=TOWN_FORTRESS?2:1));
   for(int i=0;i<ARSENAL_WEAPONS;i++)if(*market_stock(t,WEAPON_ITEMS+i)>0&&arsenalDefs[i].tier==4){assert(town_design(t)->importance==TOWN_CAPITAL);rare++;}
  }
 }assert(rare>0);
 /* Selling out persists during the day, and midnight replenishes it. */
 g.day=25;g.dayTime=23.9995f;g.lastRestock=g.day;restock();
 int t=0,it=-1;for(int i=WEAPON_ITEMS;i<ITEMS;i++)if(*market_stock(t,i)>0){it=i;break;}assert(it>=0);*market_stock(t,it)=0;
 int beforeDay=g.day;g.actors[g.selected].autoattack=0;game_tick(.2f,(Input){0});assert(g.day==beforeDay+1&&g.lastRestock==g.day);
 for(int town=0;town<TOWNS;town++)if(town_design(town)->smith){int w,a;assert(options(town,TRADE_SMITH,&w,&a)==15+10*town_design(town)->importance);assert(g.arsenal.stockDay[town]==g.day);}
 /* Preserve acquired weapons, stock quantities and empty hands over save/load. */
 mkdir("build/arsenal",0777);g.actors[0].gear[0]=WEAPON_ITEMS+99;inventory_add(&g.actors[0],WEAPON_ITEMS+47,1);*market_stock(0,WEAPON_ITEMS+38)=0;
 int elite=g.civil.eliteActor[0]-1;g.actors[elite].gear[0]=0;g.actors[elite].dead=1;
 assert(save_game("build/arsenal/roundtrip.sav"));assert(load_game("build/arsenal/roundtrip.sav"));
 assert(g.actors[0].gear[0]==WEAPON_ITEMS+99&&inventory_count(&g.actors[0],WEAPON_ITEMS+47)==1);assert(!*market_stock(0,WEAPON_ITEMS+38)&&!g.actors[elite].gear[0]);
 assert(load_game("build/old-v30.sav"));assert(g.arsenal.version==1&&g.actors[g.selected].gear[0]<WEAPON_ITEMS);
 for(int town=0;town<TOWNS;town++)if(town_design(town)->smith){int w,a;assert(options(town,TRADE_SMITH,&w,&a)==15+10*town_design(town)->importance);}
 puts("PASS 120 weapons / 38 types; strength/rarity ordering; 15–45 smith options; legendary weapons only in capitals; real midnight refresh and save/load persistence.");
}
