| PCS

	.text
	.globl	_lmemcpy
	.globl	_memcpy

_lmemcpy:
_memcpy:
arg0 =  8
arg4 =  12
arg8 =  16

	link	a6, #0
	movl	a6@(arg0), a0
	movl	a6@(arg4), a1
	movl	a6@(arg8), d1
	movl	a0, d0

1$:
	tstl	d1
	bles	5$
	cmpl	#0x7D00, d1
	bgts	3$
	subqw	#1, d1

2$:
	movb	a1@+, a0@+
	dbf		d1, 2$
	bras	5$

3$:
	subl	#0x7D00, d1
	movw	#0x7CFF, d2

4$:
	movb	a1@+, a0@+
	dbf		d2, 4$
	bras	1$

5$:
	unlk	a6
	rts
