# Hook words for RC2's walking (src/c/walk.c), and data words for RC2's numbers.
#
# PATCH addr, name, instruction: the word written over the game's instruction at addr.
# The walk handler's three stubs call C where the game holds nothing in the volatile registers
# (the code behind each site loads them all again), and go back with a b, having lost their
# return address; each sets the one register the game code it goes on to expects.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

# ---- Walk handler 0xb1630 (states 2 and 0x73; r25 = hero, r26 = hero table 0xaaafc) -------------

# Replaces lis r27, 0x72 at the head of RC1's flags (+0x3b8, +0x3be) and its stick -> target speed
# call (0x9b8c8): walk_flags() does RC2's, then RC1's push block (0xb1770) runs, which wants
# r27 = 0x720000. [RC2 0x83ef38..0x83f148]
PATCH 0x0B16C4, walk_flags, bl walk_flags_stub
	.section .text.walk_flags_stub, "ax"
walk_flags_stub:
	bl	walk_flags_entry
	lis	r27,0x72
	b	game_walk_push

# Replaces lfs f29, 0x14(r26) at the head of RC1's turn: walk_turn() (RC2's turn, or the strafe's),
# then on to the lean calls. [RC2 0x83f1f0..0x83f330]
PATCH 0x0B1804, walk_turn, bl walk_turn_stub
	.section .text.walk_turn_stub, "ax"
walk_turn_stub:
	bl	walk_turn_entry
	b	game_walk_turn_done

# Replaces lis r28, 0x72 behind the lean calls, in front of RC1's acceleration: walk_speed(), then
# on to the velocity (0xb1a48, walk_vel_stub in strafe_hooks.s), which wants r28 = 0x720000.
# [RC2 0x83f33c..0x83f4bc]
PATCH 0x0B1998, walk_speed, bl walk_speed_stub
	.section .text.walk_speed_stub, "ax"
walk_speed_stub:
	bl	walk_speed_entry
	lis	r28,0x72
	b	game_walk_velocity

# ---- Walk transitions 0xc03e4 (states 2, 0x2f, 0x73, 0x7e; r19 = hero) ----------------------------

# Replaces lwz r3, 0x2088(r19) at the head of RC1's choice between the run jump and the jump, behind
# the jump chooser (strafe flips): walk_run_jump() decides with RC2's rule. Both ways on load their
# own registers. [RC2 0x84d34c]
PATCH 0x0C0534, walk_jump, bl walk_jump_stub
	.section .text.walk_jump_stub, "ax"
walk_jump_stub:
	bl	walk_run_jump_entry
	cmpwi	r3,0
	beq	1f
	b	game_walk_run_jump
1:	b	game_walk_jump

# Replaces sth r20, 0x3be(r19): RC1's quick turn from the pad's flick event (pad +0xa4 bit 0x10000,
# set by 0x4ebeb4 and 0x4ebf6c). RC2 starts its quick turns in the walk handler (walk_flags).
PATCH 0x0C0964, walk_no_flick, nop

# ---- Data -------------------------------------------------------------------------------------------

# RC2's run speed in RC1's gait table (0x723308 +0x18): walk.c's RUN_SPEED for the walk, and the
# same reference RC2's tables give its air function (0x9ed04, RC2 0x830184) and the jump state's
# run jump test (0xc4ac4, RC2 0x8519e8). Was 5.7.
PATCH 0x723320, run_speed, .float 5.95

# Ground acceleration in the hero table, units/s^2 (+0x2a8, +0x298), as RC2's (+0x1fc, +0x200) for
# the five other handlers that read it: states 0x3f (Magneboots), 0x44/0x50, 0x54, 0x5b, 0x65. The
# walk itself takes walk.c's numbers. Were 7.5 and 8.5.
PATCH 0x0AADA4, ground_accel, .float 18.0
PATCH 0x0AAD94, ground_decel, .float 20.0

# State 0x20's speed threshold (hero_update 0xb0f3c) reads +0x298 too: replaces lfs f1, 0x298(r26)
# to keep RC1's 8.5 there, as RC2's table keeps it for the same read (+0x204). f1 is the only
# register it sets; r11 is not used near it.
PATCH 0x0B0F3C, speed_threshold, bl speed_threshold_stub
	.section .text.speed_threshold_stub, "ax"
speed_threshold_stub:
	lis	r11,g_walk_threshold@ha
	lfs	f1,g_walk_threshold@l(r11)
	blr
