#include "../src/render.c"
#include <assert.h>
#include <stddef.h>
#include <sys/stat.h>
static uint32_t canvas[256*240],reference[256*240];
static unsigned char reached[256*240];
static int floodQueue[256*240];
static void connected_hips(void){
    memset(reached,0,sizeof(reached));int first=0,last=0,seed=-1;
    for(int y=94;y<108 && seed<0;y++)for(int x=124;x<133;x++)
        if(reference[y*256+x]>>24>=128){seed=y*256+x;break;}
    assert(seed>=0);floodQueue[last++]=seed;reached[seed]=1;
    while(first<last){
        int n=floodQueue[first++],x=n%256,y=n/256;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
            int xx=x+dx,yy=y+dy;
            if(xx<0||xx>=256||yy<0||yy>=240)continue;
            int p=yy*256+xx;
            if(!reached[p] && reference[p]>>24>=128){reached[p]=1;floodQueue[last++]=p;}
        }
    }
    for(int side=0;side<2;side++){
        int joined=0;
        for(int y=160;y<176;y++)for(int x=side?129:115;x<(side?142:128);x++)joined+=reached[y*256+x];
        assert(joined>4);
    }
}
static void draw_review(Actor *a){
    memset(canvas,0,sizeof(canvas));fb=canvas;pitch=256;clipLeft=clipTop=0;clipRight=256;clipBottom=240;
    actor_painted_stance(a,128,176,4,1,0,0,0);
}
static Actor fresh_armor(int set,int face){
    Actor a=g.actors[0];a.race=a.body=a.hair=a.beard=a.scar=0;a.facing=face;
    a.squad=a.active=1;a.dead=a.unconscious=a.taskKind=a.autoattack=a.attackPhase=a.movedTimer=a.hitFlash=0;
    a.carrying=a.carriedBy=a.target=-1;a.gear[0]=a.gear[5]=NONE;a.lostMask=a.prostheticMask=0;
    armor_equip(&a,set);return a;
}
int main(void){
    game_new(73819,0);int heavy=0,occluded=0,visible=0;
    for(int s=0;s<ARMOR_SETS;s++){
        assert(armorSprites[s*6].w>0);heavy+=s>=5&&armorSets[s].heavy;
        for(int p=0;p<4;p++){
            int it=armor_set_item(s,p);assert(armor_item_set(it)==s);assert(itemDefs[it].armor>0);
            for(int other=0;other<s;other++)assert(strcmp(itemDefs[it].name,itemDefs[armor_set_item(other,p)].name));
        }
        for(int face=0;face<2;face++){
            Actor a=fresh_armor(s,face);int near=face?0:1;
            a.lostMask=(1<<LARM)|(1<<RARM);draw_review(&a);memcpy(reference,canvas,sizeof(canvas));
            connected_hips();
            a.lostMask=1<<(LARM+near);draw_review(&a);
            /* Opaque trousers hide every rear-arm pixel through the hip and thigh. */
            for(int y=176-15*4;y<176-2*4;y++)for(int x=128-16;x<128+16;x++)
                if(reference[y*256+x]>>24==255){assert(canvas[y*256+x]==reference[y*256+x]);occluded++;}
            a.lostMask=0;draw_review(&a);
            int shoulderVisible=0;
            for(int y=176-25*4;y<176-19*4;y++)for(int x=128-20;x<128+20;x++)
                shoulderVisible+=canvas[y*256+x]!=reference[y*256+x];
            assert(shoulderVisible>3);visible+=shoulderVisible;
            int arms=depth_coverage(a.gear[1],LARM);assert(arms==armorSets[s].coverage);
            assert(depth_armor(&a,LARM,SABRE,arms-1)>0);assert(depth_armor(&a,LARM,SABRE,arms)==0);
        }
    }
    assert(heavy==15);
    for(int faction=UNION;faction<=MONKS;faction++)if(war_faction_index(faction)>=0){
        int prior=0;
        for(int rank=0;rank<3;rank++){
            Actor a=fresh_armor(0,1),b=a;a.faction=b.faction=faction;
            war_outfit(&a,ROLE_MARSHAL,0,rank+1);war_outfit(&b,ROLE_MARSHAL,3,rank+1);
            for(int p=1;p<5;p++)assert(a.gear[p]==b.gear[p]);
            assert(armor_item_set(a.gear[1])==armor_uniform_set(faction,rank));
            assert(itemDefs[a.gear[1]].armor>prior);prior=itemDefs[a.gear[1]].armor;
        }
    }
    /* New loot and sold pieces survive an ordinary save/load. */
    mkdir("build/armor-sets",0777);armor_equip(&g.actors[0],29);
    int loot=armor_set_item(23,2);inventory_add(&g.actors[0],loot,1);*market_stock(0,loot)=3;*market_venue(0,loot)=TRADE_SMITH;
    assert(save_game("build/armor-sets/roundtrip.sav"));game_new(3,0);assert(load_game("build/armor-sets/roundtrip.sav"));
    assert(g.actors[0].gear[1]==armor_set_item(29,0));assert(inventory_count(&g.actors[0],loot)>0);
    assert(*market_stock(0,loot)==3&&*market_venue(0,loot)==TRADE_SMITH);
    printf("PASS 30 integrated sets / 15 new heavy; %d trouser occlusions; %d visible shoulder pixels; 3 faction rank ladders; real save/load.\n",occluded,visible);
    return 0;
}
