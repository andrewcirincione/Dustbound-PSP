#include "../src/render.c"
#include <assert.h>
#include "frontier_fixture.h"
extern int add_build(float,float,int,int,int);
static int station;
static Actor *setup(void){
 game_new(73819,0);Actor*a=&g.actors[g.selected];memset(a->pack,0,sizeof(a->pack));a->x=a->y=400;a->autoattack=0;
 for(int n=0;n<g.actorCount;n++)if(n!=g.selected)g.actors[n].active=0;
 station=add_build(400.7f,400,FORGE,FREE,g.selected);assert(station>=0);g.depth.buildings[station].condition=100;g.depth.buildings[station].labor=0;g.depth.buildings[station].fuel=0;
 fixture_door(a,station);assert(interior_enter(station));fixture_service(ROOM_FORGE);ui.screen=SCREEN_WORLD;ui.camX=a->x;ui.camY=a->y;return a;
}
int main(void){setvbuf(stdout,NULL,_IONBF,0);Actor*a=setup();CraftRequirements r;char reason[100];
 assert(recipe_requirements(0,&r)&&r.qty==1&&r.item[0]==ORE&&r.need[0]==3&&r.station==FORGE);assert(!craft_readiness(&r,reason,sizeof(reason))&&strstr(reason,"Missing 3"));
 inventory_add(a,ORE,3);assert(!craft_readiness(&r,reason,sizeof(reason))&&strstr(reason,"coal or timber"));assert(!craft_item(0)&&inventory_count(a,ORE)==3);
 g.depth.buildings[station].fuel=1;assert(craft_readiness(&r,reason,sizeof(reason))&&craft_item(0));assert(inventory_count(a,ORE)==0&&inventory_count(a,IRONBAR)==1&&g.depth.buildings[station].fuel==0);
 a=setup();interior_leave();a->x=400;a->y=400;g.buildings[station].type=CAMPFIRE;g.buildings[station].owner=-1;inventory_add(a,MEAT,2);inventory_add(a,WATER,1);assert(recipe_requirements(1,&r)&&r.alternate==MEAT&&r.alternateNeed==2);assert(craft_readiness(&r,reason,sizeof(reason)));assert(craft_item(1)&&inventory_count(a,RATION)==r.qty&&inventory_count(a,MEAT)==0&&inventory_count(a,WATER)==0);
 a=setup();g.tech=3;a->xp[SMITHING]=12*35*35;g.depth.buildings[station].fuel=2;
 ui.forgeSlot=0;ui.forgeFamily=89;ui.forgeMaterial=0;ui.forgeQuality=2;int it=forge_selection_item();assert(it==RAPIER_ITEMS+2&&forge_requirements(it,&r));
 for(int i=0;i<r.count;i++)inventory_add(a,r.item[i],r.need[i]);assert(craft_readiness(&r,reason,sizeof(reason))&&forge_equipment(it));for(int i=0;i<r.count;i++)assert(!inventory_count(a,r.item[i]));assert(inventory_count(a,it)==1&&g.depth.buildings[station].fuel==1);
 assert(!forge_requirements(REGALIA_ITEMS,&r)&&!forge_equipment(REGALIA_ITEMS));
 for(int slot=0;slot<2;slot++){ui.screen=SCREEN_FORGE;ui.forgeSlot=slot;ui.forgeFamily=slot?32:0;ui.selection=1;int count=slot?20:38;for(int n=0;n<count;n++){assert(forge_requirements(forge_selection_item(),&r));assert(itemDefs[r.output].slot==slot);expansion_input(.033f,(Input){KEY_RIGHT,KEY_RIGHT,0,0});}assert(ui.forgeFamily==(slot?32:0));}
 puts("PASS: displayed recipe quantities, alternate cooking inputs, missing materials/fuel, rollback, output counts, exact forging deductions, all 38 weapon/20 torso designs, named regalia cannot be forged.");
 a=setup();assert(save_game("build/v100/craft-missing.sav"));
 inventory_add(a,ORE,6);inventory_add(a,IRONBAR,12);inventory_add(a,WOOD,6);inventory_add(a,CLOTH,6);inventory_add(a,WATER,3);inventory_add(a,GRAIN,3);inventory_add(a,RELIC,2);inventory_add(a,COAL,2);g.tech=3;a->xp[SMITHING]=12*35*35;g.depth.buildings[station].fuel=3;assert(save_game("build/v100/craft-ready.sav"));
 a->x+=10;assert(save_game("build/v100/craft-away.sav"));
}
