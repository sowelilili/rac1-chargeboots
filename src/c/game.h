/* Ratchet & Clank HD (NPEA00385): the game memory and functions the mod uses. */
#ifndef GAME_H
#define GAME_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef short s16;

/* The game's vectors: 16-byte aligned, w unused. Its vector functions use lvx/stvx on them. */
typedef struct {
    float x, y, z, w;
} __attribute__((aligned(16))) vec4;

#define MEM(type, addr) (*(volatile type *)(addr))

/* ---- Pad ---------------------------------------------------------------------------------- */
#define PAD 0x964A40
#define PAD_HELD    0xA0                /* buttons held this frame */
#define PAD_PRESSED 0xA4                /* buttons that went down this frame */
#define BUTTON_L2       0x0001
#define BUTTON_R2       0x0002
#define BUTTON_L1       0x0004
#define BUTTON_R1       0x0008
#define BUTTON_TRIANGLE 0x0010
#define BUTTON_CIRCLE   0x0020
#define BUTTON_CROSS    0x0040
#define BUTTON_SQUARE   0x0080
#define BUTTON_SELECT   0x0100
#define BUTTON_START    0x0800
#define BUTTON_UP       0x1000
#define BUTTON_RIGHT    0x2000
#define BUTTON_DOWN     0x4000
#define BUTTON_LEFT     0x8000
#define pad_word(off) MEM(u32, PAD + (off))

/* ---- Hero ----------------------------------------------------------------------------------- */
#define HERO 0x969CE0
#define hero_f(off)   MEM(float, HERO + (off))
#define hero_i(off)   MEM(int, HERO + (off))
#define hero_u16(off) MEM(u16, HERO + (off))
#define hero_u8(off)  MEM(u8, HERO + (off))
#define hero_vec(off) ((vec4 *)(HERO + (off)))

#define H_POS            0x80    /* vec4 */
#define H_Z              0x88
#define H_YAW            0x98    /* facing, rad */
#define H_CENTRE         0xD0    /* vec4: the crate sweep is centred ahead of this */
#define H_VELOCITY       0xE0    /* vec4, units per frame */
#define H_MOTION         0x100   /* vec4: the movement made last frame */
#define H_SLIDE          0x150   /* vec4: what the idle and skid states move him by each frame */
#define H_MOVE_SPEED     0x164   /* the speed he went last frame */
#define H_MOVE_FORWARD   0x168   /* the part of it along his facing */
#define H_HEADING        0x180   /* the heading the stick asks for; the walk turns him to it */
#define H_TURN_SPEED     0x184   /* turn velocity, used by approach_angle */
#define H_TURN           0x188   /* angle left to turn */
#define H_TARGET_SPEED   0x190   /* units per frame */
#define H_SPEED          0x194   /* ground speed, units per frame */
#define H_STATE_TIMER    0x198   /* frames in the state: 0 on the first */
#define H_SKID_LOCK      0x1C4   /* frames: blocks jump and crouch in the skid state */
#define H_WALL           0x257   /* u8: touching a wall */
#define H_GROUND_Z       0x2D8   /* height of the ground under him */
#define H_GROUND_DIST    0x2DC   /* 32.0 when the ground probe found nothing (H_GROUND_Z is 0 then) */
#define H_PITCH          0x2E4   /* the ground's slope he moves along (build_velocity) */
#define H_GROUND_FRAMES  0x300   /* frames on the ground */
#define H_MAGNETIC       0x308   /* u16: the Magneboots hold him to a surface */
#define H_QUICK_TURN     0x3BE   /* u16: a quick turn is on (walk state) */
#define H_FLIP_SPEED     0x458   /* ground speed of a flip (state 0xb) */
#define H_ANIM_RATE      0xA90   /* animation speed the walk animation chooser sets */
#define H_ANIM_PENDING   0xA9C   /* set by hero_set_anim until the walk handler clears it */
#define H_UPPER_ANIM     0xD08   /* an animation is playing on his upper body (a weapon's) */
#define H_UPPER_ANIM_END 0xD14   /* 1 ends it */
#define H_HAND_MOBY      0x1090  /* the moby of the item in his hand */
#define H_HAND_FRAMES    0x10B0  /* frames the item has been in his hand: 3 when a switch starts, +1 a frame once it is in */
#define H_HAND_READY     0x10B4  /* 2 when the item in his hand is ready */
#define H_HAND_ITEM      0x10B8  /* the weapon or gadget in his hand (Circle uses it) */
#define H_STICK_X        0x1D20  /* stick left/right */
#define H_STICK_Y        0x1D24  /* stick up/down, positive down */
#define H_MOBY           0x2080  /* Ratchet's moby */
#define H_STATE          0x2084
#define H_STATE_TYPE     0x208C  /* 0 idle, 1 walk and skid, 2 falls, 0xc crouch; numbered as RC2's */
#define H_GAIT           0x2088  /* walk gait: its animation is gait + 3 */
#define H_CHARACTER      0x20A4  /* u8: 0 Ratchet, 1 Clank, 2 Giant Clank */
#define H_OVERRIDE_ANIM  0x20A8  /* u8: a weapon is being used (its animation, set by 0x99ea8) [RC2 +0x22b8] */
#define H_OVERRIDE_GUN   0x20AA  /* u8: and it is a gun's, held while firing; 0 for a glove's throw [RC2 +0x22ba].
                                  * While set, the walk starts no quick turn and stops without a skid. */
#define H_MAGNET_WALK    0x20B3  /* u8: walking on a magnetic surface */
/* Surface flags, set again every frame by the surface code 0x83ac8 from the face under him */
#define H_SLIPPERY       0x12E2  /* u8: ice (face type 7) */
#define H_SLIDE_SURFACE  0x12EA  /* u8: a surface that slides him (types 8, 0xc) */
#define H_WADING         0x20A9  /* u8: wading in water */
#define H_STICK          0x229C  /* stick deflection 0..1 */

/* Hero states */
#define STATE_IDLE   0x00
#define STATE_LOOK   0x01        /* first-person view (L1) */
#define STATE_WEAPON_LOOK 0x1E   /* the same with a weapon in hand: its moby code turns state 1 into it
                                    (the gloves, the guns, the Devastator, the RYNO, ...) */
#define STATE_WALK   0x02
#define STATE_SKID   0x03
#define STATE_CROUCH 0x04
#define STATE_LONG_JUMP 0x0A     /* the long jump (crouch and jump on the run) with the Heli-Pack on */
#define STATE_FLIP   0x0B        /* side and back flips; never forward: the long jumps are states of their own */
#define STATE_THRUSTER_JUMP 0x10 /* the long jump with the Thruster-Pack on (RC1's "charge jump") */
#define STATE_GLOVE_THROW 0x23   /* a glove's standing throw */
#define STATE_BUMP   0x7A

/* Hand items the chooser (0xa30e0) handles with a change of state */
/* Ammo: the item table (0x18 bytes an item) has the most ammo at +0xe, 0 for no ammo; the ammo
 * itself is an int an item. RC1 takes a glove's ammo when the projectile comes out. */
#define ITEM_TABLE      0x737D30
#define item_max_ammo(item) MEM(u16, ITEM_TABLE + (item) * 0x18 + 0xE)
#define AMMO            0x96C0AC
#define item_ammo(item) MEM(int, AMMO + (item) * 4)

#define ITEM_WRENCH     0x08
#define ITEM_SWINGSHOT  0x0C
#define ITEM_WALLOPER   0x12
#define ITEM_HOLOGUISE  0x1F
/* The gloves, which the chooser throws (its glove case 0xa3c14) */
#define ITEM_BOMB_GLOVE 0x0A
#define ITEM_MINE_GLOVE 0x11
#define ITEM_GLOVE_OF_DOOM 0x14
#define ITEM_DRONE      0x18
#define ITEM_DECOY_GLOVE 0x19

/* ---- Mobys ------------------------------------------------------------------------------------ */
#define MOBY_MODEL      0x24    /* model (class) pointer */
#define MOBY_ANIM_ID    0x53    /* u8: current animation */
#define MODEL_ANIMS     0x48    /* model: table of animation pointers */
#define MOBY_FORWARD    0xC0    /* vec4: row 0 of its matrix, the way it faces */
#define moby_ptr(moby, off) MEM(u32, (u32)(moby) + (off))
#define moby_u8(moby, off)  MEM(u8, (u32)(moby) + (off))
#define hero_moby() ((void *)hero_i(H_MOBY))

/* ---- Sound ------------------------------------------------------------------------------------ */
/* Voices: 0x70 bytes each, from VOICES + 0x70; +4 state (0 = free), +8 the definition playing,
 * +0x18 the moby the voice follows. Addressed here with slot + 1, as the game's own code does. */
#define VOICES 0x963760
#define voice_u8(index, off) MEM(u8, VOICES + (index) * 0x70 + (off))
#define voice_u32(index, off) MEM(u32, VOICES + (index) * 0x70 + (off))
#define SOUND_DEF_BANK 0x1C     /* sound definition: bank handle */

/* ---- Items owned (saved): a byte an item, set by GiveItem 0x112e18 ----------------------------- */
#define ITEMS_OWNED     0x96C140
#define item_owned(item) MEM(u8, ITEMS_OWNED + (item))
/* The Charge Boots are the Grindboots' item (src/asm/item_hooks.s) */
#define ITEM_CHARGE_BOOTS 0x1D

/* ---- Camera ---------------------------------------------------------------------------------- */
#define CAMERA 0x9513C0         /* camera state */
#define camera_yaw() MEM(float, CAMERA + 0x158)
#define CAMERA_ACTIVE   0x180   /* camera state: the active camera [RC2 +0x190] */
#define CAM_VARS        0x70    /* camera: its variables */
#define CAM_TYPE        0x86    /* u16 camera: 0 = the third-person camera */
/* The third-person camera's variables [RC2's, at the offsets in brackets]; the camera update
 * resets them all at its end (0x2eda8) */
#define CV_FOLLOW_K     0x11C   /* spring of its look-at point towards the hero          [+0x13c] */
#define CV_FOLLOW_D     0x120   /*                                                        [+0x140] */
#define CV_DISTANCE_ON  0x16C   /* u16: spring the distance towards CV_DISTANCE           [+0x19c] */
#define CV_DISTANCE     0x170   /*                                                        [+0x1a0] */
#define CV_DISTANCE_K   0x178   /*                                                        [+0x1a8] */
#define CV_TURN_SPEED   0x1BC   /* yaw per frame at full input (the option's speed)       [+0x1d4] */
#define CV_TURN_INPUT   0x1C4   /* the auto-turn's input, -1..1                           [+0x1dc] */
#define CV_NO_PULL_IN   0x226   /* u16: no pulling in while the camera is ahead of him    [+0x246] */

/* ---- Body joint controllers: 31 of 0xB0 bytes, updated by 0x7f608 every frame; each angle springs
 * towards its target (approach_angle), and the game clears the targets behind the update. The
 * charge leans Ratchet's body with controllers 0, 2 and 3, the strafe twists it with 0 and 1, as
 * RC2's do (its block 0x13188a0). ---------------------------------------------------------------- */
#define JOINTS      0x721BD0
#define JOINT_SIZE  0xB0
#define J_TARGET_X  0x60
#define J_TARGET_Z  0x68
#define J_STIFFNESS 0xA4
#define J_DAMPING   0xA8
#define joint_f(i, off) MEM(float, JOINTS + (i) * JOINT_SIZE + (off))

/* ---- Collision -------------------------------------------------------------------------------- */
#define COLLECT_RESULTS 0xA14A00 /* moby pointers written by collect() */

/* ---- Functions (thunks in src/asm/glue.s; the r3..r10 and f1.. registers as the game uses them) ---- */
void hero_set_state(int state, int flag);
void hero_set_anim(int anim, int flags, float blend);
void stick_target(int r3, int mode, float scale);
void rotate_hero(float x, float y, float z);
void approach(volatile float *value, float target, float step);
void build_velocity(float heading_clamp);
void spring_step(volatile float *velocity, float distance, float stiffness, float damping, float max);
void vec_scale(vec4 *out, const vec4 *in, float scale);
void vec_add(vec4 *out, const vec4 *a, const vec4 *b);
float vec_length(const vec4 *v);
int wall_probe(int r3, int r4, int r5, float height, float reach);
int collect(const vec4 *centre, int r4, int flags, void *ignore, void *damage, float radius);
int is_crate(void *moby);
void deal_damage(void *target, void *source, int flags, int r6, const vec4 *pos, const vec4 *dir, float damage);
void joint_pos(void *moby, int joint, vec4 *out);
void trail(void *moby, const vec4 *pos, const vec4 *drift, const void *descriptor, int one);
int bank_load(const void *image);
int play_sound_def(void *definition, int flags, void *moby, int r6, int r7);
void stop_voice(int slot);
float angle_diff(float a, float b);     /* a - b, wrapped to -pi..pi */
/* The third-person camera's auto-turn towards behind dir (sets its turn input); returns the angle
 * left, 0..pi. max_angle 0: always. */
float cam_auto_turn(void *camera, const vec4 *dir, float max_angle);
float angle_gap(float a, float b);      /* |a - b|, wrapped to 0..pi */
/* Turns *angle towards target on a spring: velocity += diff * stiffness - velocity * damping,
 * limited to max_speed per frame. r9 = 0: no forced direction. Returns the angle left to turn. */
float approach_angle(volatile float *angle, int r4, volatile float *velocity, int r6, int r7, int r8, int r9,
                     float target, float stiffness, float damping, float max_speed);
/* The same turn for the hero walking on a Magneboots surface; it sets H_TURN itself. r6 = 0. */
void magnet_turn(int r3, int r4, int r5, int r6, float stiffness, float damping, float max_speed);

#endif
