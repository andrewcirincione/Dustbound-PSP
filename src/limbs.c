#include "game.h"
#include "combat_pose.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor *a);
const LimbGrade limbGrades[LIMB_GRADES]={
 {"Scrap",70,250,-2,-2,0,-8,0x8a7965},
 {"Standard",100,700,0,0,0,0,0x9fa8a1},
 {"Reinforced",190,1450,10,-4,12,-6,0x8f9da4},
 {"Precision",110,2800,-2,10,-3,22,0xa5b8b2},
 {"Relic",180,5200,6,6,6,15,0xd5bb83}
};
/* Preserve Actor/save/reserve layouts. Low bits retain the original limb mask.
   Four 3-bit grade codes occupy bits 8..19 (0 = legacy Standard);
   four 3-bit ownership tags occupy bits 20..31. Copies keep their fitted parts. */
static int valid_limb(int l){return l>=LARM&&l<=RLEG;}
int limb_item(int grade,int leg){if(grade<0||grade>=LIMB_GRADES)return 0;if(grade==1)return leg?LEG:ARM;return LIMB_ITEMS+(grade>1?grade-1:grade)*2+!!leg;}
int limb_item_grade(int it){if(it==ARM||it==LEG)return 1;if(it<LIMB_ITEMS||it>=LIMB_ITEMS+8)return -1;int n=(it-LIMB_ITEMS)/2;return n?n+1:0;}
int limb_item_leg(int it){return it==LEG||(it>=LIMB_ITEMS&&it<LIMB_ITEMS+8&&((it-LIMB_ITEMS)&1));}
int limb_missing(const Actor*a,int l){return valid_limb(l)&&(a->lostMask&(1<<l))&&!(a->prostheticMask&(1<<l));}
int limb_installed(const Actor*a,int l){if(!valid_limb(l)||!(a->prostheticMask&(1<<l)))return 0;unsigned c=((uint32_t)a->prostheticMask>>(8+3*(l-LARM)))&7;return limb_item(c?(int)c-1:1,l>=LLEG);}
int limb_owner(const Actor*a,int l){return valid_limb(l)?((uint32_t)a->prostheticMask>>(20+3*(l-LARM)))&7:0;}
void limb_set(Actor*a,int l,int it,int owner){
 if(!valid_limb(l))return;int shift=8+3*(l-LARM),tag=20+3*(l-LARM),grade=limb_item_grade(it);
 uint32_t bits=(uint32_t)a->prostheticMask;bits&=~((1u<<l)|(7u<<shift)|(7u<<tag));
 if(grade>=0){bits|=(1u<<l)|((unsigned)(grade==1?0:grade+1)<<shift)|((unsigned)(owner&7)<<tag);a->lostMask|=1<<l;}
 a->prostheticMask=(int)bits;
}
float limb_max_health(const Actor*a,int l){int grade=limb_item_grade(limb_installed(a,l));return grade<0?species_health(a):limbGrades[grade].hp;}
int limb_arms(const Actor*a){return !limb_missing(a,LARM)+!limb_missing(a,RARM);}
int limb_crawling(const Actor*a){return a->kind!=BEAST&&(limb_missing(a,LLEG)||limb_missing(a,RLEG));}
int limb_bonus(const Actor*a,int skill){int n=0;if(skill!=STRENGTH&&skill!=DEXTERITY)return 0;for(int l=LARM;l<=RARM;l++){int grade=limb_item_grade(limb_installed(a,l));if(grade>=0&&a->health[l]>0)n+=skill==STRENGTH?limbGrades[grade].strength:limbGrades[grade].dexterity;}return n;}
int effective_level(const Actor*a,int skill){return (int)fmaxf(1,level(a,skill)+limb_bonus(a,skill));}
int limb_capacity_bonus(const Actor*a){int n=0;for(int l=LLEG;l<=RLEG;l++){int grade=limb_item_grade(limb_installed(a,l));if(grade>=0&&a->health[l]>0)n+=limbGrades[grade].carry;}return n;}
float limb_move_bonus(const Actor*a){int n=0;for(int l=LLEG;l<=RLEG;l++){int grade=limb_item_grade(limb_installed(a,l));if(grade>=0&&a->health[l]>0)n+=limbGrades[grade].speed;}return 1+n*.01f;}
float limb_arm_condition(const Actor*a,int w){float c[2];for(int i=0;i<2;i++)c[i]=limb_missing(a,LARM+i)?0:fmaxf(0,fminf(1,a->health[LARM+i]/limb_max_health(a,LARM+i)));return combat_two_hands(w)&&limb_arms(a)==2?fminf(c[0],c[1]):fmaxf(c[0],c[1]);}
int limb_attack_penalty(const Actor*a,int w){int arms=limb_arms(a);return arms==0?35:arms==1&&combat_two_hands(w)?25:0;}
float limb_damage_factor(const Actor*a,int w){int arms=limb_arms(a);return arms==0?.35f:arms==1&&combat_two_hands(w)?.60f:1;}
static char names[10][40],descriptions[10][180];
void limbs_content(void){
 for(int grade=0;grade<LIMB_GRADES;grade++)for(int leg=0;leg<2;leg++){
  int it=limb_item(grade,leg),n=grade*2+leg;const LimbGrade*d=&limbGrades[grade];
  snprintf(names[n],40,"%s mechanical %s",d->name,leg?"leg":"arm");
  snprintf(descriptions[n],180,leg?"%d HP; movement %+d%%; load %+d. Fit from Limbs: 2 machine parts. Repairs use parts.":"%d HP; strength %+d; dexterity %+d. Fit from Limbs: 2 machine parts. Repairs use parts.",d->hp,leg?d->speed:d->strength,leg?d->carry:d->dexterity);
  itemDefs[it]=(ItemDef){.name=names[n],.desc=descriptions[n],.price=d->price,.weight=grade==2?5:grade==3?1:grade==4?3:2,.slot=-1,.color=d->color,.family=-1};
 }
 itemDefs[SEVERED_ARM]=(ItemDef){.name="Severed arm",.desc="An organic arm lost to injury. Cannot be reattached; fit a mechanical replacement.",.weight=2,.slot=-1,.color=0x98594b,.family=-1};
 itemDefs[SEVERED_LEG]=(ItemDef){.name="Severed leg",.desc="An organic leg lost to injury. Cannot be reattached; fit a mechanical replacement.",.weight=3,.slot=-1,.color=0x98594b,.family=-1};
}
void limbs_stock(int t){
 for(int grade=0;grade<LIMB_GRADES;grade++)for(int leg=0;leg<2;leg++)g.stock[t][limb_item(grade,leg)]=0;
 int high=t==8?4:t==6||t==11?3:t==1||t==5?2:t==0?1:-1;
 if(high<0)return;for(int grade=0;grade<=high;grade++)for(int leg=0;leg<2;leg++)g.stock[t][limb_item(grade,leg)]=grade>=3?1:2;
 if(g.stock[t][PARTS]<8)g.stock[t][PARTS]=8;
}
int limb_surgeon(int n){return n>=0&&n<g.actorCount&&g.limbs.surgeonActor==n+1;}
void limbs_init(void){
 if(g.limbs.version)return;g.limbs.version=1;
 for(int n=0;n<g.actorCount;n++)if(npc_role(&g.actors[n])==ROLE_MEDIC&&g.actors[n].homeTown==0){g.limbs.surgeonActor=n+1;snprintf(g.actors[n].name,16,"Surgeon Venn");break;}
 /* Persistent wandering owners carry real replacements, not generated loot. */
 for(int e=12;e<ELITES;e++){int n=g.civil.eliteActor[e]-1;if(n<0||g.actors[n].dead||g.actors[n].squad)continue;Actor*a=&g.actors[n];int l=e%2?LLEG:RARM,grade=e==14?4:e==15?2:3;if(!(a->lostMask&(1<<l))){limb_set(a,l,limb_item(grade,l>=LLEG),0);a->health[l]=limb_max_health(a,l);}}
 for(int t=0;t<TOWNS;t++)limbs_stock(t);
}
int limb_actor_valid(const Actor*a){
 if(a->lostMask&~60)return 0;if(((uint32_t)a->prostheticMask&255u)&~60u)return 0;
 for(int l=LARM;l<=RLEG;l++){unsigned grade=((uint32_t)a->prostheticMask>>(8+3*(l-LARM)))&7,owner=limb_owner(a,l);int fitted=a->prostheticMask&(1<<l);
  if(fitted&&(!(a->lostMask&(1<<l))||grade>5))return 0;if(!fitted&&(grade||owner))return 0;
 }return 1;
}
int limbs_validate(const Game*s){
 if(s->limbs.version<0||s->limbs.version>1||s->limbs.surgeonActor<0||s->limbs.surgeonActor>s->actorCount)return 0;
 for(int n=0;n<s->actorCount;n++)if(!limb_actor_valid(&s->actors[n]))return 0;
 for(int e=0;e<EVENTS;e++)if(s->living.events[e].stored)for(int n=0;n<s->living.events[e].count;n++)if(!limb_actor_valid(&s->living.events[e].reserve[n]))return 0;
 return 1;
}
void limb_sever(Actor*a,int l,int surgery){
 if(!valid_limb(l)||limb_missing(a,l)||a->kind==BEAST)return;
 int organic=depth_organic(a,l),n=(int)(a-g.actors);limb_set(a,l,0,0);a->lostMask|=1<<l;a->health[l]=0;
 a->woundFlags &= ~((1u << l) | (1u << (WOUND_REPAIR_SHIFT + l))); a->treatedMask &= ~(1 << l);
 if(n>=0&&n<g.actorCount)g.depth.actors[n].trauma[l]=0;
 a->attackPhase=0;a->taskKind=0;if(organic&&!surgery)a->bleed+=2;
 if(!surgery&&g.nodeCount<MAX_NODES){Node*d=&g.nodes[g.nodeCount++];memset(d,0,sizeof(*d));d->x=a->x+.3f;d->y=a->y+.2f;d->type=NODE_LOOT;d->remaining=1;d->item=organic?(l>=LLEG?SEVERED_LEG:SEVERED_ARM):PARTS;d->qty=1;}
 if(a->squad&&!surgery)log_warn(organic?"%s lost %s. Bandage the wound!":"%s: %s replacement destroyed.",a->name,limbNames[l]);
}
void limb_hit(Actor*a,int l,float cut){if(!valid_limb(l)||limb_missing(a,l)||a->kind==BEAST)return;float threshold=-limb_max_health(a,l)*.55f;if((!depth_organic(a,l)||cut>=.35f)&&a->health[l]<threshold)limb_sever(a,l,0);}
static int can_fit(int n){Actor*a=&g.actors[n];return conscious(a)&&!a->attackPhase&&a->target<0&&!g.depth.actors[n].captive&&a->prisonTime<=0;}
int limb_fit(int n,int l,int slot){
 if(n<0||n>=g.actorCount||!valid_limb(l)||slot<0||slot>=PACK)return 0;Actor*a=&g.actors[n];int it=a->pack[slot].item,old=limb_installed(a,l),grade=limb_item_grade(it),owner=a->stolen[slot],oldOwner=limb_owner(a,l);
 if(grade<0||limb_item_leg(it)!=(l>=LLEG)){log_msg("Choose a replacement for that limb.");return 0;}
 if(!(a->lostMask&(1<<l))){log_msg("Natural limb intact. Venn in Dusk Haven offers surgery.");return 0;}
 if(!can_fit(n)){log_msg("Fit limbs while awake, free and out of combat.");return 0;}
 if(inventory_count(a,PARTS)<2){log_msg("Fitting requires 2 machine parts.");return 0;}
 Actor backup=*a;inventory_remove_slot(a,slot,1);inventory_remove(a,PARTS,2);
 if(old&&!inventory_add_owned(a,old,1,oldOwner)){*a=backup;log_msg("No pack room for the old replacement.");return 0;}
 limb_set(a,l,it,owner);a->health[l]=limb_max_health(a,l);g.depth.actors[n].trauma[l]=0;gain(a,ENGINEERING,20);log_msg("Fitted %s to %s. Used 2 parts.",itemDefs[it].name,limbNames[l]);return 1;
}
int limb_detach(int n,int l){if(n<0||n>=g.actorCount||!valid_limb(l))return 0;Actor*a=&g.actors[n];int it=limb_installed(a,l);if(!it||!can_fit(n)){log_msg("Detach a fitted replacement while out of combat.");return 0;}if(!inventory_add_owned(a,it,1,limb_owner(a,l))){log_msg("Make pack room before detaching.");return 0;}limb_set(a,l,0,0);a->health[l]=0;g.depth.actors[n].trauma[l]=0;log_msg("Detached %s. Fitting again costs 2 parts.",limbNames[l]);return 1;}
int limb_surgery(int sn,int pn,int l){
 if(!limb_surgeon(sn)||pn<0||pn>=g.actorCount||!valid_limb(l))return 0;Actor*s=&g.actors[sn],*a=&g.actors[pn];
 if(!can_fit(pn)||!conscious(s)||s->target>=0||depth_distance(s,a)>TALK_REACH||s->faction==HOSTILE||g.rep[s->faction]<-25){log_msg("Surgery needs Venn nearby, awake and safe.");return 0;}
 if(a->lostMask&(1<<l)){log_msg("That natural limb is already gone.");return 0;}
 if(g.money<400){log_msg("Removal surgery costs 400 silver.");return 0;}
 g.money-=400;limb_sever(a,l,1);dress_wounds(a);log_msg("Venn removed %s and closed the wound. Fit a replacement.",limbNames[l]);return 1;
}
int limb_loot_item(const Actor*a,int slot){return slot<0?0:slot<PACK?a->pack[slot].item:slot<PACK+GEARS?a->gear[slot-PACK]:slot<PACK+GEARS+4?limb_installed(a,LARM+slot-PACK-GEARS):0;}
void limb_open(int screen){ui.screen=screen;ui.selection=0;ui.scroll=0;}
int limb_fit_slot(int row){Actor*a=&g.actors[g.selected];for(int i=0;i<PACK;i++)if(limb_item_grade(a->pack[i].item)>=0&&limb_item_leg(a->pack[i].item)==(ui.limbTarget>=LLEG)&&row--==0)return i;return -1;}
int limb_fit_count(void){int n=0;while(limb_fit_slot(n)>=0)n++;return n;}
int limb_input(Input in){
 int p=in.pressed;Actor*a=&g.actors[g.selected];
 if(ui.screen==SCREEN_HEALTH&&(p&KEY_R)){ui.limbReturn=SCREEN_HEALTH;limb_open(SCREEN_LIMBS);return 1;}
 if(ui.screen<SCREEN_LIMBS||ui.screen>SCREEN_LIMBCONFIRM)return 0;
 if(p&KEY_SELECT){open_help(4);return 1;}
 if(p&(KEY_CIRCLE|KEY_START)){
  if(p&KEY_START){ui.menuReturn=0;limb_open(SCREEN_WORLD);return 1;}
  if(ui.screen==SCREEN_LIMBFIT)limb_open(SCREEN_LIMBS);else if(ui.screen==SCREEN_LIMBCONFIRM)limb_open(ui.limbConfirm==1?SCREEN_SURGERY:SCREEN_LIMBS);else limb_open(ui.limbReturn);return 1;
 }
 if(ui.screen==SCREEN_LIMBCONFIRM){if(p&KEY_CROSS){if(ui.limbConfirm==1)limb_surgery(g.limbs.surgeonActor-1,g.selected,ui.limbTarget);else limb_detach(g.selected,ui.limbTarget);limb_open(ui.limbConfirm==1?SCREEN_SURGERY:SCREEN_LIMBS);}return 1;}
 int count=ui.screen==SCREEN_LIMBFIT?limb_fit_count():4;
 if(count){if(p&KEY_UP)ui.selection=(ui.selection+count-1)%count;if(p&KEY_DOWN)ui.selection=(ui.selection+1)%count;}
 if(ui.screen==SCREEN_LIMBFIT){if((p&KEY_CROSS)&&count&&limb_fit(g.selected,ui.limbTarget,limb_fit_slot(ui.selection)))limb_open(SCREEN_LIMBS);return 1;}
 ui.limbTarget=LARM+ui.selection;
 if(ui.screen==SCREEN_SURGERY){if(p&KEY_CROSS){ui.limbConfirm=1;limb_open(SCREEN_LIMBCONFIRM);}if(p&KEY_SQUARE){ui.limbReturn=SCREEN_DIALOG;ui.tradeTown=0;ui.shopFilter=5;ui.sell=0;limb_open(SCREEN_TRADE);}return 1;}
 if(p&KEY_CROSS)limb_open(SCREEN_LIMBFIT);
 if(p&KEY_SQUARE){if(limb_installed(a,ui.limbTarget)){ui.limbConfirm=2;limb_open(SCREEN_LIMBCONFIRM);}else log_msg("Only mechanical replacements can be detached here.");}
 if(p&KEY_TRIANGLE)depth_repair(g.selected,g.selected);
 return 1;
}
