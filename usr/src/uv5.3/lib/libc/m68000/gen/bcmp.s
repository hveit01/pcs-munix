	.text
	.globl _bcmp
_bcmp:

s1	=  8
s2	=  12
n	=  16

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
		
1$:	cmpmb	a0@+, a1@+
	bnes	9$
	subl	#1,d1
	dbeq	d0, 1$

2$:	movl	d1, d0
	lsrl	#2, d1
	bras	5$

3$:	swap	d1

4$: cmpml	a0@+, a1@+
	bnes	9$

5$: dbf		d1, 4$
	swap	d1
	dbf		d1, 3$
	andw	#3, d0
	bras	7$

6$: cmpmb	a0@+, a1@+
	bnes	9$

7$: dbf		d0, 6$

8$:	clrl	d0
	bras	10$

9$: moveq	#1, d0

10$: 
	unlk	a6
	rts
