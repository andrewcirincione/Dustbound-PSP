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
static int wrap(int x, int y, int cols, const char *s, uint32_t c) {
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
    return y;
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
static inline int project_x(float x, float y) {
    return 240 + projectionX + (int)((x - y - ui.camX + ui.camY) * 22);
}
static inline int project_y(float x, float y) {
    return 126 + projectionY + (int)((x + y - ui.camX - ui.camY) * 11) - terrain_height(x, y);
}
static void project(float x, float y, int *sx, int *sy) {
    *sx = project_x(x, y);
    *sy = project_y(x, y);
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
static void texture_ground(void) {
    static uint16_t multipliers[64][64];
    static int initialized;
    if (!initialized) {
        const ArtSprite *tex = &artSprites[107];
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                uint32_t c = tex->palette[tex->pixels[tex->rows[y * 3] + x]];
                multipliers[y][x] = 222 + ((c & 255) + ((c >> 8) & 255) + ((c >> 16) & 255)) / 14;
            }
        }
        initialized = 1;
    }
    for (int y = 0; y < clipBottom; y++) {
        float u = (ui.camX + (y - 126 - projectionY) / 22.f + (-240 - projectionX) / 44.f) * 8;
        float v = (ui.camY + (y - 126 - projectionY) / 22.f - (-240 - projectionX) / 44.f) * 8;
        uint32_t *row = fb + y * pitch;
        for (int x = 0; x < clipRight; x++, u += 8 / 44.f, v -= 8 / 44.f) {
            unsigned m = multipliers[(int)v & 63][(int)u & 63];
            uint32_t d = row[x];
            row[x] = 0xff000000 | (((d & 0xff00ff) * m >> 8) & 0xff00ff) | (((d & 0xff00) * m >> 8) & 0xff00);
        }
    }
}
static void ground_tile(int x, int y, uint32_t color) {
    /* The 22x11 tile has an integer slope. Fill both halves in a single span. */
    for (int dy = -11; dy <= 11; dy++) {
        int half = 22 - 2 * abs(dy);
        rect(x - half, y + dy, 2 * half + 1, 1, color);
    }
}
static void terrain_raw(void) {
    float li = light();
    rect(0, 0, clipRight, clipBottom, shade(ground(biome(ui.camX, ui.camY)), li));
    int cx = (int)ui.camX, cy = (int)ui.camY;
    for (int y = cy - 18; y <= cy + 18; y++)
        for (int x = cx - 18; x <= cx + 18; x++) {
            int sx = project_x(x + .5f, y + .5f);
            if (sx < -24 || sx > clipRight + 24)
                continue;
            int sy = project_y(x + .5f, y + .5f);
            if (sy < -12 || sy > clipBottom + 12)
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
                ground_tile(sx, sy, c);
                int shift = (x + y) % 5;
                line(sx - 9 + shift, sy - 3, sx + 6 + shift, sy - 3, shade(c, 1.25f));
                line(sx - 8, sy + 3, sx + 8, sy + 3, shade(c, .8f));
                continue;
            }
            ground_tile(sx, sy, c);
            for (int grain = 0; grain < 35; grain++) {
                unsigned z = hash(x + grain * 9, y - grain * 7);
                int gx = (int)(z % 31) - 15, gy = (int)((z >> 8) % 13) - 6;
                if (abs(gx) + abs(gy) * 2 < 22) {
                    uint32_t fleck = shade(c, .85f + (z % 5) * .06f);
                    rect(sx + gx, sy + gy, 2 + (z % 2), 1, fleck);
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
    texture_ground();
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
    static const int ids[] = {0, 1, 2, 3, 24, 25, 27, 26, 28, 6, 29, 30, 7};
    int base = b->type == HOUSE ? (b->faction == HOSTILE ? 5 : b->faction == SHOGUN ? 4 : id % 3 == 0 ? 7 : 0) : ids[b->type];
    return civil_building_sprite(id,base);
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
    art(sprite, x, y, light(), fade || g.depth.buildings[id].labor>0);
    if(g.depth.buildings[id].labor>0){bar(x-20,y-8,40,3,100-g.depth.buildings[id].labor/35*100,GOLD);text(x-24,y+5,"BUILDING",GOLD,1);}
    if (b->type == CAMPFIRE) {
        int flicker = (int)(g.time * 10) % 3;
        px(x - 2 + flicker, y - 13 - flicker, 0xe5b66a);
    }
    if (b->type == SHOP || b->type == BAR) {
        const char *name = b->type == SHOP ? "TRADING POST" : "THE WAYWARD";
        int town=b->owner<0?town_near(b->x,b->y,15):-1;
        if(town>=OLD_TOWNS){
            static const char *shops[]={"RAIL EXCHANGE","IRON TRIBUTE HALL","HERBAL DISPENSARY"};
            static const char *inns[]={"PUMP HOUSE INN","COURT INN","PILGRIM HOSPICE"};
            name=b->type==SHOP?shops[town-OLD_TOWNS]:inns[town-OLD_TOWNS];
        }
        int w = text_width(name) + 8;
        rect(x - w / 2, y - artSprites[sprite].ay - 10, w, 11, INK);
        center(x, y - artSprites[sprite].ay - 8, name, GOLD, 1);
    }
}
#include "painted_actor.h"
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
    if(b->item==SEVERED_ARM||b->item==SEVERED_LEG){
        diamond(x,y,7,3,shade(0x653c30,light()));
        part(b->item==SEVERED_ARM?75:64,x-5,y-2,x+5,y,4,0x9d7660,light(),0,0,0,1);
        return;
    }
    int id = resource_sprite(n);
    art(id, x, y + (id < 24 ? 5 : 0), light() * (b->type == NODE_ORE ? .82f : 1), 0);
    if (b->locked) {
        frame(x - 2, y - 5, 4, 4, GOLD);
    }
}
typedef struct {
    float depth;
    int kind, index, x, y;
} Object;
static Object objects[MAX_NODES + MAX_BUILDINGS + MAX_ACTORS + 1200 + POP_GROUPS*POP_PROPS];
static int compare_obj(const void *a, const void *b) {
    float d = ((Object *)a)->depth - ((Object *)b)->depth;
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}
static void objects_draw(void) {
    int clearIds[MAX_ACTORS], clearCount = 0;
    unsigned char squadTargets[MAX_ACTORS] = {0};
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (!a->active || !a->squad) continue;
        int target = a->target, command = g.living.command[i];
        if (target >= 0 && target < g.actorCount) squadTargets[target] = 1;
        if (command >= 0 && command < g.actorCount) squadTargets[command] = 1;
    }
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (!a->active) continue;
        int target = a->target;
        int targetsSquad = target >= 0 && target < g.actorCount &&
                           g.actors[target].active && g.actors[target].squad;
        if (a->squad || (!a->dead && (squadTargets[i] || targetsSquad)))
            clearIds[clearCount++] = i;
    }
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
        if (!a->active || a->carriedBy >= 0 || actor_inside(a) || population_hidden(i))
            continue;
        int x = project_x(a->x, a->y);
        if (x <= -30 || x >= 510)
            continue;
        int y = project_y(a->x, a->y);
        if (y > -20 && y < 250)
            objects[n++] = (Object){a->x + a->y, 2, i, x, y};
    }
    int cx = ui.camX, cy = ui.camY;
    int campSites[POP_GROUPS], campCount = 0;
    for (int k = 0; k < POP_GROUPS; k++) {
        int props = population_object_count(k);
        if (!props)
            continue;
        const PopulationState *site = &g.population.state[k];
        if (fabsf(site->siteX - cx) < 24 && fabsf(site->siteY - cy) < 24)
            campSites[campCount++] = k;
        for (int j = 0; j < props; j++) {
            float wx, wy;
            if (!population_object(k, j, &wx, &wy) || fabsf(wx - ui.camX) > 20 || fabsf(wy - ui.camY) > 20)
                continue;
            int x, y;
            project(wx, wy, &x, &y);
            objects[n++] = (Object){wx + wy, 4, k * POP_PROPS + j, x, y};
        }
    }
    for(int k=0;k<6;k++)for(int r=0;r<4;r++){float x,y;if(!trail_rock(k,r,&x,&y)||fabsf(x-ui.camX)>20||fabsf(y-ui.camY)>20)continue;int sx,sy;project(x,y,&sx,&sy);objects[n++]=(Object){x+y,5,k*4+r,sx,sy};}
    for (int y = cy - 16; y < cy + 16; y++)
        for (int x = cx - 16; x < cx + 16; x++) {
            unsigned h = hash(x, y);
            if (h % 13 || town_near(x, y, 8) >= 0 || road(x, y))
                continue;
            int campGround = 0;
            for (int k = 0; k < campCount; k++) {
                const PopulationState *site = &g.population.state[campSites[k]];
                if (fabsf(x - site->siteX) < 7 && fabsf(y - site->siteY) < 7) {
                    campGround = 1;
                    break;
                }
            }
            if (campGround) continue;
            int clear=0;for(int j=0;j<clearCount;j++){Actor*a=&g.actors[clearIds[j]];if(distance(x,y,a->x,a->y)<2.3f){clear=1;break;}}if(clear)continue;
            if(trail_blocked(x,y))continue;
            int sx, sy;
            project(x + .3f, y + .4f, &sx, &sy);
            if (sx < 0 || sx > 480 || sy < 0 || sy > 252)
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
        if(o->kind==4){if(!population_prop_style(o->index/POP_PROPS,o->index%POP_PROPS))art_shadow(population_prop_art(o->index/POP_PROPS,o->index%POP_PROPS),x,y);}
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
    if (ui.screen == SCREEN_WORLD && ui.contextIndex >= 0 && !g.interiorBuilding && context_distance() <= context_reach()) {
        int x, y;
        float wx, wy;
        int id = ui.contextIndex;
        if (ui.contextKind == 0) {
            wx = g.actors[id].x;
            wy = g.actors[id].y;
        } else if (ui.contextKind == 1) {
            wx = g.nodes[id].x;
            wy = g.nodes[id].y;
        } else if(ui.contextKind==4) {wx=g.depth.siteX[id];wy=g.depth.siteY[id];
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
            if (a->squad) {float hp=(a->health[HEAD]+a->health[CHEST])*50/species_health(a);bar(o->x-10,o->y-43,21,2,hp,TEAL);}
            if (a->squad && o->index == g.selected) {
                triangle(o->x - 3, o->y - 49, o->x + 3, o->y - 49, o->x, o->y - 45, TEAL);
            }
        } else if(o->kind==5){art(11,o->x,o->y,light(),0);art(9,o->x+10,o->y+4,light(),0);
        } else if(o->kind==4){
            float wx,wy;int t=population_object(o->index/POP_PROPS,o->index%POP_PROPS,&wx,&wy),id=population_prop_art(o->index/POP_PROPS,o->index%POP_PROPS),style=population_prop_style(o->index/POP_PROPS,o->index%POP_PROPS);int x=o->x,y=o->y;
            if(style==1){uint32_t cloth=shade(0x81725a,light());triangle(x-14,y-4,x+1,y-12,x+13,y-6,cloth);triangle(x-14,y-4,x+13,y-6,x-2,y+2,cloth);line(x-12,y-5,x-1,y+1,shade(0xb6aa87,light()));line(x-10,y-6,x+1,y,shade(0xb6aa87,light()));}
            else if(style==2){line(x,y,x,y-29,shade(0x6d5037,light()));triangle(x+1,y-28,x+14,y-24,x+1,y-17,shade(0x9c403a,light()));}
            else if(style==3){art(31,x,y,light()*.78f,0);art(20,x+9,y+2,light()*.7f,0);}
            else art(id,x,y,light(),0);
            if(t==1){int flicker=(int)(g.time*8)%3;diamond(o->x,o->y-6,2+flicker,4+flicker,0xe0a24b);}
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
    for(int i=0;i<n;i++)if(objects[i].kind==2){int id=objects[i].index;Actor*a=&g.actors[id];if(a->squad||((a->target==g.selected||g.actors[g.selected].target==id)&&distance(a->x,a->y,ui.camX,ui.camY)<9))actor_outline(a,objects[i].x,objects[i].y,id==g.selected);}
}
static void room_project(float x, float y, int *sx, int *sy);
static uint32_t label_color(int faction){uint32_t c=factionColors[faction];return (((c>>16)*3+255)/4<<16)|((((c>>8&255)*3+255)/4)<<8)|(((c&255)*3+255)/4);}
/* Knockout follows either vital region failing or blood falling below 20.
   Disabled arms/legs impair combat and mobility, not consciousness. */
static float knockout_reserve(const Actor *a){
 if(a->dead||a->unconscious>0)return 0;
 float vital=fminf(a->health[HEAD],a->health[CHEST])/species_health(a);
 return fmaxf(0,fminf(1,fminf(vital,(a->blood-20)/80.f)))*100;
}
static int squad_combatant(int n){
 Actor*a=&g.actors[n];if(a->dead)return 0;
 for(int i=0;i<g.actorCount;i++)if(g.actors[i].active&&g.actors[i].squad){
  if(a->target==i||g.actors[i].target==n||g.living.command[i]==n)return 1;
 }
 return 0;
}
static void nearby_labels(int indoors){
 int ids[MAX_ACTORS],count=0;float ds[MAX_ACTORS];Actor*p=&g.actors[g.selected];
 for(int i=0;i<g.actorCount;i++){Actor*a=&g.actors[i];if(!a->active||a->squad||a->carriedBy>=0||population_hidden(i)||depth_room(a)!=depth_room(p))continue;float d=depth_distance(p,a);if(d>9)continue;ids[count]=i;ds[count++]=d;}
 for(int i=1;i<count;i++){int n=ids[i],j=i;float d=ds[i];while(j>0&&ds[j-1]>d){ids[j]=ids[j-1];ds[j]=ds[j-1];j--;}ids[j]=n;ds[j]=d;}
 int boxes[8][4],used=0;
 int fighting=0;for(int i=0;i<count;i++)if(squad_combatant(ids[i]))fighting=1;
 if(fighting)for(int i=1;i<count;i++)if(ids[i]==p->target){int swap=ids[0];ids[0]=ids[i];ids[i]=swap;break;}
 for(int i=0;i<count&&used<(fighting?1:8);i++){Actor*a=&g.actors[ids[i]];int x,y;if(indoors)room_project(depth_x(a),depth_y(a),&x,&y);else project(a->x,a->y,&x,&y);
  char label[48];snprintf(label,sizeof(label),"%.20s%s",npc_role_name(a),a->dead?" / DEAD":a->unconscious>0?" / DOWN":actor_relation(ids[i])<0?" !":"");int w=text_width(label)+8,l=x-w/2,t=fighting?y+17:y-(a->dead||a->unconscious>0?24:56);if(l<4)l=4;if(l+w>476)l=476-w;if(t<26)t=26;
  for(int attempt=0;attempt<5;attempt++){int hit=0;for(int b=0;b<used;b++)if(l<boxes[b][0]+boxes[b][2]+2&&l+w+2>boxes[b][0]&&t<boxes[b][1]+13&&t+13>boxes[b][1])hit=1;if(!hit)break;t-=13;}
  int overlaps=0;for(int b=0;b<used;b++)if(l<boxes[b][0]+boxes[b][2]+2&&l+w+2>boxes[b][0]&&t<boxes[b][1]+13&&t+13>boxes[b][1])overlaps=1;
  if(overlaps||t<25||t>221||x<0||x>480||(l+w>398&&t<100))continue;
  boxes[used][0]=l;boxes[used][1]=t;boxes[used][2]=w;used++;
  if(!fighting)line(x,y-(a->dead||a->unconscious>0?8:35),l+w/2,t+10,shade(factionColors[a->faction],.65f));
  rect(l,t,w,11,INK);rect(l,t,2,11,factionColors[a->faction]);text(l+5,t+2,label,label_color(a->faction),1);
  if(!a->dead&&actor_relation(ids[i])<0)text(l+w-9,t+2,"!",RED,1);
 }
 for(int i=0;i<count;i++)if(squad_combatant(ids[i])){Actor*a=&g.actors[ids[i]];int x,y;if(indoors)room_project(depth_x(a),depth_y(a),&x,&y);else project(a->x,a->y,&x,&y);rect(x-14,y+1,29,7,0x14110e);bar(x-13,y+2,27,5,knockout_reserve(a),a->unconscious>0?MUTED:0xe36552);
 if(a->attackPhase==1){float q=1-a->attackClock/fmaxf(.01f,a->attackDuration);bar(x-12,y+8,25,3,q*100,GOLD);line(x,y+5,x+(int)(cosf(a->attackAngle)*18),y+5+(int)(sinf(a->attackAngle)*9),GOLD);}
 if(a->hitFlash>0&&a->impactKind){const char*hit=a->unconscious>0?"DOWN":a->impactKind==2?"BLOCK":a->impactKind==3?"DODGE":"HIT";text(x+17,y-12,hit,a->unconscious>0?GOLD:TEXT,1);}}
}
static void speech_bubbles(void) {
    int best = -1;
    float near = 7;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead || a->squad || a->speechTime <= 0 || actor_inside(a) ||
            a->unconscious>0 || g.depth.actors[i].captive || population_hidden(i))
            continue;
        float d = distance(a->x, a->y, ui.camX, ui.camY);
        if (d < 7 && (best<0 || (a->speech>=24 && g.actors[best].speech<24) ||
            ((a->speech>=24)==(g.actors[best].speech>=24) && d<near))) {
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
    int w = text_width(words) + 16;
    int h = w > 320 ? (w > 605 ? 36 : 25) : 17;
    if (w > 320) w = 320;
    x -= w / 2;
    if (x < 8)
        x = 8;
    if (x + w > 392) x = 392 - w;
    y -= 64;
    if (y < 30)
        y = 30;
    if (y > 159)
        y = 159;
    rect(x, y, w, h, INK);
    frame(x, y, w, h, EDGE);
    triangle(x + w / 2 - 3, y + h - 1, x + w / 2 + 3, y + h - 1, x + w / 2, y + h + 4, EDGE);
    wrap(x + 6, y + 4, (w - 9) / 6, words, a->speech>=24?GOLD:TEXT);
}

static void civic_flag(int t){
    int x,y,f=g.townOwner[t],side=war_faction_index(f);
    project(towns[t].x+1,towns[t].y+2,&x,&y);
    uint32_t flag=factionColors[f];
    line(x,y,x,y-43,0xd0b88c);
    if(side==0){
        triangle(x,y-43,x+24,y-36,x,y-28,flag);
        line(x+2,y-39,x+11,y-37,0xeac77a);
    }else if(side==1){
        rect(x+1,y-43,13,28,flag);
        rect(x+5,y-35,6,5,0xe4b386);
        line(x-3,y-43,x+16,y-43,0xe4b386);
    }else{
        rect(x+1,y-43,17,15,flag);
        triangle(x+1,y-28,x+18,y-28,x+9,y-22,flag);
        diamond(x+9,y-35,4,4,0x574840);
    }
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
    const uint32_t *palette = part_palette(id, 0x808080, li, 0);
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
            fb[yy * pitch + xx] = palette[s->pixels[s->rows[sy * 3] + sx - lo]];
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
    texture_plane(104, x, y - 11, 22, 11, -22, 11, .5f, .5f, .83f);
    line(x - 22, y, x, y + 10, GOLD);
    text(x - 15, y + 12, "EXIT", GOLD, 1);
    room_project(3.8f, 3.6f, &x, &y);
    art(23, x, y + 10, .8f, 0);
    InteriorProp props[8];
    int count = interior_props(id, props), order[MAX_ACTORS+10];
    float depth[MAX_ACTORS+10];
    int roomActor[MAX_ACTORS+10];
    for(int i=0;i<MAX_ACTORS+10;i++)roomActor[i]=-1;
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
    if (0) {
        keeper = count;
        order[count] = count;
        depth[count++] = b->type == SHOP ? 3.8f : 4.1f;
    }
    for(int i=0;i<g.actorCount;i++)if(i!=g.selected&&g.actors[i].active&&depth_room(&g.actors[i])==g.interiorBuilding){
        roomActor[count]=i;order[count]=count;depth[count++]=depth_x(&g.actors[i])+depth_y(&g.actors[i]);
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
        if(roomActor[i]>=0){Actor *resident=&g.actors[roomActor[i]];room_project(depth_x(resident),depth_y(resident),&x,&y);person(resident,x,y,1,0);}
        else if (i == player) {
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
            static const uint32_t terrain[]={0x606553,0x72634e,0x858477,0x4d655a,0x54534c,0x69675b,0x979c99,0x405349,0x71534c};
                uint32_t c = terrain[biome(wx, wy)];
                if(terrain_height(wx,wy)/4!=terrain_height(wx+1,wy)/4)c=shade(c,.8f);
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
        if (x >= ox && x < ox + 66 && y >= oy && y < oy + 60 && a->active && !a->dead && !population_hidden(i))
            rect(x, y, 2, 2, a->squad ? TEAL : a->faction == HOSTILE ? RED : GOLD);
    }
    frame(ox - 3, oy - 3, 72, 67, EDGE);
    text(ox + 28, oy - 1, "N", TEXT, 1);
}
static void detailed_status(void) {
    Actor *a = &g.actors[g.selected];
    char value[64];
    panel(8, 44, 198, 186);
    snprintf(value, sizeof(value), "%.16s", a->name);
    text(16, 53, value, TEAL, 1);
    text(160, 53, "STATUS", GOLD, 1);
    line(16, 66, 198, 66, EDGE);
    for (int i = 0; i < LIMBS; i++) {
        int y = 74 + i * 12;
        int missing = (a->lostMask & (1 << i)) && !(a->prostheticMask & (1 << i));
        uint32_t color = missing || a->health[i] < 30 ? RED : a->health[i] < 75 ? GOLD : TEAL;
        text(16, y, limbNames[i], MUTED, 1);
        bar(86, y + 2, 66, 4, missing ? 0 : a->health[i]*100/limb_max_health(a,i), color);
        if (missing) snprintf(value, sizeof(value), "LOST");
        else snprintf(value, sizeof(value), "%d", (int)a->health[i]);
        text(163, y, value, color, 1);
    }
    const char *labels[] = {"BLOOD", "FOOD", "FATIGUE"};
    float values[] = {a->blood, a->hunger, a->fatigue};
    for (int i = 0; i < 3; i++) {
        int y = 151 + i * 13;
        uint32_t color = i == 0 ? (a->bleed > .01f || a->blood < 50 ? RED : TEAL)
                         : i == 1 ? (a->hunger < 25 ? RED : GOLD)
                         : a->fatigue > 85 ? RED : MUTED;
        text(16, y, labels[i], MUTED, 1);
        bar(86, y + 2, 66, 4, values[i], color);
        snprintf(value, sizeof(value), "%d", (int)values[i]);
        text(163, y, value, color, 1);
    }
    int capacity = actor_capacity(a);
    float load = encumbrance(a);
    snprintf(value, sizeof(value), "LOAD %.0f / %d", load, capacity);
    text(16, 196, value, load > capacity ? RED : MUTED, 1);
    snprintf(value, sizeof(value), "BLEED %.1f", a->bleed);
    text(16, 208, value, a->bleed > .01f ? RED : MUTED, 1);
    text(16, 221, "D-PAD UP: HIDE", GOLD, 1);
}
static void hud(void) {
    Actor *a = &g.actors[g.selected];
    char b[120];
    rect(0, 0, 480, 23, INK);
    line(0, 22, 480, 22, EDGE);
    int t = town_near(a->x, a->y, 10);
    text(9, 8, t >= 0 ? towns[t].name : regionNames[biome(a->x, a->y)], GOLD, 1);
    snprintf(b, sizeof(b), "DAY %d  %02d:%02d", g.day, (int)g.dayTime, (int)(g.dayTime * 60) % 60);
    text(220, 8, b, MUTED, 1);
    snprintf(b, sizeof(b), "%d SILVER", g.money);
    text(472 - (int)strlen(b) * 6, 8, b, TEXT, 1);
    if (ui.screen != SCREEN_WORLD)
        return;
    if (!g.interiorBuilding)
        mini_map();
    if(ui.waypoint&&!g.interiorBuilding){float dx=ui.travelX-a->x,dy=ui.travelY-a->y;const char*d=fabsf(dx)>fabsf(dy)?(dx<0?"W":"E"):(dy<0?"N":"S");snprintf(b,sizeof(b),"PIN %.0fm %s",distance(a->x,a->y,ui.travelX,ui.travelY),d);text(350,104,b,GOLD,1);}
    recovery_hint(b,sizeof(b));if(b[0]){rect(4,222,472,12,INK);b[77]=0;text(8,224,b,TEAL,1);}
    if (ui.paused) {
        panel(192, 43, 94, 18);
        center(239, 49, "PAUSED", GOLD, 1);
    } else if (ui.speed == 2)
        text(370, 44, "2X", GOLD, 1);

    /* A single status strip; portraits, load details and controls live in menus. */
    rect(0, 252, 480, 20, INK);
    line(0, 252, 480, 252, EDGE);
    int member = 0, total = 0;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad) {
            if (i < g.selected) member++;
            total++;
        }
    int k = 0, shown = 0, page = member / 4;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad) {
            Actor *m = &g.actors[i];
            if (k++ / 4 != page) continue;
            int x = 8 + shown++ * 56;
            if (i == g.selected) rect(x - 2, 255, 53, 16, 0x363b32);
            char name[9];
            snprintf(name, sizeof(name), "%.8s", m->name);
            text(x, 258, name, m->dead || m->unconscious > 0 ? RED : i == g.selected ? TEAL : MUTED, 1);
            bar(x, 268, 48, 2, (m->health[HEAD] + m->health[CHEST]) * 50 / species_health(m), m->bleed > 0 ? RED : TEAL);
        }
    if (total == 1) {
        snprintf(b, sizeof(b), "LOAD %.0f/%d", encumbrance(a), actor_capacity(a));
        text(78, 259, b, MUTED, 1);
    } else if (total > 4) {
        snprintf(b, sizeof(b), "%d/%d", page + 1, (total + 3) / 4);
        text(224, 259, b, MUTED, 1);
    }
    snprintf(b, sizeof(b), "BLOOD %d", (int)a->blood);
    text(250, 259, b, a->blood < 50 || a->bleed > .01f ? RED : MUTED, 1);
    snprintf(b, sizeof(b), "FOOD %d", (int)a->hunger);
    text(320, 259, b, a->hunger < 25 ? GOLD : MUTED, 1);
    const char *status = a->dead ? "DEAD" : a->unconscious > 0 ? "DOWN" : a->bleed > .01f ? "BLEEDING"
                         : a->sneaking ? "SNEAK" : a->autoattack ? "FIGHT" : "PASSIVE";
    text(472 - (int)strlen(status) * 6, 259, status, a->bleed > .01f || a->dead || a->unconscious > 0 ? RED : TEAL, 1);

    if(a->carrying>=0)snprintf(b,sizeof(b),"\001 PUT DOWN %.24s",g.actors[a->carrying].name);
    else if(nearby_actions())snprintf(b,sizeof(b),"\001 NEARBY ACTIONS%s",a->sneaking?" / SNEAKING":"");
    else b[0]=0;
    if(b[0]){rect(4,233,(int)strlen(b)*6+16,15,PANEL);text(12,237,b,TEXT,1);}
    if (a->taskKind)
        bar(180, 249, 120, 2,
            (1 - fminf(1, a->action / (a->taskKind == 3 ? 180 : a->taskKind == 2 ? 4.5f : 2.8f))) * 100, GOLD);
    if (ui.statusPanel)
        detailed_status();
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
    text(28, 237, ui.menuReturn ? "\002 BACK   \010 GUIDE" : "\002 CLOSE   \010 GUIDE", MUTED, 1);
    if ((ui.screen >= SCREEN_PACK && ui.screen <= SCREEN_SQUAD) || ui.screen==SCREEN_EQUIPPED) {
        char who[60];snprintf(who,sizeof(who),ui.screen==SCREEN_HEALTH?"\005 MEMBER   \006 LIMBS":"\005/\006 MEMBER: %.16s",g.actors[g.selected].name);text(249,237,who,MUTED,1);
    }
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
    if(!a.gear[1]&&g.depth.actors[g.selected].layer)a.gear[1]=g.depth.actors[g.selected].layer;
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
static void compare_stat(int cell, int y, const char *label, float old, float next, int decimal, int lower) {
    char b[80];float delta=next-old;int x=31+(cell%2)*211, yy=y+(cell/2)*12;
    if(fabsf(delta)<.05f)snprintf(b,sizeof(b),decimal?"%s %.2f":"%s %.0f",label,next);
    else snprintf(b,sizeof(b),decimal?"%s %.2f (%+.2f)":"%s %.0f (%+.0f)",label,next,delta);
    clipped(x,yy,b,33,fabsf(delta)<.05f?MUTED:((delta>0)!=lower?TEAL:RED));
}
static void equipment_comparison(int it, int y) {
    if(!it){text(31,y,"NO ITEMS",MUTED,1);return;}
    Actor *a=&g.actors[g.selected];ItemDef *d=&itemDefs[it];char b[100];
    if(d->slot<0){snprintf(b,sizeof(b),"%s / WEIGHT %d",d->name,d->weight);clipped(31,y,b,68,TEXT);wrap(31,y+14,67,d->desc,MUTED);return;}
    int worn=a->gear[d->slot];ItemDef *old=&itemDefs[worn];
    clipped(31,y,d->name,68,TEXT);
    if(it>=CATALOG_ITEMS&&it<OLD_ITEMS)text(374,y,it>=CATALOG_ITEMS+16?"SIGNATURE":"SPECIAL",GOLD,1);
    else if((it>=UNIQUE_ITEMS&&it<V090_ITEMS)||regalia_owner(it)>=0)text(404,y,"NAMED",GOLD,1);
    snprintf(b,sizeof(b),"WORN: %s",worn?old->name:"EMPTY");clipped(31,y+12,b,68,MUTED);
    int cell=0;
    compare_stat(cell++,y+26,d->slot==0?"DAMAGE":d->slot==5?"CAPACITY":"ARMOR",
                 d->slot==0?old->power:d->slot==5?old->capacity:old->armor,
                 d->slot==0?d->power:d->slot==5?d->capacity:d->armor,0,0);
    compare_stat(cell++,y+26,"WEIGHT",old->weight,d->weight,0,1);
    if(d->slot==0) {
        if(fabsf(old->range-d->range)>.01f)compare_stat(cell++,y+26,"REACH",worn?old->range:1.55f,d->range,1,0);
        float before=weapon_handling(a,worn)*fmaxf(.55f,(worn?old->cooldown:1)-effective_level(a,DEXTERITY)*.003f);
        float after=weapon_handling(a,it)*fmaxf(.55f,d->cooldown-effective_level(a,DEXTERITY)*.003f);
        if(fabsf(before-after)>.01f)compare_stat(cell++,y+26,"DELAY SEC",before,after,1,1);
        if(weapon_strength(it)>level(a,STRENGTH)) {
            snprintf(b,sizeof(b),"NEEDS STR %d / HAVE %d",weapon_strength(it),effective_level(a,STRENGTH));
            clipped(31+(cell%2)*211,y+26+(cell/2)*12,b,33,RED);cell++;
        }
    } else if(d->slot==5) {
        compare_stat(cell++,y+26,"LOADED WT",encumbrance(a),backpack_load_preview(a,it),0,1);
        if(backpack_combat_penalty(worn)!=backpack_combat_penalty(it))compare_stat(cell++,y+26,"COMBAT",-backpack_combat_penalty(worn),-backpack_combat_penalty(it),0,0);
        if(backpack_load_percent(worn)!=backpack_load_percent(it))compare_stat(cell++,y+26,"CARGO WT %",backpack_load_percent(worn),backpack_load_percent(it),0,1);
        if(cell<6&&old->stealth!=d->stealth)compare_stat(cell++,y+26,"STEALTH",old->stealth,d->stealth,0,0);
    } else {
        if(old->speed!=d->speed)compare_stat(cell++,y+26,"MOVE %",old->speed*.5f,d->speed*.5f,1,0);
        if(old->stealth!=d->stealth)compare_stat(cell++,y+26,"STEALTH",old->stealth,d->stealth,0,0);
        if(old->capacity!=d->capacity)compare_stat(cell++,y+26,"CAPACITY",old->capacity,d->capacity,0,0);
        if(cell<6&&old->protection!=d->protection)compare_stat(cell++,y+26,"WEATHER",old->protection,d->protection,0,0);
    }
    if(cell<6&&d->effect>=0&&item_effect_value(it)&&(old->effect!=d->effect||item_effect_value(worn)!=item_effect_value(it))) {
        snprintf(b,sizeof(b),"%s %d",effectNames[d->effect],item_effect_value(it));
        clipped(31+(cell%2)*211,y+26+(cell/2)*12,b,33,GOLD);
    }
}
static const char *item_function(int it){
    ItemDef*d=&itemDefs[it];int f=gear_family(it);
    if(it>=CIV_ARMOR_ITEMS)return d->desc;
    if(d->slot<0)return d->desc;
    if(d->slot==0){static char pattern[112];snprintf(pattern,sizeof(pattern),"%s construction. %s +%d. %s",weapon_design_name(it),d->effect>=0?effectNames[d->effect]:"Plain",item_effect_value(it),weapon_ranged(it)?"Needs ammunition.":"Reach and weight affect handling.");return pattern;}
    if(it>=RAPIER_ITEMS&&it<RAPIER_ITEMS+24)return "Fast, precise thrusts that pierce armor.";
    if(it>=SCIMITAR_ITEMS&&it<SCIMITAR_ITEMS+24)return "Curved cutting blade; serration causes bleeding.";
    if(d->slot==0){if(weapon_ranged(it))return f>=29?"Ranged weapon. Consumes cartridges; needs a clear shot.":"Ranged weapon. Consumes bolts; keep enemies at a distance.";if(f==84)return "Heavy two-handed blade. Extra reach; needs strong arms.";if(f==85)return "Heavy impact weapon. Pierces armor and staggers enemies.";return d->effect==0?"Melee weapon. Serrated strikes cause bleeding.":d->effect==1?"Melee weapon. Piercing strikes bypass some armor.":d->effect==3?"Melee weapon. Heavy impacts can stagger enemies.":"Melee weapon. Reach, handling and strength affect combat.";}
    if(d->slot==5)return "Carries supplies or grants outfit bonuses; compare load and penalties.";
    if(d->slot==2)return "Protects your head; matching headwear helps complete a disguise.";
    if(d->slot==1)return "Protects your torso; faction clothing can form a disguise.";
    return d->slot==3?"Footwear affects protection and movement.":"Legwear protects your legs; weight affects travel.";
}
static void pack_screen(void) {
    Actor *a=&g.actors[g.selected];char b[120];int count=item_view_count(),it=count?a->pack[ui.selection].item:0;
    overlay_header("PACK & EQUIPMENT","\001 EQUIP / USE   \004 WORN GEAR   \003 PASS TO ALLY");
    Actor preview=preview_actor();int wearable=it&&itemDefs[it].slot>=0;
    if(wearable)preview.gear[itemDefs[it].slot]=it;
    portrait_scale(&preview,103,69,28,81,1);
    center(79,150,wearable?"PREVIEW":"WORN",wearable?GOLD:MUTED,1);
    int first=start_row(item_view_row(ui.selection),count,5);
    for(int j=0;j<5&&first+j<count;j++) {
        int k=item_view_slot(first+j),y=84+j*13,id=a->pack[k].item;
        if(k==ui.selection){rect(139,y-3,310,13,0x4a4030);rect(139,y-3,2,13,TEAL);}
        clipped(146,y,itemDefs[id].name,46,k==ui.selection?TEXT:MUTED);
        snprintf(b,sizeof(b),"%d",a->pack[k].qty);text(442-strlen(b)*6,y,b,GOLD,1);
    }
    snprintf(b,sizeof(b),"%d/%d SLOTS  LOAD %.0f/%d%s",count,PACK,encumbrance(a),actor_capacity(a),count&&a->stolen[ui.selection]?"  STOLEN":"");text(146,150,b,MUTED,1);
    int wornDisguise=disguise_faction(a),nextDisguise=disguise_faction(&preview);
    snprintf(b,sizeof(b),"WORN DISGUISE: %s%s",wornDisguise?factionNames[wornDisguise]:"NONE",wornDisguise&&g.society.recognized[g.selected][wornDisguise]>0?" / EXPOSED":"");clipped(31,163,b,69,TEAL);
    if(wearable){snprintf(b,sizeof(b),"PREVIEW: %s%s",nextDisguise?factionNames[nextDisguise]:"NO DISGUISE",nextDisguise&&g.society.recognized[g.selected][nextDisguise]>0?" / RECOGNIZED":"");clipped(31,175,b,69,GOLD);}
    if(!it){text(31,198,"Your pack is empty.",MUTED,1);return;}
    ItemDef*d=&itemDefs[it];
    if(!wearable){snprintf(b,sizeof(b),"%s / WEIGHT %d",d->name,d->weight);clipped(31,190,b,68,TEXT);wrap(31,204,68,item_function(it),MUTED);return;}
    int before=a->gear[d->slot];ItemDef*old=&itemDefs[before];
    compare_stat(0,190,d->slot==0?"DAMAGE":d->slot==5?"CAPACITY":"ARMOR",d->slot==0?old->power:d->slot==5?old->capacity:old->armor,d->slot==0?d->power:d->slot==5?d->capacity:d->armor,0,0);
    compare_stat(1,190,"WEIGHT",old->weight,d->weight,0,1);
    if(!d->slot){compare_stat(2,190,"REACH",before?old->range:1.55f,d->range,1,0);compare_stat(3,190,"DELAY",weapon_handling(a,before)*fmaxf(.55f,(before?old->cooldown:1)-effective_level(a,DEXTERITY)*.003f),weapon_handling(a,it)*fmaxf(.55f,d->cooldown-effective_level(a,DEXTERITY)*.003f),1,1);snprintf(b,sizeof(b),"REQUIRES STR %d / HAVE %d",weapon_strength(it),effective_level(a,STRENGTH));text(31,214,b,weapon_strength(it)>effective_level(a,STRENGTH)?RED:MUTED,1);}
    else {compare_stat(2,190,"STEALTH",old->stealth,d->stealth,0,0);compare_stat(3,190,d->slot==5?"LOADED WT":"MOVE %",d->slot==5?encumbrance(a):old->speed*.5f,d->slot==5?backpack_load_preview(a,it):d->speed*.5f,1,d->slot==5);compare_stat(4,190,"WEATHER",old->protection,d->protection,0,0);compare_stat(5,190,"CAPACITY",old->capacity,d->capacity,0,0);}
    clipped(31,226,limb_arms(a)==1&&combat_two_hands(it)?"ONE ARM: -40% DAMAGE / -25 AIM / +65% DELAY":item_function(it),69,TEXT);
}
static void skills_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[80];
    overlay_header(ui.skillPage?"SKILLS / WEAPON CLASSES":"SKILLS & TRAINING", "\014 CORE / WEAPONS   \015 SELECT   BARS SHOW NEXT LEVEL");
    if(ui.skillPage){
        if(ui.selection>=WEAPON_CLASSES)ui.selection=0;
        int equipped = weapon_class(a->gear[0]);
        for (int i = 0; i < WEAPON_CLASSES; i++) {
            int y = 84 + i * 15, lv = weapon_class_level(a, i);
            if (i == ui.selection) rect(28, y - 2, 416, 13, 0x4a4030);
            text(31, y, weaponClassNames[i], i == ui.selection ? TEXT : MUTED, 1);
            if (i == equipped) text(212, y, "EQUIPPED", TEAL, 1);
            snprintf(b, sizeof(b), "%02d", lv);
            text(319, y, b, GOLD, 1);
            float low = 12.f * (lv - 1) * (lv - 1), high = 12.f * lv * lv;
            bar(348, y + 2, 88, 3, lv >= 100 ? 100 : 100 * (g.society.weaponXP[g.selected][i] - low) / (high - low), TEAL);
        }
        int lv=weapon_class_level(a,ui.selection);snprintf(b,sizeof(b),"%s LEVEL %d / DAMAGE +%.1f%% / DELAY -%.1f%%",weaponClassNames[ui.selection],lv,(lv-1)*.25f,(lv-1)*.15f);clipped(31,187,b,68,GOLD);
        wrap(31,203,68,"Use this weapon class. Strong opponents teach faster; blocked attacks teach less. Progress persists when switching weapons.",TEXT);return;
    }
    for (int i = 0; i < SKILLS; i++) {
        int x = 31 + (i / 10) * 214, y = 82 + (i % 10) * 10;
        if (i == ui.selection)
            rect(x - 3, y - 2, 199, 10, 0x4a4030);
        text(x, y, skillNames[i], i == ui.selection ? TEXT : MUTED, 1);
        int lv = level(a, i);
        snprintf(b, sizeof(b), limb_bonus(a,i)?"%02d%+d":"%02d", lv,limb_bonus(a,i));
        text(x + 137, y, b, GOLD, 1);
        float low = 12.f * (lv - 1) * (lv - 1), high = 12.f * lv * lv;
        bar(x + 177, y + 2, 14, 3, lv >= 100 ? 100 : 100 * (a->xp[i] - low) / (high - low), TEAL);
    }
    int lv = level(a, ui.selection);
    snprintf(b, sizeof(b), "%s  LEVEL %d  %s", skillNames[ui.selection], lv,
             lv == 100 ? "MASTERED" : "LEARN BY DOING");
    text(31, 187, b, GOLD, 1);
    skill_benefit(ui.selection,b,sizeof(b));
    clipped(31,201,b,68,TEAL);
    wrap(31,215,68,skill_training(ui.selection),TEXT);
}
static void health_screen(void) {
    Actor *a=&g.actors[g.selected];DepthActor *z=&g.depth.actors[g.selected];char b[110];
    overlay_header("HEALTH & RECOVERY",z->captive?"\001 ESCAPE   \003 REPAIR LIMBS   \004 MAINTAIN GEAR":"\001 BANDAGE   \003 REPAIR LIMBS   \004 MAINTAIN GEAR");
    snprintf(b,sizeof(b),"%.16s / %s",a->name,a->dead?"DEAD":a->unconscious>0?"UNCONSCIOUS":"AWAKE");text(31,83,b,TEAL,1);
    text(177,98,"HEALTH",MUTED,1);text(334,98,"TRAUMA",MUTED,1);
    for(int i=0;i<LIMBS;i++) {
        int y=111+i*13,missing=(a->lostMask&(1<<i))&&!(a->prostheticMask&(1<<i));
        if(i==ui.selection)rect(25,y-2,425,12,0x4a4030);
        text(31,y,limbNames[i],i==ui.selection?TEXT:MUTED,1);
        bar(157,y+2,91,4,missing?0:a->health[i]*100/limb_max_health(a,i),a->health[i]<30||missing?RED:TEAL);
        if(missing)snprintf(b,sizeof(b),"LOST");else snprintf(b,sizeof(b),"%.0f/%.0f",a->health[i],limb_max_health(a,i)-z->trauma[i]);
        text(261,y,b,missing?RED:TEXT,1);
        snprintf(b,sizeof(b),"%.0f%s",z->trauma[i],depth_organic(a,i)?"":" / MECH");text(337,y,b,MUTED,1);
    }
    snprintf(b,sizeof(b),"BLOOD %.0f  BLEED %.1f  FOOD %.0f  FATIGUE %.0f",a->blood,a->bleed,a->hunger,a->fatigue);text(31,194,b,a->bleed>.01f?RED:MUTED,1);
    snprintf(b,sizeof(b),"DRESSINGS %d  GEAR CONDITION %.0f%%",inventory_count(a,MEDKIT),z->upkeep);text(31,207,b,GOLD,1);
    text(31,221,z->captive?"Work shackles away from guards.":"\006 LIMBS: fit, compare and detach replacements.",MUTED,1);
}
static void limbs_screen(void) {
    Actor *a=&g.actors[g.selected];char b[140];int surgery=ui.screen==SCREEN_SURGERY,confirm=ui.screen==SCREEN_LIMBCONFIRM;
    if(confirm){
        overlay_header(ui.limbConfirm==1?"CONFIRM LIMB REMOVAL":"CONFIRM DETACHMENT","\001 CONFIRM   \002 CANCEL");
        snprintf(b,sizeof(b),"%s / %s",a->name,limbNames[ui.limbTarget]);text(31,85,b,GOLD,1);
        wrap(31,108,65,ui.limbConfirm==1?"400 silver. This permanently removes the natural limb. Venn closes the wound. Buy a replacement before surgery; fitting costs two machine parts.":"The replacement returns to your pack. Fitting it again costs two machine parts. The limb will be missing until you fit a replacement.",TEXT);
        wrap(31,173,65,ui.limbTarget>=LLEG?"You will crawl without this leg.":"One missing arm: two-handed weapons lose 40% damage and 25 accuracy; attacks take 65% longer before injury penalties.",RED);
        text(31,222,"Circle cancels without removing anything.",MUTED,1);return;
    }
    if(ui.screen==SCREEN_LIMBFIT){
        overlay_header("FIT A REPLACEMENT","\015 SELECT   \001 FIT: 2 PARTS   \002 BACK");
        snprintf(b,sizeof(b),"%s / PARTS %d/2",limbNames[ui.limbTarget],inventory_count(a,PARTS));text(31,83,b,GOLD,1);
        int count=limb_fit_count(),first=start_row(ui.selection,count,5);
        for(int j=0;j<5&&first+j<count;j++){int n=first+j,slot=limb_fit_slot(n),y=100+j*14;row(y,n==ui.selection,itemDefs[a->pack[slot].item].name,NULL);}
        if(!count)wrap(31,106,65,"No matching limbs in your pack. Look in a town market's Mechanical Limbs tab, or buy from Venn in Dusk Haven.",TEXT);
        else{int it=a->pack[limb_fit_slot(ui.selection)].item,grade=limb_item_grade(it),old=limb_installed(a,ui.limbTarget);const LimbGrade*d=&limbGrades[grade];
            snprintf(b,sizeof(b),"FITTED: %.49s",old?itemDefs[old].name:limb_missing(a,ui.limbTarget)?"MISSING":"NATURAL LIMB - SURGERY REQUIRED");clipped(31,178,b,69,MUTED);
            const LimbGrade *o=old?&limbGrades[limb_item_grade(old)]:NULL;
            snprintf(b,sizeof(b),ui.limbTarget>=LLEG?"OLD: HP %.0f / MOVE %+d%% / LOAD %+d":"OLD: HP %.0f / STR %+d / DEX %+d",limb_missing(a,ui.limbTarget)?0:limb_max_health(a,ui.limbTarget),o?(ui.limbTarget>=LLEG?o->speed:o->strength):0,o?(ui.limbTarget>=LLEG?o->carry:o->dexterity):0);text(31,193,b,MUTED,1);
            snprintf(b,sizeof(b),ui.limbTarget>=LLEG?"NEW: HP %d / MOVE %+d%% / LOAD %+d":"NEW: HP %d / STR %+d / DEX %+d",d->hp,ui.limbTarget>=LLEG?d->speed:d->strength,ui.limbTarget>=LLEG?d->carry:d->dexterity);text(31,208,b,TEAL,1);
            text(31,222,grade==2?(ui.limbTarget>=LLEG?"LOAD: tougher and heavier; slower travel.":"POWER: tougher and heavier; less dexterity."):grade==3?(ui.limbTarget>=LLEG?"SPEED: lighter; less durability and load.":"FINESSE: lighter; less durability and strength."):grade==4?"ALL-ROUND: durable and versatile; high price.":"2 parts per fitting; old part returns to pack.",MUTED,1);
        }return;
    }
    overlay_header(surgery?"VENN / REMOVAL SURGERY":"LIMBS & REPLACEMENTS",surgery?"\015 SELECT   \001 REMOVE: 400 S   \003 BUY LIMBS":"\001 FIT   \003 DETACH   \004 REPAIR   \002 BACK");
    for(int j=0;j<4;j++){int l=LARM+j,it=limb_installed(a,l),y=87+j*27;if(j==ui.selection)rect(25,y-3,425,25,0x4a4030);
        text(31,y,limbNames[l],TEXT,1);snprintf(b,sizeof(b),"%.0f/%.0f HP",limb_missing(a,l)?0:a->health[l],limb_max_health(a,l));text(310,y,b,limb_missing(a,l)?RED:TEAL,1);
        clipped(43,y+11,it?itemDefs[it].name:limb_missing(a,l)?"MISSING":a->race==2?"NATIVE MACHINE LIMB":"NATURAL LIMB",63,it?GOLD:limb_missing(a,l)?RED:MUTED);
    }
    if(surgery){text(31,200,"PERMANENT. Buy a replacement before removal.",RED,1);text(31,215,"Dusk Haven: basic / Black Mesa: relic grade",MUTED,1);}
    else{
        snprintf(b,sizeof(b),"STR %+d  DEX %+d  MOVE %+.0f%%  LOAD %+d",limb_bonus(a,STRENGTH),limb_bonus(a,DEXTERITY),(limb_move_bonus(a)-1)*100,limb_capacity_bonus(a));text(31,200,b,TEAL,1);
        const char *status=limb_crawling(a)?"CRAWLING: fit a replacement for each missing leg.":limb_arms(a)==1?"ONE ARM: two-handed weapons are slower and weaker.":limb_arms(a)==0?"NO ARMS: weapons unusable; kicks are weakened.":"Fitting costs 2 parts; metal repairs use parts.";
        clipped(31,215,status,69,limb_crawling(a)||limb_arms(a)<2?RED:MUTED);
    }
}

static void life_screen(void){
 char b[220];if(ui.screen==SCREEN_POLITICS){int t=ui.tradeTown;TownPolitics*p=&g.life.towns[t];overlay_header("COUNCIL & SUCCESSION",p->stage==1?"\015 SELECT   \001 COMMIT   \002 CLOSE":"\001 / \002 CLOSE");clipped(31,83,towns[t].name,65,GOLD);
  if(p->stage==1){
   snprintf(b,sizeof(b),"%s / %s",politics_claimant(t,p->original),factionNames[p->original]);clipped(31,97,b,68,TEAL);
   snprintf(b,sizeof(b),"%s / %s",politics_claimant(t,p->rival),factionNames[p->rival]);clipped(31,110,b,68,GOLD);
   int side=ui.selection==1,fac=side?p->rival:p->original;
   snprintf(b,sizeof(b),"%s %d vs %d influence. %s",side?"CHALLENGER":"CARETAKER",p->support[side],p->support[!side],politics_policy_text(fac));wrap(31,125,68,b,TEXT);
   const char*labels[]={"Back caretaker: 200 silver / +30 influence","Back challenger: 200 silver / +30 influence","Relief: 4 rations / caretaker +12 influence","Leave"};for(int i=0;i<4;i++)row(166+i*13,i==ui.selection,labels[i],NULL);
   snprintf(b,sizeof(b),"ONE PLEDGE / SILVER %d/200  FOOD %d/4  %s",g.money,inventory_count(&g.actors[g.selected],RATION),p->pledge<0?"OPEN":"PLEDGED");clipped(31,222,b,68,GOLD);
  }else{snprintf(b,sizeof(b),"GOVERNMENT: %s",factionNames[g.townOwner[t]]);clipped(31,105,b,68,TEAL);wrap(31,127,68,p->policy==1?"Relief administration: food, water and medicine prices reduced 15%.":p->policy==2?"Military administration: equipment prices increased 12%.":"Trade administration: supply prices reduced 7%.",TEXT);snprintf(b,sizeof(b),"REFUGEES %d / SHIPMENTS %d",p->refugees,p->shipments);text(31,184,b,GOLD,1);wrap(31,207,68,"The former ruler and their unique equipment remain where they fell.",MUTED);}return;
 }
 overlay_header("WORLD ACTIVITY","\015 SELECT   \001 COUNCIL / MAP   \002 CLOSE");int count=life_ledger_count(),first=start_row(ui.selection,count,6);for(int j=0;j<6&&first+j<count;j++){life_ledger_label(first+j,b,sizeof(b));row(84+j*14,first+j==ui.selection,b,NULL);}life_ledger_detail(ui.selection,b,sizeof(b));wrap(31,178,68,b,TEXT);
}

static void squad_screen(void) {
    Actor *a=&g.actors[g.selected];DepthActor *z=&g.depth.actors[g.selected];char b[120];
    overlay_header("SQUAD MANAGEMENT","\005/\006 MEMBER   \014 CHANGE   \001 SET   \004 PACK");
    snprintf(b,sizeof(b),"%.16s / %s",a->name,job_status(a));clipped(31,83,b,68,TEAL);
    const char *labels[]={"Immediate job","Combat stance","Travel group","Group leader","Priority queue","Priority 1","Priority 2","Priority 3","Priority 4"};
    for(int i=0;i<9;i++) {
        const char *value="";
        if(i==0)value=jobNames[a->job];
        if(i==1)value=orderNames[z->order];
        if(i==2){snprintf(b,sizeof(b),"%d / 3",z->group+1);value=b;}
        if(i==3)value=g.depth.groupLeader[z->group]==g.selected?"THIS MEMBER":"\001 SET THIS MEMBER";
        if(i==4)value=!z->queued?"OFF / IMMEDIATE JOB":z->paused?"PAUSED":"RUNNING";
        if(i>=5)value=jobNames[z->queue[i-5]];
        row(101+i*14,i==ui.selection,labels[i],value);
    }
    rect(25,233,425,15,PANEL);
    text(31,237,a->dead?"\002 BACK   \003 DISMISS FALLEN   \010 GUIDE":"\002 BACK   \010 GUIDE   L/R CHANGES MEMBER",MUTED,1);
}
/* World overview: geography + a named destination browser, not a tiny free cursor. */
static void atlas_name(int id,char *out,int size) {
    if(id==-1)snprintf(out,size,"YOUR POSITION");
    else if(id<TOWNS)snprintf(out,size,"%s",towns[id].name);
    else living_title(id-TOWNS,out,size);
}
static void world_atlas(void) {
    ui.mapCategory=0;
    const int ox=14,oy=42,size=206;
    Actor *a=&g.actors[g.selected];
    int count=atlas_count();if(ui.mapChoice>=count)atlas_select(0);
    int selected=atlas_item(ui.mapChoice);
    rect(0,0,480,272,PANEL);
    text(14,6,"WORLD ATLAS",TEXT,2);
    text(341,8,"\003 USE CHART",MUTED,1);
    static uint32_t pixels[206*206],cacheKey;static int ready;
    uint32_t key=g.seed;
    for(unsigned i=0;i<sizeof(g.seen);i++)key=(key^g.seen[i])*16777619u;
    if(!ready||key!=cacheKey){
        static const uint32_t terrain[]={0x697060,0x80715b,0x777a70,0x4d6c62,0x64645c,0x77735e,0xa9b0ad,0x465d51,0x79665d};
        for(int y=0;y<size;y++)for(int x=0;x<size;x++){
            float wx=x*(float)WORLD/size,wy=y*(float)WORLD/size;int q=((int)wy/4)*FOG_SIZE+(int)wx/4;
            uint32_t c=0x242d2e;
            if(g.seen[q>>3]&(1<<(q&7))){
                unsigned r=0,gc=0,b=0;
                for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){uint32_t t=terrain[biome(wx+dx*26,wy+dy*26)];r+=(t>>16)&255;gc+=(t>>8)&255;b+=t&255;}
                c=((r/9)<<16)|((gc/9)<<8)|(b/9);
                if(terrain_water(wx,wy))c=0x427384;
            }
            pixels[y*size+x]=abgr(c);
        }
        cacheKey=key;ready=1;
    }
    for(int y=0;y<size;y++)memcpy(fb+(oy+y)*pitch+ox,pixels+y*size,size*sizeof(uint32_t));
#define AX(v) (ox+(int)((v)*size/WORLD))
#define AY(v) (oy+(int)((v)*size/WORLD))
    int oldL=clipLeft,oldR=clipRight,oldT=clipTop,oldB=clipBottom;
    clipLeft=ox;clipRight=ox+size;clipTop=oy;clipBottom=oy+size;
    /* Same road graph as traversal, only through surveyed terrain. */
    for(int t=1;t<TOWNS;t++){
        int near=0;float best=1e9f;
        for(int j=0;j<t;j++){float d=distance(towns[t].x,towns[t].y,towns[j].x,towns[j].y);if(d<best){best=d;near=j;}}
        int steps=(int)best+1;
        for(int k=0;k<steps;k++){
            float u=k/(float)steps,v=(k+1)/(float)steps,wx=towns[t].x+(towns[near].x-towns[t].x)*u,wy=towns[t].y+(towns[near].y-towns[t].y)*u;
            int q=((int)wy/4)*FOG_SIZE+(int)wx/4;
            if(g.seen[q>>3]&(1<<(q&7)))line(AX(wx),AY(wy),AX(towns[t].x+(towns[near].x-towns[t].x)*v),AY(towns[t].y+(towns[near].y-towns[t].y)*v),0xc1b08a);
        }
    }
    if(ui.waypoint){
        float x=AX(a->x),y=AY(a->y),dx=AX(ui.travelX)-x,dy=AY(ui.travelY)-y,steps=fmaxf(fabsf(dx),fabsf(dy));
        for(int k=0;k<steps;k+=6)line(x+dx*k/fmaxf(1,steps),y+dy*k/fmaxf(1,steps),x+dx*fminf(k+3,steps)/fmaxf(1,steps),y+dy*fminf(k+3,steps)/fmaxf(1,steps),0x72cfd2);
    }
    for(int t=0;t<TOWNS;t++)if(g.discovered[t]){
        int x=AX(towns[t].x),y=AY(towns[t].y);
        if(t>=OLD_TOWNS){diamond(x,y,5,5,INK);diamond(x,y,3,3,label_color(g.townOwner[t]));}
        else{rect(x-3,y-3,7,7,INK);frame(x-2,y-2,5,5,label_color(g.townOwner[t]));}
    }
    int px=AX(a->x),py=AY(a->y);
    triangle(px,py-6,px-5,py+4,px+5,py+4,TEXT);triangle(px,py-4,px-3,py+3,px+3,py+3,0x72cfd2);
    int sx=AX(ui.mapX),sy=AY(ui.mapY);
    frame(sx-6,sy-6,13,13,GOLD);
    /* One named callout, bounded within the map. No numeric lookup or overlapping labels. */
    char name[80];atlas_name(selected,name,sizeof(name));
    int width=text_width(name)+8;if(width>198)width=198;
    int lx=sx-width/2,ly=sy-25;if(lx<ox+3)lx=ox+3;if(lx+width>ox+size-3)lx=ox+size-3-width;
    if(ly<oy+3)ly=sy+12;if(ly>oy+size-16)ly=oy+size-16;
    line(sx,sy+(ly>sy?7:-7),sx,ly+(ly>sy?0:13),GOLD);
    rect(lx,ly,width,13,INK);clipped(lx+4,ly+3,name,31,selected==-1?0x72cfd2:GOLD);

    rect(18,230,51,15,INK);line(22,233,55,233,MUTED);text(22,237,"250m",MUTED,1);
    clipLeft=oldL;clipRight=oldR;clipTop=oldT;clipBottom=oldB;
    frame(ox-1,oy-1,size+2,size+2,EDGE);
    text(198,8,"N",MUTED,1);line(211,5,211,20,MUTED);line(211,5,208,9,MUTED);line(211,5,214,9,MUTED);
    text(235,35,"KNOWN TOWNS",GOLD,1);text(332,35,"\005/\006 PAGE",MUTED,1);
    char info[100];snprintf(info,sizeof(info),"%d/%d",ui.mapChoice+1,count);text(470-text_width(info),35,info,MUTED,1);
    int first=ui.mapChoice<7?0:ui.mapChoice-6;
    for(int j=0;j<7&&first+j<count;j++){
        int n=first+j,id=atlas_item(n),y=54+j*16;atlas_name(id,name,sizeof(name));
        if(n==ui.mapChoice){rect(232,y-3,236,14,0x45473c);rect(232,y-3,2,14,GOLD);}
        if(id==-1)triangle(242,y,239,y+6,245,y+6,0x72cfd2);
        else if(id<OLD_TOWNS)frame(239,y+1,6,6,label_color(g.townOwner[id]));
        else if(id<TOWNS)diamond(242,y+4,4,4,label_color(g.townOwner[id]));else diamond(242,y+4,3,3,GOLD);
        clipped(252,y,name,35,n==ui.mapChoice?TEXT:MUTED);
    }
    if(count==1)text(252,74,ui.mapCategory?"NO ACTIVE EVENTS":"NO KNOWN TOWNS",MUTED,1);
    if(first>0)triangle(462,49,459,52,465,52,MUTED);
    if(first+7<count)triangle(462,165,459,162,465,162,MUTED);
    line(234,169,467,169,EDGE);
    if(selected==-1){text(235,180,a->name,0x72cfd2,1);text(235,195,regionNames[biome(a->x,a->y)],MUTED,1);}
    else {
        if(selected<TOWNS)snprintf(info,sizeof(info),"HELD BY %s",factionNames[g.townOwner[selected]]);
        else {WorldEvent*e=&g.living.events[selected-TOWNS];snprintf(info,sizeof(info),"%s",e->type==1||e->type>=5?"COMBAT":e->type==2?"SUPPLY DELIVERY":e->type==4?"RESCUE":e->type==3?"SEARCH":"ESCORT");}
        clipped(235,180,info,38,GOLD);
        float dx=ui.mapX-a->x,dy=ui.mapY-a->y;const char*dir=fabsf(dx)<fabsf(dy)*.4f?(dy<0?"N":"S"):fabsf(dy)<fabsf(dx)*.4f?(dx<0?"W":"E"):dy<0?(dx<0?"NW":"NE"):(dx<0?"SW":"SE");
        snprintf(info,sizeof(info),"%.0fm %s / DIRECT DISTANCE",distance(a->x,a->y,ui.mapX,ui.mapY),dir);text(235,195,info,MUTED,1);
    }
    triangle(239,214,235,221,243,221,0x72cfd2);text(250,215,"YOU",0x72cfd2,1);
    frame(289,215,6,6,TEXT);text(301,215,"TOWN",TEXT,1);
    diamond(346,218,4,4,GOLD);text(354,215,"CAPITAL",TEXT,1);

    line(235,235,249,235,0xc1b08a);text(255,232,"ROAD",MUTED,1);text(301,232,"DARK: UNEXPLORED",MUTED,1);
    text(14,257,selected==-1?"\011 SELECT   \004 LOCAL VIEW   \002 BACK":"\011 SELECT   \001 PIN   \004 LOCAL VIEW   \002 BACK",TEXT,1);
#undef AX
#undef AY
}
static void map_screen(void) {
    if(!ui.mapZoom){world_atlas();return;}
    overlay_header(ui.mapZoom ? "LOCAL MAP / BEARINGS" : "WORLD ATLAS / FRONTIER",
                   "\011 CURSOR  \001 PIN  \005/\006 TOWNS  \004 ZOOM  \003 CHART");
    float nearest=1e9f;
    for(int t=0;t<TOWNS;t++){float d=distance(ui.mapX,ui.mapY,towns[t].x,towns[t].y);if(d<nearest){nearest=d;ui.mapTown=t;}}
    int ox = ui.mapZoom?29:105, oy = 82, mw = ui.mapZoom?288:136, mh = 136;
    rect(29,82,288,136,0x252e2c);
    int spanX = ui.mapZoom ? 384 : WORLD, spanY = ui.mapZoom ? 181 : WORLD;
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
    static uint32_t atlas[288 * 136], cachedHash;
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
                static const uint32_t terrain[]={0x606553,0x72634e,0x858477,0x4d655a,0x54534c,0x69675b,0x979c99,0x405349,0x71534c};
                uint32_t c = terrain[biome(wx, wy)];
                unsigned rr=0,gg=0,bb=0;
                for(int sy=-1;sy<=1;sy++)for(int sx=-1;sx<=1;sx++){uint32_t z=terrain[biome(wx+sx*18,wy+sy*18)];rr+=(z>>16)&255;gg+=(z>>8)&255;bb+=z&255;}
                c=((rr/9)<<16)|((gg/9)<<8)|(bb/9);
                if(ui.mapZoom && terrain_height(wx,wy)/4!=terrain_height(wx+spanX/mw+1,wy)/4)c=shade(c,.8f);
                if (terrain_water(wx, wy))
                    c = 0x426e80;
                else if (terrain_road(wx, wy))
                    c = shade(c, 1.3f);
                atlas[y * mw + x] = abgr(shade(c, known ? 1 : .38f));
            }
        cachedHash = h;
        ready = 1;
    }
    for (int y = 0; y < mh; y++)
        memcpy(fb + (oy + y) * pitch + ox, atlas + y * mw, mw * sizeof(uint32_t));
    int oldL=clipLeft,oldR=clipRight,oldT=clipTop,oldB=clipBottom;
    clipLeft=ox;clipRight=ox+mw;clipTop=oy;clipBottom=oy+mh;
    for(int t=1;t<TOWNS;t++) {
        int near=0;float best=1e9f;
        for(int j=0;j<t;j++){float d=distance(towns[t].x,towns[t].y,towns[j].x,towns[j].y);if(d<best){best=d;near=j;}}
        int steps=(int)best+1;
        for(int k=0;k<steps;k++) {
            float u=k/(float)steps,v=(k+1)/(float)steps;
            float wx=towns[t].x+(towns[near].x-towns[t].x)*u,wy=towns[t].y+(towns[near].y-towns[t].y)*u;
            int q=((int)wy/4)*FOG_SIZE+(int)wx/4;
            if(INMAP(wx,wy) && (g.seen[q>>3]&(1<<(q&7))))
                line(MAPX(wx),MAPY(wy),MAPX(towns[t].x+(towns[near].x-towns[t].x)*v),
                     MAPY(towns[t].y+(towns[near].y-towns[t].y)*v),0xa29670);
        }
    }
    for (int t = 0; t < TOWNS; t++)
        if (g.discovered[t] && INMAP(towns[t].x, towns[t].y)) {
            int x = MAPX(towns[t].x), y = MAPY(towns[t].y);
            if(t>=OLD_TOWNS){diamond(x,y,5,5,INK);diamond(x,y,3,3,label_color(g.townOwner[t]));}
            else{rect(x - 2, y - 2, 5, 5, factionColors[g.townOwner[t]]);frame(x - 2, y - 2, 5, 5, GOLD);}
            char label[28];snprintf(label,sizeof(label),"%02d",t+1);
            if(ui.mapZoom||t==ui.mapTown)text(x+4,y-6,label,t==ui.mapTown?TEXT:GOLD,1);
            if(t==ui.mapTown)frame(x-4,y-4,9,9,TEXT);
        }
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && !g.actors[i].dead && INMAP(g.actors[i].x, g.actors[i].y))
            rect(MAPX(g.actors[i].x) - 1, MAPY(g.actors[i].y) - 1, 3, 3, TEAL);
    for(int i=0;i<LANDMARKS;i++)if(INMAP(g.depth.siteX[i],g.depth.siteY[i])) {
        int x=MAPX(g.depth.siteX[i]),y=MAPY(g.depth.siteY[i]);diamond(x,y,4,3,0x151b19);diamond(x,y,2,2,GOLD);
        if(ui.mapZoom && distance(ui.mapX,ui.mapY,g.depth.siteX[i],g.depth.siteY[i])<12)
            clipped(x>ox+120?x-110:x+6,y+5,landmarkNames[i],18,GOLD);
    }
    for(int i=0;i<3;i++) {int t=regional_town(i);if(g.discovered[t] && INMAP(towns[t].x,towns[t].y))
        text(MAPX(towns[t].x)+5,MAPY(towns[t].y)-8,g.regions.outcome[i]?"+":"!",g.regions.outcome[i]?TEAL:RED,1); }
    if(INMAP(g.regions.campX,g.regions.campY) && (ui.mapTown==8 || g.discovered[8])) {
        int cx=MAPX(g.regions.campX),cy=MAPY(g.regions.campY);diamond(cx,cy,4,3,g.regions.outcome[1]?TEAL:RED);
        if(ui.mapZoom)clipped(cx+6,cy-9,g.regions.outcome[1]?"TOLL AGREEMENT":"TOLL CAMP",18,TEXT);
    }
    if (g.baseExists && INMAP(g.baseX, g.baseY))
        frame(MAPX(g.baseX) - 3, MAPY(g.baseY) - 3, 7, 7, TEAL);
    Actor *leader = &g.actors[g.selected];
    int youx = MAPX(leader->x), youy = MAPY(leader->y);
    int x = MAPX(ui.mapX), y = MAPY(ui.mapY);
    if (INMAP(leader->x, leader->y)) {
        if(!ui.waypoint) {
            float dx=x-youx,dy=y-youy,steps=fmaxf(fabsf(dx),fabsf(dy));
            for(int k=0;k<steps;k+=6) line(youx+dx*k/fmaxf(1,steps),youy+dy*k/fmaxf(1,steps),
                youx+dx*fminf(k+3,steps)/fmaxf(1,steps),youy+dy*fminf(k+3,steps)/fmaxf(1,steps),GOLD);
        }
        if (ui.waypoint) {
            int dx = MAPX(ui.travelX), dy = MAPY(ui.travelY);
            /* Clip the route to the visible map rectangle. */
            float vx = dx - youx, vy = dy - youy, t = 1;
            if (vx > 0)
                t = fminf(t, (ox + mw - 1 - youx) / vx);
            else if (vx < 0)
                t = fminf(t, (ox - youx) / vx);
            if (vy > 0)
                t = fminf(t, (oy + mh - 1 - youy) / vy);
            else if (vy < 0)
                t = fminf(t, (oy - youy) / vy);
            line(youx, youy, youx + vx * t, youy + vy * t, TEAL);
        }
        diamond(youx, youy, 5, 4, TEAL);
        frame(youx - 5, youy - 5, 11, 11, TEXT);
    }
    line(x - 5, y, x + 5, y, TEXT);
    line(x, y - 5, x, y + 5, TEXT);
    clipLeft=oldL;clipRight=oldR;clipTop=oldT;clipBottom=oldB;
    frame(ox - 1, oy - 1, mw + 2, mh + 2, EDGE);
    text(328,84,"NEAREST TOWN",GOLD,1);
    char info[80];snprintf(info,sizeof(info),"%02d %s",ui.mapTown+1,g.discovered[ui.mapTown]?towns[ui.mapTown].name:"UNCHARTED");wrap(328,100,19,info,TEXT);
    text(328,131,"CONTROL",MUTED,1);wrap(328,143,19,g.discovered[ui.mapTown]?factionNames[g.townOwner[ui.mapTown]]:"Unknown",GOLD);
    text(328,174,"MAP KEY",MUTED,1);
    text(328,187,"01 TOWN / + PIN",GOLD,1);
    text(328,198,"PALE: ROAD",MUTED,1);
    text(328,209,"TEAL: YOU",TEAL,1);
    if(!ui.mapZoom){text(38,89,"N",TEXT,1);line(41,103,41,116,TEXT);line(41,103,37,108,TEXT);line(41,103,45,108,TEXT);}
    char b[90];
    snprintf(b, 90, "%s %.0fm  / %s", ui.waypoint ? "MAP PIN" : "BEARING",
             distance(leader->x, leader->y, ui.waypoint ? ui.travelX : ui.mapX,
                      ui.waypoint ? ui.travelY : ui.mapY),
             regionNames[biome(ui.mapX, ui.mapY)]);
    text(31, 222, b, TEAL, 1);
    rect(25, 233, 425, 15, PANEL);
    text(31, 237, "\002 BACK   PALE: ROAD   GOLD: SITE   DASHED: BEARING", GOLD, 1);
#undef MAPX
#undef MAPY
#undef INMAP
}

static void trade_screen(void) {
    char b[100], v[70];
    static const char *titles[] = {"BUY / ALL STOCK", "BUY / SUPPLIES", "BUY / WEAPONS", "BUY / ARMOR",
                                   "BUY / BACKPACKS", "BUY / MECHANICAL LIMBS"};
    overlay_header(ui.sell ? "SELL / CARRIED" : titles[ui.shopFilter],
                   ui.sell ? "\014 BUY-SELL   \001 SELL ONE"
                           : "\014 BUY-SELL   \005/\006 FILTER   \001 BUY ONE");
    int count = ui.sell ? item_view_count() : shop_count(), first = start_row(ui.sell ? item_view_row(ui.selection) : ui.selection, count, 6);
    for (int j = 0; j < 6 && first + j < count; j++) {
        int i = ui.sell ? item_view_slot(first+j) : first+j, it = ui.sell ? g.actors[g.selected].pack[i].item : shop_item(i);
        snprintf(b,100,"%.33s",it?itemDefs[it].name:"--");row(84+j*13,i==ui.selection,b,NULL);
        snprintf(v,sizeof(v),"%d",ui.sell?g.actors[g.selected].pack[i].qty:g.stock[ui.tradeTown][it]);text(263,84+j*13,v,MUTED,1);
        snprintf(v,sizeof(v),"%d S",item_price(ui.tradeTown,it,ui.sell));text(301,84+j*13,v,GOLD,1);
        int pct=market_percent(ui.tradeTown,it,ui.sell);snprintf(v,sizeof(v),"%+d%%",pct);text(400,84+j*13,v,pct==0?MUTED:((ui.sell?pct>0:pct<0)?0x73c58b:0xde8375),1);
    }
    int it = ui.sell ? g.actors[g.selected].pack[ui.selection].item : shop_item(ui.selection);
    equipment_comparison(it, 173);
    text(290,237,"GREEN GOOD / RED POOR",MUTED,1);
    snprintf(b, sizeof(b), "%d SILVER", g.money);
    text(360, 45, b, GOLD, 1);
}

static void build_screen(void) {
    char b[100];
    overlay_header("BUILD AN OUTPOST", "\001 PLACE BESIDE SELECTED MEMBER. OUTSIDE VILLAGES ONLY.");
    static const int iron[] = {0, 0, 0, 3, 1, 0, 0, 3};
    for (int i = 0; i < buildCount; i++) {
        snprintf(b, 100, "%d TIMBER  %d IRON", build_wood_cost(i), iron[i]);
        row(86 + i * 17, i == ui.selection, buildNames[i], b);
    }
    Actor *a = &g.actors[g.selected];
    snprintf(b, sizeof(b), "HELD: %d TIMBER / %d IRON / TIER %d / PLACE AT +2m EAST",
             inventory_count(a, WOOD), inventory_count(a, IRONBAR), g.tech);
    text(31, 225, b, MUTED, 1);
}
static const char *craft_station_name(int type){return type==FORGE?"FORGE":type==CAMPFIRE?"CAMPFIRE":"RESEARCH BENCH";}
static int craft_readiness(const CraftRequirements*r,char*out,int size){
    Actor*a=&g.actors[g.selected];int station=nearby_workstation(r->station);
    if(g.tech<r->tech){snprintf(out,size,"Research tier %d required (have %d).",r->tech,g.tech);return 0;}
    if(level(a,SMITHING)<r->smithing){snprintf(out,size,"Smithing %d required (have %d).",r->smithing,level(a,SMITHING));return 0;}
    for(int i=0;i<r->count;i++)if(inventory_count(a,r->item[i])<r->need[i]){
        if(i==0&&r->alternate){if(inventory_count(a,r->alternate)>=r->alternateNeed)continue;snprintf(out,size,"Need %d grain OR %d raw meat.",r->need[0],r->alternateNeed);return 0;}
        snprintf(out,size,"Missing %d %s.",r->need[i]-inventory_count(a,r->item[i]),itemDefs[r->item[i]].name);return 0;
    }
    if(station<0){snprintf(out,size,"Move beside a %s.",craft_station_name(r->station));return 0;}
    if(!depth_build_ready(station)){snprintf(out,size,"Finish or repair the workstation.");return 0;}
    if(r->station!=BENCH&&g.buildings[station].owner>=0&&g.depth.buildings[station].fuel<1){snprintf(out,size,"Add coal or timber in Camp upkeep.");return 0;}
    Actor copy=*a;for(int i=0;i<r->count;i++){int it=r->item[i],qty=r->need[i];if(i==0&&r->alternate&&inventory_count(&copy,it)<qty){it=r->alternate;qty=r->alternateNeed;}inventory_remove(&copy,it,qty);}
    if(!inventory_add(&copy,r->output,r->qty)){snprintf(out,size,"Make room for the crafted item.");return 0;}
    snprintf(out,size,"Ready to craft.");return 1;
}
static void craft_screen(void) {
    overlay_header("WORKSHOP", "\001 CRAFT RECIPE   \003 EQUIPMENT FORGE");
    for(int i=0;i<recipeCount;i++){
        int y=86+i*16;if(i==ui.selection){rect(25,y-3,178,14,0x4a4030);rect(25,y-3,2,14,TEAL);}
        text(31,y,recipeNames[i],i==ui.selection?TEXT:MUTED,1);
    }
    line(213,83,213,218,EDGE);CraftRequirements r;if(!recipe_requirements(ui.selection,&r))return;
    Actor*a=&g.actors[g.selected];char b[120];snprintf(b,sizeof(b),"MAKES %d %s",r.qty,itemDefs[r.output].name);clipped(225,84,b,36,GOLD);
    text(225,101,"INGREDIENT",MUTED,1);text(389,101,"HAVE/NEED",MUTED,1);
    int row=0;
    for(int i=0;i<r.count;i++){
        int y=115+row++*13,have=inventory_count(a,r.item[i]);clipped(225,y,itemDefs[r.item[i]].name,24,TEXT);snprintf(b,sizeof(b),"%d/%d",have,r.need[i]);text(389,y,b,have>=r.need[i]?0x73c58b:RED,1);
        if(i==0&&r.alternate){y=115+row++*13;snprintf(b,sizeof(b),"OR %s",itemDefs[r.alternate].name);clipped(225,y,b,24,TEXT);int alt=inventory_count(a,r.alternate);snprintf(b,sizeof(b),"%d/%d",alt,r.alternateNeed);text(389,y,b,alt>=r.alternateNeed?0x73c58b:RED,1);}
    }
    snprintf(b,sizeof(b),"RESEARCH %d/%d",g.tech,r.tech);text(225,158,b,g.tech>=r.tech?0x73c58b:RED,1);
    int station=nearby_workstation(r.station);snprintf(b,sizeof(b),"%s: %s",craft_station_name(r.station),station>=0?"IN REACH":"TOO FAR");text(225,174,b,station>=0?TEAL:RED,1);
    if(r.station==BENCH)snprintf(b,sizeof(b),"FUEL: NOT REQUIRED");
    else if(station<0)snprintf(b,sizeof(b),"OWNED STATION: 1 FUEL / CRAFT");
    else if(g.buildings[station].owner<0)snprintf(b,sizeof(b),"FUEL: TOWN SUPPLIED");
    else snprintf(b,sizeof(b),"FUEL %.0f/1",floorf(g.depth.buildings[station].fuel));
    text(225,188,b,station>=0&&r.station!=BENCH&&g.buildings[station].owner>=0?(g.depth.buildings[station].fuel>=1?0x73c58b:RED):MUTED,1);
    int ready=craft_readiness(&r,b,sizeof(b));text(225,204,ready?"\001 READY TO CRAFT":"CANNOT CRAFT YET",ready?TEAL:RED,1);clipped(31,222,b,69,ready?TEAL:RED);
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
    snprintf(b, 100, "HELD: %d/1 RELIC  %d/%d SILVER", inventory_count(&g.actors[g.selected], RELIC), g.money,
             research_cost());
    text(33, 194, g.tech < 3 ? b : "RESEARCH COMPLETE", GOLD, 1);
    text(33, 214,
         g.tech >= 3                        ? "ALL TECHNOLOGY TIERS UNLOCKED"
         : nearby_station(BENCH, 1.7f) >= 0 ? "\001 RESEARCH NEXT TIER - BENCH READY"
                                            : "WALK TO A RESEARCH BENCH FIRST",
         TEXT, 1);
}
static void menu_screen(void) {
    overlay_header("PAUSED", "\005/\006 TAB   \015 CHOOSE   \001 OPEN   \007 RESUME");
    static const char *tabs[] = {"PAUSE", "SQUAD", "WORLD"};
    for (int i = 0; i < 3; i++) {
        int x = 28 + i * 142;
        if (i == ui.menuTab) rect(x, 80, 136, 17, 0x494132);
        center(x + 68, 85, tabs[i], i == ui.menuTab ? TEAL : MUTED, 1);
    }
    int count = pause_menu_count();
    for (int i = 0; i < count; i++) {
        char value[24] = "";
        if (ui.menuTab == 0 && i == 3) snprintf(value, sizeof(value), "%dX", ui.speed);
        row(106 + i * 16, ui.selection == i, pause_menu_label(i), value);
    }
}
static void theft_screen(void) {
    Actor *target=&g.actors[ui.lootActor];char b[100],v[30];
    overlay_header("PICKPOCKET", "\015 SELECT ITEM   \001 STEAL ONE   \002 CANCEL");
    snprintf(b,sizeof(b),"%.24s / %.24s",target->name,factionNames[target->faction]);text(31,85,b,GOLD,1);
    int count=item_view_count(),first=start_row(item_view_row(ui.selection),count,5);
    for(int j=0;j<5 && first+j<count;j++) {
        int k=item_view_slot(first+j),it=target->pack[k].item;
        snprintf(v,sizeof(v),"%d   %d KG",target->pack[k].qty,itemDefs[it].weight);
        row(103+j*13,k==ui.selection,itemDefs[it].name,v);
    }
    if(!count) text(31,110,"EMPTY POCKETS",MUTED,1);
    snprintf(b,sizeof(b),"SUCCESS %.0f%%   %s   %d WITNESSES",100*theft_chance(ui.lootActor,ui.selection),
             theft_behind(ui.lootActor)?"BEHIND TARGET":"IN THEIR VIEW",theft_witnesses(ui.lootActor));
    text(31,178,b,TEAL,1);
    text(31,195,"HEAVY / VALUABLE ITEMS ARE HARDER TO LIFT.",MUTED,1);
    text(31,212,"CAUGHT: +80 BOUNTY / -18 FACTION REPUTATION",RED,1);
}

static void help_screen(void) {
    overlay_header("FIELD GUIDE", "\005/\006 CHAPTER   \014 MORE TEXT   \002 BACK");
    char b[100];
    snprintf(b, 100, "%02d / %02d   %s", ui.selection + 1, GUIDE_PAGES, guideTitles[ui.selection]);
    text(31, 86, b, TEAL, 1);
    int oldT=clipTop,oldB=clipBottom;clipTop=106;clipBottom=221;
    int end=wrap(31,106-ui.scroll,68,guideBodies[ui.selection],TEXT);
    ui.helpTopic=end+ui.scroll-221;if(ui.helpTopic<0)ui.helpTopic=0;
    clipTop=oldT;clipBottom=oldB;
    if(ui.helpTopic){snprintf(b,sizeof(b),"TEXT %d / %d    \014 CONTINUE",1+(ui.scroll+109)/110,1+(ui.helpTopic+109)/110);text(31,226,b,GOLD,1);}
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
        a.gear[0] = gear_id(w[ui.createOrigin], ui.createOrigin==1?1:0, 1);
        a.gear[1] = gear_id(t[ui.createOrigin], ui.createOrigin==1?1:0, 0);
        a.gear[2] = gear_id(h[ui.createOrigin], 0, 1);
        a.gear[3] = gear_id(ui.createOrigin==1?63:60, 0, 0);
        a.gear[4] = gear_id(ui.createOrigin==1?68:69, 0, 0);
        a.gear[5] = gear_id(ui.createOrigin==2?79:76, 0, 0);
        species_start(&a,ui.createOrigin,ui.createRace);
    }
    int bareHead=ui.selection>=2&&ui.selection<=8;
    if(bareHead)a.gear[2]=0;
    portrait_scale(&a, 156, 89, 30, 80, 1);
    center(108, 173, bareHead?"HEADWEAR HIDDEN":ui.createName, TEAL, 1);
    char stats[100];const SpeciesDef *species=&speciesDefs[species_index(&a)];
    snprintf(stats,sizeof(stats),"HP %d  LOAD %d",species->health,actor_capacity(&a));text(30,186,stats,TEAL,1);
    snprintf(stats,sizeof(stats),"BASE MOVE %d%% FOOD %d%%",(int)roundf(species->move*100),(int)roundf(species->food*100));text(30,198,stats,MUTED,1);
    const char *labels[] = {
        "NAME", "ORIGIN",    "SPECIES", "BUILD", "SKIN",
        "HAIR", "HAIR TONE", "BEARD",    "SCARS", ui.titleChoice ? "KEEP CHANGES" : "ENTER THE FRONTIER"};
    for (int i = 0; i < 10; i++) {
        int y = 82 + i * 13;
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
    snprintf(stats,sizeof(stats),"STR %d  DEX %d  TOUGH %d  DODGE %d  ATH %d  SNEAK %d  ENG %d",level(&a,STRENGTH),level(&a,DEXTERITY),level(&a,TOUGHNESS),level(&a,DODGE),level(&a,ATHLETICS),level(&a,STEALTH),level(&a,ENGINEERING));
    text(30,213,stats,TEAL,1);
    clipped(30,225,species->trait,69,TEXT);
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
    portrait_scale(a, 150, 110, 30, 84, 1);
    for (int i = 0; i < GEARS; i++) {
        int y = 86 + i * 18;
        if (i == ui.selection)
            rect(195, y - 3, 252, 16, 0x3c4034);
        text(202, y, slotNames[i], MUTED, 1);
        clipped(246, y, itemDefs[a->gear[i]].name, 33, i == ui.selection ? TEXT : GOLD);
    }
    int it = a->gear[ui.selection];
    char b[100];
    if(!it) snprintf(b,sizeof(b),"%s: EMPTY",slotNames[ui.selection]);
    else if(item_effect_value(it))snprintf(b,sizeof(b),"%s / WEIGHT %d / %s %d",slotNames[ui.selection],itemDefs[it].weight,
        effectNames[itemDefs[it].effect],item_effect_value(it));
    else snprintf(b,sizeof(b),"%s / WEIGHT %d",slotNames[ui.selection],itemDefs[it].weight);
    clipped(30, 212, b, 68, TEAL);
}
static void requirement_cell(int cell,const char*label,int need,int have){
    int x=29+(cell%2)*211,y=177+(cell/2)*13;char b[60];clipped(x,y,label,22,TEXT);snprintf(b,sizeof(b),"%d/%d",have,need);text(x+138,y,b,have>=need?0x73c58b:RED,1);
}
static void forge_screen(void) {
    overlay_header("EQUIPMENT FORGE", "\014 CONFIGURE   \001 CHOOSE   \003 RECIPES");
    int it=forge_selection_item();ItemDef*d=&itemDefs[it];Actor a=preview_actor();a.gear[d->slot]=it;
    portrait_scale(&a,150,68,30,80,1);
    char summary[60];snprintf(summary,sizeof(summary),"%s %d / WEIGHT %d",d->slot?"ARMOR":"DAMAGE",d->slot?d->armor:d->power,d->weight);text(30,152,summary,TEXT,1);
    const char*labels[]={"SLOT","DESIGN","MATERIAL","QUALITY","FORGE THIS PIECE"};
    const char*values[]={slotNames[ui.forgeSlot],forge_selection_name(),materialNames[ui.forgeMaterial],qualityNames[ui.forgeQuality],"\001"};
    for(int i=0;i<5;i++){int y=84+i*14;if(i==ui.selection)rect(195,y-3,253,13,0x3c4034);text(202,y,labels[i],MUTED,1);text(441-strlen(values[i])*6,y,values[i],GOLD,1);}
    text(29,163,"REQUIREMENTS                HAVE/NEED",MUTED,1);
    CraftRequirements r;if(!forge_requirements(it,&r))return;Actor*worker=&g.actors[g.selected];int cell=0;
    for(int i=0;i<r.count;i++)requirement_cell(cell++,itemDefs[r.item[i]].name,r.need[i],inventory_count(worker,r.item[i]));
    requirement_cell(cell++,"Research tier",r.tech,g.tech);requirement_cell(cell++,"Smithing",r.smithing,level(worker,SMITHING));
    int station=nearby_workstation(FORGE);if(station>=0&&g.buildings[station].owner>=0)requirement_cell(cell++,"Station fuel",1,(int)g.depth.buildings[station].fuel);
    char b[120],reason[90];int ready=craft_readiness(&r,reason,sizeof(reason));
    snprintf(b,sizeof(b),"FORGE %s / %s",station>=0?"IN REACH":"TOO FAR",reason);clipped(29,226,b,69,ready?TEAL:RED);
}
static void dialogue_screen(void) {
    int n = ui.dialogActor;
    if (n < 0 || n >= g.actorCount)
        return;
    Actor *a = &g.actors[n];
    overlay_header(a->name, "\001 REPLY   \015 CHOOSE   \002 LEAVE   \010 GUIDE");
    portrait_scale(a, 94, 108, 28, 83, 1);
    clipped(30, 204, npc_role_name(a), 16, TEAL);
    clipped(30, 218, factionNames[a->faction], 16, factionColors[a->faction]);
    wrap(134, 85, 50, dialogue_line(n, ui.dialogPage), TEXT);
    const char *work[] = {"Carry a dispatch - 220 silver", "Bring eight ore - 240 silver",
                          "Defeat three foes - 420 silver", "Recover a relic - 550 silver", "Freight delivery - deposit required", "Caravan escort - wages + bonus", "Mercenary patrol - wages + bonus", "Named bounty - bring the prisoner", "Back"};
    const char *prison[] = {"Pay my bail.", "Try the lock. Requires tools.", "Wait out the sentence."};
    int jailed = a->prisonTime > 0 && n == g.selected,
        count = jailed               ? 3
                : ui.dialogPage == 1 ? 9
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
    text(31, 221, "AUTOSAVE IS SEPARATE FROM YOUR MANUAL SLOTS.", TEAL, 1);
}
static void regional_screen(void) {
    char b[160];int r=ui.selection,c=g.regions.outcome[r]?g.regions.outcome[r]-1:ui.regionChoice;
    overlay_header("JOURNAL / LOCAL CONFLICTS", "\005/\006 CONTRACTS   \015 REGION   \003 LOCATE");
    for(int i=0;i<3;i++)row(83+i*14,i==r,regional_names[i],g.regions.outcome[i]?"RESOLVED":"OPEN");
    wrap(31,129,68,regional_story(r),TEXT);
    snprintf(b,sizeof(b),"%s%s",g.regions.outcome[r]?"AGREEMENT: ":"\014 ",regional_choice(r,c));text(31,166,b,GOLD,1);
    if(!g.regions.outcome[r]){regional_requirements(r,c,b,sizeof(b));text(31,181,b,MUTED,1);}
    wrap(31,g.regions.outcome[r]?185:198,68,regional_result(r,c),TEAL);
    rect(25,233,425,15,PANEL);
    snprintf(b,sizeof(b),"\002 BACK   %s",g.regions.outcome[r]?"THIS AGREEMENT IS PERMANENT":"\001 COMMIT AT TOWN (PERMANENT)");text(31,237,b,GOLD,1);
}
static void actions_screen(void){
    int count=nearby_actions(),first=start_row(ui.selection,count,5);char b[100];
    int height=43+13*(count<5?count:5),top=250-height;
    panel(14,top,286,height);text(25,top+9,ui.paused?"NEARBY / QUEUE WHILE PAUSED":"NEARBY ACTIONS",GOLD,1);
    for(int j=0;j<5&&first+j<count;j++){int k=first+j,y=top+26+j*13;if(k==ui.selection)rect(21,y-2,272,12,0x4a4030);nearby_action_label(k,b,sizeof(b));clipped(25,y,b,44,k==ui.selection?TEXT:MUTED);}
    snprintf(b,sizeof(b),"\001 %s  \006 %s  \003 CLEAR (%d)",ui.paused?"QUEUE":"DO",ui.paused?"RESUME":"PAUSE",g.society.queueCount[g.selected]);text(25,237,b,MUTED,1);
}
static void engage_screen(void){
    overlay_header("CHOOSE OPPONENT","\015 SELECT   \001 ATTACK   \002 CANCEL");char v[40];int count=combat_candidates(),first=start_row(ui.selection,count,7);
    for(int j=0;j<7&&first+j<count;j++){int n=combat_candidate(first+j),rel=actor_relation(n);snprintf(v,sizeof(v),"%s  %.1fm",rel<0?"HOSTILE":rel>0?"FRIENDLY":"NEUTRAL",depth_distance(&g.actors[g.selected],&g.actors[n]));row(85+j*16,first+j==ui.selection,g.actors[n].name,v);}
    if(!count)text(31,96,"Nobody within 14m.",MUTED,1);
    if(ui.attackFocus>=0&&actor_relation(ui.attackFocus)>=0)text(31,211,"ASSAULT: +100 BOUNTY / -20 FACTION STANDING",RED,1);
    else text(31,211,"Close to weapon reach to strike. Square stops attacking.",MUTED,1);
}
static void journal_screen(void) {
    if(ui.journalTab==1){regional_screen();return;}
    if(ui.journalTab>1)ui.journalTab=0;
    overlay_header("THE ROAD JOURNAL", "\005/\006 CONFLICTS   \001 CLAIM   \003 LOCATE");
    char b[180], v[40];
    int first = start_row(ui.selection, CONTRACTS, 6);
    for (int j = 0; j < 6; j++) {
        int k = first + j;
        Contract *c = &g.contracts[k];
        if (c->active) {
            snprintf(b, sizeof(b), "%02d %s", k + 1, towns[c->town].name);
            snprintf(v, 40, c->active==3 ? "FAILED" : c->completed ? "COMPLETE" : "%d / %d  +%d",
                     c->item ? inventory_count(&g.actors[g.selected], c->item) : c->progress, c->needed,
                     c->reward);
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
    overlay_header("FLAGS OF THE FRONTIER", "FACTION COLORS / ! HOSTILE TO YOU / YOUR STANDING");
    char b[100];
    for (int f = 0; f < FACTIONS; f++) {
        int townsHeld = 0;
        for (int t = 0; t < TOWNS; t++)
            if (g.townOwner[t] == f)
                townsHeld++;
        snprintf(b, 100, "%d TOWNS  %+d REP  %d WANTED", townsHeld, g.rep[f], wanted_amount(&g.actors[g.selected],f));
        row(80 + f * 12, ui.selection == f, factionNames[f], b);
        rect(19,80+f*12,7,7,factionColors[f]);
    }
    wrap(31, 183, 68, faction_identity(ui.selection), TEXT);
    int active = 0;
    float left = 1800;
    for (int j = 0; j < 4; j++)
        if (g.mercActor[j] && g.mercUntil[j] > g.time) {
            active++;
            left = fminf(left, g.mercUntil[j] - g.time);
        }
    if (active) {
        snprintf(b, sizeof(b), "HIRED GUARDS %d   NEXT EXPIRY %.1f MIN", active, left / 60);
        text(31, 220, b, TEAL, 1);
    } else
        clipped(31, 220, g.worldNews, 68, GOLD);
}
static void storage_screen(void) {
    Actor *a = &g.actors[g.selected];
    Building *z = &g.buildings[ui.storageIndex];
    overlay_header(ui.storageSide ? "DEPOT / WITHDRAW" : "DEPOT / DEPOSIT",
                   "\014 SIDE   \001 MOVE ONE   \003 MOVE STACK   \010 GUIDE");
    int count = item_view_count(), first = start_row(item_view_row(ui.selection), count, 9);
    char b[30];
    if(!count)text(31,100,"NO ITEMS ON THIS SIDE",MUTED,1);
    for (int j = 0; j < 9 && first + j < count; j++) {
        int k = item_view_slot(first+j);
        Stack st = ui.storageSide ? z->storage[k] : a->pack[k];
        snprintf(b, 30, "\001%d", st.qty);
        row(86 + j * 15, k == ui.selection, st.item ? itemDefs[st.item].name : "EMPTY", st.item ? b : "");
    }
}
static void loot_screen(void) {
    Actor *a=&g.actors[ui.lootActor];int count=item_view_count();char b[100];
    int theft=!a->squad&&a->faction!=HOSTILE&&a->faction!=WILDLIFE;
    overlay_header("LOOT",theft?"\001 STEAL ONE   \003 STEAL STACK   \015 CHOOSE":"\001 TAKE ONE   \003 TAKE STACK   \015 CHOOSE");
    int first=start_row(item_view_row(ui.selection),count,6);
    for(int j=0;j<6&&first+j<count;j++) {
        int k=item_view_slot(first+j),id=limb_loot_item(a,k),y=84+j*13;
        if(k==ui.selection)rect(25,y-3,425,13,0x4a4030);
        snprintf(b,sizeof(b),"%s%.55s",k>=PACK+GEARS?"FITTED: ":k>=PACK?"WORN: ":"",itemDefs[id].name);clipped(31,y,b,62,k==ui.selection?TEXT:MUTED);
        snprintf(b,sizeof(b),"%d",k<PACK?a->pack[k].qty:1);text(442-strlen(b)*6,y,b,GOLD,1);
    }
    int it=!count?0:limb_loot_item(a,ui.selection);
    snprintf(b,sizeof(b),"%d ITEMS / %.30s",count,a->name);text(31,160,b,MUTED,1);
    equipment_comparison(it,173);
}
static void assist_screen(void) {
    Actor *a = &g.actors[ui.lootActor];
    overlay_header("RESCUE & RECOVERY", "\001 CHOOSE   BLEEDING CONTINUES WHEN YOU RESUME");
    portrait_scale(a, 112, 110, 28, 83, 1);
    clipped(151, 86, a->name, 32, TEAL);
    char b[100];
    snprintf(b, sizeof(b), "BLOOD %d  BLEED %.1f  %s", (int)a->blood, a->bleed,
             a->dead              ? "DEAD"
             : a->unconscious > 0 ? "DOWN"
                                  : "AWAKE");
    text(151, 102, b, a->bleed > 0 ? RED : MUTED, 1);
    const char *opts[] = {"First aid - uses one field dressing", "Carry unconscious survivor",
                          "Recover pack and worn equipment", "Take control of this squad member"};
    for (int i = 0; i < 4; i++) {
        if (i == ui.selection)
            rect(146, 126 + i * 21, 300, 18, 0x4a4030);
        text(152, 131 + i * 21, opts[i], i == ui.selection ? GOLD : MUTED, 1);
    }
    text(31, 223, "CARRYING SLOWS TRAVEL. \001 IN THE WORLD PUTS THEM DOWN.", TEXT, 1);
}
static void log_screen(void) {
    overlay_header("RECENT MESSAGES", "NEWEST FIRST - CURRENT SESSION");
    for (int i = 0; i < LOGS && i < ui.logCount; i++)
        wrap(31, 85 + i * 24, 68, ui.logs[i], i ? MUTED : TEXT);
}
void render_title_art(uint32_t *pixels, int stride) {
    fb = pixels;
    pitch = stride;
    ui.camX = 28;
    ui.camY = 27;
    terrain();
    objects_draw();
}
#include "depth_render.h"
void render_game(uint32_t *pixels, int stride) {
    fb = pixels;
    pitch = stride;
    if(ui.screen==SCREEN_CARE)ui.screen=SCREEN_HEALTH;
    if(ui.screen==SCREEN_ORDERS)ui.screen=SCREEN_SQUAD;
    item_view_normalize();
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
    depth_world_draw();
    if(!g.interiorBuilding){
      for(int k=0;k<6;k++)if(g.trails.bed[k]){Building*b=&g.buildings[g.trails.bed[k]-1];if(distance(b->x,b->y,ui.camX,ui.camY)<13){int x,y;project(b->x,b->y,&x,&y);text(x-30,y-25,"WAYFARER REST",TEAL,1);}}
      for(int t=0;t<TOWNS;t++)if(g.life.towns[t].stage==2&&distance(towns[t].x,towns[t].y,ui.camX,ui.camY)<18){int x,y;project(towns[t].x+5,towns[t].y+5,&x,&y);line(x,y,x,y-35,TEXT);triangle(x,y-35,x+16,y-30,x,y-23,factionColors[g.townOwner[t]]);text(x-20,y+3,g.life.towns[t].policy==1?"RELIEF POST":g.life.towns[t].policy==2?"GARRISON":"TRADE COURT",GOLD,1);}
      for(int t=OLD_TOWNS;t<TOWNS;t++)if(distance(towns[t].x,towns[t].y,ui.camX,ui.camY)<18)civic_flag(t);
      int seenYards=0;
      for(int n=0;n<g.actorCount;n++)if(g.war.prisonTown[n]&&!g.actors[n].dead){
          int t=g.war.prisonTown[n]-1;
          if(seenYards&(1<<t))continue;
          seenYards|=1<<t;
          float wx,wy;war_yard(t,&wx,&wy);
          if(distance(wx,wy,ui.camX,ui.camY)<10){int x,y;project(wx,wy,&x,&y);center(x,y+9,"WORK YARD",GOLD,1);}
      }
    }
    if (ui.screen == SCREEN_WORLD)
        nearby_labels(!!g.interiorBuilding);
    if(ui.screen==SCREEN_WORLD && !g.interiorBuilding && g.actors[g.selected].target<0 && g.actors[g.selected].unconscious<=0 && !g.actors[g.selected].dead) speech_bubbles();
    hud();
    switch (ui.screen) {
    case SCREEN_EXPEDITION:case SCREEN_ORDERS:case SCREEN_CAMP:case SCREEN_CARE:case SCREEN_ENCOUNTER:case SCREEN_LAYERS:
        depth_screen();break;
    case SCREEN_CREATE:
        creator_screen();
        break;
    case SCREEN_NAME:
        name_screen();
        break;
    case SCREEN_ENGAGE: engage_screen();break;
    case SCREEN_LIFE:case SCREEN_POLITICS:life_screen();break;
    case SCREEN_ACTIONS: actions_screen();break;
    case SCREEN_THEFT:
        theft_screen();
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
    case SCREEN_ASSIST:
        assist_screen();
        break;
    case SCREEN_LOG:
        log_screen();
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
    case SCREEN_LIMBS:case SCREEN_LIMBFIT:case SCREEN_SURGERY:case SCREEN_LIMBCONFIRM:
        limbs_screen();break;
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
        portrait_scale(r, 142, 118, 30, 86, 1);
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
        if (ui.confirmAction != 1) {
            char b[100];
            SaveInfo *z = &ui.saves[ui.slot];
            snprintf(b, sizeof(b), "SLOT %d: %s  DAY %d%s", ui.slot, z->exists ? z->name : "EMPTY", z->day,
                     ui.saveExit ? " / THEN TITLE" : "");
            text(31, 175, b, GOLD, 1);
        }
        break;
    default:
        break;
    }
    if(ui.screen!=SCREEN_TITLE&&ui.screen!=SCREEN_CREATE&&ui.screen!=SCREEN_NAME){
        int priority,offset;const char*entry=log_current(&priority,&offset);
        if(entry){char lineText[76];snprintf(lineText,sizeof(lineText),"%.75s",entry+offset);
            rect(0,0,480,23,INK);rect(0,0,3,23,priority?RED:TEAL);text(9,8,lineText,priority?GOLD:TEXT,1);}
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
