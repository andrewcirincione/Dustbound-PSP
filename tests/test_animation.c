/* Numeric invariants and pictures use the actual production pose/world renderer. */
#include "../src/render.c"
#include <assert.h>
extern void init_actor(int,const char*,int,int,float,float,int);
static uint32_t canvas[512*272+64], reference[512*272];
static void write_ppm(const char*name){
 char path[160];snprintf(path,sizeof(path),"build/v160/%s.ppm",name);
 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t p=canvas[y*512+x];unsigned char c[]={p&255,p>>8&255,p>>16&255};fwrite(c,1,3,f);}fclose(f);
}
static void blank(void){memset(canvas,0,sizeof(canvas));fb=canvas;pitch=512;clipLeft=clipTop=0;clipRight=480;clipBottom=272;}
static void close_pose(CombatPose a,CombatPose b){
 assert(fabsf(a.handX-b.handX)<.001f&&fabsf(a.handY-b.handY)<.001f&&fabsf(a.blade-b.blade)<.001f);
 assert(fabsf(a.shift-b.shift)<.001f&&fabsf(a.step-b.step)<.001f&&fabsf(a.crouch-b.crouch)<.001f&&fabsf(a.twist-b.twist)<.001f);
}
static void reach_catalog(void){
 int entries=0;long combinations=0;
 for(int w=0;w<ITEMS;w++)if(!w||itemDefs[w].slot==0){entries++;
  for(int ph=1;ph<=3;ph++)close_pose(combat_pose(w,ph,1),combat_pose(w,ph==3?0:ph+1,0));
  CombatPose l=combat_pose(w,2,STRIKE_CONTACT-.001f),c=combat_pose(w,2,STRIKE_CONTACT),r=combat_pose(w,2,STRIKE_CONTACT+.001f);
  assert(fabsf((c.blade-l.blade)-(r.blade-c.blade))<.0002f);
  for(int body=0;body<3;body++)for(int dir=0;dir<16;dir++)for(int phase=0;phase<4;phase++)for(int t=0;t<=12;t++)for(int primary=0;primary<2;primary++){
   CombatPose p=phase?combat_pose(w,phase,t/12.f):combat_idle(w);
   float bw=body==0?3.1f:body==1?4.4f:5.9f,aim=dir*6.2831853f/16,face=cosf(aim)>=0?1:-1;
   float sx[2]={(-bw+1)*face*(1+p.twist),(bw-1)*face*(1+p.twist)};
   float hy=p.handY+sinf(aim)*p.handX*.4f;
   WeaponGrip g0=weapon_grip(cosf(aim)*p.handX*(1+p.twist),hy,aim+face*p.blade,1);
   g0=reachable_grip(g0,sx[primary],-24,sx[1-primary],-24,weapon_support_distance(w),combat_two_hands(w),13.5f);
   float x,y;weapon_hand(g0,w,0,&x,&y);assert(isfinite(x)&&isfinite(y)&&hypotf(x-sx[primary],y+24)<13.501f);
   if(combat_two_hands(w)){weapon_hand(g0,w,1,&x,&y);assert(hypotf(x-sx[1-primary],y+24)<13.501f);}
   combinations++;
  }
 }
 printf("PASS: %d weapon/unarmed entries, %ld grip/reach combinations, continuous phase/contact poses.\n",entries,combinations);
}
static Actor*fresh(void){
 game_new(73819,0);Actor*a=&g.actors[0];ui.screen=SCREEN_WORLD;a->autoattack=0;a->target=-1;a->x=a->y=380;g.dayTime=12;
 for(int i=1;i<g.actorCount;i++){g.actors[i].active=0;g.actors[i].target=-1;}
 for(int i=0;i<POP_GROUPS;i++){g.population.groups[i].x=g.population.groups[i].y=1500;}
 return a;
}
static void timing(void){
 Actor*a=fresh();for(int i=0;i<SKILLS;i++)a->xp[i]=117612;
 int count=0;for(int w=1;w<ITEMS;w++)if(itemDefs[w].slot==0&&weapon_class(w)>=0){int c=weapon_class(w);g.society.weaponXP[0][c]=0;float slow=combat_strike_seconds(a,w);g.society.weaponXP[0][c]=117612;float fast=combat_strike_seconds(a,w);assert(fast<slow*.9f);count++;}
 a->xp[MARTIAL]=0;float slow=combat_strike_seconds(a,NONE);a->xp[MARTIAL]=117612;assert(combat_strike_seconds(a,NONE)<slow);
 int heavy=gear_id(84,2,1);float intact=combat_strike_seconds(a,heavy);a->lostMask=1<<LARM;assert(combat_strike_seconds(a,heavy)>intact);a->lostMask=0;
 init_actor(1,"Target",RAIDER,HOSTILE,380.8f,380,1);g.actors[1].autoattack=0;g.actors[1].stun=100;
 a->gear[0]=SABRE;a->target=1;a->cooldown=0;g.actors[1].stun=0;strike(0,1);g.actors[1].stun=100;assert(a->attackPhase==1);
 while(a->attackPhase==1)simulate(.005f);
 assert(a->attackPhase==2&&fabsf(a->attackDuration-combat_strike_seconds(a,SABRE))<.001f);
 printf("PASS: proficiency accelerates actual strokes for %d catalog weapons and unarmed; missing-arm penalty and simulation phase duration.\n",count);
}
static void movement(void){
 Actor*a=fresh();float before=a->anim,x=a->x;move_actor(a,.4f,0,.1f);float delta=a->anim-before,moved=a->x-x;assert(moved>0&&fabsf(delta-moved*2.2f)<.001f);
 g.interiorBuilding=1;g.interiorActor=0;g.interiorX=.4f;g.interiorY=5.5f;a->movedTimer=0;before=a->anim;
 depth_move_room(a,-1,0,.1f);assert(a->anim==before&&a->movedTimer==0);
 a->attackPhase=2;g.interiorX=3;g.interiorY=5.5f;depth_move_room(a,1,0,.1f);assert(g.interiorX==3);g.interiorBuilding=0;
 puts("PASS: gait follows distance; blocked indoor movement and active strikes do not walk in place.");
}
static int sample_weapon(int i){return i==0?NONE:i<=WEAPON_FAMILIES?gear_id(weapon_family_index(i-1),1,1):i==37?SCIMITAR_ITEMS:RAPIER_ITEMS;}
static void sheets(void){
 Actor*a=fresh();Actor b=*a;b.gear[1]=RAGS;b.gear[2]=b.gear[5]=0;
 for(int i=0;i<39;i++){
  blank();int w=sample_weapon(i);b.gear[0]=b.attackWeapon=w;
  const char*names[]={"IDLE","GUARD","WINDUP","CONTACT","FOLLOW THROUGH","RECOVERY","WALK","LEFT GUARD"};
  for(int j=0;j<8;j++){
   int x=60+(j%4)*120,y=119+(j/4)*130;b.facing=j!=7;b.attackAngle=j==7?3.14159265f:0;b.attackPhase=j==2?1:j==3||j==4?2:j==5?3:0;
   b.attackDuration=1;b.attackClock=j==3?1-STRIKE_CONTACT:j==5?.5f:0;b.movedTimer=j==6?.12f:0;b.anim=.5f;
   actor_painted_stance(&b,x,y,2,1,j==0||j==6?0:1);text(x-53,y-115,names[j],TEXT,1);
  }
  char name[64];snprintf(name,sizeof(name),"family-%02d",i);write_ppm(name);
  /* Ordinary saved combat scenario for controller-driven EBOOT testing. */
  a=fresh();a->gear[0]=w;inventory_add(a,BOLTS,99);inventory_add(a,CARTRIDGE,99);a->autoattack=1;a->target=1;
  init_actor(1,"Sparring raider",RAIDER,HOSTILE,381,380,5);g.actors[1].gear[0]=SABRE;g.actors[1].autoattack=0;g.actors[1].cooldown=100;g.actors[1].target=-1;g.actors[1].health[HEAD]=g.actors[1].health[CHEST]=100;
  g.living.command[0]=1;char path[100];snprintf(path,sizeof(path),"build/v160/family-%02d.sav",i);assert(save_game(path));
 }
 puts("PASS: production-renderer idle/guard/windup/impact/follow-through/recovery/walk sheets and ordinary saves for 39 weapon/unarmed cases.");
}
static void world_render(void){
 Actor*a=fresh();a->gear[0]=SABRE;a->gear[1]=0;g.depth.actors[0].layer=LEATHER;init_actor(1,"Raider",RAIDER,HOSTILE,382,380,5);
 a->autoattack=1;a->target=1;a->cooldown=100;g.actors[1].cooldown=100;g.actors[1].autoattack=0;g.actors[1].target=-1;
 blank();person(a,240,150,1,0);memcpy(reference,canvas,sizeof(reference));simulate(.2f);assert(combat_stance(a)>.9f);
 blank();person(a,240,150,1,0);assert(memcmp(reference,canvas,sizeof(reference)));write_ppm("world-guard-cache");
 for(int i=512*272;i<512*272+64;i++)canvas[i]=0xdeadbeef;ui.camX=a->x;ui.camY=a->y;render_game(canvas,512);
 for(int i=512*272;i<512*272+64;i++)assert(canvas[i]==0xdeadbeef);write_ppm("world-combat");
 Actor b=*a;b.kind=BEAST;b.attackPhase=0;b.dead=0;blank();actor_painted(&b,240,150,2,1);memcpy(reference,canvas,sizeof(reference));b.dead=1;blank();actor_painted(&b,240,150,2,1);assert(memcmp(reference,canvas,sizeof(reference)));write_ppm("downed-beast");
 assert(save_game("build/v160/migration.sav"));assert(load_game("build/v160/migration.sav"));assert(combat_stance(&g.actors[0])==0);
 puts("PASS: layered armor preserves live combat stance through the world sprite cache; framebuffer guard intact; downed beast differs; save roundtrip resets transient pose state.");
}
int main(void){content_init();reach_catalog();timing();movement();sheets();world_render();return 0;}
