    .text
    .global _lmemcmp
    .global _memcmp
_lmemcmp:
_memcmp:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), a0
    movl    sp@(s2), a1
    movl    sp@(n), d1
    cmpl    a0, a1
    beqs    2$

1$: subql   #1, d1
    blts    2$
    cmpmb   a0@+, a1@+
    beqs    1$
    movb    a0@-, d0
    subb    a1@-, d0
    extw    d0
    extl    d0
    rts

2$: clrl    d0
    rts
        
    .even
