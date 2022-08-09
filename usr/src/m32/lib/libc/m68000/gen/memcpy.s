    .text
    .global _lmemcpy
    .global _memcpy
_lmemcpy:
_memcpy:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), a0
    movl    sp@(s2), a1
    movl    sp@(n), d1
    movl    a0, d0

1$: tstl    d1
    bles    3$
    cmpl    #32000, d1
    bgts    4$
    subw    #1, d1

2$: movb    a1@+, a0@+
    dbf     d1, 2$

3$: rts

4$: subl    #32000, d1
    movw    #31999, d2

5$: movb    a1@+, a0@+
    dbf     d2, 5$
    bras    1$

    .even
