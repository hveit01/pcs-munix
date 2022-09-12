	.text
	.global _strcmp
_strcmp:
s1 = 8
s2 = 12

	link	a6, #0
	movl	a6@(s1), a1
	movl	a6@(s2), a2

1$:	cmpmb	a1@+, a2@+
	bnes	2$
	tstb	a1@(-1)
	bnes	1$
	clrl	d0
	bras	3$

2$:	movb	a1@-, d0
	subb	a2@-, d0
	extw	d0
	extl	d0

3$:	unlk	a6
	rts
