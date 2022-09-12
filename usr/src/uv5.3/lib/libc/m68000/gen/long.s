
	.text
	.global imul
imul:
arg0 = 8
arg2 = 10
arg4 = 12
var4 = -4
var6 = -6
var8 = -8

	link	a6, #-8
	movl	d1,	sp@-
	clrl	a6@(var8)
	movw	a6@(arg2), d0
	movl	a6@(arg4), d1
	mulu	d1,	d0
	movl	d0,	a6@(var4)
	movw	a6@(arg0), d0
	mulu	d1,	d0
	addl	d0,	a6@(var6)
	swap	d1
	movw	a6@(arg2), d0
	mulu	d1, d0
	addl	d0, a6@(var6)
	movl	a6@(var4), d0
	movl	sp@+, d1
	unlk	a6
	rts

| internal division helper
_div:
	movl	d0, d3
	movl	d1, d4
	movl	#0xFFFF, d2
	cmpl	d2, d1
	bhiw	1$
	clrw	d0
	swap	d0
	divu	d1, d0
	movw	d0, d2
	movw	d3, d0
	divu	d1, d0
	swap	d0
	movw	d2, d0
	swap	d0
	rts
1$:	lsrl	#1,	d0
	lsrl	#1,	d1
	cmpl	d2,	d1
	bgtw	1$
	divu	d1,	d0
	andl	d2,	d0
	movl	d0,	d2
	movl	d0,	sp@-
	movl	d4,	sp@-
	jsr		imul
	addql	#8,	sp
	cmpl	d0,	d3
	bccw	2$
	subql	#1,	d2
2$:
	movl	d2,	d0
	rts

	.global udiv
udiv:
arg0 = 8
arg4 = 12
	link	a6,#0
	moveml	#0x7800, sp@-
	movl	a6@(arg0), d0
	movl	a6@(arg4), d1
	bsrw	_div
	tstl	d0
	moveml	sp@+, #0x001e
	unlk	a6
	rts

	.global idiv
idiv:
arg0 = 8
arg4 = 12
	link	a6,#0
	moveml	#0x7c00,sp@-
	moveq	#1, d5
	movl	a6@(arg0), d0
	bgew	1$
	negl	d0
	negw	d5
1$:	movl	a6@(arg4), d1
	bgew	2$
	negl	d1
	negw	d5
2$:	bsrw	_div
	tstw	d5
	bgew	3$
	negl	d0
3$:	tstl	d0
	moveml	sp@+, #0x003e
	unlk	a6
	rts

| internal modulo helper
_mod:
	movl	d0,	d3
	movl	d1,	d4
	movl	#0xFFFF, d2
	cmpl	d2, d1
	bhiw	1$
	clrw	d0
	swap	d0
	divu	d1, d0
	movw	d3, d0
	divu	d1, d0
	clrw	d0
	swap	d0
	rts
1$:	lsrl	#1,	d0
	lsrl	#1,	d1
	cmpl	d2,	d1
	bgtw	1$
	divu	d1, d0
	andl	d2, d0
	movl	d0, sp@-
	movl	d4, sp@-
	jsr		imul
	addql	#8, sp
	cmpl	d0, d3
	bccw	2$
	subl	d4,	d0
2$:	subl	d3,	d0
	negl	d0
	rts

	.global umod
umod:
arg0 = 8
arg4 = 12
	link	a6, #0
	moveml	#0x7800, sp@-
	movl	a6@(arg0), d0
	movl	a6@(arg4), d1
	bsrw	_mod
	tstl	d0
	moveml	sp@+, #0x001e
	unlk	a6
	rts

	.global imod
imod:
arg_0		=  8
arg_4		=  $C
	link	a6,	#0
	moveml	#0x7800, sp@-
	movl	a6@(arg0), d0
	bgew	1$
	negl	d0
1$:	movl	a6@(arg4), d1
	bgew	2$
	negl	d1
2$:	bsrw	_mod
	tstl	a6@(arg0)
	bgew	3$
	negl	d0
3$:	tstl	d0
	moveml	sp@+, #0x001e
	unlk	a6
	rts
