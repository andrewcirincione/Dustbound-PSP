#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int pieces(const Actor*a){int n=0;for(int p=1;p<5;p++)n+=armor_item_set(a->gear[p])>=0;return n;}
int main(void){
 game_new(73819,0);
 for(unsigned seed=0;seed<384;seed++)for(int tier=0;tier<4;tier++){
  Actor a=g.actors[0];a.squad=0;a.kind=RAIDER;a.faction=HOSTILE;
  memset(a.gear,0,sizeof(a.gear));a.gear[1]=RAGS;
  armor_wanderer(&a,seed,tier);
  assert(pieces(&a)<=(tier<=1?2:4));
  for(int p=1;p<5;p++)if(armor_item_set(a.gear[p])>=0){int set=armor_item_set(a.gear[p]);assert(set<26);if(tier<=1)assert(set==16||set==19);}
 }
 int found[30]={0},legendary=0,ordinary=0;
 for(unsigned seed=0;seed<4096;seed++){
  int item=armor_cache_item(seed,3),set=armor_item_set(item);assert(set>=0);found[set]++;
  if(set>=26){legendary++;assert(item>=ARMOR_SET_ITEMS && (item-ARMOR_SET_ITEMS)%4!=0);}else ordinary++;
 }
 assert(legendary==128 && ordinary>legendary*20);for(int s=26;s<30;s++)assert(found[s]);
 int inCache=0;
 for(int b=0;b<g.buildingCount;b++)if(armor_item_set(g.buildings[b].storage[15].item)>=26){
  assert(ruined_building(b));assert(distance(g.buildings[b].x,g.buildings[b].y,towns[0].x,towns[0].y)>300);inCache++;
 }
 assert(inCache==2);
 for(int s=26;s<30;s++){
  int owner=regalia_owner(armor_set_item(s,0)),n=g.civil.eliteActor[owner]-1;assert(n>=0);
  for(int p=0;p<4;p++){static const int slots[]={1,2,4,3};assert(g.actors[n].gear[slots[p]]==armor_set_item(s,p));}
  assert(itemDefs[armor_set_item(s,1)].price>12000);
 }
 int rareStock=0,regionalStock=0;
 for(int day=1;day<=52;day++)for(int town=0;town<TOWNS;town++){
  g.day=day;armor_stock(town);int legends=0;
  for(int it=ARMOR_SET_ITEMS;it<WEAPON_ITEMS;it++)if(*market_stock(town,it)>0){
   int set=armor_item_set(it),f=armor_set_faction(set);
   if(set>=26){legends++;rareStock++;assert(town_design(town)->importance==TOWN_CAPITAL);assert((it-ARMOR_SET_ITEMS)%4!=0);}
   else{regionalStock++;assert(f==g.townOwner[town]||f==FREE||f==SCAVENGERS);}
  }
  assert(legends<=1);
 }
 assert(rareStock>0 && regionalStock>rareStock*10);
 /* Migration/load may not refill dead elites, stripped actors, or claimed caches. */
 int owner=regalia_owner(armor_set_item(26,0)),n=g.civil.eliteActor[owner]-1;
 g.actors[n].dead=1;g.actors[n].gear[1]=NONE;
 for(int b=0;b<g.buildingCount;b++)if(armor_item_set(g.buildings[b].storage[15].item)>=26)g.buildings[b].storage[15]=(Stack){0};
 armor_world_init();assert(g.actors[n].gear[1]==NONE);
 assert(save_game("build/armor-sets/distribution.sav"));assert(load_game("build/armor-sets/distribution.sav"));
 assert(g.actors[n].gear[1]==NONE);
 for(int b=0;b<g.buildingCount;b++)assert(armor_item_set(g.buildings[b].storage[15].item)<26);
 assert(g.armor.version==2);
 printf("PASS: sparse mixed raiders, unique elite bodies, two remote nonrenewing caches, %d rare city pieces / %d regional stock entries; save/load keeps claimed loot removed.\n",rareStock,regionalStock);
}
