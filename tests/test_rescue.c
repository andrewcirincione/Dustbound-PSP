#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor*);
static int slave,prisoner;
static void setup(void){
 game_new(73819,0);Actor*a=&g.actors[g.selected];slave=g.depth.npc[NPC_ORIN];prisoner=-1;
 for(int n=0;n<g.actorCount;n++){if(!g.actors[n].squad&&g.actors[n].prisonTime>0&&prisoner<0)prisoner=n;if(n!=g.selected)g.actors[n].active=0;}
 assert(slave>=0&&prisoner>=0);g.seed=(g.seed/3)*3+(3-slave%3)%3;assert((g.seed+prisoner)%3!=0);a->x=g.depth.siteX[SITE_CAGES];a->y=g.depth.siteY[SITE_CAGES]+.8f;a->autoattack=0;a->xp[LOCKPICK]=0;inventory_add(a,TOOLS,1);
 for(int k=0;k<2;k++){int n=k?prisoner:slave;Actor*b=&g.actors[n];b->active=1;b->x=a->x+k*.7f;b->y=a->y-.8f;b->homeX=b->x;b->homeY=b->y;b->target=-1;b->autoattack=0;}
 ui.screen=SCREEN_WORLD;ui.camX=a->x;ui.camY=a->y;ui.paused=0;
}
static int row(int type,int target){nearby_actions();for(int i=0;i<nearby_actions();i++)if(nearby_action(i)->type==type&&nearby_action(i)->target==target)return i;assert(0);return -1;}
static void run(float seconds){for(int i=0;i<(int)(seconds*10);i++)game_tick(.1f,(Input){0});}
int main(void){setvbuf(stdout,NULL,_IONBF,0);
 setup();int count=g.squadCount;ui.paused=1;action_menu_open();nearby_action_choose(row(ACTION_RESCUE,slave));nearby_action_choose(row(ACTION_RESCUE,prisoner));assert(g.society.queueCount[g.selected]==2);assert(g.depth.actors[slave].captive&&g.actors[prisoner].prisonTime>0);
 game_tick(.033f,(Input){KEY_R,KEY_R,0,0});assert(!ui.paused&&ui.screen==SCREEN_WORLD);run(1);assert(society_rescue_busy(g.selected)&&g.society.queueCount[g.selected]==1);assert(save_game("build/rescue-working.sav"));assert(load_game("build/rescue-working.sav"));assert(society_rescue_busy(g.selected));run(25);
 assert(!g.depth.actors[slave].captive&&g.actors[prisoner].prisonTime==0&&g.society.freed[slave]==1&&g.society.freed[prisoner]==2);assert(g.squadCount==count&&!g.actors[slave].squad&&!g.actors[prisoner].squad);assert(!g.society.queueCount[g.selected]&&g.actors[g.selected].xp[LOCKPICK]>0);
 assert(save_game("build/rescue-freed.sav"));assert(load_game("build/rescue-freed.sav"));assert(g.society.freed[slave]==1&&!g.actors[slave].squad);
 Actor*p=&g.actors[g.selected],*b=&g.actors[prisoner];p->x=b->x;p->y=b->y;float fleeingX=b->x,fleeingY=b->y;society_npc_job(prisoner,1);assert(distance(fleeingX,fleeingY,b->x,b->y)>0);p->x=b->x;p->y=b->y;society_rescue_order(prisoner,ACTION_RESCUED_FOLLOW);p->x=b->x+4;p->y=b->y;float old=b->x;for(int i=0;i<15;i++)society_npc_job(prisoner,.1f);assert(b->x>old&&g.squadCount==count);
 p->x=b->x;p->y=b->y;society_rescue_order(prisoner,ACTION_RESCUED_LEAVE);assert(g.society.freed[prisoner]==2&&!b->squad);b->x=b->homeX;b->y=b->homeY;society_npc_job(prisoner,.1f);assert(g.society.freed[prisoner]==3&&g.squadCount==count);
 p->x=g.actors[slave].x;p->y=g.actors[slave].y;g.squadCount=SQUAD;society_rescue_order(slave,ACTION_RESCUED_RECRUIT);assert(!g.actors[slave].squad&&g.society.freed[slave]);g.squadCount=count;society_rescue_order(slave,ACTION_RESCUED_RECRUIT);assert(g.actors[slave].squad&&g.squadCount==count+1);
 puts("PASS: two paused rescues execute serially, persist mid-work and after release; freed NPCs choose following or flight without squad membership, followers can be dismissed to refuge, and join only through Recruit; full squad does not block freedom.");
 setup();int guard=-1;for(int n=0;n<g.actorCount;n++)if(g.actors[n].kind==GUARD&&g.actors[n].faction==UNION){guard=n;break;}assert(guard>=0);g.actors[guard].active=1;g.actors[guard].x=g.actors[prisoner].x+2;g.actors[guard].y=g.actors[prisoner].y;
 assert(society_rescue_start(prisoner));society_tick(.1f);assert(!society_rescue_busy(g.selected)&&g.actors[prisoner].prisonTime>0&&wanted_amount(&g.actors[g.selected],UNION)>=80);g.actors[guard].unconscious=100;
 assert(society_rescue_start(prisoner));society_tick(1);g.actors[g.selected].x+=8;society_tick(.1f);assert(!society_rescue_busy(g.selected)&&g.actors[prisoner].prisonTime>0);
 g.actors[g.selected].x=g.actors[prisoner].x;g.depth.actors[prisoner].room=1;assert(!society_rescue_start(prisoner));g.depth.actors[prisoner].room=0;g.actors[prisoner].dead=1;assert(!society_rescue_start(prisoner));
 puts("PASS: nearby conscious guards catch rescue attempts and create faction charges; distance, rooms and dead-target checks prevent remote or invalid rescues.");
 setup();assert(save_game("build/v090/DUST_0.sav"));printf("STATE: Game=%zu bytes; %d actors in fresh rescue fixture.\n",sizeof(Game),g.actorCount);
}
