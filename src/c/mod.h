/* What the mod's parts offer each other and the hook entries in mod.c. */
#ifndef MOD_H
#define MOD_H

#include "game.h"

/* charge.c */
extern int g_charge_frames;     /* frames into a charge, 0 = none */
void charge_frame(void);
int charge_update(void);
int charge_trans(void);
int charge_allows_chooser(void);
int charge_after_chooser(int result);
int charge_surface_before(void);
void charge_surface_after(void);
void crate_sweep(const vec4 *centre, float radius);
void keep_anim_slot(int slot, u32 anim);    /* an animation of the mod in a slot of Ratchet's model */

/* move.c */
void stop_cap(void);

/* strafe.c */
extern int g_strafe;             /* strafing this frame */
extern float g_strafe_heading;   /* the heading he moves along while strafing */
void strafe_frame(void);
void strafe_turn(void);

#endif
