    .text
    .global _lmemccpy
    .global _memccpy
_lmemccpy:
_memccpy:
s1 = 4
s2 = 8
c = 12
n = 16

    movl    sp@(s1), a0
    movl    sp@(s2), a1
    movl    sp@(c), d1
    movl    sp@(n), d2

1$: subql   #1, d2
    blts    2$
    movb    a1@+, a0@
    cmpb    a0@+, d1
    bnes    1$
    movl    a0, d0
    rts

2$: clrl    d0
    rts
        
    .even
