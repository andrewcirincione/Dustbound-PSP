#ifndef DUST_COMBAT_POSE_H
#define DUST_COMBAT_POSE_H
#include "game.h"
#include <math.h>
/* Shared contact marker: rendering and damage use the same point in the stroke. */
#define STRIKE_CONTACT .45f
/* Strike time is shared by simulation and tests; class proficiency affects the
   stroke itself, not only the wait before it. No persistent save layout change. */
float combat_stance(const Actor *a);
void combat_visual_reset(void);
typedef struct {
    float handX, handY, blade, shift, crouch, step, twist;
} CombatPose;
/* Hands and weapon are solved in screen space. The blade is rigid: torso twist,
   vertical body proportions and crouching must never warp its secondary grip. */
typedef struct {
    float x, y, ux, uy;
} WeaponGrip;
static inline WeaponGrip weapon_grip(float x, float y, float angle, float scale) {
    return (WeaponGrip){x, y, cosf(angle) * scale, sinf(angle) * scale};
}
static inline void grip_hand(WeaponGrip g0, int secondary, float *x, float *y) {
    float along = secondary ? -4.f : 0.f;
    *x = g0.x + g0.ux * along;
    *y = g0.y + g0.uy * along;
}
/* Support hands follow the painted fore-end/string, never the torso transform. */
static inline float weapon_support_distance(int weapon) {
    int f = gear_family(weapon);
    if (f == 29 || f == 31)
        return 7.f;
    if (f == 25 || f == 26 || weapon == CROSSBOW)
        return 5.f;
    if (f == 24)
        return -6.f;
    if (f == 28)
        return -11.5f;
    return -4.f;
}
static inline void weapon_hand(WeaponGrip g0, int weapon, int secondary, float *x, float *y) {
    float along = secondary ? weapon_support_distance(weapon) : 0;
    *x = g0.x + g0.ux * along;
    *y = g0.y + g0.uy * along;
}
static inline float pose_ease(float t) {
    t = fmaxf(0, fminf(1, t));
    return t * t * (3 - 2 * t);
}
static inline CombatPose pose_mix(CombatPose a, CombatPose b, float t) {
    t = pose_ease(t);
#define MIX(v) a.v += (b.v - a.v) * t
    MIX(handX);
    MIX(handY);
    MIX(blade);
    MIX(shift);
    MIX(crouch);
    MIX(step);
    MIX(twist);
#undef MIX
    return a;
}
static inline int combat_style(int weapon) {
    int f = gear_family(weapon);
    if (weapon == NONE)return 5;
    if (f == 24 || f == 28)return 4;
    if (weapon_rapier(weapon))return 1;
    if (weapon_ranged(weapon))
        return 3;
    if (f == 86 || f == 5 || f == 16 || weapon == STAFF || f == 3 || f == 9)
        return 1;
    if (f >= 84 || f == 11 || f == 12 || f == 13 || f == 14 || f == 15 || f == 20 || f == 23 || weapon == CLEAVER)
        return 2;
    return 0;
}
static inline int combat_two_hands(int weapon) {
    if(weapon_rapier(weapon))return 0;
    int f = gear_family(weapon);
    return f == 0 || f == 2 || (f >= 4 && f <= 6) || f == 14 || f == 15 || f == 16 || f == 20 ||
           (f >= 24 && f != 27 && f != 30) || weapon == STAFF || weapon == CROSSBOW;
}
static inline float combat_strike_seconds(const Actor *a, int weapon) {
    int style = combat_style(weapon);
    float base = style == 2 ? .32f : style == 1 ? .22f : style == 5 ? .20f : .25f;
    float injury = 1 + .25f * (1 - limb_arm_condition(a, weapon));
    return fmaxf(.12f, fminf(1.1f, base * weapon_handling(a, weapon) * injury * (weapon?1:1-(level(a,MARTIAL)-1)*.0015f)));
}
/* Carry angular and body momentum through impact. The contact key has the
   same derivative on both sides; preparation and follow-through end at rest. */
static inline CombatPose pose_stroke(CombatPose load, CombatPose hit, CombatPose finish, float p) {
    p = fmaxf(0, fminf(1, p));
    int first = p < STRIKE_CONTACT;
    float span = first ? STRIKE_CONTACT : 1 - STRIKE_CONTACT;
    float t = first ? p / span : (p - STRIKE_CONTACT) / span;
    float h00 = 2*t*t*t-3*t*t+1, h10 = t*t*t-2*t*t+t;
    float h01 = -2*t*t*t+3*t*t, h11 = t*t*t-t*t;
    CombatPose a = first ? load : hit, b = first ? hit : finish, out;
#define CURVE(v) out.v = h00*a.v+h01*b.v+(first?h11:h10)*(finish.v-load.v)*.8f*span
    CURVE(handX); CURVE(handY); CURVE(blade); CURVE(shift);
    CURVE(crouch); CURVE(step); CURVE(twist);
#undef CURVE
    return out;
}
static inline CombatPose combat_idle(int weapon) {
    int s = combat_style(weapon);
    if (s == 5)return (CombatPose){3,-14,0,0,0,0,0};
    if (s == 3 || s == 4)return (CombatPose){4,-17,.55f,0,0,0,0};
    if (s == 1)return (CombatPose){4,-17,-.65f,0,0,0,0};
    if(s==2)return (CombatPose){4,-18,-1.15f,0,0,0,0};
    return (CombatPose){4,-16,.30f,0,0,0,0};
}
static inline CombatPose combat_pose(int weapon, int phase, float p) {
    int s = combat_style(weapon);
    CombatPose guard = {6, -19, -.85f, 0, 0, 0, 0};
    CombatPose load = {-3, -27, -2.2f, -2, 1, -1, -.10f};
    CombatPose hit = {9, -20, 0, 2.5f, 2, 4, .10f};
    CombatPose finish = {7, -16, .25f, 2, 2, 4, .14f};
    if (s == 1) { /* spear / knife: retract, extend, settle */
        guard = (CombatPose){4, -19, -.15f, 0, 0, 0, 0};
        load = (CombatPose){-3, -20, -.12f, -2, 1, -1, -.08f};
        hit = (CombatPose){11, -20, 0, 2.5f, 2, 5, .08f};
        finish = (CombatPose){9, -19, .08f, 2, 2, 4, .08f};
    } else if (s == 2) { /* heavy overhead chop */
        guard = (CombatPose){5, -18, -1.1f, 0, 0, 0, 0};
        load = (CombatPose){-2, -29, -1.9f, -2, 1, -1, -.08f};
        hit = (CombatPose){9, -20, 0, 3, 3, 4, .10f};
        finish = (CombatPose){8, -16, .23f, 3, 3, 4, .12f};
    } else if (s == 3) { /* shoulder the stock; recoil then settle */
        guard = (CombatPose){5, -19, -.55f, 0, 0, 0, 0};
        load = (CombatPose){5, -23, 0, 0, 1, 2, .04f};
        hit = load;
        finish = (CombatPose){2, -24, -.13f, -1, 1, 2, -.04f};
    } else if (s == 4) { /* bow arm holds; the drawing hand releases the string */
        guard = (CombatPose){6, -20, -.35f, 0, 0, 0, 0};
        load = (CombatPose){10, -23, 0, .5f, .5f, 2, .04f};
        hit = load;
        finish = (CombatPose){10, -23, -.025f, .5f, .5f, 2, .03f};
    } else if (s == 5) { /* raised fists, short punch, retract to protection */
        guard = (CombatPose){5, -23, 0, 0, 1, 0, 0};
        load = (CombatPose){2, -24, -.1f, -1, 1, -1, -.06f};
        hit = (CombatPose){11, -23, 0, 2, 1.5f, 3, .10f};
        finish = (CombatPose){8, -22, .05f, 1, 1, 2, .08f};
    }
    if (phase == 1)return pose_mix(guard, load, p);
    if (phase == 2)return pose_stroke(load, hit, finish, p);
    if (phase == 3)return pose_mix(finish, guard, p);
    return guard;
}
/* Translate the rigid grip into the intersection of the two arm-reach discs.
   Neither wrist is detached from the weapon or stretched beyond its bones. */
static inline WeaponGrip reachable_grip(WeaponGrip grip, float px, float py,
                                        float sx, float sy, float along, int two, float reach) {
    float cx = sx-grip.ux*along, cy = sy-grip.uy*along;
    float dx = grip.x-px, dy = grip.y-py, d = hypotf(dx,dy);
    if (d > reach) { grip.x=px+dx*reach/d; grip.y=py+dy*reach/d; }
    if (!two || hypotf(grip.x-cx,grip.y-cy)<=reach)return grip;
    dx=grip.x-cx;dy=grip.y-cy;d=hypotf(dx,dy);
    float qx=cx+dx*reach/d,qy=cy+dy*reach/d;
    if(hypotf(qx-px,qy-py)<=reach){grip.x=qx;grip.y=qy;return grip;}
    dx=cx-px;dy=cy-py;d=hypotf(dx,dy);
    float mx=(px+cx)*.5f,my=(py+cy)*.5f;
    float h=sqrtf(fmaxf(0,reach*reach-d*d*.25f));
    float ax=mx-dy/d*h,ay=my+dx/d*h,bx=mx+dy/d*h,by=my-dx/d*h;
    if(hypotf(ax-grip.x,ay-grip.y)<hypotf(bx-grip.x,by-grip.y)){grip.x=ax;grip.y=ay;}
    else {grip.x=bx;grip.y=by;}
    return grip;
}
#endif
