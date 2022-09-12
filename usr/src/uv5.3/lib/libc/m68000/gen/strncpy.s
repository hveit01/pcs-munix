	.text
	.global _strncpy
_strncpy:
s1 = 8
s2 = 12
n = 16

	link	a6, #0
	movl	a6@(s1), d0
	movl	d0, a1
	movl	a6@(s2), a2
	movl	a6@(n), d1

1$:
	subql	#1, d1
	blts	3$
	movb	a2@+, a1@+
	bnes	1$

2$:	subql	#1,d1
	blts	3$
	clrb	a1@+
	bras	2$

3$:	unlk	a6
	rts
