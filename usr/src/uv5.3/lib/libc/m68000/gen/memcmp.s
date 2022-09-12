| PCS

	.text
	.globl _lmemcmp
	.globl _memcmp

_lmemcmp:
_memcmp:
arg0 =  8
arg4 =  12
arg8 =  16

	link	a6, #0
	movl	a6@(arg0), a0
	movl	a6@(arg4), a1
	movl	a6@(arg8), d1
	cmpl	a0, a1
	beqs	2$

1$:
	subql	#1, d1
	blts	2$
	cmpmb	a0@+, a1@+
	beqs	1$
	movb	a0@-, d0
	subb	a1@-, d0
	extw	d0
	extl	d0
	bras	3$

2$:
	clrl	d0

3$:
	unlk	a6
	rts
