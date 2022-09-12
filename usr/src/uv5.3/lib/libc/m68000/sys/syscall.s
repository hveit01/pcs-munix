	.text
	.global _syscall
	.extern	_errno

| this is easier to write in asm than in C

_syscall:
err  = -0x24
sysc = -0x20
argv =  8
	link	a6, #-0x24
	lea		a6@(argv), a0
	lea		a0@(4),	a1
	movl	a6@(argv),	d1
	movl	a0@-, a1@-
	movl	a0@-, a1@-
	movl	a1,	a6
	movl	d1, a6@(sysc)
	trap	#0x0D
	movl	a6@(err), d1
	tstl	d1
	beqw	1$
	movl	d1, _errno
	moveq	#-1, d0
1$:	unlk	a6
	rts
