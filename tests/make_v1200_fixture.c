#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include "frontier_fixture.h"
static int building(int s){return s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1;}
static void room(int s,int floor){fresh();int b=building(s);actor_enter_room(0,b);for(int f=0;f<floor;f++)expedition_stairs(&g.actors[0],1);expedition_tick(.1f);ui.screen=SCREEN_WORLD;ui.noticeTime=0;}
static int service(int action){fixture_service(action);g.depth.actors[0].rx=g.interiorX;g.depth.actors[0].ry=g.interiorY;int count=nearby_actions();for(int j=0;j<count;j++){const NearbyAction*x=nearby_action(j);if(x->type==ACTION_CONTEXT&&x->kind==5)return j;}assert(0);return -1;}
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
 const int sites[]={17,20,16,18,19,21};char name[64];
 for(int m=0;m<6;m++){Actor*a=fresh();int b=building(sites[m]);float x,y;building_door(b,&x,&y);a->x=g.buildings[b].x-1;a->y=g.buildings[b].y-6;expedition_tick(.1f);ui.noticeTime=0;snprintf(name,sizeof(name),"exploration-model-%d",m);save(name);}
 room(16,0);save("refuge-entrance");int trade=service(ROOM_EXP_TRADE);save("refuge-counter");
 room(16,0);int stairs=service(ROOM_EXP_UP);save("refuge-stairs");
 room(16,1);int down=service(ROOM_EXP_DOWN);save("refuge-downstairs");
 room(20,0);save("turbine-entrance");
 room(18,0);save("archive-entrance");
 room(18,1);save("archive-gallery");int upper=service(ROOM_EXP_UP);save("archive-upper-stairs");
 room(18,2);save("archive-vault");int cache=service(ROOM_EXP_CACHE);save("archive-cache");
 room(20,0);Actor*a=&g.actors[0];ExpeditionCrew*c=&g.expedition.crew[20][0];int n=c->live[0];assert(n>=0);Actor*bot=&g.actors[n];g.interiorX=g.depth.actors[0].rx=g.depth.actors[n].rx+1.1f;g.interiorY=g.depth.actors[0].ry=g.depth.actors[n].ry;
 a->gear[0]=a->attackWeapon=WEAPON_ITEMS+109;armor_equip(a,20);a->healthScale=1000;for(int l=0;l<LIMBS;l++)a->health[l]=1000;a->target=n;a->autoattack=0;bot->target=0;bot->cooldown=0;save("robot-combat");
 char path[512];snprintf(path,sizeof(path),"%s/exploration.json",root);FILE*f=fopen(path,"w");assert(f);fprintf(f,"{\"trade\":%d,\"stairs\":%d,\"upper\":%d,\"down\":%d,\"cache\":%d,\"trade_screen\":%d,\"chest_screen\":%d}",trade,stairs,upper,down,cache,SCREEN_TRADE,SCREEN_CHEST);fclose(f);
 puts("Prepared six new exteriors, occupied refuge, three-floor archive, services, finite loot and robot combat.");return 0;
}
