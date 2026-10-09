/* RC2's way of stopping, for RC1 HD.
 *
 * RC1's walk goes into its skid state when Ratchet lets go of the stick after moving faster than
 * 2.7 units/s, and plays a skid animation; slower, he stays in the walk until it has lasted 25
 * frames and then stands. RC2's walk never skids: letting go of the stick goes straight to the
 * idle state, where the slide he carries on with and his speed are capped at 1.5 units/s (RC2
 * 0x84d764..0x84d8ac). RC2's gun stance caps them the same way when a gun's animation turns a
 * walk or a skid into standing (0x8375d8), which is what stops him in place while he fires.
 *
 * move_hooks.s sends every stop of RC1's walk into its own walk -> idle code (no skid, no wait)
 * and calls stop_cap() there and in RC1's gun stance (0xa9d70). The idle and skid states move
 * him by the slide H_SLIDE, which set_state copies from his last movement and the state then
 * shortens by its deceleration every frame.
 */
#include "game.h"
#include "mod.h"

#define STOP_SPEED      1.5f        /* units/s                                [RC2 0x84acc8, +0x4c] */

void stop_cap(void)
{
    const float cap = STOP_SPEED / 60;
    vec4 *slide = hero_vec(H_SLIDE);
    float length = vec_length(slide);
    if (length > cap)
        vec_scale(slide, slide, cap / length);
    if (hero_f(H_SPEED) > cap)
        hero_f(H_SPEED) = cap;
}
