/* Game functions (NPEA00385). C calls them through the thunks in src/asm/glue.s. */
game_hero_main = 0x8B778;          /* per-frame hero entry */
game_hero_update = 0xAAEC0;        /* per-state hero update, called from hero_main */
game_hero_set_state = 0xB72B0;     /* (r3 state, r4 flag) */
game_hero_set_anim = 0x85300;      /* (r3 animation id, r4 flags, f1 blend) */
game_chooser = 0xA30E0;            /* weapon/gadget use, first thing the transition function does */
game_surface = 0x83AC8;            /* reactions to the surface under Ratchet */
game_trans_exit = 0xC81A8;         /* the transition function's exit */
game_trans_switch = 0xBE720;       /* the transition function's per-state switch, r3 = state */
game_stick_target = 0x9B380;       /* (r4 mode, f1 scale): target speed and heading from the stick */
game_rotate_hero = 0x9B9EC;        /* (f1, f2, f3): turn the hero by these euler angles */
game_approach = 0x109AF0;          /* (r3 float*, f1 target, f2 step) */
game_build_velocity = 0x9C1B0;     /* (f1 heading clamp): velocity from speed and heading */
game_spring_step = 0x109A68;       /* (r3 velocity*, f1 distance, f2 stiffness, f3 damping, f4 max) */
game_vec_scale = 0x5E822C;         /* (r3 out, r4 in, f1 scale) */
game_vec_add = 0x5E7F54;           /* (r3 out, r4 a, r5 b) */
game_vec_length = 0x5E8030;        /* (r3 vec) -> f1 */
game_wall_probe = 0x88208;         /* (r5 = 0, f1 height, f2 reach) -> 1 when a wall is ahead */
game_collect = 0x5E598;            /* (r3 centre, r5 flags, r6 ignore, r7 damage, f1 radius) -> count */
game_is_crate = 0x10AF88;          /* (r3 moby) -> non-zero for a crate */
game_deal_damage = 0x105C08;       /* (r3 target, r4 source, r5 flags, r7 pos, r8 dir, f1 damage) */
game_joint_pos = 0xF1114;          /* (r3 moby, r4 joint, r5 out) */
game_trail = 0x1349F0;             /* (r3 moby, r4 pos, r5 drift, r6 descriptor, r7 = 1) */
game_bank_load = 0x60AE8C;         /* (r3 bank image) -> handle, 0 on failure */
game_play_sound_def = 0x15CE18;    /* (r3 definition, r4 flags, r5 moby, r6, r7) -> voice slot or -1 */
game_stop_voice = 0x15CDB0;        /* (r3 voice slot) */
game_angle_diff = 0x5025E0;        /* (f1, f2) -> f1 - f2 wrapped to -pi..pi */
game_angle_gap = 0x5026A0;         /* (f1, f2) -> |f1 - f2| wrapped to 0..pi */
game_approach_angle = 0x10A050;    /* (r3 angle*, r5 velocity*, r9 = 0, f1 target, f2 stiffness, f3 damping, f4 max) -> f1 left */
game_cam_auto_turn = 0x2F0A0;      /* (r3 camera, r4 vec4 *dir, f1 max angle) -> f1 angle left: the camera turns to behind dir */
game_magnet_turn = 0x9BA88;        /* (r6 = 0, f1 stiffness, f2 damping, f3 max): the walk's turn on a Magneboots surface */

/* Strafing: where the stubs in src/asm/strafe_hooks.s and src/asm/optional/ go back into the game. */
game_walk_vel_done = 0xB1A7C;      /* walk handler, behind the velocity */
game_idle_align_back = 0xBF468;    /* idle transitions: the stick test, after the hook */
game_idle_to_walk = 0xBF47C;       /* idle transitions: on to the walk state */
game_walk_align_back = 0xC0978;    /* walk transitions: the stick test, after the hook */
game_walk_stays = 0xC0B50;         /* walk transitions: no change of state */
game_walk_to_idle = 0xC0AB8;       /* walk transitions: to idle, behind its wait */
game_flip_type_exit = 0xA1E3C;     /* flip type function 0xa1d70: return r3 */
game_flip_time_tail = 0xA1F68;     /* flip time function 0xa1e78: r31 = flip type */
game_jump_choice_back = 0xA49F4;   /* jump chooser 0xa4950, after the hook */
game_jump_choice_flip = 0xA49D8;   /* jump chooser: set_state(0xb, 1), return 1 */
game_lean_back = 0x9F320;          /* lean function 0x9f300, after the hook */
game_lean_exit = 0x9FBA8;          /* lean function's exit (restores r30 from 0x98(r1)) */
game_flip_air_done = 0x9E658;      /* air function 0x9de38, behind the flip's speed */
game_air_turn_back = 0x9EC60;      /* air function, after the hook (the default air turn) */
game_walk_anim_back = 0xA4AF0;     /* walk animation chooser 0xa4a60, after the hook */
game_walk_anim_rate = 0xA4D48;     /* walk animation chooser: the animation speed */
game_walk_anim_exit = 0xA4E08;     /* walk animation chooser's exit */

/* Charge: weapons during and after a charge. */
game_glove_moving_throw = 0xA3DD4; /* chooser's glove case: bl 0x99ea8 (moving throw), then its exit */

/* Stopping (src/asm/move_hooks.s). */
game_walk_idle_anim = 0xC0AE4;     /* walk -> idle, behind set_state(0, 0): the idle animation */
game_gun_stance_anim = 0xA9E7C;    /* gun stance, behind its set_state(0, 0) */

/* The Charge Boots as the Grindboots' item (src/asm/item_hooks.s). */
game_text_lookup_next = 0x7B744;   /* TextLookup 0x7b740, behind its first instruction */

/* Walking (src/asm/walk_hooks.s). */
game_walk_push = 0xB1770;          /* walk handler 0xb1630: its push block, behind RC1's flags and target speed */
game_walk_turn_done = 0xB1990;     /* walk handler: the lean calls, behind RC1's turn */
game_walk_velocity = 0xB1A48;      /* walk handler: the velocity, behind RC1's acceleration */
game_walk_run_jump = 0xC05A4;      /* walk transitions: set_state(9, 1), the run jump */
game_walk_jump = 0xC05B4;          /* walk transitions: set_state(7, 1), the jump */
