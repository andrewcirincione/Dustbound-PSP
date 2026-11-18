/* Accepted visual behavior: the front hand carries the passive greatsword,
   the elbow is a right angle, and combat still uses both painted grips. */
#include "../src/game.h"
static void arm_probe(const Actor*,int,int,int,float,float,float,float,float,float,int);
static void weapon_probe(int,float,float,int,float,int);
#define DUST_ARM_OBSERVER arm_probe
#define DUST_WEAPON_OBSERVER weapon_probe
#include "../src/render.c"
#include <assert.h>

static struct {float s[2],e[2],h[2];int seen,relaxed;} arm[2];
static struct {float x,y,angle;int sc,face,seen;} sword;
static uint32_t canvas[512*272];
static void arm_probe(const Actor *a,int i,int face,int sc,float sx,float sy,float ex,float ey,float hx,float hy,int relaxed){
    (void)a;(void)face;(void)sc;
    arm[i].s[0]=sx;arm[i].s[1]=sy;arm[i].e[0]=ex;arm[i].e[1]=ey;
    arm[i].h[0]=hx;arm[i].h[1]=hy;arm[i].seen=1;arm[i].relaxed=relaxed;
}
static void weapon_probe(int item,float x,float y,int sc,float angle,int face){
    (void)item;sword.x=x;sword.y=y;sword.angle=angle;sword.sc=sc;sword.face=face;sword.seen=1;
}
static void draw(Actor *a,float stance){
    memset(arm,0,sizeof(arm));memset(&sword,0,sizeof(sword));
    fb=canvas;pitch=512;clipLeft=clipTop=clipRight=clipBottom=0;
    actor_painted_stance(a,240,190,3,1,stance,0,0);
    assert(sword.seen);
}
static void right_angle(int hand){
    float ux=arm[hand].s[0]-arm[hand].e[0],uy=arm[hand].s[1]-arm[hand].e[1];
    float vx=arm[hand].h[0]-arm[hand].e[0],vy=arm[hand].h[1]-arm[hand].e[1];
    float u=hypotf(ux,uy),v=hypotf(vx,vy);
    assert(u>1&&v>1&&fabsf((ux*vx+uy*vy)/(u*v))<.001f);
    assert(fabsf(u-actorArmLength*3)<.01f&&fabsf(v-actorArmLength*3)<.01f);
}
int main(void){
    game_new(73819,0);int passive=0,guards=0,limbCases=0;
    for(int w=0;w<=20;w++)for(int race=0;race<SPECIES;race++)for(int body=0;body<3;body++)for(int face=0;face<2;face++){
        Actor a=g.actors[0];a.gear[0]=a.attackWeapon=w<20?WEAPON_ITEMS+100+w:gear_id(84,2,1);
        armor_equip(&a,(w*7)%ARMOR_SETS);a.gear[2]=a.gear[5]=NONE;
        a.race=race;a.body=body;a.facing=face;a.squad=a.active=1;
        a.target=a.carrying=a.carriedBy=-1;a.autoattack=a.attackPhase=a.hitFlash=a.movedTimer=0;
        a.dead=a.unconscious=a.taskKind=a.sneaking=0;a.lostMask=a.prostheticMask=0;
        for(int l=0;l<LIMBS;l++)a.health[l]=species_health(&a);
        draw(&a,0);int near=face?0:1,far=1-near;
        assert(arm[near].seen&&arm[far].seen&&!arm[near].relaxed&&arm[far].relaxed);
        assert(hypotf(arm[near].h[0]-sword.x,arm[near].h[1]-sword.y)<.01f);
        assert(arm[far].h[1]>arm[far].s[1]+8*sword.sc);
        assert(hypotf(arm[far].h[0]-sword.x,arm[far].h[1]-sword.y)>4*sword.sc);
        assert(fabsf(cosf(sword.angle)*(face?1:-1)+.70710678f)<.0001f);
        assert(fabsf(sinf(sword.angle)+.70710678f)<.0001f);
        right_angle(near);passive++;
        a.autoattack=1;draw(&a,1);
        int primary=hypotf(arm[0].h[0]-sword.x,arm[0].h[1]-sword.y)<.01f?0:1;
        assert(hypotf(arm[primary].h[0]-sword.x,arm[primary].h[1]-sword.y)<.01f);
        WeaponGrip grip=weapon_grip(sword.x,sword.y,sword.angle,sword.sc);
        float across=arsenal_support_across(a.gear[0])*sword.face;
        float hx=grip.x+grip.ux*weapon_support_distance(a.gear[0])-grip.uy*across;
        float hy=grip.y+grip.uy*weapon_support_distance(a.gear[0])+grip.ux*across;
        assert(hypotf(arm[1-primary].h[0]-hx,arm[1-primary].h[1]-hy)<.01f);guards++;
        for(int lost=0;lost<2;lost++){
            a.autoattack=0;a.lostMask=1<<(LARM+lost);draw(&a,0);int remaining=1-lost;
            assert(!arm[lost].seen&&arm[remaining].seen);
            assert(hypotf(arm[remaining].h[0]-sword.x,arm[remaining].h[1]-sword.y)<.01f);
            right_angle(remaining);limbCases++;
        }
    }
    printf("PASS: %d passive front-hand carries with 90-degree elbows and 45-degree blades; %d two-handed combat guards; %d one-arm fallbacks across 21 greatswords, five species, three builds and both facings.\n",passive,guards,limbCases);
    return 0;
}
