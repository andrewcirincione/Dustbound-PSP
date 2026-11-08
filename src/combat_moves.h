#ifndef DUST_COMBAT_MOVES_H
#define DUST_COMBAT_MOVES_H

enum { UNARMED_PROFILE = WEAPON_STYLES, SPEAR_PROFILE };

/* Forward hand position, blade pitch, weight shift, crouch, lead step and torso
   turn. Negative blade pitch points up. Fists also key the rear hand separately. */
#define POSE(x, y, pitch, shift, crouch, step, twist) {x, y, pitch, shift, crouch, step, twist, 0, 0}
#define FISTS(x, y, rearX, rearY, shift, crouch, step, twist) {x, y, 0, shift, crouch, step, twist, rearX, rearY}

static const CombatProfile combatProfiles[] = {
    [WS_KATANA] = {
        .idle = POSE(4, -16, .22f, 0, 0, 0, 0),
        .guard = POSE(6, -21, -.64f, .2f, .5f, .7f, 0),
        .strokeSeconds = .24f,
        .moves = {
            { /* Quick rising cut from a low chamber. */
                POSE(1, -15, .55f, -1, .8f, .2f, -.08f),
                POSE(11, -22, -.24f, 2.4f, .5f, 3.5f, .05f),
                POSE(6, -28, -1.28f, 1.4f, 0, 3, -.06f),
                {.84f, .79f, .88f}
            },
            { /* High diagonal cut that follows through beside the hip. */
                POSE(.5f, -30, -1.85f, -1.7f, .6f, .2f, -.10f),
                POSE(11, -22, -.33f, 2.7f, 1.5f, 3.9f, .12f),
                POSE(9, -16, .39f, 1.8f, 1.5f, 3.5f, .08f),
                {1.16f, 1.08f, 1.12f}
            },
            { /* Draw across the waist and finish at shoulder level. */
                POSE(-5, -19, -2.6f, -1.8f, 1.4f, .6f, -.18f),
                POSE(12, -21, -.87f, 2.8f, .7f, 4.4f, .07f),
                POSE(6, -24, -.37f, 1.6f, .6f, 4, -.12f),
                {1.01f, .94f, 1.01f}
            }
        }
    },
    [WS_HEAVY] = {
        .idle = POSE(2, -18, -2.47f, -.4f, .4f, 0, 0),
        .guard = POSE(3, -21, -1.40f, -.6f, 1.2f, 2, -.04f),
        .strokeSeconds = .35f,
        .moves = {
            { /* Overhead cleave. Set the blade back, then sink into the plant. */
                POSE(-6, -35, -2.38f, -3.6f, 2.5f, .7f, -.16f),
                POSE(8, -23, -.42f, 3.3f, 3.4f, 5.5f, .10f),
                POSE(10, -17, .32f, 3.7f, 4.5f, 5.5f, .15f),
                {1.24f, 1.05f, 1.16f}
            },
            { /* Low sweep with the hilt pulled across a wide stance. */
                POSE(-6, -16, .21f, -3.7f, 3.6f, 2.4f, -.19f),
                POSE(9, -18, -.15f, 3.9f, 3.5f, 5.1f, .16f),
                POSE(3, -27, -1.45f, 2.9f, 2, 5.1f, -.15f),
                {1.06f, 1.23f, 1.24f}
            },
            { /* Shorter, braced point thrust with the blade held level. */
                POSE(-4, -21, -.10f, -2.3f, 2.4f, 1.4f, -.12f),
                POSE(12, -20, -.20f, 4.3f, 2.7f, 6.1f, .06f),
                POSE(7, -19, -.12f, 2.1f, 3, 6, -.05f),
                {.86f, .91f, .95f}
            }
        }
    },
    [WS_SABRE] = {
        .idle = POSE(4, -16, .32f, 0, 0, 0, 0),
        .guard = POSE(7, -20, -.56f, .2f, .4f, 1, .04f),
        .strokeSeconds = .25f,
        .moves = {
            { /* Wrist cut from a forward guard. */
                POSE(4, -25, -1.20f, -.5f, 0, 0, -.03f),
                POSE(12, -21, -.18f, 2.4f, 1, 3, .11f),
                POSE(10, -18, .47f, 1.4f, 1, 3, .06f),
                {.79f, .82f, .86f}
            },
            { /* Long backhand drawn from beside the belt. */
                POSE(-3, -16, .70f, -2, 1.8f, .1f, -.16f),
                POSE(11, -21, -.29f, 2.6f, 1.4f, 3.6f, .07f),
                POSE(7, -28, -1.43f, 2, .4f, 3.2f, -.10f),
                {1.05f, 1.01f, 1.01f}
            },
            { /* Committed shoulder-height cut. */
                POSE(-5, -25, -2.44f, -2.9f, 1, -.7f, -.14f),
                POSE(12, -22, -.70f, 3.4f, 1.8f, 4.6f, .11f),
                POSE(10, -20, .08f, 2.6f, 1.6f, 4.3f, .16f),
                {1.17f, 1.14f, 1.18f}
            }
        }
    },
    [WS_HACKER] = {
        .idle = POSE(3, -17, -.96f, 0, 0, 0, 0),
        .guard = POSE(5, -19, -1.06f, .3f, .8f, 1, 0),
        .strokeSeconds = .30f,
        .moves = {
            { /* Short hook around the guard. */
                POSE(2, -25, -1.44f, -1, .6f, .6f, -.06f),
                POSE(11, -22, -.25f, 2.7f, 1.6f, 3.7f, .13f),
                POSE(9, -19, .42f, 2, 1.5f, 3.4f, .09f),
                {.80f, .85f, .91f}
            },
            { /* Full overhead chop. */
                POSE(-3, -32, -2.10f, -2.5f, 1.6f, .4f, -.10f),
                POSE(10, -22, -.27f, 3.3f, 2.7f, 4.6f, .10f),
                POSE(10, -16, .60f, 3.4f, 3.3f, 4.6f, .14f),
                {1.23f, 1.18f, 1.18f}
            },
            { /* Lift through the opponent from a bent-knee chamber. */
                POSE(0, -15, .67f, -2.1f, 2.8f, .6f, -.13f),
                POSE(11, -20, -.19f, 3, 1.7f, 4.2f, .10f),
                POSE(6, -27, -1.15f, 2.4f, .7f, 4.2f, -.07f),
                {1.04f, 1.02f, 1.06f}
            }
        }
    },
    [WS_BLUNT] = {
        .idle = POSE(3, -18, -1.34f, 0, 0, 0, 0),
        .guard = POSE(6, -20, -1.19f, -.2f, 1.2f, 1.6f, -.06f),
        .strokeSeconds = .32f,
        .moves = {
            { /* Compact forward smash. */
                POSE(4, -25, -1.34f, -.8f, 1.4f, 1, 0),
                POSE(12, -20, -.17f, 3.4f, 2.3f, 4.1f, .12f),
                POSE(9, -18, .42f, 3, 2.5f, 4, .10f),
                {.79f, .87f, .93f}
            },
            { /* Shoulder-loaded swing. */
                POSE(-5, -26, -2.74f, -3.5f, 2.4f, 0, -.18f),
                POSE(10, -22, -.48f, 3.6f, 2.6f, 5.1f, .15f),
                POSE(11, -17, .34f, 4, 3.3f, 5.1f, .18f),
                {1.18f, 1.20f, 1.22f}
            },
            { /* Upper swing driven by the legs. */
                POSE(-2, -17, .56f, -2.4f, 3.7f, 1.3f, -.14f),
                POSE(12, -21, -.22f, 3.1f, 2.1f, 4.3f, .11f),
                POSE(7, -29, -1.34f, 2.7f, .7f, 4.3f, -.07f),
                {1.07f, 1.04f, 1.09f}
            }
        }
    },
    [WS_POLEARM] = {
        .idle = POSE(4, -18, -.77f, 0, 0, 0, 0),
        .guard = POSE(6, -20, -.34f, -.3f, .9f, 1.1f, 0),
        .strokeSeconds = .27f,
        .moves = {
            { /* Quick linear strike. */
                POSE(-2, -21, -.11f, -1.6f, 1, .3f, -.07f),
                POSE(12, -21, -.13f, 2.9f, 1.4f, 4.6f, .06f),
                POSE(8, -21, -.16f, 2.3f, 1.4f, 4.6f, .04f),
                {.84f, .80f, .88f}
            },
            { /* Broad downward sweep. */
                POSE(-3, -29, -2.16f, -2.6f, 1.5f, .6f, -.13f),
                POSE(10, -22, -.42f, 3.3f, 2.6f, 5.1f, .11f),
                POSE(9, -17, .38f, 3.2f, 3.1f, 5.1f, .13f),
                {1.20f, 1.16f, 1.19f}
            },
            { /* Sweep up from a low guard. */
                POSE(-2, -16, .41f, -2.5f, 2.1f, .8f, -.12f),
                POSE(11, -20, -.31f, 3.1f, 1.6f, 4.7f, .07f),
                POSE(7, -26, -1.20f, 2.2f, 1, 4.5f, -.04f),
                {1.04f, 1.01f, 1.06f}
            }
        }
    },
    [SPEAR_PROFILE] = {
        .idle = POSE(4, -17, -.64f, 0, 0, 0, 0),
        .guard = POSE(6, -20, -.18f, -.2f, .6f, 1, 0),
        .strokeSeconds = .23f,
        .moves = {
            { /* Short jab from guard. */
                POSE(0, -20, -.16f, -.9f, .6f, .7f, -.06f),
                POSE(12, -21, -.16f, 2.3f, .7f, 3.5f, .05f),
                POSE(8, -20, -.11f, 1.7f, .6f, 3.3f, 0),
                {.79f, .81f, .86f}
            },
            { /* Deep, braced low lunge. */
                POSE(-4, -17, -.08f, -2.9f, 2.8f, .4f, -.11f),
                POSE(13, -19, -.24f, 4.2f, 3.5f, 6.3f, .09f),
                POSE(10, -18, -.20f, 3.5f, 3.4f, 6.1f, .06f),
                {1.18f, 1.14f, 1.19f}
            },
            { /* Raise the point and drive down. */
                POSE(-1, -27, -.85f, -1.9f, 1.3f, 1.1f, -.10f),
                POSE(12, -24, .02f, 3.3f, 1.9f, 4.7f, .11f),
                POSE(9, -20, .30f, 2.3f, 2.1f, 4.4f, .06f),
                {1.02f, .98f, 1.03f}
            }
        }
    },
    [WS_RAPIER] = {
        .idle = POSE(4, -17, -.58f, 0, 0, 0, 0),
        .guard = POSE(7, -21, -.11f, -.1f, .8f, 1.4f, -.12f),
        .strokeSeconds = .21f,
        .moves = {
            { /* Stop thrust with little advance. */
                POSE(3, -20, -.12f, -.7f, .9f, .8f, -.12f),
                POSE(13, -21, -.13f, 2.1f, 1.2f, 2.5f, -.04f),
                POSE(8, -20, -.15f, 1.5f, 1, 2.5f, -.08f),
                {.81f, .82f, .89f}
            },
            { /* Full forward lunge. */
                POSE(-1, -22, -.11f, -2.4f, 1.4f, .1f, -.18f),
                POSE(15, -22, -.05f, 4.7f, 2.7f, 6.9f, -.11f),
                POSE(13, -21, .04f, 3.8f, 2.8f, 6.8f, -.12f),
                {1.11f, 1.11f, 1.20f}
            },
            { /* Dip under the guard and thrust upward. */
                POSE(2, -16, .44f, -2.1f, 2.3f, .5f, -.15f),
                POSE(13, -20, -.31f, 3.5f, 2.6f, 4.8f, -.10f),
                POSE(9, -23, -.48f, 2.4f, 2, 4.8f, -.12f),
                {1.04f, .97f, 1.01f}
            }
        }
    },
    [WS_KNIFE] = {
        .idle = POSE(3, -15, -.19f, 0, 0, 0, 0),
        .guard = POSE(5, -20, -.42f, .7f, 1, 1, -.03f),
        .strokeSeconds = .21f,
        .moves = {
            { /* Close, short jab. */
                POSE(1, -21, -.20f, -1.1f, 1.1f, .4f, -.09f),
                POSE(13, -21, -.06f, 2.8f, 1.1f, 3.4f, .08f),
                POSE(8, -21, -.15f, 2, 1.2f, 3.4f, .03f),
                {.81f, .82f, .89f}
            },
            { /* Overhand slash at close range. */
                POSE(1, -29, -1.88f, -1.5f, 1.1f, .5f, -.12f),
                POSE(12, -23, -.30f, 3, 1.6f, 4.3f, .11f),
                POSE(10, -16, .68f, 2.1f, 2.4f, 4.1f, .08f),
                {1.17f, 1.12f, 1.14f}
            },
            { /* Underhand thrust from the belt. */
                POSE(1, -15, .43f, -2, 2.8f, .8f, -.13f),
                POSE(13, -21, -.52f, 3.4f, 1.6f, 4.2f, .12f),
                POSE(8, -25, -1.05f, 2.5f, 1.1f, 4.2f, .04f),
                {1.02f, .98f, 1.03f}
            }
        }
    },
    [WS_BOW] = {
        .idle = POSE(4, -17, .48f, 0, 0, 0, 0),
        .guard = POSE(6, -20, -.34f, 0, .5f, 1, 0),
        .strokeSeconds = .25f,
        .moves = {
            { /* Snap shot from a high ready position. */
                POSE(10, -23, 0, .7f, .6f, 1.4f, .02f),
                POSE(10, -23, 0, .7f, .6f, 1.4f, .02f),
                POSE(8, -22, .09f, .1f, .6f, 1.4f, 0),
                {.80f, .83f, .88f}
            },
            { /* Longer draw with a wider, settled stance. */
                POSE(11, -24, -.02f, -.9f, 1, 3, -.13f),
                POSE(11, -24, 0, -.9f, 1, 3, -.13f),
                POSE(10, -25, -.12f, -1.4f, 1, 3, -.09f),
                {1.20f, 1.16f, 1.13f}
            },
            { /* Low braced draw and release. */
                POSE(10, -22, .02f, 1.1f, 3.9f, 3.7f, .07f),
                POSE(10, -22, 0, 1.1f, 3.9f, 3.7f, .07f),
                POSE(8, -20, .14f, .3f, 3.5f, 3.7f, .03f),
                {1.04f, .98f, 1.03f}
            }
        }
    },
    [WS_CROSSBOW] = {
        .idle = POSE(4, -17, .45f, 0, 0, 0, 0),
        .guard = POSE(6, -20, -.23f, .1f, .6f, 1, 0),
        .strokeSeconds = .25f,
        .moves = {
            { /* Quick level shot. */
                POSE(9, -23, 0, .6f, .5f, 1.4f, .04f),
                POSE(9, -23, 0, .6f, .5f, 1.4f, .04f),
                POSE(7, -23, -.11f, -.3f, .6f, 1.4f, -.04f),
                {.82f, .85f, .90f}
            },
            { /* Low supported shot. */
                POSE(10, -22, 0, 1, 4, 3.6f, .10f),
                POSE(10, -22, 0, 1, 4, 3.6f, .10f),
                POSE(7, -20, .17f, -.3f, 3.6f, 3.6f, .02f),
                {1.14f, 1.13f, 1.15f}
            },
            { /* Raise the stock, then hold through release. */
                POSE(7, -26, -.03f, -1.1f, 1.4f, 2.7f, -.09f),
                POSE(7, -26, 0, -1.1f, 1.4f, 2.7f, -.09f),
                POSE(5, -27, -.18f, -1.6f, 1.4f, 2.7f, -.13f),
                {1.04f, 1.02f, 1.02f}
            }
        }
    },
    [WS_FIREARM] = {
        .idle = POSE(4, -17, .53f, 0, 0, 0, 0),
        .guard = POSE(5, -20, -.38f, 0, .6f, .8f, 0),
        .strokeSeconds = .25f,
        .moves = {
            { /* Quick shoulder shot. */
                POSE(6, -23, 0, .7f, .5f, 1.3f, .05f),
                POSE(6, -23, 0, .7f, .5f, 1.3f, .05f),
                POSE(2, -26, -.27f, -1.4f, .5f, 1.3f, -.07f),
                {.82f, .86f, .91f}
            },
            { /* Wide planted aim with a controlled recoil. */
                POSE(7, -25, 0, 1.1f, 2.2f, 3.4f, .10f),
                POSE(7, -25, 0, 1.1f, 2.2f, 3.4f, .10f),
                POSE(4, -26, -.12f, .1f, 2.3f, 3.4f, .02f),
                {1.18f, 1.13f, 1.17f}
            },
            { /* Low shot with a stronger muzzle rise. */
                POSE(9, -21, 0, 1.4f, 3.8f, 3, .06f),
                POSE(9, -21, 0, 1.4f, 3.8f, 3, .06f),
                POSE(5, -25, -.41f, -1.2f, 3.4f, 3, -.07f),
                {1.03f, 1.03f, 1.07f}
            }
        }
    },
    [UNARMED_PROFILE] = {
        .idle = FISTS(4, -15, -3, -15, 0, 0, 0, 0),
        .guard = FISTS(7.5f, -26, 1.7f, -26.4f, .5f, 1.9f, 2.3f, -.05f),
        .strokeSeconds = .23f,
        .moves = {
            [UNARMED_JAB] = { /* Lead hand snaps out, rear fist protects the jaw. */
                FISTS(5.5f, -25.5f, 1.3f, -26.9f, -.7f, 2, 1.8f, -.08f),
                FISTS(18.5f, -25.3f, 1.8f, -27, 2.7f, 1.5f, 4.1f, -.13f),
                FISTS(10, -25, 1.7f, -26.5f, 1.5f, 1.8f, 3.8f, -.08f),
                {.78f, .76f, .87f}
            },
            [UNARMED_CROSS] = { /* Rear hand drives forward as the hips turn. */
                FISTS(7, -26.1f, -.9f, -23.6f, -2.5f, 2.2f, 1.7f, .10f),
                FISTS(3.4f, -26.7f, 19.6f, -24.7f, 4, 2, 4.8f, -.28f),
                FISTS(3.4f, -26.2f, 13, -23.8f, 3.5f, 2.5f, 4.8f, -.24f),
                {1.01f, 1.00f, 1.05f}
            },
            [UNARMED_HOOK] = { /* Dip, rise through the body and curl back to guard. */
                FISTS(4.3f, -19, 1.4f, -26, -2, 4.4f, 1.8f, -.10f),
                FISTS(12.3f, -23.5f, 2.5f, -26.8f, 3.4f, 2.2f, 4.6f, .12f),
                FISTS(7.5f, -29.4f, 1.8f, -26.4f, 2.4f, .8f, 4.6f, -.16f),
                {1.19f, 1.22f, 1.18f}
            }
        }
    }
};
#undef FISTS
#undef POSE
#endif
