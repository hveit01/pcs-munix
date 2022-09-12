	.text
	.global _strncmp
_strncmp:
s1 = 8
s2 = 12
n = 16

	link	a6, #0
	movl	a6@(s1), a1
	movl	a6@(s2), a2
	movl	a6@(n), d1

1$:	subql	#1, d1
	blts	2$
	cmpmb	a1@+, a2@+
	bnes	3$
	tstb	a1@(-1)
	bnes	1$

2$: clrl	d0
	bras	4$

3$:
	movb	a1@-, d0
	subb	a2@-, d0
	extw	d0
	extl	d0

4$:	
	unlk	a6
	rts
