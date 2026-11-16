#include "frontier_fixture.h"
#include "../src/settlement_art_ids.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
extern int add_actor(const char*,int,int,float,float,int);
static int slot(Actor*a,int item){for(int i=0;i<PACK;i++)if(a->pack[i].item==item)return i;return -1;}
static void fresh(void){game_new(73819,0);ui.screen=SCREEN_WORLD;ui.paused=0;}
static void progression(void){
 fresh();Actor*a=&g.actors[0];memset(a->xp,0,sizeof(a->xp));
 const int combat[]={STRENGTH,DEXTERITY,MELEE,DEFENCE,DODGE,MARTIAL,CROSSBOWS,PERCEPTION};
 for(unsigned i=0;i<sizeof(combat)/sizeof(*combat);i++){gain(a,combat[i],20);assert(fabsf(a->xp[combat[i]]-30)<.01f);}
 gain(a,MEDICINE,20);gain(a,TOUGHNESS,20);assert(a->xp[MEDICINE]==40&&a->xp[TOUGHNESS]==40);
 int c=weapon_class(SABRE);g.society.weaponXP[0][c]=0;weapon_class_gain(a,SABRE,20);assert(g.society.weaponXP[0][c]==30);
 const int removed[]={FARMING,COOKING,ENGINEERING,SCIENCE};for(int i=0;i<4;i++){gain(a,removed[i],100);assert(!skill_enabled(removed[i])&&a->xp[removed[i]]==0);}
 assert(CORE_SKILL_ROWS==15&&buildCount==2);for(int i=2;i<8;i++)assert(!build_structure(i)&&!place_camp(i,a->x,a->y));
 assert(!strcmp(itemDefs[HIDE].desc,"Leatherworking and trade material."));
}
static void storage(void){
 fresh();int depots=0,excluded=0;
 for(int b=0;b<g.buildingCount;b++){
  int permitted=depot_public(b)||home_state(b)==HOME_OWNED;
  assert((storage_slots(b)>0)==permitted);depots+=depot_public(b);excluded+=expedition_site(b)>=0;
  if(expedition_site(b)>=0||wilderness_site(b)>=0||ruined_building(b))assert(!depot_public(b)&&!storage_slots(b));
 }
 assert(depots>0&&excluded==46);
}
static void aging(void){
 fresh();Actor*a=&g.actors[0];memset(a->pack,0,sizeof(a->pack));memset(a->gear,0,sizeof(a->gear));
 g.time=100;assert(inventory_add_owned(a,SABRE,1,UNION));int k=slot(a,SABRE);float deadline=stolen_until(a,k);assert(deadline==100+2*DAY_SECONDS);
 g.time+=DAY_SECONDS;use_item(k);assert(a->gear[0]==SABRE&&stolen_until(a,PACK)==deadline);
 assert(save_game("build/v122/theft.sav")&&load_game("build/v122/theft.sav"));a=&g.actors[0];assert(stolen_until(a,PACK)==deadline);
 use_item(PACK);k=slot(a,SABRE);assert(k>=0&&stolen_until(a,k)==deadline);
 int n=add_actor("Receiver",PLAYER,FREE,a->x,a->y,1);g.actors[n].squad=1;assert(inventory_transfer(a,&g.actors[n],k,1));int to=slot(&g.actors[n],SABRE);assert(stolen_until(&g.actors[n],to)==deadline);
 assert(inventory_transfer(&g.actors[n],a,to,1));k=slot(a,SABRE);
 assert(inventory_drop(a,k,1));int drop=g.nodeCount-1;assert(stolen_drop_until(drop)==deadline);
 interact_context(1,drop);k=slot(a,SABRE);assert(k>=0&&stolen_until(a,k)==deadline);
 int home=-1;for(int b=0;b<g.buildingCount;b++)if(home_state(b)==HOME_FOR_SALE){home=b;break;}assert(home>=0);
 g.homes.property[home]=HOME_OWNED;g.money=100000;actor_enter_room(0,home);assert(home_furnish(home,FURN_CHEST,FURN_CHEST));
 fixture_service(ROOM_HOME_STASH);assert(storage_transfer(home,0,k));assert(g.homes.stolen[0]==UNION&&stolen_home_until(0)==deadline);
 assert(save_game("build/v122/stash.sav")&&load_game("build/v122/stash.sav"));a=&g.actors[0];
 g.time=deadline-.01f;assert(stolen_home_until(0)==deadline&&g.homes.stolen[0]==UNION);
 g.time=deadline;assert(!stolen_home_until(0)&&!g.homes.stolen[0]);assert(storage_transfer(home,1,0));k=slot(a,SABRE);assert(k>=0&&!item_slot_owner(a,k));
 // Separately stolen batches retain separate deadlines instead of refreshing old theft.
 actor_leave_room(0);g.time=5000;assert(inventory_add_owned(a,WOOD,1,UNION));int first=slot(a,WOOD);float one=stolen_until(a,first);g.time+=100;assert(inventory_add_owned(a,WOOD,1,UNION));
 int second=-1;for(int i=0;i<PACK;i++)if(i!=first&&a->pack[i].item==WOOD)second=i;assert(second>=0);g.time=one;assert(!item_slot_owner(a,first)&&item_slot_owner(a,second)==UNION);
 g.time+=100;assert(!item_slot_owner(a,second));
}
static void interiors(void){
 fresh();int b=g.wilderness.building[5]-1;InteriorProp p[ROOM_PROPS];int beds[3]={0},stands[3]={0};
 unsigned long hash[3]={0};
 for(int floor=0;floor<3;floor++){
  int count=interior_floor_props(b,floor,p);for(int i=0;i<count;i++){beds[floor]+=p[i].action==ROOM_REST;stands[floor]+=p[i].art==ART_ARMOR_STAND;hash[floor]=hash[floor]*33+p[i].art+(int)(p[i].x*100)+(int)(p[i].y*100);}
 }
 assert(beds[0]>=8&&stands[0]==0&&stands[1]<=2&&stands[2]==0&&hash[0]!=hash[1]&&hash[1]!=hash[2]);
 for(int s=0;s<EXP_SITES;s++)for(int floor=0;floor<expedition_levels(s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1);floor++){
  Stack*weapon=expedition_cache((s*EXP_FLOORS+floor)*EXP_CHESTS+1);
  if(!expedition_inhabited(s))assert(itemDefs[weapon[0].item].slot==0);
 }
 assert(load_game("build/dust-v35-field-repairs.sav"));assert(g.repairs.interiorsVersion==1&&g.expedition.version&&g.actorCount>0);
}
int main(void){mkdir("build/v122",0777);progression();storage();aging();interiors();puts("PASS: 1.2.2 XP rates, removed skills/camps, depot allowlist, persistent two-day theft expiry through equip/pass/drop/home/save, distinct barracks/armory/command and genuine v35 migration.");return 0;}
