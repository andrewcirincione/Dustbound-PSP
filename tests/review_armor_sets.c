/* Actual catalog items through the production renderer; no art substitution. */
#include "../src/render.c"
#include <assert.h>
#include <sys/stat.h>
static uint32_t reviewPixels[480*272];
static void write_review(const char *root,int set,const char *kind){
    char path[512];snprintf(path,sizeof(path),"%s/%02d-%s.ppm",root,set+1,kind);
    FILE *f=fopen(path,"wb");assert(f);fputs("P6\n480 272\n255\n",f);
    for(int i=0;i<480*272;i++){uint32_t c=reviewPixels[i];unsigned char p[]={c&255,c>>8&255,c>>16&255};fwrite(p,1,3,f);}fclose(f);
}
static void setup(Actor *a,int set){
    a->body=a->race=a->hair=a->beard=a->scar=0;a->skin=2;a->hairColor=0;
    a->gear[0]=SABRE;a->gear[5]=NONE;a->facing=1;a->attackPhase=a->autoattack=a->movedTimer=a->hitFlash=0;
    a->carrying=a->carriedBy=a->target=-1;a->squad=a->active=1;a->dead=a->unconscious=a->sneaking=a->taskKind=0;
    a->lostMask=a->prostheticMask=0;armor_equip(a,set);
}
int main(int argc,char **argv){
    const char *root=argc>1?argv[1]:"build/armor-sets/frames";mkdir(root,0777);
    game_new(73819,0);
    for(int s=0;s<ARMOR_SETS;s++){
        if(!armorSprites[s*6].w)continue;
        Actor *a=&g.actors[0];setup(a,s);
        memset(a->pack,0,sizeof(a->pack));for(int p=0;p<4;p++)inventory_add(a,armor_set_item(s,p),1);
        ui.screen=SCREEN_PACK;ui.selection=0;ui.noticeTime=0;
        render_game(reviewPixels,480);write_review(root,s,"inventory");
        ui.screen=SCREEN_WORLD;ui.camX=a->x;ui.camY=a->y;g.dayTime=12;g.weather=0;
        for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
        render_game(reviewPixels,480);write_review(root,s,"world");
        fb=reviewPixels;pitch=480;clipLeft=clipTop=0;clipRight=480;clipBottom=272;
        rect(0,0,480,272,PANEL);text(12,9,armorSets[s].name,TEXT,2);
        char line[80];snprintf(line,sizeof(line),"%s / BODY ARMOR %d / BODY WEIGHT %d",armorSets[s].heavy?"HEAVY":armorSets[s].weight<=5?"LIGHT":"MEDIUM",armorSets[s].armor,armorSets[s].weight);text(12,31,line,GOLD,1);
        a->gear[0]=NONE;
        for(int c=0;c<4;c++){
            a->facing=c%2;a->autoattack=c/2;
            actor_painted_stance(a,60+c*120,202,4,1,c/2,0,0);
            center(60+c*120,223,c/2?"FIGHT":"PASSIVE",MUTED,1);
        }
        text(12,252,"DUST 1.0.2 / PRODUCTION ITEM AND CHARACTER RENDERER",MUTED,1);write_review(root,s,"poses");
        for(int c=0;c<4;c++){
            a->facing=c%2;a->gear[0]=a->attackWeapon=c/2?gear_id(4,1,1):gear_id(84,1,1);
            a->attackPhase=2;a->autoattack=1;a->attackDuration=1;a->attackClock=1-STRIKE_CONTACT;
            a->attackAngle=a->facing?0:3.14159265f;
            if(c==0){rect(0,0,480,272,PANEL);text(12,9,armorSets[s].name,TEXT,2);text(12,34,"GREATSWORD / NAGINATA / CONTACT POSES",GOLD,1);}
            actor_painted_stance(a,60+c*120,195,3,1,1,0,0);
        }
        write_review(root,s,"weapons");
        fb=reviewPixels;pitch=480;clipLeft=clipTop=0;clipRight=480;clipBottom=272;
        rect(0,0,480,272,PANEL);text(12,9,armorSets[s].name,TEXT,2);
        text(12,33,"DOWNED / CRAWLING / BOTH FACINGS",GOLD,1);
        for(int c=0;c<4;c++){
            setup(a,s);a->gear[0]=NONE;a->facing=c%2;
            if(c<2)a->unconscious=20;else a->health[LLEG]=a->health[RLEG]=0;
            actor_painted_stance(a,110+(c%2)*240,115+(c/2)*125,3,1,0,0,0);
        }
        write_review(root,s,"grounded");

    }
    for(int f=0;f<3;f++){
        const int factions[]={UNION,SHOGUN,MONKS};
        fb=reviewPixels;pitch=480;clipLeft=clipTop=0;clipRight=480;clipBottom=272;rect(0,0,480,272,PANEL);
        text(12,10,factionNames[factions[f]],TEXT,2);text(12,34,"STANDARD ISSUE / SAME FACTION AND RANK",GOLD,1);
        for(int rank=0;rank<3;rank++)for(int member=0;member<2;member++){
            Actor a=g.actors[0];setup(&a,armor_uniform_set(factions[f],rank));a.gear[0]=NONE;a.autoattack=0;a.facing=1;
            actor_painted_stance(&a,40+rank*160+member*65,202,4,1,0,0,0);
            if(!member)center(78+rank*160,224,rank==0?"SOLDIER":rank==1?"VETERAN":"OFFICER",TEAL,1);
        }
        write_review(root,f,"uniforms");
    }
    puts("Captured real catalog inventory, world, both passive/fight facings, greatsword/naginata and rank uniforms.");
    return 0;
}
