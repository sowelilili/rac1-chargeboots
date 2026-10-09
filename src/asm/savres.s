# Out-of-line float register saves and restores that gcc -Os calls (32-bit SVR4, as in
# libgcc's crtsavres): r11 points at the top of the save area; the _x restores also return
# from the caller (LR from 4(r11), r1 = r11).
	.section .text.savres, "ax"
	.globl _savefpr_14
	.globl _savefpr_15
	.globl _savefpr_16
	.globl _savefpr_17
	.globl _savefpr_18
	.globl _savefpr_19
	.globl _savefpr_20
	.globl _savefpr_21
	.globl _savefpr_22
	.globl _savefpr_23
	.globl _savefpr_24
	.globl _savefpr_25
	.globl _savefpr_26
	.globl _savefpr_27
	.globl _savefpr_28
	.globl _savefpr_29
	.globl _savefpr_30
	.globl _savefpr_31
_savefpr_14:	stfd	f14,-144(r11)
_savefpr_15:	stfd	f15,-136(r11)
_savefpr_16:	stfd	f16,-128(r11)
_savefpr_17:	stfd	f17,-120(r11)
_savefpr_18:	stfd	f18,-112(r11)
_savefpr_19:	stfd	f19,-104(r11)
_savefpr_20:	stfd	f20,-96(r11)
_savefpr_21:	stfd	f21,-88(r11)
_savefpr_22:	stfd	f22,-80(r11)
_savefpr_23:	stfd	f23,-72(r11)
_savefpr_24:	stfd	f24,-64(r11)
_savefpr_25:	stfd	f25,-56(r11)
_savefpr_26:	stfd	f26,-48(r11)
_savefpr_27:	stfd	f27,-40(r11)
_savefpr_28:	stfd	f28,-32(r11)
_savefpr_29:	stfd	f29,-24(r11)
_savefpr_30:	stfd	f30,-16(r11)
_savefpr_31:	stfd	f31,-8(r11)
	blr
	.globl _restfpr_14_x
	.globl _restfpr_15_x
	.globl _restfpr_16_x
	.globl _restfpr_17_x
	.globl _restfpr_18_x
	.globl _restfpr_19_x
	.globl _restfpr_20_x
	.globl _restfpr_21_x
	.globl _restfpr_22_x
	.globl _restfpr_23_x
	.globl _restfpr_24_x
	.globl _restfpr_25_x
	.globl _restfpr_26_x
	.globl _restfpr_27_x
	.globl _restfpr_28_x
	.globl _restfpr_29_x
	.globl _restfpr_30_x
	.globl _restfpr_31_x
_restfpr_14_x:	lfd	f14,-144(r11)
_restfpr_15_x:	lfd	f15,-136(r11)
_restfpr_16_x:	lfd	f16,-128(r11)
_restfpr_17_x:	lfd	f17,-120(r11)
_restfpr_18_x:	lfd	f18,-112(r11)
_restfpr_19_x:	lfd	f19,-104(r11)
_restfpr_20_x:	lfd	f20,-96(r11)
_restfpr_21_x:	lfd	f21,-88(r11)
_restfpr_22_x:	lfd	f22,-80(r11)
_restfpr_23_x:	lfd	f23,-72(r11)
_restfpr_24_x:	lfd	f24,-64(r11)
_restfpr_25_x:	lfd	f25,-56(r11)
_restfpr_26_x:	lfd	f26,-48(r11)
_restfpr_27_x:	lfd	f27,-40(r11)
_restfpr_28_x:	lfd	f28,-32(r11)
_restfpr_29_x:	lfd	f29,-24(r11)
_restfpr_30_x:	lfd	f30,-16(r11)
_restfpr_31_x:	lwz	r0,4(r11)
	lfd	f31,-8(r11)
	mtlr	r0
	mr	r1,r11
	blr
