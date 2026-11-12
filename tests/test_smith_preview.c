#include "../src/render.c"
#include <assert.h>
static uint32_t screen[512*272+32];
int main(void){game_new(73819,0);fb=screen;pitch=512;clipLeft=clipTop=0;clipRight=480;clipBottom=272;
 int items[]={WEAPON_ITEMS+100,WEAPON_ITEMS+109,WEAPON_ITEMS+119,armor_set_item(9,0),armor_set_item(9,1),armor_set_item(9,2),armor_set_item(9,3)};uint64_t prior=0;
 for(int k=0;k<7;k++){
  memset(screen,0,sizeof(screen));for(int i=512*272;i<512*272+32;i++)screen[i]=0xdeadbeef;
  shop_item_picture(items[k],28,81,103,85);int painted=0;uint64_t h=0;
  for(int y=0;y<272;y++)for(int x=0;x<512;x++){uint32_t c=screen[y*512+x];if(c){assert(x>=28&&x<131&&y>=81&&y<166);painted++;}h=h*31+c;}
  assert(painted>30&&h!=prior);prior=h;for(int i=512*272;i<512*272+32;i++)assert(screen[i]==0xdeadbeef);
 }
 puts("PASS selected smith weapon/armor pictures differ and stay within their pane and framebuffer.");return 0;}
