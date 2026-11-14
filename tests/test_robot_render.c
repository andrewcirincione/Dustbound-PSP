/* Robots must survive the same filtered sprite cache used by actual world frames. */
#include "../src/render.c"
#include <assert.h>
static ActorSprite sprite;
static unsigned canvas[480*272];
static unsigned long long fingerprint(void){unsigned long long h=1469598103934665603ull;int painted=0;for(int i=0;i<ACT_W*ACT_H;i++){if(sprite.pixels[i]>>24)painted++;h=(h^sprite.pixels[i])*1099511628211ull;}assert(painted>80);assert(sprite.bounds.right>sprite.bounds.left&&sprite.bounds.bottom>sprite.bounds.top);return h;}
int main(void){game_new(73819,0);fb=canvas;pitch=480;clipLeft=clipTop=0;clipRight=480;clipBottom=272;int cases=0;
 for(int type=0;type<2;type++)for(int facing=0;facing<2;facing++){
  Actor a={0};a.active=1;a.kind=BEAST;a.race=2;a.voice=77+type;a.facing=facing;a.healthScale=150;a.target=a.attackTarget=a.carrying=a.carriedBy=-1;for(int l=0;l<LIMBS;l++)a.health[l]=150;
  unsigned long long hashes[4];
  for(int frame=0;frame<4;frame++){a.movedTimer=frame==1||frame==2?.1f:0;a.anim=frame==2?1.f:.1f;a.attackPhase=frame==3?2:0;a.attackDuration=1;a.attackClock=.5f;paint_cached_actor(&sprite,&a,1,0,0);hashes[frame]=fingerprint();for(int j=0;j<frame;j++)assert(hashes[j]!=hashes[frame]);cases++;}
  a.attackPhase=0;a.movedTimer=0;paint_cached_actor(&sprite,&a,1,0,0);fingerprint();int idleHeight=sprite.bounds.bottom-sprite.bounds.top;
  a.dead=1;paint_cached_actor(&sprite,&a,1,0,0);fingerprint();assert(sprite.bounds.bottom-sprite.bounds.top<idleHeight);a.dead=0;
  memset(canvas,0,sizeof(canvas));person(&a,240,160,1,0);int body=0;for(int y=70;y<155;y++)for(int x=180;x<300;x++)body+=(canvas[y*480+x]>>24)>0;assert(body>60);
  memset(canvas,0,sizeof(canvas));person(&a,240,160,1,0);int repeat=0;for(int y=70;y<155;y++)for(int x=180;x<300;x++)repeat+=(canvas[y*480+x]>>24)>0;assert(repeat==body);
 }
 printf("PASS: %d distinct cached robot poses across both types and facings; corpses remain visible, and reused world sprites retain their bodies.\n",cases);return 0;}
