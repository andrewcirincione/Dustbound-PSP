#include "game.h"
#include "menu_art.h"
#include "combat_pose.h"
#include "art_data.h"
#include "actor_attachments.h"
#include "armor_sets_data.h"
#include "armor_art_data.h"
#include "settlement_art_ids.h"
#include "settlement_art.h"
#include "structure_art.h"
#include "furniture_art.h"
#include "furniture_models.h"

_Static_assert(ART_COUNT == SETTLEMENT_ART_FIRST, "Settlement sprite IDs must follow the base atlas");
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const ArtSprite *actor_sprite(int id) {
    if(id>=ARMOR_ART_FIRST && id<ARMOR_ART_FIRST+ARMOR_SETS*6){
        const ArtSprite *s=&armorSprites[id-ARMOR_ART_FIRST];
        if(s->w)return s;
        int set=(id-ARMOR_ART_FIRST)/6,part=(id-ARMOR_ART_FIRST)%6;
        const ArmorSetDef *z=&armorSets[set];
        int fallback=part==0?z->family[0]:part==1?z->family[1]:part==2?64+z->family[2]-68:part==3?80+z->family[3]-60:part==4?72:73;
        return &artSprites[fallback];
    }
    return &artSprites[id];
}
static int armor_actor_art(int item,int part,int fallback){
    int set=armor_item_set(item);return set<0?fallback:ARMOR_ART_FIRST+set*6+part;
}
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
static uint32_t artPalette[SCENE_ART_END][256];
static int artLight[SCENE_ART_END];
static const ArtSprite *scene_sprite(int id) {
    int model = structure_art_index(id);
    if (model >= 0) return &structureSprites[model];
    if (id >= FURNITURE_ART_FIRST) return &furnitureSprites[id - FURNITURE_ART_FIRST];
    if (id >= ART_IRON_GRILLE) return &barrierSprites[id - ART_IRON_GRILLE];
    return id < ART_COUNT ? &artSprites[id] : &settlementSprites[id - SETTLEMENT_ART_FIRST];
}
static uint32_t *sprite_palette(int id, const ArtSprite *s, float brightness) {
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
    return artPalette[id];
}
static void art(int id, int x, int y, float brightness, int fade) {
    const ArtSprite *s = scene_sprite(id);
    const uint32_t *palette = sprite_palette(id, s, brightness);
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
            uint32_t c = palette[*p++];
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
static void scaled_art(int id, int left, int top, int width, int height, float brightness, int flip, int fade) {
    const ArtSprite *s = scene_sprite(id);
    const uint32_t *palette = sprite_palette(id, s, brightness);
    int stepX = (s->w << 16) / width, stepY = (s->h << 16) / height;
    int firstY = top < clipTop ? clipTop - top : 0;
    int lastY = top + height > clipBottom ? clipBottom - top : height;
    for (int y = firstY; y < lastY; y++) {
        const uint16_t *row = &s->rows[(y * stepY >> 16) * 3];
        if (row[1] == row[2]) continue;
        int start = flip ? s->w - row[2] : row[1], end = flip ? s->w - row[1] : row[2];
        int x0 = (start * width + s->w - 1) / s->w, x1 = (end * width + s->w - 1) / s->w;
        if (left + x0 < clipLeft) x0 = clipLeft - left;
        if (left + x1 > clipRight) x1 = clipRight - left;
        if (x0 >= x1) continue;
        uint32_t *dst = fb + (top + y) * pitch + left + x0;
        int sample = x0 * stepX;
        for (int x = x0; x < x1; x++, dst++, sample += stepX) {
            int col = sample >> 16;
            if (flip) col = s->w - 1 - col;
            if (col < row[1] || col >= row[2] || (fade && ((x+y)&1))) continue;
            uint32_t c = palette[s->pixels[row[0] + col - row[1]]];
            unsigned a = c >> 24;
            if (a == 255) *dst = c;
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
    } else if (k == 10 || k == 11 || k == 14 || k == 15) {
        int dx = k == 14 ? -1 : k == 15 ? 1 : 0, dy = k == 10 ? -1 : k == 11 ? 1 : 0;
        int tx = 4 + dx * 3, ty = 3 + dy * 3;
        BL(4 - dx * 3, 3 - dy * 3, tx, ty, c);
        BL(tx, ty, tx - dx * 3 + dy * 3, ty - dy * 3 + dx * 3, c);
        BL(tx, ty, tx - dx * 3 - dy * 3, ty - dy * 3 - dx * 3, c);
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
        if (c) text(x, y, b, c, 1);
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
enum { WORLD_CENTER_Y = 150 };
static int projectionX, projectionY;
static inline int project_x(float x, float y) {
    return 240 + projectionX + (int)(view_horizontal(x - ui.camX, y - ui.camY) * 22);
}
static inline int project_y(float x, float y) {
    return WORLD_CENTER_Y + projectionY + (int)(view_vertical(x - ui.camX, y - ui.camY) * 11) - terrain_height(x, y);
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
    float dawn = (g.dayTime - 5) / 3, dusk = (21 - g.dayTime) / 4;
    float daylight = fmaxf(0, fminf(1, fminf(dawn, dusk)));
    /* Cache terrain through each small step of the lighting transition. */
    return .76f + (int)(daylight * 12) * (.32f / 12);
}
static const char *bearing(float dx, float dy) {
    if (fabsf(dx) < fabsf(dy) * .4f) return dy < 0 ? "N" : "S";
    if (fabsf(dy) < fabsf(dx) * .4f) return dx < 0 ? "W" : "E";
    return dy < 0 ? (dx < 0 ? "NW" : "NE") : (dx < 0 ? "SW" : "SE");
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
        float u = (ui.camX + (y - WORLD_CENTER_Y - projectionY) / 22.f + (-240 - projectionX) / 44.f) * 8;
        float v = (ui.camY + (y - WORLD_CENTER_Y - projectionY) / 22.f - (-240 - projectionX) / 44.f) * 8;
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
    enum { BLEND_CELL = 12, BLEND_GRID = 5 };
    int left = (int)floorf((cx - 18) / (float)BLEND_CELL) * BLEND_CELL;
    int top = (int)floorf((cy - 18) / (float)BLEND_CELL) * BLEND_CELL;
    uint32_t palette[BLEND_GRID][BLEND_GRID];
    for (int j = 0; j < BLEND_GRID; j++)
        for (int i = 0; i < BLEND_GRID; i++)
            palette[j][i] = ground(biome(left + i * BLEND_CELL, top + j * BLEND_CELL));
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
            int col = (x - left) / BLEND_CELL, row = (y - top) / BLEND_CELL;
            float u = ((x - left) % BLEND_CELL) / (float)BLEND_CELL;
            float v = ((y - top) % BLEND_CELL) / (float)BLEND_CELL;
            uint32_t c0 = palette[row][col], c1 = palette[row][col + 1];
            uint32_t c2 = palette[row + 1][col], c3 = palette[row + 1][col + 1];
            uint32_t c = c0 == c1 && c0 == c2 && c0 == c3 ? c0
                : blend(blend(c0, c1, u), blend(c2, c3, u), v);
            float brightness = (.985f + (h % 5) * .005f) * li *
                               ((g.weather == 2 || g.weather == 3) ? .85f : 1);
            if (x < 1 || y < 1 || x > WORLD - 2 || y > WORLD - 2)
                c = 0x5c6867;
            else if (t >= 0)
                c = blend(c, 0xd0bb90, .18f);
            else if (road(x, y))
                c = blend(c, 0xd4bd8e, .23f);
            c = shade(c, brightness);
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
/* Project an elevation texture between its ground endpoints. */
enum { WALL_SLICES = 4 };
static void elevation(int id, int x0, int y0, int x1, int y1, int height, float brightness, int slice) {
    int flipped = x1 < x0;
    if (flipped) {
        int tmp = x0; x0 = x1; x1 = tmp;
        tmp = y0; y0 = y1; y1 = tmp;
    }
    int width = x1 - x0;
    if (!width || x1 < clipLeft || x0 >= clipRight)
        return;
    const ArtSprite *s = scene_sprite(id);
    const uint32_t *palette = sprite_palette(id,s,brightness);
    int left = x0, right = x1;
    if (slice >= 0) {
        int part = flipped ? WALL_SLICES-1-slice : slice;
        left += width*part/WALL_SLICES;
        if (part < WALL_SLICES-1) right = x0+width*(part+1)/WALL_SLICES-1;
    }
    if (left < clipLeft) left = clipLeft;
    if (right >= clipRight) right = clipRight-1;
    for (int x = left; x <= right; x++) {
        int dx = x - x0;
        int foot = y0 + (y1-y0)*dx/width;
        int sx = dx*(s->w-1)/width;
        if (flipped) sx = s->w-1-sx;
        int top = foot - height + 1;
        int from = top < clipTop ? clipTop : top, to = foot >= clipBottom ? clipBottom-1 : foot;
        for (int y = from; y <= to; y++) {
            const uint16_t *span = &s->rows[(y-top)*s->h/height*3];
            if (sx < span[1] || sx >= span[2]) continue;
            uint32_t c = palette[s->pixels[span[0]+sx-span[1]]];
            unsigned alpha = c >> 24;
            if (alpha == 255) {
                fb[y*pitch+x] = c;
            } else if (alpha) {
                uint32_t d = fb[y*pitch+x];
                unsigned rb = (((c & 0xff00ff) * alpha + (d & 0xff00ff) * (256-alpha)) >> 8) & 0xff00ff;
                unsigned gg = (((c & 0xff00) * alpha + (d & 0xff00) * (256-alpha)) >> 8) & 0xff00;
                fb[y*pitch+x] = 0xff000000 | rb | gg;
            }
        }
    }
}
static void art_shadow(int id, int x, int y) {
    int model = structure_art_index(id);
    if (model < 0 && (id < 0 || id >= 32))
        return;
    const ArtShadow *s = model >= 0 ? &structureShadows[model] : &artShadows[id];
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
typedef struct { int x, y; } ScreenPoint;
static void wall(int id, int slice) {
    const Building *b = &g.buildings[id];
    WallRun run = building_wall_run(id);
    float dx = !run.alongY, dy = run.alongY;
    int x0, y0, x1, y1;
    project(b->x+dx*run.low,b->y+dy*run.low,&x0,&y0);
    project(b->x+dx*run.high,b->y+dy*run.high,&x1,&y1);
    int town = building_town(id), court = town >= 0 && civil_style(town) == 1;
    elevation(court ? ART_COURT_WALL : ART_STONE_WALL,x0,y0,x1,y1,court ? 36 : 27,
              light()*(run.alongY ? .88f : 1),slice);
}
static void building(int id) {
    const Building *b = &g.buildings[id];
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -155 || x > 635 || y < -80 || y > 345)
        return;
    float brightness = light();
    if (b->type != WALL) {
        int sprite = building_sprite(id), fade = g.depth.buildings[id].labor > 0;
        const ArtSprite *s = scene_sprite(sprite);
        float scale = building_scale(id);
        if (building_has_interior(b->type)) {
            const Actor *a = &g.actors[g.selected];
            int ax, ay;
            project(a->x,a->y,&ax,&ay);
            float w, d;
            int height;
            building_footprint(id,&w,&d,&height);
            int behind = (a->x < b->x-w || a->y < b->y-d) &&
                         a->x < b->x+w && a->y < b->y+d;
            fade |= behind && ax > x-s->ax*scale && ax < x+(s->w-s->ax)*scale && ay > y-s->ay*scale && ay < y;
        }
        if (scale == 1)
            art(sprite,x,y,brightness,fade);
        else
            scaled_art(sprite,x-(int)roundf(s->ax*scale),y-(int)roundf(s->ay*scale),
                       (int)roundf(s->w*scale),(int)roundf(s->h*scale),brightness,0,fade);
    }
    if (g.depth.buildings[id].labor > 0) {
        bar(x-20,y-8,40,3,100-g.depth.buildings[id].labor/35*100,GOLD);
        text(x-24,y+5,"BUILDING",GOLD,1);
    }
    if (b->type == SLAVE_YARD) {
        art(30, x-26, y-4, brightness, 0);
        art(26, x-12, y+6, brightness, 0);
        art(27, x+23, y+9, brightness, 0);
    }
}
#include "painted_actor.h"
static int resource_sprite(int n) {
    int t = g.nodes[n].type;
    return t == NODE_ORE ? ART_IRON_SEAM : t == NODE_COPPER ? ART_COPPER_SEAM :
           t == NODE_WOOD ? ART_CUT_TIMBER : t == NODE_RUIN ? 15 : 31;
}
static void resource(int n) {
    Node *b = &g.nodes[n];
    if (!b->remaining)
        return;
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -40 || x > 520 || y < -30 || y > 245)
        return;
    if (b->type == NODE_DROPPED) {
        scaled_art(20, x - 7, y - 10, 14, 12, light(), 0, 0);
        return;
    }
    if(b->item==SEVERED_ARM||b->item==SEVERED_LEG){
        diamond(x,y,7,3,shade(0x653c30,light()));
        part(b->item==SEVERED_ARM?75:64,x-5,y-2,x+5,y,4,0x9d7660,light(),0,0,0,1);
        return;
    }
    int id = resource_sprite(n);
    art(id, x, y, light(), 0);
}
typedef struct {
    float depth;
    int kind, index, x, y;
} Object;
static Object objects[MAX_NODES + MAX_BUILDINGS*(WALL_SLICES+1) + MAX_ACTORS + 1200 + POP_GROUPS*POP_PROPS];
static int compare_obj(const void *a, const void *b) {
    float d = ((Object *)a)->depth - ((Object *)b)->depth;
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}
static void squad_health_bar(const Actor *a, int x, int y) {
    float hp = 100;
    for (int l = 0; l < LIMBS; l++)
        hp = fminf(hp, a->health[l] * 100 / limb_max_health(a, l));
    if (hp >= 99.95f && a->bleed <= .01f && !a->dead && a->unconscious <= 0)
        return;
    if (a->dead || a->unconscious > 0) hp = 0;
    bar(x - 10, y - 43, 21, 3, hp, a->bleed > .01f ? RED : TEAL);
    if (a->bleed > .01f)
        rect(x - 10, y - 43, 21, 1, RED);
}
typedef struct { float x, y, w, d; } BuildingYard;
static int scenery_clear_of_buildings(float x, float y, float canopyHalf, const BuildingYard *yards, int count) {
    for (int i = 0; i < count; i++) {
        const BuildingYard *yard = &yards[i];
        float dx = x - yard->x, dy = y - yard->y;
        if (fabsf(dx) < yard->w + .65f && fabsf(dy) < yard->d + .65f)
            return 0;
        /* Keep the approach to the roofline clear of branches from behind and beside it. */
        float depth = view_vertical(dx, dy), span = yard->w + yard->d;
        if (depth <= 0 && depth > -span * 1.5f - 3.4f &&
            fabsf(view_horizontal(dx, dy)) < span + canopyHalf + .55f)
            return 0;
    }
    return 1;
}
static void objects_draw(void) {
    static const unsigned char decor[9][8] = {
        {12,12,12,9,9,8,12,8},     /* cinder scrub */
        {9,9,11,9,12,12,9,8},      /* copper outcrops */
        {9,12,9,12,9,9,12,9},      /* sparse salt flats */
        {13,13,13,12,13,10,13,9},  /* fens */
        {8,12,8,9,8,8,9,12},      /* deadwood and ash */
        {11,9,11,12,9,11,9,12},    /* mesa */
        {14,14,10,14,9,12,14,10},  /* snowy highlands */
        {10,10,10,13,10,9,10,8},   /* pines with an understory */
        {11,11,9,11,12,9,11,8}     /* canyons */
    };
    static const unsigned char spacing[] = {12, 13, 22, 8, 13, 13, 14, 8, 15};
    int n = 0;
    for (int i = 0; i < g.nodeCount; i++) {
        Node *b = &g.nodes[i];
        if (b->remaining && !g.drops[i].room && fabsf(b->x - ui.camX) < 20 && fabsf(b->y - ui.camY) < 20)
            objects[n++] = (Object){b->x + b->y, 0, i, 0, 0};
    }
    BuildingYard yards[MAX_BUILDINGS];
    int yardCount = 0;
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (portable_bedroll(i) < 0 || fabsf(b->x - ui.camX) >= 20 || fabsf(b->y - ui.camY) >= 20)
            continue;
        if (b->type == WALL) {
            /* Sort along each wall so actors standing behind a corner are covered by it. */
            WallRun run = building_wall_run(i);
            for (int part = 0; part < WALL_SLICES; part++) {
                float depth = b->x+b->y+run.low+(run.high-run.low)*(part+.5f)/WALL_SLICES;
                objects[n++] = (Object){depth, 6, i, part, 0};
            }
            if (g.depth.buildings[i].labor > 0)
                objects[n++] = (Object){b->x + b->y, 1, i, 0, 0};
        } else {
            objects[n++] = (Object){b->x + b->y, 1, i, 0, 0};
        }
        if (building_has_interior(b->type) || b->type == WINDMILL || b->type == WELL || b->type == BED ||
            b->type == WALL || b->type == SLAVE_YARD || b->type == CAMPFIRE) {
            int height;
            BuildingYard *yard = &yards[yardCount++];
            *yard = (BuildingYard){.x = b->x, .y = b->y};
            building_footprint(i, &yard->w, &yard->d, &height);
        }
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->carriedBy >= 0 || actor_inside(a) || population_hidden(i))
            continue;
        float wx = a->x, wy = a->y;
        int roll = 0;
        if (actor_in_bed(a)) {
            int bed = a->taskIndex;
            if (bed >= 0 && g.buildings[bed].type == BED) {
                wx = g.buildings[bed].x;
                wy = g.buildings[bed].y;
                roll = 1;
            } else {
                float bx, by;
                if (population_sleep_position(a, &bx, &by)) {
                    wx = bx;
                    wy = by;
                    roll = 1;
                }
            }
        }
        int x = project_x(wx, wy);
        if (x <= -30 || x >= 510)
            continue;
        int y = project_y(wx, wy);
        if (y > -20 && y < 296)
            objects[n++] = (Object){wx + wy + (roll ? .05f : 0), 2, i, x, y + (roll ? 1 : 0)};
    }
    int cx = ui.camX, cy = ui.camY;
    float campX[POP_GROUPS + 6], campY[POP_GROUPS + 6];
    int campCount = 0;
    for (int i = 0; i < 6; i++)
        if (g.trails.bed[i]) {
            const Building *bed = &g.buildings[g.trails.bed[i] - 1];
            if (fabsf(bed->x - cx) < 27 && fabsf(bed->y - cy) < 27) {
                campX[campCount] = bed->x;
                campY[campCount++] = bed->y;
                if(i%3!=1){
                    float tx=bed->x-2.35f,ty=bed->y-1.6f;int sx,sy;
                    if(can_walk(tx,ty)){project(tx,ty,&sx,&sy);objects[n++]=(Object){tx+ty,3,26,sx,sy};}
                }
            }
        }
    for (int k = 0; k < POP_GROUPS; k++) {
        int props = population_object_count(k);
        if (!props)
            continue;
        const PopulationState *site = &g.population.state[k];
        if (fabsf(site->siteX - cx) < 27 && fabsf(site->siteY - cy) < 27) {
            campX[campCount] = site->siteX;
            campY[campCount++] = site->siteY;
        }
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
    for (int y = cy - 20; y <= cy + 20; y++)
        for (int x = cx - 20; x <= cx + 20; x++) {
            if (x < 2 || y < 2 || x >= WORLD - 2 || y >= WORLD - 2)
                continue;
            unsigned h = hash(x, y);
            if (h % 8)
                continue;
            int sx, sy;
            float wx = x + .18f + ((h >> 4) & 7) * .07f, wy = y + .18f + ((h >> 12) & 7) * .07f;
            sx = project_x(wx, wy);
            if (sx < -90 || sx > 570)
                continue;
            sy = project_y(wx, wy);
            if (sy < -30 || sy > 360)
                continue;
            int region = biome(x + (int)((h >> 6) % 15) - 7, y + (int)((h >> 15) % 15) - 7);
            if ((h >> 8) % spacing[region] >= 8 || town_near(x, y, 8) >= 0 ||
                road(x, y) || terrain_water(x, y))
                continue;
            int campGround = 0;
            for (int k = 0; k < campCount; k++) {
                if (fabsf(wx - campX[k]) < 8 && fabsf(wy - campY[k]) < 8) {
                    campGround = 1;
                    break;
                }
            }
            if (campGround) continue;
            if (trail_blocked(x, y))
                continue;
            int id = decor[region][(h >> 20) & 7];
            const ArtSprite *s = &artSprites[id];
            if (sx - s->ax > 480 || sx - s->ax + s->w < 0 || sy + 4 - s->ay > 252 ||
                sy + 4 - s->ay + s->h < 0)
                continue;
            if (!scenery_clear_of_buildings(wx, wy, s->w / 44.f, yards, yardCount))
                continue;
            objects[n++] = (Object){wx + wy, 3, id, sx, sy};
        }
    for (int i = 0; i < n; i++) {
        Object *o = &objects[i];
        int x = o->x, y = o->y;
        if (o->kind == 1) {
            Building *b = &g.buildings[o->index];
            project(b->x, b->y, &x, &y);
            if (b->type != WALL)
                art_shadow(building_sprite(o->index), x, y);
        }
        if (o->kind == 0 && g.nodes[o->index].type != NODE_DROPPED && g.nodes[o->index].remaining) {
            Node *b = &g.nodes[o->index];
            project(b->x, b->y, &x, &y);
            int id = resource_sprite(o->index);
            art_shadow(id, x, y + (id < 24 ? 5 : 0));
        }
        if(o->kind==4){if(!population_prop_style(o->index/POP_PROPS,o->index%POP_PROPS))art_shadow(population_prop_art(o->index/POP_PROPS,o->index%POP_PROPS),x,y);}
        if (o->kind == 3)
            art_shadow(o->index, x, y + 4);
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
        } else if (ui.contextKind == 3) {
            building_door(id, &wx, &wy);
        } else {
            wx = g.buildings[id].x;
            wy = g.buildings[id].y;
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
            person(a, o->x, o->y, 1, o->index == g.selected && !actor_in_bed(a));

        } else if (o->kind == 6) {
            wall(o->index,o->x);
        } else if(o->kind==5){art(11,o->x,o->y,light(),0);art(9,o->x+10,o->y+4,light(),0);
        } else if (o->kind == 4) {
            int group = o->index / POP_PROPS, prop = o->index % POP_PROPS;
            int style = population_prop_style(group, prop);
            int id = style == 2 ? 30 : population_prop_art(group, prop);
            art(id, o->x, o->y, light(), 0);
        } else {
            art(o->index, o->x, o->y + 4, light(), 0);
        }
    }
    for (int i = 0; i < n; i++) {
        Object *o = &objects[i];
        if (o->kind != 2)
            continue;
        int id = o->index;
        Actor *a = &g.actors[id];
        if (a->squad) {
            squad_health_bar(a, o->x, o->y);
            if (id == g.selected)
                triangle(o->x - 3, o->y - 50, o->x + 3, o->y - 50, o->x, o->y - 46, TEAL);
        }
    }
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
typedef struct { int x,y,w,h; } LabelBox;
static LabelBox labelBoxes[48];
static int labelCount;

static void reserve_label(LabelBox box) {
    if (labelCount < (int)(sizeof(labelBoxes)/sizeof(*labelBoxes)))
        labelBoxes[labelCount++] = box;
}
static int place_label(int center, int top, int w, int h, LabelBox *out) {
    static const int rows[] = {0,-1,1,-2,2,-3,3,-4,4};
    static const int columns[] = {0,-30,30,-60,60};
    int right = 476-w, bottom = 209-h;
    for (unsigned r = 0; r < sizeof(rows)/sizeof(*rows); r++)
        for (unsigned c = 0; c < sizeof(columns)/sizeof(*columns); c++) {
            int x = center-w/2+columns[c], y = top+rows[r]*(h+3);
            if (x < 4) x = 4;
            if (x > right) x = right;
            if (y < 39) y = 39;
            if (y > bottom) y = bottom;
            int blocked = 0;
            for (int n = 0; n < labelCount; n++) {
                const LabelBox *other = &labelBoxes[n];
                if (x < other->x+other->w+2 && x+w+2 > other->x &&
                    y < other->y+other->h+2 && y+h+2 > other->y) {
                    blocked = 1;
                    break;
                }
            }
            if (blocked) continue;
            *out = (LabelBox){x,y,w,h};
            reserve_label(*out);
            return 1;
        }
    return 0;
}
static void world_label(int x, int y, const char *title, uint32_t color, int faction) {
    if (x < 0 || x >= 480 || y < -60 || y > 234)
        return;
    LabelBox box;
    if (!place_label(x,y,text_width(title)+10,11,&box))
        return;
    rect(box.x,box.y,box.w,box.h,INK);
    if (faction >= 0)
        rect(box.x,box.y,2,box.h,factionColors[faction]);
    text(box.x+5,box.y+2,title,color,1);
}
static void care_and_cooldown(int indoors) {
    Actor *selected = &g.actors[g.selected];
    for (int i = 0; i < g.actorCount; i++) {
        Actor *medic = &g.actors[i];
        int patient = treatment_patient(medic);
        if (patient < 0 || patient >= g.actorCount || (!medic->squad && !g.actors[patient].squad) ||
            !medic->active || depth_room(medic) != depth_room(selected) || actor_floor(medic) != actor_floor(selected))
            continue;
        int x, y;
        if (indoors) room_project(depth_x(medic), depth_y(medic), &x, &y);
        else project(medic->x, medic->y, &x, &y);
        if (x < 12 || x > 468 || y < 74 || y > 233) continue;
        reserve_label((LabelBox){x-13,y-61,27,17});
        rect(x - 5, y - 61, 11, 11, INK);
        rect(x - 4, y - 57, 9, 3, 0x78bdea);
        rect(x - 1, y - 60, 3, 9, 0x78bdea);
        bar(x - 12, y - 48, 25, 3, treatment_progress(medic), 0xdc9d54);
    }
    if (selected->autoattack && selected->cooldown > .01f && conscious(selected)) {
        int x, y;
        if (indoors) room_project(depth_x(selected), depth_y(selected), &x, &y);
        else project(selected->x, selected->y, &x, &y);
        CombatTiming timing = combat_timing(selected, selected->attackWeapon, combat_attack_variant(selected));
        float total = timing.windup + timing.stroke + timing.recovery;
        reserve_label((LabelBox){x-13,y+5,27,3});
        bar(x - 13, y + 5, 27, 3, (1 - fminf(1, selected->cooldown / total)) * 100, 0xdc9d54);
    }
}
static void nearby_labels(int indoors) {
    int ids[MAX_ACTORS], count = 0, fighting = 0;
    float range[MAX_ACTORS];
    const Actor *p = &g.actors[g.selected];
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->squad || a->carriedBy >= 0 || population_hidden(i) ||
            depth_room(a) != depth_room(p)) continue;
        float d = depth_distance(p,a);
        if (d > 9) continue;
        int j = count++;
        while (j > 0 && range[j-1] > d) {
            ids[j] = ids[j-1];
            range[j] = range[j-1];
            j--;
        }
        ids[j] = i;
        range[j] = d;
    }
    for (int i = 0; i < count; i++) {
        if (!squad_combatant(ids[i])) continue;
        Actor *a = &g.actors[ids[i]];
        int x,y;
        if (indoors) room_project(depth_x(a),depth_y(a),&x,&y);
        else project(a->x,a->y,&x,&y);
        fighting = 1;
        reserve_label((LabelBox){x-14,y+1,29,11});
        rect(x-14,y+1,29,7,0x14110e);
        bar(x-13,y+2,27,5,knockout_reserve(a),a->unconscious>0?MUTED:0xe36552);
        if (a->attackPhase == 1) {
            float progress = 1-a->attackClock/fmaxf(.01f,a->attackDuration);
            bar(x-12,y+8,25,3,progress*100,GOLD);
            line(x,y+5,x+(int)(cosf(a->attackAngle)*18),y+5+(int)(sinf(a->attackAngle)*9),GOLD);
        }
    }
    int focus = 0;
    if (fighting)
        for (int i = 0; i < count; i++)
            if (ids[i] == p->target) { focus = i; break; }
    for (int i = 0; i < (fighting ? 1 : count) && i < 8; i++) {
        int row = fighting ? focus : i, id = ids[row];
        Actor *a = &g.actors[id];
        if (range[row] > (fighting ? 9 : 4)) continue;
        int x,y;
        if (indoors) room_project(depth_x(a),depth_y(a),&x,&y);
        else project(a->x,a->y,&x,&y);
        char name[48];
        int enemy = actor_relation(id) < 0;
        const char *role = a->bountyReward ? a->name : escorting_actor(id) ? "Escort caravan" : npc_role_name(a);
        const char *status = a->dead ? " / DEAD" : a->unconscious > 0 ? " / DOWN" : enemy ? " !" : "";
        snprintf(name,sizeof(name),"%.19s%s%s",role,"",status);
        world_label(x,fighting ? y+16 : y-(a->dead||a->unconscious>0 ? 24 : 48),
                    name,enemy&&!a->dead ? RED : label_color(a->faction),a->faction);
        if (fighting && a->hitFlash > 0 && a->impactKind) {
            const char *hit = a->unconscious>0 ? "DOWN" : a->impactKind==2 ? "BLOCK" :
                              a->impactKind==3 ? "DODGE" : "HIT";
            world_label(x+30,y-12,hit,TEXT,-1);
        }
    }
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
    int w = (int)fminf(300,text_width(words)+16), cols = (w-12)/6;
    int h = wrap(0,0,cols,words,0)+7;
    LabelBox box;
    if (!place_label(x,y-45-h,w,h+4,&box))
        return;
    rect(box.x,box.y,w,h,INK);
    frame(box.x,box.y,w,h,EDGE);
    int tail = x < box.x+7 ? box.x+7 : x > box.x+w-7 ? box.x+w-7 : x;
    triangle(tail-3,box.y+h-1,tail+3,box.y+h-1,tail,box.y+h+3,EDGE);
    wrap(box.x+6,box.y+4,cols,words,a->speech>=24?GOLD:TEXT);
}

static void structure_labels(void) {
    const Actor *a = &g.actors[g.selected];
    for (int i = 0; i < g.buildingCount; i++) {
        const Building *b = &g.buildings[i];
        if ((!building_has_interior(b->type) && b->type != SLAVE_YARD) ||
            fabsf(b->x-a->x) >= 7 || fabsf(b->y-a->y) >= 7 ||
            distance(b->x,b->y,a->x,a->y) >= 7)
            continue;
        int x,y;
        project(b->x,b->y,&x,&y);
        const ArtSprite *s = scene_sprite(building_sprite(i));
        world_label(x,y-s->ay-13,building_name(i),GOLD,-1);
    }
    int ore = nearest_ore(a->x,a->y,5.5f,-1);
    if (ore >= 0) {
        const Node *node = &g.nodes[ore];
        int x,y;
        project(node->x,node->y,&x,&y);
        world_label(x,y-21,node->type == NODE_ORE ? "Iron ore" : "Copper ore",GOLD,-1);
    }
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
static float roomScale = 22, roomBase = 61;
static void room_project(float x, float y, int *sx, int *sy) {
    *sx = 240 + (int)((x - y) * roomScale);
    *sy = (int)(roomBase + (x + y) * roomScale * .5f);
}
static float room_furniture_scale(const InteriorProp *p) {
    return 2 * roomScale * (p->w + p->d) / furnitureModels[furniture_art_index(p->art)].span;
}
static void room_furniture(const InteriorProp *p) {
    int model = furniture_art_index(p->art), x, y;
    room_project(p->x, p->y, &x, &y);
    int flip = furniture_is_bed(p->art) && p->d > p->w;
    const ArtSprite *sprite = &furnitureSprites[model];
    float scale = room_furniture_scale(p);
    int width = (int)roundf(sprite->w * scale), height = (int)roundf(sprite->h * scale);
    int anchorX = flip ? sprite->w-1-sprite->ax : sprite->ax;
    scaled_art(FURNITURE_ART_FIRST + model,x-(int)roundf(anchorX*scale),
               y-(int)roundf(sprite->ay*scale),width,height,1.08f,flip,0);
}
static void cell_partition(int room) {
    int leftX,leftY, hingeX,hingeY, latchX,latchY, rightX,rightY;
    room_project(0,CELL_Y,&leftX,&leftY);
    room_project(CELL_HINGE_X,CELL_Y,&hingeX,&hingeY);
    room_project(CELL_LATCH_X,CELL_Y,&latchX,&latchY);
    room_project(7,CELL_Y,&rightX,&rightY);
    int open = jail_room_open(room);
    int leafX = latchX, leafY = latchY;
    if (open)
        room_project(CELL_HINGE_X,CELL_Y-(CELL_LATCH_X-CELL_HINGE_X),&leafX,&leafY);
    elevation(ART_CELL_GATE,hingeX,hingeY,leafX,leafY,36,open ? .76f : .95f,-1);
    elevation(ART_IRON_GRILLE,leftX,leftY,hingeX,hingeY,38,.92f,-1);
    elevation(ART_IRON_GRILLE,latchX,latchY,rightX,rightY,38,.92f,-1);
}
static float room_depth(float x, float y, int jail) {
    return x + y + (jail && y >= CELL_Y ? 14 : 0);
}
static void interior_draw(void) {
    int id = g.interiorBuilding - 1;
    Building *b = &g.buildings[id];
    int town = building_town(id), style = town < 0 ? 0 : civil_style(town);
    const TownDesign *plan = town_design(town);
    int floor = actor_floor(&g.actors[g.selected]);
    roomScale = b->type == BAR ? 17.6f + plan->beds * 1.16f :
                b->type == JAIL ? 18.5f + plan->custody * 2.3f :
                b->type == HOUSE ? 19.8f + (id % 3) * .9f : 21.0f + plan->marketTier;
    if (g.frontier.variant[id] == BUILD_LODGE) roomScale = 22;
    roomBase = 69 - (roomScale - 20) * 2.3f;
    rect(0, 0, 480, 272, 0x3d3429);
    int cx, cy, eastX, eastY, westX, westY;
    room_project(0, 0, &cx, &cy);
    room_project(7, 0, &eastX, &eastY);
    room_project(0, 7, &westX, &westY);
    float wall = style == 1 ? 38 : style == 2 ? 41 : 37;
    int wallArt = style == 3 || style == 5 ? 106 : style == 4 ? 107 : 105;
    int floorArt = style == 2 ? 105 : 104;
    texture_plane(wallArt, westX, westY, cx-westX, cy-westY, 0, -wall, 3, 1.2f, 1.04f);
    texture_plane(wallArt, cx, cy, eastX-cx, eastY-cy, 0, -wall, 3, 1.2f, .90f);
    texture_plane(floorArt, cx, cy, eastX-cx, eastY-cy, westX-cx, westY-cy, 2.6f, 2.6f, 1.12f);
    if (wallArt == 105)
        for (int j = 0; j <= 5; j++) {
            int x, y;
            room_project(j * 1.4f, 0, &x, &y);
            texture_plane(106, x, y, 0, -wall, 3, 1.5f, 1, 1, .67f);
            room_project(0, j * 1.4f, &x, &y);
            texture_plane(106, x, y, 0, -wall, 3, -1.5f, 1, 1, .78f);
        }
    int x, y;
    if (!floor) {
        room_project(3.5f, 6.86f, &x, &y);
        texture_plane(104, x, y - 8, 15, 8, -15, 8, .8f, .8f, 1.11f);
        text(x - 12, y + 9, "EXIT", GOLD, 1);
    }
    for (int i = 0; i < g.nodeCount; i++)
        if (g.nodes[i].type == NODE_DROPPED && g.nodes[i].remaining &&
            g.drops[i].room == id + 1 && g.drops[i].floor == floor) {
            room_project(g.drops[i].x, g.drops[i].y, &x, &y);
            scaled_art(20, x-7, y-10, 14, 12, 1.08f, 0, 0);
        }
    InteriorProp props[ROOM_PROPS];
    int jail = b->type == JAIL;
    enum { ROOM_OBJECTS = MAX_ACTORS + ROOM_PROPS + 1 };
    int count = interior_floor_props(id, floor, props), furniture = count, order[ROOM_OBJECTS];
    float depth[ROOM_OBJECTS];
    int roomActor[ROOM_OBJECTS];
    for (int i = 0; i < count; i++) {
        order[i] = i;
        depth[i] = props[i].art == 23 ? -1 : room_depth(props[i].x,props[i].y,jail);
        roomActor[i] = -1;
    }
    if (jail) {
        order[count] = count;
        roomActor[count] = -2;
        depth[count++] = 12;
    }
    Actor *controlled = &g.actors[g.selected];
    if (controlled->carriedBy < 0) {
        roomActor[count] = g.selected;
        order[count] = count;
        depth[count++] = room_depth(g.interiorX, g.interiorY, jail) + (actor_in_bed(controlled) ? .01f : 0);
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *resident = &g.actors[i];
        if (i == g.selected || !resident->active || resident->carriedBy >= 0 ||
            (depth_room(resident) != g.interiorBuilding || actor_floor(resident) != floor))
            continue;
        roomActor[count] = i;
        order[count] = count;
        depth[count++] = room_depth(depth_x(resident), depth_y(resident), jail) + (actor_in_bed(resident) ? .01f : 0);
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
        if (roomActor[i] >= 0) {
            Actor *resident = &g.actors[roomActor[i]];
            float rx = depth_x(resident), ry = depth_y(resident);
            room_project(rx, ry, &x, &y);
            int sleeping = actor_in_bed(resident);
            if (sleeping)
                for (int bed = 0; bed < furniture; bed++) {
                    const InteriorProp *p = &props[bed];
                    if (p->action != ROOM_REST || distance(rx, ry, p->x, p->y) >= .4f)
                        continue;
                    float scale = room_furniture_scale(p);
                    /* The painted mattress center is above and beside the floor anchor. */
                    x += (p->d > p->w ? -1 : 1) * (int)roundf(3 * scale);
                    y -= (int)roundf(12 * scale) - 2;
                    break;
                }
            person(resident, x, y, 1, roomActor[i] == g.selected && !sleeping);
            if (resident->squad) squad_health_bar(resident, x, y);
        } else if (roomActor[i] == -2) {
            cell_partition(id+1);
        } else {
            room_furniture(&props[i]);
        }
    }
    char roomTitle[64];
    snprintf(roomTitle, sizeof(roomTitle), "%s%s", building_name(id), floor ? " / UPSTAIRS" : "");
    const char *name = roomTitle;
    int titleWidth = text_width(name);
    rect(467-titleWidth,223,titleWidth+9,13,INK);
    text(471-titleWidth,226,name,GOLD,1);
}
/* Map views share the walking camera's axes. North is up-right. */
typedef struct { float wx, wy; int x, y; float sx, sy; } MapView;
static int map_x(MapView v, float x, float y) {
    return v.x + (int)roundf(view_horizontal(x - v.wx, y - v.wy) * v.sx);
}
static int map_y(MapView v, float x, float y) {
    return v.y + (int)roundf(view_vertical(x - v.wx, y - v.wy) * v.sy);
}
static void map_sample(MapView v, int x, int y, float *wx, float *wy) {
    view_to_world((x - v.x) / v.sx, (y - v.y) / v.sy, wx, wy);
    *wx += v.wx;
    *wy += v.wy;
}
static int in_world(float x, float y) { return x >= 0 && y >= 0 && x < WORLD && y < WORLD; }
static int map_known(float x, float y) {
    if (!in_world(x, y))
        return 0;
    int k = ((int)y / 4) * FOG_SIZE + (int)x / 4;
    return g.seen[k >> 3] & (1 << (k & 7));
}
static const uint32_t mapTerrain[] = {
    0x697060, 0x80715b, 0x777a70, 0x4d6c62, 0x64645c, 0x77735e, 0xa9b0ad, 0x465d51, 0x79665d
};
static void map_north(int x, int y) {
    line(x, y + 9, x + 13, y + 2, MUTED);
    line(x + 13, y + 2, x + 8, y + 2, MUTED);
    line(x + 13, y + 2, x + 11, y + 7, MUTED);
    text(x + 17, y, "N", TEXT, 1);
}
/* Route guidance stays inside the map viewport, even for distant pins. */
static void map_pin(MapView view, const Actor *a, int left, int top, int right, int bottom, int edge) {
    if (!ui.waypoint)
        return;
    int px = map_x(view, ui.travelX, ui.travelY), py = map_y(view, ui.travelX, ui.travelY);
    int oldL = clipLeft, oldT = clipTop, oldR = clipRight, oldB = clipBottom;
    clipLeft = left;
    clipTop = top;
    clipRight = right;
    clipBottom = bottom;
    int x = map_x(view, a->x, a->y), y = map_y(view, a->x, a->y);
    if (x >= left && x < right && y >= top && y < bottom) {
        float tx = ui.travelX, ty = ui.travelY;
        if (world_is_expanded())
            army_ford_waypoint(a->x, a->y, &tx, &ty);
        int dx = map_x(view, tx, ty) - x, dy = map_y(view, tx, ty) - y;
        float limit = 1;
        if (dx > 0) limit = fminf(limit, (right - 1 - x) / (float)dx);
        if (dx < 0) limit = fminf(limit, (left - x) / (float)dx);
        if (dy > 0) limit = fminf(limit, (bottom - 1 - y) / (float)dy);
        if (dy < 0) limit = fminf(limit, (top - y) / (float)dy);
        float steps = fmaxf(abs(dx), abs(dy));
        if (steps > 0) {
            for (int n = 1; n < steps * limit; n += 6) {
                float from = n / steps, to = fminf(limit, (n + 3) / steps);
                line(x + dx * from, y + dy * from, x + dx * to, y + dy * to, GOLD);
            }
            if (edge && (limit < 1 || tx != ui.travelX || ty != ui.travelY))
                diamond(x + dx * limit, y + dy * limit, 3, 3, GOLD);
        }
    }
    if (px >= left && px < right && py >= top && py < bottom) {
        diamond(px, py, 5, 5, INK);
        diamond(px, py, 3, 3, GOLD);
    }
    clipLeft = oldL;
    clipTop = oldT;
    clipRight = oldR;
    clipBottom = oldB;
}

static void map_escorts(MapView view, int left, int top, int right, int bottom, int showName) {
    for (int i = 0; i < CONTRACTS; i++) {
        int n = escort_contact(i);
        if (n < 0) continue;
        const Actor *a = &g.actors[n];
        int x = map_x(view, a->x, a->y), y = map_y(view, a->x, a->y);
        if (x < left + 4 || x >= right - 4 || y < top + 4 || y >= bottom - 4)
            continue;
        frame(x-3,y-3,7,7,INK);
        diamond(x,y,3,3,GOLD);
        if (showName && distance(ui.mapX,ui.mapY,a->x,a->y) < 35)
            text(x+7,y-3,"ESCORT",GOLD,1);
    }
}
static void mini_map(void) {
    int ox = 402, oy = 40;
    MapView view = {ui.camX, ui.camY, ox + 33, oy + 30, 2.5f, 1.25f};
    rect(ox - 3, oy - 3, 72, 66, PANEL);
    for (int y = oy; y < oy + 60; y += 3)
        for (int x = ox; x < ox + 66; x += 3) {
            float wx, wy;
            map_sample(view, x + 1, y + 1, &wx, &wy);
            uint32_t c = !in_world(wx, wy) ? INK : terrain_water(wx, wy) ? 0x426e80 : mapTerrain[biome(wx, wy)];
            rect(x, y, 3, 3, shade(c, .63f));
        }
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (portable_bedroll(i) < 0 || fabsf(b->x - ui.camX) > 20 || fabsf(b->y - ui.camY) > 20)
            continue;
        int x = map_x(view, b->x, b->y), y = map_y(view, b->x, b->y);
        if (x >= ox && x < ox + 65 && y >= oy && y < oy + 59)
            rect(x, y, 2, 2, 0xb5aa86);
    }
    for (int i = 0; i < g.nodeCount; i++) {
        const Node *n = &g.nodes[i];
        if (!n->remaining || n->type > NODE_WOOD ||
            fabsf(n->x - ui.camX) > 20 || fabsf(n->y - ui.camY) > 20)
            continue;
        int x = map_x(view, n->x, n->y), y = map_y(view, n->x, n->y);
        if (x >= ox + 1 && x < ox + 64 && y >= oy + 1 && y < oy + 58) {
            uint32_t color = n->type == NODE_WOOD ? 0x91cd86 : n->type == NODE_COPPER ? 0xf2a276 : 0xc4dfed;
            rect(x, y, 2, 2, color);
        }
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead || fabsf(a->x - ui.camX) > 20 || fabsf(a->y - ui.camY) > 20)
            continue;
        int x = map_x(view, a->x, a->y), y = map_y(view, a->x, a->y);
        if (x >= ox && x < ox + 65 && y >= oy && y < oy + 59 && !population_hidden(i))
            rect(x, y, 2, 2, a->squad ? TEAL : a->faction == HOSTILE ? RED : GOLD);
    }
    map_escorts(view, ox, oy, ox + 66, oy + 60, 0);
    map_pin(view, &g.actors[g.selected], ox, oy, ox + 66, oy + 60, 1);
    frame(ox - 3, oy - 3, 72, 66, EDGE);
    text(ox + 55, oy + 3, "N", TEXT, 1);
}
static uint32_t limb_condition_color(const Actor *a, int limb) {
    if (limb_missing(a, limb))
        return RED;
    if (a->health[limb] >= limb_max_health(a, limb) - .05f)
        return TEAL;
    return (a->treatedMask & (1 << limb)) ||
           (a->woundFlags & (1u << (WOUND_REPAIR_SHIFT + limb))) ? GOLD : RED;
}
static void limb_health_bar(const Actor *a, int limb, int x, int y, int w, int h) {
    uint32_t wound = limb_condition_color(a, limb);
    int hp = fmaxf(0, fminf(1, a->health[limb] / limb_max_health(a, limb))) * w;
    rect(x, y, w, h, wound);
    if (!limb_missing(a, limb))
        rect(x, y, hp, h, TEAL);
}

static void detailed_status(void) {
    Actor *a = &g.actors[g.selected];
    char value[64];
    panel(8, 44, 198, 165);
    snprintf(value, sizeof(value), "%.16s", a->name);
    text(16, 53, value, TEAL, 1);
    text(160, 53, "STATUS", GOLD, 1);
    line(16, 66, 198, 66, EDGE);
    for (int i = 0; i < LIMBS; i++) {
        int y = 74 + i * 11;
        int missing = (a->lostMask & (1 << i)) && !(a->prostheticMask & (1 << i));
        uint32_t color = limb_condition_color(a, i);
        text(16, y, limbNames[i], MUTED, 1);
        limb_health_bar(a, i, 86, y + 2, 66, 4);
        if (missing) snprintf(value, sizeof(value), "LOST");
        else snprintf(value, sizeof(value), "%d", (int)a->health[i]);
        text(163, y, value, color, 1);
    }
    const char *labels[] = {"BLOOD", "FOOD", "FATIGUE"};
    float values[] = {a->blood, a->hunger, a->fatigue};
    for (int i = 0; i < 3; i++) {
        int y = 142 + i * 10;
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
    text(16, 176, value, load > capacity ? RED : MUTED, 1);
    snprintf(value, sizeof(value), "BLEED %.1f", a->bleed);
    text(16, 187, value, a->bleed > .01f ? RED : MUTED, 1);
    text(16, 198, "D-PAD UP: HIDE", GOLD, 1);
}
static void hud(void) {
    Actor *a = &g.actors[g.selected];
    char b[120];
    rect(0, 0, 480, 23, INK);
    line(0, 22, 480, 22, EDGE);
    int t = g.interiorBuilding ? building_town(g.interiorBuilding - 1) : town_near(a->x, a->y, 15);
    text(9, 8, t >= 0 ? towns[t].name : regionNames[biome(a->x, a->y)], GOLD, 1);
    if (t >= 0)
        text(155, 8, factionNames[g.townOwner[t]], label_color(g.townOwner[t]), 1);
    snprintf(b, sizeof(b), "DAY %d %02d:%02d", g.day, (int)g.dayTime, (int)(g.dayTime * 60) % 60);
    text(316, 8, b, MUTED, 1);
    snprintf(b, sizeof(b), "%d SILVER", g.money);
    text(472 - (int)strlen(b) * 6, 8, b, TEXT, 1);
    if (ui.screen != SCREEN_WORLD)
        return;
    if (!g.interiorBuilding)
        mini_map();
    recovery_hint(b,sizeof(b));if(b[0]){rect(4,211,472,12,INK);b[77]=0;text(8,213,b,a->bleed>.01f?RED:TEAL,1);}
    if (ui.paused) {
        panel(192, 43, 94, 18);
        center(239, 49, "PAUSED", GOLD, 1);
    } else if (ui.speed == 2)
        text(370, 44, "2X", GOLD, 1);

    rect(0, 239, 480, 13, 0x252b29);
    line(0, 239, 479, 239, EDGE);
    center(240, 242, "\001 ACTIONS  \004 PACK  \010 MAP  \007 MENU  \016 GEAR \017 SQUAD  \012 STATUS \013 SKILLS", TEXT, 1);
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
            bar(x, 268, 48, 2, (m->health[HEAD] + m->health[CHEST]) * 50 / species_health(m), m->bleed > .01f ? RED : TEAL);
            if (m->bleed > .01f) rect(x, 268, 2, 2, RED);
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
    const char *status = a->dead ? "DEAD" : recovery_coma(a) ? "COMA" :
        a->unconscious > 0 ? "DOWN" : a->prisonTime > 0 ? "JAILED" :
        limb_crawling(a) ? "CRAWLING" : limb_limping(a) ? "LIMPING" :
        actor_in_bed(a) ? "RESTING" : a->autoattack ? "FIGHT" : "PASSIVE";
    text(472 - (int)strlen(status) * 6, 259, status, a->dead || a->unconscious > 0 ? RED : TEAL, 1);

    if (a->bleed > .01f) text(370, 259, "BLEED", RED, 1);
    if (a->sneaking && !a->dead && a->bleed <= .01f) {
        line(379, 263, 385, 259, GOLD);
        line(385, 259, 391, 263, GOLD);
        line(379, 263, 385, 267, GOLD);
        line(385, 267, 391, 263, GOLD);
        line(381, 268, 389, 258, GOLD);
    }

    int treating = a->taskKind == TASK_TREAT || a->taskKind == TASK_JAIL_CARE;
    if (treating)
        b[0] = 0;
    else if (a->taskKind == TASK_CELL_LOCK)
        snprintf(b, sizeof(b), "PICKING %s %.0fs   \002 CANCEL",
                 slave_in_camp(a) ? "SHACKLES" : "CELL LOCK", ceilf(a->action));
    else if (jail_cell_open(a))
        snprintf(b, sizeof(b), "%s", interior_context() == -1 ? "\001 LEAVE JAIL" : "MOVE TO THE EXIT");
    else if (a->prisonTime > 0)
        snprintf(b, sizeof(b), "%s", a->unconscious > 0 ? "RECOVERING IN CUSTODY" : "\001 JAIL ACTIONS");
    else if (slave_in_camp(a))
        snprintf(b, sizeof(b), "\001 PICK SHACKLES (%d%%)", lockpick_chance(a, LOCK_SHACKLES));
    else if (a->carrying >= 0)
        snprintf(b, sizeof(b), "\001 CARRYING %.16s", g.actors[a->carrying].name);
    else if (ui.contextKind == 1 && ui.contextIndex >= 0 && g.nodes[ui.contextIndex].owner)
        snprintf(b, sizeof(b), "\001 %s %s", context_action(), context_name());
    else if(nearby_actions())snprintf(b,sizeof(b),"\001 NEARBY ACTIONS");
    else b[0]=0;
    if (b[0]) {
        rect(4, 223, text_width(b) + 16, 13, PANEL);
        text(12, 226, b, TEXT, 1);
    }
    if (treating)
        bar(180, 236, 120, 2, treatment_progress(a), 0xdc9d54);
    else if (a->taskKind)
        bar(180, 236, 120, 2,
            (1 - fminf(1, a->action / (a->taskKind == TASK_CELL_LOCK ? jail_pick_duration(a, a->taskIndex) :
                 a->taskKind == 3 ? REST_DURATION : a->taskKind == 2 ? 4.5f : 2.8f))) * 100,
            a->taskKind == TASK_CELL_LOCK || a->taskKind == 2 ? 0xdc9d54 : GOLD);
    if (ui.statusPanel)
        detailed_status();
}
static void title(void) {
    for(int y=0;y<272;y++)for(int x=0;x<480;x++) {
        uint32_t c=menuColors[menuPixels[y*480+x]];
        uint32_t rgb=((c&255)<<16)|(c&0xff00)|((c>>16)&255);
        float shadeAmount = x < 275 ? .63f * (1-x/375.f) : .05f;
        fb[y*pitch+x]=abgr(blend(rgb,INK,shadeAmount));
    }
    line(30, 45, 205, 45, EDGE);
    text(29, 61, GAME_NAME, GOLD, 3);
    const char *opts[] = {"NEW EXPEDITION", "CONTINUE EXPEDITION", "FIELD GUIDE"};
    for (int i = 0; i < 3; i++) {
        int y = 130 + i * 27;
        if (i == ui.selection) {
            rect(25, y - 5, 255, 20, 0x494132);
            text(32, y, ">", TEAL, 1);
        }
        text(47, y, opts[i], i == ui.selection ? TEXT : MUTED, 1);
    }
    text(31, 251, "\001 BEGIN    VERSION " VERSION, GOLD, 1);
}
static void overlay_header(const char *name, const char *sub) {
    panel(14, 28, 452, 224);
    text(27, 40, name, GOLD, 2);
    text(28, 61, sub, MUTED, 1);
    line(27, 75, 451, 75, EDGE);
    int summons = ui.screen == SCREEN_DIALOG && ui.dialogPage == DIALOG_SUMMONS;
    text(28, 237, summons ? "\010 GUIDE" : ui.menuReturn ? "\002 BACK   \010 GUIDE" : "\002 CLOSE   \010 GUIDE", MUTED, 1);
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
    a.autoattack = 1;
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
    if (d->slot < 0) {
        snprintf(b, sizeof(b), "%s / WEIGHT %d", d->name, d->weight);
        clipped(31, y, b, 68, TEXT);
        if (portion_base(it) == MEDKIT)
            snprintf(b, sizeof(b), "CHARGE %d%%", portion_percent(it));
        else if (food_capacity(it))
            snprintf(b, sizeof(b), "FOOD +%d", (food_capacity(it) * portion_percent(it) + 99) / 100);
        else
            snprintf(b, sizeof(b), "%s", d->desc);
        wrap(31, y + 14, 67, b, MUTED);
        return;
    }
    int worn=a->gear[d->slot];ItemDef *old=&itemDefs[worn];
    clipped(31,y,d->name,68,TEXT);
    if(it>=CATALOG_ITEMS&&it<OLD_ITEMS)text(374,y,it>=CATALOG_ITEMS+16?"SIGNATURE":"SPECIAL",GOLD,1);
    else if((it>=UNIQUE_ITEMS&&it<V090_ITEMS)||regalia_owner(it)>=0)text(404,y,"NAMED",GOLD,1);
    snprintf(b,sizeof(b),"EQUIPPED %.24s",worn?old->name:"EMPTY");clipped(31,y+12,b,33,MUTED);
    if (d->slot == 0) {
        int skill = weapon_class(it);
        snprintf(b,sizeof(b),"Trains %s",skill >= 0 ? weaponClassNames[skill] : "Melee");
        clipped(242,y+12,b,33,MUTED);
    }
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
            snprintf(b,sizeof(b),"STR FOR FULL SPEED %d / HAVE %d",weapon_strength(it),effective_level(a,STRENGTH));
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
static const char *item_function(int it) {
    static char detail[64];
    const ItemDef *d = &itemDefs[it];
    if (d->slot == 0) {
        int c = weapon_class(it);
        snprintf(detail, sizeof(detail), "Trains %s", c >= 0 ? weaponClassNames[c] : "Melee");
    } else if (portion_base(it) == MEDKIT) {
        snprintf(detail, sizeof(detail), "Charge %d%%", portion_percent(it));
    } else if (food_capacity(it)) {
        snprintf(detail, sizeof(detail), "Food +%d", (food_capacity(it) * portion_percent(it) + 99) / 100);
    } else {
        return d->slot >= 0 ? "" : d->desc;
    }
    return detail;
}
static void pack_screen(void) {
    Actor *a=&g.actors[g.selected];char b[120];int count=item_view_count(),it=count?a->pack[ui.selection].item:0;
    overlay_header("PACK & EQUIPMENT","\001 EQUIP / USE   \004 WORN GEAR   \003 PASS TO ALLY");
    float load = encumbrance(a);
    int capacity = actor_capacity(a);
    snprintf(b, sizeof(b), "LOAD %.0f/%d", load, capacity);
    text(449 - text_width(b), 61, b, load > capacity ? GOLD : TEAL, 1);
    text(164,237,"\017 DROP",MUTED,1);
    Actor preview=preview_actor();int wearable=it&&itemDefs[it].slot>=0;
    if(wearable)preview.gear[itemDefs[it].slot]=it;
    portrait_scale(&preview,103,69,28,81,1);
    center(79,150,wearable?"PREVIEW":"WORN",wearable?GOLD:MUTED,1);
    text(371,82,"QTY",MUTED,1);
    text(406,82,"VAL/EA",MUTED,1);
    int first=start_row(item_view_row(ui.selection),count,10);
    for(int j=0;j<10&&first+j<count;j++) {
        int k=item_view_slot(first+j),y=94+j*9,id=a->pack[k].item;
        if(k==ui.selection){rect(139,y-1,310,10,0x4a4030);rect(139,y-1,2,10,TEAL);}
        if (a->stolen[k]) text(146,y,"!",RED,1);
        clipped(155,y,itemDefs[id].name,34,a->stolen[k]?RED:k==ui.selection?TEXT:MUTED);
        snprintf(b,sizeof(b),"%d",a->pack[k].qty);text(389-text_width(b),y,b,GOLD,1);
        snprintf(b,sizeof(b),"%d",itemDefs[id].price);text(442-text_width(b),y,b,TEXT,1);
    }
    snprintf(b, sizeof(b), "%d/%d SLOTS%s", count, PACK,
             count && a->stolen[ui.selection] ? "  STOLEN" : "");
    text(146, 183, b, count && a->stolen[ui.selection] ? RED : MUTED, 1);
    int wornDisguise=disguise_faction(a),nextDisguise=disguise_faction(&preview);
    if (count > 10) {
        snprintf(b, sizeof(b), "%d-%d OF %d", first + 1, first + 10, count);
        text(441 - text_width(b), 183, b, TEAL, 1);
    }
    text(31, 161, "DISGUISE", MUTED, 1);
    int disguise = wearable ? nextDisguise : wornDisguise;
    clipped(31, 171, disguise ? factionNames[disguise] : "NONE", 17, disguise ? TEAL : MUTED);
    if (disguise && g.society.recognized[g.selected][disguise] > 0)
        text(31, 181, "EXPOSED", RED, 1);
    if(!it){text(31,198,"Your pack is empty.",MUTED,1);return;}
    ItemDef*d=&itemDefs[it];
    if(!wearable){snprintf(b,sizeof(b),"%s / WEIGHT %d",d->name,d->weight);clipped(31,197,b,68,TEXT);wrap(31,209,68,item_function(it),MUTED);return;}
    int before=a->gear[d->slot];ItemDef*old=&itemDefs[before];
    compare_stat(0,190,d->slot==0?"DAMAGE":d->slot==5?"CAPACITY":"ARMOR",d->slot==0?old->power:d->slot==5?old->capacity:old->armor,d->slot==0?d->power:d->slot==5?d->capacity:d->armor,0,0);
    compare_stat(1,190,"WEIGHT",old->weight,d->weight,0,1);
    if(!d->slot){compare_stat(2,190,"REACH",before?old->range:1.55f,d->range,1,0);compare_stat(3,190,"DELAY",weapon_handling(a,before)*fmaxf(.55f,(before?old->cooldown:1)-effective_level(a,DEXTERITY)*.003f),weapon_handling(a,it)*fmaxf(.55f,d->cooldown-effective_level(a,DEXTERITY)*.003f),1,1);snprintf(b,sizeof(b),"STR FOR FULL SPEED %d / HAVE %d",weapon_strength(it),effective_level(a,STRENGTH));text(31,214,b,weapon_strength(it)>effective_level(a,STRENGTH)?RED:MUTED,1);}
    else {compare_stat(2,190,"STEALTH",old->stealth,d->stealth,0,0);compare_stat(3,190,d->slot==5?"LOADED WT":"MOVE %",d->slot==5?encumbrance(a):old->speed*.5f,d->slot==5?backpack_load_preview(a,it):d->speed*.5f,1,d->slot==5);compare_stat(4,190,"WEATHER",old->protection,d->protection,0,0);compare_stat(5,190,"CAPACITY",old->capacity,d->capacity,0,0);}
    clipped(31,226,limb_arms(a)==1&&combat_two_hands(it)?"ONE ARM: -40% DAMAGE / -25 AIM / +65% DELAY":item_function(it),69,TEXT);
}
static float skill_progress(float xp, int level) {
    float low = 12.f * (level - 1) * (level - 1), high = 12.f * level * level;
    return level >= SKILL_CAP ? 100 : fminf(100, fmaxf(0, 100 * (xp - low) / (high - low)));
}
static void skills_screen(void) {
    const Actor *a = &g.actors[g.selected];
    char b[106];
    overlay_header(ui.skillPage ? "SKILLS / WEAPONS" : "SKILLS / CORE",
                   "\014 CORE / WEAPONS   \015 SELECT   PROGRESS TO NEXT LEVEL");
    if (ui.skillPage) {
        int selection = ui.selection < WEAPON_CLASSES + 1 ? ui.selection : 0;
        int equipped = weapon_class(a->gear[0]);
        for (int i = 0; i <= WEAPON_CLASSES; i++) {
            int y = 83 + i * 14, aim = i == WEAPON_CLASSES;
            int lv = aim ? level(a, CROSSBOWS) : weapon_class_level(a, i);
            float xp = aim ? a->xp[CROSSBOWS] : g.society.weaponXP[g.selected][i];
            if (i == selection) rect(28, y - 2, 416, 12, 0x4a4030);
            text(31, y, aim ? "Ranged accuracy" : weaponClassNames[i], i == selection ? TEXT : MUTED, 1);
            if (i == equipped) text(212, y, "EQUIPPED", TEAL, 1);
            else if (aim) text(212, y, "ALL RANGED", MUTED, 1);
            snprintf(b, sizeof(b), "%02d", lv);
            text(319, y, b, GOLD, 1);
            bar(348, y + 2, 88, 3, skill_progress(xp, lv), TEAL);
        }
        int aim = selection == WEAPON_CLASSES;
        int lv = aim ? level(a, CROSSBOWS) : weapon_class_level(a, selection);
        float xp = aim ? a->xp[CROSSBOWS] : g.society.weaponXP[g.selected][selection];
        snprintf(b, sizeof(b), "%s   LV %d   %.1f%% %s", aim ? "Ranged accuracy" : weaponClassNames[selection],
                 lv, skill_progress(xp, lv), lv == SKILL_CAP ? "MAXED" : "TO NEXT");
        clipped(31, 187, b, 68, GOLD);
        if (aim) {
            skill_benefit(CROSSBOWS, b, sizeof(b));
            clipped(31, 201, b, 68, TEAL);
            clipped(31, 217, "Land shots with bows, crossbows or firearms to train accuracy.", 68, TEXT);
        } else {
            snprintf(b, sizeof(b), "DAMAGE +%.1f%% / DELAY -%.1f%%", (lv - 1) * .25f, (lv - 1) * .15f);
            text(31, 201, b, TEAL, 1);

        }
        return;
    }
    static const int skills[CORE_SKILL_ROWS] = {
        ATHLETICS, STRENGTH, DEXTERITY, TOUGHNESS, DODGE, MELEE, DEFENCE, MARTIAL, PERCEPTION,
        STEALTH, THEFT, LOCKPICK, MEDICINE, LABOUR, FARMING, COOKING, SMITHING, ENGINEERING, SCIENCE
    };
    static const struct { const char *name; int start, count, x, y; } groups[] = {
        {"BODY & MOVEMENT", 0, 5, 31, 80}, {"COMBAT", 5, 4, 31, 139},
        {"FIELDCRAFT", 9, 4, 245, 80}, {"TRADES", 13, 6, 245, 129}
    };
    for (unsigned k = 0; k < sizeof(groups) / sizeof(*groups); k++) {
        int x = groups[k].x;
        text(x, groups[k].y, groups[k].name, TEAL, 1);
        for (int j = 0; j < groups[k].count; j++) {
            int row = groups[k].start + j, y = groups[k].y + 11 + j * 9, skill = skills[row];
            if (row == ui.selection) rect(x - 3, y - 1, 199, 9, 0x4a4030);
            text(x, y, skillNames[skill], row == ui.selection ? TEXT : MUTED, 1);
            int lv = level(a, skill);
            snprintf(b, sizeof(b), limb_bonus(a, skill) ? "%02d%+d" : "%02d", lv, limb_bonus(a, skill));
            text(x + 137, y, b, GOLD, 1);
            bar(x + 173, y + 2, 18, 3, skill_progress(a->xp[skill], lv), TEAL);
        }
    }
    int skill = skills[ui.selection < CORE_SKILL_ROWS ? ui.selection : 0], lv = level(a, skill);
    snprintf(b, sizeof(b), "%s   LV %d   %.1f%% %s", skillNames[skill], lv,
             skill_progress(a->xp[skill], lv), lv == SKILL_CAP ? "MAXED" : "TO NEXT");
    text(31, 197, b, GOLD, 1);
    skill_benefit(skill, b, sizeof(b));
    clipped(31, 207, b, 68, TEAL);
    int previous_clip = clipBottom;
    clipBottom = 236;
    wrap(31, 218, 68, skill_training(skill), TEXT);
    clipBottom = previous_clip;
}
static void health_screen(int actor, int inspect) {
    if (actor < 0 || actor >= g.actorCount)
        return;
    Actor *a = &g.actors[actor];
    DepthActor *z = &g.depth.actors[actor];
    char b[110];
    overlay_header(inspect ? "CHECK HEALTH / STATS" : "HEALTH & RECOVERY", inspect ? "" :
        z->captive ? "\001 ESCAPE   \003 REPAIR LIMBS   \004 MAINTAIN GEAR" :
                     "\001 BANDAGE   \003 REPAIR LIMBS   \004 MAINTAIN GEAR");
    if(inspect){
        static const char*tab[]={"HEALTH","STATS","BOUNTY"};
        for(int i=0;i<3;i++)text(31+i*138,61,tab[i],ui.inspectTab==i?TEAL:MUTED,1);
        text(257,237,"\014 / \005 / \006 TAB",MUTED,1);
        if(ui.inspectTab==1){
            snprintf(b,sizeof(b),"%.16s / %s",a->name,factionNames[a->faction]);clipped(31,83,b,67,GOLD);
            for(int i=0;i<SKILLS;i++){
                int x=i<10?31:245,y=102+(i%10)*12;
                text(x,y,skillNames[i],MUTED,1);snprintf(b,sizeof(b),"%d",level(a,i));text(x+162,y,b,TEXT,1);
            }
            int c=weapon_class(a->gear[0]);
            if(c>=0){snprintf(b,sizeof(b),"%s %d / %.34s",weaponClassNames[c],weapon_class_level(a,c),itemDefs[a->gear[0]].name);clipped(31,224,b,68,TEAL);}
            return;
        }
        if(ui.inspectTab==2){
            text(31,85,a->name,GOLD,1);
            if(!bounty_payers(actor)){text(31,110,"No known bounty.",MUTED,1);return;}
            snprintf(b,sizeof(b),"ALIVE  %d SILVER",bounty_reward(actor,1));text(31,110,b,TEAL,1);
            snprintf(b,sizeof(b),"DEAD   %d SILVER",bounty_reward(actor,0));text(31,128,b,MUTED,1);
            text(31,154,"ACCEPTED BY",GOLD,1);bounty_details(actor,b,sizeof(b));wrap(31,169,66,b+6,TEXT);
            return;
        }
    }
    snprintf(b, sizeof(b), "%.16s / %s", a->name, a->dead ? "DEAD" : recovery_coma(a) ? "RECOVERY COMA" :
             a->unconscious > 0 ? "KNOCKED OUT" : "AWAKE");
    text(31, 83, b, a->unconscious > 0 ? GOLD : TEAL, 1);
    text(177,98,"HEALTH",MUTED,1);text(323,98,"WOUND / TRAUMA",MUTED,1);
    for(int i=0;i<LIMBS;i++) {
        int y=111+i*13,missing=(a->lostMask&(1<<i))&&!(a->prostheticMask&(1<<i));
        if(!inspect && i==ui.selection)rect(25,y-2,425,12,0x4a4030);
        text(31,y,limbNames[i],!inspect && i==ui.selection?TEXT:MUTED,1);
        limb_health_bar(a, i, 157, y + 2, 91, 4);
        if(missing)snprintf(b,sizeof(b),"LOST");else snprintf(b,sizeof(b),"%.0f/%.0f",a->health[i],limb_max_health(a,i)-z->trauma[i]);
        text(261,y,b,missing?RED:TEXT,1);
        uint32_t color = limb_condition_color(a, i);
        const char *wound = missing ? "LOST" : a->woundFlags & (1u << i) ?
            (a->bleed > .01f ? "OPEN CUT" : "CLOTTED CUT") :
            a->woundFlags & (1u << (WOUND_REPAIR_SHIFT + i)) ? "REPAIRING" :
            a->treatedMask & (1 << i) ? "DRESSED" : color == TEAL ? "OK" :
            !depth_organic(a, i) ? "DAMAGED" : "UNTREATED";
        snprintf(b, sizeof(b), "%s / %.0f", wound, z->trauma[i]);
        text(323, y, b, color, 1);
    }
    snprintf(b, sizeof(b), "BLOOD %.0f   FOOD %.0f   FATIGUE %.0f", a->blood, a->hunger, a->fatigue);
    text(31, 194, b, a->bleed > .01f ? RED : MUTED, 1);
    if (a->bleed > .01f) text(323, 194, "BLEEDING", RED, 1);
    if (inspect)
        return;
    snprintf(b, sizeof(b), "DRESSINGS %d  GEAR %.0f%%  COMA THRESHOLD %.0f", inventory_count(a,MEDKIT),
             z->upkeep, coma_threshold(a));
    text(31,207,b,GOLD,1);
    text(31, 221, z->captive ? "\001 PICK SHACKLES" : "TEAL HEALTH   RED UNTREATED   GOLD DRESSED   \006 LIMBS", MUTED, 1);
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
  }else{snprintf(b,sizeof(b),"GOVERNMENT: %s",factionNames[g.townOwner[t]]);clipped(31,100,b,68,TEAL);
   int leader=p->leader-1;
   snprintf(b,sizeof(b),"LEADER: %s",leader>=0&&politics_leader_title(leader)?g.actors[leader].name:"VACANT");
   clipped(31,114,b,68,GOLD);wrap(31,137,68,p->policy==1?"Relief administration: food, water and medicine prices reduced 15%.":p->policy==2?"Military administration: equipment prices increased 12%.":"Trade administration: supply prices reduced 7%.",TEXT);snprintf(b,sizeof(b),"REFUGEES %d / SHIPMENTS %d",p->refugees,p->shipments);text(31,184,b,GOLD,1);wrap(31,207,68,"The former ruler and their unique equipment remain where they fell.",MUTED);}return;
 }
 overlay_header("WORLD ACTIVITY","\015 SELECT   \001 COUNCIL / MAP   \002 CLOSE");int count=life_ledger_count(),first=start_row(ui.selection,count,6);for(int j=0;j<6&&first+j<count;j++){life_ledger_label(first+j,b,sizeof(b));row(84+j*14,first+j==ui.selection,b,NULL);}life_ledger_detail(ui.selection,b,sizeof(b));wrap(31,178,68,b,TEXT);
}

static void squad_screen(void) {
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &g.depth.actors[g.selected];
    char b[120];
    overlay_header("SQUAD", "\005/\006 MEMBER   \004 SWITCH TAB   \014 CHANGE");
    rect(ui.squadPage ? 155 : 29, 79, 121, 15, 0x464438);
    text(35, 83, "1 / ASSIGNMENT", ui.squadPage ? MUTED : GOLD, 1);
    text(161, 83, "2 / PRIORITIES", ui.squadPage ? GOLD : MUTED, 1);
    snprintf(b, sizeof(b), "%.16s / %s", a->name, job_status(a));
    clipped(31, 103, b, 68, TEAL);
    if (ui.squadPage) {
        row(121, ui.selection == 0, "Job list", !z->queued ? "OFF" : z->paused ? "PAUSED" : "ACTIVE");
        for (int i = 0; i < 4; i++) {
            snprintf(b, sizeof(b), "Priority %d", i + 1);
            row(139 + i * 15, ui.selection == i + 1, b, jobNames[z->queue[i]]);
        }
        text(31, 208, "First available job runs. Put Follow or Hold last.", MUTED, 1);
    } else {
        row(121, ui.selection == 0, "Direct job", slave_in_camp(a) ? "Forced labour" : jobNames[a->job]);
        row(139, ui.selection == 1, "Combat orders", orderNames[z->order]);
        snprintf(b, sizeof(b), "%d / 3", z->group + 1);
        row(157, ui.selection == 2, "Follow spacing", b);
        if (z->queued) {
            snprintf(b, sizeof(b), "Priority assignment: %s. Changing the direct job turns priorities off.",
                     jobNames[assigned_job(a)]);
            clipped(31, 182, b, 69, MUTED);
        } else
            text(31, 182, "Follow tracks the member you control.", MUTED, 1);
        text(31, 196, "The controlled member works after 5 idle seconds in Passive stance.", MUTED, 1);
    }
    if (a->dead)
        text(31, 222, "\003 DISMISS FALLEN", GOLD, 1);
}
static void pass_screen(void) {
    const Actor *a = &g.actors[g.selected];
    char title[96], detail[48];
    int item = a->pack[ui.passSlot].item;
    overlay_header("PASS TO ALLY", "\015 RECIPIENT   \001 HAND OVER ONE   \002 CANCEL");
    snprintf(title, sizeof(title), "%s is passing 1 %s", a->name, itemDefs[item].name);
    clipped(31, 84, title, 68, GOLD);
    int first = start_row(ui.selection, g.squadCount - 1, 6);
    for (int i = 0; i < 6; i++) {
        int rowIndex = first + i, n = pass_recipient(rowIndex);
        if (n < 0)
            break;
        const Actor *b = &g.actors[n];
        const char *why = pass_blocked(n);
        int y = 103 + i * 19;
        if (rowIndex == ui.selection)
            rect(26, y - 3, 425, 15, 0x413b33);
        text(31, y, b->name, why ? MUTED : TEXT, 1);
        snprintf(detail, sizeof(detail), "%s", why ? why : "Within reach");
        text(442 - text_width(detail), y, detail, why ? MUTED : TEAL, 1);
    }
    if (pass_recipient(0) < 0)
        text(31, 108, "No other squad members.", MUTED, 1);
    text(31, 226, "Recipients must be within 3m and in the same room.", MUTED, 1);
}
/* World overview: geography + a named destination browser, not a tiny free cursor. */
static void atlas_name(int id,char *out,int size) {
    if(id==-1)snprintf(out,size,"YOUR POSITION");
    else if(id<TOWNS)snprintf(out,size,"%s",towns[id].name);
    else living_title(id-TOWNS,out,size);
}
static void map_roads(MapView view) {
    for (int t = 1; t < TOWNS; t++) {
        const Town *a = &towns[t], *b = &towns[0];
        float best = 1e9f;
        for (int j = 0; j < t; j++) {
            float d = distance(a->x, a->y, towns[j].x, towns[j].y);
            if (d < best) {
                best = d;
                b = &towns[j];
            }
        }
        int steps = (int)best / 2 + 1;
        float dx = (b->x - a->x) / steps, dy = (b->y - a->y) / steps;
        for (int k = 0; k < steps; k++) {
            float x = a->x + dx * k, y = a->y + dy * k;
            if (map_known(x, y))
                line(map_x(view, x, y), map_y(view, x, y), map_x(view, x + dx, y + dy),
                     map_y(view, x + dx, y + dy), 0xc1b08a);
        }
    }
}
static void world_atlas(void) {
    ui.mapCategory = 0;
    enum { size = 206 };
    const int ox = 14, oy = 42;
    const float scale = (size - 8) / (2.f * WORLD);
    MapView view = {WORLD * .5f, WORLD * .5f, ox + size / 2, oy + size / 2, scale, scale * .68f};
    Actor *a = &g.actors[g.selected];
    int count = atlas_count();
    if (ui.mapChoice >= count) atlas_select(0);
    int selected = atlas_item(ui.mapChoice);
    rect(0, 0, 480, 272, PANEL);
    text(14, 6, "WORLD ATLAS", TEXT, 2);
    text(341, 8, "\003 USE CHART", MUTED, 1);
    static uint32_t pixels[size * size], cacheKey;
    static int ready;
    uint32_t key = g.seed;
    for (unsigned i = 0; i < sizeof(g.seen); i++) key = (key ^ g.seen[i]) * 16777619u;
    if (!ready || key != cacheKey) {
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                float wx, wy;
                map_sample(view, ox + x, oy + y, &wx, &wy);
                uint32_t c = INK;
                if (in_world(wx, wy)) {
                    c = 0x343e3b;
                    if (map_known(wx, wy))
                        c = terrain_water(wx, wy) ? 0x427384 : mapTerrain[biome(wx, wy)];
                }
                pixels[y * size + x] = abgr(c);
            }
        cacheKey = key;
        ready = 1;
    }
    for (int y = 0; y < size; y++)
        memcpy(fb + (oy + y) * pitch + ox, pixels + y * size, size * sizeof(uint32_t));
#define AX(x, y) map_x(view, x, y)
#define AY(x, y) map_y(view, x, y)
    int oldL=clipLeft,oldR=clipRight,oldT=clipTop,oldB=clipBottom;
    clipLeft=ox;clipRight=ox+size;clipTop=oy;clipBottom=oy+size;
    map_roads(view);
    for(int t=0;t<TOWNS;t++)if(g.discovered[t]){
        int x=AX(towns[t].x, towns[t].y),y=AY(towns[t].x, towns[t].y);
        if(t>=OLD_TOWNS){diamond(x,y,5,5,INK);diamond(x,y,3,3,label_color(g.townOwner[t]));}
        else{rect(x-3,y-3,7,7,INK);frame(x-2,y-2,5,5,label_color(g.townOwner[t]));}
    }
    int px=AX(a->x, a->y),py=AY(a->x, a->y);
    map_escorts(view, ox, oy, ox + size, oy + size, 1);
    map_pin(view, a, ox, oy, ox + size, oy + size, 0);
    triangle(px,py-6,px-5,py+4,px+5,py+4,TEXT);triangle(px,py-4,px-3,py+3,px+3,py+3,0x72cfd2);
    int sx=AX(ui.mapX, ui.mapY),sy=AY(ui.mapX, ui.mapY);
    frame(sx-6,sy-6,13,13,GOLD);
    /* One named callout, bounded within the map. No numeric lookup or overlapping labels. */
    char name[80];atlas_name(selected,name,sizeof(name));
    int width=text_width(name)+8;if(width>198)width=198;
    int lx=sx-width/2,ly=sy-25;if(lx<ox+3)lx=ox+3;if(lx+width>ox+size-3)lx=ox+size-3-width;
    if(ly<oy+3)ly=sy+12;if(ly>oy+size-16)ly=oy+size-16;
    line(sx,sy+(ly>sy?7:-7),sx,ly+(ly>sy?0:13),GOLD);
    rect(lx,ly,width,13,INK);clipped(lx+4,ly+3,name,31,selected==-1?0x72cfd2:GOLD);

    text(20, 236, "ALIGNED TO WALKING VIEW", MUTED, 1);
    clipLeft=oldL;clipRight=oldR;clipTop=oldT;clipBottom=oldB;
    frame(ox-1,oy-1,size+2,size+2,EDGE);
    map_north(187, 47);
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
        const char *dir = bearing(ui.mapX - a->x, ui.mapY - a->y);
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
    overlay_header("LOCAL MAP / BEARINGS", "\011 MOVE  \001 PIN  \005/\006 TOWNS  \004 ATLAS  \003 CHART");
    float nearest = 1e9f;
    for (int t = 0; t < TOWNS; t++) {
        float d = distance(ui.mapX, ui.mapY, towns[t].x, towns[t].y);
        if (d < nearest) {
            nearest = d;
            ui.mapTown = t;
        }
    }
    enum { ox = 29, oy = 82, mw = 288, mh = 136 };
    MapView view = {(int)ui.mapX, (int)ui.mapY, ox + mw / 2, oy + mh / 2, 1.05f, .525f};
#define MAPX(x, y) map_x(view, x, y)
#define MAPY(x, y) map_y(view, x, y)
#define INMAP(x, y) (MAPX(x, y) >= ox && MAPX(x, y) < ox + mw && MAPY(x, y) >= oy && MAPY(x, y) < oy + mh)
    static uint32_t atlas[mw * mh], cachedHash;
    static int ready;
    uint32_t h = (g.seed ^ (unsigned)view.wx) * 16777619u;
    h = (h ^ (unsigned)view.wy) * 16777619u;
    for (unsigned i = 0; i < sizeof(g.seen); i++)
        h = (h ^ g.seen[i]) * 16777619u;
    if (!ready || h != cachedHash) {
        for (int y = 0; y < mh; y++)
            for (int x = 0; x < mw; x++) {
                float wx, wy;
                map_sample(view, ox + x, oy + y, &wx, &wy);
                uint32_t c = INK;
                if (in_world(wx, wy)) {
                    c = mapTerrain[biome(wx, wy)];
                    if (terrain_water(wx, wy))
                        c = 0x426e80;
                    else if (terrain_height(wx, wy) / 4 != terrain_height(wx + 2, wy) / 4)
                        c = shade(c, .8f);
                    if (!map_known(wx, wy))
                        c = shade(c, .40f);
                }
                atlas[y * mw + x] = abgr(c);
            }
        cachedHash = h;
        ready = 1;
    }
    for (int y = 0; y < mh; y++)
        memcpy(fb + (oy + y) * pitch + ox, atlas + y * mw, mw * sizeof(uint32_t));
    int oldL = clipLeft, oldR = clipRight, oldT = clipTop, oldB = clipBottom;
    clipLeft = ox; clipRight = ox + mw; clipTop = oy; clipBottom = oy + mh;
    map_roads(view);
    for (int t = 0; t < TOWNS; t++)
        if (g.discovered[t] && INMAP(towns[t].x, towns[t].y)) {
            int x = MAPX(towns[t].x, towns[t].y), y = MAPY(towns[t].x, towns[t].y);
            if(t>=OLD_TOWNS){diamond(x,y,5,5,INK);diamond(x,y,3,3,label_color(g.townOwner[t]));}
            else{rect(x - 2, y - 2, 5, 5, factionColors[g.townOwner[t]]);frame(x - 2, y - 2, 5, 5, GOLD);}
            char label[28];snprintf(label,sizeof(label),"%02d",t+1);
            if(t==ui.mapTown || distance(towns[t].x, towns[t].y, ui.mapX, ui.mapY) < 70)text(x+4,y-6,label,t==ui.mapTown?TEXT:GOLD,1);
            if(t==ui.mapTown)frame(x-4,y-4,9,9,TEXT);
        }
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && !g.actors[i].dead && INMAP(g.actors[i].x, g.actors[i].y))
            rect(MAPX(g.actors[i].x, g.actors[i].y) - 1, MAPY(g.actors[i].x, g.actors[i].y) - 1, 3, 3, TEAL);
    for(int i=0;i<LANDMARKS;i++)if(g.frontier.knownSites[i] && INMAP(g.depth.siteX[i],g.depth.siteY[i])) {
        int x=MAPX(g.depth.siteX[i], g.depth.siteY[i]),y=MAPY(g.depth.siteX[i], g.depth.siteY[i]);diamond(x,y,4,3,0x151b19);diamond(x,y,2,2,GOLD);
        if(distance(ui.mapX,ui.mapY,g.depth.siteX[i],g.depth.siteY[i])<12)
            clipped(x>ox+120?x-110:x+6,y+5,landmarkNames[i],18,GOLD);
    }
    if(g.frontier.tollKnown && INMAP(g.regions.campX,g.regions.campY)) {
        int cx=MAPX(g.regions.campX, g.regions.campY),cy=MAPY(g.regions.campX, g.regions.campY);diamond(cx,cy,4,3,g.regions.outcome[1]?TEAL:RED);
        clipped(cx+6,cy-9,g.regions.outcome[1]?"TOLL AGREEMENT":"TOLL CAMP",18,TEXT);
    }
    for (int i = 0; i < LODGES; i++)
        if (g.frontier.lodge[i] && g.frontier.knownLodges[i]) {
            const Building *b = &g.buildings[g.frontier.lodge[i] - 1];
            if (INMAP(b->x, b->y)) {
                int x = MAPX(b->x, b->y), y = MAPY(b->x, b->y);
                diamond(x,y,3,3,GOLD);
                if (distance(ui.mapX, ui.mapY, b->x, b->y) < 14)
                    clipped(x+6,y-4,"DUSTWARD LODGE",16,TEXT);
            }
        }
    if (g.baseExists && INMAP(g.baseX, g.baseY))
        frame(MAPX(g.baseX, g.baseY) - 3, MAPY(g.baseX, g.baseY) - 3, 7, 7, TEAL);
    Actor *leader = &g.actors[g.selected];
    int youx = MAPX(leader->x, leader->y), youy = MAPY(leader->x, leader->y);
    int x = MAPX(ui.mapX, ui.mapY), y = MAPY(ui.mapX, ui.mapY);
    if (INMAP(leader->x, leader->y)) {
        if(!ui.waypoint) {
            float dx=x-youx,dy=y-youy,steps=fmaxf(fabsf(dx),fabsf(dy));
            for(int k=0;k<steps;k+=6) line(youx+dx*k/fmaxf(1,steps),youy+dy*k/fmaxf(1,steps),
                youx+dx*fminf(k+3,steps)/fmaxf(1,steps),youy+dy*fminf(k+3,steps)/fmaxf(1,steps),GOLD);
        }
        diamond(youx, youy, 5, 4, TEAL);
        frame(youx - 5, youy - 5, 11, 11, TEXT);
    }
    line(x - 5, y, x + 5, y, TEXT);
    line(x, y - 5, x, y + 5, TEXT);
    map_escorts(view, ox, oy, ox + mw, oy + mh, 1);
    map_pin(view, leader, ox, oy, ox + mw, oy + mh, 0);
    clipLeft=oldL;clipRight=oldR;clipTop=oldT;clipBottom=oldB;
    frame(ox - 1, oy - 1, mw + 2, mh + 2, EDGE);
    text(328,84,"NEAREST TOWN",GOLD,1);
    char info[80];snprintf(info,sizeof(info),"%02d %s",ui.mapTown+1,g.discovered[ui.mapTown]?towns[ui.mapTown].name:"UNCHARTED");wrap(328,100,19,info,TEXT);
    text(328,131,"CONTROL",MUTED,1);wrap(328,143,19,g.discovered[ui.mapTown]?factionNames[g.townOwner[ui.mapTown]]:"Unknown",GOLD);
    text(328,174,"MAP KEY",MUTED,1);
    text(328,187,"01 TOWN / + PIN",GOLD,1);
    text(328,198,"PALE: ROAD",MUTED,1);
    text(328,209,"TEAL: YOU",TEAL,1);
    map_north(287, 87);
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
    overlay_header(ui.sell ? "SELL / CARRIED" : ui.tradeSmith == TRADE_SMITH ? "SMITHY / ARMS & ARMOR" :
                   ui.tradeSmith == TRADE_INN ? "INN / FOOD & DRINK" :
                   ui.tradeSmith == TRADE_ARMORER ? "ARMORER / FITTED ARMOR" :
                   ui.tradeSmith == TRADE_APOTHECARY ? "SURGEON / SUPPLIES" : titles[ui.shopFilter],
                   ui.sell ? "\014 BUY-SELL   \001 SELL ONE"
                           : "\014 BUY-SELL   \005/\006 FILTER   \001 BUY ONE");
    int count = ui.sell ? item_view_count() : shop_count(), first = start_row(ui.sell ? item_view_row(ui.selection) : ui.selection, count, 6);
    for (int j = 0; j < 6 && first + j < count; j++) {
        int i = ui.sell ? item_view_slot(first+j) : first+j, it = ui.sell ? g.actors[g.selected].pack[i].item : shop_item(i);
        int y = 84 + j * 13;
        int stolen = ui.sell && g.actors[g.selected].stolen[i];
        row(y,i==ui.selection,"",NULL);
        if (stolen) text(31,y,"!",RED,1);
        clipped(42,y,itemDefs[it].name,35,stolen?RED:i==ui.selection?TEXT:MUTED);
        snprintf(v,sizeof(v),"%d",ui.sell?g.actors[g.selected].pack[i].qty:market_quantity(ui.tradeTown, it));text(263,84+j*13,v,MUTED,1);
        snprintf(v,sizeof(v),"%d S",item_price(ui.tradeTown,it,ui.sell));text(301,84+j*13,v,GOLD,1);
        if (stolen) {
            snprintf(v,sizeof(v),"%.0f%% CLEAR",stolen_sale_chance(ui.tradeTown,i)*100);
            text(375,y,v,RED,1);
        } else {
            int pct=market_percent(ui.tradeTown,it,ui.sell);snprintf(v,sizeof(v),"%+d%%",pct);
            text(400,y,v,pct==0?MUTED:((ui.sell?pct>0:pct<0)?0x73c58b:0xde8375),1);
        }
    }
    int it = ui.sell ? g.actors[g.selected].pack[ui.selection].item : shop_item(ui.selection);
    equipment_comparison(it, 173);
    text(ui.sell ? 205 : 290,237,ui.sell ? "! STOLEN / % CHANCE OF SALE" : "GREEN GOOD / RED POOR",MUTED,1);
    snprintf(b, sizeof(b), "%d SILVER", g.money);
    text(360, 45, b, GOLD, 1);
}

static void build_screen(void) {
    overlay_header("CAMP", "\001 CHOOSE A SPOT");
    char value[40];Actor*a=&g.actors[g.selected];
    snprintf(value,sizeof(value),"TIMBER %d/%d",inventory_count(a,WOOD),build_wood_cost(0));
    row(101,ui.selection==0,"Campfire",value);
    snprintf(value,sizeof(value),"BEDROLL %d/1",inventory_count(a,BEDROLL_ITEM));
    row(127,ui.selection==1,"Bedroll",value);
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
    const char *tiers[] = {"I   METALWORK / SABRES", "II  ARMOR FORGING / PLATE CUIRASS",
                           "III MASTERWORK FORGING"};
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
    static const char *tabs[] = {"PAUSE", "SQUAD", "WORLD", "CAMP & HOME"};
    for (int i = 0; i < 4; i++) {
        int x = 28 + i * 107;
        if (i == ui.menuTab) rect(x, 80, 103, 17, 0x494132);
        center(x + 51, 85, tabs[i], i == ui.menuTab ? TEAL : MUTED, 1);
    }
    int count = pause_menu_count();
    for (int i = 0; i < count; i++) {
        char value[24] = "";
        if (ui.menuTab == 0 && i == 3) snprintf(value, sizeof(value), "%dX", ui.speed);
        row(103 + i * 14, ui.selection == i, pause_menu_label(i), value);
        if(ui.menuTab==3 && i==1 && (!g.interiorBuilding || home_state(g.interiorBuilding-1)!=HOME_OWNED)) {
            rect(31,102+i*14,400,10,ui.selection==i?0x4a4030:INK);
            text(32,103+i*14,pause_menu_label(i),0x68645b,1);
        }
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
static void scenarios_screen(void) {
    overlay_header("STARTING SCENARIO", "\015 BROWSE   \001 CREATE CHARACTER   \002 BACK");
    for (int i = 0; i < START_SCENARIOS; i++) {
        const StartScenario *start = &startScenarios[i];
        char b[64];
        snprintf(b, sizeof(b), "%s / %s", towns[start->town].name, start->difficulty);
        row(84 + i * 16, ui.selection == i, originNames[i], b);
    }
    line(30, 159, 449, 159, EDGE);
    const StartScenario *start = &startScenarios[ui.selection];
    wrap(31, 172, 67, start->description, TEXT);
    text(31, 224, "Every scenario plays in the same frontier.", MUTED, 1);
}
static Actor creator_preview(void) {
    Actor a = ui.titleChoice ? preview_actor() : (Actor){.active = 1, .squad = 1, .facing = 1, .carrying = -1, .carriedBy = -1};
    a.body = ui.createBody;
    a.skin = ui.createSkin;
    a.hair = ui.createHair;
    a.hairColor = ui.createHairColor;
    a.beard = ui.createBeard;
    a.scar = 0;
    a.race = ui.createRace;
    if (!ui.titleChoice) {
        starting_gear(&a, ui.createOrigin);
        species_start(&a, ui.createOrigin, ui.createRace);
    }
    if (ui.selection >= 3 && ui.selection <= 6)
        a.gear[2] = NONE;
    return a;
}
static void creator_screen(void) {
    overlay_header(ui.titleChoice ? "THE BARBER" : "CREATE CHARACTER", "\014 CHANGE   \001 CHOOSE   \002 BACK");
    Actor a = creator_preview();
    rect(27, 78, 163, 123, 0x6f6250);
    portrait_scale(&a, 160, 109, 28, 81, 1.8f);
    const SpeciesDef *species = &speciesDefs[species_index(&a)];
    static const char *labels[] = {"NAME", "SPECIES", "BUILD", "SKIN", "HAIR", "HAIR TONE", "BEARD"};
    static const char *builds[] = {"Regular", "Broad", "Lean"};
    static const char *hair[] = {"Bald", "Cropped", "Side part", "Topknot", "Long", "Crest", "Ponytail"};
    static const char *beard[] = {"Shaven", "Stubble", "Goatee", "Full"};
    static const char *tones[] = {"Black", "Brown", "Ash brown", "Sandy", "Grey", "Red"};
    if (!ui.titleChoice) {
        char scenario[56];
        snprintf(scenario, sizeof(scenario), "%s / %s", originNames[ui.createOrigin],
                 towns[startScenarios[ui.createOrigin].town].name);
        clipped(205, 81, scenario, 40, GOLD);
    }
    char skin[20];
    snprintf(skin, sizeof(skin), "Tone %d", ui.createSkin + 1);
    const char *values[] = {ui.createName, raceNames[ui.createRace], builds[ui.createBody], skin,
                            hair[ui.createHair], tones[ui.createHairColor], beard[ui.createBeard]};
    for (int i = 0; i < 8; i++) {
        int y = 96 + i * 13;
        if (i == ui.selection) {
            rect(201, y - 2, 249, 12, 0x464438);
            rect(201, y - 2, 2, 12, GOLD);
        }
        int off = (i == 1 && ui.titleChoice) || (i >= 4 && i <= 6 && ui.createRace == 2);
        text(208, y, i < 7 ? labels[i] : ui.titleChoice ? "KEEP CHANGES" : "BEGIN EXPEDITION",
             i == 7 ? GOLD : MUTED, 1);
        if (i < 7) {
            const char *v = i >= 4 && ui.createRace == 2 ? "--" : values[i];
            text(441 - text_width(v), y, v, off ? MUTED : TEXT, 1);
        }
    }
    char stats[100];
    snprintf(stats, sizeof(stats), "HP %d  LOAD %d  BASE MOVE %d%%  FOOD %d%%",
             species->health, actor_capacity(&a), (int)roundf(species->move * 100),
             (int)roundf(species->food * 100));
    text(30, 207, stats, TEAL, 1);
    snprintf(stats, sizeof(stats), "STR %d  DEX %d  TOUGH %d  DODGE %d  AGI %d  SNEAK %d  ENG %d",
             level(&a, STRENGTH), level(&a, DEXTERITY), level(&a, TOUGHNESS), level(&a, DODGE),
             level(&a, ATHLETICS), level(&a, STEALTH), level(&a, ENGINEERING));
    text(30, 217, stats, MUTED, 1);
    clipped(30, 227, species->trait, 69, TEXT);
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
    Actor preview = preview_actor();
    portrait_scale(&preview, 160, 128, 29, 78, 1.9f);
    for (int i = 0; i < GEARS; i++) {
        int y = 86 + i * 18;
        if (i == ui.selection)
            rect(195, y - 3, 252, 16, 0x3c4034);
        text(202, y, slotNames[i], MUTED, 1);
        if (a->gearStolen[i]) text(246, y, "!", RED, 1);
        clipped(254, y, itemDefs[a->gear[i]].name, 31, a->gearStolen[i] ? RED : i == ui.selection ? TEXT : GOLD);
    }
    int it = a->gear[ui.selection];
    char b[100];
    if(!it) snprintf(b,sizeof(b),"%s: EMPTY",slotNames[ui.selection]);
    else if(item_effect_value(it))snprintf(b,sizeof(b),"%s / WEIGHT %d / %s %d",slotNames[ui.selection],itemDefs[it].weight,
        effectNames[itemDefs[it].effect],item_effect_value(it));
    else snprintf(b,sizeof(b),"%s / WEIGHT %d",slotNames[ui.selection],itemDefs[it].weight);
    clipped(30, 212, b, 68, TEAL);
    if (it && itemDefs[it].slot == 0) {
        int skill = weapon_class(it);
        snprintf(b, sizeof(b), "Trains %s", skill >= 0 ? weaponClassNames[skill] : "Melee");
        text(30, 224, b, GOLD, 1);
    }
}
static void requirement_cell(int cell,const char*label,int need,int have){
    int x=29+(cell%2)*211,y=177+(cell/2)*13;char b[60];clipped(x,y,label,22,TEXT);snprintf(b,sizeof(b),"%d/%d",have,need);text(x+138,y,b,have>=need?0x73c58b:RED,1);
}
static void forge_screen(void) {
    overlay_header("EQUIPMENT FORGE", "\014 CONFIGURE   \001 CHOOSE");
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
static void dialogue_row_text(int y, const char *option, int selected) {
    enum { LEFT = 137, RIGHT = 441 };
    int overflow = text_width(option) - (RIGHT - LEFT);
    int shift = 0;
    if (selected && overflow > 0) {
        const float speed = 32, start = .65f, end = 1.1f;
        float elapsed = fmodf(ui.dialogHover, start + overflow / speed + end);
        shift = (int)fminf(overflow, fmaxf(0, (elapsed - start) * speed));
    }
    int savedLeft = clipLeft, savedRight = clipRight;
    clipLeft = LEFT;
    clipRight = RIGHT;
    text(LEFT - shift, y, option, selected ? GOLD : MUTED, 1);
    clipLeft = savedLeft;
    clipRight = savedRight;
}
static void dialogue_screen(void) {
    int n = ui.dialogActor;
    if (n < 0 || n >= g.actorCount)
        return;
    Actor *a = &g.actors[n];
    int jailed = a->prisonTime > 0 && n == g.selected;
    overlay_header(jailed ? "HOLDING CELL" : ui.dialogPage == DIALOG_SUMMONS ? "THE WATCH STOPS YOU" : a->name,
                   ui.dialogPage == DIALOG_SUMMONS ? "\001 CONFIRM   \015 CHOOSE" :
                   "\001 CONFIRM   \015 CHOOSE   \002 BACK   \010 GUIDE");
    portrait_scale(a, 94, 108, 28, 83, 1);
    clipped(30, 204, npc_role_name(a), 16, TEAL);
    clipped(30, 218, factionNames[a->faction], 16, factionColors[a->faction]);
    wrap(134, 85, 50, dialogue_line(n, ui.dialogPage), TEXT);
    static const char *work[] = {"Dispatch for the watch", "Iron to repair the smithy",
        "Clear predators or raiders from the road", "Salvage for the town pump", "Freight delivery",
        "Escort a supply caravan", "Patrol with the watch", "Bring in a wanted raider"};
    char bail[48];
    snprintf(bail, sizeof(bail), "Pay %d silver bail", jail_bail_cost(a));
    char fine[48];
    snprintf(fine, sizeof(fine), "Pay %d silver to clear bounty", wanted_amount(&g.actors[g.selected], a->faction));
    const char *summons[] = {fine, "Serve time in jail"};
    const char *review[] = {fine, "Ask about local news", "Back"};
    char pick[40];
    snprintf(pick, sizeof(pick), "Pick the cell lock (%d%%)", lockpick_chance(a, LOCK_CELL));
    const char *prison[] = {bail, pick, "Wait out the sentence"};
    const char *inspection[] = {"Let the guard search", "Refuse and leave"};
    int town = town_near(g.actors[g.selected].x, g.actors[g.selected].y, 14);
    int count = jailed ? 3 : ui.dialogPage == DIALOG_WORK ? work_dialogue_count(town)
        : ui.dialogPage == DIALOG_BOUNTY ? 3 : (ui.dialogPage == DIALOG_INSPECTION || ui.dialogPage == DIALOG_SUMMONS) ? 2
        : (ui.dialogPage == DIALOG_DISMISS || ui.dialogPage == DIALOG_WORK_HINT) ? 1 : dialogue_count(n);
    int first = start_row(ui.selection, count, 5);
    for (int j = 0; j < 5 && first + j < count; j++) {
        int k = first + j, y = 153 + j * 15;
        if (k == ui.selection)
            rect(129, y - 3, 318, 14, 0x3c4034);
        const char *option;
        if (jailed) option = prison[k];
        else if (ui.dialogPage == DIALOG_WORK) {
            static char posting[88];
            int turnIn = work_turn_in(n, k), offset = 0;
            while (work_turn_in(n, offset) >= 0)
                offset++;
            int kind = n == work_contact(town) && k >= offset ? contract_offer(town, k - offset) : -1;
            if (turnIn >= 0)
                snprintf(posting, sizeof(posting), "Turn in %s", contract_title(g.contracts[turnIn].type));
            else if (kind >= 0)
                snprintf(posting, sizeof(posting), "%d silver / %s", contract_reward(town, kind), work[kind]);
            else
                snprintf(posting, sizeof(posting), "Back");
            option = posting;
        } else if (ui.dialogPage == DIALOG_INSPECTION) option = inspection[k];
        else if (ui.dialogPage == DIALOG_SUMMONS) option = summons[k];
        else if (ui.dialogPage == DIALOG_BOUNTY) option = review[k];
        else if (ui.dialogPage == DIALOG_DISMISS) option = "Leave";
        else if (ui.dialogPage == DIALOG_WORK_HINT) option = "Back";
        else option = dialogue_option(n, k);
        dialogue_row_text(y, option, k == ui.selection);
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
        snprintf(v, 100, "DAY %d %02d:%02d  %d MEMBERS  %d SILVER", z->day, z->minute / 60, z->minute % 60, z->squad, z->money);
        row(89 + i * 18, ui.selection == i, b, z->valid ? v : "");
    }
    text(31, 221, "AUTOSAVE IS SEPARATE FROM YOUR MANUAL SLOTS.", TEAL, 1);
}
static void actions_screen(void) {
    nearby_actions();
    NearbyTabs tabs;
    nearby_action_tabs(&tabs);
    int first = start_row(tabs.activeRow, tabs.rowCount, 6);
    int rows = tabs.rowCount < 6 ? tabs.rowCount : 6, top = 234 - (66 + 14 * rows);
    char b[120];
    panel(14, top, 348, 66 + 14 * rows);
    text(25, top + 8, "NEARBY ACTIONS", GOLD, 1);
    if (tabs.tabCount) {
        int actor = tabs.actors[tabs.activeTab];
        const char *state = actor >= 0 && g.actors[actor].dead ? "DEAD   " :
            actor >= 0 && g.actors[actor].unconscious > 0 ? "DOWNED   " : "";
        snprintf(b, sizeof(b), "%s%d/%d", state, tabs.activeTab + 1, tabs.tabCount);
        text(351 - text_width(b), top + 8, b, MUTED, 1);
    }
    int firstTab = start_row(tabs.activeTab, tabs.tabCount, 3);
    for (int j = 0; j < 3 && firstTab + j < tabs.tabCount; j++) {
        int tab = firstTab + j, x = 23 + 111 * j, actor = tabs.actors[tab];
        int active = tab == tabs.activeTab;
        rect(x, top + 20, 108, 14, active ? 0x58472e : INK);
        frame(x, top + 20, 108, 14, active ? GOLD : EDGE);
        clipped(x + 4, top + 24, actor < 0 ? "ENVIRONMENT" : actor == g.selected ? "SELF" :
            g.actors[actor].name, 16, active ? TEXT : MUTED);
    }
    if (!tabs.rowCount)
        text(25, top + 26, "Nothing within reach.", MUTED, 1);
    for (int j = 0; j < rows; j++) {
        int k = tabs.rows[first + j], y = top + 40 + j * 14;
        const char *why = nearby_action_reason(k);
        if (k == ui.selection)
            rect(21, y - 2, 334, 12, why ? 0x343331 : 0x4a4030);
        nearby_action_label(k, b, sizeof(b));
        clipped(25, y, b, 54, why ? 0x827d71 : k == ui.selection ? TEXT : 0xc0b5a2);
    }
    const char *why = tabs.rowCount ? nearby_action_reason(tabs.rows[tabs.activeRow]) : NULL;
    if (why)
        clipped(25, 210, why, 54, GOLD);
    snprintf(b, sizeof(b), "\014 TAB  \001 %s  \006 %s  \003 CLEAR (%d)  \002 BACK",
             ui.paused ? "QUEUE" : "DO", ui.paused ? "RESUME" : "PAUSE", g.society.queueCount[g.selected]);
    text(25, 222, b, MUTED, 1);
}
static void execute_screen(void) {
    overlay_header("EXECUTE OPPONENT", "\001 CONFIRM   \002 BACK");
    if (ui.nearbyTarget >= 0 && ui.nearbyTarget < g.actorCount) {
        char name[48];
        snprintf(name, sizeof(name), "Execute %s?", g.actors[ui.nearbyTarget].name);
        text(31, 102, name, RED, 1);
    }
}
static void engage_screen(void){
    overlay_header("CHOOSE OPPONENT","\015 SELECT   \001 ATTACK   \002 CANCEL");char v[40];int count=combat_candidates(),first=start_row(ui.selection,count,7);
    for(int j=0;j<7&&first+j<count;j++){int n=combat_candidate(first+j),rel=actor_relation(n);snprintf(v,sizeof(v),"%s  %.1fm",rel<0?"HOSTILE":rel>0?"FRIENDLY":"NEUTRAL",depth_distance(&g.actors[g.selected],&g.actors[n]));row(85+j*16,first+j==ui.selection,g.actors[n].name,v);}
    if(!count)text(31,96,"Nobody within 14m.",MUTED,1);
    if (ui.attackFocus >= 0 && warrant_authorizes(ui.attackFocus))
        text(31,211,"WANTED DEAD OR ALIVE",GOLD,1);
    else if(ui.attackFocus>=0&&actor_relation(ui.attackFocus)>=0)text(31,211,"ASSAULT: +100 BOUNTY / -20 FACTION STANDING",RED,1);
    else text(31,211,"Close to weapon reach to strike. Square stops attacking.",MUTED,1);
}
static void drop_screen(void) {
    overlay_header("DROP ITEM", "\001 DROP   \002 BACK");
    const Stack *stack = &g.actors[g.selected].pack[ui.dropSlot];
    char qty[24];
    snprintf(qty, sizeof(qty), "%d", stack->qty);
    row(90, 0, itemDefs[stack->item].name, qty);
    row(125, ui.selection == 0, "Drop one", "1");
    row(148, ui.selection == 1, "Drop stack", qty);
}
static void coach_screen(void) {
    overlay_header("STAGECOACH", "\001 PAY FARE   \015 DESTINATION   \002 BACK");
    int count = coach_count(ui.coachTown), first = start_row(ui.selection, count, 8);
    if (!count)
        text(31, 91, "Visit another town to open a route.", MUTED, 1);
    for (int i = first; i < count && i < first + 8; i++) {
        int town = coach_destination(ui.coachTown, i);
        char fare[50];
        snprintf(fare, sizeof(fare), "%.1f H / %d SILVER",
                 coach_duration(ui.coachTown, town) * 24 / DAY_SECONDS, coach_price(ui.coachTown, town));
        row(88 + (i - first) * 16, ui.selection == i, towns[town].name, fare);
    }
}
static void contract_note_screen(void) {
    const Contract *c = journal_contract(ui.contractNote);
    overlay_header("CONTRACT NOTE", "\015 SCROLL   \005/\006 PAGE   \002 BACK");
    if (!c) return;
    char body[600], details[100];
    snprintf(details, sizeof(details), "%s / %s", contract_title(c->type), towns[c->town].name);
    text(30, 86, details, GOLD, 1);
    contract_description(ui.contractNote, body, sizeof(body));
    ui.noteMaxScroll = wrap(0, 0, 43, body, 0) - 72;
    if (ui.noteMaxScroll < 0) ui.noteMaxScroll = 0;
    int top = clipTop, bottom = clipBottom;
    clipTop = 104;
    clipBottom = 176;
    wrap(31, 104-ui.scroll, 43, body, TEXT);
    clipTop = top;
    clipBottom = bottom;
    if (ui.noteMaxScroll) {
        rect(303,104,2,72,EDGE);
        rect(303,104+ui.scroll*62/ui.noteMaxScroll,2,10,GOLD);
    }
    snprintf(details, sizeof(details), "%s  %d SILVER", c->completed ? "PAID" : "REWARD", c->reward);
    text(31, 196, details, TEAL, 1);
}
static void journal_screen(void) {
    overlay_header(ui.journalTab ? "JOURNAL / COMPLETED" : "JOURNAL / ACTIVE",
                   ui.journalTab ? "\001 READ NOTE  \016 TAB  \004 WARRANTS" : "\001 READ NOTE  \016 TAB  \003 LOCATE  \004 WARRANTS");
    char b[214], v[42];
    int count = journal_count(), first = start_row(ui.selection, count, 6);
    for (int j = 0; j < 6 && first + j < count; j++) {
        int k = journal_slot(first + j);
        const Contract *c = journal_contract(k);
        snprintf(b, sizeof(b), "%.23s / %.14s", contract_title(c->type), towns[c->town].name);
        if (c->active == 3) snprintf(v, sizeof(v), "FAILED");
        else if (c->completed) snprintf(v, sizeof(v), "PAID");
        else if (contract_ready(k)) snprintf(v, sizeof(v), "READY TO TURN IN");
        else if (c->type == CONTRACT_PATROL)
            snprintf(v, sizeof(v), "%d/180S", (int)fminf(180, g.society.jobNear[k]));
        else if (c->type == CONTRACT_ESCORT)
            snprintf(v, sizeof(v), "%s", !g.society.jobStage[k] ? "RENDEZVOUS" :
                     g.society.jobWarned[k] ? "WAITING" : "EN ROUTE");
        else snprintf(v, sizeof(v), "%d/%d", c->progress, c->needed);
        row(85 + j * 14, first + j == ui.selection, b, v);
    }
    int selected = journal_slot(ui.selection);
    const Contract *entry = journal_contract(selected);
    contract_description(selected, b, sizeof(b));
    int bottom = clipBottom;
    clipBottom = 202;
    wrap(31, 171, 68, b, TEXT);
    clipBottom = bottom;
    if (entry && !ui.journalTab) {
        snprintf(b, sizeof(b), "REWARD %d SILVER", entry->reward);
        text(31, 208, b, GOLD, 1);
    }
    snprintf(b, sizeof(b), "CONTRACTS COMPLETED %d", g.contractsDone);
    text(31, 222, b, TEAL, 1);
}
static void chest_screen(void) {
    char buf[105], qty[16];
    int owner = chest_owner();
    const char *name = ui.chestKind == CHEST_PROPERTY ? "CONFISCATED BELONGINGS" :
        ui.chestKind == CHEST_FIELD ? "CHEST CONTENTS" :
        ruined_building(ui.storageIndex) ? "SALVAGE CHEST" :
        g.buildings[ui.storageIndex].type == SHOP ? "SHOP STOCK CHEST" : "HOUSEHOLD CHEST";
    overlay_header(name, "\001 TAKE ONE   \015 SELECT ITEM   \002 LEAVE");
    if (owner) {
        snprintf(buf, sizeof(buf), "OWNED BY %s. TAKING ITEMS IS THEFT.", factionNames[owner]);
        clipped(31, 85, buf, 68, GOLD);
    }
    int count = interior_chest_count();
    if (!count) {
        text(31, 114, "The chest is empty.", MUTED, 1);
        return;
    }
    int first = start_row(ui.selection, count, 6);
    text(369,98,"QTY",MUTED,1);
    text(405,98,"VAL/EA",MUTED,1);
    for (int i = 0; i < 6 && first + i < count; i++) {
        const Stack *item = interior_chest_item(first + i);
        int y = 110 + i * 16;
        row(y, ui.selection == first + i, itemDefs[item->item].name, NULL);
        snprintf(qty, sizeof(qty), "%d", item->qty); text(387-text_width(qty),y,qty,GOLD,1);
        snprintf(qty, sizeof(qty), "%d", itemDefs[item->item].price); text(442-text_width(qty),y,qty,TEXT,1);
    }
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
        int outsider=f==HOSTILE||f==WILDLIFE||g.rep[f]<-25;
        for(int t=0;t<TOWNS;t++)if((t==7||t==11||t==14||t==OLD_TOWNS+1)&&g.townOwner[t]==towns[t].faction&&g.townOwner[t]==f)outsider=1;
        if(outsider)text(32,80+f*12,factionNames[f],RED,1);
        rect(19,80+f*12,7,7,factionColors[f]);
    }
    const Actor *a = &g.actors[g.selected];
    if (wanted_amount(a, ui.selection)) {
        char charges[144];
        crime_description(a, ui.selection, charges, sizeof(charges));
        snprintf(b, sizeof(b), "%s / WANTED FOR", a->name);
        text(31, 181, b, RED, 1);
        wrap(31, 194, 68, charges, TEXT);
        return;
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
    int home = home_state(ui.storageIndex) == HOME_OWNED;
    const char *title = home ? (ui.storageSide ? "STASH / WITHDRAW" : "STASH / DEPOSIT") :
                               (ui.storageSide ? "DEPOT / WITHDRAW" : "DEPOT / DEPOSIT");
    overlay_header(title, "\014 SIDE   \001 MOVE ONE   \003 MOVE STACK");
    int count = item_view_count(), first = start_row(item_view_row(ui.selection), count, 9);
    char b[30];
    if(!count)text(31,100,"NO ITEMS ON THIS SIDE",MUTED,1);
    for (int j = 0; j < 9 && first + j < count; j++) {
        int k = item_view_slot(first+j);
        const Stack *st = ui.storageSide ? storage_stack(ui.storageIndex,k) : &a->pack[k];
        int y = 86 + j * 15, stolen = ui.storageSide ? storage_owner(ui.storageIndex,k) : a->stolen[k];
        snprintf(b, sizeof(b), "%d", st->qty);
        row(y, k == ui.selection, stolen ? "" : itemDefs[st->item].name, b);
        if (stolen) {
            text(32,y,"!",RED,1);
            clipped(44,y,itemDefs[st->item].name,55,RED);
        }
    }
    if(depot_public(ui.storageIndex)){
        int fee=depot_fee(ui.storageIndex,ui.storageSide,ui.selection);
        const Stack*stack=ui.storageSide?storage_stack(ui.storageIndex,ui.selection):&a->pack[ui.selection];
        char terms[100];snprintf(terms,sizeof(terms),"FEE %d / ITEM   STACK %d   PURSE %d",fee,fee*(stack?stack->qty:0),g.money);
        text(31,224,terms,g.money>=fee?GOLD:RED,1);
    }
}
static void placement_draw(void) {
    int indoors=ui.placeBuilding>=0,x,y;
    float w=.65f,d=.65f;
    if(indoors)room_project(ui.placeX,ui.placeY,&x,&y);else project(ui.placeX,ui.placeY,&x,&y);
    if(indoors && ui.placeType){
        const HomeFurniture*f=&homeFurniture[ui.placeType];
        const FurnitureModel*m=&furnitureModels[furniture_art_index(f->art)];
        float span=fminf((f->w+f->d)*(f->action==ROOM_REST?.86f:1),m->span/(2*ROOM_MAX_SCALE));
        float share=furniture_is_bed(f->art)&&f->d>f->w?1-m->xShare:m->xShare;
        InteriorProp p={ui.placeX,ui.placeY,span*share,span*(1-share),f->art,f->action};
        w=p.w;d=p.d;room_furniture(&p);
    } else if(!indoors)art(ui.placeType==1?ART_BEDROLL:27,x,y,1.08f,0);
    int valid=indoors&&!ui.placeType?1:placement_valid(ui.placeType,ui.placeBuilding,ui.placeX,ui.placeY);
    uint32_t color=valid?TEAL:RED;int px[4],py[4];
    const float corners[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    for(int i=0;i<4;i++){
        float xx=ui.placeX+corners[i][0]*w,yy=ui.placeY+corners[i][1]*d;
        if(indoors)room_project(xx,yy,&px[i],&py[i]);else project(xx,yy,&px[i],&py[i]);
    }
    for(int i=0;i<4;i++)line(px[i],py[i],px[(i+1)%4],py[(i+1)%4],color);
    line(x-4,y,x+4,y,color);line(x,y-3,x,y+3,color);
    rect(0,0,480,34,INK);
    rect(0,237,480,35,INK);
    const char*name=indoors?(ui.placeType?homeFurniture[ui.placeType].name:"Move / remove furniture"):ui.placeType?"Bedroll":"Campfire";
    text(18,11,name,GOLD,1);
    text(18,249,indoors&&!ui.placeType?"\011 MOVE   \001 PICK UP   \003 REMOVE   \002 BACK":"\011 MOVE   \001 PLACE   \002 BACK",TEXT,1);
    if(!valid){rect(158,215,164,15,INK);center(240,220,"SPACE OBSTRUCTED",RED,1);}
}
static void home_screen(void) {
    int id = ui.homeSite, town = building_town(id);
    char buf[72];
    if (home_state(id) == HOME_FOR_SALE) {
        overlay_header("HOUSE FOR SALE", "\001 BUY DEED");
        const ArtSprite *s = scene_sprite(building_sprite(id));
        scaled_art(building_sprite(id),108-s->w/2,95,s->w,s->h,1.05f,0,0);
        text(190,102,towns[town].name,GOLD,1);
        snprintf(buf,sizeof(buf),"DEED     %d SILVER",home_price(id));
        text(190,128,buf,g.money >= home_price(id) ? TEXT : RED,1);
        snprintf(buf,sizeof(buf),"PURSE    %d SILVER",g.money);
        text(190,149,buf,MUTED,1);
        text(190,179,"VACANT",TEAL,1);
        return;
    }
    overlay_header("PLACE FURNITURE", "\015 CHOOSE   \001 PLACE");
    for(int f=1;f<=FURN_TYPES;f++){
        char price[24]="";
        if(f<FURN_TYPES)snprintf(price,sizeof(price),"%d SILVER",homeFurniture[f].price);
        row(90+(f-1)*20,f==ui.selection,f==FURN_TYPES?"Move / remove furniture":homeFurniture[f].name,price);
    }
    snprintf(buf,sizeof(buf),"PURSE %d SILVER",g.money);text(31,220,buf,GOLD,1);
}

static void warrants_screen(void) {
    int ids[WARRANT_ROWS];
    int count = ui.warrantClosed ? warrant_receipt_count() : warrant_list(ui.warrantTown,ids);
    const char *title = ui.warrantClosed ? "WARRANTS / PAID" :
                        ui.warrantTown >= 0 ? "WANTED POSTERS" : "TRACKED WARRANTS";
    overlay_header(title, ui.warrantClosed ? "\016 OPEN WARRANTS   \015 SELECT" :
                                            "\016 PAID   \001 TRACK / DROP   \003 PIN LEAD");
    if (!count) {
        text(31,105,ui.warrantClosed ? "No bounties collected." :
             ui.warrantTown < 0 ? "Track a warrant at a jail." : "The watch has no open warrants here.",MUTED,1);
        return;
    }
    int selected = ui.selection < count ? ui.selection : count - 1;
    int first = start_row(selected,count,5);
    char label[84], detail[120];
    for (int rowIndex = first; rowIndex < count && rowIndex < first + 5; rowIndex++) {
        if (ui.warrantClosed) {
            const WarrantReceipt *r = warrant_receipt(rowIndex);
            snprintf(detail,sizeof(detail),"%d SILVER / %s",r->silver,r->dead ? "DEAD" : "ALIVE");
            row(86 + (rowIndex-first)*14,rowIndex==selected,r->name,detail);
        } else {
            WarrantView w;
            if (!warrant_view(ids[rowIndex],&w)) continue;
            snprintf(label,sizeof(label),"%s%s%s",w.tracked ? "* " : "",w.name,w.elite ? "  /  ELITE" : "");
            snprintf(detail,sizeof(detail),"%d ALIVE / %d DEAD",2*w.reward,w.reward);
            row(86 + (rowIndex-first)*14,rowIndex==selected,label,detail);
        }
    }
    line(27,157,451,157,EDGE);
    if (ui.warrantClosed) {
        const WarrantReceipt *r = warrant_receipt(selected);
        snprintf(detail,sizeof(detail),"%s paid %d silver on day %d.",factionNames[r->faction],r->silver,r->day);
        text(31,176,detail,TEXT,1);
        return;
    }
    WarrantView w;
    if (!warrant_view(ids[selected],&w)) return;
    clipped(31,166,w.offense,67,TEXT);
    int t = town_near(w.x,w.y,WORLD*2);
    int hours = (int)fmaxf(0,(g.time-w.seen)*24/DAY_SECONDS);
    snprintf(detail,sizeof(detail),"%s NEAR %s  /  %dh AGO",
             w.down==2 ? "BODY SEEN" : w.down ? "DOWNED" : "LAST REPORT",towns[t].name,hours);
    clipped(31,180,detail,67,GOLD);
    int used = snprintf(detail,sizeof(detail),"PAYS: ");
    for (int f = 0; f < FACTIONS && used < (int)sizeof(detail); f++)
        if (w.payers & (1u<<f))
            used += snprintf(detail+used,sizeof(detail)-used,"%s%s",used>6?", ":"",factionNames[f]);
    wrap(31,194,67,detail,TEXT);
    if (w.town >= 0) {
        snprintf(detail,sizeof(detail),"JAIL NEAR LEAD: %s",towns[w.town].name);
        clipped(31,220,detail,67,TEAL);
    }
}
static void loot_screen(void) {
    Actor *a=&g.actors[ui.lootActor];int count=item_view_count();char b[100];
    int theft=!a->squad&&a->faction>FREE&&a->faction!=HOSTILE&&a->faction!=WILDLIFE;
    overlay_header("LOOT",theft?"\001 STEAL ONE   \003 STEAL STACK   \015 CHOOSE":"\001 TAKE ONE   \003 TAKE STACK   \015 CHOOSE");
    text(368,82,"QTY",MUTED,1);
    text(405,82,"VAL/EA",MUTED,1);
    int first=start_row(item_view_row(ui.selection),count,6);
    for(int j=0;j<6&&first+j<count;j++) {
        int k=item_view_slot(first+j),id=limb_loot_item(a,k),y=94+j*12;
        if(k==ui.selection)rect(25,y-3,425,13,0x4a4030);
        int stolen = theft || (k < PACK ? a->stolen[k] : k < PACK + GEARS ? a->gearStolen[k-PACK] :
                               k == LOOT_LAYER_SLOT ? g.depth.actors[ui.lootActor].layerOwner : limb_owner(a, LARM + k - PACK - GEARS));
        if (stolen) text(31,y,"!",RED,1);
        snprintf(b,sizeof(b),"%s%.48s",k==LOOT_LAYER_SLOT?"UNDERLAYER: ":k>=PACK+GEARS?"FITTED: ":k>=PACK?"EQUIPPED: ":"",itemDefs[id].name);
        clipped(42,y,b,52,stolen?RED:k==ui.selection?TEXT:MUTED);
        snprintf(b,sizeof(b),"%d",k<PACK?a->pack[k].qty:1);text(386-text_width(b),y,b,GOLD,1);
        snprintf(b,sizeof(b),"%d",itemDefs[id].price);text(442-text_width(b),y,b,TEXT,1);
    }
    int it=!count?0:limb_loot_item(a,ui.selection);
    snprintf(b,sizeof(b),"%d ITEMS / %.30s",count,a->name);
    text(31,167,b,MUTED,1);
    equipment_comparison(it,179);
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
    const char *opts[] = {"First aid", "Carry unconscious survivor",
                          "Recover pack and worn equipment", "Take control of this squad member"};
    for (int i = 0; i < 4; i++) {
        if (i == ui.selection)
            rect(146, 126 + i * 21, 300, 18, 0x4a4030);
        text(152, 131 + i * 21, opts[i], i == ui.selection ? GOLD : MUTED, 1);
    }
    text(31, 223, "\001 NEARBY ACTIONS TO PUT THEM DOWN OR LAY THEM IN BED.", TEXT, 1);
}
static void log_screen(void) {
    overlay_header("RECENT MESSAGES", "NEWEST FIRST - CURRENT SESSION");
    for (int i = 0; i < 6 && i + ui.scroll < LOGS && i + ui.scroll < ui.logCount; i++)
        wrap(31, 85 + i * 24, 68, ui.logs[i + ui.scroll], i ? MUTED : TEXT);
    text(31, 235, "\015 SCROLL  \002 BACK", MUTED, 1);
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
    if(ui.screen==SCREEN_PLACE && !g.interiorBuilding){ui.camX=(a->x+ui.placeX)*.5f;ui.camY=(a->y+ui.placeY)*.5f;}
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
    labelCount = 0;
    if (ui.screen == SCREEN_WORLD) {
        if (!g.interiorBuilding) reserve_label((LabelBox){397,36,80,68});
        if (ui.statusPanel) reserve_label((LabelBox){8,44,201,168});
        if (ui.paused) reserve_label((LabelBox){192,43,97,21});
        care_and_cooldown(!!g.interiorBuilding);
        nearby_labels(!!g.interiorBuilding);
        const Actor *a = &g.actors[g.selected];
        if (!g.interiorBuilding) {
            if (a->target < 0 && a->unconscious <= 0 && !a->dead)
                speech_bubbles();
            structure_labels();
        }
    }
    if(!g.interiorBuilding){
      for(int k=0;k<6;k++)if(g.trails.bed[k]){Building*b=&g.buildings[g.trails.bed[k]-1];if(distance(b->x,b->y,ui.camX,ui.camY)<13){int x,y;project(b->x,b->y,&x,&y);if(ui.screen==SCREEN_WORLD)world_label(x,y-25,"Wayfarer rest",TEAL,-1);}}
      for(int t=0;t<TOWNS;t++)if(g.life.towns[t].stage==2&&distance(towns[t].x,towns[t].y,ui.camX,ui.camY)<18){int x,y;project(towns[t].x+5,towns[t].y+5,&x,&y);line(x,y,x,y-35,TEXT);triangle(x,y-35,x+16,y-30,x,y-23,factionColors[g.townOwner[t]]);if(ui.screen==SCREEN_WORLD)world_label(x,y+3,g.life.towns[t].policy==1?"Relief post":g.life.towns[t].policy==2?"Garrison":"Trade court",GOLD,-1);}
      for(int t=OLD_TOWNS;t<TOWNS;t++)if(distance(towns[t].x,towns[t].y,ui.camX,ui.camY)<18)civic_flag(t);
      int seenYards=0;
      for(int n=0;n<g.actorCount;n++)if(g.war.prisonTown[n]&&!g.actors[n].dead){
          int t=g.war.prisonTown[n]-1;
          if(seenYards&(1<<t))continue;
          seenYards|=1<<t;
          float wx,wy;war_yard(t,&wx,&wy);
          if(distance(wx,wy,ui.camX,ui.camY)<10){int x,y;project(wx,wy,&x,&y);if(ui.screen==SCREEN_WORLD)world_label(x,y+9,"Work yard",GOLD,-1);}
      }
    }
    hud();
    switch (ui.screen) {
    case SCREEN_ORDERS:case SCREEN_CAMP:case SCREEN_CARE:case SCREEN_LAYERS:
        depth_screen();break;
    case SCREEN_SCENARIOS:
        scenarios_screen();
        break;
    case SCREEN_CREATE:
        creator_screen();
        break;
    case SCREEN_NAME:
        name_screen();
        break;
    case SCREEN_ENGAGE: engage_screen();break;
    case SCREEN_LIFE:case SCREEN_POLITICS:life_screen();break;
    case SCREEN_ACTIONS: actions_screen();break;
    case SCREEN_PLACE: placement_draw();break;
    case SCREEN_INSPECT_HEALTH: health_screen(ui.nearbyTarget, 1);break;
    case SCREEN_EXECUTE: execute_screen();break;
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
    case SCREEN_DROP: drop_screen(); break;
    case SCREEN_STAGECOACH: coach_screen(); break;
    case SCREEN_CONTRACT_NOTE: contract_note_screen(); break;
    case SCREEN_JOURNAL:
        journal_screen();
        break;
    case SCREEN_WARRANTS:
        warrants_screen();
        break;
    case SCREEN_HOME:
        home_screen();
        break;
    case SCREEN_FACTIONS:
        factions_screen();
        break;
    case SCREEN_CHEST:
        chest_screen();
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
        health_screen(g.selected, 0);
        break;
    case SCREEN_SQUAD:
        squad_screen();
        break;
    case SCREEN_PASS:
        pass_screen();
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
    if (ui.screen != SCREEN_CREATE && ui.screen != SCREEN_NAME && ui.screen != SCREEN_SCENARIOS) {
        int priority, offset;
        const char *entry = log_current(&priority, &offset);
        if (entry) {
            int y = 23;
            rect(0, y, 480, 13, INK);
            int left = clipLeft, right = clipRight;
            clipLeft = 7;
            clipRight = 476;
            int x = 9 - offset;
            text(x, y + 3, entry, priority >= 2 ? GOLD : priority ? TEAL : TEXT, 1);
            if (offset && ui.feedCount > 1) {
                int next = x + text_width(entry);
                rect(next + 13, y + 5, 3, 3, EDGE);
                text(next + 30, y + 3, ui.feed[1], ui.feedPriority[1] >= 2 ? GOLD : ui.feedPriority[1] ? TEAL : TEXT, 1);
            }
            clipLeft = left;
            clipRight = right;
            rect(0, y, 3, 13, priority >= 2 ? RED : TEAL);
        }
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
