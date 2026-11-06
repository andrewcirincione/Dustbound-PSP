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
/* Paint the complete weapon around its authored handle pivot. Palette tinting
   preserves warm leather/wood and patina while changing exposed metal. */
static void weapon_palette(int id, const ItemDef *item, float li) {
    uint64_t key = (uint64_t)item->color | ((uint64_t)((int)(li * 64) + item->quality * 3) << 24);
    key++;
    if (partKeys[id] == key)
        return;
    partKeys[id] = key;
    int lum = (int)(li * 64) + item->quality * 2;
    for (int i = 0; i < 256; i++) {
        uint32_t c = artSprites[id].palette[i];
        int r = c & 255, green = (c >> 8) & 255, b = (c >> 16) & 255;
        int max = r > green ? r : green;
        if (b > max)
            max = b;
        int min = r < green ? r : green;
        if (b < min)
            min = b;
        // Neutral texels are iron/steel. Warm wood, rust and leather retain their
        // painted colour; subtle material staining still distinguishes staffs.
        if (max - min < 28) {
            r = r * (64 + (item->color >> 16)) / 192;
            green = green * (64 + ((item->color >> 8) & 255)) / 192;
            b = b * (64 + (item->color & 255)) / 192;
        } else {
            r = r * (224 + (item->color >> 16) / 4) / 256;
            green = green * (224 + ((item->color >> 8) & 255) / 4) / 256;
            b = b * (224 + (item->color & 255) / 4) / 256;
        }
        r = r * lum / 64;
        green = green * lum / 64;
        b = b * lum / 64;
        artPalette[id][i] = (c & 0xff000000) | (r > 255 ? 255 : r) | ((green > 255 ? 255 : green) << 8) |
                            ((b > 255 ? 255 : b) << 16);
    }
}
static int weapon_family(int item) {
    int f = gear_family(item);
    if (f < 0)
        f = item == STAFF ? 16 : item == CROSSBOW ? 25 : item == CLEAVER ? 11 : item == SABRE ? 8 : 0;
    return f >= 84 && f < FAMILIES ? f - 52 : f < 32 ? f : 0;
}
static void weapon_sprite(int item, float x, float y, int sc, float angle, int face, float li) {
    if (!item)
        return;
    int f = weapon_family(item), id = WEAPON_ART_FIRST + f;
    const ArtSprite *s = &artSprites[id];
    const WeaponArt *w = &weaponArt[f];
    float scale = w->scale * sc, ca = cosf(angle), sa = sinf(angle);
    // Reflect across the handle-to-tip axis, never around the image centre:
    // the cutting edge leads the stroke and both hands retain their pivots.
    int parity = w->edgeSide * face;
    float left = 1e6f, top = 1e6f, right = -1e6f, bottom = -1e6f;
    for (int j = 0; j < 4; j++) {
        float along = (w->gripY - ((j & 1) ? s->h : 0)) * scale;
        float across = (((j & 2) ? s->w : 0) - w->gripX) * scale * parity;
        float xx = x + ca * along - sa * across, yy = y + sa * along + ca * across;
        left = fminf(left, xx);
        right = fmaxf(right, xx);
        top = fminf(top, yy);
        bottom = fmaxf(bottom, yy);
    }
    int x0 = (int)fmaxf(clipLeft, floorf(left)), x1 = (int)fminf(clipRight, ceilf(right));
    int y0 = (int)fmaxf(clipTop, floorf(top)), y1 = (int)fminf(clipBottom, ceilf(bottom));
    weapon_palette(id, &itemDefs[item], li);
    float du = -sa / scale * parity, dv = -ca / scale;
    for (int yy = y0; yy < y1; yy++) {
        float u = w->gripX + (-(x0 + .5f - x) * sa + (yy + .5f - y) * ca) / scale * parity;
        float v = w->gripY - ((x0 + .5f - x) * ca + (yy + .5f - y) * sa) / scale;
        for (int xx = x0; xx < x1; xx++, u += du, v += dv) {
            if (u < 0 || v < 0 || u >= s->w || v >= s->h)
                continue;
            int sx = (int)u, sy = (int)v, lo = s->rows[sy * 3 + 1];
            if (sx < lo || sx >= s->rows[sy * 3 + 2])
                continue;
            rgba_over(fb + yy * pitch + xx, artPalette[id][s->pixels[s->rows[sy * 3] + sx - lo]]);
        }
    }
}
typedef struct {
    float offset, top, bottom, width;
} BackAttachment;
static BackAttachment back_attachment(int item) {
    // Mount the load on the upper back, ending at the belt. Tall attachments
    // have separate silhouettes; flags, scabbards and cloth are not packs.
    static const BackAttachment mounts[8] = {
        {1.2f, -27, -12, 12}, // Travel pack
        {1.1f, -43, -12, 16}, // Banner above shoulder / roll at belt
        {1.2f, -28, -12, 13}, // Medicine basket
        {1.2f, -31, -12, 15}, // Ore pack with raised pick
        {.5f, -30, -5, 16},   // Cape hangs from collar
        {.6f, -35, -9, 17},   // Sword rack: hilts above shoulder
        {1.2f, -28, -11, 13}, // Box and bedroll
        {.45f, -30, -15, 16}, // Mantle surrounds shoulders
    };
    int f = gear_family(item);
    return mounts[f >= 76 && f <= 83 ? f - 76 : 0];
}
static void head_attachment(int family, float *width, float *top, float *bottom) {
    // Calibrated brow/brim rows in each cropped sprite. Chin straps, cheek
    // guards and crests must not be treated as the hat's contact with the head.
    static const float widths[12] = {11, 10.4f, 9.6f, 7.4f, 7.4f, 8.4f, 7.8f, 7.8f, 10.2f, 7.8f, 11, 9.2f};
    static const float brow[12] = {.61f, .86f, .54f, .40f, .55f, .46f, .32f, .42f, .82f, .36f, .43f, .53f};
    const ArtSprite *s = &artSprites[family];
    *width = widths[family - 48];
    float height = *width * s->h / s->w;
    *top = -30.2f - brow[family - 48] * height;
    *bottom = *top + height;
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
        /* Rotate the complete dressed rig: hats, boots and packs stay attached. */
        static uint32_t prone[112*104];
        Actor standing=*a;standing.dead=0;standing.unconscious=0;standing.taskKind=0;
        standing.attackPhase=0;standing.movedTimer=0;standing.hitFlash=0;standing.sneaking=0;
        uint32_t *oldfb=fb;int oldpitch=pitch,l=clipLeft,t=clipTop,r=clipRight,b=clipBottom;
        memset(prone,0,sizeof(prone));fb=prone;pitch=112;clipLeft=clipTop=0;clipRight=112;clipBottom=104;
        actor_painted(&standing,56,88,1,li);
        fb=oldfb;pitch=oldpitch;clipLeft=l;clipTop=t;clipRight=r;clipBottom=b;
        for(int sy=0;sy<104;sy++)for(int sx=0;sx<112;sx++)if(prone[sy*112+sx]) {
            int dx=x+(68-sy)*sc,dy=y-3*sc+(sx-56)*sc/2;
            for(int yy=0;yy<sc;yy++)for(int xx=0;xx<sc;xx++)if(dx+xx>=clipLeft&&dx+xx<clipRight&&dy+yy>=clipTop&&dy+yy<clipBottom)fb[(dy+yy)*pitch+dx+xx]=prone[sy*112+sx];
        }
        return;
    }
    if (back) {
        int bid = bf >= 76 && bf <= 83 ? 88 + bf - 76 : 88;
        BackAttachment mount = back_attachment(back);
        float bx = -face * bw * mount.offset;
        // The source shows straps on its left: mirror it for a right-facing
        // wearer so straps meet the torso and the load projects behind it.
        part(bid, RX(bx, mount.top), RY(mount.top), RX(bx, mount.bottom), RY(mount.bottom), mount.width * sc,
             itemDefs[back].color, li, itemDefs[back].quality, face > 0, 0, 1);
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
        weapon_hand(grip, weapon, !i, &wrists[i][0], &wrists[i][1]);
        if (!i && !combat_two_hands(weapon)) {
            wrists[i][0] = RX(-face * 5, -17 + rig.crouch);
            wrists[i][1] = RY(-17 + rig.crouch);
        }
        if (missing & (1 << limb))
            continue;
        float sx = RX((i ? bw - 1 : -bw + 1) * face, -24), sy = RY(-24), hx = wrists[i][0], hy = wrists[i][1];
        float dx = hx - sx, dy = hy - sy, d = hypotf(dx, dy),
              bend = sqrtf(fmaxf(0, 49 * sc * sc - d * d * .25f));
        float ex = (sx + hx) * .5f - (d > .01f ? dy / d : 0) * bend * (i ? 1 : -1) * face,
              ey = (sy + hy) * .5f + (d > .01f ? dx / d : 1) * bend * (i ? 1 : -1) * face;
        int mech = a->prostheticMask & (1 << limb);
        part(mech ? 76 : 72, sx, sy, ex, ey, 4 * sc, cloth, li, 0, i ^ (face < 0), 0, 1);
        part(mech ? 77 : 75, ex, ey, hx, hy, 3 * sc, mech ? 0x777b78 : skin, li, 0, i ^ (face < 0), 0, 1);
    }
    int coat = tf == 32 || tf == 33 || tf == 39 || tf == 40 || tf == 42 || tf == 44 || tf == 47;
    // Join the chin to the collar; transparent face/collar edges otherwise
    // leave a floating head, most visible with the smaller fitted hats.
    PART(72, 0, -27.5f, 0, -24, 2.8f, skin, 0);
    PART(tf, 0, -26, coat ? -rig.shift * .35f * forward : 0, coat ? -5 : -12, bw * 2.8f, cloth,
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
        float wide, top, bottom;
        head_attachment(hf, &wide, &top, &bottom);
        PART(hf, 0, top, 0, bottom, wide, itemDefs[head].color, itemDefs[head].quality);
    }
    if (!(missing & (1 << RARM)))
        weapon_sprite(weapon, grip.x, grip.y, sc, angle, face, li);
    for (int i = 0; i < 2; i++)
        if (!(missing & (1 << (i ? RARM : LARM)))) {
            float hx = wrists[i][0], hy = wrists[i][1];
            part(78, hx, hy - 1.6f * sc, hx, hy + 1.6f * sc, 2.8f * sc, skin, li, 0, i ^ (face < 0), 0, 1);
        }
    if (a->carrying >= 0)
        part(33, x - 9 * sc, y - 24 * sc, x + 9 * sc, y - 24 * sc, 7 * sc, 0x7c6852, li, 0, 0, 0, 1);
#undef PART
#undef RX
#undef RY
}
#define ACT_W 112
#define ACT_H 104
#define ACT_CACHE 24
static uint32_t actorScratch[ACT_W * 2 * ACT_H * 2];
static struct {
    uint64_t key;
    int left, top, right, bottom;
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
        actorCache[slot].left = ACT_W;
        actorCache[slot].top = ACT_H;
        actorCache[slot].right = actorCache[slot].bottom = 0;
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
                if (alpha >= 4) {
                    if (xx < actorCache[slot].left)
                        actorCache[slot].left = xx;
                    if (yy < actorCache[slot].top)
                        actorCache[slot].top = yy;
                    if (xx + 1 > actorCache[slot].right)
                        actorCache[slot].right = xx + 1;
                    if (yy + 1 > actorCache[slot].bottom)
                        actorCache[slot].bottom = yy + 1;
                }
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
    for (int yy = actorCache[slot].top; yy < actorCache[slot].bottom; yy++) {
        int sy = y - 68 + yy;
        if (sy < clipTop || sy >= clipBottom)
            continue;
        for (int xx = actorCache[slot].left; xx < actorCache[slot].right; xx++) {
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
/* Menu portraits keep their complete silhouette inside their allotted cell.
   Cache the two comparison figures so unchanged menus only composite pixels. */
#define PORTRAIT_W 160
#define PORTRAIT_H 128
static struct {
    uint64_t key;
    int width, height, x0, y0, x1, y1, outputValid;
    float intrinsic, drawnScale;
    uint32_t pixels[PORTRAIT_W * PORTRAIT_H];
} portraitCache[2];
static unsigned portraitNext;
static void portrait_source(Actor *a) {
    uint32_t *oldfb = fb;
    int oldpitch = pitch, l = clipLeft, t = clipTop, r = clipRight, b = clipBottom;
    fb = actorScratch;
    pitch = ACT_W * 2;
    clipLeft = clipTop = 0;
    clipRight = ACT_W * 2;
    clipBottom = ACT_H * 2;
    memset(actorScratch, 0, sizeof(actorScratch));
    actor_painted(a, ACT_W, 136, 2, 1);
    fb = oldfb;
    pitch = oldpitch;
    clipLeft = l;
    clipTop = t;
    clipRight = r;
    clipBottom = b;
}
static float portrait_scale(Actor *a, int width, int height, int drawX, int drawY, float limit) {
    if (width > PORTRAIT_W || height > PORTRAIT_H || width < 1 || height < 1)
        return 1;
    uint64_t key = actor_key(a, 64);
    int slot = -1;
    for (int i = 0; i < 2; i++)
        if (portraitCache[i].key == key && portraitCache[i].width == width &&
            portraitCache[i].height == height)
            slot = i;
    int sourceReady = 0;
    if (slot < 0) {
        slot = portraitNext++ % 2;
        portraitCache[slot].key = key;
        portraitCache[slot].width = width;
        portraitCache[slot].height = height;
        portraitCache[slot].outputValid = 0;
        portrait_source(a);
        sourceReady = 1;
        int x0 = ACT_W * 2, y0 = ACT_H * 2, x1 = 0, y1 = 0;
        for (int yy = 0; yy < ACT_H * 2; yy++)
            for (int xx = 0; xx < ACT_W * 2; xx++)
                if (actorScratch[yy * ACT_W * 2 + xx] >> 24 >= 16) {
                    if (xx < x0)
                        x0 = xx;
                    if (xx + 1 > x1)
                        x1 = xx + 1;
                    if (yy < y0)
                        y0 = yy;
                    if (yy + 1 > y1)
                        y1 = yy + 1;
                }
        portraitCache[slot].x0 = x0;
        portraitCache[slot].x1 = x1;
        portraitCache[slot].y0 = y0;
        portraitCache[slot].y1 = y1;
        portraitCache[slot].intrinsic =
            x1 <= x0 || y1 <= y0 ? 1 : fminf(1, fminf((float)width / (x1 - x0), (float)height / (y1 - y0)));
    }
    float scale = portraitCache[slot].intrinsic;
    if (drawX < 0)
        return scale;
    scale = fminf(scale, limit);
    if (!portraitCache[slot].outputValid || portraitCache[slot].drawnScale != scale) {
        if (!sourceReady)
            portrait_source(a);
        int x0 = portraitCache[slot].x0, y0 = portraitCache[slot].y0;
        int w = (int)((portraitCache[slot].x1 - x0) * scale);
        int h = (int)((portraitCache[slot].y1 - y0) * scale);
        int left = (width - w) / 2, top = height - h;
        memset(portraitCache[slot].pixels, 0, sizeof(portraitCache[slot].pixels));
        for (int yy = 0; yy < h; yy++)
            for (int xx = 0; xx < w; xx++) {
                int sx = x0 + (int)((xx + .5f) / scale), sy = y0 + (int)((yy + .5f) / scale);
                portraitCache[slot].pixels[(top + yy) * PORTRAIT_W + left + xx] =
                    actorScratch[sy * ACT_W * 2 + sx];
            }
        portraitCache[slot].outputValid = 1;
        portraitCache[slot].drawnScale = scale;
    }
    for (int yy = 0; yy < height; yy++)
        for (int xx = 0; xx < width; xx++) {
            int dx = drawX + xx, dy = drawY + yy;
            if (dx >= clipLeft && dx < clipRight && dy >= clipTop && dy < clipBottom)
                rgba_over(fb + dy * pitch + dx, portraitCache[slot].pixels[yy * PORTRAIT_W + xx]);
        }
    return scale;
}
static void compare_portraits(Actor *current, Actor *candidate) {
    float scale =
        fminf(portrait_scale(current, 84, 76, -1, 0, 1), portrait_scale(candidate, 84, 76, -1, 0, 1));
    portrait_scale(current, 84, 76, 277, 92, scale);
    portrait_scale(candidate, 84, 76, 374, 92, scale);
}
