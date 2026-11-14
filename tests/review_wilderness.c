#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned pixels[480*272];
static void capture(const char*dir,const char*name){
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",dir,name);FILE*f=fopen(path,"wb");assert(f);render_game(pixels,480);fprintf(f,"P6\n480 272\n255\n");for(int p=0;p<480*272;p++){unsigned char c[]={pixels[p]&255,pixels[p]>>8&255,pixels[p]>>16&255};fwrite(c,1,3,f);}fclose(f);
}
int main(int argc,char**argv){assert(argc==3);const char*names[]={"landmark-0-exterior","landmark-0-interior","landmark-1-exterior","landmark-1-interior","landmark-2-exterior","landmark-2-interior","wilderness-camp","wilderness-bandits"};
 for(int i=0;i<8;i++){char path[512];snprintf(path,sizeof(path),"%s/%s.sav",argv[1],names[i]);assert(load_game(path));ui.screen=SCREEN_WORLD;ui.noticeTime=0;ui.camX=g.actors[0].x;ui.camY=g.actors[0].y;capture(argv[2],names[i]);}
 char path[512];snprintf(path,sizeof(path),"%s/greatsword-stance.sav",argv[1]);assert(load_game(path));ui.screen=SCREEN_WORLD;ui.noticeTime=0;ui.camX=g.actors[0].x;ui.camY=g.actors[0].y;
 capture(argv[2],"greatsword-stance-passive");game_tick(.1f,(Input){.pressed=KEY_SQUARE});for(int j=0;j<3;j++)game_tick(.1f,(Input){0});capture(argv[2],"greatsword-stance-fight");
 puts("Rendered actual locations, interior occupants, encounters and distinct greatsword stances.");return 0;
}
