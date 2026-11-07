#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static uint32_t pixels[480*272];
static void press(int key){game_tick(.033f,(Input){key,key,0,0});}
static void shot(const char*n){char path[200];snprintf(path,sizeof(path),"evidence/v081/%s.ppm",n);render_game(pixels,480);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<480*272;i++){unsigned char b[3]={pixels[i]&255,(pixels[i]>>8)&255,(pixels[i]>>16)&255};fwrite(b,1,3,f);}fclose(f);}
int main(void){
 game_new(73819,0);ui.noticeTime=0;
 assert(save_game("build/DUST_4.sav"));
 press(KEY_SELECT);assert(ui.screen==SCREEN_MAP&&!ui.mapZoom&&ui.mapChoice==0&&atlas_item(0)==-1);
 press(KEY_CROSS);assert(ui.screen==SCREEN_MAP&&!ui.waypoint);
 float x=g.actors[g.selected].x,y=g.actors[g.selected].y;
 int n=1;for(int t=0;t<TOWNS;t++)n+=!!g.discovered[t];assert(atlas_count()==n);
 for(int i=1;i<n;i++)assert(g.discovered[atlas_item(i)]);
 shot("unexplored");
 press(KEY_DOWN);assert(ui.mapChoice==1&&g.actors[g.selected].x==x&&g.actors[g.selected].y==y);
 int id=atlas_item(1);assert(ui.mapX==towns[id].x&&ui.mapY==towns[id].y);
 press(KEY_TRIANGLE);assert(ui.mapZoom&&ui.mapX==towns[id].x);press(KEY_TRIANGLE);assert(!ui.mapZoom);
 ui.mapCategory=0;atlas_select(0);press(KEY_UP);assert(ui.mapChoice==atlas_count()-1);press(KEY_DOWN);assert(ui.mapChoice==0);
 press(KEY_R);assert(ui.mapCategory==0);for(int i=0;i<atlas_count();i++)assert(atlas_item(i)<TOWNS);
 memset(g.discovered,1,sizeof(g.discovered));memset(g.seen,255,sizeof(g.seen));ui.mapCategory=0;assert(atlas_count()==TOWNS+1);atlas_select(9);shot("town");
 assert(atlas_item(9)==8);float tx=ui.mapX,ty=ui.mapY;assert(can_walk(tx,ty));press(KEY_CROSS);assert(ui.screen==SCREEN_WORLD&&ui.waypoint&&ui.travelX==tx&&ui.travelY==ty);
 assert(save_game("build/DUST_5.sav"));press(KEY_SELECT);assert(ui.screen==SCREEN_MAP);shot("travel");
 int count=atlas_count();for(int e=0;e<EVENTS;e++)g.living.events[e].status=1;ui.mapCategory=1;atlas_select(0);assert(atlas_count()==count&&ui.mapCategory==0);
 g.discovered[15]=0;g.contracts[0].active=1;g.contracts[0].type=0;g.contracts[0].progress=0;g.contracts[0].needed=1;g.contracts[0].target=15;ui.screen=SCREEN_JOURNAL;ui.journalTab=0;ui.selection=0;press(KEY_SQUARE);assert(ui.screen==SCREEN_MAP&&ui.mapZoom&&ui.mapX==towns[15].x&&ui.mapY==towns[15].y&&!g.discovered[15]);
 puts("PASS: atlas discovery filtering, named town selection, wraparound, zoom context, exact travel destination, events never enter atlas lists and D-pad preserves player position.");
}
