
| fast copy routine
| copy n bytes from s1 to s2

    .text
    .global _blt
_blt:
s1 = 8
s2 = 12
n = 16

    link    a6, #0
    movl    sp@(s1), a1     | a1 is 1st arg
    movl    sp@(s2), a0     | a0 is 2nd arg

    movw    a0, d0          | both even or odd?
    andw    #1, d0
    movw    a1, d1
    andw    #1, d1
    cmpw    d0, d1
    beqs    1$              | yes

    movl    sp@(n), d1      | no, get count
    bras    5$              | and go copying bytes

1$: movl    sp@(n), d1      | get count
    movw    a0, d0          | target is long aligned?
    andw    #3, d0
    beqs    3$              | yes       

    negw    d0               
    addqw   #4, d0          | 4 - bytes at target
    extw    d0
    cmpl    d1, d0          | more than a long difference?
    bges    5$              | yes
    subl    d0, d1          | no, d1 is number of longs to copy
    subql   #1, d0          | d0 is number of bytes

2$: movb    a1@+, a0@+      | copy bytes until at long alignment
    dbf     d0, 2$

3$: movl    d1, d0          | number of longs
    andl    #-4, d0         | mask remaining bytes into d0
    beqs    5$              | result is 0? yes, no longs to copy
    subl    d0, d1          | make number of longs 
    asrl    #2, d0          | divide by 4

4$: movl    a1@+, a0@+      | copy long words
    subql   #1, d0
    bgts    4$

5$: tstl    d1              | bytes remain to copy?
    beqs    7$              | no, exit

6$: movb    a1@+, a0@+      | copy pending bytes
    subql   #1,d1
    bgts    6$

7$: clrl    d0              | always return 0
    unlk    a6
    rts
