#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[512*272+32],gallery[960*440];
static uint64_t silhouette(void){uint64_t hash=1469598103934665603ull;for(int y=0;y<272;y++)for(int x=0;x<480;x++){hash^=(canvas[y*512+x]>>24)>64;hash*=1099511628211ull;}return hash;}
static void ppm(const char *path){FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",clipRight,clipBottom);for(int y=0;y<clipBottom;y++)for(int x=0;x<clipRight;x++){uint32_t c=fb[y*pitch+x];unsigned char rgb[]={c&255,c>>8&255,c>>16&255};fwrite(rgb,1,3,f);}fclose(f);}
static int variant(int type,int v){return type<36?gear_id(weapon_family_index(type),v/4,v%4):(type==36?SCIMITAR_ITEMS:RAPIER_ITEMS)+v;}
int main(void){game_new(73819,0);fb=canvas;pitch=512;int minimum=24,poses=0;
 for(int type=0;type<38;type++){
  uint64_t hashes[24];int distinct=0;
  for(int v=0;v<24;v++){int it=variant(type,v);memset(canvas,0,sizeof(canvas));weapon_sprite(it,240,180,3,-1.5707963f,1,1);hashes[v]=silhouette();int same=0;for(int j=0;j<v;j++)same|=hashes[v]==hashes[j];distinct+=!same;
   for(int direction=0;direction<4;direction++)for(int phase=0;phase<4;phase++){
    int side=direction==1||direction==2?-1:1;
    Actor a=g.actors[0];a.gear[0]=it;a.gear[5]=0;a.attackWeapon=it;a.facing=side>0;a.attackPhase=phase;a.attackDuration=1;a.attackClock=.55f;a.attackAngle=direction*1.5707963f;
    memset(canvas,0,sizeof(canvas));for(int i=512*272;i<512*272+32;i++)canvas[i]=0xdeadbeef;actor_painted(&a,240,180,2,1);
    for(int i=512*272;i<512*272+32;i++)assert(canvas[i]==0xdeadbeef);
    for(int y=0;y<272;y++)for(int x=0;x<480;x++)if(canvas[y*512+x]>>24>=32){assert(abs(x-240)<ACT_W);assert(y-180>=-ACT_ANCHOR&&y-180<ACT_H*2-ACT_ANCHOR);}
    poses++;
   }
  }
  printf("VISUAL: %s: %d monochrome silhouettes / 24 variants\n",type<36?familyNames[weapon_family_index(type)]:type==36?"Scimitar":"Rapier",distinct);
  assert(distinct>=12);if(distinct<minimum)minimum=distinct;
  fb=gallery;pitch=960;clipRight=960;clipBottom=440;memset(gallery,0,sizeof(gallery));rect(0,0,960,440,INK);
  char label[120];snprintf(label,sizeof(label),"%s / 12 CONSTRUCTIONS",type<36?familyNames[weapon_family_index(type)]:type==36?"SCIMITAR":"RAPIER");text(12,10,label,GOLD,2);
  for(int k=0;k<12;k++){int v=(k/2)*4+(k%2)*2,it=variant(type,v),x=80+(k%6)*160,y=k<6?162:372;
   weapon_sprite(it,x,y,3,-1.5707963f,1,1);center(x,y+30,weapon_design_name(it),TEXT,1);center(x,y+42,materialNames[v/4],MUTED,1);
  }
  text(12,428,"PRODUCTION RENDERER / 3X WEAPON VIEW / EACH CONSTRUCTION HAS TWO QUALITY FINISHES",MUTED,1);
  char galleryPath[100];snprintf(galleryPath,sizeof(galleryPath),"build/v100/weapons-%02d.ppm",type);ppm(galleryPath);
  fb=canvas;pitch=512;clipRight=480;clipBottom=272;
  /* Real equipment fixtures for controller inspection in the PSP executable. */
  game_new(73819,0);Actor*a=&g.actors[g.selected];memset(a->pack,0,sizeof(a->pack));for(int p=0;p<12;p++){int v=(p/2)*4+(p%2)*2;assert(inventory_add(a,variant(type,v),1));}a->x=a->y=400;a->autoattack=0;for(int n=0;n<g.actorCount;n++)if(n!=g.selected)g.actors[n].active=0;g.dayTime=12;g.weather=0;ui.camX=a->x;ui.camY=a->y;
  char path[100];snprintf(path,sizeof(path),"build/v100/weapon-%02d.sav",type);assert(save_game(path));
 }
 printf("PASS: all 38 weapon types have at least %d monochrome silhouettes; %d equipped direction/phase renders.\n",minimum,poses);
 return 0;
}
