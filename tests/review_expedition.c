#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned pixels[480*272];
static void capture(const char*dir,const char*name){char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",dir,name);FILE*f=fopen(path,"wb");assert(f);render_game(pixels,480);fprintf(f,"P6\n480 272\n255\n");for(int p=0;p<480*272;p++){unsigned char c[]={pixels[p]&255,pixels[p]>>8&255,pixels[p]>>16&255};fwrite(c,1,3,f);}fclose(f);}
int main(int argc,char**argv){assert(argc==3);const char*names[]={"exploration-model-0","exploration-model-1","exploration-model-2","exploration-model-3","exploration-model-4","exploration-model-5","refuge-entrance","refuge-counter","refuge-stairs","turbine-entrance","archive-entrance","archive-gallery","archive-vault","archive-cache","robot-combat"};for(int i=0;i<15;i++){char path[512];snprintf(path,sizeof(path),"%s/%s.sav",argv[1],names[i]);assert(load_game(path));ui.screen=SCREEN_WORLD;ui.noticeTime=0;ui.camX=g.actors[0].x;ui.camY=g.actors[0].y;capture(argv[2],names[i]);}return 0;}
