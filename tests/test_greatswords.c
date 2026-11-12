#include "game.h"
#include "arsenal_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 game_new(73819,0);assert(g.greatswords.version==1);
 for(int i=100;i<120;i++){assert(arsenalDefs[i].type==32&&arsenalDefs[i].family==84);assert(weapon_style(WEAPON_ITEMS+i)==WS_HEAVY);}
 assert(load_game("build/old-v31.sav"));Actor kept=g.actors[g.selected];int money=g.money;
 /* The actual v31 fixture migrates once, without changing acquired weapons. */
 assert(g.greatswords.version==1&&kept.gear[0]==WEAPON_ITEMS);
 g.actors[0].gear[0]=WEAPON_ITEMS+119;inventory_add(&g.actors[0],WEAPON_ITEMS+117,1);
 *market_stock(0,WEAPON_ITEMS+100)=0;*market_stock(0,WEAPON_ITEMS+7)=0;
 assert(save_game("build/arsenal/greatsword-roundtrip.sav")&&load_game("build/arsenal/greatsword-roundtrip.sav"));
 assert(g.money==money&&g.actors[0].gear[0]==WEAPON_ITEMS+119&&inventory_count(&g.actors[0],WEAPON_ITEMS+117));
 assert(!*market_stock(0,WEAPON_ITEMS+100)&&!*market_stock(0,WEAPON_ITEMS+7));
 puts("PASS 20 appended greatswords; true two-handed classes; genuine v31 migration; new and old item/stock persistence.");
}
