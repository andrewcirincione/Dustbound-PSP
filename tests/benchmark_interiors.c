#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static uint32_t pixels[512*272];
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static int cmp(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return(x>y)-(x<y);}
int main(void){
 puts("[");for(int scroll=0;scroll<2;scroll++)for(int floor=0;floor<3;floor++){
  game_new(73819,0);ui.screen=SCREEN_WORLD;ui.paused=0;int b=g.wilderness.building[5]-1;actor_enter_room(0,b);g.innFloor[0]=floor;
  g.interiorX=g.depth.actors[0].rx=4;g.interiorY=g.depth.actors[0].ry=expedition_height(b)-3;expedition_tick(.05f);
  double render[180],tick[180];for(int frame=-12;frame<180;frame++){
   if(scroll)g.interiorX=g.depth.actors[0].rx=4+(frame+12)*.06f;
   double start=now();render_game(pixels,512);double r=(now()-start)*1000;start=now();game_tick(1.f/30,(Input){0});double t=(now()-start)*1000;
   if(frame>=0){render[frame]=r;tick[frame]=t;}
  }
  qsort(render,180,sizeof(double),cmp);qsort(tick,180,sizeof(double),cmp);
  printf("%s{\"floor\":%d,\"mode\":\"%s\",\"render_median_ms\":%.6f,\"render_p95_ms\":%.6f,\"tick_median_ms\":%.6f,\"tick_p95_ms\":%.6f}",scroll||floor?",\n":"",floor,scroll?"camera_scroll":"standing",render[90],render[171],tick[90],tick[171]);
 }puts("\n]");return 0;
}
