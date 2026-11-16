#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/render.c"
static void capture(const char *name){
 static uint32_t pixels[480*272];render_game(pixels,480);char path[256];snprintf(path,sizeof(path),"build/v122/visual/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<480*272;i++){unsigned char rgb[]={pixels[i]&255,pixels[i]>>8&255,pixels[i]>>16&255};fwrite(rgb,3,1,f);}fclose(f);
}
int main(void){
 mkdir("build/v122",0777);mkdir("build/v122/visual",0777);game_new(73819,0);Actor*a=&g.actors[0];
 armor_equip(a,3);a->hair=0;a->beard=0;a->skin=2;a->gear[2]=0;a->autoattack=0;a->target=-1;
 for(int w=0;w<GREATSWORD_ADDITIONS;w++)for(int face=0;face<2;face++){
  ActorSprite picture;memset(&picture,0,sizeof(picture));a->facing=face;a->gear[0]=WEAPON_ITEMS+100+w;paint_cached_actor(&picture,a,1,0,0);
  char path[160];snprintf(path,sizeof(path),"build/v122/visual/greatsword-%d-%d.rgba",w,face);FILE*f=fopen(path,"wb");assert(f);fwrite(picture.pixels,sizeof(picture.pixels),1,f);fclose(f);
 }
 ui.screen=SCREEN_PACK;ui.selection=PACK;ui.noticeTime=ui.feedCount=0;capture("inventory-weights");
 ui.screen=SCREEN_TRADE;ui.sell=1;ui.tradeTown=0;ui.tradeSmith=TRADE_SMITH;capture("shop-weights");
 ui.screen=SCREEN_SKILLS;ui.skillPage=0;ui.selection=14;capture("skills");ui.screen=SCREEN_BUILD;ui.selection=0;capture("camp");
 int b=g.wilderness.building[5]-1;actor_enter_room(0,b);a->autoattack=0;
 for(int floor=0;floor<3;floor++){
  g.innFloor[0]=floor;g.interiorX=g.depth.actors[0].rx=floor?12:4;g.interiorY=g.depth.actors[0].ry=floor?12:9;ui.screen=SCREEN_WORLD;ui.noticeTime=ui.feedCount=0;
  char name[64];snprintf(name,sizeof(name),"redoubt-floor-%d",floor);capture(name);
 }
 puts("PASS: production renderer captured 40 greatsword fits, weight columns, simplified skills/camp and three Redoubt floors.");return 0;
}
