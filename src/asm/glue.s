# 64-bit glue between the game (PPC64, 32-bit pointers) and the 32-bit C code.
#
# The C code is built with -m32 and kept off r14-r31, so it never saves the game's non-volatile
# registers with 32-bit stores; it does use the volatile ones freely. Everything the game calls
# goes through an entry here that gives the C function a 64-bit frame of its own, and every game
# function the C code calls goes through a thunk that gives the game a full 64-bit frame
# (LR save at +16, parameter save area at +48).

# ---- Helpers ---------------------------------------------------------------------------------

# A game function for C: THUNK c_name, game_symbol
.macro THUNK name, target
	.section .text.\name, "ax"
	.globl \name
\name:
	stdu	r1,-128(r1)
	mflr	r0
	std	r0,112(r1)
	bl	\target
	ld	r0,112(r1)
	mtlr	r0
	addi	r1,r1,128
	blr
.endm

# A C function for the game, as a plain call: ENTRY entry_name, c_function
.macro ENTRY name, cfunc
	.section .text.\name, "ax"
	.globl \name
\name:
	stdu	r1,-160(r1)
	mflr	r0
	std	r0,144(r1)
	bl	\cfunc
	ld	r0,144(r1)
	mtlr	r0
	addi	r1,r1,160
	blr
.endm

# Entries that stand in for a call the game makes (bl target): they keep the call's argument
# registers r3-r10 and f1-f8 for the game function. Frame: 0x30 r3-r10, 0x70 f1-f8, 0xb0 LR,
# 0xb8 and 0xc0 two words for the C results.
.set FRAME, 0xd0
.macro OPEN
	stdu	r1,-FRAME(r1)
	mflr	r0
	std	r0,0xb0(r1)
	std	r3,0x30(r1)
	std	r4,0x38(r1)
	std	r5,0x40(r1)
	std	r6,0x48(r1)
	std	r7,0x50(r1)
	std	r8,0x58(r1)
	std	r9,0x60(r1)
	std	r10,0x68(r1)
	stfd	f1,0x70(r1)
	stfd	f2,0x78(r1)
	stfd	f3,0x80(r1)
	stfd	f4,0x88(r1)
	stfd	f5,0x90(r1)
	stfd	f6,0x98(r1)
	stfd	f7,0xa0(r1)
	stfd	f8,0xa8(r1)
.endm
.macro ARGS
	ld	r3,0x30(r1)
	ld	r4,0x38(r1)
	ld	r5,0x40(r1)
	ld	r6,0x48(r1)
	ld	r7,0x50(r1)
	ld	r8,0x58(r1)
	ld	r9,0x60(r1)
	ld	r10,0x68(r1)
	lfd	f1,0x70(r1)
	lfd	f2,0x78(r1)
	lfd	f3,0x80(r1)
	lfd	f4,0x88(r1)
	lfd	f5,0x90(r1)
	lfd	f6,0x98(r1)
	lfd	f7,0xa0(r1)
	lfd	f8,0xa8(r1)
.endm
.macro CLOSE
	ld	r0,0xb0(r1)
	mtlr	r0
	addi	r1,r1,FRAME
.endm

# ---- Entries ---------------------------------------------------------------------------------

# Replaces bl hero_main (twice): mod_frame() runs first, every frame.
	.section .text.frame_entry, "ax"
	.globl frame_entry
frame_entry:
	OPEN
	bl	mod_frame
	ARGS
	CLOSE
	b	game_hero_main

# Replaces bl hero_update: mod_update() returns non-zero when it updated the hero itself.
	.section .text.update_entry, "ax"
	.globl update_entry
update_entry:
	OPEN
	bl	mod_update
	stw	r3,0xb8(r1)
	ARGS
	lwz	r11,0xb8(r1)
	CLOSE
	cmpwi	r11,0
	bne	1f
	b	game_hero_update
1:	blr

# Replaces bl chooser (weapon/gadget use): mod_chooser_before() returns 0 to skip the chooser
# (it then returns 0), otherwise the chooser runs and mod_chooser_after(result) returns what the
# game sees.
	.section .text.chooser_entry, "ax"
	.globl chooser_entry
chooser_entry:
	OPEN
	bl	mod_chooser_before
	cmpwi	r3,0
	beq	1f
	ARGS
	bl	game_chooser
	bl	mod_chooser_after
	CLOSE
	blr
1:	CLOSE
	li	r3,0
	blr

# Replaces bl surface (reactions to the ground): mod_surface_before() returns non-zero when
# mod_surface_after() has to run once the game function is done.
	.section .text.surface_entry, "ax"
	.globl surface_entry
surface_entry:
	OPEN
	bl	mod_surface_before
	stw	r3,0xb8(r1)
	ARGS
	bl	game_surface
	std	r3,0xc0(r1)
	lwz	r11,0xb8(r1)
	cmpwi	r11,0
	beq	1f
	bl	mod_surface_after
1:	ld	r3,0xc0(r1)
	CLOSE
	blr

# ---- Thunks ---------------------------------------------------------------------------------

THUNK hero_set_state, game_hero_set_state
THUNK hero_set_anim, game_hero_set_anim
THUNK stick_target, game_stick_target
THUNK rotate_hero, game_rotate_hero
THUNK approach, game_approach
THUNK build_velocity, game_build_velocity
THUNK spring_step, game_spring_step
THUNK vec_scale, game_vec_scale
THUNK vec_add, game_vec_add
THUNK vec_length, game_vec_length
THUNK wall_probe, game_wall_probe
THUNK collect, game_collect
THUNK is_crate, game_is_crate
THUNK deal_damage, game_deal_damage
THUNK joint_pos, game_joint_pos
THUNK trail, game_trail
THUNK bank_load, game_bank_load
THUNK play_sound_def, game_play_sound_def
THUNK stop_voice, game_stop_voice
THUNK angle_diff, game_angle_diff
THUNK angle_gap, game_angle_gap
THUNK approach_angle, game_approach_angle
THUNK magnet_turn, game_magnet_turn
THUNK cam_auto_turn, game_cam_auto_turn

ENTRY stop_cap_entry, stop_cap
ENTRY charge_trans_entry, charge_trans

# Walking (src/asm/walk_hooks.s)
ENTRY walk_flags_entry, walk_flags
ENTRY walk_turn_entry, walk_turn
ENTRY walk_speed_entry, walk_speed
ENTRY walk_run_jump_entry, walk_run_jump

# Strafing (src/asm/strafe_hooks.s; the optional hooks have their entries in their own files)
ENTRY strafe_idle_align_entry, strafe_idle_align
ENTRY strafe_walk_align_entry, strafe_walk_align
ENTRY strafe_jump_flips_entry, strafe_jump_flips
ENTRY strafe_lean_entry, strafe_lean
ENTRY strafe_flip_air_entry, strafe_flip_air
ENTRY strafe_air_turn_entry, strafe_air_turn
