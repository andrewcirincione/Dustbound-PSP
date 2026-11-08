#ifndef DUSTBOUND_LIFE_H
#define DUSTBOUND_LIFE_H
void life_init(void);
int life_validate(const Game *state);
const char *life_goal_name(int goal);
int life_group_goal(int group);
int population_hidden(int actor);
int population_tactics(int actor,float dt);
int population_prop_art(int group,int object);
int population_prop_style(int group,int object);
void politics_tick(float dt);
void politics_begin(int elite);
void politics_occupy(int town,int faction);
int politics_action(int town,int action);
float politics_price(int town,int item);
int politics_input(Input input);
void politics_open(int town);
int life_ledger_count(void);
int life_ledger_item(int row);
void life_ledger_label(int row,char *out,int size);
void life_ledger_detail(int row,char *out,int size);
#endif
