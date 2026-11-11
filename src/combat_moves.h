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
        .guard = POSE(5.8f, -16.5f, -.30f, .1f, .4f, 1, 0),
        .strokeSeconds = .24f,
        .moves = {
            { /* Quick rising cut from a low chamber. */
                POSE(7.25f, -15.9f, 0.55f, -0.55f, 0.56f, 0.14f, -0.028f),
                POSE(9.75f, -18.35f, -0.24f, 1.32f, 0.35f, 2.45f, 0.0175f),
                POSE(8.5f, -20.45f, -1.28f, 0.77f, 0, 2.1f, -0.021f),
                {.84f, .79f, .88f}
            },
            { /* High diagonal cut that follows through beside the hip. */
                POSE(7.125f, -21.15f, -1.85f, -0.935f, 0.42f, 0.14f, -0.035f),
                POSE(9.75f, -18.35f, -0.33f, 1.485f, 1.05f, 2.73f, 0.042f),
                POSE(9.25f, -16.25f, 0.39f, 0.99f, 1.05f, 2.45f, 0.028f),
                {1.16f, 1.08f, 1.12f}
            },
            { /* Short advancing cut from a compact waist guard. */
                POSE(5.75f, -17.3f, -.4f, -0.99f, 0.98f, 0.42f, -0.063f),
                POSE(10, -18, -0.87f, 1.54f, 0.49f, 3.08f, 0.0245f),
                POSE(8.5f, -19.05f, -0.37f, 0.88f, 0.42f, 2.8f, -0.042f),
                {1.01f, .94f, 1.01f}
            }
        }
    },
    [WS_HEAVY] = {
        .idle = POSE(2, -18, -2.47f, -.4f, .4f, 0, 0),
        .guard = POSE(5.3f, -16.2f, -.95f, -.3f, .8f, 1.5f, -.02f),
        .strokeSeconds = .35f,
        .moves = {
            { /* Raised guard to a planted diagonal cleave. */
                POSE(8, -25, -1.55f, -.8f, .9f, .6f, -.02f),
                POSE(9, -18, -.35f, 1.3f, 1.8f, 3, .04f),
                POSE(8, -14.8f, .36f, 1.7f, 2.1f, 3.3f, .04f),
                {1.24f, 1.05f, 1.16f}
            },
            { /* Low rising sweep; the hilt stays in front of the chest. */
                POSE(5, -16, .35f, -.7f, 1.8f, .8f, -.03f),
                POSE(9, -17.5f, -.05f, 1.4f, 1.6f, 2.8f, .04f),
                POSE(7, -20, -.65f, 1, 1, 3, .02f),
                {1.06f, 1.23f, 1.24f}
            },
            { /* Shorter, braced point thrust with the blade held level. */
                POSE(5, -18, -.10f, -.5f, 1, .8f, -.02f),
                POSE(11, -18.5f, -.12f, 1.7f, 1.5f, 3.6f, .03f),
                POSE(7.5f, -18, -.15f, .9f, 1.2f, 3.4f, .01f),
                {.86f, .91f, .95f}
            }
        }
    },
    [WS_SABRE] = {
        .idle = POSE(4, -16, .32f, 0, 0, 0, 0),
        .guard = POSE(7, -13.7f, .10f, 0, .3f, .7f, 0),
        .strokeSeconds = .25f,
        .moves = {
            { /* Wrist cut from a forward guard. */
                POSE(5, -18.5f, -.78f, -.4f, .4f, .5f, -.02f),
                POSE(10.5f, -17.8f, -.12f, 1.1f, .8f, 2.4f, .03f),
                POSE(8.2f, -14.8f, .38f, .7f, .9f, 2.2f, .02f),
                {.79f, .82f, .86f}
            },
            { /* Rising backhand from beside the belt. */
                POSE(4, -14.8f, .25f, -.6f, 1.1f, .6f, -.02f),
                POSE(10.2f, -18.8f, -.35f, 1.2f, .8f, 2.6f, .03f),
                POSE(8, -19.5f, -.65f, .8f, .5f, 2.4f, .01f),
                {1.05f, 1.01f, 1.01f}
            },
            { /* Diagonal cut with a compact raised chamber. */
                POSE(4.5f, -20.5f, -.95f, -.7f, .7f, .5f, -.03f),
                POSE(9.5f, -17.5f, -.18f, 1.3f, 1.1f, 2.8f, .04f),
                POSE(8.5f, -14.4f, .35f, 1, 1.3f, 2.6f, .03f),
                {1.17f, 1.14f, 1.18f}
            }
        }
    },
    [WS_HACKER] = {
        .idle = POSE(3, -17, -.96f, 0, 0, 0, 0),
        .guard = POSE(6.5f, -14.8f, -.45f, .1f, .5f, .8f, 0),
        .strokeSeconds = .30f,
        .moves = {
            { /* Short hook around the guard. */
                POSE(7.5f, -19.4f, -1.44f, -0.55f, 0.42f, 0.42f, -0.021f),
                POSE(9.75f, -18.35f, -0.25f, 1.485f, 1.12f, 2.59f, 0.0455f),
                POSE(9.25f, -17.3f, 0.42f, 1.1f, 1.05f, 2.38f, 0.0315f),
                {.80f, .85f, .91f}
            },
            { /* Full overhead chop. */
                POSE(6.25f, -21.85f, -1.5f, -1.375f, 1.12f, 0.28f, -0.035f),
                POSE(9.5f, -18.35f, -0.27f, 1.815f, 1.89f, 3.22f, 0.035f),
                POSE(9.5f, -16.25f, 0.6f, 1.87f, 2.31f, 3.22f, 0.049f),
                {1.23f, 1.18f, 1.18f}
            },
            { /* Lift through the opponent from a bent-knee chamber. */
                POSE(7, -15.9f, 0.67f, -1.155f, 1.96f, 0.42f, -0.0455f),
                POSE(9.75f, -17.65f, -0.19f, 1.65f, 1.19f, 2.94f, 0.035f),
                POSE(8.5f, -20.1f, -1.15f, 1.32f, 0.49f, 2.94f, -0.0245f),
                {1.04f, 1.02f, 1.06f}
            }
        }
    },
    [WS_BLUNT] = {
        .idle = POSE(3, -18, -1.34f, 0, 0, 0, 0),
        .guard = POSE(6, -16.5f, -.85f, -.1f, .8f, 1.3f, -.02f),
        .strokeSeconds = .32f,
        .moves = {
            { /* Compact forward smash. */
                POSE(5.5f, -21, -1.15f, -.5f, 1.1f, .6f, -.02f),
                POSE(9.5f, -18.5f, -.15f, 1.2f, 1.8f, 2.8f, .04f),
                POSE(8.5f, -15.5f, .35f, 1.4f, 2, 3, .04f),
                {.79f, .87f, .93f}
            },
            { /* Shoulder-loaded swing. */
                POSE(6, -22.5f, -1.75f, -.8f, 1, .4f, -.03f),
                POSE(9, -18, -.25f, 1.4f, 1.7f, 3.2f, .04f),
                POSE(8, -16, .20f, 1.6f, 2.1f, 3.3f, .04f),
                {1.18f, 1.20f, 1.22f}
            },
            { /* Upper swing driven by the legs. */
                POSE(5, -14.5f, .5f, -.7f, 2.2f, .7f, -.03f),
                POSE(9, -18.5f, -.30f, 1.3f, 1.6f, 3, .03f),
                POSE(7.5f, -20.5f, -1, .9f, .7f, 3, .02f),
                {1.07f, 1.04f, 1.09f}
            }
        }
    },
    [WS_POLEARM] = {
        .idle = POSE(4, -18, -.77f, 0, 0, 0, 0),
        .guard = POSE(5, -17.5f, -.18f, -.2f, .7f, 1.3f, 0),
        .strokeSeconds = .27f,
        .moves = {
            { /* Quick linear strike. */
                POSE(6.5f, -18, -0.11f, -0.88f, 0.7f, 0.21f, -0.0245f),
                POSE(10, -18, -0.13f, 1.595f, 0.98f, 3.22f, 0.021f),
                POSE(9, -18, -0.16f, 1.265f, 0.98f, 3.22f, 0.014f),
                {.84f, .80f, .88f}
            },
            { /* Broad downward sweep. */
                POSE(6.25f, -20.8f, -1.45f, -1.43f, 1.05f, 0.42f, -0.0455f),
                POSE(9.5f, -18.35f, -0.42f, 1.815f, 1.82f, 3.57f, 0.0385f),
                POSE(9.25f, -16.6f, 0.38f, 1.76f, 2.17f, 3.57f, 0.0455f),
                {1.20f, 1.16f, 1.19f}
            },
            { /* Sweep up from a low guard. */
                POSE(6.5f, -16.25f, 0.41f, -1.375f, 1.47f, 0.56f, -0.042f),
                POSE(9.75f, -17.65f, -0.31f, 1.705f, 1.12f, 3.29f, 0.0245f),
                POSE(8.75f, -19.75f, -1.2f, 1.21f, 0.7f, 3.15f, -0.014f),
                {1.04f, 1.01f, 1.06f}
            }
        }
    },
    [SPEAR_PROFILE] = {
        .idle = POSE(4, -17, -.64f, 0, 0, 0, 0),
        .guard = POSE(5, -17, -.10f, -.1f, .5f, 1.3f, 0),
        .strokeSeconds = .23f,
        .moves = {
            { /* Short jab from guard. */
                POSE(7, -17.65f, -0.16f, -0.495f, 0.42f, 0.49f, -0.021f),
                POSE(10, -18, -0.16f, 1.265f, 0.49f, 2.45f, 0.0175f),
                POSE(9, -17.65f, -0.11f, 0.935f, 0.42f, 2.31f, 0),
                {.79f, .81f, .86f}
            },
            { /* Deep, braced low lunge. */
                POSE(6, -16.6f, -0.08f, -1.595f, 1.96f, 0.28f, -0.0385f),
                POSE(10.25f, -17.3f, -0.24f, 2.31f, 2.45f, 4.41f, 0.0315f),
                POSE(9.5f, -16.95f, -0.2f, 1.925f, 2.38f, 4.27f, 0.021f),
                {1.18f, 1.14f, 1.19f}
            },
            { /* Raise the point and drive down. */
                POSE(6.75f, -20.1f, -0.85f, -1.045f, 0.91f, 0.77f, -0.035f),
                POSE(10, -19.05f, 0.02f, 1.815f, 1.33f, 3.29f, 0.0385f),
                POSE(9.25f, -17.65f, 0.3f, 1.265f, 1.47f, 3.08f, 0.021f),
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
                POSE(7.75f, -17.65f, -0.12f, -0.385f, 0.63f, 0.56f, -0.042f),
                POSE(10.25f, -18, -0.13f, 1.155f, 0.84f, 1.75f, -0.014f),
                POSE(9, -17.65f, -0.15f, 0.825f, 0.7f, 1.75f, -0.028f),
                {.81f, .82f, .89f}
            },
            { /* Full forward lunge. */
                POSE(6.75f, -18.35f, -0.11f, -1.32f, 0.98f, 0.07f, -0.063f),
                POSE(10.75f, -18.35f, -0.05f, 2.585f, 1.89f, 4.83f, -0.0385f),
                POSE(10.25f, -18, 0.04f, 2.09f, 1.96f, 4.76f, -0.042f),
                {1.11f, 1.11f, 1.20f}
            },
            { /* Dip under the guard and thrust upward. */
                POSE(7.5f, -16.25f, 0.44f, -1.155f, 1.61f, 0.35f, -0.0525f),
                POSE(10.25f, -17.65f, -0.31f, 1.925f, 1.82f, 3.36f, -0.035f),
                POSE(9.25f, -18.7f, -0.48f, 1.32f, 1.4f, 3.36f, -0.042f),
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
                POSE(7.25f, -18, -0.2f, -0.605f, 0.77f, 0.28f, -0.0315f),
                POSE(10.25f, -18, -0.06f, 1.54f, 0.77f, 2.38f, 0.028f),
                POSE(9, -18, -0.15f, 1.1f, 0.84f, 2.38f, 0.0105f),
                {.81f, .82f, .89f}
            },
            { /* Overhand slash at close range. */
                POSE(7.25f, -20.8f, -1.88f, -0.825f, 0.77f, 0.35f, -0.042f),
                POSE(10, -18.7f, -0.3f, 1.65f, 1.12f, 3.01f, 0.0385f),
                POSE(9.5f, -16.25f, 0.68f, 1.155f, 1.68f, 2.87f, 0.028f),
                {1.17f, 1.12f, 1.14f}
            },
            { /* Underhand thrust from the belt. */
                POSE(7.25f, -15.9f, 0.43f, -1.1f, 1.96f, 0.56f, -0.0455f),
                POSE(10.25f, -18, -0.52f, 1.87f, 1.12f, 2.94f, 0.042f),
                POSE(9, -19.4f, -1.05f, 1.375f, 0.77f, 2.94f, 0.014f),
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
                POSE(10, -22, .02f, .7f, 2.2f, 2.6f, .04f),
                POSE(10, -22, 0, .7f, 2.2f, 2.6f, .04f),
                POSE(8, -20, .14f, .3f, 2, 2.6f, .02f),
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
                POSE(6.8f, -19.7f, 0, 0.3f, 0.4f, 1.12f, 0.014f),
                POSE(6.8f, -19.7f, 0, 0.3f, 0.4f, 1.12f, 0.014f),
                POSE(6.4f, -19.7f, -0.11f, -0.15f, 0.48f, 1.12f, -0.014f),
                {.82f, .85f, .90f}
            },
            { /* Low supported shot. */
                POSE(7, -19.35f, 0, 0.5f, 3.2f, 2.88f, 0.035f),
                POSE(7, -19.35f, 0, 0.5f, 3.2f, 2.88f, 0.035f),
                POSE(6.4f, -18.65f, 0.17f, -0.15f, 2.88f, 2.88f, 0.007f),
                {1.14f, 1.13f, 1.15f}
            },
            { /* Raise the stock, then hold through release. */
                POSE(6.4f, -20.75f, -0.03f, -0.55f, 1.12f, 2.16f, -0.0315f),
                POSE(6.4f, -20.75f, 0, -0.55f, 1.12f, 2.16f, -0.0315f),
                POSE(6, -21.1f, -0.18f, -0.8f, 1.12f, 2.16f, -0.0455f),
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
                POSE(6.2f, -19.7f, 0, 0.35f, 0.4f, 1.04f, 0.0175f),
                POSE(6.2f, -19.7f, 0, 0.35f, 0.4f, 1.04f, 0.0175f),
                POSE(5.4f, -20.75f, -0.27f, -0.7f, 0.4f, 1.04f, -0.0245f),
                {.82f, .86f, .91f}
            },
            { /* Wide planted aim with a controlled recoil. */
                POSE(6.4f, -20.4f, 0, 0.55f, 1.76f, 2.72f, 0.035f),
                POSE(6.4f, -20.4f, 0, 0.55f, 1.76f, 2.72f, 0.035f),
                POSE(5.8f, -20.75f, -0.12f, 0.05f, 1.84f, 2.72f, 0.007f),
                {1.18f, 1.13f, 1.17f}
            },
            { /* Low shot with a stronger muzzle rise. */
                POSE(6.8f, -19, 0, 0.7f, 3.04f, 2.4f, 0.021f),
                POSE(6.8f, -19, 0, 0.7f, 3.04f, 2.4f, 0.021f),
                POSE(6, -20.4f, -0.41f, -0.6f, 2.72f, 2.4f, -0.0245f),
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
                FISTS(6, -25.5f, 1.3f, -26.9f, -.4f, 1.8f, 1.5f, -.03f),
                FISTS(14, -25.3f, 1.8f, -27, 1.2f, 1.5f, 2.6f, -.05f),
                FISTS(9, -25, 1.7f, -26.5f, .6f, 1.8f, 2.4f, -.03f),
                {.78f, .76f, .87f}
            },
            [UNARMED_CROSS] = { /* Rear hand drives forward as the hips turn. */
                FISTS(7, -26.1f, 1.5f, -24, -.7f, 2, 1.5f, .03f),
                FISTS(4.3f, -26.7f, 9, -24.7f, 1.5f, 1.8f, 2.8f, -.08f),
                FISTS(4.8f, -26.2f, 6, -24, .9f, 2, 2.6f, -.05f),
                {1.01f, 1.00f, 1.05f}
            },
            [UNARMED_HOOK] = { /* Dip, rise through the body and curl back to guard. */
                FISTS(6, -21, 1.4f, -26, -.7f, 2.6f, 1.4f, -.03f),
                FISTS(11, -23.5f, 2.5f, -26.8f, 1.2f, 1.8f, 2.6f, .05f),
                FISTS(7.5f, -26, 1.8f, -26.4f, .7f, 1.3f, 2.5f, -.04f),
                {1.19f, 1.22f, 1.18f}
            }
        }
    }
};
#undef FISTS
#undef POSE
#endif
