/* RC2's walking for RC1 HD.
 *
 * RC1's walk handler (hero_update 0xb1630, states 2 and 0x73) is the older version of RC2's
 * (0x83ee6c). walk_hooks.s takes over the middle of RC1's with three calls, in RC2's order:
 *
 *   walk_flags    the "move along the stick's heading" flag and the quick turn [RC2 0x83ef38], and
 *                 the target speed from the stick [0x83f064, 0x82d2a0]; RC1's own push block
 *                 (0xb1770) runs behind it as RC2's does
 *   walk_turn     the turn [0x83f1f0, 0x82d708], or strafe_turn() while strafing
 *                 RC1's lean calls (0xb1990) run behind it
 *   walk_speed    the quick turn's cap, RC2's acceleration and the slow-stick rule, and the step to
 *                 the target speed [0x83f33c..0x83f4bc]; RC1's velocity code (0xb1a48) runs behind it
 *
 * and in the walk transitions RC2's choice of the run jump [0x84d34c] replaces RC1's, and RC1's
 * quick turn from the pad's flick event (0xc0918) is taken out: RC2 starts its quick turns here.
 *
 * What changes against RC1:
 *   speed     the target speed is 5.95 units/s times the stick, not RC1's 0.9 or 5.7; 18 and 20
 *             units/s^2 up and down, not 7.5 and 8.5
 *   heading   below 1.7 units/s he moves along the stick's heading while he turns to it, until he
 *             is within 5 degrees; RC1 does so only from a standstill with the stick past 0.85, and
 *             drops it as soon as he runs. A reversal of the stick at full deflection starts a quick
 *             turn that also moves him along the new heading at once.
 *   turning   RC2's springs: stiffer and better damped than RC1's
 *
 * The flags are RC1's own hero fields, so the rest of RC1's code sees them: +0x3b8 (move along the
 * heading; RC2 +0x40c, read by the velocity code at 0xb1a48) and +0x3be (quick turn; RC2 +0x40f).
 */
#include "game.h"
#include "mod.h"

/* ---- Tuning ------------------------------------------------------------------------------------
 * Speeds in units/s, accelerations in units/s^2, angles in rad, turn speeds in rad/s; the game
 * works per frame at 60 frames per second. In brackets: RC2's code, and offsets in its hero table
 * 0x838f7c (+0x..) and its turn function's table 0x82d6c8 (t+0x..). */
#define RUN_SPEED       5.95f       /* target speed = this * stick                 [0x82d2ec, 0x131a61c] */
#define WALK_73_RATE    0.8f        /* state 0x73 (RC2 0x70): target * this, at least  [0x82d314] */
#define WALK_73_MIN     2.5f
#define STOP_STICK      0.2f        /* below: target 0, heading held                         [0x83f07c] */
/* Flags                                                                               [0x83ef38] */
#define ALONG_SPEED     1.7f        /* below this speed he moves along the stick's heading    [+0x54] */
#define ALONG_DONE      0.08726646f /* ... until the turn left is under 5 degrees              [0x83f020] */
#define QUICK_REVERSE   2.0943952f  /* quick turn: turn over 120 degrees at full stick, below ALONG_SPEED */
#define QUICK_SWING     1.9198622f  /* or the stick swung over 110 degrees from 7 to 1 frames ago [+0x218] */
#define QUICK_SWING_TURN 1.2217305f /* ... with a turn over 70 degrees left                     [0x83eff0] */
#define QUICK_DONE      0.06981317f /* a quick turn ends within 4 degrees                       [0x83f048] */
/* Turning: the springs are below. With the stick short of full, stiffness TURN_K * stick and max
 * speed TURN_MAX * stick, each at least its _MIN and at most the full stick's.            [0x82d708] */
#define TURN_K          0.02f       /*                                                     [t+0x4, t+0x8] */
#define TURN_K_MIN      0.01f
#define TURN_MAX        3.8397243f  /*                                                    [t+0x10, t+0x14] */
#define TURN_MAX_MIN    1.9198622f
#define AIM_STICK       0.95f       /* aiming (below): the stiff spring over this stick, else the low one [0x82d880] */
/* Speed */
#define ACCEL           18.0f       /* towards a higher target, times the factor below      [+0x1fc] */
#define DECEL           20.0f       /* towards a lower one                                  [+0x200] */
#define QUICK_DECEL     17.0f       /*   during a quick turn                                [+0x22c] */
#define FACTOR_STICK    0.9f        /* factor 0 while the turn is over FACTOR_TURN, unless the stick */
#define FACTOR_TURN     0.7853982f  /* is at FACTOR_STICK and there is no gun or a quick turn [0x83f2d8] */
#define CAP_SPEED       5.95f       /* quick turn: target at most CAP_SPEED - turn * CAP_PER_TURN */
#define CAP_PER_TURN    (5.5f * 0.31830987f)  /* per rad                    [0x83f33c, +0x220, +0x40 * +0x224] */
#define QUICK_SCALE     2.6179938f  /* quick turn: target * (QUICK_SCALE - turn), within 0.2..1  [+0x228] */
#define QUICK_SCALE_MIN 0.2f
#define SLOW_STICK      0.75f       /* stick under this and turn over SLOW_TURN: target          */
#define SLOW_TURN       0.61086524f /* times 1 - SLOW_RATE * turn                     [0x83f400, +0x70] */
#define SLOW_RATE       0.4f
#define RUN_JUMP_STICK  0.85f       /* a jump from the walk is the run jump above this, not strafing [0x84d34c] */
/* The one read of the ground deceleration (hero table +0x298, now RC2's 20) that keeps RC1's 8.5,
 * as RC2's does from its own table entry: walk_hooks.s, speed_threshold_stub. [0x83e888, +0x204] */
const float g_walk_threshold = 8.5f;

/* approach_angle's springs: stiffness, damping, max speed (given in rad/s, kept per frame) */
struct spring {
    float k, d, max;
};
#define SPRING(k, d, max) {k, d, (max) / 60}
static const struct spring run_spring = SPRING(0.02f, 0.17f, 9.948377f);      /* running: gait 1, no quick
                                                                     turn, no gun [+0xec, +0xdc, +0x21c] */
static const struct spring full_spring = SPRING(0.022f, 0.27f, 5.5850534f);   /* full stick [t+0x0, +0x18, +0xc] */
static const struct spring quick_spring = SPRING(0.035f, 0.27f, 11.693706f);  /* quick turn [t+0x1c, +0x20] */
static const struct spring aim_spring = SPRING(0.017f, 0.18f, 8.726646f);     /* aiming [t+0x24, +0x28, +0x2c] */
static const struct spring aim_low_spring = SPRING(0.009f, 0.15f, 4.712389f); /* k, max times the stick [t+0x30..] */

/* RC1's guns that aim while they fire (its turn function 0x9bf10); RC2's list (0x82d830) has its
 * own weapons' numbers. */
#define AIM_ITEM_A      0x0F
#define AIM_ITEM_B      0x10
#define AIM_ITEM_C      0x15

/* Hero fields only this file uses */
#define H_ALONG_HEADING 0x3B8       /* u16: move along the heading, not the facing [RC2 +0x40c] */
#define STATE_WALK_73   0x73        /* RC1's other state with this walk handler (+0x20a9) [RC2 0x70] */

/* The pad keeps the stick's angle for the last 30 frames (written at 0x4ebdf4) */
#define PAD_HISTORY_NOW   0x8E      /* s16: the slot of this frame */
#define PAD_HISTORY_COUNT 0x90      /* frames kept so far */
#define PAD_STICK_ANGLES  0x158     /* 30 floats */

static float factor;                /* acceleration factor, from walk_turn to walk_speed (f31 in RC2) */
static float held_heading;          /* his facing on the last frame with the stick out [+0x408] */

static float absf(float v) { return __builtin_fabsf(v); }

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* The stick's angle so many frames ago [RC2 0xb946f0] */
static float stick_angle(int frames)
{
    int count = MEM(int, PAD + PAD_HISTORY_COUNT);
    if (frames > count)
        frames = count;
    int slot = (MEM(s16, PAD + PAD_HISTORY_NOW) - frames + 30) % 30;
    return MEM(float, PAD + PAD_STICK_ANGLES + 4 * slot);
}

/* ---- Walk handler, in place of RC1's 0xb16c4..0xb176c ------------------------------------------ */

void walk_flags(void)
{
    float stick = absf(hero_f(H_STICK));
    float turn = absf(hero_f(H_TURN));     /* left from the last frame */
    int full = stick >= 1.0f;               /* the stick is clamped to 1 (hero_main 0x8b868) */

    if (hero_i(H_STATE_TIMER) == 0)
        held_heading = hero_f(H_YAW);       /* as RC2's set_state does for the walk [0x845c80] */

    if (!g_strafe) {                        /* [0x83ef38] */
        if (hero_f(H_MOVE_SPEED) < ALONG_SPEED / 60) {
            hero_u16(H_ALONG_HEADING) = 1;
            if (turn > QUICK_REVERSE && full)
                hero_u16(H_QUICK_TURN) = 1;
        }
        if (angle_gap(stick_angle(1), stick_angle(7)) > QUICK_SWING && turn > QUICK_SWING_TURN && full) {
            hero_u16(H_QUICK_TURN) = 1;
            hero_u16(H_ALONG_HEADING) = 1;
        }
        if (turn < ALONG_DONE)
            hero_u16(H_ALONG_HEADING) = 0;
    }
    if (hero_u16(H_QUICK_TURN) && turn < QUICK_DONE)
        hero_u16(H_QUICK_TURN) = 0;

    if (stick < STOP_STICK) {               /* [0x83f094] */
        hero_f(H_TARGET_SPEED) = 0;
        hero_f(H_HEADING) = held_heading;
        return;
    }
    stick_target(0, 0, 1.0f);               /* target = stick, heading from the stick [0x82d2a0] */
    if (hero_f(H_TARGET_SPEED) > 0)
        hero_f(H_TARGET_SPEED) = RUN_SPEED / 60 * stick;
    if (hero_i(H_STATE) == STATE_WALK_73) {
        float target = hero_f(H_TARGET_SPEED) * WALK_73_RATE;
        hero_f(H_TARGET_SPEED) = target < WALK_73_MIN / 60 ? WALK_73_MIN / 60 : target;
    }
    held_heading = hero_f(H_YAW);
}

/* ---- In place of RC1's turn 0xb1804..0xb198c ----------------------------------------------------- */

/* A gun out while it fires, with a gun that aims [0x82d82c]. */
static int aiming(void)
{
    int item = hero_i(H_HAND_READY) == 2 ? hero_i(H_HAND_ITEM) : -1;
    return (item == AIM_ITEM_A || item == AIM_ITEM_B || item == AIM_ITEM_C) && hero_u8(H_OVERRIDE_ANIM);
}

void walk_turn(void)
{
    factor = 1;
    if (g_strafe) {
        strafe_turn();
        return;
    }
    g_strafe_heading = hero_f(H_YAW);       /* [0x83f218] */

    float stick = absf(hero_f(H_STICK));
    int quick = hero_u16(H_QUICK_TURN);
    int gun = hero_u8(H_OVERRIDE_GUN);
    struct spring s = run_spring;           /* [0x83f230] */
    if (hero_i(H_GAIT) != 1 || quick || gun) {
        if ((stick < FACTOR_STICK || (gun && !quick)) && absf(hero_f(H_TURN)) > FACTOR_TURN)
            factor = 0;                     /* [0x83f2d8] */
        s = full_spring;                    /* [0x82d708] */
        if (stick < 1.0f) {
            s.k = clampf(TURN_K * stick, TURN_K_MIN, full_spring.k);
            s.max = clampf(TURN_MAX / 60 * stick, TURN_MAX_MIN / 60, full_spring.max);
        }
        if (quick)
            s = quick_spring;
        else if (aiming()) {
            s = stick > AIM_STICK ? aim_spring : aim_low_spring;
            if (stick <= AIM_STICK) {
                s.k *= stick;
                s.max *= stick;
            }
        }
    }
    if (hero_u8(H_MAGNET_WALK))
        magnet_turn(0, 0, 0, 0, s.k, s.d, s.max);      /* it sets the turn left itself */
    else
        hero_f(H_TURN) = approach_angle(&hero_f(H_YAW), 0, &hero_f(H_TURN_SPEED), 0, 0, 0, 0,
                                        hero_f(H_HEADING), s.k, s.d, s.max);
}

/* ---- In place of RC1's 0xb1998..0xb1a44, behind the lean calls ------------------------------------ */

void walk_speed(void)
{
    float turn = hero_f(H_TURN);
    float target = hero_f(H_TARGET_SPEED);
    float decel = DECEL / 3600;
    if (hero_u16(H_QUICK_TURN)) {
        float cap = CAP_SPEED / 60 - absf(turn) * (CAP_PER_TURN / 60);    /* [0x83f33c] */
        if (cap < 0)
            cap = 0;
        if (target > cap)
            target = cap;
        target *= clampf(QUICK_SCALE - turn, QUICK_SCALE_MIN, 1);         /* signed, as in both games [0x83f3c4] */
        decel = QUICK_DECEL / 3600;
    }
    if (absf(hero_f(H_STICK)) < SLOW_STICK && absf(turn) > SLOW_TURN) {   /* [0x83f400] */
        float slow = 1 - SLOW_RATE * absf(turn);
        target *= slow > 0 ? slow : 0;
    }
    hero_f(H_TARGET_SPEED) = target;
    approach(&hero_f(H_SPEED), target,                                    /* [0x83f464] */
             absf(target) > absf(hero_f(H_SPEED)) ? ACCEL / 3600 * factor : decel);
}

/* ---- Walk transitions: the jump from the walk ------------------------------------------------------ */

/* 1 = the run jump (state 9), 0 = the jump (7) [0x84d34c]. RC1 also wanted the run gait, its heading
 * within 60 degrees of his facing and 85% of the run speed last frame (0xc0534). */
int walk_run_jump(void)
{
    return absf(hero_f(H_STICK)) > RUN_JUMP_STICK && !g_strafe;
}
