#ifndef DUSTBOUND_SETTLEMENT_ART_IDS_H
#define DUSTBOUND_SETTLEMENT_ART_IDS_H

enum {
    SETTLEMENT_ART_FIRST = 144,
    ART_RUST_HOME = SETTLEMENT_ART_FIRST, ART_DRY_SHOP, ART_ROAD_INN, ART_DRY_INN,
    ART_ARMORER, ART_STOCKADE, ART_ASH_HOME, ART_DUSTWARD,
    ART_SNOW_LODGE, ART_REED_HOME, ART_COURT_INN, ART_HERB_SHOP,
    ART_IRON_SEAM, ART_COPPER_SEAM, ART_CUT_TIMBER, ART_WATCH_CAGE,
    ART_LOW_COT, ART_SIDE_ALTAR, ART_IRON_STOVE, ART_ARMOR_STAND,
    ART_CARAVAN_BALES, ART_PRACTICE_POST, ART_CELL_PANEL, ART_SIDE_COUNTER,
    ART_IRON_GRILLE, ART_CELL_GATE, ART_COURT_WALL, ART_STONE_WALL,
    SETTLEMENT_ART_END,
    ART_INN_STAIRS = SETTLEMENT_ART_END + 15, ART_INN_BAR, ART_RESEARCH_TABLE, ART_BEDROLL
};

typedef struct {
    float w, d, doorX, doorY;
    int height, eastDoor;
} BuildingModel;

typedef struct {
    float span, xShare;
} FurnitureModel;

enum { FURNITURE_ART_FIRST = SETTLEMENT_ART_END, FURNITURE_ART_COUNT = 19, ROOM_MAX_SCALE = 25 };

static inline int furniture_art_index(int art) {
    if (art >= 16 && art <= 23) return art - 16;
    if (art >= ART_LOW_COT && art <= ART_PRACTICE_POST) return 8 + art - ART_LOW_COT;
    if (art == ART_SIDE_COUNTER) return 14;
    if (art == ART_INN_STAIRS || art == ART_INN_BAR) return 15 + art - ART_INN_STAIRS;
    return -1;
}

static inline int furniture_is_bed(int art) {
    return art == 16 || art == ART_LOW_COT;
}

static inline int structure_art_index(int sprite) {
    if (sprite >= 0 && sprite < 8) return sprite;
    if (sprite >= ART_RUST_HOME && sprite <= ART_HERB_SHOP) return sprite - ART_RUST_HOME + 8;
    return sprite == ART_WATCH_CAGE ? 20 : -1;
}

int building_sprite(int id);

#endif
