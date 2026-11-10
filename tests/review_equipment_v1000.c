/* Visual catalog using the production renderer, at its actual articulated scale. */
#include "../src/render.c"
#include <sys/stat.h>
static uint32_t reviewPixels[512*272];
static void save_frame(const char*root,const char*name){
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",root,name);FILE*f=fopen(path,"wb");if(!f){perror(path);exit(1);}fprintf(f,"P6\n480 272\n255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=reviewPixels[y*512+x];unsigned char rgb[]={c&255,c>>8&255,c>>16&255};fwrite(rgb,1,3,f);}fclose(f);
}
static void clear_frame(void){fb=reviewPixels;pitch=512;clipLeft=clipTop=0;clipRight=480;clipBottom=272;rect(0,0,480,272,0x313129);}
static void pose(Actor*a,int cell,int state,int face,int weapon){
 a->facing=face;a->gear[0]=a->attackWeapon=weapon;a->autoattack=state!=0;a->attackPhase=state==2?2:state==3?1:0;a->attackAngle=face?0:3.14159265f;a->attackClock=state==2?1-STRIKE_CONTACT:.4f;a->attackDuration=1;
 int x=60+(cell%4)*120,y=127+(cell/4)*136;
 actor_painted_stance(a,x,y,2,1,state?1:0,0,0);
}
int main(int argc,char**argv){
 const char*root=argc>1?argv[1]:"build/v1000/gear-review";mkdir(root,0777);game_new(73819,0);Actor a=g.actors[0];a.gear[2]=a.gear[5]=0;a.body=0;a.race=0;a.hitFlash=0;a.carrying=-1;a.movedTimer=0;
 for(int page=0;page<8;page++){
  clear_frame();
  for(int row=0;row<2;row++){
   int family=32+page*2+row;a.gear[1]=gear_id(family,1,1);
   for(int col=0;col<4;col++){
    int weapon=col==3?gear_id(84,1,1):SABRE;pose(&a,row*4+col,col,col==2?0:1,weapon);
    char label[40];snprintf(label,sizeof(label),"BODY %d / %s",family,col==0?"PASSIVE":col==3?"HEAVY":col==2?"STRIKE":"GUARD");text(col*120+5,row*136+5,label,TEXT,1);
   }
  }
  char name[40];snprintf(name,sizeof(name),"armor-%02d",page);save_frame(root,name);
 }
 a.gear[1]=gear_id(38,1,1);
 for(int page=0;page<5;page++){
  clear_frame();for(int cell=0;cell<8;cell++){
   int index=page*8+cell;if(index>=39)continue;
   int weapon=index==0?0:index<=WEAPON_FAMILIES?gear_id(weapon_family_index(index-1),1,1):index==37?SCIMITAR_ITEMS:RAPIER_ITEMS;
   pose(&a,cell,cell%3+1,cell%2,weapon);char label[64];snprintf(label,sizeof(label),"%d %.16s",index,weapon?itemDefs[weapon].name:"UNARMED");text(cell%4*120+5,cell/4*136+5,label,TEXT,1);
  }char name[40];snprintf(name,sizeof(name),"weapons-%02d",page);save_frame(root,name);
 }
 printf("Reviewed 16 armor silhouettes and 39 weapon/unarmed families in production-rendered sheets.\n");return 0;
}
