/* Articulated painted parts share the simulation's combat pose and rigid grip.
   Only world sprites are composed at 2x and filtered. Portraits sample the same
   indexed art directly; no runtime decoder, allocation or per-outfit atlas. */
static uint64_t partKeys[ART_COUNT];
static uint32_t paintHair = 0x30251e;
static void rgba_over(uint32_t *dst, uint32_t src) {
    unsigned a = src >> 24;
    if (!a)
        return;
    if (a == 255) {
        *dst = src;
        return;
    }
    uint32_t d = *dst;
    unsigned da = d >> 24, remain = da * (255 - a) / 255, out = a + remain;
    unsigned r = ((src & 255) * a + (d & 255) * remain) / out;
    unsigned g0 = (((src >> 8) & 255) * a + ((d >> 8) & 255) * remain) / out;
    unsigned b = (((src >> 16) & 255) * a + ((d >> 16) & 255) * remain) / out;
    *dst = (out << 24) | r | (g0 << 8) | (b << 16);
}
static void part_palette(int id, uint32_t tint, float li, int quality) {
    int lum = (int)(li * 64);
    uint64_t key = (uint64_t)tint | ((uint64_t)(lum + quality * 3) << 24);
    if (id >= 60 && id < 64)
        key ^= (uint64_t)paintHair << 39;
    key++;
    if (partKeys[id] == key)
        return;
    partKeys[id] = key;
    for (int i = 0; i < 256; i++) {
        uint32_t c = artSprites[id].palette[i];
        int r = c & 255, g0 = (c >> 8) & 255, b = (c >> 16) & 255;
        if (id >= 60 && id < 64) {
            if (r < 78 && g0 < 70) {
                int value = (r + g0 + b) / 3;
                r = value * (64 + (paintHair >> 16)) / 96;
                g0 = value * (64 + ((paintHair >> 8) & 255)) / 96;
                b = value * (64 + (paintHair & 255)) / 96;
            } else {
                r = r * (tint >> 16) / 185;
                g0 = g0 * ((tint >> 8) & 255) / 143;
                b = b * (tint & 255) / 107;
            }
        } else {
            r = r * (160 + (tint >> 16) / 2) / 224;
            g0 = g0 * (160 + ((tint >> 8) & 255) / 2) / 224;
            b = b * (160 + (tint & 255) / 2) / 224;
        }
        r = r * (lum + quality * 2) / 64;
        g0 = g0 * (lum + quality * 2) / 64;
        b = b * (lum + quality * 2) / 64;
        artPalette[id][i] = (c & 0xff000000) | (r > 255 ? 255 : r) | ((g0 > 255 ? 255 : g0) << 8) |
                            ((b > 255 ? 255 : b) << 16);
    }
}
static void part(int id, float ax, float ay, float bx, float by, float width, uint32_t tint, float li,
                 int quality, int flip, float v0, float v1) {
    const ArtSprite *s = &artSprites[id];
    float dx = bx - ax, dy = by - ay, length = sqrtf(dx * dx + dy * dy);
    if (length < .1f || width < .1f)
        return;
    float ux = dy / length, uy = -dx / length;
    int left = (int)floorf(fminf(ax, bx) - fabsf(ux) * width * .5f),
        right = (int)ceilf(fmaxf(ax, bx) + fabsf(ux) * width * .5f);
    int top = (int)floorf(fminf(ay, by) - fabsf(uy) * width * .5f),
        bottom = (int)ceilf(fmaxf(ay, by) + fabsf(uy) * width * .5f);
    if (left < clipLeft)
        left = clipLeft;
    if (top < clipTop)
        top = clipTop;
    if (right > clipRight)
        right = clipRight;
    if (bottom > clipBottom)
        bottom = clipBottom;
    part_palette(id, tint, li, quality);
    float dux = ux / width, duy = uy / width, dvx = dx / (length * length), dvy = dy / (length * length);
    for (int y = top; y < bottom; y++) {
        float u = ((left + .5f - ax) * dux + (y + .5f - ay) * duy) + .5f,
              v = (left + .5f - ax) * dvx + (y + .5f - ay) * dvy;
        for (int x = left; x < right; x++, u += dux, v += dvx) {
            if (u < 0 || u >= 1 || v < 0 || v >= 1)
                continue;
            int sx = (int)((flip ? 1 - u : u) * s->w), sy = (int)((v0 + (v1 - v0) * v) * s->h);
            if (sx < 0 || sx >= s->w || sy < 0 || sy >= s->h)
                continue;
            int lo = s->rows[sy * 3 + 1], hi = s->rows[sy * 3 + 2];
            if (sx < lo || sx >= hi)
                continue;
            rgba_over(fb + y * pitch + x, artPalette[id][s->pixels[s->rows[sy * 3] + sx - lo]]);
        }
    }
}
static void actor_painted(Actor *a, int x, int y, int sc, float li) {
    static const uint32_t skins[] = {0xe0b78a, 0xc39971, 0xad7d58, 0x956645,
                                     0x744c36, 0x553d30, 0xc5b69b, 0x817a68};
    static const uint32_t hairs[] = {0x211e1b, 0x433026, 0x776047, 0xa4835c, 0xc1b5a0, 0x713e32};
    uint32_t skin = skins[a->skin % 8];
    paintHair = hairs[a->hairColor % 6];
    int torso = a->gear[1], head = a->gear[2], legs = a->gear[4], back = a->gear[5], boots = a->gear[3];
    int tf = gear_family(torso), hf = gear_family(head), lf = gear_family(legs), bf = gear_family(back),
        ff = gear_family(boots);
    if (tf < 32 || tf > 47)
        tf = torso == DUSTER ? 32 : torso == PLATE ? 38 : torso == LEATHER ? 35 : 33;
    if (hf < 48 || hf > 59)
        hf = head == STRAW ? 48 : head == HELM ? 50 : head == HAT ? 49 : -1;
    if (lf < 68 || lf > 75)
        lf = 69;
    if (ff < 60 || ff > 67)
        ff = boots == SANDALS ? 61 : 60;
    int face = a->facing ? 1 : -1, missing = a->lostMask & ~a->prostheticMask;
    float progress = a->attackDuration > 0 ? 1 - a->attackClock / a->attackDuration : 0;
    int weapon = a->attackPhase ? a->attackWeapon : a->gear[0];
    CombatPose rig = combat_pose(weapon, a->attackPhase, progress);
    float aim = a->attackPhase ? a->attackAngle : (face > 0 ? 0 : 3.14159265f), forward = cosf(aim),
          depth = sinf(aim);
    float pose = a->movedTimer > 0 ? sinf(a->anim * 2) * 2.6f : 0, lean = rig.shift * forward;
    if (a->hitFlash > 0 && a->impactKind == 1)
        lean -= (a->recoilX - a->recoilY) * 3 * pose_ease(a->hitFlash / .38f);
    float crouch = rig.crouch + (a->sneaking ? 3 : 0), bw = a->body == 1 ? 5 : a->body == 2 ? 3.8f : 4.4f;
#define RX(xx, yy) (x + ((xx) * (1 + rig.twist) + lean * fminf(1, fmaxf(0, -(yy) / 24.f))) * sc)
#define RY(yy) (y + ((yy) + crouch * fminf(1, fmaxf(0, -(yy) / 13.f))) * sc)
#define PART(id, xx, yy, xx1, yy1, w, tint, q)                                                               \
    part(id, RX(xx, yy), RY(yy), RX(xx1, yy1), RY(yy1), (w) * sc, tint, li, q, face < 0, 0, 1)
    if (a->kind == BEAST) {
        int species = (a->skin % 4) * 2 + (a->attackPhase == 2);
        float bob = a->movedTimer > 0 ? fabsf(pose) * .5f : 0;
        PART(96 + species, 0, -22 - bob, 0, 0, 33, 0xb4a38e, 0);
        return;
    }
    uint32_t cloth = torso ? itemDefs[torso].color : 0x9b8670, lc = legs ? itemDefs[legs].color : 0x786d5e;
    if (a->dead || a->unconscious > 0 || a->taskKind == 3) {
        part(tf, x - 9 * sc, y - 2 * sc, x + 6 * sc, y - 2 * sc, 8 * sc, cloth, li, itemDefs[torso].quality,
             0, 0, 1);
        part(64 + lf - 68, x - 9 * sc, y - 2 * sc, x - 18 * sc, y, 5 * sc, lc, li, 0, 0, 0, 1);
        part(60, x + 7 * sc, y - 5 * sc, x + 11 * sc, y, 5 * sc, skin, li, 0, 0, 0, 1);
        weapon_sprite(weapon, x, y + 3 * sc, sc, .15f, li);
        return;
    }
    if (back) {
        int bid = bf >= 76 && bf <= 83 ? 88 + bf - 76 : 88;
        float height = bf == 77 ? 36 : bf == 80 ? 27 : bf == 83 ? 18 : 18;
        PART(bid, -face * 4, -height - 4, -face * 4, -4, 12, itemDefs[back].color, itemDefs[back].quality);
    }
    for (int i = 0; i < 2; i++) {
        int limb = i ? RLEG : LLEG;
        if (missing & (1 << limb))
            continue;
        float xx = i ? 2 : -2.5f, off = (i ? pose : -pose) * (a->health[limb] < 20 ? .5f : 1);
        float foot = xx + (i ? 1 : -1) * rig.step * forward + off * .65f,
              knee = xx + rig.shift * forward + off * .35f;
        float lift = a->attackPhase ? 0 : fmaxf(0, off) * .4f;
        int mech = a->prostheticMask & (1 << limb), id = mech ? 76 : 64 + lf - 68;
        part(id, RX(xx, -14), RY(-14), RX(knee, -7), RY(-7), 4.8f * sc, lc, li, itemDefs[legs].quality,
             face < 0, 0, mech ? 1 : .54f);
        part(mech ? 77 : id, RX(knee, -7.4f), RY(-7.4f), RX(foot, -2 - lift), RY(-2 - lift), 4.1f * sc, lc,
             li, itemDefs[legs].quality, face < 0, mech ? 0 : .48f, 1);
        PART(mech ? 87 : 80 + ff - 60, foot, -5 - lift, foot + face * .6f, -.2f - lift, 5.4f,
             boots ? itemDefs[boots].color : skin, itemDefs[boots].quality);
    }
    float angle = aim + face * rig.blade, handx = forward * rig.handX,
          handy = rig.handY + depth * rig.handX * .4f;
    WeaponGrip grip = weapon_grip(RX(handx, handy), RY(handy), angle, sc);
    float wrists[2][2];
    // Arms behind the body; wrists and handle remain one rigid construction.
    for (int i = 0; i < 2; i++) {
        int limb = i ? RARM : LARM;
        grip_hand(grip, !i, &wrists[i][0], &wrists[i][1]);
        if (!i && !combat_two_hands(weapon)) {
            wrists[i][0] = RX(-face * 5, -17 + rig.crouch);
            wrists[i][1] = RY(-17 + rig.crouch);
        }
        if (missing & (1 << limb))
            continue;
        float sx = RX(i ? bw - 1 : -bw + 1, -24), sy = RY(-24), hx = wrists[i][0], hy = wrists[i][1];
        float dx = hx - sx, dy = hy - sy, d = hypotf(dx, dy),
              bend = sqrtf(fmaxf(0, 49 * sc * sc - d * d * .25f));
        float ex = (sx + hx) * .5f - (d > .01f ? dy / d : 0) * bend * (i ? 1 : -1),
              ey = (sy + hy) * .5f + (d > .01f ? dx / d : 1) * bend * (i ? 1 : -1);
        int mech = a->prostheticMask & (1 << limb);
        part(mech ? 76 : 72, sx, sy, ex, ey, 4 * sc, cloth, li, 0, i, 0, 1);
        part(mech ? 77 : 75, ex, ey, hx, hy, 3 * sc, mech ? 0x777b78 : skin, li, 0, i, 0, 1);
    }
    int coat = tf == 32 || tf == 33 || tf == 39 || tf == 40 || tf == 42 || tf == 44 || tf == 47;
    PART(tf, 0, -26, coat ? -rig.shift * .35f : 0, coat ? -5 : -12, bw * 2.8f, cloth,
         itemDefs[torso].quality);
    // Painted face with independently tinted hair and visible creator details.
    int faceid = a->hair == 0 ? 61 : a->hair == 3 ? 62 : a->beard ? 63 : 60;
    PART(faceid, 0, -33, 0, -25, 5.8f, skin, 0);
    if (a->hair == 2 || a->hair == 4 || a->hair == 5 || a->hair == 6) {
        PART(72, -face * 2.2f, -31, -face * 3, -23, 1.8f, paintHair, 0);
        if (a->hair == 4)
            PART(72, face * 2.2f, -31, face * 3, -23, 1.8f, paintHair, 0);
        if (a->hair == 6)
            PART(72, -face * 3, -31, -face * 5, -25, 2.2f, paintHair, 0);
    }
    if (a->scar)
        line(RX(face, -29), RY(-29), RX(face * 1.5f, -27), RY(-27), shade(0x874b38, li));
    if (a->race == 2)
        px(RX(-face * 1.7f, -29), RY(-29), 0x8eae9e);
    if (head && hf >= 48) {
        float wide = (hf == 48 || hf == 49 || hf == 56 || hf == 58) ? 14 : 10;
        float bottom = (hf == 48 || hf == 49 || hf == 56 || hf == 58) ? -28 : -25;
        PART(hf, 0, -37, 0, bottom, wide, itemDefs[head].color, itemDefs[head].quality);
    }
    if (!(missing & (1 << RARM)))
        weapon_sprite(weapon, (int)grip.x, (int)grip.y, sc, angle, li);
    for (int i = 0; i < 2; i++)
        if (!(missing & (1 << (i ? RARM : LARM)))) {
            float hx = wrists[i][0], hy = wrists[i][1];
            part(78, hx, hy - 1.6f * sc, hx, hy + 1.6f * sc, 2.8f * sc, skin, li, 0, i, 0, 1);
        }
    if (a->carrying >= 0)
        part(33, x - 9 * sc, y - 24 * sc, x + 9 * sc, y - 24 * sc, 7 * sc, 0x7c6852, li, 0, 0, 0, 1);
#undef PART
#undef RX
#undef RY
}
#define ACT_W 96
#define ACT_H 80
#define ACT_CACHE 24
static uint32_t actorScratch[ACT_W * 2 * ACT_H * 2];
static struct {
    uint64_t key;
    uint32_t pixels[ACT_W * ACT_H];
} actorCache[ACT_CACHE];
static unsigned actorNext;
static uint64_t actor_key(const Actor *a, int lum) {
    uint64_t k = 1469598103934665603ull;
#define HASHV(v) k = (k ^ (uint32_t)(v)) * 1099511628211ull
    for (int i = 0; i < GEARS; i++) {
        HASHV(a->gear[i]);
    }
    HASHV(a->body);
    HASHV(a->skin);
    HASHV(a->hair);
    HASHV(a->hairColor);
    HASHV(a->beard);
    HASHV(a->scar);
    HASHV(a->race);
    HASHV(a->facing);
    HASHV(a->lostMask);
    HASHV(a->prostheticMask);
    HASHV(a->sneaking);
    HASHV(a->dead);
    HASHV(a->unconscious > 0);
    HASHV(a->taskKind == 3);
    HASHV(a->kind);
    HASHV(a->attackPhase);
    HASHV(a->attackWeapon);
    HASHV((int)(a->attackAngle * 64));
    HASHV(a->attackDuration > 0 ? (int)((1 - a->attackClock / a->attackDuration) * 32) : 0);
    HASHV(a->movedTimer > 0 ? (int)(a->anim * 8) : 0);
    HASHV(a->health[LLEG] < 20);
    HASHV(a->health[RLEG] < 20);
    HASHV(a->carrying >= 0);
    HASHV((int)(a->hitFlash * 30));
    HASHV(a->impactKind);
    HASHV((int)(a->recoilX * 8));
    HASHV((int)(a->recoilY * 8));
    HASHV(lum);
#undef HASHV
    return k ? k : 1;
}
static void person(Actor *a, int x, int y, int sc, int chosen) {
    float li = sc == 1 ? light() : 1;
    if (chosen && sc == 1) {
        line(x - 10, y + 1, x, y + 6, TEAL);
        line(x, y + 6, x + 10, y + 1, TEAL);
    }
    if (sc != 1) {
        actor_painted(a, x, y, sc, li);
        return;
    }
    uint64_t key = actor_key(a, (int)(li * 64));
    int slot = -1;
    for (int i = 0; i < ACT_CACHE; i++)
        if (actorCache[i].key == key) {
            slot = i;
            break;
        }
    if (slot < 0) {
        slot = actorNext++ % ACT_CACHE;
        actorCache[slot].key = key;
        uint32_t *oldfb = fb;
        int oldpitch = pitch, l = clipLeft, t = clipTop, r = clipRight, b = clipBottom;
        memset(actorScratch, 0, sizeof(actorScratch));
        fb = actorScratch;
        pitch = ACT_W * 2;
        clipLeft = clipTop = 0;
        clipRight = ACT_W * 2;
        clipBottom = ACT_H * 2;
        actor_painted(a, ACT_W, 136, 2, li);
        fb = oldfb;
        pitch = oldpitch;
        clipLeft = l;
        clipTop = t;
        clipRight = r;
        clipBottom = b;
        for (int yy = 0; yy < ACT_H; yy++)
            for (int xx = 0; xx < ACT_W; xx++) {
                unsigned alpha = 0, red = 0, green = 0, blue = 0;
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++) {
                        uint32_t c = actorScratch[(yy * 2 + dy) * ACT_W * 2 + xx * 2 + dx];
                        unsigned a0 = c >> 24;
                        alpha += a0;
                        red += (c & 255) * a0;
                        green += ((c >> 8) & 255) * a0;
                        blue += ((c >> 16) & 255) * a0;
                    }
                actorCache[slot].pixels[yy * ACT_W + xx] = alpha ? ((alpha / 4) << 24) | (red / alpha) |
                                                                       ((green / alpha) << 8) |
                                                                       ((blue / alpha) << 16)
                                                                 : 0;
            }
    }
    // Small directional contact shadow, matching the environment light vector.
    for (int j = 0; j < 12; j++) {
        int sx = x + j, sy = y + j / 2;
        for (int k = -2; k < 3; k++)
            if (sx + k >= clipLeft && sx + k < clipRight && sy >= clipTop && sy < clipBottom) {
                uint32_t *d = fb + sy * pitch + sx + k;
                unsigned m = 238;
                *d = 0xff000000 | (((*d & 0xff00ff) * m >> 8) & 0xff00ff) |
                     (((*d & 0xff00) * m >> 8) & 0xff00);
            }
    }
    for (int yy = 0; yy < ACT_H; yy++) {
        int sy = y - 68 + yy;
        if (sy < clipTop || sy >= clipBottom)
            continue;
        for (int xx = 0; xx < ACT_W; xx++) {
            int sx = x - ACT_W / 2 + xx;
            if (sx < clipLeft || sx >= clipRight)
                continue;
            rgba_over(fb + sy * pitch + sx, actorCache[slot].pixels[yy * ACT_W + xx]);
        }
    }
    if (a->hitFlash > 0) {
        char damage[16];
        snprintf(damage, sizeof(damage), "-%d", a->impactDamage);
        text(x - 8, y - 42 - (int)((.38f - a->hitFlash) * 20),
             a->impactKind == 2   ? "CLANG"
             : a->impactKind == 3 ? "DODGE"
                                  : damage,
             a->impactKind == 1 ? RED : GOLD, 1);
    }
}
