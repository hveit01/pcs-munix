	.text
	.global _strcat
_strcat:
s1 = 8
s2 = 12

	link	a6, #0
	movl	a6@(s1), d0
	movl	d0, a1
	movl	a6@(s2), a2

1$:	tstb	a1@+
	bnes	1$
	subql	#1, a1
	
2$:	movb	a2@+, a1@+
	bnes	2$
	unlk	a6
	rts
