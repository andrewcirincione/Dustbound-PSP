#include "game.h"
#include "combat_pose.h"
#include "art_data.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared, allocation-free software rasterizer. PSP stages in cached main RAM.
   No external asset files are required at runtime. */
static uint32_t *fb;
static int pitch;
static int clipLeft = 0, clipTop = 0, clipRight = 480, clipBottom = 272;
static const uint32_t INK = 0x1b1916, PANEL = 0x302a23, EDGE = 0x74624b, GOLD = 0xc5a26d, TEXT = 0xe1d3b8,
                      MUTED = 0xa89a83, TEAL = 0xa6b3a0, RED = 0xc07459;
static uint32_t abgr(uint32_t c) { return 0xff000000 | ((c & 255) << 16) | (c & 0xff00) | ((c >> 16) & 255); }
static uint32_t shade(uint32_t c, float f) {
    int r = (int)((c >> 16) * f), g0 = (int)(((c >> 8) & 255) * f), b = (int)((c & 255) * f);
    if (r > 255)
        r = 255;
    if (g0 > 255)
        g0 = 255;
    if (b > 255)
        b = 255;
    return (r << 16) | (g0 << 8) | b;
}
static uint32_t blend(uint32_t a, uint32_t b, float f) {
    int r = (a >> 16) * (1 - f) + (b >> 16) * f, v = ((a >> 8) & 255) * (1 - f) + ((b >> 8) & 255) * f,
        c = (a & 255) * (1 - f) + (b & 255) * f;
    return (r << 16) | (v << 8) | c;
}
/* Indexed row spans skip transparent margins. Only edge texels need blending.
   A lighting change updates 256 palette entries, not every sprite pixel. */
static uint32_t artPalette[ART_COUNT][256];
static int artLight[ART_COUNT];
static void art(int id, int x, int y, float brightness, int fade) {
    const ArtSprite *s = &artSprites[id];
    int lum = (int)(brightness * 128) + 1;
    if (artLight[id] != lum) {
        for (int i = 0; i < 256; i++) {
            uint32_t c = s->palette[i];
            unsigned r = (c & 255) * (lum - 1) / 128, g0 = ((c >> 8) & 255) * (lum - 1) / 128,
                     b = ((c >> 16) & 255) * (lum - 1) / 128;
            artPalette[id][i] = (c & 0xff000000) | (r > 255 ? 255 : r) | ((g0 > 255 ? 255 : g0) << 8) |
                                ((b > 255 ? 255 : b) << 16);
        }
        artLight[id] = lum;
    }
    x -= s->ax;
    y -= s->ay;
    int top = y < clipTop ? clipTop - y : 0, bottom = y + s->h > clipBottom ? clipBottom - y : s->h;
    for (int yy = top; yy < bottom; yy++) {
        int start = s->rows[yy * 3 + 1], end = s->rows[yy * 3 + 2];
        if (x + start < clipLeft)
            start = clipLeft - x;
        if (x + end > clipRight)
            end = clipRight - x;
        if (start >= end)
            continue;
        const uint8_t *p = s->pixels + s->rows[yy * 3] + start - s->rows[yy * 3 + 1];
        uint32_t *dst = fb + (y + yy) * pitch + x + start;
        for (int xx = start; xx < end; xx++, dst++) {
            uint32_t c = artPalette[id][*p++];
            unsigned a = c >> 24;
            if (fade && ((xx + yy) & 1))
                continue;
            if (a == 255)
                *dst = c;
            else if (a) {
                uint32_t d = *dst;
                uint32_t rb = (((c & 0xff00ff) * a + (d & 0xff00ff) * (256 - a)) >> 8) & 0xff00ff;
                uint32_t gg = (((c & 0xff00) * a + (d & 0xff00) * (256 - a)) >> 8) & 0xff00;
                *dst = 0xff000000 | rb | gg;
            }
        }
    }
}
static unsigned hash(int x, int y) {
    unsigned v = x * 374761393u + y * 668265263u + g.seed;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}
static void px(int x, int y, uint32_t c) {
    if (x >= clipLeft && x < clipRight && y >= clipTop && y < clipBottom)
        fb[y * pitch + x] = abgr(c);
}
static void rect(int x, int y, int w, int h, uint32_t c) {
    if (x < clipLeft) {
        w += x - clipLeft;
        x = clipLeft;
    }
    if (y < clipTop) {
        h += y - clipTop;
        y = clipTop;
    }
    if (x + w > clipRight)
        w = clipRight - x;
    if (y + h > clipBottom)
        h = clipBottom - y;
    if (w <= 0 || h <= 0)
        return;
    uint32_t z = abgr(c);
    for (int yy = y; yy < y + h; yy++) {
        uint32_t *p = fb + yy * pitch + x;
        for (int i = 0; i < w; i++)
            p[i] = z;
    }
}
static void line(int x0, int y0, int x1, int y1, uint32_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (int n = 0; n < 1500; n++) {
        px(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        int e = err * 2;
        if (e >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
static void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t c) {
    if (y0 > y1) {
        int t = y0;
        y0 = y1;
        y1 = t;
        t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y1 > y2) {
        int t = y1;
        y1 = y2;
        y2 = t;
        t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y0 > y1) {
        int t = y0;
        y0 = y1;
        y1 = t;
        t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y2 < clipTop || y0 >= clipBottom || y2 == y0)
        return;
    int from = y0 < clipTop ? clipTop : y0, to = y2 >= clipBottom ? clipBottom - 1 : y2;
    for (int y = from; y <= to; y++) {
        int a = x0 + (int)((float)(x2 - x0) * (y - y0) / (y2 - y0));
        int b = y < y1 ? (y1 == y0 ? x1 : x0 + (int)((float)(x1 - x0) * (y - y0) / (y1 - y0)))
                       : (y2 == y1 ? x2 : x1 + (int)((float)(x2 - x1) * (y - y1) / (y2 - y1)));
        if (a > b) {
            int t = a;
            a = b;
            b = t;
        }
        rect(a, y, b - a + 1, 1, c);
    }
}
static void quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t c) {
    triangle(x0, y0, x1, y1, x2, y2, c);
    triangle(x0, y0, x2, y2, x3, y3, c);
}
static void diamond(int x, int y, int w, int h, uint32_t c) {
    quad(x, y - h, x + w, y, x, y + h, x - w, y, c);
}
static void ellipse(int x, int y, int w, int h, uint32_t c) {
    for (int yy = -h; yy <= h; yy++) {
        int span = (int)(w * sqrtf(fmaxf(0, 1 - (float)(yy * yy) / (h * h))));
        rect(x - span, y + yy, span * 2 + 1, 1, c);
    }
}
static void frame(int x, int y, int w, int h, uint32_t c) {
    rect(x, y, w, 1, c);
    rect(x, y + h - 1, w, 1, c);
    rect(x, y, 1, h, c);
    rect(x + w - 1, y, 1, h, c);
}
/* 5 by 7 glyphs, column-major. Own compact UI font using conventional forms. */
static const uint8_t font[64][5] = {
    {0, 0, 0, 0, 0},       {0, 0, 95, 0, 0},       {0, 7, 0, 7, 0},       {20, 127, 20, 127, 20},
    {36, 42, 127, 42, 18}, {35, 19, 8, 100, 98},   {54, 73, 85, 34, 80},  {0, 5, 3, 0, 0},
    {0, 28, 34, 65, 0},    {0, 65, 34, 28, 0},     {20, 8, 62, 8, 20},    {8, 8, 62, 8, 8},
    {0, 80, 48, 0, 0},     {8, 8, 8, 8, 8},        {0, 96, 96, 0, 0},     {32, 16, 8, 4, 2},
    {62, 81, 73, 69, 62},  {0, 66, 127, 64, 0},    {66, 97, 81, 73, 70},  {33, 65, 69, 75, 49},
    {24, 20, 18, 127, 16}, {39, 69, 69, 69, 57},   {60, 74, 73, 73, 48},  {1, 113, 9, 5, 3},
    {54, 73, 73, 73, 54},  {6, 73, 73, 41, 30},    {0, 54, 54, 0, 0},     {0, 86, 54, 0, 0},
    {8, 20, 34, 65, 0},    {20, 20, 20, 20, 20},   {0, 65, 34, 20, 8},    {2, 1, 81, 9, 6},
    {50, 73, 121, 65, 62}, {126, 17, 17, 17, 126}, {127, 73, 73, 73, 54}, {62, 65, 65, 65, 34},
    {127, 65, 65, 34, 28}, {127, 73, 73, 73, 65},  {127, 9, 9, 9, 1},     {62, 65, 73, 73, 122},
    {127, 8, 8, 8, 127},   {0, 65, 127, 65, 0},    {32, 64, 65, 63, 1},   {127, 8, 20, 34, 65},
    {127, 64, 64, 64, 64}, {127, 2, 12, 2, 127},   {127, 4, 8, 16, 127},  {62, 65, 65, 65, 62},
    {127, 9, 9, 9, 6},     {62, 65, 81, 33, 94},   {127, 9, 25, 41, 70},  {70, 73, 73, 73, 49},
    {1, 1, 127, 1, 1},     {63, 64, 64, 64, 63},   {31, 32, 64, 32, 31},  {63, 64, 56, 64, 63},
    {99, 20, 8, 20, 99},   {7, 8, 112, 8, 7},      {97, 81, 73, 69, 67},  {0, 127, 65, 65, 0},
    {2, 4, 8, 16, 32},     {0, 65, 65, 127, 0},    {4, 2, 1, 2, 4},       {64, 64, 64, 64, 64}};
/* PSP controls: shape glyphs and keycaps, shared by all prompts and guides.
   1 cross,2 circle,3 square,4 triangle,5/6 shoulders,7 start,8 select,
   9 d-pad,12 horizontal arrows,13 vertical arrows. */
static int glyph_width(unsigned k) { return k < 16 ? (k == 7 ? 24 : k == 8 ? 28 : 10) : 6; }
static int text_width(const char *s) {
    int n = 0;
    for (; *s; s++)
        n += glyph_width((unsigned char)*s);
    return n;
}
static void button_icon(int x, int y, unsigned k, int sc, uint32_t color) {
#define BX(a) (x + (a) * sc)
#define BY(a) (y + (a) * sc)
#define BL(a, b, c, d, col) line(BX(a), BY(b), BX(c), BY(d), col)
    uint32_t c = k == 1 ? 0xa8bfca : k == 2 ? 0xcd8b78 : k == 3 ? 0xbfa3ba : k == 4 ? 0xa6b99b : color;
    if (k == 1) {
        BL(1, 0, 7, 6, c);
        BL(7, 0, 1, 6, c);
    } else if (k == 2) {
        BL(2, 0, 6, 0, c);
        BL(2, 6, 6, 6, c);
        BL(1, 1, 1, 5, c);
        BL(7, 1, 7, 5, c);
    } else if (k == 3)
        frame(x + sc, y, 7 * sc, 7 * sc, c);
    else if (k == 4) {
        BL(4, 0, 0, 6, c);
        BL(0, 6, 8, 6, c);
        BL(8, 6, 4, 0, c);
    } else if (k == 5 || k == 6) {
        frame(x, y - sc, 9 * sc, 9 * sc, c);
        int v = k == 5 ? 'L' : 'R';
        for (int i = 0; i < 5; i++)
            for (int yy = 0; yy < 7; yy++)
                if (font[v - 32][i] & (1 << yy))
                    rect(BX(i + 2), BY(yy), sc, sc, c);
    } else if (k == 7 || k == 8) {
        /* Full labels inside the long physical keycaps; no ST/SE abbreviations. */
        frame(x, y - sc, (k == 7 ? 23 : 27) * sc, 9 * sc, c);
        const char *label = k == 7 ? "START" : "SELECT";
        static const uint16_t tiny[] = {0x5bea, 0x5aeb, 0x72cf, 0x2497, 0x7249, 0x724f, 0x79cf};
        const char *letters = "ARETLCS";
        for (int j = 0; label[j]; j++) {
            const char *found = strchr(letters, label[j]);
            uint16_t bits = found ? tiny[found - letters] : 0;
            for (int yy = 0; yy < 5; yy++)
                for (int xx = 0; xx < 3; xx++)
                    if (bits & (1 << (yy * 3 + xx)))
                        rect(BX(2 + j * 4 + xx), BY(1 + yy), sc, sc, c);
        }
    } else if (k == 12) {
        BL(0, 3, 8, 3, c);
        BL(0, 3, 3, 0, c);
        BL(0, 3, 3, 6, c);
        BL(8, 3, 5, 0, c);
        BL(8, 3, 5, 6, c);
    } else if (k == 13) {
        BL(4, 0, 4, 6, c);
        BL(4, 0, 1, 3, c);
        BL(4, 0, 7, 3, c);
        BL(4, 6, 1, 3, c);
        BL(4, 6, 7, 3, c);
    } else {
        rect(BX(3), BY(0), 3 * sc, 7 * sc, c);
        rect(BX(1), BY(2), 7 * sc, 3 * sc, c);
        rect(BX(4), BY(3), sc, sc, INK);
    }
#undef BX
#undef BY
#undef BL
}
static void text(int x, int y, const char *s, uint32_t c, int scale) {
    for (; *s; s++) {
        unsigned k = (unsigned char)*s;
        if (k < 16)
            button_icon(x, y, k, scale, c);
        else {
            k = toupper(k);
            if (k < 32 || k > 95)
                k = '?';
            for (int i = 0; i < 5; i++)
                for (int yy = 0; yy < 7; yy++)
                    if (font[k - 32][i] & (1 << yy))
                        rect(x + i * scale, y + yy * scale, scale, scale, c);
        }
        x += glyph_width((unsigned char)*s) * scale;
    }
}
static void center(int x, int y, const char *s, uint32_t c, int z) {
    text(x - text_width(s) * z / 2, y, s, c, z);
}
static void wrap(int x, int y, int cols, const char *s, uint32_t c) {
    while (*s) {
        char b[128];
        int n = 0, width = 0, lastSpace = -1;
        while (s[n] && s[n] != '\n' && n < 126 && width + glyph_width((unsigned char)s[n]) <= cols * 6) {
            b[n] = s[n];
            if (s[n] == ' ')
                lastSpace = n;
            width += glyph_width((unsigned char)s[n]);
            n++;
        }
        if (s[n] && s[n] != '\n' && lastSpace > 0)
            n = lastSpace;
        if (!n && *s != '\n')
            n = 1;
        memcpy(b, s, n);
        b[n] = 0;
        text(x, y, b, c, 1);
        s += n;
        y += 11;
        while (*s == ' ')
            s++;
        if (*s == '\n') {
            s++;
            y += 4;
        }
    }
}
static void panel(int x, int y, int w, int h) {
    rect(x + 2, y + 3, w, h, 0x141a1b);
    rect(x, y, w, h, PANEL);
    for (int yy = y + 3; yy < y + h - 2; yy += 4)
        line(x + 2, yy, x + w - 3, yy, 0x332d25);
    frame(x, y, w, h, EDGE);
    line(x + 1, y + 1, x + w - 2, y + 1, 0x93927b);
}
static void bar(int x, int y, int w, int h, float value, uint32_t c) {
    rect(x, y, w, h, 0x111a1c);
    int v = (int)(fmaxf(0, fminf(100, value)) * w / 100);
    rect(x, y, v, h, c);
}
static int projectionX, projectionY;
static void project(float x, float y, int *sx, int *sy) {
    *sx = 240 + projectionX + (int)((x - y - ui.camX + ui.camY) * 22);
    *sy = 126 + projectionY + (int)((x + y - ui.camX - ui.camY) * 11) - terrain_height(x, y);
}
static uint32_t ground(int b) {
    static const uint32_t p[] = {0x998063, 0x98664b, 0xb1a58a, 0x646b50, 0x65625d,
                                 0x777663, 0xb2b8ad, 0x53614a, 0x725848};
    return p[b];
}
static float light(void) {
    return g.dayTime >= 7 && g.dayTime < 18    ? 1
           : g.dayTime >= 5 && g.dayTime < 7   ? .83f
           : g.dayTime >= 18 && g.dayTime < 20 ? .8f
                                               : .55f;
}
static int road(float x, float y) { return terrain_road(x, y); }
static void terrain_raw(void) {
    float li = light();
    rect(0, 0, clipRight, clipBottom, shade(ground(biome(ui.camX, ui.camY)), li));
    int cx = (int)ui.camX, cy = (int)ui.camY;
    for (int y = cy - 18; y <= cy + 18; y++)
        for (int x = cx - 18; x <= cx + 18; x++) {
            int sx, sy;
            project(x + .5f, y + .5f, &sx, &sy);
            if (sx < -24 || sx > clipRight + 24 || sy < -12 || sy > clipBottom + 12)
                continue;
            unsigned h = hash(x, y);
            int t = town_near(x, y, 7);
            uint32_t c = ground(biome(x, y));
            if (g.weather == 2 || g.weather == 3)
                c = shade(c, .85f);
            c = shade(c, (.985f + (h % 5) * .005f) * li);
            if (x < 1 || y < 1 || x > WORLD - 2 || y > WORLD - 2)
                c = shade(0x5c6867, li);
            else if (t >= 0)
                c = shade(blend(c, 0xd0bb90, .18f), li);
            else if (road(x, y))
                c = shade(blend(c, 0xd4bd8e, .23f), li);
            if (terrain_water(x, y)) {
                c = shade(0x485f5b, li);
                diamond(sx, sy, 22, 11, c);
                int shift = (x + y) % 5;
                line(sx - 9 + shift, sy - 3, sx + 6 + shift, sy - 3, shade(c, 1.25f));
                line(sx - 8, sy + 3, sx + 8, sy + 3, shade(c, .8f));
                continue;
            }
            diamond(sx, sy, 22, 11, c);
            for (int grain = 0; grain < 35; grain++) {
                unsigned z = hash(x + grain * 9, y - grain * 7);
                int gx = (int)(z % 31) - 15, gy = (int)((z >> 8) % 13) - 6;
                if (abs(gx) + abs(gy) * 2 < 22) {
                    uint32_t fleck = shade(c, .85f + (z % 5) * .06f);
                    line(sx + gx, sy + gy, sx + gx + 1 + (z % 2), sy + gy, fleck);
                }
            }
            if (h % 5 == 0) {
                line(sx - 13, sy + 1, sx - 5, sy + 3, shade(c, .87f));
                line(sx - 5, sy + 3, sx, sy + 2, shade(c, .9f));
            }
            if (h % 3 == 0) {
                px(sx + 6, sy - 3, shade(c, .7f));
                px(sx - 5, sy + 1, shade(c, 1.15f));
            }
            if (t >= 0 && h % 3 == 1) {
                line(sx - 18, sy, sx - 2, sy + 8, shade(c, .92f));
            }
        }
    const ArtSprite *tex = &artSprites[107];
    for (int yy = 0; yy < clipBottom; yy++) {
        float u = (ui.camX + (yy - 126 - projectionY) / 22.f + (-240 - projectionX) / 44.f) * 8;
        float v = (ui.camY + (yy - 126 - projectionY) / 22.f - (-240 - projectionX) / 44.f) * 8;
        for (int xx = 0; xx < clipRight; xx++, u += 8 / 44.f, v -= 8 / 44.f) {
            int tx = ((int)u) & 63, ty = ((int)v) & 63;
            uint32_t c = tex->palette[tex->pixels[tex->rows[ty * 3] + tx]];
            unsigned m = 222 + ((c & 255) + ((c >> 8) & 255) + ((c >> 16) & 255)) / 14;
            uint32_t d = fb[yy * pitch + xx];
            fb[yy * pitch + xx] =
                0xff000000 | (((d & 0xff00ff) * m >> 8) & 0xff00ff) | (((d & 0xff00) * m >> 8) & 0xff00);
        }
    }
}
/* 32-pixel overscan: scrolling reuses terrain until the camera crosses a
   cache margin. Buildings/actors/weather remain live and depth-sorted. */
static uint32_t groundCache[544 * 336];
static void terrain(void) {
    static float cx, cy, lastLight;
    static uint32_t seed;
    static int valid, weather;
    int dx = (int)((cx - cy - ui.camX + ui.camY) * 22), dy = (int)((cx + cy - ui.camX - ui.camY) * 11);
    if (!valid || abs(dx) > 30 || abs(dy) > 30 || seed != g.seed || weather != g.weather ||
        light() != lastLight) {
        uint32_t *old = fb;
        int oldPitch = pitch;
        fb = groundCache;
        pitch = 544;
        clipRight = 544;
        clipBottom = 336;
        projectionX = projectionY = 32;
        terrain_raw();
        projectionX = projectionY = 0;
        clipRight = 480;
        clipBottom = 272;
        fb = old;
        pitch = oldPitch;
        cx = ui.camX;
        cy = ui.camY;
        seed = g.seed;
        weather = g.weather;
        lastLight = light();
        valid = 1;
        dx = dy = 0;
    }
    for (int y = 0; y < 272; y++)
        memcpy(fb + y * pitch, groundCache + (y + 32 - dy) * 544 + 32 - dx, 480 * 4);
}
static int building_sprite(int id) {
    Building *b = &g.buildings[id];
    if (b->type == HOUSE)
        return b->faction == HOSTILE ? 5 : b->faction == SHOGUN ? 4 : id % 3 == 0 ? 7 : 0;
    static const int ids[] = {0, 1, 2, 3, 24, 25, 27, 26, 28, 6, 29, 30, 7};
    return ids[b->type];
}
static void art_shadow(int id, int x, int y) {
    if (id < 0 || id >= 32)
        return;
    const ArtShadow *s = &artShadows[id];
    x -= s->ax;
    y -= s->ay;
    for (int yy = 0; yy < s->h; yy++) {
        if (y + yy < clipTop || y + yy >= clipBottom)
            continue;
        int lo = s->rows[yy * 3 + 1], hi = s->rows[yy * 3 + 2];
        for (int xx = lo; xx < hi; xx++) {
            if (x + xx < clipLeft || x + xx >= clipRight)
                continue;
            unsigned alpha = s->pixels[s->rows[yy * 3] + xx - lo];
            if (!alpha)
                continue;
            uint32_t *dst = fb + (y + yy) * pitch + x + xx, c = *dst;
            unsigned m = 256 - alpha;
            *dst = 0xff000000 | (((c & 0xff00ff) * m >> 8) & 0xff00ff) | (((c & 0xff00) * m >> 8) & 0xff00);
        }
    }
}
static void building(int id) {
    Building *b = &g.buildings[id];
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -120 || x > 600 || y < -80 || y > 310)
        return;
    int sprite = building_sprite(id), ax, ay;
    project(g.actors[g.selected].x, g.actors[g.selected].y, &ax, &ay);
    int fade = building_has_interior(b->type) && abs(ax - x) < artSprites[sprite].w / 2 && ay < y + 16 &&
               ay > y - artSprites[sprite].ay;
    art(sprite, x, y, light(), fade);
    if (b->type == CAMPFIRE) {
        int flicker = (int)(g.time * 10) % 3;
        px(x - 2 + flicker, y - 13 - flicker, 0xe5b66a);
    }
    if (b->type == SHOP || b->type == BAR) {
        const char *name = b->type == SHOP ? "TRADING POST" : "THE WAYWARD";
        int w = text_width(name) + 8;
        rect(x - w / 2, y - artSprites[sprite].ay - 10, w, 11, INK);
        center(x, y - artSprites[sprite].ay - 8, name, GOLD, 1);
    }
}
static int resource_sprite(int n) {
    int t = g.nodes[n].type;
    return t <= NODE_COPPER ? 9 : t == NODE_WOOD ? 8 : t == NODE_RUIN ? 15 : 31;
}
static void resource(int n) {
    Node *b = &g.nodes[n];
    if (!b->remaining)
        return;
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -40 || x > 520 || y < -30 || y > 245)
        return;
    int id = resource_sprite(n);
    art(id, x, y + (id < 24 ? 5 : 0), light() * (b->type == NODE_ORE ? .82f : 1), 0);
    if (b->locked) {
        frame(x - 2, y - 5, 4, 4, GOLD);
    }
}
static void thickline(int x, int y, int xx, int yy, int width, uint32_t c) {
    for (int i = -width / 2; i <= width / 2; i++)
        line(x + i, y, xx + i, yy, c);
}
/* Equipment is layered onto the same animated body; no per-outfit sprite sheet. */
static void weapon_sprite(int id, int x, int y, int sc, float angle, float li) {
    if (!id)
        return;
    ItemDef *d = &itemDefs[id];
    int f = gear_family(id), q = d->quality;
    if (f < 0)
        f = id == STAFF ? 16 : id == CROSSBOW ? 25 : id == CLEAVER ? 11 : id == SABRE ? 8 : 0;
    float ux = cosf(angle) * sc, uy = sinf(angle) * sc, vx = -uy, vy = ux;
    uint32_t c = shade(d->color, li), edge = shade(d->color, 1.4f * li), grip = shade(0x46372c, li),
             trim = shade(d->trim, li);
#define WX(a, b) (x + (int)(ux * (a) + vx * (b)))
#define WY(a, b) (y + (int)(uy * (a) + vy * (b)))
#define WL(a, b, c0, d0, col) thickline(WX(a, b), WY(a, b), WX(c0, d0), WY(c0, d0), sc, col)
#define WQ(a, b, c0, d0, e, f0, g0, h, col)                                                                  \
    quad(WX(a, b), WY(a, b), WX(c0, d0), WY(c0, d0), WX(e, f0), WY(e, f0), WX(g0, h), WY(g0, h), col)
    int len = 16 + (f % 4) * 2;
    if (f == 1 || f == 3 || f == 9)
        len = 11 + f % 3;
    if (f == 2 || f == 6 || f == 20)
        len = 27;
    if (f == 4 || f == 5 || f == 6 || f == 16) {
        WL(-12, 0, 24, 0, grip);
        WL(-11, 1, 23, 1, c);
        if (f == 4) {
            WQ(14, -1, 29, -3, 26, 2, 15, 3, c);
            WL(17, 3, 29, -3, edge);
        } else if (f == 5) {
            WQ(20, -3, 31, 0, 20, 3, 23, 0, c);
            WL(22, -2, 31, 0, edge);
        } else if (f == 6) {
            WQ(9, -1, 29, -2, 24, 3, 10, 3, c);
            WL(11, 3, 29, -2, edge);
        }
        for (int j = 0; j < q + 1; j++)
            WL(8 + j * 2, -1, 8 + j * 2, 2, trim);
    } else if (f >= 29) {
        len = f == 30 ? 12 : f == 31 ? 20 : 27;
        WQ(-6, -2, 3, -2, 5, 2, -5, 4, grip);
        WQ(0, -2, len, -2, len, 0, 0, 1, c);
        WL(5, -2, len, -2, edge);
        WL(3, 0, 3, 5, grip);
        WL(-3, -1, 1, -1, trim);
        if (f == 29) {
            WL(11, -4, 17, -4, c);
            WL(12, -3, 12, -1, grip);
        }
        if (f == 31)
            WL(5, 1, len, 1, c);
    } else if (f >= 24) {
        WL(-5, 0, 13, 0, grip);
        WL(0, -11 - (f % 3), 6, 0, c);
        WL(6, 0, 0, 11 + (f % 3), c);
        WL(0, -11 - (f % 3), -2, 0, trim);
        WL(-2, 0, 0, 11 + (f % 3), trim);
        WL(0, 0, 16, 0, edge);
        if (f == 26)
            WL(1, -10, 7, 0, c);
        if (f == 27) {
            WL(-4, -2, 5, -2, c);
            WL(-4, -2, -5, 4, grip);
        }
        if (f == 28) {
            WL(0, -17, 6, 0, c);
            WL(6, 0, 0, 17, c);
            WL(0, -17, -2, 0, trim);
            WL(-2, 0, 0, 17, trim);
        }
    } else if (f == 12 || f == 13 || f == 14 || f == 15 || f == 23) {
        WL(-5, 0, 21, 0, grip);
        if (f == 12) {
            WQ(15, -2, 20, -9, 21, 6, 18, 2, c);
            WL(20, -9, 21, 6, edge);
        } else if (f == 13) {
            WQ(14, 0, 17, -8, 24, -7, 21, 4, c);
            WL(17, -8, 24, -7, edge);
        } else {
            int start = f == 14 ? 7 : f == 15 ? 4 : 12, wide = f == 14 ? 3 : f == 15 ? 4 : 2;
            WQ(start, -wide, 21, -wide, 23, 1, start, wide, c);
            for (int j = start + 2; j < 22; j += 4) {
                WL(j, -wide, j, -wide - 2, trim);
                WL(j, wide, j, wide + 2, trim);
            }
        }
    } else {
        int bend = (f == 8 || f == 18 || f == 19 || f == 0 || f == 2) ? 3 : 0,
            width = f == 11 || f == 20 ? 4 : 2;
        WQ(3, -1, len - 3, -1 - bend, len, -bend, 4, width, c);
        WL(4, width, len, -bend, edge);
        WL(-5, 0, 3, 0, grip);
        WL(3, -3, 3, 4, trim);
        if (f == 8) {
            WL(-4, 3, 3, 4, trim);
            WL(-5, 0, -4, 3, trim);
        }
        if (f == 9) {
            WQ(7, -1, len, -3, len - 2, 1, 7, 2, c);
            WL(7, -1, len, -3, edge);
        }
        if (f == 7 || f == 17 || f == 22) {
            WL(len - 2, -2, len - 2, -8, c);
            WL(len - 2, -8, len - 7, -7, edge);
        }
        if (f == 21)
            for (int j = 5; j < len - 2; j += 3)
                WL(j, 2, j + 1, 4, edge);
        if (f == 18)
            for (int j = 7; j < len - 3; j += 5)
                ellipse(WX(j, -2), WY(j, -2), sc, sc, trim);
        if (f == 7) {
            for (int j = 0; j < 7; j++)
                ellipse(WX(-6 - j, -j % 2), WY(-6 - j, -j % 2), sc, sc, trim);
        }
    }
    for (int j = 0; j <= q; j++)
        WL(-3 + j * 2, -1, -3 + j * 2, 1, trim);
#undef WX
#undef WY
#undef WL
#undef WQ
}
#include "painted_actor.h"
typedef struct {
    float depth;
    int kind, index, x, y;
} Object;
static Object objects[MAX_NODES + MAX_BUILDINGS + MAX_ACTORS + 1200];
static int compare_obj(const void *a, const void *b) {
    float d = ((Object *)a)->depth - ((Object *)b)->depth;
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}
static void objects_draw(void) {
    int n = 0;
    for (int i = 0; i < g.nodeCount; i++) {
        Node *b = &g.nodes[i];
        if (fabsf(b->x - ui.camX) < 20 && fabsf(b->y - ui.camY) < 20)
            objects[n++] = (Object){b->x + b->y, 0, i, 0, 0};
    }
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (fabsf(b->x - ui.camX) < 20 && fabsf(b->y - ui.camY) < 20)
            objects[n++] = (Object){b->x + b->y, 1, i, 0, 0};
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->carriedBy >= 0)
            continue;
        int x, y;
        project(a->x, a->y, &x, &y);
        if (x > -30 && x < 510 && y > -20 && y < 250)
            objects[n++] = (Object){a->x + a->y, 2, i, x, y};
    }
    int cx = ui.camX, cy = ui.camY;
    for (int y = cy - 16; y < cy + 16; y++)
        for (int x = cx - 16; x < cx + 16; x++) {
            unsigned h = hash(x, y);
            if (h % 13 || town_near(x, y, 8) >= 0 || road(x, y))
                continue;
            int sx, sy;
            project(x + .3f, y + .4f, &sx, &sy);
            if (sx < 0 || sx > 480 || sy < 0 || sy > 210)
                continue;
            objects[n++] = (Object){x + y + .7f, 3, biome(x, y) * 4 + (int)(h % 4), sx, sy};
        }
    for (int i = 0; i < n; i++) {
        Object *o = &objects[i];
        int x = o->x, y = o->y;
        if (o->kind == 1) {
            Building *b = &g.buildings[o->index];
            project(b->x, b->y, &x, &y);
            art_shadow(building_sprite(o->index), x, y);
        }
        if (o->kind == 0 && g.nodes[o->index].remaining) {
            Node *b = &g.nodes[o->index];
            project(b->x, b->y, &x, &y);
            int id = resource_sprite(o->index);
            art_shadow(id, x, y + (id < 24 ? 5 : 0));
        }
        if (o->kind == 3) {
            int region = o->index / 4, variant = o->index % 4;
            int id = region == 7                    ? 10
                     : region == 3                  ? 13
                     : region == 6                  ? 14
                     : (region == 8 || region == 5) ? 11
                     : variant < 2                  ? 9
                                                    : 12;
            art_shadow(id, x, y + 4);
        }
    }
    if (ui.contextIndex >= 0 && !g.interiorBuilding) {
        int x, y;
        float wx, wy;
        int id = ui.contextIndex;
        if (ui.contextKind == 0) {
            wx = g.actors[id].x;
            wy = g.actors[id].y;
        } else if (ui.contextKind == 1) {
            wx = g.nodes[id].x;
            wy = g.nodes[id].y;
        } else {
            wx = g.buildings[id].x + (ui.contextKind == 3 ? 1.45f : 0);
            wy = g.buildings[id].y + (ui.contextKind == 3 ? .65f : 0);
        }
        project(wx, wy, &x, &y);
        uint32_t color = context_distance() <= context_reach() ? TEAL : MUTED;
        line(x - 12, y, x, y + 6, color);
        line(x, y + 6, x + 12, y, color);
    }
    qsort(objects, n, sizeof(Object), compare_obj);
    for (int i = 0; i < n; i++) {
        Object *o = &objects[i];
        if (o->kind == 0)
            resource(o->index);
        else if (o->kind == 1)
            building(o->index);
        else if (o->kind == 2) {
            Actor *a = &g.actors[o->index];
            person(a, o->x, o->y, 1, o->index == g.selected);
            if (a->target >= 0 || a->unconscious > 0 || a->squad) {
                float hp = (a->health[HEAD] + a->health[CHEST]) * .5f;
                bar(o->x - 10, o->y - 43, 21, 2, hp, a->squad ? TEAL : RED);
            }
            if (!a->squad && a->kind != BEAST && a->kind != RAIDER && ui.contextKind == 0 &&
                ui.contextIndex == o->index) {
                center(o->x, o->y - 52, npc_role_name(a), a->kind == RECRUIT ? TEAL : GOLD, 1);
            }
            if (a->squad && o->index == g.selected) {
                triangle(o->x - 3, o->y - 49, o->x + 3, o->y - 49, o->x, o->y - 45, TEAL);
            }
        } else {
            int x = o->x, y = o->y;
            int region = o->index / 4, variant = o->index % 4;
            int sprite = region == 7                    ? 10
                         : region == 3                  ? 13
                         : region == 6                  ? 14
                         : (region == 8 || region == 5) ? 11
                         : variant < 2                  ? 9
                                                        : 12;
            art(sprite, x, y + 4, light(), 0);
        }
    }
}
static void speech_bubbles(void) {
    int best = -1;
    float near = 7;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead || a->squad || a->speechTime <= 0)
            continue;
        float d = distance(a->x, a->y, ui.camX, ui.camY);
        if (d < near) {
            best = i;
            near = d;
        }
    }
    if (best < 0)
        return;
    Actor *a = &g.actors[best];
    int x, y;
    project(a->x, a->y, &x, &y);
    const char *words = ambient_line(best);
    int w = strlen(words) * 6 + 12;
    if (w > 260)
        w = 260;
    x -= w / 2;
    if (x < 8)
        x = 8;
    if (x + w > 391)
        x = 391 - w;
    y -= 64;
    if (y < 30)
        y = 30;
    if (y > 159)
        y = 159;
    rect(x, y, w, 23, INK);
    frame(x, y, w, 23, EDGE);
    triangle(x + w / 2 - 3, y + 22, x + w / 2 + 3, y + 22, x + w / 2, y + 27, EDGE);
    wrap(x + 6, y + 5, (w - 12) / 6, words, TEXT);
}

static void texture_plane(int id, float x, float y, float ux, float uy, float vx, float vy, float repeatU,
                          float repeatV, float li) {
    const ArtSprite *s = &artSprites[id];
    float det = ux * vy - uy * vx;
    if (fabsf(det) < .1f)
        return;
    int left = floorf(fminf(fminf(x, x + ux), fminf(x + vx, x + ux + vx))),
        right = ceilf(fmaxf(fmaxf(x, x + ux), fmaxf(x + vx, x + ux + vx)));
    int top = floorf(fminf(fminf(y, y + uy), fminf(y + vy, y + uy + vy))),
        bottom = ceilf(fmaxf(fmaxf(y, y + uy), fmaxf(y + vy, y + uy + vy)));
    if (left < clipLeft)
        left = clipLeft;
    if (right > clipRight)
        right = clipRight;
    if (top < clipTop)
        top = clipTop;
    if (bottom > clipBottom)
        bottom = clipBottom;
    part_palette(id, 0x808080, li, 0);
    for (int yy = top; yy < bottom; yy++)
        for (int xx = left; xx < right; xx++) {
            float dx = xx + .5f - x, dy = yy + .5f - y, u = (dx * vy - dy * vx) / det,
                  v = (dy * ux - dx * uy) / det;
            if (u < 0 || v < 0 || u >= 1 || v >= 1)
                continue;
            int sx = (int)(u * repeatU * s->w) % s->w, sy = (int)(v * repeatV * s->h) % s->h;
            int lo = s->rows[sy * 3 + 1];
            if (sx < lo || sx >= s->rows[sy * 3 + 2])
                continue;
            fb[yy * pitch + xx] = artPalette[id][s->pixels[s->rows[sy * 3] + sx - lo]];
        }
}
static void room_project(float x, float y, int *sx, int *sy) {
    *sx = 240 + (int)((x - y) * 22);
    *sy = 61 + (int)((x + y) * 11);
}
static void interior_draw(void) {
    int id = g.interiorBuilding - 1;
    Building *b = &g.buildings[id];
    rect(0, 0, 480, 272, 0x211d18);
    texture_plane(105, 86, 138, 154, -77, 0, -44, 3, 1.2f, .92f);
    texture_plane(105, 240, 61, 154, 77, 0, -44, 3, 1.2f, .68f);
    texture_plane(104, 240, 61, 154, 77, -154, 77, 2, 3, .96f);
    for (int j = 0; j <= 7; j++) {
        int x = 86 + j * 22, y = 138 - j * 11;
        texture_plane(106, x, y, 0, -44, 3, -1.5f, 1, 1, .8f);
        x = 240 + j * 22;
        y = 61 + j * 11;
        texture_plane(106, x, y, 0, -44, 3, 1.5f, 1, 1, .62f);
    }
    line(86, 94, 240, 17, 0x51402f);
    line(240, 17, 394, 94, 0x382f25);
    /* Door threshold remains visible on the removed front wall. */
    int x, y;
    room_project(3.5f, 6.8f, &x, &y);
    diamond(x, y, 22, 10, 0xb3986b);
    line(x - 22, y, x, y + 10, GOLD);
    text(x - 15, y + 12, "EXIT", GOLD, 1);
    room_project(3.8f, 3.6f, &x, &y);
    art(23, x, y + 10, .8f, 0);
    InteriorProp props[8];
    int count = interior_props(id, props), order[10];
    float depth[10];
    for (int i = 0; i < count; i++) {
        order[i] = i;
        depth[i] = props[i].x + props[i].y;
    }
    int player = count;
    order[count] = count;
    depth[count++] = g.interiorX + g.interiorY;
    int sleepProp = -1;
    if (g.actors[g.selected].taskKind == 3)
        for (int i = 0; i < player; i++)
            if (props[i].action == ROOM_REST) {
                sleepProp = i;
                depth[player] = props[i].x + props[i].y + .01f;
                break;
            }
    int keeper = -1;
    if (b->type == SHOP || b->type == BAR) {
        keeper = count;
        order[count] = count;
        depth[count++] = b->type == SHOP ? 3.8f : 4.1f;
    }
    for (int i = 1; i < count; i++) {
        int a = order[i], j = i;
        while (j > 0 && depth[order[j - 1]] > depth[a]) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = a;
    }
    for (int k = 0; k < count; k++) {
        int i = order[k];
        if (i == player) {
            if (sleepProp >= 0) {
                room_project(props[sleepProp].x, props[sleepProp].y, &x, &y);
                y -= 12;
            } else
                room_project(g.interiorX, g.interiorY, &x, &y);
            person(&g.actors[g.selected], x, y, 1, sleepProp < 0);
        } else if (i == keeper) {
            Actor resident = g.actors[g.selected];
            resident.attackPhase = resident.movedTimer = resident.hitFlash = 0;
            resident.gear[0] = 0;
            resident.gear[1] = gear_id(35, 0, 1);
            resident.gear[2] = HAT;
            resident.gear[5] = 0;
            resident.skin = 3;
            resident.sneaking = 0;
            resident.taskKind = 0;
            resident.dead = 0;
            resident.unconscious = 0;
            room_project(b->type == SHOP ? 3.f : 4.f, b->type == SHOP ? .8f : .4f, &x, &y);
            person(&resident, x, y, 1, 0);
        } else {
            room_project(props[i].x, props[i].y, &x, &y);
            art(props[i].art, x, y + 5, 1, 0);
        }
    }
    const char *name = b->type == SHOP    ? "TRADING POST / INTERIOR"
                       : b->type == BAR   ? "THE WAYWARD / ROOMS"
                       : b->type == FORGE ? "SMITHY / INTERIOR"
                       : b->type == STORE ? "DEPOT / INTERIOR"
                       : b->type == BENCH ? "WORKSHOP / INTERIOR"
                       : b->type == TOWER ? "WATCH HOUSE / INTERIOR"
                                          : "FRONTIER HOME / INTERIOR";
    text(12, 32, name, GOLD, 1);
}
static void mini_map(void) {
    int ox = 402, oy = 31;
    rect(ox - 3, oy - 3, 72, 67, PANEL);
    for (int y = 0; y < 60; y += 3)
        for (int x = 0; x < 66; x += 3) {
            float wx = ui.camX + (x - 33) * .4f, wy = ui.camY + (y - 30) * .4f;
            uint32_t c = ground(biome(wx, wy));
            rect(ox + x, oy + y, 3, 3, shade(c, .55f));
        }
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        int x = ox + 33 + (b->x - ui.camX) * 2.5f, y = oy + 30 + (b->y - ui.camY) * 2.5f;
        if (x >= ox && x < ox + 66 && y >= oy && y < oy + 60)
            rect(x, y, 3, 2, 0xb5aa86);
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        int x = ox + 33 + (a->x - ui.camX) * 2.5f, y = oy + 30 + (a->y - ui.camY) * 2.5f;
        if (x >= ox && x < ox + 66 && y >= oy && y < oy + 60 && a->active && !a->dead)
            rect(x, y, 2, 2, a->squad ? TEAL : a->faction == HOSTILE ? RED : GOLD);
    }
    frame(ox - 3, oy - 3, 72, 67, EDGE);
    text(ox + 28, oy - 1, "N", TEXT, 1);
}
static void hud(void) {
    Actor *a = &g.actors[g.selected];
    char b[120];
    rect(0, 0, 480, 23, INK);
    line(0, 22, 480, 22, EDGE);
    int t = town_near(a->x, a->y, 10);

    text(9, 8, t >= 0 ? towns[t].name : regionNames[biome(a->x, a->y)], GOLD, 1);
    snprintf(b, 120, "DAY %d  %02d:%02d", g.day, (int)g.dayTime, (int)(g.dayTime * 60) % 60);
    text(220, 8, b, MUTED, 1);
    snprintf(b, 120, "%d SILVER", g.money);
    text(397 - strlen(b) * 2, 8, b, TEXT, 1);
    if (!g.interiorBuilding)
        mini_map();
    else {
        panel(408, 30, 64, 60);
        text(414, 36, "INDOORS", GOLD, 1);
        frame(418, 49, 43, 32, EDGE);
        rect(420 + (int)(g.interiorX * 5), 51 + (int)(g.interiorY * 4), 3, 3, TEAL);
    }

    if (ui.paused) {
        panel(192, 31, 94, 20);
        center(239, 38, "TACTICAL PAUSE", GOLD, 1);
    } else if (ui.speed == 2)
        text(338, 31, "2X", GOLD, 1);
    rect(0, 212, 480, 60, INK);
    line(0, 212, 480, 212, EDGE);
    int member = 0;
    for (int i = 0; i < g.selected; i++)
        if (g.actors[i].squad)
            member++;
    int k = 0, shown = 0, page = member / 4;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad) {
            Actor *m = &g.actors[i];
            if (k++ / 4 != page)
                continue;
            int x = 8 + shown++ * 72;
            rect(x, 219, 65, 33, i == g.selected ? 0x494132 : PANEL);
            frame(x, 219, 65, 33, i == g.selected ? TEAL : EDGE);
            Actor portrait = *m;
            portrait.attackPhase = 0;
            portrait.hitFlash = 0;
            portrait.movedTimer = 0;
            clipLeft = x + 1;
            clipTop = 220;
            clipRight = x + 23;
            clipBottom = 251;
            person(&portrait, x + 12, 261, 1, 0);
            clipLeft = clipTop = 0;
            clipRight = 480;
            clipBottom = 272;
            char shortname[7];
            snprintf(shortname, sizeof(shortname), "%.6s", m->name);
            text(x + 24, 224, shortname, i == g.selected ? TEAL : TEXT, 1);
            bar(x + 24, 237, 35, 3, (m->health[HEAD] + m->health[CHEST]) * .5f, m->dead ? RED : TEAL);
            text(x + 24, 244,
                 m->dead                ? "DEAD"
                 : m->unconscious > 0   ? "DOWN"
                 : m->job == JOB_FOLLOW ? "FOLLOW"
                                        : "JOB",
                 MUTED, 1);
        }
    int hx = 307;
    snprintf(b, 120, "FOOD %d", (int)a->hunger);
    text(hx, 220, b, TEXT, 1);
    bar(hx + 65, 220, 47, 5, a->hunger, GOLD);
    snprintf(b, 120, "BLOOD %d", (int)a->blood);
    text(hx, 231, b, a->bleed > 0 ? RED : TEXT, 1);
    bar(hx + 65, 231, 47, 5, a->blood, a->bleed > 0 ? RED : TEAL);
    snprintf(b, 120, "LOAD %.0f / %d", encumbrance(a), 30 + level(a, STRENGTH) * 2 + gear_bonus(a, 3));
    text(hx, 242, b, MUTED, 1);
    if (a->sneaking)
        text(430, 221, "SNEAK", TEAL, 1);
    else
        text(430, 221, a->autoattack ? "FIGHT" : "FLEE", GOLD, 1);
    text(8, 261, "\001 ACT  \004 PACK  \003 FIGHT  \002 SNEAK  \005 SQUAD  \006 PAUSE", MUTED, 1);
    text(352, 261, "\010 MAP \007 MENU", MUTED, 1);
    if (ui.noticeTime > 0) {
        rect(7, 26, 386, 14, PANEL);
        char notice[64];
        snprintf(notice, sizeof(notice), "%.62s", ui.notice);
        text(12, 30, notice, TEXT, 1);
    }
    panel(67, 177, 346, 32);
    if (g.interiorBuilding) {
        center(240, 182, "INDOOR SERVICE", GOLD, 1);
        text(80, 195, "\001", TEAL, 1);
        text(96, 195, interior_prompt(), TEXT, 1);
    } else if (ui.contextIndex >= 0) {
        center(240, 182, context_name(), GOLD, 1);
        float d = context_distance();
        if (d > context_reach()) {
            char action[72];
            snprintf(action, sizeof(action), "MOVE CLOSER  %.1fm / %.1fm", d, context_reach());
            center(240, 195, action, MUTED, 1);
        } else {
            char action[72];
            snprintf(action, sizeof(action), "\001 %s%s", context_action(),
                     a->taskKind == 1 ? "  /  MOVE TO STOP" : "");
            center(240, 195, action, TEXT, 1);
        }
    } else {
        center(240, 182, "FIND SUPPLIES, WORK OR A ROAD OUT", GOLD, 1);
        center(240, 195, "\010 ATLAS   \007 SERVICES & FIELD GUIDE", MUTED, 1);
    }
    if (a->taskKind) {
        int w = 120;
        bar(180, 172, w, 3,
            (1 - fminf(1, a->action / (a->taskKind == 3   ? 20
                                       : a->taskKind == 2 ? 4.5f
                                                          : 2.8f))) *
                100,
            GOLD);
    }
}
static void title(void) {
    ui.camX = 28;
    ui.camY = 27;
    terrain();
    objects_draw();
    for (int y = 0; y < 272; y++)
        for (int x = 0; x < 290; x++) {
            uint32_t c = fb[y * pitch + x];
            uint32_t rgb = ((c & 255) << 16) | (c & 0xff00) | ((c >> 16) & 255);
            fb[y * pitch + x] = abgr(blend(rgb, INK, .84f));
        }
    line(30, 45, 205, 45, EDGE);
    text(29, 61, "DUSTBOUND", GOLD, 3);
    text(31, 93, "A WASTELAND WITHOUT HEROES", TEXT, 1);
    text(31, 112, "SQUAD SURVIVAL RPG / PSP", MUTED, 1);
    const char *opts[] = {"WANDERER - ONE DRIFTER", "CARAVAN - THREE SURVIVORS", "CONTINUE EXPEDITION",
                          "FIELD GUIDE"};
    for (int i = 0; i < 4; i++) {
        int y = 148 + i * 23;
        if (i == ui.selection) {
            rect(25, y - 5, 255, 20, 0x494132);
            text(32, y, ">", TEAL, 1);
        }
        text(47, y, opts[i], i == ui.selection ? TEXT : MUTED, 1);
    }
    text(31, 251, "\001 BEGIN    VERSION " VERSION " / ALPHA", GOLD, 1);
}
static void overlay_header(const char *name, const char *sub) {
    panel(14, 28, 452, 224);
    text(27, 40, name, GOLD, 2);
    text(28, 61, sub, MUTED, 1);
    line(27, 75, 451, 75, EDGE);
    text(28, 237, "\002 CLOSE   \010 GUIDE", MUTED, 1);
    if (ui.screen >= SCREEN_PACK && ui.screen <= SCREEN_SQUAD)
        text(249, 237, "\005 / \006: PACK  SKILLS  BODY  SQUAD", MUTED, 1);
}
static int start_row(int selected, int max, int visible) {
    int first = selected - visible / 2;
    if (first < 0)
        first = 0;
    if (first > max - visible)
        first = max - visible;
    return first < 0 ? 0 : first;
}
static void row(int y, int selected, const char *label, const char *value) {
    if (selected) {
        rect(25, y - 3, 425, 14, 0x4a4030);
        rect(25, y - 3, 2, 14, TEAL);
    }
    text(32, y, label, selected ? TEXT : MUTED, 1);
    if (value)
        text(442 - (int)strlen(value) * 6, y, value, selected ? GOLD : MUTED, 1);
}
static const char *slotNames[GEARS] = {"WEAPON", "BODY", "HEAD", "FEET", "LEGS", "BACK"};
static void clipped(int x, int y, const char *s, int chars, uint32_t c) {
    char b[100];
    snprintf(b, sizeof(b), "%.*s", chars, s);
    text(x, y, b, c, 1);
}
static Actor preview_actor(void) {
    Actor a = g.actors[g.selected];
    a.attackPhase = 0;
    a.hitFlash = 0;
    a.unconscious = 0;
    a.dead = 0;
    a.sneaking = 0;
    a.taskKind = 0;
    a.movedTimer = 0;
    a.carrying = -1;
    a.facing = 1;
    return a;
}
static void compare_number(int x, int y, const char *label, float old, float next, int decimals, int lower) {
    char b[80];
    float delta = next - old;
    if (decimals)
        snprintf(b, sizeof(b), "%s %.1f > %.1f (%+.1f)", label, old, next, delta);
    else
        snprintf(b, sizeof(b), "%s %d > %d (%+d)", label, (int)old, (int)next, (int)delta);
    text(x, y, b, fabsf(delta) < .01f ? MUTED : ((delta > 0) != lower ? TEAL : RED), 1);
}
static void equipment_comparison(int it, int y) {
    Actor *a = &g.actors[g.selected];
    ItemDef *d = &itemDefs[it];
    char b[110];
    if (!it) {
        text(31, y + 12, "SELECT AN ITEM TO INSPECT OR COMPARE.", MUTED, 1);
        return;
    }
    if (d->slot < 0) {
        snprintf(b, sizeof(b), "%s  /  WEIGHT %d", d->name, d->weight);
        clipped(31, y, b, 68, TEXT);
        wrap(31, y + 14, 67, d->desc, MUTED);
        return;
    }
    int worn = a->gear[d->slot];
    ItemDef *old = &itemDefs[worn];
    snprintf(b, sizeof(b), "TRY  %s", d->name);
    clipped(31, y, b, 68, TEXT);
    snprintf(b, sizeof(b), "WORN %s", worn ? old->name : "EMPTY SLOT");
    clipped(31, y + 12, b, 68, MUTED);
    compare_number(31, y + 26, d->slot == 0 ? "DMG" : "ARM", d->slot == 0 ? old->power : old->armor,
                   d->slot == 0 ? d->power : d->armor, 0, 0);
    compare_number(229, y + 26, "WT", old->weight, d->weight, 0, 1);
    if (d->slot == 0) {
        compare_number(31, y + 38, "REACH", worn ? old->range : 1.55f, d->range, 1, 0);
        compare_number(229, y + 38, "DELAY", worn ? old->cooldown : 1.2f, d->cooldown, 1, 1);
    } else {
        compare_number(31, y + 38, "CARRY", old->capacity, d->capacity, 0, 0);
        compare_number(229, y + 38, "SPEED", old->speed, d->speed, 0, 0);
    }
    snprintf(b, sizeof(b), "EFFECT: %s +%d > %s +%d", old->effect < 0 ? "NONE" : effectNames[old->effect],
             old->potency, d->effect < 0 ? "NONE" : effectNames[d->effect], d->potency);
    clipped(31, y + 50, b, 68, GOLD);
}
static void pack_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[100];
    int it = a->pack[ui.selection].item;
    overlay_header("PACK & EQUIPMENT", "\001 EQUIP / USE   \004 WORN GEAR   \003 PASS TO ALLY");
    int first = start_row(ui.selection, PACK, 6);
    for (int j = 0; j < 6; j++) {
        int k = first + j, y = 84 + j * 14, id = a->pack[k].item;
        if (k == ui.selection)
            rect(25, y - 3, 244, 13, 0x4a4030);
        snprintf(b, sizeof(b), "%02d %.32s", k + 1, id ? itemDefs[id].name : "--");
        text(31, y, b, k == ui.selection ? TEXT : MUTED, 1);
        if (id) {
            snprintf(b, sizeof(b), "%d", a->pack[k].qty);
            text(262 - strlen(b) * 6, y, b, GOLD, 1);
        }
    }
    Actor current = preview_actor(), preview = current;
    if (it && itemDefs[it].slot >= 0)
        preview.gear[itemDefs[it].slot] = it;
    person(&current, 310, 160, 2, 0);
    person(&preview, 409, 160, 2, 0);
    text(287, 82, "WORN", MUTED, 1);
    text(386, 82, "TRY ON", GOLD, 1);
    equipment_comparison(it, 173);
}

static void skills_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[80];
    overlay_header("SKILLS", "IMPROVE BY DOING. XP PERSISTS WITH EACH SQUAD MEMBER.");
    for (int i = 0; i < SKILLS; i++) {
        int col = i / 10, r = i % 10, x = 31 + col * 214, y = 84 + r * 14;
        text(x, y, skillNames[i], TEXT, 1);
        int lv = level(a, i);
        snprintf(b, 80, "%02d", lv);
        text(x + 147, y, b, GOLD, 1);
        bar(x + 166, y + 2, 24, 3, lv, TEAL);
    }
    text(31, 226, a->name, TEAL, 1);
}
static void health_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[100];
    overlay_header("BODY & FIRST AID", "\001 USE A FIELD DRESSING. LOST LIMBS NEED PROSTHETICS.");
    person(a, 99, 196, 2, 0);
    for (int i = 0; i < LIMBS; i++) {
        int y = 88 + i * 18;
        text(185, y, limbNames[i], TEXT, 1);
        bar(266, y + 1, 100, 5, a->health[i], a->health[i] < 30 ? RED : TEAL);
        snprintf(b, 100, "%d", (int)a->health[i]);
        text(376, y, b, TEXT, 1);
        if (a->lostMask & (1 << i))
            text(405, y, a->prostheticMask & (1 << i) ? "MECH" : "LOST", GOLD, 1);
    }
    snprintf(b, 100, "BLOOD %d   BLEED %.1f   FATIGUE %d", (int)a->blood, a->bleed, (int)a->fatigue);
    text(185, 205, b, a->bleed ? RED : MUTED, 1);
    text(185, 219,
         a->dead              ? "DEAD"
         : a->unconscious > 0 ? "UNCONSCIOUS - NEEDS RESCUE"
                              : "CONSCIOUS",
         TEAL, 1);
}
static void squad_screen(void) {
    Actor *a = &g.actors[g.selected];
    overlay_header("SQUAD ORDERS", "\014 MEMBER   \001 JOB   \003 DISMISS FALLEN");
    clipped(31, 85, a->name, 16, TEAL);
    char b[60];
    snprintf(b, 60, "%d / 8 SURVIVORS", g.squadCount);
    text(326, 85, b, GOLD, 1);
    for (int i = 0; i < 8; i++)
        row(103 + i * 15, ui.selection == i, jobNames[i], a->job == i ? "ASSIGNED" : "");
}

static void map_screen(void) {
    overlay_header(ui.mapZoom ? "LOCAL ROUTES / FRONTIER" : "WORLD ATLAS / FRONTIER",
                   "\011 CURSOR  \001 TRAVEL  \005/\006 TOWNS  \004 ZOOM  \003 CHART");
    int ox = 29, oy = 82, mw = 256, mh = 136;
    int spanX = ui.mapZoom ? 384 : WORLD, spanY = ui.mapZoom ? 204 : WORLD;
    int left = ui.mapZoom ? (int)ui.mapX - spanX / 2 : 0, top = ui.mapZoom ? (int)ui.mapY - spanY / 2 : 0;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    if (left > WORLD - spanX)
        left = WORLD - spanX;
    if (top > WORLD - spanY)
        top = WORLD - spanY;
#define MAPX(wx) (ox + (int)(((wx) - left) * mw / spanX))
#define MAPY(wy) (oy + (int)(((wy) - top) * mh / spanY))
#define INMAP(wx, wy) ((wx) >= left && (wx) < left + spanX && (wy) >= top && (wy) < top + spanY)
    static uint32_t atlas[256 * 136], cachedHash;
    static int ready;
    uint32_t h = g.seed;
    h = (h ^ (unsigned)left) * 16777619u;
    h = (h ^ (unsigned)top) * 16777619u;
    h = (h ^ (unsigned)spanX) * 16777619u;
    for (unsigned i = 0; i < sizeof(g.seen); i++)
        h = (h ^ g.seen[i]) * 16777619u;
    if (!ready || h != cachedHash) {
        for (int y = 0; y < mh; y++)
            for (int x = 0; x < mw; x++) {
                int wx = left + x * spanX / mw, wy = top + y * spanY / mh, k = (wy / 4) * FOG_SIZE + wx / 4,
                    known = g.seen[k >> 3] & (1 << (k & 7));
                uint32_t c = ground(biome(wx, wy));
                if (terrain_water(wx, wy))
                    c = 0x3f6464;
                else if (terrain_road(wx, wy))
                    c = shade(c, 1.3f);
                atlas[y * mw + x] = abgr(shade(c, known ? 1 : .32f));
            }
        cachedHash = h;
        ready = 1;
    }
    for (int y = 0; y < mh; y++)
        memcpy(fb + (oy + y) * pitch + ox, atlas + y * mw, mw * sizeof(uint32_t));
    for (int t = 0; t < TOWNS; t++)
        if (g.discovered[t] && INMAP(towns[t].x, towns[t].y)) {
            int x = MAPX(towns[t].x), y = MAPY(towns[t].y);
            rect(x - 2, y - 2, 5, 5, factionColors[g.townOwner[t]]);
            frame(x - 2, y - 2, 5, 5, GOLD);
            if (ui.mapZoom && y > oy + 16)
                clipped(x < ox + 130 ? x + 5 : x - 76, y - 8, towns[t].name, 12, GOLD);
        }
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && !g.actors[i].dead && INMAP(g.actors[i].x, g.actors[i].y))
            rect(MAPX(g.actors[i].x) - 1, MAPY(g.actors[i].y) - 1, 3, 3, TEAL);
    if (g.baseExists && INMAP(g.baseX, g.baseY))
        frame(MAPX(g.baseX) - 3, MAPY(g.baseY) - 3, 7, 7, TEAL);
    Actor *leader = &g.actors[g.selected];
    int youx = MAPX(leader->x), youy = MAPY(leader->y);
    int x = MAPX(ui.mapX), y = MAPY(ui.mapY);
    if (INMAP(leader->x, leader->y)) {
        if (ui.waypoint)
            line(youx, youy, x, y, TEAL);
        diamond(youx, youy, 5, 4, TEAL);
        frame(youx - 5, youy - 5, 11, 11, TEXT);
        text(youx < 235 ? youx + 8 : youx - 25, youy < oy + 12 ? youy + 8 : youy - 8, "YOU", TEXT, 1);
    }
    line(x - 5, y, x + 5, y, TEXT);
    line(x, y - 5, x, y + 5, TEXT);
    frame(ox - 1, oy - 1, mw + 2, mh + 2, EDGE);
    text(300, 84, "SETTLEMENTS", GOLD, 1);
    int first = start_row(ui.mapTown, TOWNS, 9);
    for (int j = 0; j < 9; j++) {
        int t = first + j;
        if (t == ui.mapTown)
            rect(298, 98 + j * 13, 148, 12, 0x3c4034);
        clipped(303, 100 + j * 13, g.discovered[t] ? towns[t].name : "UNCHARTED", 23,
                g.discovered[t] ? TEXT : MUTED);
    }
    char b[90];
    snprintf(b, 90, "YOU %d,%d  DEST %.0fm  %s", (int)leader->x, (int)leader->y,
             distance(leader->x, leader->y, ui.mapX, ui.mapY), regionNames[biome(ui.mapX, ui.mapY)]);
    text(31, 222, b, TEAL, 1);
#undef MAPX
#undef MAPY
#undef INMAP
}

static void trade_screen(void) {
    char b[100], v[70];
    overlay_header(ui.sell ? "THE MARKET / SELL" : "THE MARKET / BUY",
                   "\014 BUY-SELL   \005/\006 FILTER   \001 TRADE ONE");
    int count = ui.sell ? PACK : shop_count(), first = start_row(ui.selection, count, 6);
    for (int j = 0; j < 6 && first + j < count; j++) {
        int i = first + j, it = ui.sell ? g.actors[g.selected].pack[i].item : shop_item(i);
        snprintf(b, 100, "%.39s", it ? itemDefs[it].name : "--");
        snprintf(v, 70, "%d S  \001%d", item_price(ui.tradeTown, it, ui.sell),
                 ui.sell ? g.actors[g.selected].pack[i].qty : g.stock[ui.tradeTown][it]);
        row(85 + j * 16, i == ui.selection, b, it ? v : "");
    }
    int it = ui.sell ? g.actors[g.selected].pack[ui.selection].item : shop_item(ui.selection);
    equipment_comparison(it, 173);
    snprintf(b, sizeof(b), "%d SILVER", g.money);
    text(360, 45, b, GOLD, 1);
}

static void build_screen(void) {
    char b[100];
    overlay_header("BUILD AN OUTPOST", "\001 PLACE BESIDE SELECTED MEMBER. OUTSIDE VILLAGES ONLY.");
    static const int wood[] = {3, 5, 6, 8, 6, 3, 5, 8}, iron[] = {0, 0, 0, 3, 1, 0, 0, 3};
    for (int i = 0; i < buildCount; i++) {
        snprintf(b, 100, "%d TIMBER  %d IRON", wood[i], iron[i]);
        row(86 + i * 17, i == ui.selection, buildNames[i], b);
    }
    text(31, 225, "BUILDINGS ATTRACT RAIDS. KEEP SUPPLIES AND A RETREAT ROUTE.", MUTED, 1);
}
static void craft_screen(void) {
    overlay_header("WORKSHOP", "\001 CRAFT. STAND NEAR THE CORRECT CAMP WORKSTATION.");
    const char *cost[] = {"3 ORE / FORGE",
                          "3 GRAIN OR 2 MEAT + 1 WATER",
                          "2 CLOTH + 1 WATER / BENCH",
                          "3 IRON + 1 TIMBER / TIER 1",
                          "6 IRON + 2 CLOTH / TIER 2",
                          "5 IRON + 1 RELIC / TIER 3",
                          "5 IRON + 1 RELIC / TIER 3",
                          "1 IRON + 1 TIMBER / FORGE"};
    for (int i = 0; i < recipeCount; i++)
        row(87 + i * 17, i == ui.selection, recipeNames[i], NULL);
    text(31, 225, cost[ui.selection], GOLD, 1);
}
static void research_screen(void) {
    char b[100];
    overlay_header("OLD-WORLD RESEARCH", "A BENCH, SALVAGED RELICS, AND A LITTLE CURIOSITY.");
    const char *tiers[] = {"I   METALWORK / SABRES / WIND PUMPS", "II  ARMOR FORGING / PLATE CUIRASS",
                           "III PROSTHETICS / MECHANICAL LIMBS"};
    for (int i = 0; i < 3; i++) {
        text(33, 94 + i * 31, tiers[i], g.tech > i ? TEAL : TEXT, 1);
        text(379, 94 + i * 31, g.tech > i ? "DONE" : "LOCKED", g.tech > i ? TEAL : MUTED, 1);
    }
    snprintf(b, 100, "NEXT: 1 RELIC + %d SILVER", (g.tech + 1) * 150);
    text(33, 194, g.tech < 3 ? b : "RESEARCH COMPLETE", GOLD, 1);
    text(33, 214, "\001 RESEARCH THE NEXT TIER", TEXT, 1);
}
static void menu_screen(void) {
    overlay_header("EXPEDITION", "\015 CHOOSE   \001 OPEN   \010 GUIDE");
    const char *opts[] = {"Return to the wasteland",
                          "Build an outpost",
                          "Crafting recipes",
                          "Research",
                          "Save expedition",
                          "Load expedition",
                          "Simulation speed",
                          "Field guide",
                          "Start a new wanderer",
                          "Save expedition and title",
                          "Pay bounty / repair reputation",
                          "Forge equipment",
                          "Road journal",
                          "Factions and world news",
                          "Character appearance",
                          "Sleep in a nearby bed"};
    int first = start_row(ui.selection, 16, 9);
    char b[30];
    for (int j = 0; j < 9; j++) {
        int i = first + j;
        snprintf(b, 30, i == 6 ? "%dX" : "", ui.speed);
        row(85 + j * 16, ui.selection == i, opts[i], b);
    }
}

static void help_screen(void) {
    overlay_header("FIELD GUIDE", "\005/\006 OR \015 CHAPTER   \002 RETURN TO PREVIOUS SCREEN");
    char b[100];
    snprintf(b, 100, "%02d / %02d   %s", ui.selection + 1, GUIDE_PAGES, guideTitles[ui.selection]);
    text(31, 86, b, TEAL, 1);
    wrap(31, 106, 68, guideBodies[ui.selection], TEXT);
}
static void creator_screen(void) {
    overlay_header(ui.titleChoice ? "THE BARBER" : "A NAME IN THE DUST",
                   "\014 CHANGE   \001 CHOOSE   \010 GUIDE");
    Actor a = preview_actor();
    a.body = ui.createBody;
    a.skin = ui.createSkin;
    a.hair = ui.createHair;
    a.hairColor = ui.createHairColor;
    a.beard = ui.createBeard;
    a.scar = ui.createScar;
    a.race = ui.createRace;
    if (!ui.titleChoice) {
        const int w[] = {8, 0, 12, 9, 24}, t[] = {32, 34, 35, 46, 45}, h[] = {49, 48, 56, 51, 54};
        a.gear[0] = gear_id(w[ui.createOrigin], 1, 1);
        a.gear[1] = gear_id(t[ui.createOrigin], 1, 1);
        a.gear[2] = gear_id(h[ui.createOrigin], 0, 1);
        a.gear[3] = gear_id(60, 0, 0);
        a.gear[4] = gear_id(69, 0, 0);
        a.gear[5] = gear_id(76, 0, 0);
    }
    person(&a, 106, 196, 2, 0);
    center(105, 211, ui.createName, TEAL, 1);
    const char *labels[] = {
        "NAME", "ORIGIN",    "ANCESTRY", "BUILD", "SKIN",
        "HAIR", "HAIR TONE", "BEARD",    "SCARS", ui.titleChoice ? "KEEP CHANGES" : "ENTER THE FRONTIER"};
    for (int i = 0; i < 10; i++) {
        int y = 83 + i * 14;
        char b[40];
        if (i == ui.selection) {
            rect(203, y - 3, 245, 13, 0x3c4034);
            rect(203, y - 3, 2, 13, GOLD);
        }
        text(211, y, labels[i], i == 9 ? GOLD : MUTED, 1);
        if (i == 0)
            snprintf(b, 40, "%.15s", ui.createName);
        else if (i == 1)
            snprintf(b, 40, "%s", originNames[ui.createOrigin]);
        else if (i == 2)
            snprintf(b, 40, "%s", raceNames[ui.createRace]);
        else if (i == 9)
            b[0] = 0;
        else {
            int vals[] = {0,
                          0,
                          0,
                          ui.createBody,
                          ui.createSkin,
                          ui.createHair,
                          ui.createHairColor,
                          ui.createBeard,
                          ui.createScar};
            snprintf(b, 40, "< %02d >", vals[i] + 1);
        }
        text(442 - (int)strlen(b) * 6, y, b, TEXT, 1);
    }
}
static void name_screen(void) {
    overlay_header("NAME YOUR DRIFTER", "\011 SELECT   \001 LETTER   \003 ERASE   \007 DONE");
    center(240, 91, ui.createName, TEAL, 2);
    const char *keys = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";
    for (int j = 0; j < 40; j++) {
        int x = 35 + j % 10 * 41, y = 125 + j / 10 * 24;
        rect(x, y, 36, 19, j == ui.nameCell ? 0x68553c : INK);
        frame(x, y, 36, 19, j == ui.nameCell ? GOLD : EDGE);
        char b[2] = {j < 38 ? keys[j] : 0, 0};
        center(x + 18, y + 6, j == 38 ? "DEL" : j == 39 ? "DONE" : b, j == ui.nameCell ? TEXT : MUTED, 1);
    }
}
static void equipped_screen(void) {
    Actor *a = &g.actors[g.selected];
    overlay_header("WORN EQUIPMENT", "\001 UNEQUIP   \004 PACK   \010 GUIDE");
    person(a, 100, 193, 2, 0);
    for (int i = 0; i < GEARS; i++) {
        int y = 86 + i * 18;
        if (i == ui.selection)
            rect(195, y - 3, 252, 16, 0x3c4034);
        text(202, y, slotNames[i], MUTED, 1);
        clipped(246, y, itemDefs[a->gear[i]].name, 33, i == ui.selection ? TEXT : GOLD);
    }
    int it = a->gear[ui.selection];
    char b[100];
    snprintf(b, sizeof(b), "%s | WT %d | %s +%d", slotNames[ui.selection], itemDefs[it].weight,
             it >= BASE_ITEMS ? effectNames[itemDefs[it].effect] : "STANDARD", itemDefs[it].potency);
    clipped(30, 212, b, 68, TEAL);
}
static void forge_screen(void) {
    overlay_header("THE IRON TRADE", "\014 CONFIGURE   \001 FORGE   \010 GUIDE");
    int it = gear_id(ui.forgeFamily, ui.forgeMaterial, ui.forgeQuality);
    ItemDef *d = &itemDefs[it];
    Actor a = preview_actor();
    a.gear[d->slot] = it;
    person(&a, 102, 184, 2, 0);
    const char *labels[] = {"SLOT", "DESIGN", "MATERIAL", "QUALITY", "FORGE THIS PIECE"};
    const char *values[] = {slotNames[ui.forgeSlot], familyNames[ui.forgeFamily],
                            materialNames[ui.forgeMaterial], qualityNames[ui.forgeQuality], "\001"};
    for (int i = 0; i < 5; i++) {
        int y = 85 + i * 20;
        if (i == ui.selection)
            rect(195, y - 3, 253, 16, 0x3c4034);
        text(202, y, labels[i], MUTED, 1);
        text(441 - strlen(values[i]) * 6, y, values[i], GOLD, 1);
    }
    char b[100];
    snprintf(b, 100, "IRON %d  CLOTH %d  WOOD %d  RELIC %d  COAL %d", 2 + d->material + d->quality * 2,
             d->slot ? 2 + d->quality : 0, d->slot == 0, d->material == 5, d->quality >= 2);
    text(29, 205, b, TEXT, 1);
    snprintf(b, 100, "RESEARCH %d / %d   SMITHING %d / %d", g.tech,
             d->material >= 4   ? 3
             : d->material >= 2 ? 2
                                : 1,
             level(&g.actors[g.selected], SMITHING), d->quality * 12);
    text(29, 218, b, TEAL, 1);
    snprintf(b, 100, "%s +%d   DMG %d  ARM %d", effectNames[d->effect], d->potency, d->power, d->armor);
    text(201, 187, b, MUTED, 1);
}
static void dialogue_screen(void) {
    int n = ui.dialogActor;
    if (n < 0 || n >= g.actorCount)
        return;
    Actor *a = &g.actors[n];
    overlay_header(a->name, "\001 REPLY   \015 CHOOSE   \002 LEAVE   \010 GUIDE");
    person(a, 76, 178, 2, 0);
    clipped(30, 204, npc_role_name(a), 16, TEAL);
    clipped(30, 218, factionNames[a->faction], 16, MUTED);
    wrap(134, 85, 50, dialogue_line(n, ui.dialogPage), TEXT);
    const char *work[] = {"Carry a dispatch - 220 silver", "Bring eight ore - 240 silver",
                          "Defeat three foes - 420 silver", "Recover a relic - 550 silver", "Back"};
    const char *prison[] = {"Pay my bail.", "Try the lock. Requires tools.", "Wait out the sentence."};
    int jailed = a->prisonTime > 0 && n == g.selected,
        count = jailed               ? 3
                : ui.dialogPage == 1 ? 5
                                     : dialogue_count(n),
        first = start_row(ui.selection, count, 5);
    for (int j = 0; j < 5 && first + j < count; j++) {
        int k = first + j, y = 153 + j * 15;
        if (k == ui.selection)
            rect(129, y - 3, 318, 14, 0x3c4034);
        text(137, y,
             jailed               ? prison[k]
             : ui.dialogPage == 1 ? work[k]
                                  : dialogue_option(n, k),
             k == ui.selection ? GOLD : MUTED, 1);
    }
}
static void save_screen(void) {
    overlay_header(ui.saveMode ? "SAVE EXPEDITION" : "LOAD EXPEDITION",
                   "\001 SELECT SLOT   SLOT ZERO AUTOSAVES   \010 GUIDE");
    char b[100], v[100];
    for (int i = 0; i < SAVE_SLOTS; i++) {
        SaveInfo *z = &ui.saves[i];
        snprintf(b, 100, i ? "SLOT %d  %s" : "AUTO %d  %s", i,
                 z->exists ? (z->valid ? z->name : "DAMAGED / INCOMPATIBLE") : "EMPTY");
        snprintf(v, 100, "DAY %d  %d MEMBERS  %d SILVER", z->day, z->squad, z->money);
        row(89 + i * 18, ui.selection == i, b, z->valid ? v : "");
    }
    text(31, 221, "CHECKSUM VERIFIED. PREVIOUS SAVE KEPT AS .BAK.", TEAL, 1);
}
static void journal_screen(void) {
    overlay_header("THE ROAD JOURNAL", "\001 CLAIM AT THE ISSUING TOWN   \010 GUIDE");
    char b[180], v[40];
    int first = start_row(ui.selection, CONTRACTS, 6);
    for (int j = 0; j < 6; j++) {
        int k = first + j;
        Contract *c = &g.contracts[k];
        if (c->active) {
            snprintf(b, sizeof(b), "%02d %s", k + 1, towns[c->town].name);
            snprintf(v, 40, c->completed ? "COMPLETE" : "%d / %d  +%d", c->progress, c->needed, c->reward);
        } else {
            snprintf(b, sizeof(b), "%02d NO CONTRACT", k + 1);
            v[0] = 0;
        }
        row(85 + j * 16, k == ui.selection, b, v);
    }
    contract_description(ui.selection, b, sizeof(b));
    wrap(31, 187, 68, b, TEXT);
    snprintf(b, sizeof(b), "FRONTIER LEGACY %d/4   COMPLETED %d", g.storyStage, g.contractsDone);
    text(31, 221, b, TEAL, 1);
}
static void factions_screen(void) {
    overlay_header("FLAGS OF THE FRONTIER", "REPUTATION AFFECTS PRICES, GUARDS AND REFUGE");
    char b[100];
    for (int f = 0; f < FACTIONS; f++) {
        int townsHeld = 0;
        for (int t = 0; t < TOWNS; t++)
            if (g.townOwner[t] == f)
                townsHeld++;
        snprintf(b, 100, "%+d REP  %d TOWNS", g.rep[f], townsHeld);
        row(85 + f * 16, ui.selection == f, factionNames[f], b);
    }
    clipped(31, 220, g.worldNews, 68, GOLD);
}
static void storage_screen(void) {
    Actor *a = &g.actors[g.selected];
    Building *z = &g.buildings[ui.storageIndex];
    overlay_header(ui.storageSide ? "DEPOT / WITHDRAW" : "DEPOT / DEPOSIT",
                   "\014 CHOOSE SIDE   \001 MOVE ONE   \010 GUIDE");
    int count = ui.storageSide ? 16 : PACK, first = start_row(ui.selection, count, 9);
    char b[30];
    for (int j = 0; j < 9 && first + j < count; j++) {
        int k = first + j;
        Stack st = ui.storageSide ? z->storage[k] : a->pack[k];
        snprintf(b, 30, "\001%d", st.qty);
        row(86 + j * 15, k == ui.selection, st.item ? itemDefs[st.item].name : "EMPTY", st.item ? b : "");
    }
}
static void loot_screen(void) {
    Actor *a = &g.actors[ui.lootActor];
    overlay_header("SALVAGE THE FALLEN", "\001 TAKE ONE   \015 PACK AND WORN GEAR");
    int first = start_row(ui.selection, PACK + GEARS, 9);
    char b[80], v[30];
    for (int j = 0; j < 9; j++) {
        int k = first + j, it = k < PACK ? a->pack[k].item : a->gear[k - PACK];
        snprintf(b, 80, "%s %.34s", k < PACK ? "PACK" : slotNames[k - PACK],
                 it ? itemDefs[it].name : "EMPTY");
        snprintf(v, 30, "\001%d", k < PACK ? a->pack[k].qty : 1);
        row(85 + j * 15, k == ui.selection, b, it ? v : "");
    }
    clipped(31, 223, a->name, 30, TEAL);
}
void render_title_art(uint32_t *pixels, int stride) {
    fb = pixels;
    pitch = stride;
    ui.camX = 28;
    ui.camY = 27;
    terrain();
    objects_draw();
}
void render_game(uint32_t *pixels, int stride) {
    fb = pixels;
    pitch = stride;
    if (ui.screen == SCREEN_TITLE) {
        title();
        return;
    }
    Actor *a = &g.actors[g.selected];
    ui.camX = a->x + (g.shake > 0 ? sinf(g.time * 97) * g.shake * .12f : 0);
    ui.camY = a->y + (g.shake > 0 ? cosf(g.time * 83) * g.shake * .12f : 0);
    if (g.interiorBuilding)
        interior_draw();
    else {
        terrain();
        objects_draw();
    }
    if (!g.interiorBuilding && g.weather) {
        for (int i = 0; i < 25; i++) {
            int x = ((int)(g.time * 63) + i * 79) % 520 - 20, y = (i * 37) % 210;
            line(x, y, x + (g.weather == 1 ? 14 : 3), y + (g.weather == 1 ? 3 : 9),
                 g.weather == 3   ? 0x829a6b
                 : g.weather == 4 ? 0xc7cbbd
                                  : shade(0xc7ba92, .85f));
        }
    }
    if (ui.screen == SCREEN_WORLD && !g.interiorBuilding)
        speech_bubbles();
    hud();
    switch (ui.screen) {
    case SCREEN_CREATE:
        creator_screen();
        break;
    case SCREEN_NAME:
        name_screen();
        break;
    case SCREEN_EQUIPPED:
        equipped_screen();
        break;
    case SCREEN_FORGE:
        forge_screen();
        break;
    case SCREEN_DIALOG:
        dialogue_screen();
        break;
    case SCREEN_SAVES:
        save_screen();
        break;
    case SCREEN_JOURNAL:
        journal_screen();
        break;
    case SCREEN_FACTIONS:
        factions_screen();
        break;
    case SCREEN_STORAGE:
        storage_screen();
        break;
    case SCREEN_LOOT:
        loot_screen();
        break;
    case SCREEN_PACK:
        pack_screen();
        break;
    case SCREEN_SKILLS:
        skills_screen();
        break;
    case SCREEN_HEALTH:
        health_screen();
        break;
    case SCREEN_SQUAD:
        squad_screen();
        break;
    case SCREEN_MAP:
        map_screen();
        break;
    case SCREEN_TRADE:
        trade_screen();
        break;
    case SCREEN_BUILD:
        build_screen();
        break;
    case SCREEN_CRAFT:
        craft_screen();
        break;
    case SCREEN_RESEARCH:
        research_screen();
        break;
    case SCREEN_MENU:
        menu_screen();
        break;
    case SCREEN_HELP:
        help_screen();
        break;
    case SCREEN_RECRUIT: {
        Actor *r = &g.actors[ui.selection];
        overlay_header("A FELLOW WANDERER", "\001 RECRUIT FOR 300 SILVER   /   \002 LEAVE");
        person(r, 103, 201, 2, 0);
        text(186, 92, r->name, TEAL, 2);
        wrap(186, 118, 39,
             "I can work, fight, and carry my share. Three hundred silver settles my debts. After that, we "
             "look out for each other.",
             TEXT);
        text(186, 185, "SQUAD LIMIT: EIGHT", GOLD, 1);
        break;
    }
    case SCREEN_CONFIRM:
        overlay_header("CONFIRM EXPEDITION CHANGE", "\001 CONFIRM    \002 CANCEL");
        wrap(31, 100, 65,
             ui.confirmAction == 1   ? "Start a new expedition? Unsaved progress will be discarded. Existing "
                                       "saves remain available."
             : ui.confirmAction == 2 ? "Load the selected save slot? Unsaved progress will be discarded."
                                     : "Save to this slot? Its previous expedition will be replaced; the "
                                       "last version is kept as a backup.",
             TEXT);
        break;
    default:
        break;
    }
    if (ui.screen != SCREEN_WORLD && ui.noticeTime > 0) {
        rect(0, 253, 480, 19, INK);
        char message[76];
        snprintf(message, sizeof(message), "%.74s", ui.notice);
        text(14, 258, message, GOLD, 1);
    }
}

void render_catalog_scale(uint32_t *pixels, int stride, int item, int sc) {
    fb = pixels;
    pitch = stride;
    rect(0, 0, 480, 272, INK);
    Actor a = preview_actor();
    memset(a.gear, 0, sizeof(a.gear));
    a.gear[itemDefs[item].slot] = item;
    person(&a, 240, 235, sc, 0);
}
void render_catalog_sample(uint32_t *pixels, int stride, int item) {
    render_catalog_scale(pixels, stride, item, 5);
}
