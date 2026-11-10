#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
extern int add_actor(const char*,int,int,float,float,int);
static const char *root;
static void save(const char*name){char p[512];snprintf(p,sizeof(p),"%s/%s.sav",root,name);assert(save_game(p));}
static Actor*fresh(void){game_new(73819,0);g.selected=0;ui.screen=SCREEN_WORLD;g.money=60000;g.dayTime=12;Actor*a=&g.actors[0];a->autoattack=0;a->target=-1;a->gear[2]=a->gear[5]=0;return a;}
static void in_room(int b,float x,float y){actor_enter_room(0,b);g.interiorBuilding=b+1;g.interiorActor=0;g.interiorX=x;g.interiorY=y;g.depth.actors[0].rx=x;g.depth.actors[0].ry=y;}
static void homes(void){Actor*a=fresh();int b=-1;for(int i=0;i<g.buildingCount;i++)if(home_state(i)==HOME_FOR_SALE&&building_town(i)==3){b=i;break;}assert(b>=0);g.homes.property[b]=HOME_OWNED;in_room(b,3.5f,5.9f);assert(home_place(b,0,FURN_BED,1.4f,1.5f));assert(home_place(b,1,FURN_CHEST,5.4f,1.5f));assert(home_place(b,2,FURN_TABLE,2.0f,4.0f));assert(home_place(b,3,FURN_SHELF,5.4f,4.6f));a->gear[1]=gear_id(36,4,1);save("home");}
static void camp(void){Actor*a=fresh();inventory_add(a,WOOD,20);inventory_add(a,BEDROLL_ITEM,2);int found=0;for(int y=250;y<1100&&!found;y+=13)for(int x=250;x<1100;x+=13){a->x=x-3;a->y=y;if(placement_valid(0,-1,x,y)&&placement_valid(1,-1,x,y+3)){found=1;break;}}assert(found);save("camp");}
static void arena(int heavy){Actor*a=fresh();a->body=0;a->race=0;a->gear[0]=heavy?gear_id(84,2,1):SABRE;a->gear[1]=gear_id(36,4,1);a->gear[3]=BOOTS;a->gear[4]=gear_id(69,1,1);a->xp[STRENGTH]=12*45*45;a->xp[DEXTERITY]=12*12*12;g.society.weaponXP[0][weapon_class(a->gear[0])]=12*9*9;
 float x=0,y=0;int found=0;for(int iy=250;iy<1100&&!found;iy+=13)for(int ix=250;ix<1100;ix+=13)if(can_walk(ix,iy)&&can_walk(ix+1,iy)&&town_near(ix,iy,30)<0){x=ix;y=iy;found=1;break;}assert(found);a->x=x;a->y=y;
 for(int n=1;n<g.actorCount;n++)if(!g.actors[n].squad&&!actor_inside(&g.actors[n])){g.actors[n].x=g.actors[n].y=1450;g.actors[n].target=-1;}
 for(int k=0;k<POP_GROUPS;k++){g.population.groups[k].x=g.population.groups[k].y=1400;g.population.state[k].siteX=g.population.state[k].siteY=1400;}
 int n=add_actor("Practice raider",RAIDER,HOSTILE,x+.9f,y-.15f,60);assert(n>=0);Actor*b=&g.actors[n];b->gear[1]=gear_id(38,4,3);b->gear[2]=HELM;b->autoattack=0;b->target=-1;b->cooldown=1000;
 a->autoattack=1;a->target=n;g.living.command[0]=n;save(heavy?"greatsword":"onehand");}
static void inn(void){fresh();int b=-1;for(int i=0;i<g.buildingCount;i++)if(inn_has_upstairs(i)){b=i;break;}assert(b>=0);in_room(b,4.6f,5.2f);inn_change_floor(&g.actors[0],1);save("upstairs");}
static void depot(void){fresh();int b=-1;for(int i=0;i<g.buildingCount;i++)if(depot_public(i)){b=i;break;}assert(b>=0);in_room(b,3.4f,4.2f);inventory_add(&g.actors[0],IRONBAR,4);save("depot");}
static void jail(void){Actor*a=fresh();int b=g.field.jail[3]-1;assert(b>=0);in_room(b,3.5f,5.6f);int n=add_actor("Wanted raider",RAIDER,HOSTILE,a->x,a->y,5);assert(n>=0);Actor*p=&g.actors[n];p->bountyReward=640;p->unconscious=300;p->carriedBy=0;a->carrying=n;g.depth.actors[n].room=b+1;g.depth.actors[n].rx=3.5f;g.depth.actors[n].ry=5.6f;save("bounty");}
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);homes();camp();arena(0);arena(1);inn();depot();jail();puts("Saved seven production-game QA scenes.");return 0;}
