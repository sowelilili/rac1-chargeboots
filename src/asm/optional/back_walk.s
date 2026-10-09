# RC2's backwards walk (strafe.c, strafe_walk_anim). Built and patched only when BACK_ANIM_ADDR in
# src/c/strafe.c is not 0 (the Makefile reads it); without it RC1's walk animation chooser is left
# alone.

.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

# A C function for the game, as a plain call (as in src/asm/glue.s).
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

.set ANIM_DONE, 1               # strafe_walk_anim's results, as in strafe.c
.set ANIM_RATE, 2

ENTRY strafe_walk_anim_entry, strafe_walk_anim

# Walk animation chooser 0xa4a60 (r31 = hero; called without arguments): replaces lis r4, 0xa in
# front of RC1's gait choice, where the volatile registers are free (the chooser loads them all
# from here on). Strafing backwards, strafe_walk_anim() puts RC2's animation on him and the chooser
# returns; afterwards it puts his gait animation back and only the animation speed is left to the
# chooser. [RC2 0x836018]
PATCH 0x0A4AEC, walk_anim, bl walk_anim_stub
	.section .text.walk_anim_stub, "ax"
walk_anim_stub:
	lis	r11,g_strafe@ha
	lwz	r12,g_strafe@l(r11)
	lis	r11,g_back_walk@ha
	lwz	r11,g_back_walk@l(r11)
	or.	r12,r12,r11
	bne	1f
	lis	r4,0xa
	blr
1:	bl	strafe_walk_anim_entry
	cmpwi	r3,ANIM_DONE
	beq	2f
	cmpwi	r3,ANIM_RATE
	beq	3f
	lis	r4,0xa
	b	game_walk_anim_back
2:	b	game_walk_anim_exit
3:	b	game_walk_anim_rate
