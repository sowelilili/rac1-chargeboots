# Hook words for RC2's way of stopping (src/c/move.c).
#
# PATCH addr, name, instruction: the word written over the game's instruction at addr.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

# Walk transitions (0xc03e4, r19 = hero), the stick let go (below 0.17): replaces lis r3, 0x72 at
# the head of RC1's choice between its skid (more than 2.7 units/s last frame, no gun) and
# standing up once the walk has lasted 25 frames. Always straight on to RC1's walk -> idle.
# [RC2 0x84d7a4]
PATCH 0x0C0988, walk_stop, b game_walk_to_idle

# Walk -> idle, behind a set_state(0, 0) that worked: replaces li r3, 0 in front of the idle
# animation's choice. [RC2 0x84d808]
PATCH 0x0C0AE0, walk_stop_cap, bl walk_stop_cap_stub
	.section .text.walk_stop_cap_stub, "ax"
walk_stop_cap_stub:
	bl	stop_cap_entry
	li	r3,0
	b	game_walk_idle_anim

# Gun stance 0xa9d70, behind its set_state(0, 0) that worked: replaces lis r3, 0xb (the volatile
# registers are loaded again behind it). [RC2 0x8375d8]
PATCH 0x0A9E78, gun_stop_cap, bl gun_stop_cap_stub
	.section .text.gun_stop_cap_stub, "ax"
gun_stop_cap_stub:
	bl	stop_cap_entry
	lis	r3,0xb
	b	game_gun_stance_anim

# ---- In the air (air function 0x9de38; r31 = hero, r3 = its constant pool 0x9dd74) ----------------
# The stick let go in a jump: replaces lfs f2, 0x48(r3), RC1's brake of 2 units/s^2, where he drifts
# on. RC2 brakes at 8, and at 2 only once he has touched a wall (+0x524, RC1's +0x4a4) [RC2
# 0x830230]; 8 is already in the pool at +0x44. (With +0x30a set, RC1's 6 is RC2's 11: a data word
# in src/patch.txt.)
PATCH 0x09EE14, air_brake, bl air_brake_stub
	.section .text.air_brake_stub, "ax"
air_brake_stub:
	lhz	r12,0x4a4(r31)
	lfs	f2,0x44(r3)
	cmpwi	r12,0
	beqlr
	lfs	f2,0x48(r3)
	blr

# ---- Turning while attacking (hero_update; r26 = hero table 0xaaafc) ---------------------------
# The hero table's +0x250 is the top turn speed of the wrench combo, hyper-strike, comet strike,
# state 0x20, the bump and the glove throw (seven reads), and RC2 raises it to 16.93 rad/s (a data
# word in src/patch.txt) [RC2 table +0x188]. Its two other reads keep RC1's and RC2's 15.01: the jumps
# here and 0xb5470. Each replaces lfs fN, 0x250(r26).
PATCH 0x0B45DC, jump_turn_max, bl jump_turn_max_stub
PATCH 0x0B5470, other_turn_max, bl other_turn_max_stub
	.section .text.jump_turn_max_stub, "ax"
jump_turn_max_stub:
	lis	r11,turn_max_15@ha
	lfs	f2,turn_max_15@l(r11)
	blr
	.section .text.other_turn_max_stub, "ax"
other_turn_max_stub:
	lis	r11,turn_max_15@ha
	lfs	f3,turn_max_15@l(r11)
	blr
	.section .rodata.turn_max_15, "a"
	.balign 4
turn_max_15:
	.long	0x41702845		# 15.009831 rad/s, RC1's +0x250 as it was [RC2 table +0x31c]
