#ifndef DUST_COMBAT_POSE_H
#define DUST_COMBAT_POSE_H
#include "game.h"
#include <math.h>
/* Shared contact marker: rendering and damage use the same point in the stroke. */
#define STRIKE_CONTACT .45f
#define STRIKE_SECONDS .24f
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
    if (weapon_rapier(weapon))return 1;
    if (weapon_ranged(weapon))
        return 3;
    if (f == 86 || f == 5 || f == 16 || weapon == STAFF || weapon == NONE || f == 3 || f == 9)
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
static inline CombatPose combat_pose(int weapon, int phase, float p) {
    int s = combat_style(weapon);
    CombatPose guard = {6, -19, -.85f, 0, 0, 0, 0};
    CombatPose load = {-4, -27, -2.2f, -2, 1, -2, -.13f};
    CombatPose hit = {10, -20, 0, 3, 2, 5, .12f};
    CombatPose finish = {8, -14, 1.05f, 2, 2, 5, .18f};
    if (s == 1) { /* spear / knife / fist: draw back, extend, retract */
        guard = (CombatPose){4, -19, -.15f, 0, 0, 0, 0};
        load = (CombatPose){-4, -20, -.12f, -2, 1, -2, -.1f};
        hit = (CombatPose){14, -20, 0, 3, 2, 6, .1f};
        finish = (CombatPose){11, -19, .08f, 2, 2, 5, .1f};
    } else if (s == 2) { /* heavy overhead chop */
        guard = (CombatPose){5, -18, -1.1f, 0, 0, 0, 0};
        load = (CombatPose){-2, -30, -1.9f, -2, 1, -2, -.08f};
        hit = (CombatPose){11, -20, 0, 3, 3, 5, .13f};
        finish = (CombatPose){9, -12, .9f, 3, 3, 5, .15f};
    } else if (s == 3) { /* shoulder the stock; recoil then settle */
        guard = (CombatPose){5, -19, -.55f, 0, 0, 0, 0};
        load = (CombatPose){5, -23, 0, 0, 1, 2, .05f};
        hit = load;
        finish = (CombatPose){2, -24, -.13f, -1, 1, 2, -.04f};
    }
    if (phase == 1)
        return pose_mix(guard, load, p);
    if (phase == 2)
        return p < STRIKE_CONTACT ? pose_mix(load, hit, p / STRIKE_CONTACT)
                                  : pose_mix(hit, finish, (p - STRIKE_CONTACT) / (1 - STRIKE_CONTACT));
    if (phase == 3)
        return pose_mix(finish, guard, p);
    return guard;
}
#endif
