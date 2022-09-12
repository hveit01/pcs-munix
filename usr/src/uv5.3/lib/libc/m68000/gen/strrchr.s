	.text
	.global _strrchr
_strrchr:
s1 = 8
c = 12

	link	a6, #0
	movl	a6@(s1), a0
	movl	a6@(c), d1
	clrl	d0

1$:	cmpb	a0@, d1
	bnes	2$
	movl	a0, d0

2$:	tstb	a0@+
	bnes	1$
	
	unlk	a6
	rts
