#include "../src/game.h"
static void arm_probe(const Actor*,int,int,int,float,float,float,float,float,float,int);
static void weapon_probe(int,float,float,int,float,int);
#define DUST_ARM_OBSERVER arm_probe
#define DUST_WEAPON_OBSERVER weapon_probe
#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[512*272+32],beforeWeapon[512*272],finalFigure[512*272],weaponOnly[512*272];
static int captureLayer,capturedLayer;
static struct {float wrist[2][2],shoulder[2][2],elbow[2][2],x,y,angle;int seen[2],sc,face,item;} probe;
static void arm_probe(const Actor*a,int i,int face,int sc,float sx,float sy,float ex,float ey,float hx,float hy,int relaxed){
 (void)a;(void)face;(void)relaxed;probe.wrist[i][0]=hx;probe.wrist[i][1]=hy;probe.shoulder[i][0]=sx;probe.shoulder[i][1]=sy;probe.elbow[i][0]=ex;probe.elbow[i][1]=ey;probe.seen[i]=1;
 if((!relaxed&&(fabsf(hypotf(ex-sx,ey-sy)-actorArmLength*sc)>=.05f||fabsf(hypotf(hx-ex,hy-ey)-actorArmLength*sc)>=.05f))||(relaxed&&(hypotf(ex-sx,ey-sy)>actorArmLength*sc+.1f||hypotf(hx-ex,hy-ey)>actorArmLength*sc+.1f))){fprintf(stderr,"Arm length case item %d race %d face %d phase %d arm %d: upper %.3f lower %.3f expected %.3f reach %.3f\n",a->gear[0]-WEAPON_ITEMS,a->race,face,a->attackPhase,i,hypotf(ex-sx,ey-sy),hypotf(hx-ex,hy-ey),actorArmLength*sc,hypotf(hx-sx,hy-sy));assert(0);}
}
static void weapon_probe(int item,float x,float y,int sc,float angle,int face){if(captureLayer){memcpy(beforeWeapon,canvas,sizeof(beforeWeapon));capturedLayer++;}probe.item=item;probe.x=x;probe.y=y;probe.sc=sc;probe.angle=angle;probe.face=face;}
static int opaque_near(const ArtSprite*s,float x,float y,int radius){for(int yy=(int)roundf(y)-radius;yy<=(int)roundf(y)+radius;yy++)for(int xx=(int)roundf(x)-radius;xx<=(int)roundf(x)+radius;xx++){
 if(xx<0||xx>=s->w||yy<0||yy>=s->h)continue;int lo=s->rows[yy*3+1],hi=s->rows[yy*3+2];if(xx>=lo&&xx<hi&&(s->palette[s->pixels[s->rows[yy*3]+xx-lo]]>>24)>=128)return 1;
 }return 0;}
int main(void){
#ifndef ARSENAL_ALLOW_INCOMPLETE
 assert(ARSENAL_READY==ARSENAL_WEAPONS);
#endif
 game_new(73819,0);fb=canvas;pitch=512;clipLeft=clipTop=0;clipRight=480;clipBottom=272;
 uint64_t hashes[ARSENAL_WEAPONS];int poseCases=0,grips=0;
 for(int n=0;n<ARSENAL_WEAPONS;n++){
  int item=WEAPON_ITEMS+n;const ArtSprite*s=&arsenalSprites[n];const WeaponArt*w=&arsenalWeaponArt[n];
#ifdef ARSENAL_ALLOW_INCOMPLETE
 if(!s->w)continue;
#endif
 assert(s->w>0&&s->h>0);assert(opaque_near(s,w->gripX,w->gripY,1));
  assert(weapon_art_id(item)==ARSENAL_ART_FIRST+n);
  memset(canvas,0,sizeof(canvas));weapon_sprite(item,240,190,3,-1.5707963f,1,1);
  uint64_t h=1469598103934665603ull;for(int y=0;y<272;y++)for(int x=0;x<480;x++){h^=(canvas[y*512+x]>>24)>64;h*=1099511628211ull;}hashes[n]=h;
  for(int k=0;k<n;k++)if(arsenalSprites[k].w)assert(hashes[k]!=h);
  /* Rasterize all directions and both facings: grip stays on actual pixels. */
  for(int direction=0;direction<16;direction++)for(int face=-1;face<=1;face+=2){
   memset(canvas,0,sizeof(canvas));for(int i=512*272;i<512*272+32;i++)canvas[i]=0xdeadbeef;
   weapon_sprite(item,240,140,3,direction*.39269908f,face,1);
   int hit=0;for(int y=136;y<144;y++)for(int x=236;x<244;x++)hit+=(canvas[y*512+x]>>24)>=64;assert(hit);
   for(int i=512*272;i<512*272+32;i++)assert(canvas[i]==0xdeadbeef);grips++;
  }
  /* Geometry-only passes still use the production compositor and shared poses. */
  for(int race=0;race<3;race++)for(int face=0;face<2;face++)for(int variant=0;variant<3;variant++)for(int phase=0;phase<4;phase++){
   Actor a=g.actors[0];a.gear[0]=a.attackWeapon=item;a.gear[5]=NONE;a.squad=a.active=a.autoattack=1;a.race=race==0?0:race==1?3:4;a.body=a.hair=a.beard=0;
   a.facing=face;a.target=a.carrying=a.carriedBy=-1;a.dead=a.unconscious=a.taskKind=a.sneaking=a.movedTimer=a.hitFlash=0;a.lostMask=a.prostheticMask=0;
   for(int l=0;l<LIMBS;l++)a.health[l]=species_health(&a);
   a.attackPhase=phase;a.attackDuration=1;a.attackClock=phase==2?1-STRIKE_CONTACT:.55f;a.attackAngle=face?0:3.14159265f;
   memset(&probe,0,sizeof(probe));clipRight=clipBottom=0;actor_painted_stance(&a,240,190,3,1,1,variant,0);
   assert(probe.seen[0]&&probe.seen[1]&&probe.item==item);
   int primary=hypotf(probe.wrist[0][0]-probe.x,probe.wrist[0][1]-probe.y)<.1f?0:1;
   assert(hypotf(probe.wrist[primary][0]-probe.x,probe.wrist[primary][1]-probe.y)<.1f);
   if(combat_two_hands(item)&&weapon_style(item)!=WS_BOW){
    int i=1-primary;float dx=(probe.wrist[i][0]-probe.x)/probe.sc,dy=(probe.wrist[i][1]-probe.y)/probe.sc;
    float ca=cosf(probe.angle),sa=sinf(probe.angle),along=dx*ca+dy*sa,across=(-dx*sa+dy*ca)/(w->edgeSide*probe.face);
    if(!opaque_near(s,w->gripX+across/w->scale,w->gripY-along/w->scale,2)){fprintf(stderr,"Support hand misses item %d, face %d phase %d\n",n,face,phase);assert(0);}
   }
   poseCases++;
  }
  clipRight=480;clipBottom=272;
 }
 int visibleOverBody=0,leadingOcclusions=0;
 for(int n=100;n<120;n++)for(int facing=0;facing<2;facing++){
  Actor a=g.actors[0];a.gear[0]=a.attackWeapon=WEAPON_ITEMS+n;a.gear[1]=armor_set_item(9,0);a.gear[2]=a.gear[5]=NONE;a.race=a.body=a.hair=a.beard=a.scar=0;
  a.facing=facing;a.active=a.squad=a.autoattack=1;a.attackPhase=a.hitFlash=a.movedTimer=a.unconscious=a.dead=a.taskKind=a.sneaking=0;a.lostMask=a.prostheticMask=0;a.target=a.carrying=a.carriedBy=-1;
  memset(canvas,0,sizeof(canvas));memset(&probe,0,sizeof(probe));capturedLayer=0;captureLayer=1;
  actor_painted_stance(&a,240,220,3,1,0,0,0);captureLayer=0;assert(capturedLayer==1);memcpy(finalFigure,canvas,sizeof(finalFigure));typeof(probe) pose=probe;
  int headPixels=0;for(int yy=125;yy<139;yy++)for(int xx=230;xx<250;xx++)headPixels+=(beforeWeapon[yy*512+xx]>>24)>240;
  assert(headPixels>20); /* Sword must be drawn after the head/body, not behind them. */
  memset(canvas,0,sizeof(canvas));weapon_sprite(pose.item,pose.x,pose.y,pose.sc,pose.angle,pose.face,1);memcpy(weaponOnly,canvas,sizeof(weaponOnly));
  int near=facing?0:1;memset(canvas,0,sizeof(canvas));painted_arm(&a,near,pose.shoulder[near],pose.wrist[near],3,facing?1:-1,0,actor_skin(&a),itemDefs[a.gear[1]].color,1,0,0,ARM_BOTH);
  for(int yy=0;yy<272;yy++)for(int xx=0;xx<480;xx++){
   int j=yy*512+xx;if((weaponOnly[j]>>24)!=255||(beforeWeapon[j]>>24)<240)continue;
   if(hypotf(xx-pose.wrist[0][0],yy-pose.wrist[0][1])<10||hypotf(xx-pose.wrist[1][0],yy-pose.wrist[1][1])<10)continue;
   if(!(canvas[j]>>24)){assert(finalFigure[j]==weaponOnly[j]);visibleOverBody++;}
   else if(finalFigure[j]!=weaponOnly[j])leadingOcclusions++;
  }
 }
 assert(visibleOverBody>100&&leadingOcclusions>20);
 printf("PASS shoulder-rest depth: %d blade pixels cover the body; %d overlaps remain behind the leading arm, across 20 greatswords and both facings.\n",visibleOverBody,leadingOcclusions);
 int cacheCases=0;
 for(int n=100;n<ARSENAL_WEAPONS;n++)for(int variant=0;variant<3;variant++)for(int phase=0;phase<4;phase++)for(int direction=0;direction<8;direction++)for(int sample=0;sample<=4;sample++){
  Actor a=g.actors[0];a.gear[0]=a.attackWeapon=WEAPON_ITEMS+n;a.gear[5]=NONE;a.autoattack=1;a.race=a.body=0;a.target=a.carrying=a.carriedBy=-1;
  a.attackPhase=phase;a.attackDuration=1;a.attackClock=1-sample/4.f;a.attackAngle=direction*.785398163f;a.facing=cosf(a.attackAngle)>=0;
  memset(canvas,0,sizeof(canvas));actor_painted_stance(&a,240,152,2,1,1,variant,0);
  for(int y=0;y<272;y++)for(int x=0;x<480;x++)if((canvas[y*512+x]>>24)>=32){int dx=x-240,dy=y-152;assert(dx>=-ACT_W&&dx<ACT_W&&dy>=-ACT_ANCHOR&&dy<ACT_H*2-ACT_ANCHOR);}
  cacheCases++;
 }
 printf("PASS %d new greatsword world-cache bounds poses across all three attack motions and eight directions.\n",cacheCases);
 printf("PASS 120 distinct silhouettes, %d rotated/mirrored grip checks and %d equipped anatomy/attack poses; main and support palms stay on the actual drawn handle.\n",grips,poseCases);
}
