#ifndef DUSTBOUND_GAME_H
#define DUSTBOUND_GAME_H
#include <stdint.h>
#define VERSION "0.1.0"
#define WORLD 128
#define MAX_ACTORS 80
#define MAX_BUILDINGS 100
#define MAX_NODES 200
#define PACK 20
#define SKILLS 20
#define LIMBS 6
#define ITEMS 30
#define TOWNS 5
#define SQUAD 4
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
    RELIC
};
enum { PLAYER, SETTLER, GUARD, TRADER, RECRUIT, RAIDER, BEAST, CARAVAN };
enum { FREE, UNION, SCAVENGERS, HOSTILE };
enum { HOUSE, SHOP, BAR, TOWER, WINDMILL, WELL, CAMPFIRE, BED, FARM, FORGE, BENCH, WALL, STORE };
enum { NODE_ORE, NODE_COPPER, NODE_WOOD, NODE_LOOT, NODE_LOCKED, NODE_RUIN };
enum { JOB_FOLLOW, JOB_HOLD, JOB_MINE, JOB_FARM, JOB_MEDIC };
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
    SCREEN_CONFIRM
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
} ItemDef;
typedef struct {
    char name[16];
    float x, y, homeX, homeY, tx, ty, health[LIMBS], blood, hunger, fatigue, bleed, cooldown, action, anim,
        unconscious;
    float xp[SKILLS];
    Stack pack[PACK];
    int gear[4];
    int active, kind, faction, squad, dead, job, target, taskKind, taskIndex, sneaking, autoattack, carrying,
        carriedBy, lostMask, prostheticMask, bounty, arrested, lootTaken;
} Actor;
typedef struct {
    float x, y;
    int type, faction, owner, level, stock;
    float progress;
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
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[4], kills, discovered[TOWNS], stock[TOWNS][ITEMS],
        lastRestock;
    int actorCount, buildingCount, nodeCount, baseExists;
    float baseX, baseY;
    uint8_t seen[WORLD * WORLD / 8];
    Actor actors[MAX_ACTORS];
    Building buildings[MAX_BUILDINGS];
    Node nodes[MAX_NODES];
} Game;
typedef struct {
    int screen, selection, scroll, tab, paused, speed, contextKind, contextIndex, tradeTown, sell, slot,
        confirmAction, titleChoice, waypoint;
    float mapX, mapY, camX, camY, noticeTime, repeat;
    char logs[LOGS][80], notice[100];
    int logCount;
} UI;
typedef struct {
    int held, pressed;
    float ax, ay;
} Input;
extern Game g;
extern UI ui;
extern volatile uint32_t display_buffer;
extern volatile uint32_t rendered_frames;
extern const ItemDef itemDefs[ITEMS];
extern const Town towns[TOWNS];
extern const char *skillNames[SKILLS], *limbNames[LIMBS], *buildNames[], *jobNames[];
int level(const Actor *a, int skill);
void gain(Actor *a, int skill, float xp);
int inventory_count(const Actor *a, int item);
int inventory_add(Actor *a, int item, int qty);
int inventory_remove(Actor *a, int item, int qty);
float encumbrance(const Actor *a);
float movement_speed(const Actor *a);
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
#endif
