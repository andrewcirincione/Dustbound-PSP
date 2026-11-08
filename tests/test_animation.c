/* Numeric invariants and pictures use the actual production pose/world renderer. */
#include "../src/render.c"
#include <assert.h>
extern void init_actor(int,const char*,int,int,float,float,int);
static uint32_t canvas[512*272+64], reference[512*272];
static const char *outputDir = "build/v160";
static void write_ppm(const char*name){
 char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",outputDir,name);
 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t p=canvas[y*512+x];unsigned char c[]={p&255,p>>8&255,p>>16&255};fwrite(c,1,3,f);}fclose(f);
}
static void blank(void){memset(canvas,0,sizeof(canvas));fb=canvas;pitch=512;clipLeft=clipTop=0;clipRight=480;clipBottom=272;}
static void close_pose(CombatPose a,CombatPose b){
 assert(fabsf(a.handX-b.handX)<.001f&&fabsf(a.handY-b.handY)<.001f&&fabsf(a.blade-b.blade)<.001f);
 assert(fabsf(a.shift-b.shift)<.001f&&fabsf(a.step-b.step)<.001f&&fabsf(a.crouch-b.crouch)<.001f&&fabsf(a.twist-b.twist)<.001f);
 assert(fabsf(a.rearX-b.rearX)<.001f&&fabsf(a.rearY-b.rearY)<.001f);
}
static void reach_catalog(void){
 int entries=0;long combinations=0;
 for(int w=0;w<ITEMS;w++)if(!w||itemDefs[w].slot==0)for(int v=0;v<ATTACK_VARIANTS;v++){entries++;
  for(int ph=1;ph<=3;ph++)close_pose(combat_pose(w,ph,1,v),combat_pose(w,ph==3?0:ph+1,0,v));
  CombatPose l=combat_pose(w,2,STRIKE_CONTACT-.001f,v),c=combat_pose(w,2,STRIKE_CONTACT,v),r=combat_pose(w,2,STRIKE_CONTACT+.001f,v);
  assert(fabsf((c.blade-l.blade)-(r.blade-c.blade))<.0002f);
  for(int body=0;body<3;body++)for(int dir=0;dir<16;dir++)for(int phase=0;phase<4;phase++)for(int t=0;t<=12;t++)for(int primary=0;primary<2;primary++){
   CombatPose p=phase?combat_pose(w,phase,t/12.f,v):combat_idle(w);
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
 printf("PASS: %d weapon/unarmed motion sets, %ld grip/reach combinations, continuous phase/contact poses.\n",entries,combinations);
}
static Actor*fresh(void){
 game_new(73819,0);Actor*a=&g.actors[0];ui.screen=SCREEN_WORLD;a->autoattack=0;a->target=-1;a->x=a->y=380;g.dayTime=12;
 for(int i=1;i<g.actorCount;i++){g.actors[i].active=0;g.actors[i].target=-1;}
 for(int i=0;i<POP_GROUPS;i++){g.population.groups[i].x=g.population.groups[i].y=1500;}
 return a;
}
static void timing(void) {
 Actor *a = fresh();
 a->xp[STRENGTH] = a->xp[DEXTERITY] = 117612;
 int count = 0;
 for (int w = 0; w < ITEMS; w++) {
  if (w && itemDefs[w].slot != 0) continue;
  int c = weapon_class(w);
  assert(w == NONE || c >= 0);
  float *xp = w ? &g.society.weaponXP[0][c] : &a->xp[MARTIAL];
  float stroke[ATTACK_VARIANTS];
  for (int v = 0; v < ATTACK_VARIANTS; v++) {
   *xp = 0;
   CombatTiming novice = combat_timing(a, w, v);
   *xp = 117612;
   CombatTiming master = combat_timing(a, w, v);
   assert(master.windup <= novice.windup && master.stroke < novice.stroke * .9f);
   assert(master.recovery <= novice.recovery);
   stroke[v] = novice.stroke;
   for (int previous = 0; previous < v; previous++)
    assert(fabsf(stroke[v] - stroke[previous]) > .01f);
   count++;
  }
 }
 int heavy = gear_id(84, 2, 1);
 for (int v = 0; v < ATTACK_VARIANTS; v++) {
  float intact = combat_timing(a, heavy, v).stroke;
  a->lostMask = 1 << LARM;
  assert(combat_timing(a, heavy, v).stroke > intact);
  a->lostMask = 0;
  assert(combat_timing(a, heavy, v).stroke > combat_timing(a, gear_id(0, 2, 1), v).stroke);
 }
 puts("PASS: each motion has a distinct speed; proficiency accelerates every actual stroke; heavy blades and arm injuries retain their timing penalties.");
 printf("Covered %d catalog weapon/unarmed motion timings.\n", count);
}
static void attack_sequence(void) {
 Actor *a = fresh();
 init_actor(1, "Target", GUARD, HOSTILE, 380.8f, 380, 1);
 Actor *target = &g.actors[1];
 target->autoattack = 0;
 a->gear[0] = SABRE;
 int last = -1;
 unsigned seen = 0;
 for (int attack = 0; attack < 12; attack++) {
  a->cooldown = 0;
  target->stun = 0;
  target->x = 380.8f;
  target->y = 380;
  for (int limb = 0; limb < LIMBS; limb++) target->health[limb] = 100;
  target->blood = 100;
  uint32_t rng = g.rng;
  strike(0, 1);
  assert(a->attackPhase == 1 && g.rng == rng);
  int variant = combat_attack_variant(a);
  assert(variant != last && !(seen & (1u << variant)));
  seen |= 1u << variant;
  last = variant;
  if (attack % ATTACK_VARIANTS == ATTACK_VARIANTS - 1) {
   assert(seen == (1u << ATTACK_VARIANTS) - 1);
   seen = 0;
  }
  CombatTiming timing = combat_timing(a, a->attackWeapon, variant);
  target->stun = 100;
  assert(fabsf(a->attackDuration - timing.windup) < .001f);
  for (int frame = 0; a->attackPhase && frame < 1000; frame++) {
   assert(combat_attack_variant(a) == variant);
   float expected = a->attackPhase == 1 ? timing.windup : a->attackPhase == 2 ? timing.stroke : timing.recovery;
   assert(fabsf(a->attackDuration - expected) < .001f);
   simulate(.005f);
  }
  assert(a->attackPhase == 0);
 }
 puts("PASS: 12 live strikes cycle through all three motions without adjacent repeats or mid-strike changes.");
}
static void movement(void){
 Actor*a=fresh();float before=a->anim,x=a->x;move_actor(a,.4f,0,.1f);float delta=a->anim-before,moved=a->x-x;assert(moved>0&&fabsf(delta-moved*2.2f)<.001f);
 g.interiorBuilding=1;g.interiorActor=0;g.interiorX=.4f;g.interiorY=5.5f;a->movedTimer=0;before=a->anim;
 depth_move_room(a,-1,0,.1f);assert(a->anim==before&&a->movedTimer==0);
 a->attackPhase=2;g.interiorX=3;g.interiorY=5.5f;depth_move_room(a,1,0,.1f);assert(g.interiorX==3);g.interiorBuilding=0;
 puts("PASS: gait follows distance; blocked indoor movement and active strikes do not walk in place.");
}
static int sample_weapon(int i){return i==0?NONE:i<=WEAPON_FAMILIES?gear_id(weapon_family_index(i-1),1,1):i==37?SCIMITAR_ITEMS:RAPIER_ITEMS;}
static void sprite_cache_bounds(void) {
 for (unsigned alpha = 1; alpha <= 4 * 255; alpha++) {
  uint32_t reciprocal = alpha_reciprocal(alpha);
  for (unsigned channel = 0; channel <= 255; channel++) {
   unsigned sum = channel * alpha;
   assert(alpha_average(sum, alpha, reciprocal) == channel);
   if (sum) assert(alpha_average(sum - 1, alpha, reciprocal) == channel - 1);
  }
 }
 static uint32_t full[ACT_SCRATCH_STRIDE * ACT_H * 2];
 static ActorSprite cached;
 Actor a = *fresh();
 a.gear[2] = HELM;
 a.gear[5] = gear_id(77, 1, 1);
 for (int family = 0; family < 39; family++) {
  a.gear[0] = a.attackWeapon = sample_weapon(family);
  for (int pose = 0; pose < 6; pose++) {
   a.facing = pose % 2;
   a.attackAngle = a.facing ? .38f : 2.76f;
   a.attackPhase = pose < 4 ? pose : 0;
   a.attackClock = .44f;
   a.attackDuration = 1;
   a.unconscious = pose == 4;
   a.lostMask = pose == 5 ? 1 << LLEG : 0;
   int variant = family % ATTACK_VARIANTS;
   memset(full, 0, sizeof(full));
   fb = full;
   pitch = ACT_SCRATCH_STRIDE;
   clipLeft = clipTop = 0;
   clipRight = ACT_SCRATCH_STRIDE;
   clipBottom = ACT_H * 2;
   actor_painted_stance(&a, ACT_W, ACT_ANCHOR, 2, 1, 1, variant);
   paint_cached_actor(&cached, &a, 1, 1, variant);
   assert(!memcmp(full, actorScratch, sizeof(full)));
   for (int y = 0; y < ACT_H; y++) {
    for (int x = 0; x < ACT_W; x++) {
     const uint32_t *p = full + y * 2 * ACT_SCRATCH_STRIDE + x * 2;
     unsigned alpha = 0, red = 0, green = 0, blue = 0;
     for (int dy = 0; dy < 2; dy++) {
      for (int dx = 0; dx < 2; dx++) {
       uint32_t color = p[dy * ACT_SCRATCH_STRIDE + dx];
       unsigned a0 = color >> 24;
       alpha += a0;
       red += (color & 255) * a0;
       green += (color >> 8 & 255) * a0;
       blue += (color >> 16 & 255) * a0;
      }
     }
     uint32_t expected = alpha < 4 ? 0 : (alpha / 4 << 24) | (red / alpha) |
                                        (green / alpha << 8) | (blue / alpha << 16);
     assert(cached.pixels[y * ACT_W + x] == expected);
    }
   }
  }
 }
 blank();
 puts("PASS: bounded sprite filtering matches full-raster reference, including prone and crawling poses and cache reuse.");
}
static void sparring_save(int caseNumber, int weapon, int master) {
 Actor *a = fresh();
 a->x = 160;
 a->y = 352;
 a->gear[0] = weapon;
 a->gear[2] = a->gear[5] = 0;
 a->xp[STRENGTH] = 117612;
 a->xp[DEXTERITY] = 2700;
 a->xp[MARTIAL] = master ? 117612 : 0;
 if (weapon) g.society.weaponXP[0][weapon_class(weapon)] = master ? 117612 : 0;
 inventory_add(a, BOLTS, 99);
 inventory_add(a, CARTRIDGE, 99);
 a->autoattack = 1;
 a->target = 1;
 float spacing = caseNumber == 33 ? 1.73f : caseNumber == 1 ? 1.22f : .74f;
 assert(can_walk(a->x, a->y) && can_walk(a->x + spacing, a->y - .15f));
 init_actor(1, "Sparring guard", GUARD, HOSTILE, a->x + spacing, a->y - .15f, 5);
 Actor *b = &g.actors[1];
 b->gear[0] = SABRE;
 b->gear[1] = PLATE;
 b->gear[2] = HELM;
 b->autoattack = 0;
 b->cooldown = 100;
 b->xp[DEFENCE] = b->xp[TOUGHNESS] = 117612;
 g.living.command[0] = 1;
 char path[512];
 snprintf(path, sizeof(path), "%s/family-%02d%s.sav", outputDir, caseNumber, master ? "-master" : "");
 assert(save_game(path));
}
static void sheets(void){
 Actor*a=fresh();Actor b=*a;b.gear[1]=RAGS;b.gear[2]=b.gear[5]=0;
 for(int i=0;i<39;i++){
  int w=sample_weapon(i);b.gear[0]=b.attackWeapon=w;
  for(int v=0;v<ATTACK_VARIANTS;v++){
   blank();
   const char*names[]={"IDLE","GUARD","WINDUP","CONTACT","FOLLOW THROUGH","RECOVERY","WALK","MIRRORED HIT"};
   for(int j=0;j<8;j++){
    int x=60+(j%4)*120,y=135+(j/4)*131;b.facing=j!=7;b.attackAngle=j==7?3.14159265f:0;b.attackPhase=j==2?1:j==3||j==4||j==7?2:j==5?3:0;
    b.attackDuration=1;b.attackClock=j==3||j==7?1-STRIKE_CONTACT:j==5?.5f:0;b.movedTimer=j==6?.12f:0;b.anim=.5f;
    actor_painted_stance(&b,x,y,2,1,j==0||j==6?0:1,v);text(x-53,y-130,names[j],TEXT,1);
   }
   char name[64];snprintf(name,sizeof(name),"family-%02d-%d",i,v);write_ppm(name);
  }
  sparring_save(i,w,0);
  if(i==0||i==1||i==33)sparring_save(i,w,1);
 }
 puts("PASS: production-renderer pose sheets for all three motions of 39 weapon/unarmed families, plus ordinary PSP test saves.");
}
static void world_render(void){
 Actor*a=fresh();a->gear[0]=SABRE;a->gear[1]=0;g.depth.actors[0].layer=LEATHER;init_actor(1,"Raider",RAIDER,HOSTILE,382,380,5);
 a->autoattack=1;a->target=1;a->cooldown=100;g.actors[1].cooldown=100;g.actors[1].autoattack=0;g.actors[1].target=-1;
 blank();person(a,240,150,1,0);memcpy(reference,canvas,sizeof(reference));simulate(.2f);assert(combat_stance(a)>.9f);
 blank();person(a,240,150,1,0);assert(memcmp(reference,canvas,sizeof(reference)));write_ppm("world-guard-cache");
 for(int i=512*272;i<512*272+64;i++)canvas[i]=0xdeadbeef;ui.camX=a->x;ui.camY=a->y;render_game(canvas,512);
 for(int i=512*272;i<512*272+64;i++)assert(canvas[i]==0xdeadbeef);write_ppm("world-combat");
 Actor b=*a;b.kind=BEAST;b.attackPhase=0;b.dead=0;blank();actor_painted(&b,240,150,2,1);memcpy(reference,canvas,sizeof(reference));b.dead=1;blank();actor_painted(&b,240,150,2,1);assert(memcmp(reference,canvas,sizeof(reference)));write_ppm("downed-beast");
 char savePath[512];
 snprintf(savePath, sizeof(savePath), "%s/migration.sav", outputDir);
 assert(save_game(savePath));assert(load_game(savePath));assert(combat_stance(&g.actors[0])==0);
 puts("PASS: layered armor preserves live combat stance through the world sprite cache; framebuffer guard intact; downed beast differs; save roundtrip resets transient pose state.");
}
int main(int argc, char **argv) {
 if (argc > 1) outputDir = argv[1];
 content_init();reach_catalog();timing();attack_sequence();movement();sprite_cache_bounds();sheets();world_render();
 return 0;
}
