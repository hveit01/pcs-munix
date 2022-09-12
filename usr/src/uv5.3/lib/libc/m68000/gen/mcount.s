	
	.text
	.global mcount
	.extern __countbase
mcount:

	tstl	a0@
	beqs	1$
	movl	a0@, a1
	bras	2$

1$:	movl	__countbase, d0
	beqs	3$
	movl	d0, a1
	addql	#8, __countbase
	movl	sp@, a1@+
	movl	a1, a0@

2$:	addql	#1, a1@

3$:	rts
