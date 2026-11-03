#include "game.h"
#include "combat_pose.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared, allocation-free software rasterizer. PSP writes to uncached VRAM.
   No external asset files are required at runtime. */
static uint32_t *fb;
static int pitch;
static int clipLeft=0, clipTop=0, clipRight=480, clipBottom=272;
static const uint32_t INK = 0x171e21, PANEL = 0x252b2c, EDGE = 0x6e715e, GOLD = 0xd8ba78, TEXT = 0xe4dbc0,
                      MUTED = 0xa5aa99, TEAL = 0x8abeb1, RED = 0xcb7560;
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
        w += x-clipLeft;
        x = clipLeft;
    }
    if (y < clipTop) {
        h += y-clipTop;
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
        while (*s && *s != '\n' && n < cols) {
            b[n++] = *s++;
        }
        if (*s && *s != '\n' && n == cols) {
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
        if (*s == '\n') {
            s++;
            y += 4;
        }
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
    *sy = 126 + (int)((x + y - ui.camX - ui.camY) * 11) - terrain_height(x, y);
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
            if (g.weather == 2 || g.weather == 3)
                c = shade(c, .85f);
            c = shade(c, (.965f + (h % 9) * .008f) * li);
            if (x < 1 || y < 1 || x > WORLD - 2 || y > WORLD - 2)
                c = shade(0x5c6867, li);
            else if (t >= 0)
                c = shade(blend(c, 0xd0bb90, .18f), li);
            else if (road(x, y))
                c = shade(blend(c, 0xd4bd8e, .23f), li);
            if (terrain_water(x, y)) {
                c = shade(0x485f5b, li);
                diamond(sx, sy, 22, 11, c);
                int shift = ((int)(g.time * 3) + x + y) % 5;
                line(sx - 9 + shift, sy - 3, sx + 6 + shift, sy - 3, shade(c, 1.25f));
                line(sx - 8, sy + 3, sx + 8, sy + 3, shade(c, .8f));
                continue;
            }
            diamond(sx, sy, 22, 11, c);
            for (int grain = 0; grain < 7; grain++) {
                unsigned z = hash(x + grain * 9, y - grain * 7);
                int gx = (int)(z % 25) - 12, gy = (int)((z >> 8) % 9) - 4;
                px(sx + gx, sy + gy, shade(c, .72f + (z % 7) * .08f));
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
}
static void box(int x, int y, int w, int d, int h, uint32_t c) {
    quad(x - w, y, x, y + d, x, y + d - h, x - w, y - h, shade(c, .73f));
    quad(x, y + d, x + w, y, x + w, y - h, x, y + d - h, shade(c, .9f));
    diamond(x, y - h, w, d, shade(c, 1.08f));
    line(x - w, y - h, x, y + d - h, shade(c, 1.2f));
    line(x, y + d - h, x + w, y - h, shade(c, .93f));
}
static void banner(int x, int y, int f, int size) {
    uint32_t c = factionColors[f % FACTIONS];
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
    uint32_t wall = shade(b->faction == HOSTILE  ? 0x625b50
                          : b->faction == SHOGUN ? 0xa5997f
                                                 : 0x958069,
                          li),
             dark = shade(0x766e5b, li);
    int type = b->type;
    if (type <= TOWER) {
        int w = type == TOWER ? 19 : 34, d = type == TOWER ? 10 : 17, h = type == TOWER ? 65 : 39;
        quad(x - w, y + 4, x + 1, y + 23, x + 58, y + 9, x + 24, y - 8,
             shade(ground(biome(b->x, b->y)), li * .72f));
        box(x, y, w, d, h, wall);
        box(x, y - h, w + 2, d + 1, 5, dark);
        if (type != TOWER) {
            uint32_t roof = shade(b->faction == SHOGUN || b->faction == MONKS ? 0x495653 : 0x5d4a3d, li);
            quad(x - w - 5, y - h, x, y - h - d - 14, x + w + 5, y - h, x, y - h + d, roof);
            for (int z = -w; z <= w; z += 5)
                line(x + z, y - h + abs(z) * 0.2f - d / 2, x + z, y - h + d - abs(z) * .5f,
                     shade(roof, .65f));
            line(x - w - 7, y - h - 3, x, y - h + d, shade(roof, 1.4f));
            line(x, y - h + d, x + w + 7, y - h - 3, shade(roof, 1.25f));
            line(x - 4, y - h - d - 14, x + 4, y - h - d - 14, shade(0x99836a, li));
        } else
            diamond(x, y - h - 5, w - 4, d - 3, shade(0x6c6657, li));
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
        if (type != TOWER) {
            for (int plank = -w + 5; plank < w; plank += 7) {
                int yy = y + abs(plank) * -.45f + d;
                line(x + plank, yy - 3, x + plank, yy - h + 6, shade(wall, .76f));
            }
            line(x - w - 4, y + d + 4, x + w + 4, y - d + 4, shade(0x4b3e30, li));
            ellipse(x + 21, y - 9, 3, 5, shade(0xcfaa70, li));
            line(x + 21, y - 14, x + 21, y - 18, dark);
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
static void person(Actor *a, int x, int y, int sc, int chosen) {
    static const uint32_t skins[] = {0xe0b78a, 0xc39971, 0xad7d58, 0x956645,
                                     0x744c36, 0x553d30, 0xc5b69b, 0x817a68};
    static const uint32_t hairs[] = {0x211e1b, 0x433026, 0x776047, 0xa4835c, 0xc1b5a0, 0x713e32};
    float li = sc == 1 ? light() : 1;
    uint32_t skin = shade(skins[a->skin % 8], li), hair = shade(hairs[a->hairColor % 6], li);
    int torso = a->gear[1], head = a->gear[2], legs = a->gear[4], back = a->gear[5];
    int tf = gear_family(torso), hf = gear_family(head), lf = gear_family(legs), bf = gear_family(back);
    if (tf < 0)
        tf = torso == DUSTER ? 32 : torso == PLATE ? 38 : 35;
    if (hf < 0)
        hf = head == STRAW ? 48 : head == HELM ? 50 : head == HAT ? 51 : -1;
    uint32_t cloth = shade(torso ? itemDefs[torso].color : 0x8a7358, li),
             trim = shade(itemDefs[torso].trim, li), hc = shade(itemDefs[head].color, li);
    uint32_t lc = shade(legs ? itemDefs[legs].color : 0x484239, li), bc = shade(itemDefs[back].color, li),
             boot = shade(a->gear[3] ? itemDefs[a->gear[3]].color : skins[a->skin % 8], li);
    int face = a->facing ? 1 : -1, pose = a->movedTimer > 0 ? (int)(sinf(a->anim * 2) * 2.8f) : 0;
    int missing = a->lostMask & ~a->prostheticMask;
    float progress = a->attackDuration > 0 ? 1 - a->attackClock / a->attackDuration : 0;
    int weapon = a->attackPhase ? a->attackWeapon : a->gear[0];
    CombatPose rig = combat_pose(weapon, a->attackPhase, progress);
    float aim = a->attackPhase ? a->attackAngle : (face > 0 ? 0 : 3.14159265f);
    float forward = cosf(aim), depth = sinf(aim);
    float lean = rig.shift * forward;
    if (a->hitFlash > 0 && a->impactKind == 1)
        lean -= (a->recoilX-a->recoilY)*4*pose_ease(a->hitFlash/.38f);
    float crouch = rig.crouch + (a->sneaking ? 3 : 0);
    /* Longer legs and a narrow torso. Grounded feet stay fixed while the hips load. */
#define RX(xx, yy) (x + (int)lroundf(((xx)*(1+rig.twist) + lean*fminf(1,fmaxf(0,-(yy)/24.f)))*sc))
#define RY(yy) (y + (int)lroundf(((yy)*1.25f + crouch*fminf(1,fmaxf(0,-(yy)/13.f)))*sc))
#define Q(xx, yy, xx1, yy1, xx2, yy2, xx3, yy3, c) quad(RX(xx,yy),RY(yy),RX(xx1,yy1),RY(yy1),RX(xx2,yy2),RY(yy2),RX(xx3,yy3),RY(yy3),c)
#define R(xx, yy, w, h, c) Q(xx,yy,(xx)+(w),yy,(xx)+(w),(yy)+(h),xx,(yy)+(h),c)
#define L(xx, yy, xx1, yy1, c) thickline(RX(xx,yy),RY(yy),RX(xx1,yy1),RY(yy1),sc,c)
    ellipse(x + 2 * sc, y + 2 * sc, 10 * sc, 3 * sc, shade(0x39342c, li));
    if (chosen && sc == 1) {
        diamond(x, y + 2, 13, 6, 0x665948);
        line(x - 13, y + 2, x, y + 8, TEAL);
        line(x, y + 8, x + 13, y + 2, TEAL);
    }
    if (a->kind == BEAST) {
        ellipse(x, y - 7, 12, 5, shade(0x695547, li));
        for (int j = -1; j <= 1; j += 2) {
            L(j * 8, -6, j * 10 + pose, 1, 0x302a26);
            L(j * 6, -5, j * 6 - pose, 2, 0x40372d);
        }
        Q(6, -11, 13, -15, 18, -9, 8, -5, shade(0x99806a, li));
        L(-10, -8, -18, -14, 0x695547);
        R(13, -11, 1, 1, RED);
        return;
    }
    if (a->dead || a->unconscious > 0 || a->taskKind == 3) {
        if (a->bleed > 0 || a->dead)
            ellipse(x + 2 * sc, y + 2 * sc, 10 * sc, 3 * sc, 0x552f26);
        R(-11, -5, 16, 6, cloth);
        R(-17, -4, 7, 3, lc);
        ellipse(x + 8 * sc, y - 3 * sc, 3 * sc, 3 * sc, skin);
        L(-10, -4, 2, -1, trim);
        weapon_sprite(a->gear[0], x - 4 * sc, y + 4 * sc, sc, .1f, li);
        return;
    }
    if (back) {
        if (bf == 80 || bf == 83) {
            int hem = bf == 83 ? -7 : -3;
            Q(-7, -23, 6, -24, 9 + pose, hem, -9, hem - 1, bc);
            L(-9, hem - 1, 9, hem, itemDefs[back].trim);
            for (int j = 0; j <= itemDefs[back].quality; j++)
                R(-9 + j, hem - 3, 1, 1, itemDefs[back].trim);
        } else {
            R(-9, -25, 7 + (bf % 3), 17, shade(bc, .65f));
            R(-9, -25, 7 + (bf % 3), 4, bc);
            L(-8, -23, -8, -10, itemDefs[back].trim);
        }
        if (bf == 77) {
            L(-7, -16, -7, -44, 0x514536);
            Q(-7, -44, 3, -42, 3, -31, -7, -33, bc);
            L(-5, -41, 1, -36, itemDefs[back].trim);
        }
        if (bf == 81) {
            weapon_sprite(gear_id(0, itemDefs[back].material, itemDefs[back].quality), x - 7 * sc,
                          y - 18 * sc, sc, -2.f, li);
        }
        if (bf == 82) {
            R(-10, -27, 15, 5, bc);
            L(-7, -27, -7, -22, trim);
            L(1, -27, 1, -22, trim);
        }
        if (bf == 78) {
            R(-9, -17, 5, 2, TEXT);
            R(-7, -19, 1, 6, TEXT);
        }
        if (bf == 79) {
            R(-12, -20, 4, 10, shade(bc, .8f));
            L(-12, -20, -9, -15, itemDefs[back].trim);
        }
    }
    for (int i = 0; i < 2; i++) {
        int limb = i ? RLEG : LLEG;
        if (missing & (1 << limb))
            continue;
        int xx = i ? 1 : -5, off = i ? pose : -pose;
        if (a->health[limb] < 20) off /= 2;
        float foot = xx + (i ? 1 : -1)*rig.step*forward;
        float knee = xx + rig.shift*forward + (i ? 1 : -1);
        float lift = a->attackPhase ? 0 : fmaxf(0,off)*.5f;
        uint32_t c = a->prostheticMask & (1 << limb) ? 0x777b78 : lc;
        Q(xx,-13,xx+3,-13,knee+3,-7,knee,-7,shade(c,.8f));
        Q(knee,-8,knee+3,-8,foot+3,-2-lift,foot,-2-lift,c);
        L(xx,-12,knee,-7,shade(c,1.12f));
        L(knee,-7,foot,-3-lift,shade(c,1.12f));
        R(foot-1,-3-lift,5,2,shade(boot,.65f));
        R(foot+face,-2-lift,4,1,boot);
        if (lf == 68) {
            R(xx - 1, -13, 6, 7, lc);
            L(xx, -13, xx, -6, itemDefs[legs].trim);
        } else if (lf == 70) {
            for (int j = 0; j < 3; j++)
                R(xx - 1, -13 + j * 2, 5, 1, shade(itemDefs[legs].trim, li));
        } else if (lf == 72) {
            R(xx, -12, 2, 7, shade(lc, .5f));
            R(xx, -8, 4, 2, itemDefs[legs].trim);
        } else if (lf == 75) {
            for (int j = 0; j < 4; j++)
                R(xx + j % 2, -12 + j * 2, 3, 1, shade(itemDefs[legs].trim, li));
        } else if (lf == 71)
            R(xx, -12, 2, 6, shade(c, .6f));
        else if (lf == 74)
            for (int j = 0; j < 3; j++)
                L(xx, -12 + j * 3, xx + 3, -10 + j * 3, itemDefs[legs].trim);
        else if (lf == 73)
            R(-6, -13, 12, 8, lc);
        if (legs)
            for (int j = 0; j <= itemDefs[legs].quality; j++)
                R(xx + j, -6 + off, 1, 1, itemDefs[legs].trim);
        int bootf = gear_family(a->gear[3]);
        if (bootf >= 60) {
            R(xx, -5 + off - (bootf - 60) % 4, 4, 2, shade(boot, 1.35f));
            if (bootf == 64) {
                L(xx, -7 + off, xx + 3, -5 + off, itemDefs[a->gear[3]].trim);
                L(xx, -5 + off, xx + 3, -4 + off, itemDefs[a->gear[3]].trim);
            }
            if (bootf == 66)
                R(xx - 2, -2 + off, 2, 1, itemDefs[a->gear[3]].trim);
            if (bootf == 65)
                R(xx + 3, -3 + off, 3, 1, shade(boot, .5f));
            if (bootf == 67) {
                L(xx, -8 + off, xx + 3, -6 + off, itemDefs[a->gear[3]].trim);
            }
            if (bootf == 63)
                R(xx + 2, -3 + off, 1, 2, INK);
            for (int j = 0; j <= itemDefs[a->gear[3]].quality; j++)
                R(xx + j, -3 + off, 1, 1, itemDefs[a->gear[3]].trim);
        }
    }

    int bw = a->body == 1 ? 5 : a->body == 2 ? 3 : 4;
    int coat = tf == 32 || tf == 33 || tf == 39 || tf == 40 || tf == 42 || tf == 44 || tf == 47;
    if (coat) {
        Q(-bw, -23, bw, -23, bw + 2 + pose - rig.shift, -5, -bw - 2 - rig.shift, -6, cloth);
        L(0, -17, 0, -5, shade(cloth, .48f));
    }
    Q(-bw, -24, bw, -24, bw - 1, -12, -bw + 1, -12, cloth);
    L(-bw, -23, -bw + 1, -13, shade(cloth, 1.18f));
    if (tf == 34 || tf == 36 || tf == 38 || tf == 42 || tf == 44 || tf == 47) {
        R(-bw - 2, -24, 4, 5, shade(cloth, 1.15f));
        R(bw - 1, -24, 4, 5, shade(cloth, .78f));
        for (int j = 0; j < 4; j++) {
            L(-bw + 1, -22 + j * 3, bw - 1, -22 + j * 3, trim);
            for (int z = -bw + 2; z < bw; z += 3)
                R(z, -21 + j * 3, 1, 1, shade(cloth, .5f));
        }
    } else if (tf == 37) {
        for (int j = -1; j <= 1; j++)
            L(j * 3, -23, j * 3 + 1, -15, 0xbbad8c);
    } else {
        L(-bw, -23, bw - 1, -12, trim);
        if (tf == 45) {
            for (int j = 0; j < 5; j++)
                R(-3 + j, -23 + j * 2, 2, 2, 0xb59a6c);
        }
        if (tf == 41 || tf == 43)
            for (int j = -3; j < 4; j += 3)
                L(j, -22, j, -13, shade(cloth, .75f));
    }
    if (tf == 40) { /* ash cloak: asymmetric weather-beaten overlap */
        Q(-bw,-23,1,-22,bw+2,-7,-2,-9,shade(cloth,.66f));
        L(1,-22,-2,-9,shade(trim,.7f));
    }
    if (coat) {
        L(-bw+1,-14,-bw-1-rig.shift,-7,shade(cloth,.62f));
        L(bw-1,-16,bw+1+pose-rig.shift,-7,shade(cloth,1.1f));
    }
    R(-bw, -13, bw * 2, 2, 0x3c3027);
    R(0, -13, 2, 2, trim);
    L(-bw,-14,bw,-14,shade(trim,.65f));
    if (torso) { /* irregular seam wear, subdued against the fabric */
        R(-bw+1,-20,1,2,shade(cloth,.6f));
        R(bw-2,-17,1,1,shade(cloth,1.2f));
    }
    if (torso) {
        for (int j = 0; j < itemDefs[torso].quality + 1; j++)
            R(-bw + 1 + j * 2, -15, 1, 1, trim);
        R(tf % 7 - 3, -21, 1 + (tf % 2), 2, shade(cloth, .65f));
    }
    float angle = aim + face * rig.blade;
    float handx = forward * rig.handX, handy = rig.handY + depth*rig.handX*.4f;
    for (int i = 0; i < 2; i++) {
        int limb = i ? RARM : LARM;
        if (missing & (1 << limb)) continue;
        float sx = i ? bw : -bw;
        float hx = handx, hy = handy;
        if (!i) {
            if (combat_two_hands(weapon)) {
                hx -= cosf(angle)*4; hy -= sinf(angle)*3.2f;
            } else { hx = -face*5; hy = -17 + rig.crouch; }
        }
        /* Two bone arm: elbows bend outward, hands remain attached to the grip. */
        float dx=hx-sx, dy=hy+23, d=sqrtf(dx*dx+dy*dy);
        float bend=sqrtf(fmaxf(0,49-d*d*.25f));
        float ex=(sx+hx)*.5f-(d>.01f ? dy/d : 0)*bend*(i?1:-1);
        float ey=(-23+hy)*.5f+(d>.01f ? dx/d : 1)*bend*(i?1:-1);
        uint32_t c = a->prostheticMask & (1 << limb) ? 0x858982 : skin;
        thickline(RX(sx,-23),RY(-23),RX(ex,ey),RY(ey),3*sc,shade(cloth,.65f));
        thickline(RX(ex,ey),RY(ey),RX(hx,hy),RY(hy),2*sc,shade(c,.8f));
        L(ex,ey,hx,hy,c);
        R(hx-1,hy-1,2,2,c);
    }
    Q(-2,-30,2,-30,3,-26,1,-24,skin);
    Q(-2,-30,-3,-26,-1,-24,1,-24,shade(skin,.68f));
    L(-2,-29,2,-29,shade(skin,.5f));
    R(-2, -23, 4, 2, skin);
    if (a->hair) {
        R(-3, -30, 6, 2, hair);
        if (a->hair == 2 || a->hair == 4)
            R(-4, -29, 2, 8, hair);
        if (a->hair == 3)
            R(-1, -34, 3, 5, hair);
        if (a->hair == 5)
            L(-3, -28, -5, -21, hair);
        if (a->hair == 6) {
            R(2, -30, 3, 3, hair);
            R(3, -27, 2, 6, hair);
        }
    }
    R(face > 0 ? 1 : -2, -27, 1, 1, INK);
    if (a->beard) {
        R(-2, -24, 4, 1 + a->beard, hair);
    }
    if (a->scar) {
        L(a->scar % 3 - 1, -27, a->scar % 3, -24, 0x8a4837);
    }
    if (a->race == 2) {
        R(-3, -27, 2, 2, 0x89a6a1);
        R(-3, -27, 1, 1, 0xd2e0b8);
    }
    if (head) {
        if (hf == 48 || hf == 58) {
            Q(-9, -28, 0, hf == 48 ? -35 : -32, 9, -28, 0, -27, hc);
            L(-9, -28, 9, -28, shade(hc, .6f));
            for (int j = -6; j < 8; j += 3)
                L(0, -33, j, -28, itemDefs[head].trim);
        } else if (hf == 49 || hf == 56) {
            R(-5, -33, 9, 6, hc);
            L(-9, -28, 9, -28, shade(hc, 1.3f));
            R(-4, -29, 8, 1, itemDefs[head].trim);
            if (hf == 56)
                R(-3, -27, 6, 2, 0x2d3432);
        } else if (hf == 50 || hf == 53 || hf == 59) {
            Q(-5, -29, -4, -33, 3, -34, 5, -29, hc);
            R(-6, -29, 12, 2, hc);
            R(-5, -27, 2, 6, hc);
            R(3, -27, 2, 6, hc);
            if (hf != 53) {
                L(-5, -31, -8, -36, itemDefs[head].trim);
                L(5, -31, 8, -36, itemDefs[head].trim);
            }
            if (hf == 59)
                R(-1, -37, 2, 5, 0x87422f);
        } else {
            R(-4, -31, 8, 4, hc);
            R(-5, -28, 2, 7, hc);
            R(3, -28, 2, 7, hc);
            if (hf == 51 || hf == 55) {
                R(-3, -25, 7, 3, hc);
                R(2, -25, 1, 1, trim);
                if (hf == 55) {
                    R(-4, -26, 2, 4, 0xd1b784);
                    R(3, -26, 2, 4, 0xd1b784);
                    L(-4, -31, -6, -35, hc);
                }
            }
            if (hf == 57) {
                Q(-6, -29, 0, -35, 6, -29, 0, -31, hc);
            }
            if (hf == 52) {
                L(-4, -29, 4, -31, itemDefs[head].trim);
                L(-4, -27, 4, -29, itemDefs[head].trim);
            }
        }
        for (int j = 0; j <= itemDefs[head].quality; j++)
            R(-2 + j * 2, -30, 1, 1, itemDefs[head].trim);
    }
    if (!(missing & (1 << RARM)))
        weapon_sprite(a->attackPhase ? a->attackWeapon : a->gear[0], RX(handx,handy), RY(handy), sc,
                      angle, li);
    if (a->carrying >= 0) {
        R(-9, -25, 18, 5, 0x7c6852);
        ellipse(x + 10 * sc, y - 23 * sc, 3 * sc, 3 * sc, skin);
    }
    /* Brief, restrained muzzle flash. No luminous circular melee trails. */
    if (sc == 1 && a->attackPhase == 2 && weapon_ranged(weapon) &&
        progress >= STRIKE_CONTACT && progress < STRIKE_CONTACT+.18f) {
        int ex=RX(handx,handy)+(int)(cosf(angle)*27);
        int ey=RY(handy)+(int)(sinf(angle)*27);
        if (gear_family(weapon)>=29) {
            line(ex,ey,ex+(int)(cosf(angle)*6),ey+(int)(sinf(angle)*6),0xd7b98a);
            px(ex,ey,0xeee0bc);
        }
    }
    if (sc == 1 && a->hitFlash > 0) {
        uint32_t c = a->impactKind == 2 ? 0xe6d3a3 : a->impactKind == 3 ? MUTED : 0x9a4934;
        float t = (.38f - a->hitFlash) * 24;
        for (int j = 0; j < 6; j++) {
            float an = j * 1.0472f;
            line(x + cosf(an) * t, y - 19 + sinf(an) * t, x + cosf(an) * (t + 3), y - 19 + sinf(an) * (t + 3),
                 c);
        }
        char b[16];
        snprintf(b, sizeof(b), "-%d", a->impactDamage);
        text(x - 6, y - 43 - (int)t,
             a->impactKind == 2   ? "CLANG"
             : a->impactKind == 3 ? "DODGE"
                                  : b,
             a->impactKind == 1 ? RED : GOLD, 1);
    }
#undef R
#undef L
#undef Q
#undef RX
#undef RY
}
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
            int region = o->index / 4, variant = o->index % 4;
            if (region == 7) {
                line(x, y, x, y - 43, shade(0x443a2b, light()));
                for (int z = 0; z < 3; z++) {
                    int yy = y - 12 - z * 10;
                    triangle(x - 13 + z * 3, yy, x, yy - 23, x + 13 - z * 3, yy,
                             shade(z % 2 ? 0x344b3b : 0x435540, light()));
                }
            } else if (region == 3) {
                for (int j = 0; j < 5; j++) {
                    line(x - 7 + j * 3, y, x - 10 + j * 4, y - 15 - j % 3 * 4, shade(0x747b4c, light()));
                    rect(x - 11 + j * 4, y - 18 - j % 3 * 4, 2, 6, shade(0x4b4837, light()));
                }
            } else if (region == 8 || region == 5) {
                int h = 18 + variant * 9;
                triangle(x - 10, y, x - 5, y - h, x + 12, y, shade(0x6b4d3c, light()));
                triangle(x - 5, y - h, x + 2, y - h + 2, x + 12, y, shade(0xa77e58, light()));
                for (int z = 5; z < h; z += 7)
                    line(x - 5, y - z, x + 7 - z / 6, y - z + 2, shade(0x47382d, light()));
            } else if (variant < 2) {
                triangle(x - 6, y, x - 2, y - 6, x + 7, y, shade(0x8b8369, light()));
                line(x - 2, y - 6, x + 7, y, shade(0xb5a886, light()));
            } else {
                for (int j = 0; j < 4; j++)
                    line(x, y, x - 5 + j * 3, y - 4 - (j % 2) * 3, shade(0x757954, light()));
            }
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
    mini_map();
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
            rect(x, 219, 65, 33, i == g.selected ? 0x374640 : PANEL);
            frame(x, 219, 65, 33, i == g.selected ? TEAL : EDGE);
            Actor portrait = *m;
            portrait.attackPhase=0; portrait.hitFlash=0; portrait.movedTimer=0;
            clipLeft=x+1; clipTop=220; clipRight=x+23; clipBottom=251;
            person(&portrait, x+12, 261, 1, 0);
            clipLeft=clipTop=0; clipRight=480; clipBottom=272;
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
    text(8, 261, "X ACT TRI PACK SQ FIGHT O SNEAK L SQUAD R PAUSE", MUTED, 1);
    text(355, 261, "SEL MAP START MENU", MUTED, 1);
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
                                                : "TALK / SERVICES";
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
    const char *opts[] = {"WANDERER - ONE DRIFTER", "CARAVAN - THREE SURVIVORS", "CONTINUE EXPEDITION",
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
    text(28, 237, "O CLOSE   SELECT GUIDE", MUTED, 1);
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
static void pack_screen(void) {
    Actor *a = &g.actors[g.selected];
    char b[100];
    overlay_header("PACK & EQUIPMENT", "X EQUIP / USE   TRI WORN GEAR   SQUARE PASS TO ALLY");
    person(a, 101, 179, 2, 0);
    int first = start_row(ui.selection, PACK, 8);
    for (int j = 0; j < 8; j++) {
        int k = first + j, y = 84 + j * 15, it = a->pack[k].item;
        if (k == ui.selection)
            rect(199, y - 3, 250, 14, 0x3c4034);
        snprintf(b, 100, "%02d %.30s", k + 1, it ? itemDefs[it].name : "--");
        text(207, y, b, k == ui.selection ? TEXT : MUTED, 1);
        if (it) {
            snprintf(b, 100, "%d", a->pack[k].qty);
            text(443 - strlen(b) * 6, y, b, GOLD, 1);
        }
    }
    int it = a->pack[ui.selection].item;
    snprintf(b, 100, "LOAD %.0f/%d", encumbrance(a), 30 + level(a, STRENGTH) * 2 + gear_bonus(a, 3));
    text(31, 188, b, TEAL, 1);
    if (it) {
        snprintf(b, 100, "DMG %d  ARM %d  WT %d   %s", itemDefs[it].power, itemDefs[it].armor,
                 itemDefs[it].weight, a->stolen[ui.selection] ? "STOLEN" : "");
        text(31, 210, b, TEXT, 1);
        clipped(31, 222, itemDefs[it].desc, 68, MUTED);
    } else
        text(31, 220, "24 STACKS. EQUIPMENT HAS SIX VISIBLE SLOTS.", MUTED, 1);
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
    overlay_header("SQUAD ORDERS", "LEFT / RIGHT MEMBER   X JOB   SQUARE DISMISS FALLEN");
    clipped(31, 85, a->name, 16, TEAL);
    char b[60];
    snprintf(b, 60, "%d / 8 SURVIVORS", g.squadCount);
    text(326, 85, b, GOLD, 1);
    for (int i = 0; i < 8; i++)
        row(103 + i * 15, ui.selection == i, jobNames[i], a->job == i ? "ASSIGNED" : "");
}

static void map_screen(void) {
    overlay_header("ATLAS OF THE FRONTIER", "D-PAD CURSOR   X TRAVEL   L/R TOWNS   SQUARE CHART");
    int ox = 29, oy = 82, mw = 256, mh = 136;
    static uint32_t atlas[256 * 136], cachedHash;
    static int ready;
    uint32_t h = g.seed;
    for (unsigned i = 0; i < sizeof(g.seen); i++)
        h = (h ^ g.seen[i]) * 16777619u;
    if (!ready || h != cachedHash) {
        for (int y = 0; y < mh; y++)
            for (int x = 0; x < mw; x++) {
                int wx = x * WORLD / mw, wy = y * WORLD / mh, k = wy * WORLD + wx,
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
        if (g.discovered[t]) {
            int x = ox + towns[t].x * mw / WORLD, y = oy + towns[t].y * mh / WORLD;
            rect(x - 2, y - 2, 5, 5, factionColors[g.townOwner[t]]);
            frame(x - 2, y - 2, 5, 5, GOLD);
        }
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && !g.actors[i].dead)
            rect(ox + g.actors[i].x * mw / WORLD - 1, oy + g.actors[i].y * mh / WORLD - 1, 3, 3, TEAL);
    if (g.baseExists)
        frame(ox + g.baseX * mw / WORLD - 3, oy + g.baseY * mh / WORLD - 3, 7, 7, TEAL);
    int x = ox + ui.mapX * mw / WORLD, y = oy + ui.mapY * mh / WORLD;
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
    snprintf(b, 90, "%03d / %03d  %s", (int)ui.mapX, (int)ui.mapY, regionNames[biome(ui.mapX, ui.mapY)]);
    text(31, 222, b, TEAL, 1);
}

static void trade_screen(void) {
    char b[100], v[70];
    const char *filters[] = {"ALL", "SUPPLIES", "WEAPONS", "ARMOR"};
    overlay_header(ui.sell ? "THE MARKET / SELL" : "THE MARKET / BUY",
                   "LEFT / RIGHT BUY-SELL   L/R FILTER   X TRADE ONE");
    int count = ui.sell ? PACK : shop_count(), first = start_row(ui.selection, count, 8);
    for (int j = 0; j < 8 && first + j < count; j++) {
        int i = first + j, it = ui.sell ? g.actors[g.selected].pack[i].item : shop_item(i);
        snprintf(b, 100, "%.39s", it ? itemDefs[it].name : "--");
        snprintf(v, 70, "%d S  X%d", item_price(ui.tradeTown, it, ui.sell),
                 ui.sell ? g.actors[g.selected].pack[i].qty : g.stock[ui.tradeTown][it]);
        row(85 + j * 16, i == ui.selection, b, it ? v : "");
    }
    int it = ui.sell ? g.actors[g.selected].pack[ui.selection].item : shop_item(ui.selection);
    if (it)
        clipped(31, 210, itemDefs[it].desc, 68, TEAL);
    snprintf(b, 100, "%s  /  %d SILVER  /  %s", towns[ui.tradeTown].name, g.money, filters[ui.shopFilter]);
    text(31, 221, b, GOLD, 1);
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
    overlay_header("EXPEDITION", "UP / DOWN CHOOSE   X OPEN   SELECT GUIDE");
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
    overlay_header("FIELD GUIDE", "L/R OR UP/DOWN CHAPTER   O RETURN TO PREVIOUS SCREEN");
    char b[100];
    snprintf(b, 100, "%02d / %02d   %s", ui.selection + 1, GUIDE_PAGES, guideTitles[ui.selection]);
    text(31, 86, b, TEAL, 1);
    wrap(31, 106, 68, guideBodies[ui.selection], TEXT);
}
static void creator_screen(void) {
    overlay_header(ui.titleChoice ? "THE BARBER" : "A NAME IN THE DUST",
                   "LEFT / RIGHT CHANGE   X CHOOSE   SELECT GUIDE");
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
    overlay_header("NAME YOUR DRIFTER", "D-PAD SELECT   X LETTER   SQUARE ERASE   START DONE");
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
    overlay_header("WORN EQUIPMENT", "X UNEQUIP   TRI PACK   SELECT GUIDE");
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
    overlay_header("THE IRON TRADE", "LEFT / RIGHT CONFIGURE   X FORGE   SELECT GUIDE");
    int it = gear_id(ui.forgeFamily, ui.forgeMaterial, ui.forgeQuality);
    ItemDef *d = &itemDefs[it];
    Actor a = preview_actor();
    a.gear[d->slot] = it;
    person(&a, 102, 184, 2, 0);
    const char *labels[] = {"SLOT", "DESIGN", "MATERIAL", "QUALITY", "FORGE THIS PIECE"};
    const char *values[] = {slotNames[ui.forgeSlot], familyNames[ui.forgeFamily],
                            materialNames[ui.forgeMaterial], qualityNames[ui.forgeQuality], "X"};
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
    overlay_header(a->name, "X REPLY   UP / DOWN CHOOSE   O LEAVE   SELECT GUIDE");
    person(a, 76, 178, 2, 0);
    clipped(30, 204, factionNames[a->faction], 23, TEAL);
    wrap(134, 85, 50, dialogue_line(n, ui.dialogPage), TEXT);
    const char *opts[] = {"Show me the market.",
                          "Any paying work?",
                          "A bed for the night. (25)",
                          "Treat my wounds. (80)",
                          "Bounties and news.",
                          "Change my appearance.",
                          a->kind == RECRUIT ? "Travel with us. (300)" : "What have you heard?",
                          "Let me use the forge.",
                          "Safe travels."};
    const char *work[] = {"Carry a dispatch - 220 silver", "Bring eight ore - 240 silver",
                          "Defeat three foes - 420 silver", "Recover a relic - 550 silver", "Back"};
    const char *prison[] = {"Pay my bail.", "Try the lock. Requires tools.", "Wait out the sentence."};
    int jailed = a->prisonTime > 0 && n == g.selected,
        count = jailed               ? 3
                : ui.dialogPage == 1 ? 5
                                     : 9,
        first = start_row(ui.selection, count, 5);
    for (int j = 0; j < 5 && first + j < count; j++) {
        int k = first + j, y = 153 + j * 15;
        if (k == ui.selection)
            rect(129, y - 3, 318, 14, 0x3c4034);
        text(137, y,
             jailed               ? prison[k]
             : ui.dialogPage == 1 ? work[k]
                                  : opts[k],
             k == ui.selection ? GOLD : MUTED, 1);
    }
}
static void save_screen(void) {
    overlay_header(ui.saveMode ? "SAVE EXPEDITION" : "LOAD EXPEDITION",
                   "X SELECT SLOT   SLOT ZERO AUTOSAVES   SELECT GUIDE");
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
    overlay_header("THE ROAD JOURNAL", "X CLAIM AT THE ISSUING TOWN   SELECT GUIDE");
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
                   "LEFT / RIGHT CHOOSE SIDE   X MOVE ONE   SELECT GUIDE");
    int count = ui.storageSide ? 16 : PACK, first = start_row(ui.selection, count, 9);
    char b[30];
    for (int j = 0; j < 9 && first + j < count; j++) {
        int k = first + j;
        Stack st = ui.storageSide ? z->storage[k] : a->pack[k];
        snprintf(b, 30, "X%d", st.qty);
        row(86 + j * 15, k == ui.selection, st.item ? itemDefs[st.item].name : "EMPTY", st.item ? b : "");
    }
}
static void loot_screen(void) {
    Actor *a = &g.actors[ui.lootActor];
    overlay_header("SALVAGE THE FALLEN", "X TAKE ONE   UP / DOWN PACK AND WORN GEAR");
    int first = start_row(ui.selection, PACK + GEARS, 9);
    char b[80], v[30];
    for (int j = 0; j < 9; j++) {
        int k = first + j, it = k < PACK ? a->pack[k].item : a->gear[k - PACK];
        snprintf(b, 80, "%s %.34s", k < PACK ? "PACK" : slotNames[k - PACK],
                 it ? itemDefs[it].name : "EMPTY");
        snprintf(v, 30, "X%d", k < PACK ? a->pack[k].qty : 1);
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
    terrain();
    objects_draw();
    if (g.weather) {
        for (int i = 0; i < 25; i++) {
            int x = ((int)(g.time * 63) + i * 79) % 520 - 20, y = (i * 37) % 210;
            line(x, y, x + (g.weather == 1 ? 14 : 3), y + (g.weather == 1 ? 3 : 9),
                 g.weather == 3   ? 0x829a6b
                 : g.weather == 4 ? 0xc7cbbd
                                  : shade(0xc7ba92, .85f));
        }
    }
    if (ui.screen == SCREEN_WORLD)
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
        overlay_header("A FELLOW WANDERER", "X RECRUIT FOR 300 SILVER   /   O LEAVE");
        person(r, 103, 201, 2, 0);
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
