	.text
	.global _strncat
_strncat:
s1 = 8
s2 = 12
n = 16

	link	a6, #0
	movl	a6@(s1), d0
	movl	d0, a1
	movl	a6@(s2), a2
	movl	a6@(n), d1

1$:	tstb	a1@+
	bnes	1$
	subql	#1, a1

2$:	movb	a2@+, a1@+
	beqs	3$
	subql	#1, d1
	bges	2$
	clrb	a1@-

3$:	unlk	a6
	rts
