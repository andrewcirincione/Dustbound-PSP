#include "../src/game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static int legChecks;
static void leg_check(const Actor*a,int i,int sc,float hx,float hy,float kx,float ky,float ax,float ay){
    (void)a;(void)i;
    assert(hypotf(kx-hx,ky-hy)/sc<7.2f);
    assert(hypotf(ax-kx,ay-ky)/sc<6.9f);
    assert(hypotf(ax-hx,ay-hy)/sc<13.6f);legChecks++;
}
#define DUST_LEG_OBSERVER leg_check
#include "../src/render.c"
static void sprite(const char*name,Actor*a,float stance){
    static ActorSprite picture;memset(&picture,0,sizeof(picture));paint_cached_actor(&picture,a,1,stance,0);
    char path[180];snprintf(path,sizeof(path),"build/v121/visual/%s.rgba",name);FILE*f=fopen(path,"wb");assert(f);assert(fwrite(picture.pixels,sizeof(picture.pixels),1,f)==1);fclose(f);
}
static void screen(const char*name){
    static uint32_t pixels[480*272];render_game(pixels,480);char path[180];snprintf(path,sizeof(path),"build/v121/visual/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<480*272;i++){unsigned char rgb[]={pixels[i]&255,pixels[i]>>8&255,pixels[i]>>16&255};fwrite(rgb,3,1,f);}fclose(f);
}
int main(void){
    mkdir("build/v121",0777);mkdir("build/v121/visual",0777);game_new(73819,0);Actor base=g.actors[0];
    base.kind=PLAYER;base.squad=base.active=1;base.dead=base.unconscious=base.taskKind=base.sneaking=base.hitFlash=0;base.carrying=base.carriedBy=base.target=-1;
    memset(base.gear,0,sizeof(base.gear));base.gear[4]=gear_id(69,0,0);base.gear[3]=BOOTS;base.body=0;base.race=0;
    char name[80];
    for(int tone=0;tone<8;tone++){Actor a=base;a.skin=tone;a.beard=0;a.hair=0;a.autoattack=0;snprintf(name,sizeof(name),"tone-%d",tone);sprite(name,&a,0);}
    for(int face=0;face<2;face++)for(int hair=0;hair<7;hair++)for(int beard=0;beard<8;beard++){
        Actor a=base;a.facing=face;a.skin=2;a.hair=hair;a.beard=beard;a.hairColor=1;
        a.autoattack=0;snprintf(name,sizeof(name),"face-%d-hair-%d-beard-%d",face,hair,beard);sprite(name,&a,0);
    }
    for(int bag=0;bag<8;bag++)for(int face=0;face<2;face++){
        Actor a=base;a.facing=face;a.gear[1]=LEATHER;a.gear[5]=gear_id(76+bag,0,0);a.autoattack=0;snprintf(name,sizeof(name),"pack-%d-face-%d",bag,face);sprite(name,&a,0);
    }
    for(int armor=0;armor<30;armor++)for(int face=0;face<2;face++){
        Actor a=base;armor_equip(&a,armor);a.facing=face;a.autoattack=0;snprintf(name,sizeof(name),"armor-%d-face-%d",armor,face);sprite(name,&a,0);
        a.movedTimer=.12f;a.routeTargetX=a.x+1;a.routeTargetY=a.y;
        for(int step=0;step<12;step++){a.anim=step*6.2831853f/12;snprintf(name,sizeof(name),"walk-%d-face-%d-step-%d",armor,face,step);sprite(name,&a,0);}
    }
    for(int family=0;family<2;family++)for(int face=0;face<2;face++){
        Actor a=base;armor_equip(&a,3);a.gear[0]=family?WEAPON_ITEMS+109:gear_id(0,1,1);a.facing=face;a.autoattack=family;snprintf(name,sizeof(name),"weapon-%d-face-%d",family,face);sprite(name,&a,0);
    }
    // A cached cap must exactly match a fresh render after other hats occupy the caches.
    Actor a=base;armor_equip(&a,19);a.facing=1;static uint32_t expected[ACT_W*ACT_H];static ActorSprite check;paint_cached_actor(&check,&a,1,0,0);memcpy(expected,check.pixels,sizeof(expected));
    for(int set=0;set<30;set++){Actor other=base;armor_equip(&other,set);paint_cached_actor(&check,&other,1,0,0);}
    paint_cached_actor(&check,&a,1,0,0);assert(!memcmp(expected,check.pixels,sizeof(expected)));
    g.actors[0]=a;ui.screen=SCREEN_PACK;ui.selection=PACK+2;ui.noticeTime=ui.feedCount=0;screen("equipped-inventory");
    ui.screen=SCREEN_TRADE;ui.sell=1;ui.tradeTown=0;ui.tradeSmith=TRADE_SMITH;ui.selection=PACK+2;screen("equipped-selling");
    ui.screen=SCREEN_FORGE;ui.selection=4;ui.forgeSlot=0;ui.forgeFamily=13;ui.forgeMaterial=2;ui.forgeQuality=2;screen("forge-weapon-stats");
    ui.forgeSlot=1;ui.forgeFamily=38;screen("forge-armor-stats");
    ui.screen=SCREEN_CONTRACT_NOTE;ui.offerMode=1;ui.offerTown=0;ui.offerKind=CONTRACT_FREIGHT;screen("contract-before-accept");
    ui.screen=SCREEN_WORLD;g.actors[0].taskKind=3;g.actors[0].fatigue=40;g.actors[0].health[LARM]=35;screen("rest-recovery");
    g.actors[0].taskKind=0;g.actors[0].unconscious=30;g.feedback.wakeTotal[0]=60;screen("consciousness-recovery");
    g.actors[0].unconscious=0;g.weather=2;g.time=13.25f;screen("rain");g.time+=.2f;screen("rain-moving");
    puts("PASS: 8 skin tones, 112 hair/beard/facing combinations, 16 back attachments, 60 outfits, 720 walking poses with fixed bone lengths, cap cache isolation, inventory/sales/forge/quest/recovery/rain screens.");
    assert(legChecks>1000);return 0;
}
