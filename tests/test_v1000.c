#include "game.h"
#include "settlement_art_ids.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
extern int add_actor(const char*,int,int,float,float,int);
static Actor *fresh(void){game_new(73819,0);g.selected=0;ui.screen=SCREEN_WORLD;ui.paused=0;ui.menuReturn=0;g.money=200000;g.dayTime=12;Actor*a=&g.actors[0];a->autoattack=0;a->target=-1;a->taskKind=0;return a;}
static void room(int b,float x,float y){Actor*a=&g.actors[0];actor_enter_room(0,b);g.interiorBuilding=b+1;g.interiorActor=0;g.interiorX=x;g.interiorY=y;g.depth.actors[0].rx=x;g.depth.actors[0].ry=y;a->taskKind=0;}
static void outdoors(float x,float y){g.interiorBuilding=0;g.depth.actors[0].room=0;g.innFloor[0]=0;g.actors[0].x=x;g.actors[0].y=y;}
static int home(void){for(int b=0;b<g.buildingCount;b++)if(home_state(b)==HOME_FOR_SALE)return b;assert(0);return -1;}
static int slot_item(Actor*a,int item){for(int i=0;i<PACK;i++)if(a->pack[i].item==item)return i;return -1;}
static void property(void){
 fresh();int b=home();g.homes.property[b]=HOME_OWNED;room(b,3.5f,5.9f);int cash=g.money;
 assert(home_place(b,0,FURN_CHEST,1.5f,1.5f));assert(g.money==cash-homeFurniture[FURN_CHEST].price);
 cash=g.money;assert(home_place(b,0,FURN_CHEST,1.5f,4.4f));assert(g.money==cash);
 assert(home_place(b,1,FURN_BED,5.4f,1.4f));cash=g.money;
 assert(!home_place(b,2,FURN_TABLE,1.5f,4.4f));assert(!home_place(b,2,FURN_TABLE,3.5f,6.0f));assert(g.money==cash);
 assert(home_place(b,7,FURN_SHELF,5.5f,4.5f));
 assert(save_game("build/v1000/property.sav"));assert(load_game("build/v1000/property.sav"));
 assert(g.plans.furniture[b][7]==FURN_SHELF&&fabsf(g.plans.y[b][0]-4.4f)<.001f);
 room(b,3.5f,5.9f);placement_open(FURN_TABLE,b);float before=g.time,x=ui.placeX;
 game_tick(.1f,(Input){.held=KEY_RIGHT});assert(g.time==before&&ui.placeX>x&&ui.screen==SCREEN_PLACE);
 game_tick(.1f,(Input){.pressed=KEY_CIRCLE});assert(ui.screen!=SCREEN_PLACE);
 puts("PASS home purchase furniture, free movement, overlap/door protection, extra slots, paused placement and save roundtrip.");
}
static void camps(void){
 Actor*a=fresh();memset(a->pack,0,sizeof(a->pack));inventory_add(a,WOOD,20);inventory_add(a,BEDROLL_ITEM,1);
 int found=0;float x=0,y=0;
 for(int iy=200;iy<1300&&!found;iy+=13)for(int ix=200;ix<1300;ix+=13){outdoors(ix-3,iy);if(placement_valid(0,-1,ix,iy)&&placement_valid(1,-1,ix,iy+3)){x=ix;y=iy;found=1;break;}}
 assert(found);int b=g.buildingCount;assert(place_camp(0,x,y));assert(place_camp(1,x,y+3));assert(inventory_count(a,BEDROLL_ITEM)==0);
 assert(g.buildings[b].type==CAMPFIRE&&g.buildings[b+1].type==BED&&portable_bedroll(b+1)>0);
 inventory_add(a,MEAT,2);inventory_add(a,WATER,1);outdoors(x-.9f,y);int foe=add_actor("Nearby raider",RAIDER,HOSTILE,x+2,y,3);assert(foe>=0);
 assert(craft_item(1));assert(inventory_count(a,RATION)>0);assert(!craft_item(5)&&!craft_item(6));
 g.time+=DAY_SECONDS+2;outdoors(x+100,y+100);placement_tick(1);
 assert(portable_bedroll(b)<0&&portable_bedroll(b+1)<0);
 puts("PASS chosen camp sites, bedroll item consumption, cooking beside a hostile, forbidden limb recipes, one-day camp expiry.");
}
static void depots(void){
 Actor*a=fresh();int depot[3],n=0;for(int b=0;b<g.buildingCount&&n<3;b++)if(depot_public(b))depot[n++]=b;assert(n>=2);
 for(int i=0;i<n;i++){int clerk=depot_clerk(depot[i]);assert(clerk>=0&&npc_role(&g.actors[clerk])==ROLE_DEPOT);int offered=0;for(int j=0;j<dialogue_count(clerk);j++)offered|=dialogue_action(clerk,j)==21;assert(offered);}
 memset(a->pack,0,sizeof(a->pack));inventory_add(a,IRONBAR,2);room(depot[0],3.4f,3.8f);int slot=slot_item(a,IRONBAR),fee=depot_fee(depot[0],0,slot),cash=g.money;
 assert(fee>0&&storage_transfer(depot[0],0,slot));assert(g.money==cash-fee&&inventory_count(a,IRONBAR)==1);
 room(depot[1],3.4f,3.8f);int stored=-1;for(int i=0;i<storage_slots(depot[1]);i++)if(storage_stack(depot[1],i)->item==IRONBAR){stored=i;break;}assert(stored>=0);
 cash=g.money;fee=depot_fee(depot[1],1,stored);assert(storage_transfer(depot[1],1,stored));assert(g.money==cash-fee&&inventory_count(a,IRONBAR)==2);
 g.money=0;assert(!storage_transfer(depot[1],0,slot_item(a,IRONBAR)));assert(inventory_count(a,IRONBAR)==2);
 puts("PASS staffed depot dialogue, shared cross-town inventory, quoted fees both ways, atomic insufficient-money failure.");
}
static void dispatch(void){
 Actor*a=fresh();memset(a->pack,0,sizeof(a->pack));for(int i=0;i<PACK;i++)a->pack[i]=(Stack){gear_id(i,0,0),1};
 assert(!accept_contract(3,CONTRACT_DISPATCH));memset(a->pack,0,sizeof(a->pack));assert(accept_contract(3,CONTRACT_DISPATCH));assert(inventory_count(a,DISPATCH_ITEM)==1);
 int k=-1;for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&g.contracts[i].type==CONTRACT_DISPATCH){k=i;break;}assert(k>=0);
 int target=g.contracts[k].target;outdoors(towns[target].x,towns[target].y);contract_tick();assert(g.contracts[k].progress==1&&inventory_count(a,DISPATCH_ITEM)==0);
 assert(!g.contracts[k].completed&&g.contracts[k].active==1);
 puts("PASS physical dispatch given on acceptance, pack-space check, delivered packet removed, return payment still required.");
}
static void bounty_and_loot(void){
 Actor*a=fresh();int target=add_actor("Test outlaw",RAIDER,HOSTILE,a->x+.6f,a->y,6);assert(target>=0);Actor*p=&g.actors[target];p->unconscious=100;p->bountyReward=500;ui.lootActor=target;
 p->gear[0]=SABRE;p->gear[1]=PLATE;g.depth.actors[target].layer=RAGS;
 assert(loot_take(PACK,0)&&p->gear[0]==0);assert(loot_take(PACK+1,0)&&p->gear[1]==0&&g.depth.actors[target].layer==RAGS);
 assert(limb_loot_item(p,LOOT_LAYER_SLOT)==RAGS);assert(loot_take(LOOT_LAYER_SLOT,0)&&g.depth.actors[target].layer==0);
 int jail=-1;for(int t=0;t<TOWNS;t++)if(g.field.jail[t]&&(bounty_payers(target)&(1u<<g.townOwner[t]))){jail=g.field.jail[t]-1;break;}assert(jail>=0);
 room(jail,3.5f,5.6f);a->carrying=target;p->carriedBy=0;g.depth.actors[target].room=jail+1;
 for(int i=1;i<g.actorCount;i++)if(i!=target&&g.actors[i].kind==GUARD)g.actors[i].active=0;
 int count=nearby_actions(),action=-1;for(int i=0;i<count;i++){const NearbyAction*x=nearby_action(i);if(x->type==ACTION_BOUNTY_TURN_IN&&x->kind==5)action=i;}assert(action>=0&&!nearby_action_reason(action));
 int cash=g.money,reward=bounty_reward(target,1);ui.selection=action;nearby_action_choose(action);
 assert(g.money==cash+reward&&a->carrying<0&&p->prisonTime>0&&p->bountyReward==0);assert(!bounty_turn_in_jail(jail));
 puts("PASS real enemy weapon, armor and underlayer removal; jail environment turn-in without any keeper; payout once.");
}
static void shops_and_inns(void){
 fresh();for(int h=0;h<24;h++){g.dayTime=h;assert(depth_trade_open(3));}
 int upstairs=0,smiths=0;
 for(int b=0;b<g.buildingCount;b++){
  if(inn_has_upstairs(b)){InteriorProp down[ROOM_PROPS],up[ROOM_PROPS];int n=interior_floor_props(b,0,down),m=interior_floor_props(b,1,up),asc=0,desc=0;for(int i=0;i<n;i++)asc|=down[i].action==ROOM_STAIRS&&down[i].art==ART_INN_STAIRS;for(int i=0;i<m;i++)desc|=up[i].action==ROOM_STAIRS&&up[i].art==ART_STAIRS_DOWN;assert(asc&&desc);upstairs++;}
  if(g.buildings[b].type==FORGE&&g.buildings[b].owner<0&&!ruined_building(b)){int t=building_town(b);if(t<0)continue;restock_town(t);int weapons=0,armor=0;for(int it=1;it<ITEMS;it++)if(g.frontier.venue[t][it]==TRADE_SMITH&&*market_stock(t,it)>0){weapons+=itemDefs[it].slot==0;armor+=itemDefs[it].slot>0&&itemDefs[it].slot<=4;}assert(weapons>=8&&armor>=6);smiths++;}
 }
 assert(upstairs&&smiths);printf("PASS markets open all 24 hours, %d smithies with expanded stock, %d inns with separate down stairwell.\n",smiths,upstairs);
}
static void corpse_cleanup(void){
 Actor*a=fresh();int n=add_actor("Discarded corpse",RAIDER,HOSTILE,1200,1200,2);assert(n>=0);Actor*p=&g.actors[n];p->dead=1;p->target=-1;p->bountyReward=0;g.time=500;g.plans.corpseSince[n]=1;
 population_prune_corpses();assert(!p->active&&g.living.owned[n]<0);
 int k=add_actor("Carried corpse",RAIDER,HOSTILE,a->x,a->y,2);assert(k>=0);p=&g.actors[k];p->dead=1;p->bountyReward=0;p->carriedBy=0;a->carrying=k;g.plans.corpseSince[k]=1;population_prune_corpses();assert(p->active);
 puts("PASS distant unclaimed corpse slots recycle; carried bodies remain protected.");
}
static void migration(const char*path){
 assert(load_game(path));int b=-1;for(int i=0;i<g.buildingCount;i++)if(home_state(i)==HOME_OWNED){b=i;break;}assert(b>=0&&g.plans.version==1);
 assert(g.plans.furniture[b][0]==FURN_CHEST&&g.plans.furniture[b][1]==FURN_BED);assert(g.homes.stash[0].item==IRONBAR&&g.homes.stash[0].qty==3&&g.homes.stolen[0]==UNION);
 assert(g.stock[1][RATION]==17);int p=portion_item(MEDKIT,63);assert(*market_stock(1,p)==4&&g.frontier.venue[1][p]==TRADE_GENERAL);
 placement_tick(1);assert(contract_item_count(DISPATCH_ITEM,-1)==1);assert(save_game("build/v1000/migrated.sav"));assert(load_game("build/v1000/migrated.sav"));
 puts("PASS actual v0.19.0 save migration: home bays, shared stash, stolen ownership, partial medical stock/venue, pending dispatch, v29 roundtrip.");
}
int main(int argc,char**argv){mkdir("build/v1000",0777);property();camps();depots();dispatch();bounty_and_loot();shops_and_inns();corpse_cleanup();if(argc>1)migration(argv[1]);puts("PASS Dust 1.0.0 integration checks.");return 0;}
