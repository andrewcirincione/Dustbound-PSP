#ifndef DUSTBOUND_POPULATION_H
#define DUSTBOUND_POPULATION_H
void population_init(void);
void population_tick(float dt);
int population_owned(int actor);
int population_walk(int actor,float dt);
int population_hostile(const Actor *a,const Actor *b);
void population_anger(int actor);
int population_validate(const Game *state);
void population_roll_limbs(Actor *actor,unsigned seed,int tier);
void population_generate(int group,int type);
int population_materialize(int group);
int population_store(int group);
int population_object(int group,int object,float *x,float *y);
int population_object_count(int group);
int population_action_valid(int group,int action);
int population_action(int group,int action);
void population_action_label(int group,int action,char *out,int size);
void population_detail(int group,char *out,int size);
const char *population_name(int group);
const char *population_actor_role(int actor);
const char *population_dialogue(int actor);
int population_sleep_valid(const Actor *actor);
#endif
