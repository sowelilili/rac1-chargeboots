/* The calls the game makes into the mod (see src/asm/glue.s), handed to its parts. */
#include "mod.h"

/* Every frame, in front of hero_main. */
void mod_frame(void)
{
    strafe_frame();
    charge_frame();
}

/* In place of hero_update: non-zero when the mod moved the hero itself this frame. */
int mod_update(void)
{
    return charge_update();
}

/* Around the weapon/gadget chooser: 0 skips it. */
int mod_chooser_before(void)
{
    return charge_allows_chooser();
}

int mod_chooser_after(int result)
{
    return charge_after_chooser(result);
}

/* Around the surface reactions. */
int mod_surface_before(void)
{
    return charge_surface_before();
}

void mod_surface_after(void)
{
    charge_surface_after();
}
