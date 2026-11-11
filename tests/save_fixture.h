#ifndef DUSTBOUND_SAVE_FIXTURE_H
#define DUSTBOUND_SAVE_FIXTURE_H
#include "game.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic pre-war payloads for targeted migration cases. Separate suites
 * load the archived saves written by older executables. */
static void save_prior_fixture(const char *path, int version) {
    int items = version>=11 ? TOWN_STOCK_ITEMS : version==10 ? V100_ITEMS :
                version==9 ? V090_ITEMS : version==8 ? OLD_ITEMS :
                version>=5 ? CATALOG_ITEMS : 2052;
    Game *copy=malloc(sizeof(*copy)); assert(copy); *copy=g;
    /* Modern residents may occupy buildings absent from a 320-building payload. */
    for(int n=0;n<MAX_ACTORS;n++) if(copy->depth.actors[n].room>320) copy->depth.actors[n].room=0;
    if(copy->interiorBuilding>320) copy->interiorBuilding=0;
    if(version>=12) copy->population.version=1;
    size_t size = version == 2 ? 404444 : version == 5 ? 410656 : version == 7 ? 435592 : 2148384;
    unsigned char *raw = malloc(sizeof(Game));
    assert(raw);
    unsigned char *dest = raw;
    const unsigned char *src = (const unsigned char *)copy;
#define THROUGH(end) do { \
        const unsigned char *last = (const unsigned char *)(end); \
        size_t count = last - src; \
        memcpy(dest, src, count); dest += count; src = last; \
    } while (0)
#define KEEP(field, n) do { \
        THROUGH(copy->field); \
        THROUGH(copy->field + (n)); \
        src = (const unsigned char *)(copy->field + sizeof(copy->field) / sizeof(copy->field[0])); \
    } while (0)
    KEEP(discovered, OLD_TOWNS);
    for (int t = 0; t < OLD_TOWNS; ++t) KEEP(stock[t], items);
    src = (const unsigned char *)&copy->lastRestock;
    THROUGH(&copy->buildingCount);
    int buildings = copy->buildingCount < 320 ? copy->buildingCount : 320;
    memcpy(dest, &buildings, sizeof(buildings));
    dest += sizeof(buildings);
    src += sizeof(buildings);
    KEEP(buildings, 320);
    KEEP(townOwner, OLD_TOWNS);
    KEEP(prosperity, OLD_TOWNS);
    KEEP(townState, OLD_TOWNS);
    KEEP(depth.buildings, 320);
    KEEP(society.crisis, OLD_TOWNS);
    KEEP(population.groups, OLD_POP_GROUPS);
    KEEP(population.state, OLD_POP_GROUPS);
    KEEP(population.social, OLD_POP_GROUPS);
    KEEP(life.groups, OLD_POP_GROUPS);
    KEEP(life.towns, OLD_TOWNS);
    KEEP(fieldNotes.group, OLD_POP_GROUPS);
    THROUGH(&copy->war);
#undef KEEP
#undef THROUGH
    assert((size_t)(dest - raw) >= size);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) hash = (hash ^ raw[i]) * 16777619u;
    uint32_t header[] = {(uint32_t)version, (uint32_t)size, hash};
    FILE *file = fopen(path, "wb");
    assert(file && fwrite("DUSTSAVE", 8, 1, file) == 1 && fwrite(header, sizeof(header), 1, file) == 1);
    assert(fwrite(raw, size, 1, file) == 1 && fclose(file) == 0);
    free(raw);
    free(copy);
}
#endif
