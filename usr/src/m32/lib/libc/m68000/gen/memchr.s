    .text
    .global _lmemchr
    .global _memchr
_lmemchr:
_memchr:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), a0
    movl    sp@(s2), d1
    movl    sp@(n), d2

1$: subql   #1, d2
    blts    2$
    cmpb    a0@+, d1
    bnes    1$
    movl    a0, d0
    subql   #1, d0
    rts

2$: clrl    d0
    rts

    .even
