# Hook words for strafing (src/c/strafe.c), and the stubs that work on the game's registers.
#
# Each hook replaces one instruction of RC1's hero code with a bl to its stub; the stub executes
# that instruction itself ("the displaced instruction") where the game's own path goes on. When
# Ratchet is not strafing, every stub only runs the displaced instruction and a test of a global
# on r11/r12 (and cr0, which the game sets again right behind every site). The C parts are called
# through the entries in src/asm/glue.s, and only from points where the game holds nothing in the
# volatile registers: the code it goes on with loads all of them first. A stub that has called C
# has lost its return address, so it goes back with a b to the game address behind its site.
#
# The optional hook (the backwards walk) is in src/asm/optional/.

# PATCH addr, name, instruction: the word written over the game's instruction at addr.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

# Loads a C global (int or float) by symbol: LOADW reg, symbol / LOADF freg, symbol (uses r11).
.macro LOADW reg, sym
	lis	r11,\sym@ha
	lwz	\reg,\sym@l(r11)
.endm
.macro LOADF freg, sym
	lis	r11,\sym@ha
	lfs	\freg,\sym@l(r11)
.endm

.set WALK_STAY, 1               # strafe_walk_align's results, as in strafe.c
.set WALK_TO_IDLE, 2

# ---- Walk handler 0xb1630 (r25 = hero, r26 = hero table) ----------------------------------------
# The flags and the turn are walk.c's (src/asm/walk_hooks.s), which calls strafe_turn() while
# strafing and skips the flags then, as RC2 does. [RC2 0x83ef38, 0x83f1f0]

# Replaces lhz r3, 0x3b8(r25) in front of the velocity: strafing, it goes along the strafe heading.
# [RC2 0x83f4c0]
PATCH 0x0B1A48, walk_vel, bl walk_vel_stub
	.section .text.walk_vel_stub, "ax"
walk_vel_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	bne	1f
	lhz	r3,0x3b8(r25)
	blr
1:	LOADF	f1,g_strafe_heading
	bl	game_build_velocity
	b	game_walk_vel_done

# ---- Transition function 0xbe228 (r19 = hero, r25 = its table) ------------------------------------
# Both sites are followed by a stick test that loads f1 and f2 itself, and every way on from there
# loads its own registers.

# Idle: replaces lfs f1, 0x30(r25). Holding a strafe button while not facing the camera: on to the walk state.
# [RC2 0x84bd80]
PATCH 0x0BF464, idle_align, bl idle_align_stub
	.section .text.idle_align_stub, "ax"
idle_align_stub:
	LOADW	r12,g_strafe_held
	cmpwi	r12,0
	beq	1f
	bl	strafe_idle_align_entry
	cmpwi	r3,0
	bne	2f
1:	lfs	f1,0x30(r25)
	b	game_idle_align_back
2:	b	game_idle_to_walk

# Walk: replaces lfs f1, 0x80(r25), the stick deflection for walking on, which strafe_walk_align()
# gets as its argument. [RC2 0x84d770, 0x84d7f0]
PATCH 0x0C0974, walk_align, bl walk_align_stub
	.section .text.walk_align_stub, "ax"
walk_align_stub:
	LOADW	r12,g_strafe_held
	cmpwi	r12,0
	beq	1f
	lfs	f1,0x80(r25)
	bl	strafe_walk_align_entry
	cmpwi	r3,WALK_STAY
	beq	2f
	cmpwi	r3,WALK_TO_IDLE
	beq	3f
1:	lfs	f1,0x80(r25)
	b	game_walk_align_back
2:	b	game_walk_stays
3:	b	game_walk_to_idle

# ---- Flips -----------------------------------------------------------------------------------------

# Flip type function 0xa1d70: replaces bl stick_target. Strafing, the flip's type is the strafe
# direction (RC1's flip types are left, right, forward, back, as DIR_* in strafe.c). [RC2 0x833698]
PATCH 0x0A1D9C, flip_type, bl flip_type_stub
	.section .text.flip_type_stub, "ax"
flip_type_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	bne	1f
	b	game_stick_target
1:	LOADW	r3,g_strafe_dir
	b	game_flip_type_exit

# The same in 0xa1e78, which also gives the jump's buffer time from the type in r31. [RC2 0x83386c]
PATCH 0x0A1ECC, flip_time, bl flip_time_stub
	.section .text.flip_time_stub, "ax"
flip_time_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	bne	1f
	b	game_stick_target
1:	LOADW	r31,g_strafe_dir
	b	game_flip_time_tail

# Jump chooser 0xa4950: replaces cmpwi r31, 0, after RC1's own flip test (R1 or R2 held). Strafing
# with the stick pushed anywhere but forward, the jump is a flip. Both ways on from here load their
# own registers. [RC2 0x835e64]
PATCH 0x0A49F0, jump_choice, bl jump_choice_stub
	.section .text.jump_choice_stub, "ax"
jump_choice_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	bne	1f
	cmpwi	r31,0
	blr
1:	bl	strafe_jump_flips_entry
	cmpwi	r3,0
	bne	2f
	cmpwi	r31,0
	b	game_jump_choice_back
2:	b	game_jump_choice_flip

# set_state's entry for the flip (r27 = hero, r3 = the flip's type, live): replaces
# lfs f1, 0x180(r27), the heading the flip goes along. Strafing with the stick past
# FLIP_AIM_STICK it is the strafe heading, and the flip is marked as a strafe flip. [RC2 0x8487e0]
PATCH 0x0BB84C, flip_heading, bl flip_heading_stub
	.section .text.flip_heading_stub, "ax"
flip_heading_stub:
	lfs	f1,0x180(r27)
	li	r12,0
	lis	r11,g_strafe_flip@ha
	stw	r12,g_strafe_flip@l(r11)
	LOADW	r12,g_strafe
	cmpwi	r12,0
	beqlr
	lfs	f0,0x229c(r27)
	LOADF	f13,g_strafe_flip_aim
	fcmpu	cr0,f0,f13
	blelr
	li	r12,1
	lis	r11,g_strafe_flip@ha
	stw	r12,g_strafe_flip@l(r11)
	LOADF	f1,g_strafe_heading
	blr

# Replaces stfs f27, 0x458(r27) (f27 = 0; r3 = 1, live): a strafe flip starts at FLIP_SPEED and
# carries no speed from before (+0x454). [RC2 0x848814]
PATCH 0x0BB888, flip_speed, bl flip_speed_stub
	.section .text.flip_speed_stub, "ax"
flip_speed_stub:
	stfs	f27,0x458(r27)
	LOADW	r12,g_strafe_flip
	cmpwi	r12,0
	beqlr
	LOADF	f0,g_strafe_flip_start
	stfs	f0,0x458(r27)
	stfs	f27,0x454(r27)
	blr

# ---- Air function 0x9de38 (r31 = hero) ------------------------------------------------------------

# Replaces lhz r4, 0x41e(r31) at the flip's speed (state 0xb): a strafe flip's speed follows the
# stick, in strafe_flip_air(). RC1 calls approach() from both ways through this block, and the code
# behind it loads its own registers. [RC2 0x82f91c]
PATCH 0x09E5C0, flip_air, bl flip_air_stub
	.section .text.flip_air_stub, "ax"
flip_air_stub:
	LOADW	r12,g_strafe_flip
	cmpwi	r12,0
	bne	1f
	lhz	r4,0x41e(r31)
	blr
1:	bl	strafe_flip_air_entry
	b	game_flip_air_done

# Replaces lis r3, 0xa in front of the default air turn, which loads all its registers itself:
# strafing, strafe_air_turn() makes the camera's yaw the heading to turn to. [RC2 0x83007c]
PATCH 0x09EC5C, air_turn, bl air_turn_stub
	.section .text.air_turn_stub, "ax"
air_turn_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	beq	1f
	bl	strafe_air_turn_entry
1:	lis	r3,0xa
	b	game_air_turn_back

# Replaces stfs f1, 0x188(r31) behind the turn: strafing, the heading to move along is the strafe
# heading again. [RC2 0x830120]
PATCH 0x09ECBC, air_turned, bl air_turned_stub
	.section .text.air_turned_stub, "ax"
air_turned_stub:
	stfs	f1,0x188(r31)
	LOADW	r12,g_strafe
	cmpwi	r12,0
	beqlr
	LOADF	f0,g_strafe_heading
	stfs	f0,0x180(r31)
	blr

# Jump handler (r26 = hero table): replaces lfs f1, 0x128(r26), the heading clamp for
# build_velocity. Strafing, the velocity goes along the strafe heading. [RC2 0x8429e4]
PATCH 0x0B465C, jump_vel, bl jump_vel_stub
	.section .text.jump_vel_stub, "ax"
jump_vel_stub:
	lfs	f1,0x128(r26)
	LOADW	r12,g_strafe
	cmpwi	r12,0
	beqlr
	LOADF	f1,g_strafe_heading
	blr

# ---- Lean function 0x9f300 (r31 = hero; called without arguments) ---------------------------------

# Replaces lwz r3, 0x2084(r31) at its top. Strafing and moving sideways, strafe_lean() twists the
# body and the function returns at once. It has not saved r30 yet and its exit restores it, so the
# stub saves it there first (r30 is still the caller's: C never touches r14-r31). [RC2 0x830a5c]
PATCH 0x09F31C, lean, bl lean_stub
	.section .text.lean_stub, "ax"
lean_stub:
	LOADW	r12,g_strafe
	cmpwi	r12,0
	beq	1f
	bl	strafe_lean_entry
	cmpwi	r3,0
	bne	2f
1:	lwz	r3,0x2084(r31)
	b	game_lean_back
2:	std	r30,0x98(r1)
	b	game_lean_exit
