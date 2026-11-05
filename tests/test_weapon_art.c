/* Exercise the actual rasterizer, including painted alpha and actor cache bounds. */
#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[512 * 272 + 32];
static void save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n480 272\n255\n");
    for (int y = 0; y < 272; y++)
        for (int x = 0; x < 480; x++) {
            uint32_t p = canvas[y * 512 + x];
            unsigned char rgb[] = {p & 255, p >> 8 & 255, p >> 16 & 255};
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}
int main(void) {
    game_new(73819, 0);
    fb = canvas;
    pitch = 512;
    int grips = 0, poses = 0, minx = 999, maxx = -999, miny = 999, maxy = -999;
    Actor a = g.actors[0];
    a.gear[1] = gear_id(34, 1, 1);
    a.gear[2] = gear_id(49, 0, 1);
    a.gear[3] = gear_id(60, 0, 1);
    a.gear[4] = gear_id(68, 0, 1);
    a.gear[5] = gear_id(82, 0, 1);
    a.movedTimer = 0;
    for (int family = 0; family < 32; family++) {
        const ArtSprite *s = &artSprites[WEAPON_ART_FIRST + family];
        const WeaponArt *w = &weaponArt[family];
        int px0 = (int)w->gripX, py0 = (int)w->gripY;
        assert(px0 >= 0 && px0 < s->w && py0 >= 0 && py0 < s->h);
        int lo = s->rows[py0 * 3 + 1];
        assert(px0 >= lo && px0 < s->rows[py0 * 3 + 2]);
        assert(s->palette[s->pixels[s->rows[py0 * 3] + px0 - lo]] >> 24 >= 128);
        for (int sc = 1; sc <= 5; sc++)
            for (int dir = 0; dir < 16; dir++) {
                memset(canvas, 0, sizeof(canvas));
                weapon_sprite(gear_id(family, 3, 2), 240, 136, sc, dir * .39269908f, 1);
                int near = 0;
                for (int y = 135; y <= 137; y++)
                    for (int x = 239; x <= 241; x++)
                        near += (canvas[y * 512 + x] >> 24) >= 64;
                if (!near) {
                    fprintf(stderr, "Detached grip family %d scale %d dir %d\n", family, sc, dir);
                    return 1;
                }
                grips++;
            }
        a.gear[0] = a.attackWeapon = gear_id(family, 3, 2);
        for (int phase = 0; phase < 4; phase++)
            for (int dir = 0; dir < 8; dir++)
                for (int step = 0; step <= 12; step++) {
                    memset(canvas, 0, sizeof(canvas));
                    a.attackPhase = phase;
                    a.attackDuration = 1;
                    a.attackClock = 1 - step / 12.f;
                    a.attackAngle = dir * .785398163f;
                    a.facing = cosf(a.attackAngle) >= 0;
                    actor_painted(&a, 240, 152, 2, 1);
                    for (int y = 0; y < 272; y++)
                        for (int x = 0; x < 480; x++) {
                            if (canvas[y * 512 + x] >> 24 < 32)
                                continue;
                            int dx = x - 240, dy = y - 152;
                            if (dx < minx)
                                minx = dx;
                            if (dx > maxx)
                                maxx = dx;
                            if (dy < miny)
                                miny = dy;
                            if (dy > maxy)
                                maxy = dy;
                            if (dx < -ACT_W || dx >= ACT_W || dy < -136 || dy >= ACT_H * 2 - 136) {
                                fprintf(stderr,
                                        "Actor cache clipping: family=%d phase=%d direction=%d step=%d "
                                        "pixel=%d,%d\n",
                                        family, phase, dir, step, dx, dy);
                                return 1;
                            }
                        }
                    poses++;
                }
    }
    for (int family = 0; family < 32; family++) {
        Actor candidate = a;
        a.attackPhase = candidate.attackPhase = 0;
        a.facing = candidate.facing = 1;
        a.gear[0] = gear_id(0, 1, 1);
        candidate.gear[0] = gear_id(family, 3, 2);
        memset(canvas, 0, sizeof(canvas));
        compare_portraits(&a, &candidate);
        uint64_t first = 0;
        for (int y = 0; y < 272; y++)
            for (int x = 0; x < 512; x++) {
                int inside = y >= 92 && y < 168 && ((x >= 277 && x < 361) || (x >= 374 && x < 458));
                assert(inside || canvas[y * 512 + x] == 0);
                first = first * 31 + canvas[y * 512 + x];
            }
        memset(canvas, 0, sizeof(canvas));
        compare_portraits(&a, &candidate);
        uint64_t second = 0;
        for (int i = 0; i < 512 * 272; i++)
            second = second * 31 + canvas[i];
        assert(first == second);
    }
    for (int page = 0; page < 4; page++) {
        rect(0, 0, 480, 272, INK);
        text(12, 8, "PAINTED WEAPONS / NATIVE RENDER REVIEW", GOLD, 1);
        for (int cell = 0; cell < 8; cell++) {
            int family = page * 8 + cell, x = 58 + (cell % 4) * 120, y = 122 + (cell / 4) * 128;
            a.gear[0] = gear_id(family, 3, 2);
            a.attackPhase = 0;
            a.facing = 1;
            person(&a, x, y - 10, 2, 0);
            center(x, y, familyNames[family], TEXT, 1);
        }
        char path[100];
        snprintf(path, sizeof(path), "build/v041-weapons-%d.ppm", page);
        save_ppm(path);
    }
    printf("PASS: %d painted grip samples; %d attack poses fit actor cache; 32 bounded, cache-stable "
           "comparison pairs. Bounds x=%d..%d y=%d..%d at 2x.\n",
           grips, poses, minx, maxx, miny, maxy);
    return 0;
}
