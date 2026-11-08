#ifndef DUSTBOUND_CIVILIZATION_H
#define DUSTBOUND_CIVILIZATION_H
typedef struct { const char *name,*trait; int health,carry; float move,food; int bonus[SKILLS]; } SpeciesDef;
typedef struct {const char *name,*title,*weapon,*armor,*story;int town,race,skill,weaponFamily,armorFamily,faction;} EliteDef;
extern const SpeciesDef speciesDefs[SPECIES];
extern const EliteDef eliteDefs[ELITES];
int species_index(const Actor*a);
int species_start_level(int origin,int race,int skill);
void species_start(Actor*a,int origin,int race);
float species_health(const Actor*a);
int actor_capacity(const Actor*a);
float species_food(const Actor*a);
void civil_content(void);
void civil_init(void);
void civil_tick(float dt);
void civil_stock(int town);
int civil_validate(const Game*s);
int civil_elite_index(int actor);
int civil_npc_job(int actor,float dt);
int civil_style(int town);
const char *civil_style_name(int town);
int civil_building_sprite(int building,int fallback);
int regalia_owner(int item);
#endif
