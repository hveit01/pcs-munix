	.text
	.globl _bcopy
_bcopy:
s1 = 8
s2 = 12
n = 16

	link	a6, #0
	movl	a6@(s1), a0
	movl	a6@(s2), a1
	movl	a6@(n), d1
	bles	8$
	movw	a0, d0
	andw	#3, d0
	beqs	2$
	negw	d0
	addw	#3, d0

1$: movb	a0@+, a1@+
	subl	#1, d1
	dbeq	d0, 1$

2$: movl	d1, d0
	lsrl	#2, d1
	bras	5$

3$: swap	d1

4$: movl	a0@+, a1@+

5$: dbf		d1, 4$
	swap	d1
	dbf	d1, 3$
	andw	#3, d0
	bras	7$

6$: movb	a0@+, a1@+

7$: dbf		d0, 6$

8$: clrl	d0
	unlk	a6
	rts
