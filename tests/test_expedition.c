#include "game.h"
#include "frontier_fixture.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int seen[84*84],queue[84*84];
static float point(int x){return .5f+x*.4f;}
static void reachable_floor(int b,int floor){
 enum{SIDE=84};memset(seen,0,sizeof(seen));int first=-1;
 for(int y=0;y<80&&first<0;y++)for(int x=0;x<80;x++)if(room_floor_walkable(b+1,floor,point(x),point(y))){first=y*SIDE+x;break;}assert(first>=0);
 int used=1;queue[0]=first;seen[first]=1;
 for(int j=0;j<used;j++){int cell=queue[j],x=cell%SIDE,y=cell/SIDE;static const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
  for(int k=0;k<4;k++){int xx=x+dx[k],yy=y+dy[k],n=yy*SIDE+xx;if(xx<0||yy<0||xx>=SIDE||yy>=SIDE||seen[n]||!room_floor_walkable(b+1,floor,point(xx),point(yy))||!room_floor_walkable(b+1,floor,point(x)+dx[k]*.2f,point(y)+dy[k]*.2f))continue;seen[n]=1;queue[used++]=n;}
 }
 InteriorProp props[ROOM_PROPS];int count=interior_floor_props(b,floor,props),actions=0;
 for(int p=0;p<count;p++)if(props[p].action){int reached=0;for(int j=0;j<used;j++){int c=queue[j];if(distance(point(c%SIDE),point(c/SIDE),props[p].x,props[p].y)<1.6f){reached=1;break;}}if(!reached){fprintf(stderr,"Unreachable site %d floor%d action%d at%.1f,%.1f\n",expedition_site(b),floor,props[p].action,props[p].x,props[p].y);assert(reached);}actions++;}
 assert(actions>=5&&used>500);
}
static void analog_walk_to_stairs(void){
 enum{SIDE=84,CELLS=SIDE*SIDE};static int parent[CELLS],path[CELLS];
 game_new(73819,0);ui.screen=SCREEN_WORLD;Actor*a=&g.actors[0];a->autoattack=0;int b=g.expedition.primary[0]-1;actor_enter_room(0,b);expedition_tick(.1f);
 float ox=g.interiorX,oy=g.interiorY;fixture_service(ROOM_EXP_UP);float gx=g.interiorX,gy=g.interiorY;g.interiorX=ox;g.interiorY=oy;
 int sx=lroundf((ox-.5f)/.4f),sy=lroundf((oy-.5f)/.4f),start=sy*SIDE+sx,end=-1,used=1;queue[0]=start;for(int j=0;j<CELLS;j++)parent[j]=-2;parent[start]=-1;
 for(int j=0;j<used;j++){int c=queue[j],x=c%SIDE,y=c/SIDE;if(distance(point(x),point(y),gx,gy)<.3f){end=c;break;}static const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
  for(int k=0;k<4;k++){int xx=x+dx[k],yy=y+dy[k],n=yy*SIDE+xx;if(xx<0||yy<0||xx>=SIDE||yy>=SIDE||parent[n]!=-2||!room_floor_walkable(b+1,0,point(xx),point(yy))||!room_floor_walkable(b+1,0,point(x)+dx[k]*.2f,point(y)+dy[k]*.2f))continue;parent[n]=c;queue[used++]=n;}}
 assert(end>=0);int count=0;for(int c=end;c!=start;c=parent[c])path[count++]=c;assert(count>50);
 for(int j=count-1;j>=0;j--){float x=point(path[j]%SIDE),y=point(path[j]/SIDE);int ticks=0;while(distance(g.interiorX,g.interiorY,x,y)>.17f&&ticks++<80){float dx=x-g.interiorX,dy=y-g.interiorY,len=hypotf(dx,dy);game_tick(.03f,(Input){.ax=view_horizontal(dx/len,dy/len),.ay=view_vertical(dx/len,dy/len)});}assert(ticks<80);}
 assert(interior_context()>=0);interior_interact();assert(actor_floor(a)==1);puts("PASS: ordinary analog game input crosses the refuge through connected rooms to the far staircase and climbs to floor two.");
}
int main(void){
 game_new(73819,0);assert(sizeof(Game)<4*1024*1024);int added=0,floors=0,occupied=0,robots=0;
 for(int i=0;i<EXP_ADDITIONS;i++)added+=g.expedition.primary[i]>0;assert(added==30);
 for(int s=0;s<EXP_SITES;s++){int b=s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1;assert(b>=0&&expedition_site(b)==s);assert(expedition_width(b)>=16&&expedition_height(b)>=24);
  float x,y;building_door(b,&x,&y);assert(can_walk(x,y));occupied+=expedition_inhabited(s);
  for(int f=0;f<expedition_levels(b);f++){reachable_floor(b,f);floors++;}
 }
 printf("PASS: 46 sites (%d occupied), %d floors; all stairs, records, counters, beds and caches reachable through actual room collision.\n",occupied,floors);
 assert(load_game("build/v33-modern.sav"));assert(g.money==12345&&g.expedition.version==1);for(int i=0;i<EXP_ADDITIONS;i++)assert(g.expedition.primary[i]);
 int b=g.expedition.primary[0]-1;actor_enter_room(g.selected,b);ui.screen=SCREEN_WORLD;expedition_tick(.1f);assert(g.expedition.crew[16][0].count==3);
 fixture_service(ROOM_EXP_TRADE);interior_interact();assert(ui.screen==SCREEN_TRADE&&shop_count()>0);ui.screen=SCREEN_WORLD;
 fixture_service(ROOM_EXP_CACHE);interior_interact();assert(ui.screen==SCREEN_CHEST);int index=ui.storageIndex;Stack*loot=expedition_cache(index);int qty=loot[0].qty;interior_chest_take(0);assert(loot[0].qty==qty-1);
 assert(save_game("build/expedition-interaction.sav")&&load_game("build/expedition-interaction.sav"));assert(expedition_cache(index)[0].qty==qty-1);ui.screen=SCREEN_WORLD;
 fixture_service(ROOM_EXP_UP);interior_interact();assert(actor_floor(&g.actors[g.selected])==1);expedition_tick(.1f);assert(save_game("build/expedition-floor1.sav")&&load_game("build/expedition-floor1.sav"));assert(actor_floor(&g.actors[g.selected])==1);
 fixture_service(ROOM_EXP_DOWN);interior_interact();assert(actor_floor(&g.actors[g.selected])==0);
 b=g.expedition.primary[2]-1;actor_enter_room(g.selected,b);ui.screen=SCREEN_WORLD;expedition_tick(.1f);
 ExpeditionCrew*c=&g.expedition.crew[18][0];assert(c->initialized&&c->live[0]>=0);int n=c->live[0];Actor*bot=&g.actors[n];assert(expedition_robot(bot)>=0&&bot->race==2&&!inventory_count(bot,MEAT)&&inventory_count(bot,PARTS)>0);robots++;
 bot->dead=1;bot->health[CHEST]=-100;assert(inventory_remove(bot,PARTS,inventory_count(bot,PARTS)));interior_leave();expedition_tick(.1f);assert(c->live[0]<0&&c->actor[0].dead&&!inventory_count(&c->actor[0],PARTS));
 actor_enter_room(g.selected,b);expedition_tick(.1f);assert(c->live[0]>=0&&g.actors[c->live[0]].dead&&!inventory_count(&g.actors[c->live[0]],PARTS));
 fixture_service(ROOM_EXP_UP);interior_interact();expedition_tick(.1f);fixture_service(ROOM_EXP_UP);interior_interact();expedition_tick(.1f);assert(actor_floor(&g.actors[g.selected])==2);
 int vault=(18*EXP_FLOORS+2)*EXP_CHESTS+3;assert(arsenal_tier(expedition_cache(vault)[0].item)==g.expedition.tier[18]);
 assert(save_game("build/expedition-floor2.sav")&&load_game("build/expedition-floor2.sav"));assert(actor_floor(&g.actors[g.selected])==2);
 puts("PASS: authentic v33 upgrade; occupied outpost trading, finite cache depletion, three-floor stairs/save/load, robot salvage and cleared enemies persist; vault rarity follows site tier.");
 analog_walk_to_stairs();(void)robots;return 0;
}
