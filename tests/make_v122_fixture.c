#define main previous_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include "frontier_fixture.h"
static void road(Actor*a){int found=0;for(int y=160;y<1200&&!found;y+=17)for(int x=160;x<1200;x+=17)if(town_near(x,y,30)<0&&can_walk(x,y)&&can_walk(x+12,y)&&can_walk(x-2,y)){a->x=x;a->y=y;found=1;break;}assert(found);a->homeX=a->x;a->homeY=a->y;for(int n=1;n<g.actorCount;n++)if(!g.actors[n].squad&&!actor_inside(&g.actors[n])){g.actors[n].x=g.actors[n].y=1450;g.actors[n].target=-1;}}
static int site_building(int s){return s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1;}
static void room(int s,int floor,float x,float y){Actor*a=fresh();int b=site_building(s);in_room(b,x,y);g.innFloor[0]=floor;g.interiorX=g.depth.actors[0].rx=x;g.interiorY=g.depth.actors[0].ry=y;expedition_tick(.1f);for(int n=1;n<g.actorCount;n++)if(depth_room(&g.actors[n])==b+1)g.actors[n].faction=FREE;a->autoattack=0;ui.noticeTime=ui.feedCount=0;}
static void cache(int c){int b=g.interiorBuilding-1,floor=actor_floor(&g.actors[0]);InteriorProp p[ROOM_PROPS];int n=interior_floor_props(b,floor,p);for(int i=0;i<n;i++)if(p[i].action==ROOM_EXP_CACHE&&expedition_cache_index(b,floor,p[i].x,p[i].y)%EXP_CHESTS==c){g.interiorX=g.depth.actors[0].rx=p[i].x+1;g.interiorY=g.depth.actors[0].ry=p[i].y;assert(room_floor_walkable(b+1,floor,g.interiorX,g.interiorY));return;}assert(0);}
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);char name[80];
 for(int face=0;face<2;face++){Actor*a=fresh();road(a);armor_equip(a,3);a->gear[0]=WEAPON_ITEMS+109;a->gear[2]=0;a->hair=0;a->beard=0;a->facing=face;snprintf(name,sizeof(name),"v122-carry-%d",face);save(name);}
 Actor*a=fresh();road(a);a->health[LLEG]=20;a->autoattack=0;assert(limb_limping(a));save("v122-limping");
 a=fresh();road(a);memset(a->pack,0,sizeof(a->pack));memset(a->stolen,0,sizeof(a->stolen));g.time=100;inventory_add_owned(a,SABRE,1,UNION);g.time=100+2*DAY_SECONDS-1.0f;save("v122-stolen");
 for(int floor=0;floor<3;floor++){room(5,floor,floor?12:4,floor?12:9);snprintf(name,sizeof(name),"v122-redoubt-%d",floor);save(name);}
 room(5,0,4,9);fixture_service(ROOM_EXP_UP);save("v122-redoubt-stairs");
 room(5,1,12,12);cache(1);save("v122-armory-cache");
 room(5,2,12,12);cache(3);save("v122-command-cache");
 room(7,0,4,9);cache(0);save("v122-storehouse");
 room(20,0,10,12);save("v122-factory");room(18,1,12,15);save("v122-archive");room(6,0,4,10);save("v122-infirmary");fixture_service(ROOM_REST);g.actors[0].health[LARM]=35;g.actors[0].fatigue=45;dress_wounds(&g.actors[0]);save("v122-infirmary-bed");room(16,1,12,16);save("v122-refuge");
 a=fresh();road(a);inventory_add(a,WOOD,40);inventory_add(a,BEDROLL_ITEM,2);save("v122-camp");
 puts("Prepared declared 1.2.2 QA scenarios: carry/limping, timed stolen ownership, barracks/armory/command, stairs, useful caches, storehouse chest, factory/archive/infirmary/refuge and simple camps. Interior guards neutralized for layout walks.");return 0;
}
