    .text
    .global _lmemset
    .global _memset
_lmemset:
_memset:
s = 4
c = 8
n = 12

    movl    sp@(s), a0
    movl    a0, d0
    movl    sp@(c), d1
    movl    sp@(n), d2

1$: tstl    d2
    bles    3$
    cmpl    #32000, d2
    bgts    4$
    subqw   #1, d2

2$: movb    d1, a0@+
    dbf     d2, 2$

3$: rts

4$: subl    #32000, d2
    movw    #31999, d3

5$: movb    d1, a0@+
    dbf     d3, 5$
    bras    1$

    .even
