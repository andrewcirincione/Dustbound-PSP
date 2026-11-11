/* Review output comes from the same filtered actor compositor as the world. */
#include "../src/game.h"
#include <string.h>
static float reviewJoints[2][6];
static void review_arm(const Actor *a,int i,int face,int sc,float sx,float sy,float ex,float ey,float hx,float hy,int relaxed){
    (void)a;(void)face;(void)sc;(void)relaxed;
    float p[6]={sx,sy,ex,ey,hx,hy};memcpy(reviewJoints[i],p,sizeof(p));
}
#define DUST_ARM_OBSERVER review_arm
#include "../src/render.c"
#include <sys/stat.h>
#include <assert.h>

static void output(const char *root,const char *name,Actor *a,float stance,int variant){
    static ActorSprite sprite;
    memset(&sprite,0,sizeof(sprite));
    memset(reviewJoints,0,sizeof(reviewJoints));
    paint_cached_actor(&sprite,a,1,stance,variant);
    char path[512];snprintf(path,sizeof(path),"%s/%s.rgba",root,name);
    FILE *f=fopen(path,"wb");assert(f);assert(fwrite(sprite.pixels,sizeof(sprite.pixels),1,f)==1);fclose(f);
    /* Menu portraits sample this same 2x source before the world filter. */
    snprintf(path,sizeof(path),"%s/%s-hi.rgba",root,name);
    f=fopen(path,"wb");assert(f);assert(fwrite(actorScratch,sizeof(actorScratch),1,f)==1);fclose(f);
    snprintf(path,sizeof(path),"%s/%s-joints.bin",root,name);
    f=fopen(path,"wb");assert(f);assert(fwrite(reviewJoints,sizeof(reviewJoints),1,f)==1);fclose(f);
}
int main(int argc,char **argv){
    const char *root=argc>1?argv[1]:"build/pose-review/after";mkdir(root,0777);
    int stills=argc>2&&!strcmp(argv[2],"--stills");
    game_new(73819,0);Actor base=g.actors[0];
    unsigned seed=0x20261111u;
    FILE *manifest;char path[512];snprintf(path,sizeof(path),"%s/loadouts.json",root);manifest=fopen(path,"w");assert(manifest);fputs("[\n",manifest);
    for(int n=0;n<16;n++){
        Actor a=base;a.kind=PLAYER;a.squad=a.active=1;a.dead=a.unconscious=a.taskKind=0;
        a.carrying=a.carriedBy=a.target=-1;a.hitFlash=a.sneaking=a.movedTimer=a.attackPhase=0;
        a.lostMask=a.prostheticMask=0;a.scar=0;a.body=n%3;a.race=n==8?2:n==12?3:n==15?4:0;
        a.skin=n%8;a.hair=n%7;a.beard=n%3;a.hairColor=n%6;
        int family=n==1?36:n==4?33:32+n,weaponFamily=weapon_family_index((n*7+9)%WEAPON_FAMILIES);
        a.gear[0]=n==0?SABRE:n==1?gear_id(84,4,1):n==5?NONE:gear_id(weaponFamily,n%5,n%3);
        if(n==2)a.gear[0]=gear_id(4,2,1);
        if(n==4)a.gear[0]=RAPIER_ITEMS+4;
        if(n==7)a.gear[0]=CROSSBOW;
        if(n==11)a.gear[0]=gear_id(24,1,1);
        a.gear[1]=gear_id(family,n==1?4:n%5,1);
        a.gear[2]=n==5?NONE:n==0?HAT:gear_id(48+((seed=seed*1664525u+1013904223u)%12),n%4,n%2);
        a.gear[3]=gear_id(60+n%8,n%4,1);a.gear[4]=gear_id(68+(n*3)%8,n%4,1);
        a.gear[5]=n==3||n==7||n==10?gear_id(76+n%8,n%4,1):NONE;
        if(n==0){a.gear[1]=DUSTER;a.body=a.race=0;a.hair=0;a.beard=0;}
        a.attackWeapon=a.gear[0];a.attackDuration=1;
        fprintf(manifest,"%s{\"id\":%d,\"armor\":\"%s\",\"weapon\":\"%s\",\"head\":\"%s\",\"race\":%d,\"body\":%d}",n?",\n":"",n+1,itemDefs[a.gear[1]].name,a.gear[0]?itemDefs[a.gear[0]].name:"Unarmed",a.gear[2]?itemDefs[a.gear[2]].name:"None",a.race,a.body);
        for(int face=0;face<2;face++)for(int mode=0;mode<2;mode++){
            a.facing=face;a.autoattack=mode;a.attackPhase=0;a.attackAngle=face?0:3.14159265f;
            char name[80];snprintf(name,sizeof(name),"%02d-%c-%s",n+1,face?'R':'L',mode?"fight":"passive");
            output(root,name,&a,mode,0);
        }
        if(!stills)for(int face=0;face<2;face++)for(int variant=0;variant<3;variant++)for(int frame=0;frame<48;frame++){
            a.facing=face;a.autoattack=1;a.attackAngle=face?0:3.14159265f;a.attackPhase=frame<12?1:frame<36?2:3;
            float p=frame<12?frame/11.f:frame<36?(frame-12)/23.f:(frame-36)/11.f;
            a.attackClock=1-p;
            char name[80];snprintf(name,sizeof(name),"motion-%02d-%c-v%d-%02d",n+1,face?'R':'L',variant,frame);output(root,name,&a,1,variant);
        }
        a.attackPhase=0;a.attackClock=0;
        if(!stills)for(int face=0;face<2;face++)for(int frame=0;frame<16;frame++){
            a.facing=face;a.autoattack=0;a.movedTimer=.12f;a.anim=frame*6.2831853f/16;
            a.attackAngle=face?0:3.14159265f;
            char name[80];snprintf(name,sizeof(name),"walk-%02d-%c-%02d",n+1,face?'R':'L',frame);
            output(root,name,&a,0,0);
        }
        a.movedTimer=0;a.autoattack=1;a.facing=1;a.attackAngle=0;
        a.hitFlash=.27f;a.impactKind=2;
        char name[80];snprintf(name,sizeof(name),"block-%02d",n+1);output(root,name,&a,1,0);
        a.hitFlash=a.impactKind=0;
        for(int face=0;face<2;face++){
            a.facing=face;a.unconscious=10;
            snprintf(name,sizeof(name),"downed-%02d-%c",n+1,face?'R':'L');output(root,name,&a,0,0);
            if(face){snprintf(name,sizeof(name),"downed-%02d",n+1);output(root,name,&a,0,0);}
            a.unconscious=0;a.health[LLEG]=a.health[RLEG]=0;
            snprintf(name,sizeof(name),"crawl-%02d-%c",n+1,face?'R':'L');output(root,name,&a,0,0);
            a.health[LLEG]=limb_max_health(&a,LLEG);a.health[RLEG]=limb_max_health(&a,RLEG);
        }
    }
    fputs("\n]\n",manifest);fclose(manifest);
    for(int body=0;body<3;body++)for(int face=0;face<2;face++)for(int mode=0;mode<2;mode++){
        Actor a=base;a.kind=PLAYER;a.squad=a.active=1;a.dead=a.unconscious=a.taskKind=0;
        a.carrying=a.carriedBy=a.target=-1;a.hitFlash=a.sneaking=a.movedTimer=a.attackPhase=0;
        a.lostMask=a.prostheticMask=0;a.scar=0;a.body=body;a.race=0;a.beard=a.hair=0;
        a.gear[0]=a.gear[1]=a.gear[2]=a.gear[5]=NONE;a.facing=face;a.autoattack=mode;
        a.attackAngle=face?0:3.14159265f;
        char name[80];snprintf(name,sizeof(name),"bare-%d-%c-%s",body,face?'R':'L',mode?"fight":"passive");
        output(root,name,&a,mode,0);
    }
    puts("Wrote passive/fight, block, prone, crawling and unarmored stills.");
    if(stills)return 0;
    snprintf(path,sizeof(path),"%s/catalog.json",root);manifest=fopen(path,"w");assert(manifest);fputs("[\n",manifest);
    for(int n=0;n<39;n++){
        Actor a=base;a.kind=PLAYER;a.squad=a.active=1;a.dead=a.unconscious=a.taskKind=0;
        a.carrying=a.carriedBy=a.target=-1;a.hitFlash=a.sneaking=a.movedTimer=0;
        a.lostMask=a.prostheticMask=0;a.scar=0;a.body=n%3;
        a.gear[0]=a.attackWeapon=n==0?NONE:n<=36?gear_id(weapon_family_index(n-1),n%5,1):n==37?SCIMITAR_ITEMS+4:RAPIER_ITEMS+4;
        a.gear[1]=gear_id(32+n%16,n%5,1);a.gear[2]=gear_id(48+n%12,n%4,1);a.gear[5]=NONE;
        a.attackDuration=1;a.autoattack=1;
        fprintf(manifest,"%s{\"id\":%d,\"armor\":\"%s\",\"weapon\":\"%s\"}",n?",\n":"",n,a.gear[1]?itemDefs[a.gear[1]].name:"None",a.gear[0]?itemDefs[a.gear[0]].name:"Unarmed");
        for(int face=0;face<2;face++)for(int variant=0;variant<3;variant++)for(int pose=0;pose<5;pose++){
            a.facing=face;a.attackAngle=face?0:3.14159265f;
            a.attackPhase=pose==0?0:pose==1?1:pose<4?2:3;
            a.attackClock=pose==2?1-STRIKE_CONTACT:pose==4?.5f:0;
            char name[80];snprintf(name,sizeof(name),"catalog-%02d-%c-v%d-p%d",n,face?'R':'L',variant,pose);
            output(root,name,&a,1,variant);
        }
    }
    fputs("\n]\n",manifest);fclose(manifest);
    puts("Also wrote 4608 attack frames, walking and both facings of all three attacks for 39 weapon/unarmed families.");
    return 0;
}
