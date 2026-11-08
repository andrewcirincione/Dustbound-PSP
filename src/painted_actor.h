/* Articulated painted parts share the simulation's combat pose and rigid grip.
   Only world sprites are composed at 2x and filtered. Portraits sample the same
   indexed art directly; no runtime decoder, allocation or per-outfit atlas. */
#include "weapon_design.h"
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
#define WL(a,b,c,e,col) line(WX(a,b),WY(a,b),WX(c,e),WY(c,e),col)
#define WQ(a,b,c,e,f,h,j,k,col) quad(WX(a,b),WY(a,b),WX(c,e),WY(c,e),WX(f,h),WY(f,h),WX(j,k),WY(j,k),col)
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
static void weapon_sprite(int item, float x, float y, int sc, float angle, int face, float li) {
    if (!item)return;
    WeaponDesign design=weapon_design(item);float ca=cosf(angle),sa=sinf(angle),anchor=weapon_design_anchor(item);
    if(weapon_rapier(item)){
        float len=24*design.length;uint32_t steel=shade(itemDefs[item].color,li);
        for(int i=0;i<(design.pattern==1?2:1)*sc;i++)line(x+ca*3*sc-sa*i,y+sa*3*sc+ca*i,x+ca*len*sc,y+sa*len*sc,steel);
        line(x,y,x-ca*4*sc,y-sa*4*sc,0x614a38);
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
    weapon_palette(id,&itemDefs[item],li);
    for(int yy=y0;yy<y1;yy++)for(int xx=x0;xx<x1;xx++){
        float dx=(xx+.5f-x)/sc,dy=(yy+.5f-y)/sc;
        float along=weapon_design_unalong(dx*ca+dy*sa,anchor,design),blend=weapon_design_blend(along);
        float cross=(-dx*sa+dy*ca-design.sweep*blend*blend*face)/(1+(design.width-1)*blend);
        float u=w->gripX+cross/(scale*parity),v=w->gripY-along/scale;
        if(u<0||v<0||u>=s->w||v>=s->h)continue;
        int sx=u,sy=v,lo=s->rows[sy*3+1];if(sx<lo||sx>=s->rows[sy*3+2])continue;
        rgba_over(fb+yy*pitch+xx,artPalette[id][s->pixels[s->rows[sy*3]+sx-lo]]);
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
static void actor_painted_stance(Actor *a, int x, int y, int sc, float li, float stance) {
    static const uint32_t skins[] = {0xe0b78a, 0xc39971, 0xad7d58, 0x956645,
                                     0x744c36, 0x553d30, 0xc5b69b, 0x817a68};
    static const uint32_t hairs[] = {0x211e1b, 0x433026, 0x776047, 0xa4835c, 0xc1b5a0, 0x713e32};
    uint32_t skin = skins[a->skin % 8];
    int race=species_index(a);
    static const uint32_t ashSkin[]={0xa9a6a0,0x90877d,0x776f69,0x62615e,0x514e49,0x454446,0xc0b6a6,0x8b8580};
    static const uint32_t reedSkin[]={0x9eaf7c,0x879967,0x6f8058,0x667756,0x536346,0x42513a,0xb3b58b,0x8b956c};
    static const uint32_t stoneSkin[]={0xb3ad9d,0x9a9385,0x817b70,0x77766e,0x67655d,0x555952,0xc0bdad,0x8e9089};
    if(race==1)skin=ashSkin[a->skin%8];
    if(race==2)skin=stoneSkin[a->skin%8];
    if(race==3)skin=reedSkin[a->skin%8];
    if(race==4)skin=stoneSkin[a->skin%8];
    paintHair = hairs[a->hairColor % 6];
    int torso = a->gear[1], head = a->gear[2], legs = a->gear[4], back = a->gear[5], boots = a->gear[3];
    int tf = gear_family(torso), hf = gear_family(head), lf = gear_family(legs), bf = gear_family(back),
        ff = gear_family(boots);
    if (tf < 32 || tf > 47)
        tf = torso == DUSTER ? 32 : torso == PLATE ? 38 : torso == LEATHER ? 35 : torso == RAGS ? 46 : torso ? 33 : 72;
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
    float blend=a->attackPhase>=2?1:a->attackPhase==1?fmaxf(stance,pose_ease(progress)):stance;
    rig=pose_mix(combat_idle(weapon),rig,blend);
    float aim = a->attackPhase ? a->attackAngle : (face > 0 ? 0 : 3.14159265f), forward = cosf(aim),
          depth = sinf(aim);
    float pose = a->movedTimer > 0 ? sinf(a->anim * 2) * 2.6f * fminf(1,a->movedTimer/.12f) : 0, lean = rig.shift * forward;
    if (a->hitFlash > 0 && a->impactKind == 1)
        lean += (a->recoilX - a->recoilY) * 3 * pose_ease(a->hitFlash / .38f);
    float crouch = rig.crouch + (a->sneaking ? 3 : 0), bw = a->body == 1 ? 5 : a->body == 2 ? 3.8f : 4.4f;
    if(!torso||torso==RAGS)bw*=.82f;else if(itemDefs[torso].armor>=18)bw*=1.12f;
    bw*=race==3?.85f:race==4?1.18f:1;
#define RX(xx, yy) (x + ((xx) * (1 + rig.twist) + lean * fminf(1, fmaxf(0, -(yy) / 24.f))) * sc)
#define RY(yy) (y + ((yy) + crouch * fminf(1, fmaxf(0, -(yy) / 13.f))) * sc)
#define PART(id, xx, yy, xx1, yy1, w, tint, q)                                                               \
    part(id, RX(xx, yy), RY(yy), RX(xx1, yy1), RY(yy1), (w) * sc, tint, li, q, face < 0, 0, 1)
    uint32_t cloth = torso ? itemDefs[torso].color : 0x9b8670, lc = legs ? itemDefs[legs].color : 0x786d5e;
    if (a->dead || a->unconscious > 0 || a->taskKind == 3) {
        /* Rotate the complete dressed rig: hats, boots and packs stay attached. */
        static uint32_t prone[128*120];
        Actor standing=*a;standing.dead=0;standing.unconscious=0;standing.taskKind=0;
        standing.attackPhase=0;standing.movedTimer=0;standing.hitFlash=0;standing.sneaking=0;
        uint32_t *oldfb=fb;int oldpitch=pitch,l=clipLeft,t=clipTop,r=clipRight,b=clipBottom;
        memset(prone,0,sizeof(prone));fb=prone;pitch=128;clipLeft=clipTop=0;clipRight=128;clipBottom=120;
        actor_painted_stance(&standing,64,104,1,li,0);
        fb=oldfb;pitch=oldpitch;clipLeft=l;clipTop=t;clipRight=r;clipBottom=b;
        for(int sy=0;sy<120;sy++)for(int sx=0;sx<128;sx++)if(prone[sy*128+sx]) {
            int dx=x+(84-sy)*sc,dy=y-3*sc+(sx-64)*sc/2;
            for(int yy=0;yy<sc;yy++)for(int xx=0;xx<sc;xx++)if(dx+xx>=clipLeft&&dx+xx<clipRight&&dy+yy>=clipTop&&dy+yy<clipBottom)fb[(dy+yy)*pitch+dx+xx]=prone[sy*128+sx];
        }
        return;
    }
    if (a->kind == BEAST) {
        int species = (a->skin % 4) * 2 + (a->attackPhase == 2);
        float bob = a->movedTimer > 0 ? fabsf(pose) * .5f : 0;
        PART(96 + species, 0, -22 - bob, 0, 0, 33, 0xb4a38e, 0);
        return;
    }
    if(limb_crawling(a)) {
        float cycle=a->movedTimer>0?sinf(a->anim*2.8f):0;
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
        int faceid=race==2||a->hair==0?61:a->hair==3?62:a->beard?63:60;
        part(faceid,x+face*9*sc,y-17*sc,x+face*8*sc,y-9*sc,(race==4?6.7f:5.8f)*sc,skin,li,0,face<0,0,1);
        if(head&&hf>=48){float wide,top,bottom;head_attachment(hf,&wide,&top,&bottom);part(hf,x+face*9*sc,y+(top+16)*sc,x+face*9*sc,y+(bottom+16)*sc,wide*sc,itemDefs[head].color,li,itemDefs[head].quality,face<0,0,1);}
        if(limb_arms(a)&&weapon)weapon_sprite(weapon,handx,handy,sc,face>0?-.18f:3.32f,face,li);
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
        float foot = xx + (i == (face>0?1:0) ? rig.step * forward : 0) + off * .65f,
              knee = xx + rig.shift * forward + off * .35f;
        float lift = a->attackPhase ? 0 : fmaxf(0, off) * .4f;
        int grade=limb_item_grade(limb_installed(a,limb));
        uint32_t limbTint=grade>=0?limbGrades[grade].color:lc;
        int mech = race==2 || (a->prostheticMask & (1 << limb)), id = mech ? 76 : 64 + lf - 68;
        part(id, RX(xx, -14), RY(-14), RX(knee, -7), RY(-7), 4.8f * sc, limbTint, li, itemDefs[legs].quality,
             face < 0, 0, mech ? 1 : .54f);
        part(mech ? 77 : id, RX(knee, -7.4f), RY(-7.4f), RX(foot, -2 - lift), RY(-2 - lift), 4.1f * sc, limbTint,
             li, itemDefs[legs].quality, face < 0, mech ? 0 : .48f, 1);
        PART(mech ? 87 : 80 + ff - 60, foot, -5 - lift, foot + face * .6f, -.2f - lift, 5.4f,
             boots ? itemDefs[boots].color : skin, itemDefs[boots].quality);
    }
    float angle = aim + face * rig.blade, handx = forward * rig.handX,
          handy = rig.handY + depth * rig.handX * .4f;
    WeaponGrip grip = weapon_grip(RX(handx, handy), RY(handy), angle, sc);
    float wrists[2][2], shoulders[2][2];
    int primary=limb_missing(a,RARM)?0:1;
    for(int i=0;i<2;i++){
        shoulders[i][0]=RX((i?bw-1:-bw+1)*face,-24);
        shoulders[i][1]=RY(-24);
    }
    float support=weapon_support_distance(weapon);
    if(combat_style(weapon)==4){
        float draw=a->attackPhase==1?pose_ease(progress):a->attackPhase==2?1-pose_ease((progress-STRIKE_CONTACT)/(1-STRIKE_CONTACT)):0;
        support=-3+(support+3)*draw;
    }
    grip=reachable_grip(grip,shoulders[primary][0],shoulders[primary][1],
        shoulders[1-primary][0],shoulders[1-primary][1],support,
        combat_two_hands(weapon)&&limb_arms(a)==2,13.5f*sc);
    // Arms behind the body; wrists and handle remain one rigid construction.
    for (int i = 0; i < 2; i++) {
        int limb = i ? RARM : LARM;
        float along=i!=primary?support:0;
        wrists[i][0]=grip.x+grip.ux*along;
        wrists[i][1]=grip.y+grip.uy*along;
        if (i!=primary && !combat_two_hands(weapon)) {
            wrists[i][0] = RX(-face * 5, -14 - 8*blend + rig.crouch);
            wrists[i][1] = RY(-14 - 8*blend + rig.crouch);
        }
        if (missing & (1 << limb))
            continue;
        float sx = shoulders[i][0], sy = shoulders[i][1], hx = wrists[i][0], hy = wrists[i][1];
        float dx = hx - sx, dy = hy - sy, d = hypotf(dx, dy),
              bend = sqrtf(fmaxf(0, 49 * sc * sc - d * d * .25f));
        float ex = (sx + hx) * .5f - (d > .01f ? dy / d : 0) * bend * (i ? 1 : -1) * face,
              ey = (sy + hy) * .5f + (d > .01f ? dx / d : 1) * bend * (i ? 1 : -1) * face;
        int mech = race==2 || (a->prostheticMask & (1 << limb)),grade=limb_item_grade(limb_installed(a,limb));
        uint32_t limbTint=grade>=0?limbGrades[grade].color:cloth;
        part(mech ? 76 : 72, sx, sy, ex, ey, 4 * sc, mech?limbTint:(!torso||torso==RAGS?skin:limbTint), li, 0, i ^ (face < 0), 0, 1);
        part(mech ? 77 : 75, ex, ey, hx, hy, 3 * sc, mech ? limbTint : skin, li, 0, i ^ (face < 0), 0, 1);
    }
    int coat = tf == 32 || tf == 33 || tf == 39 || tf == 40 || tf == 42 || tf == 44 || tf == 47;
    // Join the chin to the collar; transparent face/collar edges otherwise
    // leave a floating head, most visible with the smaller fitted hats.
    PART(72, 0, -27.5f, 0, -24, 2.8f, skin, 0);
    PART(tf, 0, -26, coat ? -rig.shift * .35f * forward : 0, coat ? -5 : -12, bw * (torso?2.8f:1.8f), torso?cloth:skin,
         itemDefs[torso].quality);
    // Painted face with independently tinted hair and visible creator details.
    int faceid = race==2 || a->hair == 0 ? 61 : a->hair == 3 ? 62 : a->beard ? 63 : 60;
    PART(faceid, 0, -33, 0, -25, race==4?6.7f:race==3?5.1f:5.8f, skin, 0);
    if (race!=2 && (a->hair == 2 || a->hair == 4 || a->hair == 5 || a->hair == 6)) {
        PART(72, -face * 2.2f, -31, -face * 3, -23, 1.8f, paintHair, 0);
        if (a->hair == 4)
            PART(72, face * 2.2f, -31, face * 3, -23, 1.8f, paintHair, 0);
        if (a->hair == 6)
            PART(72, -face * 3, -31, -face * 5, -25, 2.2f, paintHair, 0);
    }
    if (a->scar)
        line(RX(face, -29), RY(-29), RX(face * 1.5f, -27), RY(-27), shade(0x874b38, li));
    if(race==2){
        line(RX(-1.8f,-29),RY(-29),RX(1.8f,-29),RY(-29),0x9ec1b6);
        line(RX(-1.4f,-26),RY(-26),RX(1.4f,-26),RY(-26),shade(0x343d3c,li));
    }
    if(race==3)for(int side=-1;side<=1;side+=2){
        PART(72,side*2,-29,side*5.5f,-32,1.8f,skin,0);
    }
    if(race==4)for(int side=-1;side<=1;side+=2){
        PART(72,side*2.6f,-29,side*4,-31,2.8f,skin,0);
        PART(72,side*3,-32,side*4.2f,-35,1.6f,0xaaa58f,0);
    }
    if (head && hf >= 48) {
        float wide, top, bottom;
        head_attachment(hf, &wide, &top, &bottom);
        PART(hf, 0, top, 0, bottom, wide, itemDefs[head].color, itemDefs[head].quality);
    }
    if (limb_arms(a))
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
/* Direct previews use the same pose resolver as the cached world renderer. */
static void actor_painted(Actor *a,int x,int y,int sc,float li){
    actor_painted_stance(a,x,y,sc,li,combat_stance(a));
}
#define ACT_W 128
#define ACT_H 120
#define ACT_ANCHOR 152
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
    float stance=combat_stance(a);
    if(sc==1 && (uintptr_t)a>=(uintptr_t)g.actors && (uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)) {
        int n=(int)(a-g.actors),rel=actor_relation(n);
        if(!a->squad&&!a->dead){uint32_t col=rel<0?RED:rel>0?TEAL:GOLD;
            if(rel<0){line(x-3,y+3,x,y+6,col);line(x,y+6,x+3,y+3,col);}
            else if(rel>0)line(x-3,y+4,x+3,y+4,col);else rect(x-1,y+3,2,2,col);
        }
        int target=g.living.command[g.selected]>=0?g.living.command[g.selected]:g.actors[g.selected].target;
        if(n==target&&!a->dead){int left=x-11,right=x+11,top=y-37,bottom=y+2;
            line(left,top,left+4,top,RED);line(left,top,left,top+4,RED);line(right-4,top,right,top,RED);line(right,top,right,top+4,RED);
            line(left,bottom,left+4,bottom,RED);line(left,bottom-4,left,bottom,RED);line(right-4,bottom,right,bottom,RED);line(right,bottom-4,right,bottom,RED);
        }
    }
    Actor dressed;
    if (!a->gear[1] && (uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors+g.actorCount) &&
        g.depth.actors[a-g.actors].layer) {
        dressed = *a;
        dressed.gear[1] = g.depth.actors[a-g.actors].layer;
        a = &dressed;
    }
    float li = sc == 1 ? fminf(1.15f, light() + (chosen ? .16f : .06f)) : 1;
    if (chosen && sc == 1) {
        line(x - 11, y + 2, x, y + 7, 0x101713);
        line(x, y + 7, x + 11, y + 2, 0x101713);
        line(x - 10, y + 1, x, y + 6, TEAL);
        line(x, y + 6, x + 10, y + 1, TEAL);
    }
    if (sc != 1) {
        actor_painted_stance(a, x, y, sc, li,stance);
        return;
    }
    uint64_t key = actor_key(a, (int)(li * 64)) ^ ((uint64_t)(stance*32+1)*1099511628211ull);
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
        actor_painted_stance(a, ACT_W, ACT_ANCHOR, 2, li,stance);
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
    /* One-pixel silhouette edge separates clothing from earth without a panel. */
    for(int yy=1;yy<ACT_H-1;yy++) for(int xx=1;xx<ACT_W-1;xx++) {
        uint32_t *px=actorCache[slot].pixels;int k=yy*ACT_W+xx;
        if((px[k]>>24)>24) continue;
        int above=(px[k-ACT_W]>>24)>128,below=(px[k+ACT_W]>>24)>128,
            left=(px[k-1]>>24)>128,right=(px[k+1]>>24)>128;
        if(!above&&!below&&!left&&!right) continue;
        int sx=x-ACT_W/2+xx,sy=y-ACT_ANCHOR/2+yy;
        if(sx>=clipLeft&&sx<clipRight&&sy>=clipTop&&sy<clipBottom)
            rgba_over(fb+sy*pitch+sx,chosen && (below||right)?0x997aada1:0x99101513);
    }
    for (int yy = actorCache[slot].top; yy < actorCache[slot].bottom; yy++) {
        int sy = y - ACT_ANCHOR / 2 + yy;
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
    actor_painted(a, ACT_W, ACT_ANCHOR, 2, 1);
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

/* Late silhouette pass retains the real pose through foreground scenery. */
static void actor_outline(Actor*a,int x,int y,int chosen){
 Actor dressed;if(!a->gear[1]&&g.depth.actors[a-g.actors].layer){dressed=*a;dressed.gear[1]=g.depth.actors[a-g.actors].layer;a=&dressed;}
 float li=fminf(1.15f,light()+(chosen?.16f:.06f));uint64_t key=actor_key(a,(int)(li*64));int slot=-1;for(int i=0;i<ACT_CACHE;i++)if(actorCache[i].key==key){slot=i;break;}if(slot<0)return;
 uint32_t col=a->squad?(a->dead||a->unconscious>0?0xc0a98572:0xb0a1b5ad):0x907a8bb7;
 uint32_t*p=actorCache[slot].pixels;
 for(int yy=1;yy<ACT_H-1;yy++)for(int xx=1;xx<ACT_W-1;xx++){int k=yy*ACT_W+xx;if((p[k]>>24)>24)continue;if((p[k-1]>>24)<128&&(p[k+1]>>24)<128&&(p[k-ACT_W]>>24)<128&&(p[k+ACT_W]>>24)<128)continue;int sx=x-ACT_W/2+xx,sy=y-ACT_ANCHOR/2+yy;if(sx>=clipLeft&&sx<clipRight&&sy>=clipTop&&sy<clipBottom)rgba_over(fb+sy*pitch+sx,col);}
}
