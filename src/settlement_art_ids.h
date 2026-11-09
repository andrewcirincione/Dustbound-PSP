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
    SETTLEMENT_ART_END
};

typedef struct {
    float x, y; /* Painted threshold, measured in the cropped sprite. */
    int rightWall;
} PaintedDoor;

int building_sprite(int id);
PaintedDoor painted_door(int sprite);
void building_threshold(int id, float *x, float *y);

#endif
