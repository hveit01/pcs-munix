| PCS

	.text
	.globl	_rindex
_rindex:
arg0 =  8
arg4 =  12

	link	a6, #0
	movl	a6@(arg0), a0
	movl	a6@(arg4), d1
	clrl	d0

1$:
	cmpb	a0@, d1
	bnes	2$
	movl	a0, d0

2$:
	tstb	a0@+
	bnes	1$
	unlk	a6
	rts
