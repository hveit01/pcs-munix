	.text
	.global _strchr
_strchr:
s1 = 8
c  = 12

	link	a6, #0
	movl	a6@(s1), a0
	movl	a6@(c), d1

1$:	cmpb	a0@, d1
	bnes	2$
	movl	a0, d0
	bras	3$

2$:	tstb	a0@+
	bnes	1$
	clrl	d0
	
3$:	unlk	a6
	rts
