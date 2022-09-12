
| typedef long[9] jmp_buf;

	.text
	.global _setjmp
_setjmp:
retp = 0
linkp = 4
env	= 8

	link	a6, #0
	movl	a6@(env), a0		| jmp_buf
	movl	a6@(linkp), a0@		| jmp_buf[0] = old linkp
	movl	a6, a0@(4)			| jmp_buf[1] = current linkp
	movl	a6@, a0@(8)			| jmp_buf[2] = ret pc
	moveml	#0x38e0, a0@(12)	| jmp_buf[3-8] = regs d7-5,a5-3
	clrl	d0
	unlk	a6
	rts



	.global _longjmp
_longjmp:
linkp = 4
env = 8
retval = 12

	link	a6,#0
	movl	a6@(env), a0		| jmp_buf
	movl	a0@(4), a1			| tmplinkp = jmp_buf[1]
	movl	a0@, a1@(linkp)		| old linkp = jmp_buf[0]
	movl	a0@(8), a1@			| ret pc = jmp_buf[2]
	moveml	a0@(12), #0x38e0	| regs d7-5, a5-3 = jmp_buf[3-8]

	movl	a6@(retval), d0		| return value
	bnes	1$
	moveq	#1, d0				| ensure it is != 0

1$:	movl	a1, a6				| restore current linkp
	unlk	a6					| exit
	rts
	
	.even

