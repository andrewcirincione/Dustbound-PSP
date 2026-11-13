#ifndef DUSTBOUND_WEAPON_DESIGN_H
#define DUSTBOUND_WEAPON_DESIGN_H
/* Twelve constructions per weapon family, each with two quality finishes.
   Item IDs stay stable. The handle and support-hand anchors never move. */
typedef struct {float length,width,sweep;int pattern,group;} WeaponDesign;
static inline WeaponDesign weapon_design(int item){
 static const float length[12]={.86f,.97f,1.18f,1.10f,1.04f,.91f,1.02f,1.03f,.96f,1.13f,1.06f,1.16f};
 static const float width[12]={.92f,1.5f,.86f,.65f,1.08f,1.17f,1.25f,.90f,1.30f,1.20f,1,.90f};
 static const float sweep[12]={0,0,0,0,1.6f,-1,0,0,-2,0,0,1.2f};
 int f=gear_family(item),owner=regalia_owner(item),p=owner>=0?(owner*7+3)%12:(itemDefs[item].material*2+itemDefs[item].quality/2)%12;
 if(item<BASE_ITEMS)p=0;
 int group=f>=29&&f<=31?3:f>=24&&f<=28?2:f==4||f==5||f==6||f==12||f==13||f==14||f==15||f==16||f==23||f==85||f==86?1:0;
 int model=arsenal_index(item);
 if(model>=0){
  /* Trim the oversized greatsword blades, leaving the five-unit grip zone
     and both palms fixed. The three compact designs retain their size. */
  int compact=model==100||model==101||model==103;
  int oversized=f==84&&!compact;
  return (WeaponDesign){oversized?.86f:1,oversized?.94f:1,0,0,group};
 }
 return (WeaponDesign){length[p],width[p],sweep[p],p,group};
}
static inline const char *weapon_design_name(int item){
 static const char *names[4][12]={
  {"Plain","Broad","Long","Needle","Swept","Clipped","Split-guard","Basket","Hooked","Serrated","Ring-guard","Crested"},
  {"Short-haft","Broad-head","Long-haft","Narrow-head","Swept","Spike-cap","Forked","Flanged","Hooked","Studded","Ringed","Crested"},
  {"Compact","Wide-limb","Longstock","Slim-limb","Recurve","Reinforced","Forked-limb","Braced","Hook-tip","Laminated","Ringed","Crested"},
  {"Short-barrel","Heavy-barrel","Long-barrel","Slim-barrel","Curved-stock","Muzzle-brake","Twin-barrel","Braced-stock","Hook-stock","Ribbed-barrel","Ring-sight","Crested"}};
 WeaponDesign d=weapon_design(item);if(weapon_rapier(item)&&d.pattern==8)return "Swept-hilt";if(weapon_rapier(item)&&d.pattern==9)return "Fluted";return names[d.group][d.pattern];
}
static inline float weapon_design_anchor(int item){return weapon_ranged(item)?14:5;}
static inline float weapon_design_along(float a,float anchor,WeaponDesign d){return a>anchor?anchor+(a-anchor)*d.length:a<-anchor?-anchor+(a+anchor)*d.length:a;}
static inline float weapon_design_unalong(float a,float anchor,WeaponDesign d){return a>anchor?anchor+(a-anchor)/d.length:a<-anchor?-anchor+(a+anchor)/d.length:a;}
static inline float weapon_design_blend(float a) {
    float distance = fabsf(a);
    if (distance >= 13) return 1;
    if (!(distance > 3)) return 0;
    return (distance - 3) / 10;
}
#endif
