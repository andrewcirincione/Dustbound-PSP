#ifndef DUSTBOUND_GAME_H
#define DUSTBOUND_GAME_H
#include <stdint.h>
#define VERSION "0.9.0"
#define WORLD 1536
#define FOG_SIZE 384
#define WORK_REACH 1.15f
#define TALK_REACH 1.6f
#define MAX_ACTORS 192
#define MAX_BUILDINGS 320
#define MAX_NODES 640
#define PACK 24
#define SKILLS 20
#define LIMBS 6
#define BASE_ITEMS 36
#define FAMILIES 88
#define WEAPON_FAMILIES 36
#define MATERIALS 6
#define QUALITIES 4
#define CATALOG_ITEMS (BASE_ITEMS + FAMILIES * MATERIALS * QUALITIES)
#define ITEMS (CATALOG_ITEMS + 88)
#define EVENTS 8
#define GEARS 6
#define FACTIONS 8
#define CONTRACTS 24
#define GUIDE_PAGES 25
#define SAVE_SLOTS 7
#define TOWNS 16
#define SQUAD 8
#define LOGS 6
enum {
    ATHLETICS,
    STRENGTH,
    DEXTERITY,
    MELEE,
    DEFENCE,
    TOUGHNESS,
    DODGE,
    MARTIAL,
    STEALTH,
    THEFT,
    LOCKPICK,
    MEDICINE,
    LABOUR,
    FARMING,
    COOKING,
    SMITHING,
    ENGINEERING,
    SCIENCE,
    CROSSBOWS,
    PERCEPTION
};
enum { HEAD, CHEST, LARM, RARM, LLEG, RLEG };
enum {
    NONE,
    RATION,
    MEDKIT,
    ORE,
    COPPER,
    WOOD,
    CLOTH,
    WATER,
    GRAIN,
    MEAT,
    IRONBAR,
    RUSTSWORD,
    SABRE,
    CLEAVER,
    STAFF,
    CROSSBOW,
    BOLTS,
    RAGS,
    LEATHER,
    PLATE,
    DUSTER,
    STRAW,
    HAT,
    HELM,
    BOOTS,
    SANDALS,
    ARM,
    LEG,
    TOOLS,
    RELIC,
    CARTRIDGE,
    HIDE,
    COAL,
    PARTS,
    TONIC,
    MAPSCROLL
};
enum { PLAYER, SETTLER, GUARD, TRADER, RECRUIT, RAIDER, BEAST, CARAVAN };
enum { FREE, UNION, SCAVENGERS, HOSTILE, SHOGUN, MONKS, OUTLAWS, WILDLIFE };
enum { HOUSE, SHOP, BAR, TOWER, WINDMILL, WELL, CAMPFIRE, BED, FARM, FORGE, BENCH, WALL, STORE };
enum { NODE_ORE, NODE_COPPER, NODE_WOOD, NODE_LOOT, NODE_LOCKED, NODE_RUIN };
enum { JOB_FOLLOW, JOB_HOLD, JOB_MINE, JOB_FARM, JOB_MEDIC, JOB_SMELT, JOB_COOK, JOB_HAUL };
enum {
    SCREEN_WORLD,
    SCREEN_PACK,
    SCREEN_SKILLS,
    SCREEN_HEALTH,
    SCREEN_SQUAD,
    SCREEN_MAP,
    SCREEN_TRADE,
    SCREEN_RECRUIT,
    SCREEN_BUILD,
    SCREEN_CRAFT,
    SCREEN_RESEARCH,
    SCREEN_MENU,
    SCREEN_HELP,
    SCREEN_TITLE,
    SCREEN_CONFIRM,
    SCREEN_CREATE,
    SCREEN_NAME,
    SCREEN_DIALOG,
    SCREEN_FORGE,
    SCREEN_JOURNAL,
    SCREEN_FACTIONS,
    SCREEN_STORAGE,
    SCREEN_SAVES,
    SCREEN_EQUIPPED,
    SCREEN_LOOT,
    SCREEN_ASSIST,
    SCREEN_LOG, SCREEN_EXPEDITION, SCREEN_ORDERS, SCREEN_CAMP, SCREEN_CARE, SCREEN_ENCOUNTER, SCREEN_LAYERS,
    SCREEN_THEFT, SCREEN_ENGAGE, SCREEN_ACTIONS
};
enum {
    KEY_UP = 1,
    KEY_DOWN = 2,
    KEY_LEFT = 4,
    KEY_RIGHT = 8,
    KEY_CROSS = 16,
    KEY_CIRCLE = 32,
    KEY_SQUARE = 64,
    KEY_TRIANGLE = 128,
    KEY_L = 256,
    KEY_R = 512,
    KEY_START = 1024,
    KEY_SELECT = 2048
};
typedef struct {
    int item, qty;
} Stack;
typedef struct {
    const char *name, *desc;
    int price, weight, slot, power, armor;
    uint32_t color;
    int family, material, quality, effect, potency, penetration, accuracy, speed, stealth, capacity,
        protection;
    float range, cooldown;
    uint32_t trim;
} ItemDef;
typedef struct {
    char name[16];
    float x, y, homeX, homeY, tx, ty, health[LIMBS], blood, hunger, fatigue, bleed, cooldown, action, anim,
        unconscious;
    float xp[SKILLS];
    Stack pack[PACK];
    int gear[GEARS];
    int active, kind, faction, squad, dead, job, target, taskKind, taskIndex, sneaking, autoattack, carrying,
        carriedBy, lostMask, prostheticMask, bounty, arrested, lootTaken;
    int body, skin, hair, hairColor, beard, scar, origin, race, facing, homeTown;
    int voice, speech, prisonTown, stolen[PACK], bountyReward;
    float speechTime, speechCooldown, prisonTime, stun, exposure, bleedTimer;
    int routeLen, routePos, routeX[24], routeY[24];
    float routeClock, routeTargetX, routeTargetY;
    int attackPhase, attackTarget, attackWeapon, impactLimb, impactDamage, impactKind, defeatCredited,
        gearStolen[GEARS];
    float attackClock, attackDuration, attackAngle, hitFlash, recoilX, recoilY, movedTimer;

} Actor;
typedef struct {
    float x, y;
    int type, faction, owner, level, stock;
    float progress;
    Stack storage[16];
} Building;
typedef struct {
    float x, y;
    int type, remaining, locked, owner, item, qty;
    float regen;
} Node;
typedef struct {
    const char *name;
    float x, y;
    int faction;
} Town;
typedef struct {
    int active, type, town, target, item, needed, progress, reward, completed, baseline;
} Contract;
typedef struct {
    int exists, valid, version, day, squad, money;
    char name[16];
} SaveInfo;
#include "depth_types.h"
typedef struct { int version,outcome[3],toll[3]; float campX,campY; } RegionalState;
typedef struct {
    unsigned seed; int serial,type,modifier,faction,town,origin,tier,count,status,spawned,participated;
    float x,y,age,presence; int members[16],stored,reserveAnger[16];
    Actor reserve[16]; DepthActor reserveDepth[16];
} WorldEvent;
typedef struct {
    int version,serial,history[16],historyPos,owned[MAX_ACTORS],anger[MAX_ACTORS],command[MAX_ACTORS];
    float pulse; WorldEvent events[EVENTS];
} LivingWorld;
#include "society_types.h"
typedef struct {
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[FACTIONS], kills, discovered[TOWNS], stock[TOWNS][ITEMS],
        lastRestock;
    int actorCount, buildingCount, nodeCount, baseExists;
    float baseX, baseY;
    uint8_t seen[FOG_SIZE * FOG_SIZE / 8];
    Actor actors[MAX_ACTORS];
    Building buildings[MAX_BUILDINGS];
    Node nodes[MAX_NODES];
    Contract contracts[CONTRACTS];
    int townOwner[TOWNS], prosperity[TOWNS], townState[TOWNS], weather, season, contractsDone;
    int factionWar[FACTIONS], storyStage, playOrigin, unlocks, difficulty;
    float eventTimer, weatherTimer, worldPulse, hitstop, shake;
    char worldNews[96];
    /* Version 3 extension. Version 2 ends immediately before these fields. */
    int interiorBuilding, interiorActor;
    float interiorX, interiorY;
    /* v5: autonomous hired guards; actor IDs use +1, zero means vacant. */
    int mercActor[4], mercMode[4], identityVersion;
    float mercUntil[4];
    /* v6 append-only extension. v5 prefix is exactly 410656 bytes. */
    FrontierDepth depth;
    /* v7 append-only regional consequences. */
    RegionalState regions;
    LivingWorld living;
    Society society;
} Game;
enum { ROOM_NONE, ROOM_TRADE, ROOM_REST, ROOM_FORGE, ROOM_RESEARCH, ROOM_STORAGE };
typedef struct {
    float x, y, w, d;
    int art, action;
} InteriorProp;
int building_has_interior(int type);
int interior_enter(int building);
void interior_leave(void);
int actor_inside(const Actor *a);
int interior_props(int building, InteriorProp *props);
int interior_context(void);
const char *interior_prompt(void);
void interior_interact(void);
void interior_move(float dx, float dy, float dt);
typedef struct {
    int screen, selection, scroll, tab, paused, speed, contextKind, contextIndex, tradeTown, sell, slot,
        confirmAction, titleChoice, waypoint;
    float mapX, mapY, camX, camY, noticeTime, repeat, travelX, travelY;
    char logs[LOGS][80], notice[100];
    int logCount;
    char feed[12][100];
    unsigned char feedPriority[12];
    int feedCount, wantedTown, wantedValue;
    float feedClock, statusClock;
    int helpReturn, helpTopic, dialogActor, dialogPage, forgeSlot, forgeFamily, forgeMaterial, forgeQuality;
    int createRow, createOrigin, createBody, createSkin, createHair, createHairColor, createBeard, createScar,
        createRace;
    int nameCursor, nameCell, storageIndex, storageSide, shopFilter, saveMode, lootActor, mapZoom, mapTown;
    char createName[16];
    SaveInfo saves[SAVE_SLOTS];
    int helpSelection, helpScroll, saveReturn, saveExit, contextChoice, contextCount, squadCyclePending;
    int depthSite, depthRow;
    int menuTab, menuRow, menuReturn, statusPanel;
    int journalTab,regionChoice,attackFocus;
    int mapCategory,mapChoice,skillPage;
    float travelStall, travelLastX, travelLastY;
} UI;
typedef struct {
    int held, pressed;
    float ax, ay;
} Input;
extern Game g;
extern UI ui;
extern volatile uint32_t display_buffer;
extern volatile uint32_t rendered_frames;
extern ItemDef itemDefs[ITEMS];
extern Town towns[TOWNS];
void world_layout(int expanded);
int world_is_expanded(void);
enum {
    ROLE_CITIZEN,
    ROLE_MERCHANT,
    ROLE_RECRUIT,
    ROLE_MARSHAL,
    ROLE_INNKEEPER,
    ROLE_MEDIC,
    ROLE_SMITH,
    ROLE_BARBER,
    ROLE_SCOUT,
    ROLE_MERCENARY
};
int npc_role(const Actor *a);
int weapon_family_index(int i);
int weapon_strength(int item);
float weapon_handling(const Actor *a, int item);
void frontier_identity(void);
void living_init(void);
void living_tick(float dt);
int living_validate(const Game *s);
void living_generate(int slot);
int living_owned(int n);
int living_hostile(const Actor *a,const Actor *b);
int living_walk(int n,float dt);
void living_loot(int n,int role,int tier);
void living_title(int slot,char *out,int size);
void living_detail(int slot,char *out,int size);
void living_action(int slot);
int combat_candidate(int row);
int combat_candidates(void);
void command_attack(int n);
int actor_relation(int n);
void frontier_restock(int town);
extern const char *regional_names[3];
int regional_town(int region);
int regional_at(float x,float y);
void regional_init(void);
int regional_validate(const Game *state);
int regional_named(int actor);
int regional_hostile(const Actor *a,const Actor *b);
int regional_toll_left(void);
float regional_price(int town,int item);
void regional_restock(int town);
int regional_bed_cost(int town);
int regional_well(Actor *actor,Building *well);
const char *regional_story(int region);
const char *regional_choice(int region,int choice);
void regional_requirements(int region,int choice,char *out,int size);
const char *regional_result(int region,int choice);
int regional_resolve(int region,int choice);
void regional_tick(void);
void frontier_tick(float dt);
int merc_slot(const Actor *a);
int merc_hire(int actor, int mode);
void merc_dismiss(int actor);
const char *faction_identity(int faction);
const char *npc_rumor(const Actor *a);
const char *npc_role_name(const Actor *a);
int dialogue_count(int actor);
int dialogue_action(int actor, int row);
const char *dialogue_option(int actor, int row);
float context_distance(void);
float context_reach(void);
const char *context_name(void);
const char *context_action(void);
void spread_frontier(Game *game);
void place_wilderness_encounters(void);
extern const char *skillNames[SKILLS], *limbNames[LIMBS], *buildNames[], *jobNames[];
int level(const Actor *a, int skill);
void gain(Actor *a, int skill, float xp);
int inventory_count(const Actor *a, int item);
int inventory_add(Actor *a, int item, int qty);
int inventory_remove(Actor *a, int item, int qty);
int inventory_add_owned(Actor *a, int item, int qty, int owner);
int inventory_remove_slot(Actor *a, int slot, int qty);
int inventory_input_owner(const Actor *a, int item, int qty);
int inventory_transfer(Actor *from, Actor *to, int slot, int qty);
int loot_take(int slot, int all);
int nearby_service(int role);
const char *job_status(const Actor *a);
const char *skill_training(int skill);
void recipe_status(int recipe, char *out, int size);
int nearby_station(int type, float radius);
int build_wood_cost(int choice);
int research_cost(void);
int harvest_yield(const Actor *actor);
int needs_aid(const Actor *actor);
float encumbrance(const Actor *a);
float backpack_load_preview(const Actor *a, int item);
int backpack_load_percent(int item);
int backpack_combat_penalty(int item);
int item_effect_value(int item);
float movement_speed(const Actor *a);
const char *travel_hold_reason(const Actor *a);
float distance(float x, float y, float u, float v);
int can_walk(float x, float y);
int biome(float x, float y);
int town_near(float x, float y, float radius);
void game_new(uint32_t seed, int scenario);
void game_tick(float dt, Input in);
void simulate(float dt);
void move_actor(Actor *a, float dx, float dy, float dt);
void strike(int attacker, int defender);
void use_item(int slot);
int buy_item(int town, int item);
int sell_item(int town, int slot);
int recruit_actor(int index);
int build_structure(int choice);
int craft_item(int recipe);
int research_tech(void);
int heal_actor(int medic, int patient);
void interact(void);
void find_context(void);
void log_msg(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_tick(float dt);
const char *log_current(int *priority, int *offset);
int save_game(const char *path);
int load_game(const char *path);
int save_slot(int slot);
int load_slot(int slot);
void set_save_root(const char *path);
void render_game(uint32_t *pixels, int stride);
void render_title_art(uint32_t *pixels, int stride);
void debug_snapshot(const char *path);
extern const char *recipeNames[];
extern const int recipeCount, buildCount;

extern const char *factionNames[FACTIONS], *regionNames[9], *weatherNames[5];
extern const char *originNames[5], *raceNames[3], *effectNames[12], *materialNames[MATERIALS],
    *qualityNames[QUALITIES];
extern const char *familyNames[FAMILIES], *guideTitles[GUIDE_PAGES], *guideBodies[GUIDE_PAGES];
extern const uint32_t factionColors[FACTIONS];
void audio_event(int kind);
void audio_init(void);
void audio_shutdown(void);
void content_init(void);
int gear_id(int family, int material, int quality);
int gear_family(int item);
int weapon_ranged(int item);
int weapon_rapier(int item);
int gear_bonus(const Actor *a, int stat);
int item_price(int town, int item, int selling);
int shop_count(void);
int shop_item(int index);
void init_expansion(void);
void tick_expansion(float dt);
int expansion_input(float dt, Input in);
void dialogue_open(int actor);
const char *dialogue_line(int actor, int page);
const char *ambient_line(int actor);
int forge_equipment(int item);
int storage_transfer(int building, int fromStorage, int slot);
int accept_contract(int town, int kind);
int complete_contract(int index);
void contract_description(int index, char *dst, int size);
int pickpocket(int actor);
int pickpocket_slot(int actor, int slot);
float theft_chance(int actor, int slot);
int theft_witnesses(int actor);
int theft_behind(int actor);
int item_view_enabled(void);
int item_view_count(void);
int item_view_slot(int row);
int item_view_row(int slot);
void item_view_normalize(void);
void item_view_move(int direction);
void cycle_squad(int direction);
void skill_benefit(int skill, char *out, int size);
float pickpocket_chance(const Actor *actor);
int pickpocket_item(int actor);
int pause_menu_count(void);
const char *pause_menu_label(int row);
void character_begin(int origin);
void character_finish(void);
int save_inspect(int slot, SaveInfo *info);
void save_refresh(void);
int terrain_height(float x, float y);
int terrain_water(float x, float y);
int terrain_road(float x, float y);
void route_toward(Actor *a, float x, float y, float dt);
int nearby_workstation(int type);
int start_sleep(int town);
int doctor_heal(int town);
void open_help(int topic);
int atlas_item(int row);
int atlas_count(void);
void atlas_select(int row);
void atlas_sync(void);
void render_catalog_scale(uint32_t *pixels, int stride, int item, int sc);
void render_catalog_sample(uint32_t *pixels, int stride, int item);
#include "depth.h"
#include "society.h"
#endif
