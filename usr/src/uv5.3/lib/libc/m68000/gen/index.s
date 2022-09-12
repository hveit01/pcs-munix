| PCS

	.text
	.globl _index

_index:
arg0 = 8
arg4 = 12

	link	a6, #0
	movl	a6@(arg0), a0
	movl	a6@(arg4), d1

1$:
	cmpb	a0@, d1
	bnes	2$

	movl	a0, d0
	bras	3$
2$:
	tstb	a0@+
	bnes	1$

	clrl	d0
3$:
	unlk	a6
	rts
