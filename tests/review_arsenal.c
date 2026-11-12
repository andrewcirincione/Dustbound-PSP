/* Each real catalog item, held by the production character compositor. */
#include "../src/render.c"
#include <assert.h>
#include <sys/stat.h>
static uint32_t canvas[480*272];
static void save_frame(const char*root,int n,const char*name){char path[512];snprintf(path,sizeof(path),"%s/weapon-%03d-%s.ppm",root,n,name);FILE*f=fopen(path,"wb");assert(f);fputs("P6\n480 272\n255\n",f);for(int i=0;i<480*272;i++){uint32_t c=canvas[i];unsigned char p[]={c&255,c>>8&255,c>>16&255};fwrite(p,1,3,f);}fclose(f);}
static Actor actor(int item,int face){Actor a=g.actors[0];a.gear[0]=a.attackWeapon=item;a.gear[1]=armor_set_item(0,0);a.gear[2]=a.gear[5]=NONE;a.race=a.body=a.hair=a.beard=0;a.skin=2;a.squad=a.active=1;a.autoattack=0;a.facing=face;a.target=a.carrying=a.carriedBy=-1;a.dead=a.unconscious=a.sneaking=a.taskKind=a.movedTimer=a.hitFlash=a.attackPhase=0;a.lostMask=a.prostheticMask=0;for(int l=0;l<LIMBS;l++)a.health[l]=species_health(&a);return a;}
int main(int argc,char**argv){const char*root=argc>1?argv[1]:"build/arsenal/review";mkdir(root,0777);game_new(73819,0);fb=canvas;pitch=480;clipLeft=clipTop=0;clipRight=480;clipBottom=272;
 for(int n=0;n<ARSENAL_WEAPONS;n++){
  if(!arsenalSprites[n].w)continue;int item=WEAPON_ITEMS+n;
  rect(0,0,480,272,PANEL);text(10,9,itemDefs[item].name,TEXT,2);char line[100];snprintf(line,sizeof(line),"DAMAGE %d / WEIGHT %d / PIERCE %d",itemDefs[item].power,itemDefs[item].weight,itemDefs[item].penetration);text(10,30,line,GOLD,1);
  weapon_sprite(item,45,208,3,-1.5707963f,1,1);
  for(int c=0;c<4;c++){Actor a=actor(item,c%2);a.autoattack=c>=2;a.attackAngle=a.facing?0:3.14159265f;actor_painted_stance(&a,118+c*101,218,3,1,c>=2,0,0);center(118+c*101,240,c>=2?"GUARD":"CARRY",TEAL,1);}
  save_frame(root,n,"held");
  rect(0,0,480,272,PANEL);text(10,9,itemDefs[item].name,TEXT,2);text(10,32,"REAL ATTACK POSES / BOTH FACINGS",GOLD,1);
  for(int c=0;c<6;c++){Actor a=actor(item,c%2);a.autoattack=1;a.attackPhase=2;a.attackDuration=1;a.attackClock=1-STRIKE_CONTACT;a.attackAngle=a.facing?0:3.14159265f;actor_painted_stance(&a,42+c*80,212,2,1,1,c/2,0);}
  save_frame(root,n,"attack");
  if(n>=100){
   rect(0,0,480,272,PANEL);text(10,9,itemDefs[item].name,TEXT,2);text(10,32,"SHOULDER REST / BOTH FACINGS",GOLD,1);
   for(int c=0;c<2;c++){Actor a=actor(item,c);a.autoattack=1;actor_painted_stance(&a,125+c*230,230,3,1,0,0,0);}
   save_frame(root,n,"shoulder");
  }
 }
 puts("Captured independently drawn weapons held on the actual rig in both facings, carry, guard and all three contact variants.");return 0;
}
