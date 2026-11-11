/* Regressions reported during the user's visual review: torso occlusion,
   reflection, and arm motion through the real renderer. */
#include "../src/game.h"
static void observe_arm(const Actor*,int,int,int,float,float,float,float,float,float,int);
#define DUST_ARM_OBSERVER observe_arm
#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[256*240], reference[256*240];
static int geometryOnly;
static struct {float elbow[2],wrist[2];int seen;} observed[2];
static void observe_arm(const Actor *a,int i,int face,int sc,float sx,float sy,
                        float ex,float ey,float hx,float hy,int relaxed){
    (void)a;(void)face;(void)sx;(void)sy;(void)relaxed;
    observed[i].elbow[0]=ex/sc;observed[i].elbow[1]=ey/sc;
    observed[i].wrist[0]=hx/sc;observed[i].wrist[1]=hy/sc;observed[i].seen=1;
}
static void draw(Actor *a,int variant){
    if(!geometryOnly)memset(canvas,0,sizeof(canvas));memset(observed,0,sizeof(observed));
    fb=canvas;pitch=256;clipLeft=clipTop=0;clipRight=geometryOnly?0:256;clipBottom=geometryOnly?0:240;
    actor_painted_stance(a,128,160,3,1,1,variant,0);
}
static Actor fresh(void){
    Actor a=g.actors[0];a.autoattack=a.squad=1;a.active=1;
    a.carrying=a.carriedBy=a.target=-1;a.dead=a.unconscious=a.taskKind=0;
    a.hitFlash=a.sneaking=a.movedTimer=a.attackPhase=0;
    a.lostMask=a.prostheticMask=0;a.attackDuration=1;a.race=a.body=a.hair=a.beard=0;
    a.gear[2]=a.gear[5]=NONE;return a;
}
static int weapon(int n){return n==0?NONE:n<=36?gear_id(weapon_family_index(n-1),1,1):n==37?SCIMITAR_ITEMS:RAPIER_ITEMS;}
static void occlusion(void){
    int checked=0;
    for(int family=32;family<48;family++)for(int w=0;w<39;w++)for(int face=0;face<2;face++){
        Actor a=fresh();a.gear[0]=a.attackWeapon=weapon(w);a.gear[1]=gear_id(family,1,1);
        a.facing=face;a.attackAngle=face?0:3.14159265f;
        a.lostMask=(1<<LARM)|(1<<RARM);draw(&a,0);memcpy(reference,canvas,sizeof(reference));
        int near=face?0:1;a.lostMask=1<<(LARM+near);draw(&a,0);
        for(int y=160-24*3;y<160-14*3;y++)for(int x=128-15;x<128+15;x++)
            if(reference[y*256+x]>>24==255){
                if(canvas[y*256+x]!=reference[y*256+x]){
                    fprintf(stderr,"Rear arm painted across torso: family=%d w=%d face=%d at %d,%d\n",family,w,face,x,y);assert(0);
                }
                checked++;
            }
    }
    printf("PASS: %d opaque torso pixels retain occlusion of the rear arm/hand/weapon.\n",checked);
}
static void reflection(void){
    int cases=0,total=0,maxDiff=0;
    for(int w=0;w<39;w++)for(int variant=0;variant<3;variant++)for(int pose=0;pose<4;pose++){
        Actor a=fresh();a.gear[0]=a.attackWeapon=weapon(w);a.gear[1]=gear_id(32+w%16,1,1);
        a.attackPhase=pose;a.attackClock=pose==2?1-STRIKE_CONTACT:.4f;
        a.facing=1;a.attackAngle=0;draw(&a,variant);memcpy(reference,canvas,sizeof(reference));
        a.facing=0;a.attackAngle=3.14159265f;draw(&a,variant);
        int diff=0,visible=0;
        for(int y=0;y<240;y++)for(int x=0;x<256;x++){
            unsigned p=canvas[y*256+x],q=reference[y*256+255-x];
            if(p||q)visible++;
            if(p!=q)diff++;
        }
        if(diff>12+visible/25){fprintf(stderr,"Pose reflection mismatch: w=%d variant=%d pose=%d difference=%d visible=%d\n",w,variant,pose,diff,visible);assert(0);}
        cases++;total+=diff;if(diff>maxDiff)maxDiff=diff;
    }
    printf("PASS: %d actual left/right pose reflections (max raster differences %d).\n",cases,maxDiff);
}
static void motion(void){
    geometryOnly=1;
    float maximum=0,styleMax[39]={0};int frames=0,detail[39][5]={{0}};
    for(int style=0;style<39;style++)for(int v=0;v<3;v++)for(int dir=0;dir<16;dir++)for(int build=0;build<5;build++){
        float aim=dir*6.2831853f/16;
        int face=cosf(aim)>=0,body=build<3?build:build==3?1:2;
        Actor a=fresh();a.gear[0]=a.attackWeapon=weapon(style);
        a.gear[1]=gear_id(32+style%16,1,1);a.facing=face;a.body=body;a.race=build<3?0:build==3?4:3;a.attackAngle=aim;
        float previous[2][4];int have=0;
        for(int phase=1;phase<=3;phase++)for(int step=0;step<=64;step++){
            a.attackPhase=phase;a.attackClock=1-step/64.f;draw(&a,v);
            for(int i=0;i<2;i++){
                assert(observed[i].seen);
                float now[4]={observed[i].elbow[0],observed[i].elbow[1],observed[i].wrist[0],observed[i].wrist[1]};
                if(have){
                    float delta=hypotf(now[0]-previous[i][0],now[1]-previous[i][1]);
                    if(delta>maximum)maximum=delta;
                    if(delta>styleMax[style]){styleMax[style]=delta;detail[style][0]=v;detail[style][1]=face;detail[style][2]=body;detail[style][3]=phase;detail[style][4]=step;}
                    if(delta>.75f){fprintf(stderr,"Elbow jump: style=%d v=%d dir=%d body=%d race=%d phase=%d step=%d arm=%d delta=%g\n",style,v,dir,body,a.race,phase,step,i,delta);assert(0);}
                }
                memcpy(previous[i],now,sizeof(now));
            }
            have=1;frames++;
        }
    }
    printf("PASS: %d rendered arm-motion samples; largest elbow step %.3f world pixels.\n",frames,maximum);
    for(int i=0;i<39;i++)printf("  set=%d max=%.3f variant=%d face=%d body=%d phase=%d sample=%d\n",i,styleMax[i],detail[i][0],detail[i][1],detail[i][2],detail[i][3],detail[i][4]);
    geometryOnly=0;
}
int main(void){game_new(73819,0);occlusion();reflection();motion();return 0;}
