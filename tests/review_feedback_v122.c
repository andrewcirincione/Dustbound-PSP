#include "../src/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static void head_probe(float,float,float,float,float);
static void head_before(float,float,float,float,float);
#define DUST_HEAD_OBSERVER head_probe
#define DUST_BEFORE_HEAD_OBSERVER head_before
#include "../src/render.c"
static int hx0,hy0,hx1,hy1,facePixels,faceChanges;
static uint32_t beforeHead[ACT_SCRATCH_STRIDE*ACT_H*2],headBackground[ACT_SCRATCH_STRIDE*ACT_H*2];
static void head_before(float x0,float y0,float x1,float y1,float width){
 hx0=(int)floorf(fminf(x0,x1)-width*.5f);hx1=(int)ceilf(fmaxf(x0,x1)+width*.5f);
 hy0=(int)floorf(fminf(y0,y1));hy1=(int)floorf(fmaxf(y0,y1))-1;
 memset(beforeHead,0,sizeof(beforeHead));memset(headBackground,0,sizeof(headBackground));
 for(int yy=hy0;yy<=hy1;yy++)for(int xx=hx0;xx<=hx1;xx++)if(xx>=0&&xx<ACT_SCRATCH_STRIDE&&yy>=0&&yy<ACT_H*2)headBackground[yy*ACT_SCRATCH_STRIDE+xx]=fb[yy*pitch+xx];
}
static void head_probe(float x0,float y0,float x1,float y1,float width){
 (void)x0;(void)y0;(void)x1;(void)y1;(void)width;
 for(int yy=hy0;yy<=hy1;yy++)for(int xx=hx0;xx<=hx1;xx++)if(xx>=0&&xx<ACT_SCRATCH_STRIDE&&yy>=0&&yy<ACT_H*2){
  int at=yy*ACT_SCRATCH_STRIDE+xx;if(fb[yy*pitch+xx]!=headBackground[at])beforeHead[at]=fb[yy*pitch+xx];
 }
}
static void capture(const char *name){
 static uint32_t pixels[480*272];render_game(pixels,480);char path[256];snprintf(path,sizeof(path),"build/v122/visual/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<480*272;i++){unsigned char rgb[]={pixels[i]&255,pixels[i]>>8&255,pixels[i]>>16&255};fwrite(rgb,3,1,f);}fclose(f);
}
int main(void){
 mkdir("build/v122",0777);mkdir("build/v122/visual",0777);game_new(73819,0);Actor*a=&g.actors[0];
 armor_equip(a,3);a->hair=0;a->beard=0;a->skin=2;a->gear[2]=0;a->autoattack=0;a->target=-1;
 for(int race=0;race<SPECIES;race++)for(int body=0;body<3;body++)for(int w=0;w<GREATSWORD_ADDITIONS;w++)for(int face=0;face<2;face++){
  ActorSprite picture;memset(&picture,0,sizeof(picture));memset(beforeHead,0,sizeof(beforeHead));a->body=body;a->race=race;a->facing=face;a->gear[0]=WEAPON_ITEMS+100+w;paint_cached_actor(&picture,a,1,0,0);
  int changed=0;
  for(int i=0;i<ACT_SCRATCH_STRIDE*ACT_H*2;i++)if((beforeHead[i]>>24)>=128){facePixels++;faceChanges+=beforeHead[i]!=actorScratch[i];changed+=beforeHead[i]!=actorScratch[i];}

  if(race||body)continue;
  char path[160];snprintf(path,sizeof(path),"build/v122/visual/greatsword-%d-%d.rgba",w,face);FILE*f=fopen(path,"wb");assert(f);fwrite(picture.pixels,sizeof(picture.pixels),1,f);fclose(f);
 }
 a->body=0;a->race=0;
 ui.screen=SCREEN_PACK;ui.selection=PACK;ui.noticeTime=ui.feedCount=0;capture("inventory-weights");
 ui.screen=SCREEN_TRADE;ui.sell=1;ui.tradeTown=0;ui.tradeSmith=TRADE_SMITH;capture("shop-weights");
 ui.screen=SCREEN_SKILLS;ui.skillPage=0;ui.selection=14;capture("skills");ui.screen=SCREEN_BUILD;ui.selection=0;capture("camp");
 int b=g.wilderness.building[5]-1;actor_enter_room(0,b);a->autoattack=0;
 for(int floor=0;floor<3;floor++){
  g.innFloor[0]=floor;g.interiorX=g.depth.actors[0].rx=floor?12:4;g.interiorY=g.depth.actors[0].ry=floor?12:9;ui.screen=SCREEN_WORLD;ui.noticeTime=ui.feedCount=0;
  char name[64];snprintf(name,sizeof(name),"redoubt-floor-%d",floor);capture(name);
 }
 printf("Greatsword face audit: %d painted face pixels checked, %d occluded or changed.\n",facePixels,faceChanges);assert(facePixels>100&&faceChanges==0);
 puts("PASS: production renderer checked 600 greatsword/race/body/facing fits and captured 40 weapon previews, weight columns, simplified skills/camp and three Redoubt floors.");return 0;
}
