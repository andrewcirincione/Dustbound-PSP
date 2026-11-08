#ifndef DUSTBOUND_LIMBS_H
#define DUSTBOUND_LIMBS_H
#define LIMB_GRADES 5
typedef struct { const char *name; int hp,price,strength,dexterity,carry,speed; uint32_t color; } LimbGrade;
extern const LimbGrade limbGrades[LIMB_GRADES];
int limb_item(int grade,int leg);
int limb_item_grade(int item);
int limb_item_leg(int item);
int limb_missing(const Actor *a,int limb);
int limb_installed(const Actor *a,int limb);
int limb_owner(const Actor *a,int limb);
void limb_set(Actor *a,int limb,int item,int owner);
float limb_max_health(const Actor *a,int limb);
int limb_arms(const Actor *a);
int limb_crawling(const Actor *a);
int limb_bonus(const Actor *a,int skill);
int limb_capacity_bonus(const Actor *a);
float limb_move_bonus(const Actor *a);
int effective_level(const Actor *a,int skill);
float limb_arm_condition(const Actor *a,int weapon);
float limb_damage_factor(const Actor *a,int weapon);
int limb_attack_penalty(const Actor *a,int weapon);
void limbs_content(void);
void limbs_init(void);
void limbs_stock(int town);
int limbs_validate(const Game *state);
void limb_sever(Actor *a,int limb,int surgery);
void limb_hit(Actor *a,int limb,float cut);
int limb_fit(int actor,int limb,int packSlot);
int limb_detach(int actor,int limb);
int limb_surgeon(int actor);
int limb_surgery(int surgeon,int patient,int limb);
int limb_loot_item(const Actor *a,int slot);
int limb_input(Input in);
void limb_open(int screen);
int limb_fit_slot(int row);
int limb_fit_count(void);
#endif
