| PCS

	.text
	.globl	_lmemset
	.globl	_memset

_lmemset:
_memset:
arg0 =  8
arg4 =  12
arg8 =  16

	link	a6, #0
	movl	a6@(arg0), a0
	movl	a0, d0
	movl	a6@(arg4), d1
	movl	a6@(arg8), d2

1$:
	tstl	d2
	bles	5$
	cmpl	#0x7D00, d2
	bgts	3$
	subqw	#1, d2

2$:
	movb	d1, a0@+
	dbf		d2, 2$
	bras	5$

3$:
	subl	#0x7D00, d2
	movw	#0x7CFF, d3

4$:
	movb	d1, a0@+
	dbf		d3, 4$
	bras	1$

5$:
	unlk	a6
	rts
