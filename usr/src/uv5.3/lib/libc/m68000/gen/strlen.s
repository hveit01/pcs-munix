	.text
	.global _strlen
_strlen:
s = 8

	link	a6, #0
	movl	a6@(s), a0
	clrl	d0

1$:	tstb	a0@+
	beqs	2$
	addql	#1, d0
	bras	1$

2$:	unlk	a6
	rts
