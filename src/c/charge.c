/* RC2's Charge Boots for RC1 HD.
 *
 * Port of Joe-GH-18/RAC-1-Mods' Charge Boots (Apache 2.0), which ports RC2's charge state (hero
 * state 0x7b, handler in RC2's 0x8392e0) using RC1's counterparts of the functions it calls.
 *
 * RC1 has no charge state, so the charge runs while Ratchet is in his walk state: the frame hook
 * (mod_frame, in front of hero_main) reads R1; the transition function starts charges and runs
 * RC2's charge transitions in place of the walk state's own (charge_trans, from trans_stub in
 * charge_hooks.s), at the same point of the frame as RC2, behind the state's physics and the
 * weapon chooser; while a charge is on, mod_update moves him instead of hero_update. The walk state's type (1) is what makes the game play a
 * weapon's animation on his upper body only, as during RC2's charge.
 *
 * Weapons: RC2's gloves and guns fire from their own code while he charges, and that ends the
 * charge into a skid that keeps his speed. RC1 fires them through its weapon/gadget chooser, which
 * runs during a charge for every weapon that fires without a change of state; gloves then always
 * make the moving throw (glove_throw_stub). After a charge the skid slows at RC2's rate, not RC1's.
 * A gun held while it fires (RC1's gun flag, RC2's +0x22ba: Blaster, Pyrocitor, Tesla Claw, Suck
 * Cannon, Morph-O-Ray, RYNO) turns the skid into RC1's gun stance, which now stops him as RC2's
 * does (move.c); the others (gloves, Devastator) leave him skidding.
 *
 * After the charge it works as RC2 with the Black Label re-patch (isaka & robo), which takes out the
 * 30 frames in which RC2 hides the buttons from Ratchet after a charge. RC1 never hid them, so here
 * that is RC1's own behaviour: during the skid's lock (SKID_LOCK) he can throw, swing the wrench,
 * shoot and charge again; only jumping, crouching and walking off wait for the lock, as in both
 * games. A glove that ends a charge leaves the skid straight into the glove's standing throw (state
 * 0x23, RC2's 0x22), which keeps his speed and can be jumped out of (X, or R1 + X) partway through,
 * before the lock is over. While he slides on (the skid, a crouch, the throw), he slows at RC2's
 * rates.
 */
#include "game.h"
#include "mod.h"

/* ---- Tuning ------------------------------------------------------------------------------------
 * Speeds in units per second, accelerations in units per second squared; the game works per frame
 * at 60 frames per second. Numbers in brackets are RC2's (float table 0x838f7c / transition table
 * 0x84ac7c of RC2.ppu.elf). */
#define CHARGE_BUTTON   BUTTON_R1
#define TAP_WINDOW      30          /* frames between the two presses                   [30.5] */
#define MIN_FRAMES      60          /* releasing R1 only ends a charge after this long          */
#define BURST_FRAMES    60          /* burst speed and slow steering for this long              */
#define BURST_SPEED     30.0f       /*                                                   [30]   */
#define CRUISE_SPEED    11.5f       /*                                                   [11.5] */
#define ACCEL           100.0f      /* towards a higher target speed                     [+0x234] */
#define DECEL           15.0f       /* towards a lower one                               [+0x158] */
#define TURN_RATE       1.2217f     /* rad/s at full stick                               [+0x7c] */
#define BURST_TURN      0.1f        /* steering scale during the burst                   [+0x1a0] */
#define STEER_SMOOTH    0.17f       /* per-frame approach of the steering input          [+0xdc] */
#define STEER_DEADZONE  0.2f
/* Hover: his height springs towards the ground plus HOVER_HEIGHT, as in RC2's handler, with no
 * gravity; he moves level, so on slopes the hover (and the ground snap, given RC2's springs in
 * src/patch.txt) follows the ground. */
#define HOVER_HEIGHT    0.17f
#define HOVER_STIFFNESS 0.04f
#define HOVER_DAMPING   0.3f
#define HOVER_MAX_SPEED 7.5f        /* units/s                                           [+0x23c] */
#define HOVER_SNAP      0.01f       /* snap when closer than the max speed times this    [0xb6871c] */
#define GROUNDED_HEIGHT 0.4f        /* within this of the ground he counts as standing for the surface code */
/* Crates ahead: a sweep centred this far ahead (times his motion) with this radius. */
#define SWEEP_AHEAD     3.0f        /*                                                   [+0xac] */
#define SWEEP_RADIUS    0.57f       /*                                                   [+0x240] */
/* Ending: R1 let go (after MIN_FRAMES), the Magneboots or an override animation -> skid. */
#define SKID_LOCK       30          /* frames the skid then blocks jump and crouch */
#define SKID_ANIM       6
#define SKID_ANIM_BLEND 11.0f
/* A glove that ends a charge is followed by the standing throw from the skid (RC2's glove case
 * [0x835934], on the next frame) only when the glove has been out more than GLOVE_DRAWN frames,
 * Circle was pressed in the last GLOVE_BUFFER frames or is held, and there is ammo left for it.
 * Otherwise the throw goes on as it began and he skids on, as with a weapon fired once. */
#define GLOVE_DRAWN     22
#define GLOVE_BUFFER    7
/* Sliding on after a charge, u/s^2 (RC1's own: skid 12, crouch 6.48). Standing slows at RC2's
 * 17.7 at all times (RC1's 12.6), as part of RC2's walking (walk.c); the glove's standing throw
 * slows at 35 in both games. */
#define SKID_DECEL      27.0f       /*                                                   [+0x3c] */
#define CROUCH_DECEL    27.0f
#define IDLE_DECEL      17.7f       /*                                                   [+0x58] */
/* Running into a wall -> bump state, when he moved less than this part of his velocity or the
 * probe finds a wall ahead. */
#define BUMP_RATIO      0.75f
#define PROBE_HEIGHT    1.0f
#define PROBE_REACH     1.25f

/* RC2's charge animation (Ratchet animation 139) and sounds, written by the patch (data/; addresses
 * as in src/patch.txt and tools/make_data.py). The animation goes into the free slot 134 of
 * Ratchet's model's animation table. */
#define ANIM_ADDR       0x940000
#define ANIM_SLOT       166         /* see keep_anim_slot */
#define ANIM_BLEND      -2.0f
#define SOUND_ADDR      0x931400    /* +0 loop definition, +0x20 end definition, +0x40 bank */
#define SOUND_LOOP      ((void *)SOUND_ADDR)
#define SOUND_END       ((void *)(SOUND_ADDR + 0x20))
#define SOUND_BANK      ((const void *)(SOUND_ADDR + 0x40))
#define SOUND_LOOP_FLAGS 4

/* RC2's lean while charging [0x83f914..0x83f9b4]: the springs of three of Ratchet's body joint
 * controllers (game.h's JOINTS; RC2's block 0x13188a0), and their targets, which lean his body
 * into the steering. */
static const struct { u8 joint; float stiffness, damping; } lean_springs[] = {
    {0, 0.017f, 0.3f}, {2, 0.025f, 0.3f}, {3, 0.03f, 0.27f},
};
#define LEAN_GAIN       27.0f
#define LEAN_LIMIT      1.7f

/* RC2's camera while charging [RC2 0xad832c]: the camera turns to behind him and its look-at point
 * follows him on a stiffer spring. RC2's camera mode logic does this for every frame the hero is in
 * the charge state; RC1's camera knows no charge (it runs in the walk state here), so
 * charge_camera() does the same on RC1's fields of the third-person camera (matched to RC2's by
 * their code; game.h). */
#define CAM_FOLLOW_K    0.04f       /* RC1's and RC2's default 0.015 */
#define CAM_FOLLOW_D    0.2f
#define CAM_TURN_SPEED  0.7853982f  /* rad per frame at full turn input: the auto-turn eases in */
#define CAM_DISTANCE    3.0f
#define CAM_DISTANCE_K  0.003f

/* Foot trails: two layers from each foot. */
static const u8 trail_joints[] = {0x16, 0x17};
static const vec4 trail_drift = {-5.0f / 60, 0, 0, 0};
static const u32 trail_layers[2][4] __attribute__((aligned(16))) = {
    {0x40ff8020, 0x00cf6000, 0x00640046, 0x00000019},
    {0x20ffff00, 0x00008000, 0x005a002d, 0x0000000f},
};

/* ---- State (zero when the game boots) ------------------------------------------------------- */
int g_charge_frames;            /* frames of charge movement so far + 1, 0 = none */
int g_charge_pending;           /* a second tap: the charge starts at this frame's transitions */
int g_charge_slide;             /* he is sliding on after a charge: skid, crouch, glove throw, idle */
int g_moving_throw;             /* gloves make the moving throw (glove_throw_stub) */
/* Read by the stubs in charge_hooks.s: the first two while g_charge_slide is set */
const float g_skid_decel = SKID_DECEL;
const float g_crouch_decel = CROUCH_DECEL;
const float g_idle_decel = IDLE_DECEL;

static int r1_last;             /* R1 held last frame */
static int tap_window;          /* frames left to press R1 again */
static int old_throw;           /* a glove throw that was on when the charge started */
static int circle_last;         /* Circle held last frame */
static int circle_since;        /* frames since Circle was pressed, + 1; 0 = long ago */
static int weapon_used;         /* a weapon was used by the start of this frame */
static int from_look;           /* in look mode, or in the crouch that came out of it */
static float steer;             /* smoothed steering input */
static float hover_velocity;    /* kept from one charge to the next, as RC2's +0xa00 */
static int sound_bank;          /* 0 = not loaded, -1 = failed */
static int loop_voice;          /* voice slot + 1 of the loop, 0 = none */
static int end_sound_due;       /* play the end sound when the loop stops */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float absf(float v) { return v < 0 ? -v : v; }

/* RC2's animations sit in slots of Ratchet's model's animation table past its 134 entries, in the
 * zeroed space before its mesh (+0x2e4, slots 134..166, in every level). Not at its start: RC1's
 * scenes lend a model animations there, one at the slot its count (+0xc) points at, and take them
 * back by zeroing that slot (0xe0230 adds, 0x161788 and 0x15fe00 remove; the removal runs on
 * pausing), so the first free slots come and go. These are the last two. They are kept filled every
 * frame, not only while they play: Ratchet's upper-body layers copy his animations (0xf0dd8) and can
 * keep one after it is over, to use it again later, and a null animation there crashes the game
 * (0xf0d18). Only on Ratchet's own model, and only into a slot that is empty or ours. */
void keep_anim_slot(int slot, u32 anim)
{
    void *moby = hero_moby();
    if (!moby || !anim || hero_u8(H_CHARACTER) != 0)
        return;
    u32 model = moby_ptr(moby, MOBY_MODEL);
    if (!model)
        return;
    volatile u32 *entry = &MEM(u32, model + MODEL_ANIMS + 4 * slot);
    if (*entry == 0 || *entry == anim)
        *entry = anim;
}

/* ---- Sound ------------------------------------------------------------------------------------ */

/* Keeps the charge loop playing on Ratchet; loads the bank the first time. */
static void charge_sound(void)
{
    if (sound_bank == 0) {
        int handle = bank_load(SOUND_BANK);
        sound_bank = handle ? handle : -1;
        MEM(int, (u32)SOUND_LOOP + SOUND_DEF_BANK) = sound_bank;
        MEM(int, (u32)SOUND_END + SOUND_DEF_BANK) = sound_bank;
    }
    if (sound_bank < 0)
        return;
    if (loop_voice && voice_u8(loop_voice, 4) != 0 && voice_u32(loop_voice, 8) == (u32)SOUND_LOOP)
        return;
    int slot = play_sound_def(SOUND_LOOP, SOUND_LOOP_FLAGS, hero_moby(), 0, 0x400);
    loop_voice = slot + 1;
    if (slot >= 0)
        voice_u32(loop_voice, 0x18) = (u32)hero_moby();   /* the voice follows Ratchet */
}

/* Stops the loop once nothing needs it, and plays the end sound if a charge ended. */
static void charge_sound_stop(void)
{
    if (!loop_voice)
        return;
    if (voice_u32(loop_voice, 8) == (u32)SOUND_LOOP)
        stop_voice(loop_voice - 1);
    loop_voice = 0;
    if (end_sound_due) {
        end_sound_due = 0;
        play_sound_def(SOUND_END, 0, hero_moby(), 0, 0x400);
    }
}

/* ---- Effects ----------------------------------------------------------------------------------- */

static void charge_trails(void)
{
    void *moby = hero_moby();
    for (int foot = 0; foot < 2; foot++) {
        vec4 pos;
        joint_pos(moby, trail_joints[foot], &pos);
        for (int layer = 0; layer < 2; layer++)
            trail(hero_moby(), &pos, &trail_drift, trail_layers[layer], 1);
    }
}

/* The game clears the targets after every update (0x7f608); RC2 leaves the springs as the charge
 * set them until another state sets its own (the walk's and the air's lean, 0x9f300), so nothing is
 * undone when a charge ends. Zeroing them, as this port once did, stopped the springs: a joint still
 * swinging carried on the long way round, his torso turning over through the floor. */
static void lean(float yaw_change)
{
    for (unsigned i = 0; i < sizeof lean_springs / sizeof lean_springs[0]; i++) {
        joint_f(lean_springs[i].joint, J_STIFFNESS) = lean_springs[i].stiffness;
        joint_f(lean_springs[i].joint, J_DAMPING) = lean_springs[i].damping;
    }
    float turn = clampf(yaw_change * LEAN_GAIN, -LEAN_LIMIT, LEAN_LIMIT);
    joint_f(0, J_TARGET_X) = -turn;
    joint_f(2, J_TARGET_Z) = turn * 0.5f;
    joint_f(3, J_TARGET_Z) = turn * 0.5f;
}

/* The camera update runs behind the hero's and resets all of these at its end (0x2eda8), so
 * nothing is undone when the charge ends; the distance then springs back on its own. */
static void charge_camera(void)
{
    u32 cam = MEM(u32, CAMERA + CAMERA_ACTIVE);
    if (!cam || MEM(u16, cam + CAM_TYPE) != 0)
        return;                     /* the third-person camera only, as in RC2 */
    u32 vars = MEM(u32, cam + CAM_VARS);
    MEM(float, vars + CV_FOLLOW_K) = CAM_FOLLOW_K;
    MEM(float, vars + CV_FOLLOW_D) = CAM_FOLLOW_D;
    MEM(float, vars + CV_TURN_SPEED) = CAM_TURN_SPEED;  /* the right stick still takes over */
    /* to behind him: the auto-turn eases its input in as 2t - t^2, t = error / 90 degrees. RC1 holds
     * it at 1 beyond 90 degrees, RC2 lets it fall again (to 0 at 180 degrees). */
    float t = absf(cam_auto_turn((void *)cam, (const vec4 *)((u32)hero_moby() + MOBY_FORWARD), 0)) * 0.63661975f;
    if (t > 1) {
        float input = 2 * t - t * t;
        MEM(float, vars + CV_TURN_INPUT) = MEM(float, vars + CV_TURN_INPUT) < 0 ? -input : input;
    }
    MEM(u16, vars + CV_DISTANCE_ON) = 1;
    MEM(float, vars + CV_DISTANCE) = CAM_DISTANCE;
    MEM(float, vars + CV_DISTANCE_K) = CAM_DISTANCE_K;
    MEM(u16, vars + CV_NO_PULL_IN) = 1;
}

/* Wrench damage to every crate within radius of centre, as the game's own crate sweeps do. */
void crate_sweep(const vec4 *centre, float radius)
{
    vec4 dir;
    vec_scale(&dir, hero_vec(H_MOTION), 7.0f);
    int count = collect(centre, 0, 0, hero_moby(), 0, radius);
    void **found = (void **)COLLECT_RESULTS;
    for (int i = 0; i < count; i++)
        if (is_crate(found[i]))
            deal_damage(found[i], hero_moby(), 0x10000, 0, hero_vec(H_POS), &dir, 1.0f);
}

/* ---- The charge ------------------------------------------------------------------------------ */

static void charge_off(void)
{
    g_charge_frames = 0;
}

/* ---- The Charge Boots as an item: the Grindboots' (item_hooks.s) -------------------------------- */

static int boots_owned(void)
{
    return item_owned(ITEM_CHARGE_BOOTS) != 0;
}

/* Where a charge may start [RC2 0x835c14]: from the idle state (state type 0), the walk and the
 * skid (type 1), a crouch (0xc), or a fall or jump (2 and 4) on a frame he is on the ground; not in
 * the first-person view or state 0x1d. And only as Ratchet, with the item in his hand ready, as
 * RC2's weapon chooser, which starts its charges, requires. */
/* RC2 never charges out of look mode (L1): neither RC1's own (state 1) nor the one a weapon in his
 * hand turns it into (STATE_WEAPON_LOOK). RC1's look mode goes into a crouch when L1 is let go with
 * R1 or R2 held [0xbff98], and a crouch may charge, so a crouch that came out of look mode may not
 * either (from_look). With L1 held that hop is also a buffer in the air: a glove's standing throw
 * ends into idle, which goes into look mode before its fall test [0xbebe0]. */
static int charge_may_start(int state)
{
    if (!boots_owned() || state == STATE_LOOK || state == STATE_WEAPON_LOOK || state == 0x1D ||
        from_look)
        return 0;
    if (hero_u8(H_CHARACTER) != 0 || hero_i(H_HAND_MOBY) == 0 || hero_i(H_HAND_READY) != 2)
        return 0;
    int type = hero_i(H_STATE_TYPE);
    return type == 0 || type == 1 || type == 0xC || ((type == 2 || type == 4) && hero_i(H_GROUND_FRAMES) != 0);
}

/* The charge runs in the walk state, with the speed he has. RC1's walk entry turns into the ice
 * state (0x2f), the wading walk (0x73) or the slide (0x79) when the surface asks for them
 * [0xb9124, 0xb91d0, 0xb90f0], and the charge would end at once; RC2's charge has none of that
 * [0x845fd8] and starts on ice or in water too. So those surface flags are off for that one call
 * (the surface code sets them again every frame). On ice the charge ends into RC1's ice state, as
 * RC2's ends into its own, with the charge's speed (set_state's skid case, 0xb95ac). */
static void charge_host(void)
{
    if (hero_i(H_STATE) == STATE_WALK)
        return;
    float speed = hero_f(H_SPEED);
    u8 slippery = hero_u8(H_SLIPPERY), slide = hero_u8(H_SLIDE_SURFACE), wading = hero_u8(H_WADING);
    hero_u8(H_SLIPPERY) = hero_u8(H_SLIDE_SURFACE) = hero_u8(H_WADING) = 0;
    hero_set_state(STATE_WALK, 0);
    hero_u8(H_SLIPPERY) = slippery;
    hero_u8(H_SLIDE_SURFACE) = slide;
    hero_u8(H_WADING) = wading;
    hero_f(H_SPEED) = speed;
}

static void charge_start(void)
{
    g_charge_frames = 1;
    g_charge_slide = 0;
    steer = 0;
    /* RC2's charge animation takes a glove's throw off the weapon flag, so the throw does not end
     * the charge; here the throw plays on (its bomb is still to come) and the flag is ignored. */
    old_throw = hero_u8(H_OVERRIDE_ANIM) && !hero_u8(H_OVERRIDE_GUN);
    weapon_used = 0;
    charge_host();
}

static int is_glove(int item)
{
    return item == ITEM_BOMB_GLOVE || item == ITEM_MINE_GLOVE || item == ITEM_GLOVE_OF_DOOM ||
           item == ITEM_DRONE || item == ITEM_DECOY_GLOVE;
}

/* Whether RC2's glove case would make the standing throw on the next frame (GLOVE_DRAWN above).
 * RC2 takes the ammo when a throw starts, RC1 when its projectile comes out, so "ammo left" is a
 * second one here. */
static int glove_throw_follows(void)
{
    int item = hero_i(H_HAND_ITEM);
    if (hero_i(H_HAND_FRAMES) + 1 <= GLOVE_DRAWN)
        return 0;                   /* e.g. just switched back to it from the wrench */
    int recent = circle_since && circle_since - 1 < GLOVE_BUFFER - 1;    /* still in the buffer next frame */
    if (!(pad_word(PAD_HELD) & BUTTON_CIRCLE) && !recent)
        return 0;
    return item_max_ammo(item) == 0 || item_ammo(item) > 1;
}

/* At the end of a charge: RC2's skid animation takes the weapon's animation off his upper body
 * when it is not a gun's [0x81b470]; with a glove, the chooser then makes the standing throw from
 * the skid. A glove that will not be followed by that keeps its throw: RC1's gloves release their
 * projectile partway through the throw's animation (RC2's on their own timer), so it has to play
 * out for the projectile to come. */
static void end_weapon_use(void)
{
    if (!hero_u8(H_OVERRIDE_ANIM) || hero_u8(H_OVERRIDE_GUN))
        return;
    if (is_glove(hero_i(H_HAND_ITEM)) && !glove_throw_follows())
        return;
    if (hero_i(H_UPPER_ANIM))
        hero_i(H_UPPER_ANIM_END) = 1;
    hero_u8(H_OVERRIDE_ANIM) = 0;
}

/* RC2's charge handler, run in place of hero_update while charging. */
static void charge_move(void)
{
    void *moby = hero_moby();
    if (moby_u8(moby, MOBY_ANIM_ID) != ANIM_SLOT)
        hero_set_anim(ANIM_SLOT, 0, ANIM_BLEND);
    hero_i(H_ANIM_PENDING) = 0;     /* the walk handler would clear it; it does not run here */

    charge_sound();

    stick_target(0, 0, 1.0f);       /* stick -> target speed and heading, as the walk state does */

    int burst = g_charge_frames <= BURST_FRAMES;
    float target = (burst ? BURST_SPEED : CRUISE_SPEED) / 60;
    float turn_scale = burst ? BURST_TURN : 1.0f;
    hero_f(H_TARGET_SPEED) = target;

    /* Steering: smoothed -stick x, turning him relative to his own heading. */
    approach(&steer, -hero_f(H_STICK_X), STEER_SMOOTH);
    steer = clampf(steer, -1, 1);
    float stick = hero_f(H_STICK);
    float yaw = TURN_RATE / 60 * stick * steer * turn_scale;
    if (absf(stick) > STEER_DEADZONE)
        rotate_hero(0, 0, yaw);

    approach(&hero_f(H_SPEED), target, (target > hero_f(H_SPEED) ? ACCEL : DECEL) / 3600);
    hero_f(H_PITCH) = 0;            /* level, as RC2's probe leaves it for the charge [0x80ba44] */
    build_velocity(99999.0f);       /* along his own heading */

    /* Hover: no gravity; the height springs towards the ground + HOVER_HEIGHT, at most at the
     * hover's top speed. With no ground found below, the probe leaves the ground at 0, so he
     * sinks towards height HOVER_HEIGHT of the world, as in RC2. */
    float z = hero_f(H_Z);
    float want = hero_f(H_GROUND_Z) + HOVER_HEIGHT;
    float gap = absf(want - z);
    hover_velocity = clampf(hover_velocity, -gap, gap);    /* RC2's spring does this first [0xb68490] */
    spring_step(&hover_velocity, want - z, HOVER_STIFFNESS, HOVER_DAMPING, HOVER_MAX_SPEED / 60);
    z += hover_velocity;
    hero_f(H_Z) = z;
    if (absf(want - z) < HOVER_MAX_SPEED / 60 * HOVER_SNAP) {
        hero_f(H_Z) = want;
        hover_velocity = 0;
    }

    vec4 ahead;
    vec_scale(&ahead, hero_vec(H_MOTION), SWEEP_AHEAD);
    vec_add(&ahead, &ahead, hero_vec(H_CENTRE));
    crate_sweep(&ahead, SWEEP_RADIUS);

    charge_trails();
    lean(yaw);
    charge_camera();
}

/* RC2's transitions out of the charge state [0x84dba8]. Returns 1 when the charge goes on. */
static int charge_transitions(int r1_held)
{
    if (hero_u16(H_MAGNETIC) || weapon_used || (!r1_held && g_charge_frames > MIN_FRAMES)) {
        charge_off();
        end_sound_due = 1;          /* RC2 plays the end sound on this exit only */
        g_charge_slide = 1;
        hero_i(H_SKID_LOCK) = SKID_LOCK;
        hero_set_state(STATE_SKID, 0);
        hero_set_anim(SKID_ANIM, 4, SKID_ANIM_BLEND);
        end_weapon_use();
        return 0;
    }
    if (hero_u8(H_WALL)) {
        float moved = vec_length(hero_vec(H_MOTION));
        if (moved < vec_length(hero_vec(H_VELOCITY)) * BUMP_RATIO || wall_probe(0, 0, 0, PROBE_HEIGHT, PROBE_REACH)) {
            charge_off();
            hero_set_state(STATE_BUMP, 1);
            return 0;
        }
    }
    return 1;
}

/* The transition function, while charging, in front of its per-state switch: RC2's charge
 * transitions instead of the walk state's. A charge that ends here ends into the skid, whose own
 * transitions run from the next frame. Returns 1 to skip the state's own transitions. */
int charge_trans(void)
{
    int state = hero_i(H_STATE);
    if (g_charge_pending) {
        /* RC2's trigger is the weapon chooser's tail: a weapon that changed his state this frame
         * (a glove's standing throw) comes first [0x835c14]. */
        g_charge_pending = 0;
        if (g_charge_frames > 0 || !charge_may_start(state))
            return 0;
        charge_start();
        return 1;
    }
    if (state != STATE_WALK)
        return 0;                   /* something else changed his state this frame */
    if (charge_transitions(r1_last))
        g_charge_frames++;          /* RC2's state timer, behind its physics [0x812e28] */
    return 1;
}

/* Takes R1 out of the pad words, so the game does not crouch while it starts or holds a charge. */
static void hide_charge_button(void)
{
    pad_word(PAD_HELD) &= ~CHARGE_BUTTON;
    pad_word(PAD_PRESSED) &= ~CHARGE_BUTTON;
}

void charge_frame(void)
{
    keep_anim_slot(ANIM_SLOT, ANIM_ADDR);

    int held = (pad_word(PAD_HELD) & CHARGE_BUTTON) != 0;
    int pressed = held && !r1_last;
    r1_last = held;
    int state = hero_i(H_STATE);
    if (state == STATE_LOOK || state == STATE_WEAPON_LOOK)
        from_look = 1;
    else if (state != STATE_CROUCH)
        from_look = 0;

    int circle = (pad_word(PAD_HELD) & BUTTON_CIRCLE) != 0;
    if (circle && !circle_last)
        circle_since = 1;
    else if (circle_since && circle_since < GLOVE_BUFFER + 1)
        circle_since++;
    else
        circle_since = 0;
    circle_last = circle;

    /* RC2's double tap [0x835c14]: every press of R1 opens a window of TAP_WINDOW frames, and a
     * press inside it is the second tap. The press that starts a charge and presses during one
     * count too, so after a charge cut short (a weapon, the Magneboots) one press can be enough.
     * RC2 counts in its weapon chooser, which does nothing while the item in his hand is being
     * switched: then the window stands still and presses are lost. */
    int second_tap = 0;
    if (hero_i(H_HAND_READY) == 2 && hero_i(H_HAND_MOBY) != 0) {
        if (tap_window > 0)
            tap_window--;
        second_tap = pressed && tap_window > 0;
        if (pressed)
            tap_window = TAP_WINDOW;
    }

    if (g_charge_frames > 0) {
        if (state != STATE_WALK) {
            charge_off();           /* anything else (damage, a surface, a cutscene) took over */
            return;
        }
        /* A weapon used since the last frame's transitions ends the charge at this frame's, as
         * RC2's weapons, which fire behind its transitions, do. */
        if (old_throw && !hero_u8(H_OVERRIDE_ANIM))
            old_throw = 0;
        weapon_used = hero_u8(H_OVERRIDE_ANIM) && !old_throw;
        hide_charge_button();
        return;
    }

    if (state != STATE_SKID && state != STATE_CROUCH && state != STATE_GLOVE_THROW && state != STATE_IDLE)
        g_charge_slide = 0;

    if (!second_tap)
        return;
    if (charge_may_start(state)) {
        g_charge_pending = 1;       /* the charge starts behind this frame's physics, as RC2's */
        hide_charge_button();
    }
}

/* In place of hero_update: 1 when the charge moved him this frame. */
int charge_update(void)
{
    if (g_charge_frames > 0 && hero_i(H_STATE) == STATE_WALK) {
        charge_move();
        return 1;
    }
    charge_sound_stop();
    return 0;
}

/* While charging, the chooser runs for the weapons that fire without a change of state, and for
 * the PDA (it opens the PDA screen, nothing else); the ones it would change his state for (the
 * wrench, the Swingshot, the Walloper and the Hologuise) are skipped, as RC2's chooser skips them
 * during a charge. */
int charge_allows_chooser(void)
{
    g_moving_throw = g_charge_frames > 0;
    if (g_charge_frames <= 0)
        return 1;
    switch (hero_i(H_HAND_ITEM)) {
    case ITEM_WRENCH: case ITEM_SWINGSHOT: case ITEM_WALLOPER: case ITEM_HOLOGUISE:
        return 0;
    }
    return 1;
}

int charge_after_chooser(int result)
{
    return result;
}

/* RC2's surface function makes two exceptions for a charging Ratchet, which the surface code
 * (0x83ac8) gets here for the one call:
 *   standing  hovering close to the ground counts as standing on it [RC2 0x811548]; RC1 counts
 *             frames on the ground at H_GROUND_FRAMES, which stays 0 while he hovers
 *   sinking   mud (and lava and Hoven's deadly layer) only take him when he is moving down, by his
 *             last frame's movement; the hover moves his height outside of that, so it reads 0.
 *             RC2 waives the test for the charge [0x811920, 0x811880, 0x8119c8]: charging onto
 *             mud sinks him at once (state 0x68), ending the charge.
 * Returns non-zero when charge_surface_after has to undo them. */
static int surface_undo;            /* 1: H_GROUND_FRAMES set, 2: the movement's z set */
static float saved_move_z;

int charge_surface_before(void)
{
    surface_undo = 0;
    if (g_charge_frames <= 0 || hero_i(H_STATE) != STATE_WALK)
        return 0;
    if (hero_i(H_GROUND_FRAMES) == 0 && absf(hero_f(H_GROUND_DIST)) < GROUNDED_HEIGHT) {
        hero_i(H_GROUND_FRAMES) = 1;
        surface_undo |= 1;
    }
    saved_move_z = hero_f(H_MOTION + 8);
    if (!(saved_move_z < 0)) {
        hero_f(H_MOTION + 8) = -1e-4f;
        surface_undo |= 2;
    }
    return surface_undo;
}

void charge_surface_after(void)
{
    if (surface_undo & 1)
        hero_i(H_GROUND_FRAMES) = 0;
    if (surface_undo & 2)
        hero_f(H_MOTION + 8) = saved_move_z;
}

