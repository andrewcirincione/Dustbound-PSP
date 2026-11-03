#include "game.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared, allocation-free software rasterizer. PSP writes to uncached VRAM.
   No external asset files are required at runtime. */
static uint32_t *fb;
static int pitch;
static const uint32_t INK = 0x171e21, PANEL = 0x252b2c, EDGE = 0x6e715e, GOLD = 0xd8ba78, TEXT = 0xe4dbc0,
                      MUTED = 0xa5aa99, TEAL = 0x8abeb1, RED = 0xcb7560;
static uint32_t abgr(uint32_t c) {
    return 0xff000000 | ((c & 255) << 16) | (c & 0xff00) | ((c >> 16) & 255);
}
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
static unsigned hash(int x, int y) {
    unsigned v = x * 374761393u + y * 668265263u + g.seed;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}
static void px(int x, int y, uint32_t c) {
    if ((unsigned)x < 480 && (unsigned)y < 272)
        fb[y * pitch + x] = abgr(c);
}
static void rect(int x, int y, int w, int h, uint32_t c) {
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > 480)
        w = 480 - x;
    if (y + h > 272)
        h = 272 - y;
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
    if (y2 < 0 || y0 >= 272 || y2 == y0)
        return;
    int from = y0 < 0 ? 0 : y0, to = y2 > 271 ? 271 : y2;
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
static void text(int x, int y, const char *s, uint32_t c, int scale) {
    for (int j = 0; s[j]; j++) {
        int k = toupper((unsigned char)s[j]);
        if (k < 32 || k > 95)
            k = '?';
        for (int i = 0; i < 5; i++) {
            int v = font[k - 32][i];
            for (int yy = 0; yy < 7; yy++)
                if (v & (1 << yy))
                    rect(x + j * 6 * scale + i * scale, y + yy * scale, scale, scale, c);
        }
    }
}
static void center(int x, int y, const char *s, uint32_t c, int z) {
    text(x - (int)strlen(s) * 3 * z, y, s, c, z);
}
static void wrap(int x, int y, int cols, const char *s, uint32_t c) {
    char b[100];
    int n = 0;
    while (*s) {
        while (*s && n < cols) {
            b[n++] = *s++;
        }
        if (*s && n == cols) {
            int k = n;
            while (k > 0 && b[k - 1] != ' ')
                k--;
            if (k > 0) {
                s -= n - k;
                n = k;
            }
        }
        while (n && b[n - 1] == ' ')
            n--;
        b[n] = 0;
        text(x, y, b, c, 1);
        y += 11;
        n = 0;
        while (*s == ' ')
            s++;
    }
}
static void panel(int x, int y, int w, int h) {
    rect(x + 2, y + 3, w, h, 0x141a1b);
    rect(x, y, w, h, PANEL);
    frame(x, y, w, h, EDGE);
    line(x + 1, y + 1, x + w - 2, y + 1, 0x93927b);
}
static void bar(int x, int y, int w, int h, float value, uint32_t c) {
    rect(x, y, w, h, 0x111a1c);
    int v = (int)(fmaxf(0, fminf(100, value)) * w / 100);
    rect(x, y, v, h, c);
}
static void project(float x, float y, int *sx, int *sy) {
    *sx = 240 + (int)((x - y - ui.camX + ui.camY) * 22);
    *sy = 126 + (int)((x + y - ui.camX - ui.camY) * 11);
}
static uint32_t ground(int b) {
    static const uint32_t p[] = {0xa59672, 0xb38964, 0xbab69a, 0x7c8b7a, 0x777477};
    return p[b];
}
static float light(void) {
    return g.dayTime >= 7 && g.dayTime < 18    ? 1
           : g.dayTime >= 5 && g.dayTime < 7   ? .83f
           : g.dayTime >= 18 && g.dayTime < 20 ? .8f
                                               : .55f;
}
static int road(float x, float y) {
    for (int i = 0; i < 4; i++)
        for (int j = i + 1; j < 4; j++) {
            if (i == 0 && j == 3)
                continue;
            float ax = towns[i].x, ay = towns[i].y, bx = towns[j].x, by = towns[j].y;
            float dx = bx - ax, dy = by - ay, t = ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy);
            if (t > 0 && t < 1 && distance(x, y, ax + t * dx, ay + t * dy) < .8f)
                return 1;
        }
    return 0;
}
static void terrain(void) {
    float li = light();
    rect(0, 0, 480, 272, shade(ground(biome(ui.camX, ui.camY)), li));
    int cx = (int)ui.camX, cy = (int)ui.camY;
    for (int y = cy - 18; y <= cy + 18; y++)
        for (int x = cx - 18; x <= cx + 18; x++) {
            int sx, sy;
            project(x + .5f, y + .5f, &sx, &sy);
            if (sx < -24 || sx > 504 || sy < -12 || sy > 227)
                continue;
            unsigned h = hash(x, y);
            int t = town_near(x, y, 7);
            uint32_t c = ground(biome(x, y));
            c = shade(c, (.965f + (h % 9) * .008f) * li);
            if (x < 1 || y < 1 || x > 126 || y > 126)
                c = shade(0x5c6867, li);
            else if (t >= 0)
                c = shade(blend(c, 0xd0bb90, .18f), li);
            else if (road(x, y))
                c = shade(blend(c, 0xd4bd8e, .23f), li);
            diamond(sx, sy, 22, 11, c);
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
}
static void box(int x, int y, int w, int d, int h, uint32_t c) {
    quad(x - w, y, x, y + d, x, y + d - h, x - w, y - h, shade(c, .73f));
    quad(x, y + d, x + w, y, x + w, y - h, x, y + d - h, shade(c, .9f));
    diamond(x, y - h, w, d, shade(c, 1.08f));
    line(x - w, y - h, x, y + d - h, shade(c, 1.2f));
    line(x, y + d - h, x + w, y - h, shade(c, .93f));
}
static void banner(int x, int y, int f, int size) {
    uint32_t c = f == UNION ? 0x577e81 : f == SCAVENGERS ? 0xa86446 : 0x625673;
    line(x, y, x, y - size, 0x514c40);
    quad(x + 1, y - size, x + 12, y - size + 3, x + 12, y - size + 12, x + 1, y - size + 9, c);
    line(x + 4, y - size + 4, x + 9, y - size + 5, 0xd6bd8b);
}
static void building(int id) {
    Building *b = &g.buildings[id];
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -100 || x > 580 || y < -80 || y > 295)
        return;
    float li = light();
    uint32_t wall = shade(b->faction == HOSTILE ? 0x7b777a : 0xb8a482, li), dark = shade(0x766e5b, li);
    int type = b->type;
    if (type <= TOWER) {
        int w = type == TOWER ? 19 : 34, d = type == TOWER ? 10 : 17, h = type == TOWER ? 65 : 39;
        quad(x - w, y + 4, x + 1, y + 23, x + 58, y + 9, x + 24, y - 8,
             shade(ground(biome(b->x, b->y)), li * .72f));
        box(x, y, w, d, h, wall);
        box(x, y - h, w + 2, d + 1, 5, dark);
        diamond(x, y - h - 5, w - 4, d - 3, shade(0x8c8369, li));
        for (int j = 1; j < 4; j++) {
            int yy = y - h + j * h / 4;
            line(x - w + 2, yy, x - 1, yy + d - 2, shade(wall, .65f));
            line(x + 2, yy + d - 2, x + w - 2, yy, shade(wall, .78f));
        }
        quad(x + 8, y + d - 4, x + 19, y + d - 10, x + 19, y + d - 29, x + 8, y + d - 23, 0x333b38);
        quad(x + 10, y + d - 5, x + 16, y + d - 8, x + 16, y + d - 24, x + 10, y + d - 21,
             shade(0x655a42, li));
        line(x + 8, y + d - 23, x + 13, y + d - 29, shade(wall, .5f));
        quad(x - w + 9, y - h + 10, x - w + 17, y - h + 14, x - w + 17, y - h + 24, x - w + 9, y - h + 20,
             0x39423e);
        line(x - w + 13, y - h + 12, x - w + 13, y - h + 22, dark);
        if (type == TOWER) {
            for (int i = -2; i <= 2; i++) {
                box(x + i * 7, y - h + abs(i) * 2, 3, 2, 9, wall);
            }
            banner(x, y - h - 5, b->faction, 22);
        } else {
            box(x - 13, y - h - 8, 5, 3, 13, dark);
            for (int i = 0; i < 3; i++) {
                int p = (int)(g.time * 6 + i * 11) % 32;
                ellipse(x - 9 + p / 3, y - h - 22 - p, 3 + p / 7, 2 + p / 8, shade(0xb7ac95, .8f));
            }
        }
        if (type == SHOP || type == BAR) {
            uint32_t canvas = type == SHOP ? 0x668a83 : 0xb47953;
            quad(x + 5, y - 21, x + 31, y - 34, x + 46, y - 19, x + 16, y - 3, shade(canvas, li));
            for (int i = 0; i < 4; i++)
                line(x + 8 + i * 6, y - 20 - i * 3, x + 19 + i * 7, y - 5 - i * 4, shade(canvas, .65f));
            line(x + 17, y - 4, x + 17, y + 14, dark);
            line(x + 46, y - 19, x + 46, y - 1, dark);
            box(x + 25, y + 4, 7, 4, 6, 0x8e7150);
            text(x - 27, y - h - 17, type == SHOP ? "TRADING POST" : "THE WAYWARD", TEXT, 1);
        }
    } else if (type == WINDMILL) {
        box(x, y, 8, 4, 8, dark);
        line(x - 7, y, x, y - 55, dark);
        line(x + 7, y, x, y - 55, dark);
        for (int i = 0; i < 4; i++)
            line(x - 6 + i, y - i * 10, x + 6 - i, y - i * 10, dark);
        float an = g.time * .7f;
        for (int i = 0; i < 4; i++) {
            float a = an + i * 1.5708f;
            int ex = x + cosf(a) * 24, ey = y - 50 + sinf(a) * 24;
            line(x, y - 50, ex, ey, 0x5c665d);
            triangle(ex, ey, ex - (int)(sinf(a) * 8), ey + (int)(cosf(a) * 8), x + (int)(cosf(a) * 8),
                     y - 50 + (int)(sinf(a) * 8), shade(0xc6bea2, li));
        }
        ellipse(x, y - 50, 3, 3, 0x4e574f);
    } else if (type == WELL) {
        box(x, y, 12, 6, 6, dark);
        diamond(x, y - 6, 8, 4, 0x303f3e);
        line(x - 10, y - 2, x - 10, y - 20, dark);
        line(x + 10, y - 2, x + 10, y - 20, dark);
        line(x - 10, y - 20, x + 10, y - 20, dark);
        line(x, y - 20, x, y - 6, 0x5c5548);
    } else if (type == CAMPFIRE) {
        ellipse(x, y + 1, 12, 5, shade(0x72634d, li));
        line(x - 8, y + 1, x + 7, y - 2, 0x4a4338);
        line(x - 7, y - 4, x + 8, y + 2, 0x4a4338);
        int f = (int)(g.time * 9) % 4;
        triangle(x - 5, y, x + 5, y, x + f - 2, y - 14, 0xcf713c);
        triangle(x - 2, y, x + 3, y, x + 1, y - 9, 0xf5c76f);
        px(x + 4, y - 17 - f, 0xf7d68d);
    } else if (type == BED) {
        diamond(x, y, 24, 12, shade(0x655e4b, li));
        quad(x - 18, y - 1, x + 1, y - 11, x + 20, y, x, y + 9, 0x78867c);
        triangle(x - 26, y, x, y - 33, x + 26, y, shade(0xb9aa7e, li));
        triangle(x, y - 33, x + 26, y, x + 2, y + 9, shade(0x968b68, li));
        triangle(x, y - 24, x + 13, y + 2, x - 4, y + 5, 0x37413a);
        line(x, y - 34, x, y + 3, dark);
    } else if (type == FARM) {
        diamond(x, y, 33, 16, shade(0x71674e, li));
        for (int i = 0; i < 5; i++) {
            line(x - 25 + i * 11, y - 3 + i * 4, x - 10 + i * 11, y - 10 + i * 4, 0x514f3d);
            if (b->progress > 15) {
                int h = 3 + b->progress / 12;
                for (int j = 0; j < 3; j++) {
                    int xx = x - 23 + i * 11 + j * 4, yy = y - 3 + i * 4 - j * 2;
                    line(xx, yy, xx, yy - h, 0x60744e);
                    line(xx, yy - 2, xx - 3, yy - h + 2, b->progress > 80 ? 0xd4bd70 : 0x98a76c);
                }
            }
        }
    } else if (type == FORGE) {
        box(x, y, 15, 8, 12, shade(0x716d62, li));
        box(x - 3, y - 7, 8, 4, 20, 0x555e5b);
        rect(x + 4, y - 10, 7, 5, 0xe2a45c);
        box(x + 19, y + 6, 7, 4, 7, 0x657674);
    } else if (type == BENCH) {
        box(x, y, 18, 9, 13, 0x82785e);
        box(x - 5, y - 15, 8, 4, 4, 0xc0b88b);
        line(x + 10, y - 15, x + 10, y - 28, 0x65766c);
        ellipse(x + 10, y - 28, 3, 3, 0x8dc9b9);
    } else if (type == WALL) {
        for (int i = -3; i <= 3; i++) {
            line(x + i * 5, y + abs(i) * 2, x + i * 5, y + abs(i) * 2 - 30, dark);
            triangle(x + i * 5 - 2, y - 25 + abs(i) * 2, x + i * 5 + 2, y - 25 + abs(i) * 2, x + i * 5,
                     y - 34 + abs(i) * 2, 0x8b7a56);
        }
    } else if (type == STORE) {
        box(x, y, 17, 9, 15, 0x9c805b);
        for (int j = -2; j < 3; j++)
            line(x + j * 5, y - 14 + abs(j) * 2, x + j * 5, y + abs(j) * 2, 0x655b45);
    }
}
static void resource(int n) {
    Node *b = &g.nodes[n];
    if (!b->remaining)
        return;
    int x, y;
    project(b->x, b->y, &x, &y);
    if (x < -25 || x > 505 || y < -30 || y > 240)
        return;
    float li = light();
    if (b->type == NODE_ORE || b->type == NODE_COPPER) {
        uint32_t c = b->type == NODE_ORE ? 0x78817b : 0x9a6b50;
        ellipse(x, y + 2, 16, 5, shade(0x766e53, li));
        triangle(x - 14, y, x - 5, y - 17, x + 6, y + 2, shade(c, .75f * li));
        triangle(x - 5, y - 17, x + 13, y - 5, x + 6, y + 2, shade(c, 1.2f * li));
        triangle(x + 6, y + 2, x + 13, y - 5, x + 17, y + 2, shade(c, .9f * li));
        line(x - 5, y - 15, x + 1, y - 4, shade(c, 1.6f * li));
    } else if (b->type == NODE_WOOD) {
        line(x, y, x - 2, y - 24, 0x675d46);
        line(x - 2, y - 12, x - 12, y - 22, 0x675d46);
        line(x - 1, y - 18, x + 6, y - 28, 0x675d46);
        line(x - 10, y - 20, x - 13, y - 29, 0x675d46);
        line(x + 3, y - 23, x + 11, y - 25, 0x675d46);
    } else if (b->type == NODE_RUIN) {
        box(x, y, 17, 9, 8, 0x7e8078);
        box(x - 8, y - 5, 5, 3, 27, 0x868b80);
        box(x + 9, y - 2, 5, 3, 18, 0x6f7770);
        box(x, y - 8, 5, 3, 5, 0x68978c);
        px(x, y - 15, TEAL);
    } else {
        box(x, y, 11, 6, 9, b->owner ? 0x92794e : 0x8b7860);
        line(x - 8, y - 11, x + 2, y - 6, 0xbca275);
        line(x, y - 3, x, y + 4, 0x363f3c);
        if (b->locked) {
            rect(x - 2, y - 5, 4, 4, GOLD);
        }
    }
}
static void person(Actor *a, int x, int y, int sc, int chosen) {
    uint32_t skin = a->name[0] == 'S' ? 0xa98564 : 0xc4ae86;
    uint32_t body = itemDefs[a->gear[1]].color, head = itemDefs[a->gear[2]].color;
    float li = sc == 1 ? light() : 1;
    if (!body)
        body = 0x95876a;
    body = shade(body, li);
    skin = shade(skin, li);
    int down = a->dead || a->unconscious > 0;
    int pose = sin(a->anim * 2) * 2;
    int attack = a->cooldown > .65f;
    ellipse(x + 3 * sc, y + 2 * sc, 9 * sc, 3 * sc, shade(0x6c6956, li * .78f));
    if (a->kind == BEAST) {
        ellipse(x, y - 7, 12, 5, shade(0x8f8971, li));
        line(x - 8, y - 5, x - 11, y + 1, 0x514f42);
        line(x + 7, y - 5, x + 9, y + 2, 0x514f42);
        triangle(x + 8, y - 13, x + 17, y - 8, x + 8, y - 5, 0xaba184);
        line(x - 11, y - 8, x - 18, y - 14, 0x716951);
        px(x + 12, y - 10, 0xdfb875);
        return;
    }
    if (down) {
        rect(x - 10 * sc, y - 5 * sc, 15 * sc, 5 * sc, body);
        ellipse(x + 8 * sc, y - 4 * sc, 3 * sc, 3 * sc, skin);
        line(x - 11 * sc, y - 4 * sc, x - 16 * sc, y - 1 * sc, 0x484b40);
        if (a->bleed > 0)
            ellipse(x + 3 * sc, y + 3 * sc, 7 * sc, 2 * sc, 0x875444);
        return;
    }
    if (chosen && sc == 1) {
        line(x - 12, y + 1, x, y + 7, TEAL);
        line(x, y + 7, x + 12, y + 1, TEAL);
        line(x + 12, y + 1, x, y - 5, TEAL);
        line(x, y - 5, x - 12, y + 1, TEAL);
    }
    int crouch = a->sneaking ? 4 * sc : 0;
    y += crouch;
    for (int i = 0; i < 2; i++) {
        int limb = i ? RLEG : LLEG;
        if ((a->lostMask & (1 << limb)) && !(a->prostheticMask & (1 << limb)))
            continue;
        uint32_t c = (a->prostheticMask & (1 << limb)) ? 0x94aba5 : 0x4d5449;
        int off = (i ? pose : -pose) * sc;
        rect(x + (i ? 1 : -4) * sc, y - 10 * sc, 3 * sc, (8 + (i ? pose : -pose)) * sc, shade(c, li));
        rect(x + (i ? 1 : -5) * sc, y - 3 * sc + off, 5 * sc, 3 * sc,
             shade(itemDefs[a->gear[3]].color ? itemDefs[a->gear[3]].color : skin, li));
    }
    if (a->gear[1] == DUSTER) {
        quad(x - 5 * sc, y - 20 * sc, x + 5 * sc, y - 20 * sc, x + 7 * sc, y - 5 * sc, x - 7 * sc, y - 5 * sc,
             body);
        line(x, y - 16 * sc, x, y - 5 * sc, shade(body, .6f));
    } else {
        quad(x - 5 * sc, y - 21 * sc, x + 5 * sc, y - 21 * sc, x + 4 * sc, y - 10 * sc, x - 4 * sc,
             y - 10 * sc, body);
    }
    if (a->gear[1] == PLATE) {
        rect(x - 7 * sc, y - 22 * sc, 5 * sc, 4 * sc, shade(body, 1.3f));
        rect(x + 3 * sc, y - 22 * sc, 5 * sc, 4 * sc, shade(body, 1.08f));
        for (int i = 0; i < 3; i++)
            line(x - 3 * sc, y - (18 - i * 3) * sc, x + 3 * sc, y - (18 - i * 3) * sc, shade(body, 1.3f));
    } else {
        line(x - 3 * sc, y - 20 * sc, x + 3 * sc, y - 11 * sc, shade(body, 1.3f));
        rect(x - 5 * sc, y - 11 * sc, 10 * sc, 2 * sc, shade(0x5b4d3c, li));
    }
    for (int i = 0; i < 2; i++) {
        int limb = i ? RARM : LARM;
        if ((a->lostMask & (1 << limb)) && !(a->prostheticMask & (1 << limb)))
            continue;
        int xx = x + (i ? 6 : -7) * sc, yy = y - (attack && i ? 19 : 14) * sc;
        rect(xx, y - 20 * sc, 2 * sc, (attack && i ? 3 : 7) * sc,
             (a->prostheticMask & (1 << limb)) ? 0x9daaa6 : skin);
        rect(xx, yy, 3 * sc, 3 * sc, skin);
    }
    rect(x - 3 * sc, y - 27 * sc, 6 * sc, 6 * sc, skin);
    rect(x - 2 * sc, y - 22 * sc, 5 * sc, 2 * sc, shade(0x655d4b, li));
    px(x + sc, y - 25 * sc, INK);
    if (a->gear[2] == STRAW) {
        triangle(x - 8 * sc, y - 27 * sc, x + 9 * sc, y - 27 * sc, x, y - 32 * sc, shade(head, li));
        line(x - 8 * sc, y - 27 * sc, x + 9 * sc, y - 27 * sc, shade(head, .6f * li));
    } else if (a->gear[2] == HELM) {
        rect(x - 4 * sc, y - 29 * sc, 8 * sc, 5 * sc, shade(head, li));
        rect(x - 4 * sc, y - 25 * sc, 3 * sc, 5 * sc, shade(head, .8f * li));
        rect(x + 2 * sc, y - 25 * sc, 3 * sc, 5 * sc, shade(head, .8f * li));
        rect(x - 2 * sc, y - 25 * sc, 4 * sc, sc, INK);
        line(x, y - 30 * sc, x, y - 26 * sc, shade(head, 1.3f * li));
    } else if (a->gear[2] == HAT) {
        rect(x - 4 * sc, y - 29 * sc, 8 * sc, 5 * sc, shade(head, li));
        rect(x - 3 * sc, y - 23 * sc, 6 * sc, 3 * sc, 0x414c45);
    }
    int w = a->gear[0];
    if (w) {
        int xx = x + 8 * sc, yy = y - (attack ? 18 : 12) * sc;
        uint32_t c = shade(itemDefs[w].color, li);
        if (w == CROSSBOW) {
            line(xx - 3 * sc, yy + 3 * sc, xx + 9 * sc, yy - 8 * sc, c);
            line(xx - 3 * sc, yy - 9 * sc, xx + 10 * sc, yy + 2 * sc, c);
            line(xx - 3 * sc, yy - 9 * sc, xx + 3 * sc, yy - 1 * sc, 0xc4bb98);
            line(xx + 10 * sc, yy + 2 * sc, xx + 3 * sc, yy - 1 * sc, 0xc4bb98);
        } else if (w == STAFF) {
            line(xx - 3 * sc, yy + 8 * sc, xx + 8 * sc, yy - 20 * sc, c);
            line(xx - 2 * sc, yy + 8 * sc, xx + 9 * sc, yy - 20 * sc, c);
        } else {
            int ex = xx + (attack ? 17 : 8) * sc, ey = yy - (attack ? 2 : 17) * sc;
            line(xx, yy, ex, ey, c);
            line(xx + sc, yy, ex + sc, ey, shade(c, 1.3f));
            if (w == CLEAVER)
                line(xx + 2 * sc, yy, ex + 3 * sc, ey, c);
            line(xx - 2 * sc, yy - 2 * sc, xx + 4 * sc, yy, 0x4b5045);
        }
    }
    if (a->carrying >= 0 && sc == 1) {
        rect(x - 8, y - 23, 16, 5, 0x8a826a);
        ellipse(x + 9, y - 21, 3, 3, skin);
    }
}
typedef struct {
    float depth;
    int kind, index, x, y;
} Object;
static Object objects[1000];
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
            objects[n++] = (Object){x + y + .7f, 3, (int)(h % 4), sx, sy};
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
                bar(o->x - 10, o->y - 38, 21, 2, hp, a->squad ? TEAL : RED);
            }
            if (a->kind == TRADER || a->kind == RECRUIT) {
                center(o->x, o->y - 40, a->kind == TRADER ? "TRADE" : "RECRUIT",
                       a->kind == TRADER ? GOLD : TEAL, 1);
            }
            if (a->squad && o->index == g.selected) {
                triangle(o->x - 3, o->y - 46, o->x + 3, o->y - 46, o->x, o->y - 42, TEAL);
            }
        } else {
            int x = o->x, y = o->y;
            if (o->index < 2) {
                triangle(x - 6, y, x - 2, y - 6, x + 7, y, shade(0x8b8369, light()));
                line(x - 2, y - 6, x + 7, y, shade(0xb5a886, light()));
            } else {
                for (int j = 0; j < 4; j++)
                    line(x, y, x - 5 + j * 3, y - 4 - (j % 2) * 3, shade(0x757954, light()));
            }
        }
    }
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
    static const char *regions[] = {"THE AMBER EXPANSE", "COPPER BADLANDS", "THE SALT FLATS", "REED MARSH",
                                    "THE ASH WASTE"};
    text(9, 8, t >= 0 ? towns[t].name : regions[biome(a->x, a->y)], GOLD, 1);
    snprintf(b, 120, "DAY %d  %02d:%02d", g.day, (int)g.dayTime, (int)(g.dayTime * 60) % 60);
    text(220, 8, b, MUTED, 1);
    snprintf(b, 120, "%d SILVER", g.money);
    text(397 - strlen(b) * 2, 8, b, TEXT, 1);
    mini_map();
    if (ui.paused) {
        panel(192, 31, 94, 20);
        center(239, 38, "TACTICAL PAUSE", GOLD, 1);
    } else if (ui.speed == 2)
        text(338, 31, "2X", GOLD, 1);
    rect(0, 212, 480, 60, INK);
    line(0, 212, 480, 212, EDGE);
    int k = 0;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad) {
            Actor *m = &g.actors[i];
            int x = 8 + k * 72;
            rect(x, 219, 65, 33, i == g.selected ? 0x374640 : PANEL);
            frame(x, 219, 65, 33, i == g.selected ? TEAL : EDGE);
            person(m, x + 12, 248, 1, 0);
            text(x + 24, 224, m->name, i == g.selected ? TEAL : TEXT, 1);
            bar(x + 24, 237, 35, 3, (m->health[HEAD] + m->health[CHEST]) * .5f, m->dead ? RED : TEAL);
            text(x + 24, 244,
                 m->dead                ? "DEAD"
                 : m->unconscious > 0   ? "DOWN"
                 : m->job == JOB_FOLLOW ? "FOLLOW"
                                        : "JOB",
                 MUTED, 1);
            k++;
        }
    int hx = 307;
    snprintf(b, 120, "FOOD %d", (int)a->hunger);
    text(hx, 220, b, TEXT, 1);
    bar(hx + 65, 220, 47, 5, a->hunger, GOLD);
    snprintf(b, 120, "BLOOD %d", (int)a->blood);
    text(hx, 231, b, a->bleed > 0 ? RED : TEXT, 1);
    bar(hx + 65, 231, 47, 5, a->blood, a->bleed > 0 ? RED : TEAL);
    snprintf(b, 120, "LOAD %.0f / %d", encumbrance(a), 30 + level(a, STRENGTH) * 2);
    text(hx, 242, b, MUTED, 1);
    if (a->sneaking)
        text(430, 221, "SNEAK", TEAL, 1);
    else
        text(430, 221, a->autoattack ? "FIGHT" : "FLEE", GOLD, 1);
    text(8, 261, "X ACT  TRI PACK  SQ STANCE  O SNEAK  L SQUAD  R PAUSE", MUTED, 1);
    text(343, 261, "SELECT MAP  START MENU", MUTED, 1);
    if (ui.noticeTime > 0) {
        rect(7, 191, 466, 15, PANEL);
        text(13, 195, ui.notice, TEXT, 1);
    } else if (ui.contextIndex >= 0) {
        const char *s = "INTERACT";
        int id = ui.contextIndex;
        if (ui.contextKind == 0) {
            Actor *m = &g.actors[id];
            s = m->squad                        ? "FIRST AID / CARRY"
                : m->dead || m->unconscious > 0 ? "LOOT BODY"
                : m->kind == TRADER             ? "TRADE SUPPLIES"
                                                : "RECRUIT COMPANION";
        } else if (ui.contextKind == 1) {
            Node *m = &g.nodes[id];
            s = m->locked                ? "PICK LOCK"
                : m->type <= NODE_COPPER ? "MINE RESOURCE"
                : m->type == NODE_WOOD   ? "GATHER TIMBER"
                                         : "TAKE LOOT";
        } else {
            Building *m = &g.buildings[id];
            s = m->type == WELL                         ? "DRAW WATER"
                : m->type == BED || m->type == CAMPFIRE ? "REST"
                : m->type == FARM                       ? "WATER / HARVEST"
                : m->type == FORGE                      ? "CRAFT"
                : m->type == BENCH                      ? "RESEARCH"
                                                        : "STORAGE";
        }
        int w = strlen(s) * 6 + 28;
        panel(240 - w / 2, 186, w, 20);
        text(240 - w / 2 + 7, 193, "X", TEAL, 1);
        text(240 - w / 2 + 20, 193, s, TEXT, 1);
    }
    if (a->taskKind) {
        int w = 120;
        bar(180, 178, w, 4,
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
    const char *opts[] = {"WANDERER - ONE DRIFTER", "CARAVAN - THREE SURVIVORS", "CONTINUE - SAVE SLOT 1",
                          "FIELD GUIDE"};
    for (int i = 0; i < 4; i++) {
        int y = 148 + i * 23;
        if (i == ui.selection) {
            rect(25, y - 5, 255, 20, 0x374640);
            text(32, y, ">", TEAL, 1);
        }
        text(47, y, opts[i], i == ui.selection ? TEXT : MUTED, 1);
    }
    text(31, 251, "X BEGIN    VERSION " VERSION " / ALPHA", GOLD, 1);
}
static void overlay_header(const char *name, const char *sub) {
    panel(14, 28, 452, 224);
    text(27, 40, name, GOLD, 2);
    text(28, 61, sub, MUTED, 1);
    line(27, 75, 451, 75, EDGE);
    text(28, 237, "O CLOSE", MUTED, 1);
    if (ui.screen >= SCREEN_PACK && ui.screen <= SCREEN_SQUAD)
        text(249, 237, "L / R: PACK  SKILLS  BODY  SQUAD", MUTED, 1);
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
        rect(25, y - 3, 425, 14, 0x3c4a43);
        rect(25, y - 3, 2, 14, TEAL);
    }
    text(32, y, label, selected ? TEXT : MUTED, 1);
    if (value)
        text(442 - (int)strlen(value) * 6, y, value, selected ? GOLD : MUTED, 1);
}
static void pack_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[100];
    overlay_header("EQUIPMENT & PACK", "X EQUIP / USE   SQUARE PASS ONE ITEM TO NEARBY ALLY");
    line(197, 81, 197, 222, EDGE);
    person(a, 108, 162, 2, 0);
    text(31, 176, a->name, TEAL, 1);
    snprintf(b, 100, "LOAD %.0f / %d", encumbrance(a), 30 + level(a, STRENGTH) * 2);
    text(31, 188, b, MUTED, 1);
    snprintf(b, 100, "%s", itemDefs[a->gear[0]].name);
    text(31, 200, b, TEXT, 1);
    text(31, 212, itemDefs[a->gear[1]].name, MUTED, 1);
    int start = start_row(ui.selection, PACK, 9);
    for (int j = 0; j < 9; j++) {
        int i = start + j, y = 84 + j * 14, it = a->pack[i].item;
        if (i == ui.selection) {
            rect(205, y - 3, 244, 13, 0x3b4b43);
            rect(205, y - 3, 2, 13, TEAL);
        }
        snprintf(b, 100, "%02d %s", i + 1, it ? itemDefs[it].name : "--");
        text(213, y, b, i == ui.selection ? TEXT : MUTED, 1);
        if (it) {
            snprintf(b, 100, "%d", a->pack[i].qty);
            text(443 - strlen(b) * 6, y, b, GOLD, 1);
        }
    }
    int it = a->pack[ui.selection].item;
    if (it) {
        rect(23, 215, 433, 16, PANEL);
        text(28, 219, itemDefs[it].desc, TEXT, 1);
    }
    text(429, 60, "20", MUTED, 1);
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
    overlay_header("BODY & FIRST AID", "X USE A FIELD DRESSING. LOST LIMBS NEED PROSTHETICS.");
    person(a, 99, 196, 3, 0);
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
    char b[100];
    overlay_header("SQUAD ORDERS", "LEFT / RIGHT SELECT MEMBER. X ASSIGN A PERSISTENT JOB.");
    text(32, 88, a->name, TEAL, 2);
    for (int i = 0; i < 5; i++) {
        snprintf(b, 100, "%s", jobNames[i]);
        row(113 + i * 20, ui.selection == i, b, a->job == i ? "ASSIGNED" : "");
    }
    text(32, 222, "MINERS USE NEARBY ORE; FARMERS NEED WATER IN THEIR PACK.", MUTED, 1);
}
static void map_screen(void) {
    overlay_header("THE BORDERLANDS", "D-PAD MOVE CURSOR  /  X SET TRAVEL WAYPOINT");
    int ox = 28, oy = 82;
    for (int y = 0; y < WORLD; y++)
        for (int x = 0; x < WORLD; x++) {
            int k = y * WORLD + x;
            uint32_t c =
                g.seen[k >> 3] & (1 << (k & 7)) ? ground(biome(x, y)) : shade(ground(biome(x, y)), .35f);
            rect(ox + x * 2, oy + y, 2, 1, c);
        }
    for (int t = 0; t < TOWNS; t++)
        if (g.discovered[t]) {
            int x = ox + towns[t].x * 2, y = oy + towns[t].y;
            rect(x - 2, y - 2, 5, 5, t == 4 ? RED : GOLD);
        }
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && !g.actors[i].dead) {
            int x = ox + g.actors[i].x * 2, y = oy + g.actors[i].y;
            rect(x - 1, y - 1, 3, 3, TEAL);
        }
    if (g.baseExists) {
        int x = ox + g.baseX * 2, y = oy + g.baseY;
        frame(x - 3, y - 3, 7, 7, TEAL);
    }
    int x = ox + ui.mapX * 2, y = oy + ui.mapY;
    line(x - 5, y, x + 5, y, TEXT);
    line(x, y - 5, x, y + 5, TEXT);
    frame(ox - 1, oy - 1, 258, 130, EDGE);
    text(304, 85, "KNOWN SETTLEMENTS", GOLD, 1);
    for (int t = 0; t < TOWNS; t++)
        text(304, 103 + t * 18, g.discovered[t] ? towns[t].name : "UNDISCOVERED",
             g.discovered[t] ? TEXT : MUTED, 1);
    text(304, 200, "TEAL: YOUR SQUAD", TEAL, 1);
    char b[80];
    snprintf(b, 80, "CURSOR %03d / %03d", (int)ui.mapX, (int)ui.mapY);
    text(31, 220, b, MUTED, 1);
}
static void trade_screen(void) {
    char b[100], v[80];
    overlay_header(ui.sell ? "TRADING / SELL" : "TRADING / BUY",
                   "LEFT / RIGHT: BUY OR SELL  /  X TRADE ONE  /  O LEAVE");
    int count = ui.sell ? PACK : ITEMS - 1, start = start_row(ui.selection, count, 9);
    for (int j = 0; j < 9 && start + j < count; j++) {
        int i = start + j, it = ui.sell ? g.actors[g.selected].pack[i].item : i + 1;
        if (ui.sell)
            snprintf(v, 80, "%d SILVER  X%d", itemDefs[it].price * 6 / 10, g.actors[g.selected].pack[i].qty);
        else
            snprintf(v, 80, "%d SILVER  STOCK %d", itemDefs[it].price, g.stock[ui.tradeTown][it]);
        row(86 + j * 15, ui.selection == i, it ? itemDefs[it].name : "--", it ? v : "");
    }
    int it = ui.sell ? g.actors[g.selected].pack[ui.selection].item : ui.selection + 1;
    snprintf(b, 100, "PURSE: %d SILVER", g.money);
    text(31, 223, b, GOLD, 1);
    if (it)
        text(31, 70, "", TEXT, 1);
}
static void build_screen(void) {
    char b[100];
    overlay_header("BUILD AN OUTPOST", "X PLACE BESIDE SELECTED MEMBER. OUTSIDE VILLAGES ONLY.");
    static const int wood[] = {3, 5, 6, 8, 6, 3, 5, 8}, iron[] = {0, 0, 0, 3, 1, 0, 0, 3};
    for (int i = 0; i < buildCount; i++) {
        snprintf(b, 100, "%d TIMBER  %d IRON", wood[i], iron[i]);
        row(86 + i * 17, i == ui.selection, buildNames[i], b);
    }
    text(31, 225, "BUILDINGS ATTRACT RAIDS. KEEP SUPPLIES AND A RETREAT ROUTE.", MUTED, 1);
}
static void craft_screen(void) {
    overlay_header("WORKSHOP", "X CRAFT. STAND NEAR THE CORRECT CAMP WORKSTATION.");
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
    text(33, 214, "X RESEARCH THE NEXT TIER", TEXT, 1);
}
static void menu_screen(void) {
    char b[100];
    overlay_header("EXPEDITION", "TIME STOPS IN MENUS. LEFT / RIGHT CHANGES SAVE SLOT.");
    const char *options[] = {"Return to the wasteland",
                             "Build an outpost",
                             "Crafting recipes",
                             "Research",
                             "Save expedition",
                             "Load expedition",
                             "Simulation speed",
                             "Field guide",
                             "Start a new wanderer",
                             "Save slot 1 and title",
                             "Pay bounty / repair reputation"};
    for (int i = 0; i < 11; i++) {
        snprintf(b, 100, i == 4 || i == 5 ? "SLOT %d" : i == 6 ? "%dX" : "", i == 6 ? ui.speed : ui.slot);
        row(83 + i * 13, ui.selection == i, options[i], b);
    }
}
static void help_screen(void) {
    overlay_header("FIELD GUIDE", "UP / DOWN: PAGES    O CLOSE");
    const char *heads[] = {"1 / 3    SURVIVING THE FIRST DAY", "2 / 3    THE CONTROLS",
                           "3 / 3    BUILDING A LIFE"};
    text(31, 86, heads[ui.selection % 3], TEAL, 1);
    const char *pages[] = {
        "You begin weak. Recruit Ivo near the inn for 300 silver. The guarded town is a refuge. Loot the "
        "abandoned crate east of the well for a leather vest. Equip it from your pack.\nMine iron south of "
        "town and sell it to Mara. Buy food and field dressings before leaving. Injuries persist. A "
        "knocked-out ally can be bandaged or carried with X. Rest at a camp to recover.",
        "ANALOG / D-PAD: move in screen direction.\nX: interact with nearest object or person.\nTRIANGLE: "
        "pack, skills, body and squad tabs.\nSQUARE: switch fighting or passive stance.\nCIRCLE: sneak. L: "
        "select next squad member.\nR: tactical pause. SELECT: world map.\nSTART: build, craft, research, "
        "save, help.\nMenus pause time. Autosave uses slot 0.",
        "Gather timber and mine iron. Trade ore for iron bars to build your first forge. Build a camp "
        "outside village boundaries. Farms need water; harvest grain and cook it on a campfire.\nResearch "
        "unlocks better gear and prosthetics. Assign allies to follow, hold, mine, farm or heal. Your camp "
        "attracts raiders. Lockpick ruins for relics. Stealing owned crates can anger the guards."};
    const char *p = pages[ui.selection % 3];
    int y = 105;
    char b[256];
    while (*p) {
        int n = 0;
        while (*p && *p != '\n' && n < 255)
            b[n++] = *p++;
        b[n] = 0;
        wrap(31, y, 68, b, TEXT);
        y += ((n + 67) / 68) * 11 + 8;
        if (*p == '\n')
            p++;
    }
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
    ui.camX = a->x;
    ui.camY = a->y;
    terrain();
    objects_draw();
    if ((int)(g.time / 140) % 4 == 2) {
        for (int i = 0; i < 25; i++) {
            int x = ((int)(g.time * 63) + i * 79) % 520 - 20, y = (i * 37) % 210;
            line(x, y, x + 14, y + 3, shade(0xc7ba92, .9f));
        }
    }
    hud();
    switch (ui.screen) {
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
        overlay_header("A FELLOW WANDERER", "X RECRUIT FOR 300 SILVER   /   O LEAVE");
        person(r, 103, 201, 3, 0);
        text(186, 92, r->name, TEAL, 2);
        wrap(186, 118, 39,
             "I can work, fight, and carry my share. Three hundred silver settles my debts. After that, we "
             "look out for each other.",
             TEXT);
        text(186, 185, "SQUAD LIMIT: FOUR", GOLD, 1);
        break;
    }
    case SCREEN_CONFIRM:
        overlay_header("CONFIRM EXPEDITION CHANGE", "X CONFIRM    O CANCEL");
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
        rect(25, 254, 431, 13, INK);
        text(28, 257, ui.notice, GOLD, 1);
    }
}
