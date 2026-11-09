/* Articulated painted parts share the simulation's combat pose and rigid grip.
   Only world sprites are composed at 2x and filtered. Portraits sample the same
   indexed art directly; no runtime decoder, allocation or per-outfit atlas. */
#include "weapon_design.h"
static uint32_t paintHair = 0x30251e;
/* A fight interleaves many outfits. Keep their small shaded palettes across
   poses instead of rebuilding the palette each time a sprite changes wearer. */
enum { PALETTE_SETS = 32, PALETTE_WAYS = 4 };
typedef struct {
    uint64_t key;
    uint32_t colors[256];
} PaintPalette;
static PaintPalette paintPalettes[PALETTE_SETS][PALETTE_WAYS];
static unsigned char paletteNext[PALETTE_SETS];

static uint32_t *palette_slot(int id, uint32_t tint, uint32_t hair, unsigned brightness, int *fresh) {
    uint64_t key = (uint64_t)(id + 1) << 56 | (uint64_t)(hair & 0xffffff) << 32 |
                   (uint64_t)(brightness & 255) << 24 | (tint & 0xffffff);
    uint32_t hash = (uint32_t)key ^ (uint32_t)(key >> 32);
    hash = (hash ^ (hash >> 16)) * 0x7feb352du;
    unsigned set = (hash ^ (hash >> 15)) & (PALETTE_SETS - 1);
    for (int i = 0; i < PALETTE_WAYS; i++) {
        if (paintPalettes[set][i].key == key) {
            *fresh = 0;
            return paintPalettes[set][i].colors;
        }
    }
    PaintPalette *slot = &paintPalettes[set][paletteNext[set]++ & (PALETTE_WAYS - 1)];
    slot->key = key;
    *fresh = 1;
    return slot->colors;
}
/* Exclusive edges in the current paint target. The tracked rectangle bounds
   the scratch work without changing how individual parts are rasterized. */
typedef struct {
    int left, top, right, bottom;
} PaintBounds;
static PaintBounds *trackedPaint;

static inline PaintBounds clip_paint(PaintBounds b, int left, int top, int right, int bottom) {
    if (b.left < left) b.left = left;
    if (b.top < top) b.top = top;
    if (b.right > right) b.right = right;
    if (b.bottom > bottom) b.bottom = bottom;
    return b;
}
static inline void include_paint(PaintBounds *b, PaintBounds area) {
    if (area.left >= area.right || area.top >= area.bottom)
        return;
    if (area.left < b->left) b->left = area.left;
    if (area.top < b->top) b->top = area.top;
    if (area.right > b->right) b->right = area.right;
    if (area.bottom > b->bottom) b->bottom = area.bottom;
}
static inline void track_paint(PaintBounds area) {
    if (trackedPaint)
        include_paint(trackedPaint, clip_paint(area, clipLeft, clipTop, clipRight, clipBottom));
}
static void painted_line(int x0, int y0, int x1, int y1, uint32_t color) {
    track_paint((PaintBounds){x0 < x1 ? x0 : x1, y0 < y1 ? y0 : y1,
                              (x0 > x1 ? x0 : x1) + 1, (y0 > y1 ? y0 : y1) + 1});
    line(x0, y0, x1, y1, color);
}
static void painted_quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                         uint32_t color) {
    if (trackedPaint) {
        PaintBounds b = {x0, y0, x0 + 1, y0 + 1};
        include_paint(&b, (PaintBounds){x1, y1, x1 + 1, y1 + 1});
        include_paint(&b, (PaintBounds){x2, y2, x2 + 1, y2 + 1});
        include_paint(&b, (PaintBounds){x3, y3, x3 + 1, y3 + 1});
        track_paint(b);
    }
    quad(x0, y0, x1, y1, x2, y2, x3, y3, color);
}
static uint32_t alphaReciprocal[4 * 255 + 1];
static inline uint32_t alpha_reciprocal(unsigned alpha) {
    uint32_t reciprocal = alphaReciprocal[alpha];
    if (!reciprocal)
        alphaReciprocal[alpha] = reciprocal = (1u << 24) / alpha;
    return reciprocal;
}
static inline unsigned alpha_average(unsigned sum, unsigned alpha, uint32_t reciprocal) {
    /* sum <= 255 * alpha, so this product fits in 32 bits. Truncating the
       reciprocal can miss by one. The remainder check restores exact division. */
    unsigned value = (sum * reciprocal) >> 24;
    return value + (sum - value * alpha >= alpha);
}
static void rgba_over(uint32_t *dst, uint32_t src) {
    unsigned a = src >> 24;
    if (!a)
        return;
    if (a == 255) {
        *dst = src;
        return;
    }
    uint32_t d = *dst;
    if (!(d >> 24)) {
        *dst = src;
        return;
    }
    unsigned da = d >> 24, remain = da * (255 - a) / 255, out = a + remain;
    unsigned red = (src & 255) * a + (d & 255) * remain;
    unsigned green = ((src >> 8) & 255) * a + ((d >> 8) & 255) * remain;
    unsigned blue = ((src >> 16) & 255) * a + ((d >> 16) & 255) * remain;
    /* The world framebuffer is opaque. Constant division is cheaper on MIPS. */
    if (out == 255)
        *dst = 0xff000000 | (red / 255) | (green / 255 << 8) | (blue / 255 << 16);
    else {
        uint32_t reciprocal = alpha_reciprocal(out);
        *dst = (out << 24) | alpha_average(red, out, reciprocal) |
               (alpha_average(green, out, reciprocal) << 8) |
               (alpha_average(blue, out, reciprocal) << 16);
    }
}
static const uint32_t *part_palette(int id, uint32_t tint, float li, int quality) {
    int lum = (int)(li * 64) + quality * 2;
    int head = id >= 60 && id < 64, fresh;
    uint32_t *palette = palette_slot(id, tint, head && id != 61 ? paintHair : 0, lum, &fresh);
    if (!fresh)
        return palette;
    for (int i = 0; i < 256; i++) {
        uint32_t c = artSprites[id].palette[i];
        int r = c & 255, g0 = (c >> 8) & 255, b = (c >> 16) & 255;
        if (head) {
            if (id != 61 && r < 78 && g0 < 70) {
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
        r = r * lum / 64;
        g0 = g0 * lum / 64;
        b = b * lum / 64;
        palette[i] = (c & 0xff000000) | (r > 255 ? 255 : r) | ((g0 > 255 ? 255 : g0) << 8) |
                     ((b > 255 ? 255 : b) << 16);
    }
    return palette;
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
    if (trackedPaint)
        include_paint(trackedPaint, (PaintBounds){left, top, right, bottom});
    const uint32_t *palette = part_palette(id, tint, li, quality);
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
            rgba_over(fb + y * pitch + x, palette[s->pixels[s->rows[sy * 3] + sx - lo]]);
        }
    }
}
/* Paint the complete weapon around its authored handle pivot. Palette tinting
   preserves warm leather/wood and patina while changing exposed metal. */
static const uint32_t *weapon_palette(int id, const ItemDef *item, float li) {
    int lum = (int)(li * 64) + item->quality * 2;
    int fresh;
    uint32_t *palette = palette_slot(id, item->color, 0, lum, &fresh);
    if (!fresh)
        return palette;
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
        palette[i] = (c & 0xff000000) | (r > 255 ? 255 : r) | ((green > 255 ? 255 : green) << 8) |
                     ((b > 255 ? 255 : b) << 16);
    }
    return palette;
}
static int weapon_family(int item) {
    int f = gear_family(item);
    if (f < 0)
        f = item == STAFF ? 16 : item == CROSSBOW ? 25 : item == CLEAVER ? 11 : item == SABRE ? 8 : 0;
    return f >= 84 && f < FAMILIES ? f - 52 : f < 32 ? f : 0;
}
/* Find the painted blade/barrel at a longitudinal station. Curved blades do
   not lie on the grip axis; a fitting must meet the metal, not float beside it. */
static float weapon_center(int item,float along){
    if(weapon_rapier(item)||along<4)return 0;
    int f=weapon_family(item);const ArtSprite*s=&artSprites[WEAPON_ART_FIRST+f];const WeaponArt*w=&weaponArt[f];
    WeaponDesign d=weapon_design(item);float a=weapon_design_unalong(along,weapon_design_anchor(item),d);
    int row=(int)(w->gripY-a/w->scale);if(row<0)row=0;if(row>=s->h)row=s->h-1;
    int lo=s->rows[row*3+1],hi=s->rows[row*3+2];float total=0,weighted=0;
    for(int i=lo;i<hi;i++){unsigned alpha=s->palette[s->pixels[s->rows[row*3]+i-lo]]>>24;weighted+=(i+.5f)*alpha;total+=alpha;}
    float blend=weapon_design_blend(a),center=total?weighted/total-w->gripX:0;
    return center*w->scale*w->edgeSide*(1+(d.width-1)*blend)+d.sweep*blend*blend;
}
/* Small forged fittings share the same rigid handle-to-tip axis as the paint. */
static void weapon_fitting(int item,float x,float y,int sc,float angle,int face,float li,float tip){
    WeaponDesign d=weapon_design(item);int p=d.pattern;float ca=cosf(angle),sa=sinf(angle);
    uint32_t metal=shade(itemDefs[item].color,li*1.15f),trim=shade(itemDefs[item].trim,li),dark=shade(0x3e3025,li);
#define WX(a,b) (x+(ca*(a)-sa*((b)+weapon_center(item,a))*face)*sc)
#define WY(a,b) (y+(sa*(a)+ca*((b)+weapon_center(item,a))*face)*sc)
#define WL(a,b,c,e,col) painted_line(WX(a,b),WY(a,b),WX(c,e),WY(c,e),col)
#define WQ(a,b,c,e,f,h,j,k,col) painted_quad(WX(a,b),WY(a,b),WX(c,e),WY(c,e),WX(f,h),WY(f,h),WX(j,k),WY(j,k),col)
    if(d.group==0){
        float guard=(p==0?2:p==1?5:p==2?3:p==3?1.5f:p==4?4:p==5?2.5f:5.5f)*.7f;
        WQ(1,-guard,2,-guard,2,guard,1,guard,trim);
        if(p==6){WL(2,-guard,5,-guard,trim);WL(2,guard,5,guard,trim);}
        if(p==7||p==10){int count=p==7?2:1;for(int r=0;r<count;r++)for(int i=0;i<12;i++){float a=i*.523599f,b=(i+1)*.523599f;WL(-1+cosf(a)*(3+r),sinf(a)*(3+r),-1+cosf(b)*(3+r),sinf(b)*(3+r),trim);}}
        if(p==8){if(weapon_rapier(item)){WL(2,-guard,-4,-guard,trim);WL(-4,-guard,-5,0,trim);}else WQ(tip*.65f,0,tip*.78f,0,tip*.63f,2.5f,tip*.60f,2,metal);}
        if(p==9){if(weapon_rapier(item)){WL(-2,-4,3,4,trim);WL(3,-4,-2,4,trim);}else for(int i=0;i<4;i++){float a=tip*.45f+i*tip*.1f;WQ(a,0,a+2,0,a+1,2,a,1,metal);}}
        if(p==11){WQ(-5,-1,-6,-3,-8,0,-6,3,trim);WL(tip*.25f,0,tip*.65f,0,trim);}
        WL(-1,-.7f,-4,-.7f,dark);if(p%3==1)WL(-4,-2,-4,2,trim);
    }else if(d.group==1){
        int f=gear_family(item);float at=tip*.72f;
        WL(at,-2,at,2,trim);WL(at+1,-2,at+1,2,trim);
        if(p==5||p==6||p==11){WQ(tip-3,-1,tip+2,0,tip-3,1,tip-4,0,metal);}
        if(p==6||p==7||p==8){float span=p==7?4:3;WQ(at,-1,at+3,-span,at+5,-span,at+1,1,metal);if(p!=8)WQ(at,1,at+3,span,at+5,span,at+1,-1,metal);}
        if(p==9)for(int i=0;i<4;i++){float a=tip*.55f+i*2;WQ(a,-3,a+1,-5,a+2,-3,a+1,3,metal);}
        if(p==10)for(int k=0;k<4;k++)WL(tip*.4f+k*3,-3,tip*.4f+k*3,3,trim);
        if(f==16&&p<5){WL(tip-1,-2,tip-1,2,trim);WL(-7,-2,-7,2,trim);}
    }else if(d.group==2){
        float at=tip*.65f;
        if(p==6||p==8){WL(at,-2,tip+2,-4,trim);WL(at,2,tip+2,4,trim);}
        if(p==7||p==10){WL(-5,-4,at,2,dark);WL(-5,4,at,-2,dark);}
        if(p==5||p==9||p==11)for(int k=0;k<3;k++)WL(at-k*3,-2,at-k*3,2,trim);
    }else{
        if(p==5||p==9){WQ(tip-3,-1,tip+.5f,-1,tip+.5f,1,tip-3,1,metal);if(p==9)for(int k=0;k<4;k++)WL(tip-4-k*2,-1.5f,tip-4-k*2,1.5f,trim);}
        if(p==6){WQ(5,2,tip,2,tip,4,5,4,metal);WL(5,3,tip,3,dark);}
        if(p==7||p==8)WQ(-3,1,-8,5,-9,4,-5,0,dark);
        if(p==10||p==11){WQ(3,-3,12,-3,12,-5,3,-5,metal);WL(5,-2,5,-4,trim);WL(10,-2,10,-4,trim);}
    }
#undef WX
#undef WY
#undef WL
#undef WQ
}
typedef struct {
    float left, right, slope;
} WeaponScanSpan;
static WeaponScanSpan weapon_scan_span(float low, float high, float x, int sc,
                                       float xComponent, float yComponent, int x0, int x1) {
    /* The bounding box already handles the axis nearly parallel to the row. */
    if (fabsf(xComponent) < .125f)
        return (WeaponScanSpan){x0, x1, 0};
    float inverse = sc / xComponent, origin = x - .5f;
    float a = origin + low * inverse, b = origin + high * inverse;
    return (WeaponScanSpan){a < b ? a : b, a < b ? b : a, -yComponent * inverse};
}
static void weapon_sprite(int item, float x, float y, int sc, float angle, int face, float li) {
    if (!item)return;
    WeaponDesign design=weapon_design(item);float ca=cosf(angle),sa=sinf(angle),anchor=weapon_design_anchor(item);
    if(weapon_rapier(item)){
        float len=24*design.length;uint32_t steel=shade(itemDefs[item].color,li);
        for(int i=0;i<(design.pattern==1?2:1)*sc;i++)painted_line(x+ca*3*sc-sa*i,y+sa*3*sc+ca*i,x+ca*len*sc,y+sa*len*sc,steel);
        painted_line(x,y,x-ca*4*sc,y-sa*4*sc,0x614a38);
        weapon_fitting(item,x,y,sc,angle,face,li,len);return;
    }
    int f=weapon_family(item),id=WEAPON_ART_FIRST+f;
    const ArtSprite *s=&artSprites[id];const WeaponArt *w=&weaponArt[f];
    float scale=w->scale,tip=weapon_design_along(w->gripY*scale,anchor,design);
    int parity=w->edgeSide*face;
    float left=1e6f,top=1e6f,right=-1e6f,bottom=-1e6f;
    for(int j=0;j<4;j++){
        float along=(w->gripY-((j&1)?s->h:0))*scale;
        float across=(((j&2)?s->w:0)-w->gripX)*scale*parity;
        /* Conservative bounds include the maximum sweep in either direction. */
        along=weapon_design_along(along,anchor,design);
        across*=fmaxf(1,design.width);
        float xx=x+(ca*along-sa*across)*sc,yy=y+(sa*along+ca*across)*sc;
        left=fminf(left,xx);right=fmaxf(right,xx);top=fminf(top,yy);bottom=fmaxf(bottom,yy);
    }
    float margin=(fabsf(design.sweep)+1)*sc;
    int x0=fmaxf(clipLeft,floorf(left-margin)),x1=fminf(clipRight,ceilf(right+margin));
    int y0=fmaxf(clipTop,floorf(top-margin)),y1=fminf(clipBottom,ceilf(bottom+margin));
    if (trackedPaint)
        include_paint(trackedPaint, (PaintBounds){x0, y0, x1, y1});
    /* Intersect each row with the weapon's local rectangle. The final
       pixel tests still use the full warp and the original atlas samples. */
    float maxWidth = fmaxf(1, design.width);
    float c0 = -w->gripX * scale * parity * maxWidth;
    float c1 = (s->w - w->gripX) * scale * parity * maxWidth;
    float sweep = design.sweep * face;
    float alongLow = weapon_design_along((w->gripY - s->h) * scale, anchor, design);
    WeaponScanSpan alongSpan = weapon_scan_span(alongLow, tip, x, sc, ca, sa, x0, x1);
    WeaponScanSpan acrossSpan = weapon_scan_span(fminf(c0, c1) + fminf(0, sweep),
                                                 fmaxf(c0, c1) + fmaxf(0, sweep),
                                                 x, sc, -sa, ca, x0, x1);
    const uint32_t *palette = weapon_palette(id, &itemDefs[item], li);
    /* The horizontal projection repeats on every row. Evaluate it once at
       the same pixel centers and reuse the exact float contributions. */
    int columns = x1 > x0 ? x1 - x0 : 1;
    struct { float along, across; } column[columns];
    for (int xx = x0; xx < x1; xx++) {
        float dx = (xx + .5f - x) / sc;
        column[xx - x0].along = dx * ca;
        column[xx - x0].across = -dx * sa;
    }
    for (int yy = y0; yy < y1; yy++) {
        float dy = (yy + .5f - y) / sc;
        float alongY = dy * sa, acrossY = dy * ca;
        float a = alongSpan.slope * dy, b = acrossSpan.slope * dy;
        float rowLeft = alongSpan.left + a, rowRight = alongSpan.right + a;
        float acrossLeft = acrossSpan.left + b, acrossRight = acrossSpan.right + b;
        /* Pad outward to account for integer conversion and float rounding. */
        int first = (int)(rowLeft > acrossLeft ? rowLeft : acrossLeft) - 1;
        int end = (int)(rowRight < acrossRight ? rowRight : acrossRight) + 2;
        if (first < x0) first = x0;
        if (end > x1) end = x1;
        uint32_t *row = fb + yy * pitch;
        for (int xx = first; xx < end; xx++) {
            float along = weapon_design_unalong(column[xx - x0].along + alongY, anchor, design);
            float v = w->gripY - along / scale;
            if (v < 0 || v >= s->h) continue;
            float blend = weapon_design_blend(along);
            float across = column[xx - x0].across + acrossY;
            float cross = (across - design.sweep * blend * blend * face) / (1 + (design.width - 1) * blend);
            float u = w->gripX + cross / (scale * parity);
            if (u < 0 || u >= s->w) continue;
            int sx = u, sy = v, lo = s->rows[sy * 3 + 1];
            if (sx < lo || sx >= s->rows[sy * 3 + 2]) continue;
            rgba_over(row + xx, palette[s->pixels[s->rows[sy * 3] + sx - lo]]);
        }
    }
    weapon_fitting(item,x,y,sc,angle,face,li,tip);
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
static void painted_arm(const Actor *a, int i, const float shoulder[2], const float wrist[2],
                        int sc, int face, int race, uint32_t skin, uint32_t cloth, float li, int fist, int relaxed) {
    int limb = LARM + i;
    if (limb_missing(a, limb))
        return;
    float sx = shoulder[0], sy = shoulder[1], hx = wrist[0], hy = wrist[1];
    float dx = hx - sx, dy = hy - sy, d = hypotf(dx, dy);
    float bend = sqrtf(fmaxf(0, 49 * sc * sc - d * d * .25f));
    if (relaxed) bend *= .25f;
    int elbowSide = (fist || i ? 1 : -1) * face;
    float ex = (sx + hx) * .5f - (d > .01f ? dy / d : 0) * bend * elbowSide;
    float ey = (sy + hy) * .5f + (d > .01f ? dx / d : 1) * bend * elbowSide;
    int grade = limb_item_grade(limb_installed(a, limb)), mech = race == 2 || grade >= 0;
    int torso = a->gear[1], flip = i ^ (face < 0);
    uint32_t tint = grade >= 0 ? limbGrades[grade].color : cloth;
    part(mech ? 76 : 72, sx, sy, ex, ey, 4 * sc, !mech && (!torso || torso == RAGS) ? skin : tint, li, 0, flip, 0, 1);
    part(mech ? 77 : 75, ex, ey, hx, hy, 3 * sc, mech ? tint : skin, li, 0, flip, 0, 1);
    if (fist) {
        /* Rotate the closed fist with its forearm so it reads during contact. */
        float length = fmaxf(1, hypotf(hx - ex, hy - ey));
        float ux = (hx - ex) / length * sc, uy = (hy - ey) / length * sc;
        part(78, hx - ux, hy - uy, hx + ux * 2.5f, hy + uy * 2.5f, 3.5f * sc,
             mech ? tint : skin, li, 0, flip, 0, 1);
    }
}
static float blade_clearance(int weapon, float angle, float height) {
    float down = sinf(angle);
    if (down <= 0)
        return angle;
    WeaponDesign design = weapon_design(weapon);
    const WeaponArt *art = &weaponArt[weapon_family(weapon)];
    float length = weapon_rapier(weapon) ? 24 * design.length
        : weapon_design_along(art->gripY * art->scale, weapon_design_anchor(weapon), design);
    /* Leave room for the painted edge and for the grip solver to settle. */
    float limit = fmaxf(0, height - 3) / length;
    if (down <= limit)
        return angle;
    float pitch = asinf(limit);
    return cosf(angle) >= 0 ? pitch : 3.14159265f - pitch;
}
/* A small shaded cap adds the curve and grain visible in the creator. */
static void hair_patch(float cx, float cy, float rx, float ry, uint32_t tint, float li) {
    int x0 = (int)floorf(cx - rx), x1 = (int)ceilf(cx + rx);
    int y0 = (int)floorf(cy - ry), y1 = (int)ceilf(cy + ry);
    track_paint((PaintBounds){x0, y0, x1, y1});
    uint32_t shades[] = {shade(tint, li * .70f), shade(tint, li), shade(tint, li * 1.28f)};
    for (int y = y0; y < y1; y++) {
        float v = (y + .5f - cy) / ry;
        for (int x = x0; x < x1; x++) {
            float u = (x + .5f - cx) / rx;
            float edge = u * u + v * v;
            if (edge <= 1) {
                int fleck = ((x - x0) * 7 + (y - y0) * 11) % 13;
                px(x, y, shades[edge > .79f || v > .38f ? 0 : fleck < 4 && v < .2f ? 2 : 1]);
            }
        }
    }
}
/* Head coordinates are relative to the painted scalp so the overlays travel
   with the face through a walk, attack or crawl. */
static void painted_head(const Actor *a, float x, float top, float endX, float bottom,
                         float width, uint32_t skin, float li, int face) {
    float half = width * .5f, height = bottom - top;
    int style = a->race == 2 ? 0 : a->hair, beard = a->race == 2 ? 0 : a->beard;
    uint32_t hair = shade(paintHair, li), dark = shade(paintHair, li * .67f),
             sheen = shade(blend(paintHair, 0xb39a7d, .22f), li);
#define HX(u, v) ((int)roundf(x + (endX - x) * (v) + face * half * (u)))
#define HY(v) ((int)roundf(top + height * (v)))
#define HAIR(u, v, p, q, r, s, t, w, c) \
    painted_quad(HX(u, v), HY(v), HX(p, q), HY(q), HX(r, s), HY(s), HX(t, w), HY(w), c)
    if (style == 4) {
        HAIR(-.9f,.19f, -.36f,.14f, -.42f,1.09f, -1.22f,1.17f, dark);
        HAIR(.58f,.19f, .87f,.26f, .95f,1.02f, .37f,1.11f, dark);
    } else if (style == 6) {
        HAIR(-.70f,.17f, -.94f,.15f, -1.35f,.55f, -1.71f,1.08f, dark);
        HAIR(-.98f,.31f, -1.23f,.46f, -1.65f,.85f, -1.71f,1.09f, hair);
    }
    part(61, x, top, endX, bottom, width, skin, li, 0, face < 0, 0, 1);
    if (style == 5) {
        hair_patch(HX(.02f,.01f), HY(.01f), half * .41f, height * .24f, paintHair, li);
    } else if (style) {
        float crown = style == 1 ? .17f : .11f;
        hair_patch(HX(-.05f,crown), HY(crown), half * .95f, height * (style == 1 ? .16f : .24f),
                   paintHair, li);
        if (style == 2)
            HAIR(-.57f,.14f, .25f,.03f, .89f,.23f, .73f,.38f, hair);
        if (style == 3) {
            hair_patch(HX(-.64f,-.16f), HY(-.16f), half * .38f, height * .18f, paintHair, li);
            painted_line(HX(-.72f,-.03f), HY(-.03f), HX(-.38f,.02f), HY(.02f), sheen);
        }
    }
    if (beard == 1) {
        uint32_t stubble = shade(blend(paintHair, skin, .52f), li);
        painted_line(HX(.11f,.69f), HY(.69f), HX(.62f,.79f), HY(.79f), stubble);
        painted_line(HX(.08f,.86f), HY(.86f), HX(.52f,.87f), HY(.87f), stubble);
    } else if (beard == 2) {
        HAIR(.16f,.72f, .70f,.73f, .51f,.97f, .22f,.93f, dark);
    } else if (beard == 3) {
        HAIR(-.49f,.51f, -.19f,.66f, .71f,.68f, .81f,.81f, dark);
        HAIR(-.49f,.51f, .81f,.81f, .40f,1.02f, -.35f,.87f, hair);
        painted_line(HX(-.21f,.73f), HY(.73f), HX(.41f,.90f), HY(.90f), sheen);
    }
    if (a->race == 2) {
        painted_line(HX(-.39f,.82f), HY(.82f), HX(.39f,.82f), HY(.82f), shade(0x343d3c, li));
    }
    if (a->race == 3) {
        uint32_t rim = shade(skin, li * .84f);
        HAIR(-.65f,.43f, -1.50f,.27f, -1.11f,.49f, -.69f,.57f, rim);
        HAIR(.66f,.45f, 1.44f,.29f, 1.14f,.49f, .75f,.57f, rim);
    }
    if (a->race == 4) {
        uint32_t stone = shade(skin, li * 1.08f);
        HAIR(-.67f,.54f, -1.10f,.29f, -.95f,-.16f, -.61f,.30f, stone);
        HAIR(.62f,.55f, 1.13f,.31f, .94f,-.19f, .61f,.30f, stone);
    }
#undef HAIR
#undef HY
#undef HX
}
typedef struct { float stride, lift; } WalkFoot;
static WalkFoot walk_foot(float phase) {
    const float plant = .58f;
    if (phase < plant)
        return (WalkFoot){1 - phase * 2 / plant, 0};
    float swing = (phase - plant) / (1 - plant);
    return (WalkFoot){-1 + 2 * pose_ease(swing), sinf(swing * 3.14159265f)};
}
static uint32_t actor_skin(const Actor *a) {
    static const uint32_t skins[] = {0xe0b78a, 0xc39971, 0xad7d58, 0x956645,
                                     0x744c36, 0x553d30, 0xc5b69b, 0x817a68};
    uint32_t skin = skins[a->skin % 8];
    int race=species_index(a);
    static const uint32_t ashSkin[]={0xa9a6a0,0x90877d,0x776f69,0x62615e,0x514e49,0x454446,0xc0b6a6,0x8b8580};
    static const uint32_t reedSkin[]={0x9eaf7c,0x879967,0x6f8058,0x667756,0x536346,0x42513a,0xb3b58b,0x8b956c};
    static const uint32_t stoneSkin[]={0xb3ad9d,0x9a9385,0x817b70,0x77766e,0x67655d,0x555952,0xc0bdad,0x8e9089};
    if(race==1)skin=ashSkin[a->skin%8];
    if(race==2)skin=stoneSkin[a->skin%8];
    if(race==3)skin=reedSkin[a->skin%8];
    if(race==4)skin=stoneSkin[a->skin%8];
    return skin;
}
static uint32_t actor_hair(const Actor *a) {
    static const uint32_t hairs[] = {0x211e1b, 0x433026, 0x776047, 0xa4835c, 0xc1b5a0, 0x713e32};
    return hairs[a->hairColor % 6];
}
static void actor_painted_stance(Actor *a, int x, int y, int sc, float li, float stance, int variant, int laid_out) {
    uint32_t skin = actor_skin(a);
    int race = species_index(a);
    paintHair = actor_hair(a);
    int torso = a->gear[1], head = a->gear[2], legs = a->gear[4], back = a->gear[5], boots = a->gear[3];
    int tf = gear_family(torso), hf = gear_family(head), lf = gear_family(legs), bf = gear_family(back),
        ff = gear_family(boots);
    if (tf < 32 || tf > 47)
        tf = torso == DUSTER ? 32 : torso == PLATE ? 38 : torso == LEATHER ? 35 : (torso == RAGS || torso == SLAVE_GARB) ? 46 : torso ? 33 : 72;
    if (hf < 48 || hf > 59)
        hf = head == STRAW ? 48 : head == HELM ? 50 : head == HAT ? 49 : -1;
    if (lf < 68 || lf > 75)
        lf = 69;
    if (ff < 60 || ff > 67)
        ff = boots == SANDALS ? 61 : 60;
    int face = a->facing ? 1 : -1, missing = a->lostMask & ~a->prostheticMask;
    float progress = a->attackDuration > 0 ? fmaxf(0, fminf(1, 1 - a->attackClock / a->attackDuration)) : 0;
    float block = a->impactKind == 2 && a->hitFlash > 0 ? pose_ease(a->hitFlash / .27f) : 0;
    int stowed = a->squad && !a->autoattack && !a->attackPhase && block <= 0 ? a->gear[0] : 0;
    int weapon = a->attackPhase ? a->attackWeapon : stowed ? 0 : a->gear[0];
    int relaxed = !a->attackPhase && block <= 0 &&
                  ((a->squad && !a->autoattack) || (!weapon && stance < .2f));
    if (relaxed)
        stance = 0;
    const CombatProfile *profile = combat_profile(weapon);
    CombatPose rig = combat_pose(weapon, a->attackPhase, progress, variant);
    float blend=a->attackPhase>=2?1:a->attackPhase==1?fmaxf(stance,pose_ease(progress)):stance;
    rig=pose_mix(profile->idle,rig,blend);
    if (block > 0) {
        CombatPose parry = profile->guard;
        parry.handX = 7; parry.handY = -26;
        parry.rearX = 4; parry.rearY = -27;
        parry.blade = -1.18f; parry.crouch = 3.1f;
        parry.shift = -1.5f; parry.step = 2;
        rig = pose_mix(rig, parry, block);
    }
    float aim = a->attackPhase ? a->attackAngle : (face > 0 ? 0 : 3.14159265f), forward = cosf(aim),
          depth = sinf(aim);
    int arms = limb_arms(a), leadLeg = face > 0 ? 1 : 0;
    float kick = !weapon && !arms && a->attackPhase
        ? fmaxf(0, fmaxf(rig.handX, rig.rearX) - profile->guard.handX) : 0;
    float plantLift = 0;
    if (a->attackPhase == 2 && progress < STRIKE_CONTACT) {
        const CombatMove *move = &profile->moves[variant];
        plantLift = fmaxf(0, move->hit.step - move->load.step) * .4f *
            sinf(3.14159265f * progress / STRIKE_CONTACT);
    }
    float motion = fminf(1, a->movedTimer / .12f);
    float walk = a->attackPhase ? 0 : motion * (1 - .22f * blend);
    float walkX, walkY;
    walk_heading(a, &walkX, &walkY);
    float phase = a->anim / 6.2831853f;
    WalkFoot feet[2] = {walk_foot(phase), walk_foot(fmodf(phase + .5f, 1))};
    int limping = limb_limping(a);
    int weakLeg = a->health[LLEG] / limb_max_health(a, LLEG) <
                  a->health[RLEG] / limb_max_health(a, RLEG) ? 0 : 1;
    float lean = rig.shift * forward + walk * walkX * .75f;
    if (limping)
        lean += walk * (weakLeg ? -1 : 1) * (1.2f + feet[weakLeg].lift * 1.8f);
    if (a->hitFlash > 0 && a->impactKind == 1)
        lean += (a->recoilX - a->recoilY) * 3 * pose_ease(a->hitFlash / .38f);
    float crouch = rig.crouch + (a->sneaking ? 3 : 0) + walk * .65f * (1 + cosf(a->anim * 2));
    float bw = a->body == 1 ? 5 : a->body == 2 ? 3.8f : 4.4f;
    if(!torso||torso==RAGS)bw*=.82f;else if(itemDefs[torso].armor>=18)bw*=1.12f;
    bw*=race==3?.85f:race==4?1.18f:1;
#define RX(xx, yy) (x + ((xx) * (1 + rig.twist) + lean * fminf(1, fmaxf(0, -(yy) / 24.f))) * sc)
#define RY(yy) (y + ((yy) + crouch * fminf(1, fmaxf(0, -(yy) / 13.f))) * sc)
#define PART(id, xx, yy, xx1, yy1, w, tint, q)                                                               \
    part(id, RX(xx, yy), RY(yy), RX(xx1, yy1), RY(yy1), (w) * sc, tint, li, q, face < 0, 0, 1)
    uint32_t cloth = torso ? itemDefs[torso].color : 0x9b8670, lc = legs ? itemDefs[legs].color : 0x786d5e;
    if (a->dead || a->unconscious > 0 || a->taskKind == 3 || playing_dead(a)) {
        /* Rotate the complete dressed rig: hats, boots and packs stay attached. */
        static uint32_t prone[128*120];
        Actor standing=*a;standing.dead=0;standing.unconscious=0;standing.taskKind=0;
        standing.attackPhase=0;standing.movedTimer=0;standing.hitFlash=0;standing.sneaking=0;
        standing.gear[0] = 0;
        standing.carrying = -1;
        int in_bed = actor_in_bed(a);
        if (in_bed) {
            standing.gear[2] = standing.gear[5] = 0;
            standing.facing = 1;
            standing.health[LLEG] = standing.health[RLEG] = species_health(a);
        }
        uint32_t *oldfb=fb;int oldpitch=pitch,l=clipLeft,t=clipTop,r=clipRight,b=clipBottom;
        PaintBounds *oldPaint = trackedPaint, pronePaint = {128, 120, 0, 0};
        memset(prone,0,sizeof(prone));fb=prone;pitch=128;clipLeft=clipTop=0;clipRight=128;clipBottom=120;
        trackedPaint = &pronePaint;
        actor_painted_stance(&standing,64,104,1,li,0,0,1);
        trackedPaint = oldPaint;
        fb=oldfb;pitch=oldpitch;clipLeft=l;clipTop=t;clipRight=r;clipBottom=b;
        if (pronePaint.left >= pronePaint.right || pronePaint.top >= pronePaint.bottom)
            return;
        /* Project the relaxed figure onto the ground. Sample backward so
           adjacent shoulders and limbs stay distinct at the gameplay scale. */
        float ax = in_bed ? -.84f : .91f * face, ay = in_bed ? -.42f : -.31f;
        float bx = in_bed ? .29f : .25f * face, by = in_bed ? -.60f : .74f;
        float det = ax * by - ay * bx;
        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (int corner = 0; corner < 4; corner++) {
            float along = 84 - (corner & 1 ? pronePaint.top : pronePaint.bottom);
            float across = (corner & 2 ? pronePaint.left : pronePaint.right) - 64;
            float px = x + sc * (ax * along + bx * across);
            float py = y - 3 * sc + sc * (ay * along + by * across);
            minX = fminf(minX, px); maxX = fmaxf(maxX, px);
            minY = fminf(minY, py); maxY = fmaxf(maxY, py);
        }
        PaintBounds ground = clip_paint((PaintBounds){floorf(minX), floorf(minY), ceilf(maxX), ceilf(maxY)},
                                        clipLeft, clipTop, clipRight, clipBottom);
        track_paint(ground);
        for (int py = ground.top; py < ground.bottom; py++)
            for (int px = ground.left; px < ground.right; px++) {
                float dx = (px + .5f - x) / sc, dy = (py + .5f - y) / sc + 3;
                int sx = (int)floorf(64 + (ax * dy - ay * dx) / det);
                int sy = (int)floorf(84 - (by * dx - bx * dy) / det);
                if (sx < pronePaint.left || sx >= pronePaint.right || sy < pronePaint.top || sy >= pronePaint.bottom)
                    continue;
                uint32_t pixel = prone[sy * 128 + sx];
                if (pixel) fb[py * pitch + px] = pixel;
            }
        return;
    }
    if (a->kind == BEAST) {
        int species = (a->skin % 4) * 2 + (a->attackPhase == 2);
        float bob = fabsf(sinf(a->anim * 2)) * 1.3f * motion;
        PART(96 + species, 0, -22 - bob, 0, 0, 33, 0xb4a38e, 0);
        return;
    }
    if(!laid_out && limb_crawling(a)) {
        float cycle=a->movedTimer>0?sinf(a->anim*2):0;
        /* Horizontal torso, dragging legs, and alternating grounded hands. */
        for(int i=0;i<2;i++){
            int l=LLEG+i;if(limb_missing(a,l))continue;int it=limb_installed(a,l),grade=limb_item_grade(it),mech=race==2||it;
            uint32_t tint=grade>=0?limbGrades[grade].color:lc;
            float yy=-3+(i?1:-1),end=-face*(17+(i?cycle:-cycle));
            part(mech?76:64+lf-68,x-face*5*sc,y-5*sc,x-face*11*sc,y+yy*sc,4.5f*sc,tint,li,0,face<0,0,mech?1:.54f);
            part(mech?77:64+lf-68,x-face*11*sc,y+yy*sc,x+end*sc,y-1*sc,3.8f*sc,tint,li,0,face<0,mech?0:.48f,1);
        }
        if(back){int bid=bf>=76&&bf<=83?88+bf-76:88;part(bid,x-face*4*sc,y-12*sc,x+face*4*sc,y-14*sc,9*sc,itemDefs[back].color,li,itemDefs[back].quality,face>0,0,1);}
        part(tf,x+face*5*sc,y-10*sc,x-face*7*sc,y-5*sc,bw*2.2f*sc,cloth,li,itemDefs[torso].quality,face<0,0,1);
        float handx=x+face*(13+cycle*2)*sc,handy=y-1*sc;
        int primary=limb_missing(a,RARM)?0:1;
        for(int i=0;i<2;i++){
            int l=LARM+i;if(limb_missing(a,l))continue;int grade=limb_item_grade(limb_installed(a,l)),mech=race==2||grade>=0;uint32_t tint=grade>=0?limbGrades[grade].color:cloth;
            float xx=x+face*(11+(i?cycle:-cycle)*3)*sc,yy=y-(i?1:3)*sc;
            part(mech?76:72,x+face*4*sc,y-10*sc,x+face*7*sc,y-4*sc,3.8f*sc,tint,li,0,i^(face<0),0,1);
            part(mech?77:75,x+face*7*sc,y-4*sc,xx,yy,3*sc,mech?tint:skin,li,0,i^(face<0),0,1);
            if(i==primary){handx=xx;handy=yy;}
        }
        painted_head(a, x + face * 9 * sc, y - 17 * sc, x + face * 8 * sc, y - 9 * sc,
                     (race == 4 ? 6.7f : 5.8f) * sc, skin, li, face);
        if(head&&hf>=48){float wide,top,bottom;head_attachment(hf,&wide,&top,&bottom);part(hf,x+face*9*sc,y+(top+16)*sc,x+face*9*sc,y+(bottom+16)*sc,wide*sc,itemDefs[head].color,li,itemDefs[head].quality,face<0,0,1);}
        if(limb_arms(a)&&weapon)weapon_sprite(weapon,handx,handy,sc,face>0?-.18f:3.32f,face,li);
        return;
    }
    if (stowed)
        weapon_sprite(stowed, RX(-face * 6, -10), RY(-10), sc, -1.5707963f, face, li * .74f);
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
        float xx = i ? 2 : -2.5f;
        float stride = feet[i].stride * walk * (a->sneaking ? 5.3f : 7.3f) *
                       (limping && weakLeg == i ? .28f : 1);
        float groundY = stride * walkY * .5f;
        float step = i == leadLeg ? rig.step + kick * .72f : weapon ? 0 : -1.4f * blend;
        float foot = xx + step * forward + stride * walkX;
        float lift = feet[i].lift * walk * (limping && weakLeg == i ? .45f : 2.7f);
        float knee = xx + (step * .55f + rig.shift * .40f) * forward +
                     stride * walkX * .50f + lift * walkX * .48f;
        float kneeY = -7 + groundY * .52f - lift * .52f;
        if (i == leadLeg)
            lift += plantLift + kick * .85f;
        float ankle = groundY - lift;
        int grade=limb_item_grade(limb_installed(a,limb));
        uint32_t limbTint=grade>=0?limbGrades[grade].color:lc;
        int mech = race==2 || (a->prostheticMask & (1 << limb)), id = mech ? 76 : 64 + lf - 68;
        part(id, RX(xx, -14), RY(-14), RX(knee, kneeY), RY(kneeY), 4.8f * sc, limbTint, li, itemDefs[legs].quality,
             face < 0, 0, mech ? 1 : .54f);
        /* The boot anchors are in ground space, outside torso lean and twist. */
        part(mech ? 77 : id, RX(knee, kneeY - .35f), RY(kneeY - .35f),
             x + foot * sc, y + (ankle - 2) * sc, 4.1f * sc, limbTint,
             li, itemDefs[legs].quality, face < 0, mech ? 0 : .48f, 1);
        part(mech ? 87 : 80 + ff - 60, x + foot * sc, y + (ankle - 5) * sc,
             x + (foot + face * .6f) * sc, y + (ankle - .2f) * sc, 5.4f * sc,
             boots ? itemDefs[boots].color : skin, li, itemDefs[boots].quality, face < 0, 0, 1);
    }
    int ranged = weapon_ranged(weapon);
    float angle = (ranged ? aim : (face > 0 ? 0 : 3.14159265f) + face * .42f * depth) + face * rig.blade;
    float handx = forward * rig.handX - feet[limb_missing(a, RARM) ? 0 : 1].stride * walkX * walk * .65f,
          handy = rig.handY + depth * rig.handX * .4f;
    if (weapon && !ranged)
        angle = blade_clearance(weapon, angle, (y - RY(handy)) / sc);
    WeaponGrip grip = weapon_grip(RX(handx, handy), RY(handy), angle, sc);
    float wrists[2][2], shoulders[2][2];
    int primary=limb_missing(a,RARM)?0:1;
    for(int i=0;i<2;i++){
        float roll = weapon ? 0 : fmaxf(0, -rig.twist) * 9;
        shoulders[i][0]=RX((i ? bw-1-roll : -bw+1+roll) * face,-24);
        shoulders[i][1]=RY(-24);
    }
    float support=weapon_support_distance(weapon);
    if(weapon_style(weapon)==WS_BOW){
        float draw=a->attackPhase==1?pose_ease(progress):a->attackPhase==2?1-pose_ease((progress-STRIKE_CONTACT)/(1-STRIKE_CONTACT)):0;
        support=-3+(support+3)*draw;
    }
    grip=reachable_grip(grip,shoulders[primary][0],shoulders[primary][1],
        shoulders[1-primary][0],shoulders[1-primary][1],support,
        combat_two_hands(weapon)&&arms==2,13.5f*sc);
    int twoHands = combat_two_hands(weapon);
    int rearPunch = !weapon && a->attackPhase && variant == UNARMED_CROSS;
    for (int i = 0; i < 2; i++) {
        if (relaxed || (weapon && !twoHands && i != primary)) {
            float swing = -feet[i].stride * walk * (1 - .65f * blend);
            float out = (i ? face : -face) * (weapon ? .95f : 1.2f);
            wrists[i][0] = shoulders[i][0] + (out + swing * walkX * 1.8f) * sc;
            wrists[i][1] = shoulders[i][1] + (13.1f + swing * walkY * .7f) * sc;
        } else if (!weapon) {
            float hx = i == primary ? rig.handX : rig.rearX;
            float hy = i == primary ? rig.handY : rig.rearY;
            if (i == primary && arms == 1 && rearPunch) {
                /* A single arm can perform the cross from its own guard. */
                CombatPose guard = pose_mix(profile->idle, profile->guard, blend);
                hx = rig.rearX + guard.handX - guard.rearX;
                hy = rig.rearY + guard.handY - guard.rearY;
            }
            float reachX = face * (.60f + .40f * fabsf(forward)) * hx;
            float reachY = hy + depth * hx * .36f;
            reachX -= feet[i].stride * walkX * walk * (1 - blend) * 3.1f;
            reachY -= feet[i].stride * walkY * walk * (1 - blend) * 1.25f;
            WeaponGrip fist = { .x = RX(reachX, reachY), .y = RY(reachY) };
            fist = reachable_grip(fist, shoulders[i][0], shoulders[i][1], 0, 0, 0, 0, 13.5f * sc);
            wrists[i][0] = fist.x;
            wrists[i][1] = fist.y;
        } else {
            float along = i != primary ? support : 0;
            wrists[i][0] = grip.x + grip.ux * along;
            wrists[i][1] = grip.y + grip.uy * along;
        }
    }
    if (relaxed || (weapon && !twoHands))
        painted_arm(a, 1 - primary, shoulders[1 - primary], wrists[1 - primary], sc, face, race, skin, cloth, li, 0, 1);
    int coat = tf == 32 || tf == 33 || tf == 39 || tf == 40 || tf == 42 || tf == 44 || tf == 47;
    // Join the chin to the collar; transparent face/collar edges otherwise
    // leave a floating head, most visible with the smaller fitted hats.
    PART(72, 0, -27.5f, 0, -24, 2.8f, skin, 0);
    PART(tf, 0, -26, coat ? -rig.shift * .35f * forward : 0, coat ? -5 : -12, bw * (torso?2.8f:1.8f), torso?cloth:skin,
         itemDefs[torso].quality);
    painted_head(a, RX(0, -33), RY(-33), RX(0, -25), RY(-25),
                 (race == 4 ? 6.7f : race == 3 ? 5.1f : 5.8f) * sc, skin, li, face);
    if (a->scar)
        painted_line(RX(face, -29), RY(-29), RX(face * 1.5f, -27), RY(-27), shade(0x874b38, li));
    if (head && hf >= 48) {
        float wide, top, bottom;
        head_attachment(hf, &wide, &top, &bottom);
        PART(hf, 0, top, 0, bottom, wide, itemDefs[head].color, itemDefs[head].quality);
    }
    if (!relaxed && (!weapon || twoHands))
        painted_arm(a, 1 - primary, shoulders[1 - primary], wrists[1 - primary], sc, face, race, skin, cloth, li, !weapon, 0);
    painted_arm(a, primary, shoulders[primary], wrists[primary], sc, face, race, skin, cloth, li, !weapon && !relaxed, relaxed);
    if (weapon && arms) {
        weapon_sprite(weapon, grip.x, grip.y, sc, angle, face, li);
        for (int i = 0; i < 2; i++) {
            if ((missing & (1 << (LARM + i))) || (!twoHands && i != primary))
                continue;
            float hx = wrists[i][0], hy = wrists[i][1];
            int grade = limb_item_grade(limb_installed(a, LARM + i));
            uint32_t tint = grade >= 0 ? limbGrades[grade].color : skin;
            part(78, hx, hy - 1.6f * sc, hx, hy + 1.6f * sc, 2.8f * sc, tint, li, 0, i ^ (face < 0), 0, 1);
        }
    }
    if (a->carrying >= 0 && a->carrying < g.actorCount) {
        const Actor *carried = &g.actors[a->carrying];
        uint32_t tint = carried->gear[1] ? itemDefs[carried->gear[1]].color : actor_skin(carried);
        float sway = walk * sinf(a->anim) * 1.7f;
        part(33, x - face * 10 * sc, y - 23 * sc, x + face * 8 * sc, y - 24 * sc,
             8 * sc, tint, li, 0, 0, 0, 1);
        part(61, x + face * 11 * sc, y - (28 + sway) * sc, x + face * 12 * sc, y - (22 + sway) * sc,
             5.4f * sc, actor_skin(carried), li, 0, face < 0, 0, 1);
        part(67, x - face * 10 * sc, y - 23 * sc, x - face * 17 * sc, y - (15 - sway) * sc,
             4.5f * sc, 0x6b6253, li, 0, face < 0, 0, 1);
    }
#undef PART
#undef RX
#undef RY
}
/* Direct previews use the same pose resolver as the cached world renderer. */
static inline void actor_painted(Actor *a,int x,int y,int sc,float li){
    actor_painted_stance(a,x,y,sc,li,combat_stance(a),combat_attack_variant(a),0);
}
#define ACT_W 128
#define ACT_H 120
#define ACT_ANCHOR 152
#define ACT_CACHE 24
#define ACT_SCRATCH_STRIDE (ACT_W * 2)
static uint32_t actorScratch[ACT_SCRATCH_STRIDE * ACT_H * 2];
static PaintBounds actorScratchBounds;
typedef struct {
    uint64_t key;
    PaintBounds bounds;
    uint32_t pixels[ACT_W * ACT_H];
} ActorSprite;
static ActorSprite actorCache[ACT_CACHE];
static unsigned actorNext;

static void clear_paint(uint32_t *pixels, int stride, PaintBounds b) {
    if (b.left >= b.right)
        return;
    for (int y = b.top; y < b.bottom; y++)
        memset(pixels + y * stride + b.left, 0, (b.right - b.left) * sizeof(*pixels));
}
static void paint_actor_scratch(Actor *a, float li, float stance, int variant) {
    uint32_t *oldfb = fb;
    int oldpitch = pitch;
    PaintBounds oldClip = {clipLeft, clipTop, clipRight, clipBottom}, *oldPaint = trackedPaint;
    clear_paint(actorScratch, ACT_SCRATCH_STRIDE, actorScratchBounds);
    actorScratchBounds = (PaintBounds){ACT_SCRATCH_STRIDE, ACT_H * 2, 0, 0};
    trackedPaint = &actorScratchBounds;
    fb = actorScratch;
    pitch = ACT_SCRATCH_STRIDE;
    clipLeft = clipTop = 0;
    clipRight = ACT_SCRATCH_STRIDE;
    clipBottom = ACT_H * 2;
    actor_painted_stance(a, ACT_W, ACT_ANCHOR, 2, li, stance, variant, 0);
    fb = oldfb;
    pitch = oldpitch;
    clipLeft = oldClip.left;
    clipTop = oldClip.top;
    clipRight = oldClip.right;
    clipBottom = oldClip.bottom;
    trackedPaint = oldPaint;
}
static void paint_cached_actor(ActorSprite *sprite, Actor *a, float li, float stance, int variant) {
    paint_actor_scratch(a, li, stance, variant);
    clear_paint(sprite->pixels, ACT_W, sprite->bounds);
    sprite->bounds = (PaintBounds){ACT_W, ACT_H, 0, 0};
    PaintBounds b = actorScratchBounds;
    /* Round outward to full 2x2 filter samples, including transparent pixels. */
    for (int y = b.top / 2; y < (b.bottom + 1) / 2; y++) {
        for (int x = b.left / 2; x < (b.right + 1) / 2; x++) {
            const uint32_t *p = actorScratch + y * 2 * ACT_SCRATCH_STRIDE + x * 2;
            const uint32_t samples[] = {p[0], p[1], p[ACT_SCRATCH_STRIDE], p[ACT_SCRATCH_STRIDE + 1]};
            unsigned alpha = (samples[0] >> 24) + (samples[1] >> 24) +
                             (samples[2] >> 24) + (samples[3] >> 24);
            if (alpha < 4)
                continue;
            uint32_t result;
            if (alpha == 4 * 255) {
                /* An opaque 2x2 patch needs only an ordinary channel average. */
                unsigned redBlue = 0, green = 0;
                for (int i = 0; i < 4; i++) {
                    redBlue += samples[i] & 0xff00ff;
                    green += samples[i] & 0xff00;
                }
                result = 0xff000000 | (redBlue >> 2 & 0xff00ff) | (green >> 2 & 0xff00);
            } else {
                unsigned red = 0, green = 0, blue = 0;
                for (int i = 0; i < 4; i++) {
                    uint32_t color = samples[i];
                    unsigned a0 = color >> 24;
                    red += (color & 255) * a0;
                    green += (color >> 8 & 255) * a0;
                    blue += (color >> 16 & 255) * a0;
                }
                uint32_t reciprocal = alpha_reciprocal(alpha);
                result = (alpha / 4 << 24) | alpha_average(red, alpha, reciprocal) |
                         (alpha_average(green, alpha, reciprocal) << 8) |
                         (alpha_average(blue, alpha, reciprocal) << 16);
            }
            sprite->pixels[y * ACT_W + x] = result;
            include_paint(&sprite->bounds, (PaintBounds){x, y, x + 1, y + 1});
        }
    }
}
static uint64_t actor_key(const Actor *a, int lum, float stance, int variant) {
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
    HASHV(a->squad && !a->autoattack);
    HASHV(a->dead);
    HASHV(a->unconscious > 0);
    HASHV(playing_dead(a));
    HASHV(a->taskKind == 3);
    HASHV(actor_in_bed(a));
    HASHV(a->kind);
    HASHV(a->attackPhase);
    HASHV(a->attackWeapon);
    HASHV(variant);
    HASHV((int)(stance * 32));
    HASHV((int)(a->attackAngle * 64));
    HASHV(a->attackDuration > 0 ? (int)((1 - a->attackClock / a->attackDuration) * 32) : 0);
    HASHV(a->movedTimer > 0 ? (int)(a->anim * 9) : 0);
    if (a->movedTimer > 0) {
        float walkX, walkY;
        walk_heading(a, &walkX, &walkY);
        HASHV((int)(a->movedTimer * 50));
        HASHV((int)(walkX * 8));
        HASHV((int)(walkY * 8));
    }
    HASHV(limb_crawling(a));
    HASHV(limb_limping(a) ? (a->health[LLEG] / limb_max_health(a, LLEG) <
                             a->health[RLEG] / limb_max_health(a, RLEG) ? 1 : 2) : 0);
    HASHV(a->carrying + 1);
    if (a->carrying >= 0 && a->carrying < g.actorCount) {
        const Actor *carried = &g.actors[a->carrying];
        HASHV(carried->gear[1]);
        HASHV(carried->skin);
        HASHV(carried->race);
    }
    HASHV((int)(a->hitFlash * 30));
    HASHV(a->impactKind);
    HASHV((int)(a->recoilX * 8));
    HASHV((int)(a->recoilY * 8));
    HASHV(lum);
#undef HASHV
    return k ? k : 1;
}
static void person(Actor *a, int x, int y, int sc, int chosen) {
    float stance=combat_stance(a);
    int variant=combat_attack_variant(a);
    if(sc==1 && (uintptr_t)a>=(uintptr_t)g.actors && (uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)) {
        int n=(int)(a-g.actors),rel=actor_relation(n);
        if(!a->squad&&!a->dead){uint32_t col=rel<0?RED:rel>0?TEAL:GOLD;
            if(rel<0){line(x-3,y+3,x,y+6,col);line(x,y+6,x+3,y+3,col);}
            else if(rel>0)line(x-3,y+4,x+3,y+4,col);else rect(x-1,y+3,2,2,col);
        }
        const Actor *controlled = &g.actors[g.selected];
        if (controlled->sneaking && !a->squad && conscious(a) &&
            depth_distance(controlled, a) < 8) {
            int seen = npc_sees(a, controlled, npc_sight_range(a));
            int eyeX = x + 14, eyeY = y - 15;
            rect(eyeX - 5, eyeY - 4, 11, 9, INK);
            uint32_t c = seen ? RED : MUTED;
            line(eyeX - 4, eyeY, eyeX, eyeY - 2, c);
            line(eyeX, eyeY - 2, eyeX + 4, eyeY, c);
            line(eyeX + 4, eyeY, eyeX, eyeY + 2, c);
            line(eyeX, eyeY + 2, eyeX - 4, eyeY, c);
            if (seen) rect(eyeX, eyeY - 1, 1, 3, c);
        }
        int target=g.living.command[g.selected]>=0?g.living.command[g.selected]:g.actors[g.selected].target;
        if(n==target&&!a->dead){int left=x-11,right=x+11,top=y-37,bottom=y+2;
            line(left,top,left+4,top,RED);line(left,top,left,top+4,RED);line(right-4,top,right,top,RED);line(right,top,right,top+4,RED);
            line(left,bottom,left+4,bottom,RED);line(left,bottom-4,left,bottom,RED);line(right-4,bottom,right,bottom,RED);line(right,bottom-4,right,bottom,RED);
        }
    }
    Actor dressed;
    if ((uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + g.actorCount)) {
        int layer = a->gear[1] ? NONE : g.depth.actors[a - g.actors].layer;
        if (layer || playing_dead(a)) {
            dressed = *a;
            if (layer) dressed.gear[1] = layer;
            if (playing_dead(a)) dressed.unconscious = .5f;
            a = &dressed;
        }
    }
    float li = sc == 1 ? fminf(1.15f, light() + (chosen ? .16f : .06f)) : 1;
    if (chosen && sc == 1) {
        line(x - 11, y + 2, x, y + 7, 0x101713);
        line(x, y + 7, x + 11, y + 2, 0x101713);
        line(x - 10, y + 1, x, y + 6, TEAL);
        line(x, y + 6, x + 10, y + 1, TEAL);
    }
    if (sc != 1) {
        actor_painted_stance(a, x, y, sc, li,stance,variant,0);
        return;
    }
    uint64_t key = actor_key(a, (int)(li * 64), stance, variant);
    int slot = -1;
    for (int i = 0; i < ACT_CACHE; i++)
        if (actorCache[i].key == key) {
            slot = i;
            break;
        }
    if (slot < 0) {
        slot = actorNext++ % ACT_CACHE;
        actorCache[slot].key = key;
        paint_cached_actor(&actorCache[slot], a, li, stance, variant);
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
    const ActorSprite *sprite = &actorCache[slot];
    int originX = x - ACT_W / 2, originY = y - ACT_ANCHOR / 2;
    PaintBounds body = clip_paint(sprite->bounds, clipLeft - originX, clipTop - originY,
                                  clipRight - originX, clipBottom - originY);
    for (int yy = body.top; yy < body.bottom; yy++)
        for (int xx = body.left; xx < body.right; xx++)
            rgba_over(fb + (originY + yy) * pitch + originX + xx, sprite->pixels[yy * ACT_W + xx]);
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
    paint_actor_scratch(a, 1, combat_stance(a), combat_attack_variant(a));
}
static float portrait_scale(Actor *a, int width, int height, int drawX, int drawY, float limit) {
    if (width > PORTRAIT_W || height > PORTRAIT_H || width < 1 || height < 1)
        return 1;
    uint64_t key = actor_key(a, 64, combat_stance(a), combat_attack_variant(a));
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
        for (int yy = actorScratchBounds.top; yy < actorScratchBounds.bottom; yy++)
            for (int xx = actorScratchBounds.left; xx < actorScratchBounds.right; xx++)
                if (actorScratch[yy * ACT_SCRATCH_STRIDE + xx] >> 24 >= 16) {
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
            x1 <= x0 || y1 <= y0 ? 1 : fminf((float)width / (x1 - x0), (float)height / (y1 - y0));
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
                    actorScratch[sy * ACT_SCRATCH_STRIDE + sx];
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
