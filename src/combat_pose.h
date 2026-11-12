#ifndef DUST_COMBAT_POSE_H
#define DUST_COMBAT_POSE_H
#include "game.h"
#include <math.h>
/* Shared contact marker: rendering and damage use the same point in the stroke. */
#define STRIKE_CONTACT .45f
#define ATTACK_VARIANTS 3
float combat_stance(const Actor *a);
int combat_attack_variant(const Actor *a);
void combat_visual_reset(void);
typedef struct {
    float handX, handY, blade, shift, crouch, step, twist, rearX, rearY;
} CombatPose;
typedef struct {
    float windup, stroke, recovery;
} CombatTiming;
typedef struct {
    CombatPose load, hit, finish;
    CombatTiming pace;
} CombatMove;
typedef struct {
    CombatPose idle, guard;
    float strokeSeconds;
    CombatMove moves[ATTACK_VARIANTS];
} CombatProfile;
enum { UNARMED_JAB, UNARMED_CROSS, UNARMED_HOOK };
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
    if(arsenal_index(weapon)>=0)return arsenal_support_along(weapon);
    int f = gear_family(weapon);
    if (f == 29 || f == 31)
        return 7.f;
    if (f == 25 || f == 26 || weapon == CROSSBOW)
        return 5.f;
    if (f == 24)
        return -6.f;
    if (f == 28)
        return -11.5f;
    if(f==14||f==15)return -3.2f;
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
    MIX(rearX);
    MIX(rearY);
#undef MIX
    return a;
}
static inline int combat_two_hands(int weapon) {
    if(weapon_rapier(weapon))return 0;
    int f = gear_family(weapon);
    return f == 0 || f == 2 || (f >= 4 && f <= 6) || f == 14 || f == 15 || f == 16 || f == 20 ||
           (f >= 24 && f != 27 && f != 30) || weapon == STAFF || weapon == CROSSBOW;
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
    CURVE(crouch); CURVE(step); CURVE(twist); CURVE(rearX); CURVE(rearY);
#undef CURVE
    return out;
}
#include "combat_moves.h"

static inline const CombatProfile *combat_profile(int weapon) {
    if (!weapon)
        return &combatProfiles[UNARMED_PROFILE];
    int style = weapon_style(weapon);
    if (style == WS_POLEARM && gear_family(weapon) == 5)
        return &combatProfiles[SPEAR_PROFILE];
    return &combatProfiles[style < 0 ? WS_SABRE : style];
}
static inline CombatPose combat_idle(int weapon) { return combat_profile(weapon)->idle; }
static inline CombatPose combat_pose(int weapon, int phase, float p, int variant) {
    const CombatProfile *profile = combat_profile(weapon);
    const CombatMove *move = &profile->moves[variant];
    if (phase == 1) return pose_mix(profile->guard, move->load, p);
    if (phase == 2) return pose_stroke(move->load, move->hit, move->finish, p);
    if (phase == 3) return pose_mix(move->finish, profile->guard, p);
    return profile->guard;
}
static inline CombatTiming combat_timing(const Actor *a, int weapon, int variant) {
    const CombatProfile *profile = combat_profile(weapon);
    CombatTiming pace = profile->moves[variant].pace;
    float handling = weapon_handling(a, weapon);
    float martial = weapon ? 1 : 1 - (level(a, MARTIAL) - 1) * .0015f;
    if (a->kind != BEAST)
        handling *= martial;
    float cadence = handling * fmaxf(.55f, (weapon ? itemDefs[weapon].cooldown : 1) - effective_level(a, DEXTERITY) * .003f);
    float telegraph = a->squad ? .20f : .45f;
    float preparation = cadence * (weapon_ranged(weapon) ? .46f : .34f);
    float strokeBase = a->kind == BEAST && !weapon ? .20f * martial : profile->strokeSeconds;
    float stroke = strokeBase * handling * (1 + .25f * (1 - limb_arm_condition(a, weapon)));
    float recovery = fmaxf(.18f, cadence - fmaxf(telegraph, preparation) - stroke);
    if (a->kind == BEAST)
        pace = (CombatTiming){1, 1, 1};
    return (CombatTiming){fmaxf(telegraph, preparation * pace.windup), stroke * pace.stroke, recovery * pace.recovery};
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
