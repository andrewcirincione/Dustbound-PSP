/* Visual regression sheets use the production actor renderer at 2x PSP size. */
#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[512 * 272];
static uint32_t reference[512 * 272];
static void verify_mirroring(void) {
    int pairs = 0;
    for (int f = 0; f < WEAPON_FAMILIES; f++)
        for (int direction = 0; direction < 8; direction++) {
            int item = gear_id(weapon_family_index(f), 3, 2);
            float angle = direction * .39269908f - 1.4f;
            memset(canvas, 0, sizeof(canvas));
            weapon_sprite(item, 240, 136, 3, angle, 1, 1);
            memcpy(reference, canvas, sizeof(canvas));
            memset(canvas, 0, sizeof(canvas));
            weapon_sprite(item, 240, 136, 3, 3.14159265f - angle, -1, 1);
            int visible = 0, different = 0;
            for (int y = 0; y < 272; y++)
                for (int x = 0; x < 480; x++) {
                    uint32_t left = canvas[y * 512 + 479 - x], right = reference[y * 512 + x];
                    if (left || right)
                        visible++;
                    if (left != right)
                        different++;
                }
            if (different > 6 + visible / 20) {
                fprintf(stderr, "Weapon fails facing reflection: family=%d angle=%g mismatched=%d/%d\n", f,
                        angle, different, visible);
                assert(0);
            }
            pairs++;
        }
    printf("PASS: %d asymmetric weapon raster reflections preserve edge orientation.\n", pairs);
}
static void verify_back_mounts(Actor a) {
    const int bags[] = {76, 78, 79, 82};
    int cases = 0;
    for (int b = 0; b < 4; b++)
        for (int torso = 32; torso < 48; torso++)
            for (int body = 0; body < 3; body++)
                for (int face = 0; face < 2; face++)
                    for (int pose = 0; pose < 3; pose++) {
                        a.gear[0] = a.attackWeapon = NONE;
                        a.gear[1] = gear_id(torso, 1, 1);
                        a.body = body;
                        a.facing = face;
                        a.sneaking = pose == 1;
                        a.attackPhase = pose == 2 ? 2 : 0;
                        a.attackAngle = face ? 0 : 3.14159265f;
                        a.attackDuration = 1;
                        a.attackClock = 0;
                        a.gear[5] = NONE;
                        memset(canvas, 0, sizeof(canvas));
                        actor_painted(&a, 240, 150, 2, 1);
                        memcpy(reference, canvas, sizeof(canvas));
                        a.gear[5] = gear_id(bags[b], 0, 1);
                        memset(canvas, 0, sizeof(canvas));
                        actor_painted(&a, 240, 150, 2, 1);
                        int changed = 0;
                        for (int y = 0; y < 272; y++)
                            for (int x = 0; x < 480; x++)
                                if (canvas[y * 512 + x] != reference[y * 512 + x]) {
                                    changed++;
                                    // Packs end above the knees even while crouching.
                                    assert(y < 138);
                                    assert(y >= 80);
                                    assert(face ? x < 248 : x > 231);
                                }
                        assert(changed > 15); // Not hidden completely inside torso.
                        cases++;
                    }
    printf("PASS: %d backpack/body/torso/facing/pose renders keep visible loads on the upper back.\n", cases);
}
static void save_sheet(const char *name) {
    FILE *f = fopen(name, "wb");
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
static void verify_hat_fit(Actor a) {
    int cases = 0;
    a.gear[0] = a.gear[5] = a.attackWeapon = NONE;
    a.attackPhase = a.sneaking = a.movedTimer = 0;
    for (int family = 48; family < 60; family++)
        for (int body = 0; body < 3; body++)
            for (int face = 0; face < 2; face++) {
                a.body = body;
                a.facing = face;
                a.gear[2] = NONE;
                memset(canvas, 0, sizeof(canvas));
                actor_painted(&a, 240, 150, 2, 1);
                memcpy(reference, canvas, sizeof(canvas));
                a.gear[2] = gear_id(family, 0, 0);
                memset(canvas, 0, sizeof(canvas));
                actor_painted(&a, 240, 150, 2, 1);
                int brow = 0, lo = 480, hi = 0;
                for (int y = 0; y < 272; y++)
                    for (int x = 0; x < 480; x++)
                        if (canvas[y * 512 + x] != reference[y * 512 + x]) {
                            assert(y > 55 && y < 115);
                            if (x < lo)
                                lo = x;
                            if (x > hi)
                                hi = x;
                            if (y >= 85 && y <= 101 && x >= 235 && x <= 245)
                                brow++;
                        }
                assert(brow > 4 && hi - lo < 24);
                cases++;
            }
    printf("PASS: %d headwear/body/facing renders overlap the brow and stay within fitted head bounds.\n",
           cases);
}
int main(int argc, char **argv) {
    const char *tag = argc > 1 ? argv[1] : "after";
    game_new(73819, 0);
    fb = canvas;
    pitch = 512;
    Actor a = g.actors[0];
    a.gear[1] = gear_id(34, 1, 1);
    a.gear[2] = gear_id(49, 0, 1);
    a.gear[3] = gear_id(60, 0, 1);
    a.gear[4] = gear_id(68, 0, 1);
    a.movedTimer = 0;
    a.attackDuration = 1;
    verify_mirroring();
    verify_back_mounts(a);
    verify_hat_fit(a);
    char path[160];
    for (int pose = 0; pose < 3; pose++) {
        rect(0, 0, 480, 272, INK);
        text(10, 8, "BACK EQUIPMENT / BOTH FACINGS / 2X", GOLD, 1);
        for (int f = 0; f < 8; f++) {
            int x = 60 + (f % 4) * 120, y = 116 + (f / 4) * 128;
            a.gear[0] = a.attackWeapon = NONE;
            a.gear[5] = gear_id(76 + f, 0, 1);
            a.attackPhase = 0;
            a.sneaking = pose == 1;
            a.movedTimer = pose == 2;
            a.anim = .7f;
            for (int face = 0; face < 2; face++) {
                a.facing = face;
                person(&a, x + (face ? -26 : 26), y - 8, 2, 0);
            }
            center(x, y, familyNames[76 + f], TEXT, 1);
        }
        snprintf(path, sizeof(path), "build/v051-%s-backs-%d.ppm", tag, pose);
        save_sheet(path);
    }
    a.sneaking = a.movedTimer = 0;
    a.gear[0] = NONE;
    a.gear[5] = NONE;
    for (int group = 0; group < 11; group++) {
        rect(0, 0, 480, 272, INK);
        text(10, 8, "CLOTHING FIT / BOTH FACINGS / 2X", GOLD, 1);
        for (int cell = 0; cell < 8; cell++) {
            int family = 32 + group * 4 + cell / 2, item = gear_id(family, 0, 1);
            Actor dressed = a;
            dressed.gear[1] = gear_id(34, 0, 1);
            dressed.gear[2] = NONE;
            dressed.gear[itemDefs[item].slot] = item;
            dressed.facing = !(cell & 1);
            int x = 60 + (cell % 4) * 120, y = 116 + (cell / 4) * 128;
            person(&dressed, x, y - 8, 2, 0);
            center(x, y, familyNames[family], TEXT, 1);
        }
        snprintf(path, sizeof(path), "build/v051-%s-clothing-%02d.ppm", tag, group);
        save_sheet(path);
    }
    a.gear[2] = gear_id(49, 0, 1);
    a.gear[5] = gear_id(82, 0, 1);
    for (int page = 0; page < 9; page++) {
        for (int pose = 0; pose < 4; pose++) {
            rect(0, 0, 480, 272, INK);
            text(10, 8, "WEAPON GRIPS / BOTH FACINGS / 2X", GOLD, 1);
            for (int cell = 0; cell < 8; cell++) {
                int f = page * 4 + cell / 2, x = 60 + (cell % 4) * 120, y = 116 + (cell / 4) * 128;
                a.gear[0] = a.attackWeapon = gear_id(weapon_family_index(f), 3, 2);
                a.facing = !(cell & 1);
                a.attackAngle = a.facing ? 0 : 3.14159265f;
                a.attackPhase = pose ? 2 : 0;
                a.attackClock = pose == 1 ? 1 : pose == 2 ? 1 - STRIKE_CONTACT : 0;
                person(&a, x, y - 8, 2, 0);
                center(x, y, familyNames[weapon_family_index(f)], TEXT, 1);
            }
            snprintf(path, sizeof(path), "build/v051-%s-weapons-%d-%d.ppm", tag, page, pose);
            save_sheet(path);
        }
    }
    puts("Attachment visual sheets rendered.");
    return 0;
}
