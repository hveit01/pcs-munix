| PCS

	.text
	.globl	_lmemccpy
	.globl  _memccpy

_lmemccpy:
_memccpy:
arg0 = 8
arg4 = 12
arg8 = 16
arg12= 20

	link	a6, #0

	movl	a6@(arg0), a0
	movl	a6@(arg4), a1
	movl	a6@(arg8), d1
	movl	a6@(arg12), d2

1$:
	subql	#1, d2
	blts	2$

	movb	a1@+, a0@
	cmpb	a0@+, d1
	bnes	1$

	movl	a0, d0
	bras	3$

2$:
	clrl	d0

3$:
	unlk	a6
	rts
