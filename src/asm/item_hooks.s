# The Charge Boots take the place of the Grindboots (item 0x1d): RC1 sells them on Blarg (the
# Scrawny Scientist, the game's only GiveItem(0x1d); 1,000 bolts here, src/patch.txt), keeps them
# in the save and shows them under Foot Items. Owning them still lets Ratchet grind on rails. Here their texts become the
# Charge Boots' and challenge mode keeps them. (charge.c reads the item's owned flag.)
#
# PATCH addr, name, instruction: the word written over the game's instruction at addr.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name, "ax"
	\instr
.endm

# ---- Texts ---------------------------------------------------------------------------------------
# TextLookup 0x7b740 (r3 = text id -> r3 = the string): the ids in boots_texts get our strings, in
# every language. Replaces its first instruction, lis r5, 0x97, with a plain branch, so its caller's
# return address stays in LR; r5 and r6 are loaded again behind it. Not covered: the help messages
# (the purchase's and the rail's), which the help system finds without TextLookup, and their voice.
PATCH 0x07B740, text_lookup, b text_lookup_stub
	.section .text.text_lookup_stub, "ax"
text_lookup_stub:
	lis	r5,boots_texts@ha
	addi	r5,r5,boots_texts@l
1:	lwz	r6,0(r5)
	cmpwi	r6,0
	beq	2f
	cmpw	r6,r3
	addi	r5,r5,8
	bne	1b
	lwz	r3,-4(r5)
	blr
2:	lis	r5,0x97
	b	game_text_lookup_next

# {text id, string}. In the strings \014..\010 highlights, \025 is the R1 icon, \022 the triangle.
	.section .rodata.boots_texts, "a"
	.balign 4
boots_texts:
	.long	0x4E68, t_name          # the item's name (pause menu)
	.long	0x4F88, t_description   # its description (pause menu)
	.long	0x177D, t_bought        # the message when Ratchet gets it (one short line: more runs off screen)
	.long	0x1787, t_buy           # the scientist's offer
	.long	0x1788, t_no_bolts
	.long	0x534E, t_mission       # the mission in the pause menu
	.long	0x5349, t_mission_text
	.long	0x4EA8, t_mission_short
	.long	0
t_name:
	.asciz	"Charge Boots"
t_description:
	.asciz	"Tap \025 twice and hold it, and the \014Charge Boots\010 dash Ratchet along the ground. Let go of \025 after a second, use a weapon or run into a wall to stop. They still grind on rails."
t_bought:
	.asciz	"You got the \014Charge Boots\010!"
t_buy:
	.asciz	"\022 Buy \014Charge Boots\010 for 1,000 bolts"
t_no_bolts:
	.asciz	"You need 1,000 bolts for the \014Charge Boots\010"
t_mission:
	.asciz	"Buy Charge Boots"
t_mission_text:
	.asciz	"This is definitely a worthwhile purchase. Charge Boots are rare but very useful."
t_mission_short:
	.asciz	"Buy Charge Boots for 1,000 bolts"

# ---- Challenge mode ------------------------------------------------------------------------------
# 0x13e634 saves the items owned, resets the game and gives back only the items of its list
# 0x731698 (the PDA among them, not the Grindboots), and never gives back "obtained" (0x96c165),
# which the scientist checks before selling them again. Behind that loop (r27 = the saved copy, with
# owned at +0x30; r26 = owned 0x96c140): the Charge Boots come back, owned and obtained. Replaces
# ori r3, r22, 0; the function keeps its return address on its stack, so the bl is safe.
PATCH 0x13E85C, carry_boots, bl carry_boots_stub
	.section .text.carry_boots_stub, "ax"
carry_boots_stub:
	lbz	r0,0x30+0x1D(r27)
	stb	r0,0x1D(r26)            # owned[0x1d]
	stb	r0,0x25+0x1D(r26)       # obtained[0x1d], 0x96c182
	ori	r3,r22,0
	blr
