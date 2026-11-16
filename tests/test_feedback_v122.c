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
  if(g.buildings[b].type==STORE&&!permitted){
   g.actors[0].x=g.buildings[b].x+.4f;g.actors[0].y=g.buildings[b].y;ui.screen=SCREEN_WORLD;
   interact_context(2,b);assert(ui.screen!=SCREEN_STORAGE);
  }
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
static void stored_theft(void){
 fresh();Actor*a=&g.actors[0];memset(a->pack,0,sizeof(a->pack));memset(a->stolen,0,sizeof(a->stolen));
 g.time=100;assert(inventory_add_owned(a,BEDROLL_ITEM,1,UNION));int k=slot(a,BEDROLL_ITEM);float deadline=stolen_until(a,k);
 float bx=0,by=0;int found=0;for(int y=250;y<1100&&!found;y+=13)for(int x=250;x<1100;x+=13){a->x=x-1.6f;a->y=y;if(placement_valid(1,-1,x,y)){bx=x;by=y;found=1;break;}}assert(found);
 assert(place_camp(1,bx,by));int bed=g.buildingCount-1;assert(g.repairs.rollUntil[bed]==deadline);
 g.time=deadline-100;pack_bedroll(bed);k=slot(a,BEDROLL_ITEM);assert(k>=0&&stolen_until(a,k)==deadline);
 assert(place_camp(1,bx,by));bed=g.buildingCount-1;g.time=deadline;assert(portable_bedroll(bed)==1);pack_bedroll(bed);k=slot(a,BEDROLL_ITEM);assert(k>=0&&!item_slot_owner(a,k));
 // A seized-property chest preserves the item's original theft deadline.
 fresh();a=&g.actors[0];memset(a->pack,0,sizeof(a->pack));g.time=200;
 g.field.seized[0][0]=(Stack){SABRE,1};g.field.seizedOwner[0][0]=UNION;g.repairs.seizedUntil[0][0]=3700;
 ui.chestKind=CHEST_PROPERTY;ui.storageIndex=0;interior_chest_take(0);k=slot(a,SABRE);assert(k>=0&&stolen_until(a,k)==3700);
 g.field.seized[0][0]=(Stack){WOOD,1};g.field.seizedOwner[0][0]=UNION;g.repairs.seizedUntil[0][0]=3700;g.time=3700;assert(!stolen_property_until(0,0)&&!g.field.seizedOwner[0][0]);
}
static void interiors(void){
 fresh();int b=g.wilderness.building[5]-1;InteriorProp p[ROOM_PROPS];int beds[3]={0},stands[3]={0};
 unsigned long hash[3]={0};
 for(int floor=0;floor<3;floor++){
  int count=interior_floor_props(b,floor,p);for(int i=0;i<count;i++){beds[floor]+=p[i].action==ROOM_REST;stands[floor]+=p[i].art==ART_ARMOR_STAND;hash[floor]=hash[floor]*33+p[i].art+(int)(p[i].x*100)+(int)(p[i].y*100);}
 }
 assert(beds[0]>=8&&stands[0]==0&&stands[1]<=2&&stands[2]==0&&hash[0]!=hash[1]&&hash[1]!=hash[2]);
 // The stairs, chests and furniture must have distinct physical footprints.
 for(int site=0;site<EXP_SITES;site++){
  int building=site<16?g.wilderness.building[site]-1:g.expedition.primary[site-16]-1;
  for(int floor=0;floor<expedition_levels(building);floor++){
   int count=interior_floor_props(building,floor,p);
   for(int i=0;i<count;i++)for(int j=0;j<i;j++)assert(fabsf(p[i].x-p[j].x)>=p[i].w+p[j].w||fabsf(p[i].y-p[j].y)>=p[i].d+p[j].d);
  }
 }
 for(int s=0;s<EXP_SITES;s++)for(int floor=0;floor<expedition_levels(s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1);floor++){
  Stack*weapon=expedition_cache((s*EXP_FLOORS+floor)*EXP_CHESTS+1);
  if(!expedition_inhabited(s))assert(itemDefs[weapon[0].item].slot==0);
 }
 int ward=g.wilderness.building[6]-1;actor_enter_room(0,ward);fixture_service(ROOM_REST);interior_interact();assert(g.actors[0].taskKind==3);
 assert(load_game("build/dust-v35-field-repairs.sav"));assert(g.repairs.interiorsVersion==1&&g.expedition.version&&g.actorCount>0);
}
int main(void){mkdir("build/v122",0777);progression();storage();aging();stored_theft();interiors();puts("PASS: 1.2.2 XP rates, removed skills/camps, depot allowlist, persistent two-day theft expiry through equip/pass/drop/home/save, distinct barracks/armory/command and genuine v35 migration.");return 0;}
