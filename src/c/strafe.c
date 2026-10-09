/* RC2's strafing for RC1 HD, on L2 or R2.
 *
 * Port of Joe-GH-18/RAC-1-Mods' strafing (Apache 2.0). RC2 keeps a timer at hero +0x1454 that is
 * set while the strafe button is held (hero_main 0x816aa0) and tests it all through its hero code;
 * RC1's hero code is the older version of the same functions, so each of those tests is a hook at
 * the matching RC1 instruction (src/asm/strafe_hooks.s). Like upstream it strafes on L2 or R2;
 * STRAFE_BUTTONS below picks the buttons.
 *
 *   strafe_frame      every frame in front of hero_main: the button and the stick's direction
 *   walk              Ratchet turns to the camera's yaw and moves along the stick's heading, with
 *                     RC2's damping when left and right are reversed (RC2 0x82c238); walk.c calls
 *                     strafe_turn() in place of its own turn, as RC2's walk handler does (0x83f1f0)
 *   idle and walk     holding the button turns him to the camera through the walk state (RC2 0x84bd80),
 *                     which lasts until he is aligned (0x84d770); letting go of the stick while
 *                     strafing goes straight to idle (0x84d7f0; move.c caps the speed), not to
 *                     RC1's skid
 *   body twist        moving sideways twists his body through joint controllers 0 and 1 (0x830a5c)
 *   flips             a jump with the stick sideways or back is RC1's flip state 0xb, chosen and
 *                     aimed by the strafe direction (0x835e64, 0x833698, 0x83388c, 0x848810, 0x82f91c)
 *   air               he keeps facing the camera and moves along the stick's heading (0x83007c, 0x8429e4)
 *   backwards walk    RC2's backwards walk animation (0x836018), only with BACK_ANIM_ADDR set
 *
 * On foot (states up to 0x15, not on a magnetic surface) the strafe buttons are taken out of the
 * pad words every frame, so they only strafe; in RC1 L2 would do what L1 does (first-person view)
 * and R2 what R1 does (crouch). Strafing is off while a charge is on: the charge moves him itself
 * (the buttons are still taken from the game meanwhile), and in a long jump.
 *
 * At the start of a long jump (state 0xa with the Heli-Pack, 0x10 with the Thruster-Pack) R2 is
 * RC1's crouch again, for runs that alternate R1 and R2 to chain long jumps and side flips. RC1
 * cancels a long jump into a side flip in its first LONG_JUMP_CANCEL frames on the ground, with a
 * crouch button held and a flick of the stick (0xc3ec4), and makes the next long jump out of the
 * side flip with a fresh press of a crouch button (0xc40e8, from its pad history, which still has
 * the strafe buttons): the R2 press that made the long jump has to be that held crouch. After those
 * frames R2 strafes again, still held or not, and L2 always strafes (RC1's L2 is the first-person
 * view).
 */
#include "game.h"
#include "mod.h"

/* ---- Switch ------------------------------------------------------------------------------------
 * The Makefile reads this line too (it decides whether src/asm/optional/back_walk.s goes into the
 * patch), and "make BACK_ANIM_ADDR=..." overrides it for one build. RC2's acceleration and turning,
 * once the switch RC2_ACCELERATION, are always on now: walk.c. */

/* RC2's backwards walk: the address of RC2's Ratchet animation 0x14 (30 frames, 38080 bytes),
 * which the patch writes there (data/back_anim.bin, made by tools/make_data.py with its frame
 * pointers relocated for this address); the hook src/asm/optional/back_walk.s plays it as upstream
 * does. 0 = none: the walk animation chooser is not hooked, and walking backwards plays RC1's own
 * walk/run cycle for his speed, as when not strafing. */
#ifndef BACK_ANIM_ADDR
#define BACK_ANIM_ADDR  0x945000
#endif

/* ---- Tuning ------------------------------------------------------------------------------------
 * Speeds in units per second, accelerations in units per second squared; the game works per frame
 * at 60 frames per second. Numbers in brackets are where RC2 has them (RC2.ppu.elf). */
#define STRAFE_BUTTONS  (BUTTON_L2 | BUTTON_R2)   /* either one strafes, as upstream */
#define LAST_ON_FOOT    0x15        /* states up to this: idle, walk, skid, crouch, the jumps, the wrench */
/* At the start of a long jump the strafe buttons that are RC1's crouch buttons crouch (see the top
 * of the file), for as long as RC1 can cancel it into a side flip: its buffer for flips with the
 * stick sideways or back (0xa1e78). */
#define CROUCH_BUTTONS  (BUTTON_R1 | BUTTON_R2)   /* RC1's hero code tests them together */
#define LONG_JUMP_CANCEL 12         /* frames */
/* The walk turns him to the camera on approach_angle's spring                        [0x82c220] */
#define TURN_STIFFNESS  0.037f
#define TURN_DAMPING    0.25f
#define TURN_MAX_SPEED  12.217304f  /* rad/s */
/* Reversing: left within REVERSE_FRAMES of going right multiplies the speed by REVERSE; right within
 * them of going left by (4 - frames left) / 4                                         [0x82c220] */
#define REVERSE_FRAMES  5
#define REVERSE         -0.25f
/* Facing the camera                                                [transition table 0x84ac7c] */
#define ALIGN_IDLE      0.17453292f /* rad: holding the button further off than this turns him (idle -> walk) [+0x18] */
#define ALIGN_WALK      0.034906585f /* rad: the walk state lasts until he is this close          [+0x48] */
/* Body twist                                                                          [0x8308f8] */
#define TWIST_STICK     0.15f       /* stick deflection that counts as moving */
#define TWIST_RATE_A    0.037f      /* joint controller 0 +0xa4 */
#define TWIST_RATE_B    0.27f       /* joint controllers 0 and 1 +0xa8 */
#define TWIST_RATE_C    0.025f      /* joint controller 1 +0xa4 */
/* Flips */
#define FLIP_STICK      0.9f        /* a jump becomes a flip above this stick deflection       [0x835e7c] */
#define FLIP_AIM_STICK  0.3f        /* a flip goes along the strafe heading above this         [0x848838] */
#define FLIP_SPEED      5.0f        /* start speed, then towards stick * this     [0x844784 +0x194, 0x82f2c0 +0x3c] */
#define FLIP_MIN_SPEED  2.0f        /*                                                    [0x82f2c0 +0x48] */
#define FLIP_ACCEL      37.0f       /*                                                    [0x82f2c0 +0x4c] */
/* Backwards walk (BACK_ANIM_ADDR)                                                     [0x835f70] */
#define BACK_SLOT       165         /* a free slot of Ratchet's animation table (charge.c, keep_anim_slot) */
#define BACK_BLEND      11.0f       /* frames */
#define BACK_RATE       25.0f       /* animation speed = ground speed per frame * this, within: */
#define BACK_RATE_MIN   0.7f
#define BACK_RATE_MAX   2.2f
#define GAIT_BLEND      7.0f        /* frames back to the walk/run animation */

/* The stick's direction while strafing, as RC1's flip types: left, right, forward, back. */
#define DIR_LEFT        0
#define DIR_RIGHT       1
#define DIR_FORWARD     2
#define DIR_BACK        3

/* What strafe_walk_align tells walk_align_stub (src/asm/strafe_hooks.s) */
#define WALK_RC1        0           /* RC1's own walk transitions */
#define WALK_STAY       1           /* stay in the walk state */
#define WALK_TO_IDLE    2           /* to idle, behind RC1's wait */

/* ---- State (zero when the game boots) ---------------------------------------------------------
 * In brackets RC2's hero offsets. The g_ ones are read by the stubs. */
int g_strafe;                   /* strafing this frame [+0x1454, a timer that is only ever 0 or 1 here] */
int g_strafe_held;              /* a strafe button held on foot this frame, taken from the game */
int g_strafe_dir;               /* DIR_*                                                 [+0x1457] */
float g_strafe_heading;         /* the heading he moves along                            [+0x1458] */
int g_strafe_flip;              /* the current flip is a strafe flip                      [+0x52d] */
static int backwards;           /* moving backwards                                      [+0x1456] */
static float turn_speed;        /* the turn's velocity, kept between frames              [+0x145c] */
static int since_right;         /* frames since going right (counts down from 5)         [+0x146c] */
static int since_left;          /* frames since going left                               [+0x146e] */
static int twisted;             /* the body twist is applied */

/* Constants the stubs read */
const float g_strafe_flip_aim = FLIP_AIM_STICK;
const float g_strafe_flip_start = FLIP_SPEED / 60;

static float absf(float v) { return __builtin_fabsf(v); }

/* No stick at all: the stick's heading is then his own yaw, while the deflection +0x229c takes some
 * frames to fall and keeps him walking, so the last heading is kept; otherwise he would run forwards
 * out of a strafe backwards. */
static int stick_at_rest(void)
{
    return absf(hero_f(H_STICK_X)) + absf(hero_f(H_STICK_Y)) == 0;
}

/* The game's count-down (0x4fff60): one less, never below 0. */
static int count_down(int frames)
{
    return frames > 1 ? frames - 1 : 0;
}

static void untwist(void)
{
    if (!twisted)
        return;
    twisted = 0;
    joint_f(0, J_TARGET_Z) = 0;
    joint_f(1, J_TARGET_Z) = 0;
}

/* ---- Every frame -------------------------------------------------------------------------------- */

/* The quarter of atan2(y, x) the stick is in [RC2 0x82cf70]; at rest it counts as right. */
static int stick_direction(void)
{
    float x = hero_f(H_STICK_X), y = hero_f(H_STICK_Y);
    float ax = absf(x), ay = absf(y);
    if (ax + ay == 0)
        return DIR_RIGHT;
    if (y > ax)
        return DIR_BACK;
    if (x > ay)
        return DIR_RIGHT;
    if (-x > ay)
        return DIR_LEFT;
    return DIR_FORWARD;
}

/* In front of hero_main [RC2 0x816ac4]. */
void strafe_frame(void)
{
    keep_anim_slot(BACK_SLOT, BACK_ANIM_ADDR);      /* see charge.c */
    u32 state = hero_i(H_STATE);

    /* At the start of a long jump R2 crouches (see the top of the file) */
    int long_jump = state == STATE_LONG_JUMP || state == STATE_THRUSTER_JUMP;
    u32 strafe_buttons = STRAFE_BUTTONS;
    if (long_jump && hero_i(H_STATE_TIMER) < LONG_JUMP_CANCEL)
        strafe_buttons &= ~CROUCH_BUTTONS;
    int held = 0;
    if ((pad_word(PAD_HELD) & strafe_buttons) && state <= LAST_ON_FOOT && !hero_u8(H_MAGNET_WALK)) {
        pad_word(PAD_HELD) &= ~strafe_buttons;      /* on foot they only strafe */
        pad_word(PAD_PRESSED) &= ~strafe_buttons;
        held = 1;
    }
    g_strafe_held = held;
    g_strafe = held && !long_jump && g_charge_frames <= 0;
    if (!g_strafe) {
        backwards = 0;
        untwist();
        return;
    }
    g_strafe_dir = stick_direction();
    backwards = g_strafe_dir == DIR_BACK && hero_f(H_STICK) > TWIST_STICK;
}

/* ---- Walk state (walk handler 0xb1630) ------------------------------------------------------------ */

/* In place of RC1's turn, while strafing [RC2 0x82c238]: he turns to the camera's yaw, and the
 * stick's heading is kept to move along. */
void strafe_turn(void)
{
    if (!stick_at_rest())
        g_strafe_heading = hero_f(H_HEADING);
    since_right = count_down(since_right);
    since_left = count_down(since_left);

    float factor = 1.0f;
    if (hero_u8(H_CHARACTER) == 0) {
        if (g_strafe_dir == DIR_LEFT) {
            since_left = REVERSE_FRAMES;
            if (since_right)
                factor = REVERSE;
        } else if (g_strafe_dir == DIR_RIGHT) {
            int left = since_left;
            since_right = REVERSE_FRAMES;
            if (left)
                factor = (4 - left) * 0.25f;
        }
    }
    hero_f(H_SPEED) *= factor;

    float yaw = camera_yaw();
    hero_f(H_HEADING) = yaw;
    hero_f(H_TURN_SPEED) = turn_speed;
    approach_angle(&hero_f(H_YAW), 0, &hero_f(H_TURN_SPEED), 0, 0, 0, 0,
                   yaw, TURN_STIFFNESS, TURN_DAMPING, TURN_MAX_SPEED / 60);
    hero_f(H_TURN) = 0;
    turn_speed = hero_f(H_TURN_SPEED);
    hero_f(H_TURN_SPEED) = 0;
}

/* ---- Idle and walk transitions (transition function 0xbe228) -------------------------------------- */

/* [RC2 0x82c184] He is not facing the camera yet. */
static int unaligned(float threshold)
{
    return angle_gap(hero_f(H_YAW), camera_yaw()) >= threshold;
}

/* Idle, while a strafe button is held [RC2 0x84bd80]: 1 = on to the walk state, to turn to the camera. */
int strafe_idle_align(void)
{
    return unaligned(ALIGN_IDLE);
}

/* Walk, while a strafe button is held [RC2 0x84d770, 0x84d7f0]; walk_on is RC1's stick deflection for walking on.
 * Not yet facing the camera: he stays in the walk state. Strafing with the stick let go: to idle,
 * where RC1 would skid; the walk -> idle code caps his slide and speed at 1.5 units/s as RC2's
 * does (move.c), as for every stop. */
int strafe_walk_align(float walk_on)
{
    if (unaligned(ALIGN_WALK))
        return WALK_STAY;
    if (!g_strafe || hero_f(H_STICK) >= walk_on)
        return WALK_RC1;
    return WALK_TO_IDLE;
}

/* ---- Jumps and flips -------------------------------------------------------------------------- */

/* Jump chooser 0xa4950, while strafing [RC2 0x835e64]: 1 = a flip, with the stick pushed anywhere
 * but forward. */
int strafe_jump_flips(void)
{
    return g_strafe_dir != DIR_FORWARD && hero_f(H_STICK) > FLIP_STICK;
}

/* Air function 0x9de38, in a strafe flip [RC2 0x82f91c]: the flip's speed follows the stick. */
void strafe_flip_air(void)
{
    float target = hero_f(H_STICK) * (FLIP_SPEED / 60);
    if (target < FLIP_MIN_SPEED / 60)
        target = FLIP_MIN_SPEED / 60;
    approach(&hero_f(H_FLIP_SPEED), target, FLIP_ACCEL / 3600);
    hero_f(H_SPEED) = hero_f(H_MOVE_SPEED);
}

/* Air function, in front of its default turn, while strafing [RC2 0x83007c]: he turns to the
 * camera, and the stick's heading is kept to move along (air_turned_stub puts it back). */
void strafe_air_turn(void)
{
    if (!stick_at_rest())
        g_strafe_heading = hero_f(H_HEADING);
    hero_f(H_HEADING) = camera_yaw();
}

/* ---- Body twist (lean function 0x9f300) ----------------------------------------------------------- */

/* While strafing [RC2 0x830a5c]: moving sideways twists his body by the angle between the way he
 * goes and the way he faces. 1 = done, the lean function returns at once. */
int strafe_lean(void)
{
    if (hero_f(H_STICK) > TWIST_STICK && hero_i(H_STATE) != STATE_FLIP && !backwards) {
        twisted = 1;
        joint_f(0, J_STIFFNESS) = TWIST_RATE_A;
        joint_f(0, J_DAMPING) = TWIST_RATE_B;
        joint_f(1, J_DAMPING) = TWIST_RATE_B;
        joint_f(1, J_STIFFNESS) = TWIST_RATE_C;
        float twist = angle_diff(g_strafe_heading, hero_f(H_YAW));
        joint_f(0, J_TARGET_Z) = twist;
        joint_f(1, J_TARGET_Z) = -twist;
        return 1;
    }
    untwist();
    return 0;
}

/* ---- Backwards walk (walk animation chooser 0xa4a60), src/asm/optional/back_walk.s ------------- */
#if BACK_ANIM_ADDR
#define ANIM_RC1        0           /* the chooser's own choice */
#define ANIM_DONE       1           /* nothing more: the backwards walk is on */
#define ANIM_RATE       2           /* only its animation speed: the gait animation was put back */

int g_back_walk;                /* the backwards animation is on */

/* While strafing or while the backwards animation is on [RC2 0x836018]. */
int strafe_walk_anim(void)
{
    void *moby = hero_moby();
    if (g_strafe && backwards) {
        g_back_walk = 1;
        if (moby_u8(moby, MOBY_ANIM_ID) != BACK_SLOT)
            hero_set_anim(BACK_SLOT, 0, BACK_BLEND);
        float rate = hero_f(H_SPEED) * BACK_RATE;
        hero_f(H_ANIM_RATE) = rate < BACK_RATE_MIN ? BACK_RATE_MIN : rate > BACK_RATE_MAX ? BACK_RATE_MAX : rate;
        return ANIM_DONE;
    }
    if (!g_back_walk)
        return ANIM_RC1;
    g_back_walk = 0;
    hero_set_anim(hero_i(H_GAIT) + 3, 0, GAIT_BLEND);
    return ANIM_RATE;
}
#endif
