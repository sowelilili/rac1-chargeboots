# Hook words for the Charge Boots, and the small stubs that work on the game's registers.
#
# PATCH addr, name, instruction: the word written over the game's instruction at addr.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

.set STATE_IDLE, 0x00

# ---- Calls the game makes, taken over by src/asm/glue.s -----------------------------------
PATCH 0x08C3FC, frame_a, bl frame_entry           # bl hero_main (0x8c3d0)
PATCH 0x0955A0, frame_b, bl frame_entry           # bl hero_main (0x954c8)
PATCH 0x08C400, surface_a, bl surface_entry       # bl surface, right behind
PATCH 0x0955A4, surface_b, bl surface_entry
PATCH 0x08B93C, update, bl update_entry           # bl hero_update in hero_main
PATCH 0x0BE2FC, chooser, bl chooser_entry         # bl chooser in the transition function

# ---- Transition function 0xbe228 (r19 = hero) ----------------------------------------------
# Replaces lwz r3, 0x2084(r19), the load of the state for the per-state transition switch: the
# point where RC2 starts charges and runs its charge state's transitions (behind the weapon
# chooser). While a charge starts or runs, charge_trans() decides, in place of the state's own
# transitions (no jump, crouch or fall); on the frame a charge ends, the skid's own wait for the
# next frame, as RC2's do.
PATCH 0x0BE71C, trans, bl trans_stub
	.section .text.trans_stub, "ax"
trans_stub:
	lis	r11,g_charge_frames@ha
	lwz	r12,g_charge_frames@l(r11)
	lis	r11,g_charge_pending@ha
	lwz	r11,g_charge_pending@l(r11)
	or.	r12,r12,r11
	lwz	r3,0x2084(r19)
	beqlr
	bl	charge_trans_entry
	cmpwi	r3,0
	lwz	r3,0x2084(r19)
	bne	1f
	b	game_trans_switch
1:	b	game_trans_exit

# Skid state (0xc13dc): replaces li r3, 0 behind its stick test. RC2 keeps a skid that ended a
# charge going while the lock hero +0x1c4 runs; RC1 has no such test. (Its jump and crouch already
# wait for the lock, in both games.)
PATCH 0x0C14F0, skid, bl skid_stub
	.section .text.skid_stub, "ax"
skid_stub:
	lis	r11,g_charge_slide@ha
	lwz	r12,g_charge_slide@l(r11)
	cmpwi	r12,0
	beq	1f
	lwz	r12,0x1c4(r19)
	cmpwi	r12,0
	beq	1f
	b	game_trans_exit
1:	li	r3,0
	blr

# Skid state update (hero_update's state 3 block; r26 = the hero table 0xaaafc, r3 is live):
# replaces lfs f1, 0x3c(r26), the skid's deceleration of 12 u/s^2. A skid that ended a charge
# slows at RC2's 27 instead. The game still scales it by its surface factor (f31).
PATCH 0x0AB830, skid_decel, bl skid_decel_stub
	.section .text.skid_decel_stub, "ax"
skid_decel_stub:
	lfs	f1,0x3c(r26)
	lis	r11,g_charge_slide@ha
	lwz	r12,g_charge_slide@l(r11)
	cmpwi	r12,0
	beqlr
	lis	r11,g_skid_decel@ha
	lfs	f1,g_skid_decel@l(r11)
	blr

# The same block for a crouch (state 4, on ordinary ground): replaces lfs f1, 0x58(r26), 6.48.
PATCH 0x0AB960, crouch_decel, bl crouch_decel_stub
	.section .text.crouch_decel_stub, "ax"
crouch_decel_stub:
	lfs	f1,0x58(r26)
	lis	r11,g_charge_slide@ha
	lwz	r12,g_charge_slide@l(r11)
	cmpwi	r12,0
	beqlr
	lis	r11,g_crouch_decel@ha
	lfs	f1,g_crouch_decel@l(r11)
	blr

# And for the other states that come here (r25 = hero): replaces lfs f1, 0x5c(r26), 12.6. Only the
# idle state takes RC2's rate, and always, not only after a charge: RC2's walk stops into it
# (src/c/walk.c, move.c).
PATCH 0x0AB974, idle_decel, bl idle_decel_stub
	.section .text.idle_decel_stub, "ax"
idle_decel_stub:
	lfs	f1,0x5c(r26)
	lwz	r12,0x2084(r25)
	cmpwi	r12,STATE_IDLE
	bnelr
	lis	r11,g_idle_decel@ha
	lfs	f1,g_idle_decel@l(r11)
	blr

# ---- Weapons (chooser 0xa30e0, r20 = hero) ----------------------------------------------------
# Glove case (Bomb, Mine and Decoy Gloves, Glove of Doom, Drones): replaces lwz r3, 0x208c(r20)
# in front of the choice between the standing throw (set_state 0x23, which stops him) and the
# moving throw (0x99ea8, an override on his upper body). When charge.c asks for it, always the
# moving throw: RC2's gloves throw from their own code, without a change of state.
PATCH 0x0A3D88, glove_throw, bl glove_throw_stub
	.section .text.glove_throw_stub, "ax"
glove_throw_stub:
	lwz	r3,0x208c(r20)
	lis	r11,g_moving_throw@ha
	lwz	r12,g_moving_throw@l(r11)
	cmpwi	r12,0
	beqlr
	b	game_glove_moving_throw

# Glove case: Circle held fires only while the glove has been out more than 8 and less than 17
# frames (+0x10b0, counted from when it came out, not from the last throw), so holding Circle throws
# once; a press within the last 7 frames fires at any time. RC2 fires on Circle held as on a press
# [0x835a18], so holding it keeps throwing, each throw once the last one's animation is over (the
# override test in front). Replaces bge 0xa46e0, the 17-frame limit.
PATCH 0x0A3D34, glove_hold, nop

# Throw converter 0xa2600 (run by the transition function before its per-state switch, so also
# while charging): in the idle, walk or skid state, a glove's moving throw in its first frames
# with the stick below 0.7 becomes the standing throw. RC2's (0x8342c8) never sees the charge,
# which is a state of its own, nor a glove's throw that ended a charge, whose flag RC2's skid
# animation clears; a throw the port keeps (charge.c, end_weapon_use) must go on as it began.
# Replaces lwz r3, 0x2084(r31), its load of the state: while charging or sliding on after a
# charge, a state it does not take.
PATCH 0x0A2618, throw_converter, bl throw_converter_stub
	.section .text.throw_converter_stub, "ax"
throw_converter_stub:
	lwz	r3,0x2084(r31)
	lis	r11,g_charge_frames@ha
	lwz	r12,g_charge_frames@l(r11)
	lis	r11,g_charge_slide@ha
	lwz	r11,g_charge_slide@l(r11)
	or.	r12,r12,r11
	beqlr
	li	r3,-1
	blr

# Devastator (moby update 0x29ead8, r25 = hero), firing: in the walk state with the stick below
# 0.7 it only plays its animation on the whole body (and in the idle state), otherwise it uses
# the weapon (0x99ea8, the flag that ends a charge). RC2's charge is no walk, so its launchers end
# it. Replaces lwz r3, 0x2084(r25) in front of the walk test: while charging, not the walk.
PATCH 0x29FA78, devastator, bl devastator_stub
	.section .text.devastator_stub, "ax"
devastator_stub:
	lwz	r3,0x2084(r25)
	lis	r11,g_charge_frames@ha
	lwz	r12,g_charge_frames@l(r11)
	cmpwi	r12,0
	blelr
	li	r3,-1
	blr
