    .text
    .global _strncat
_strncat:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), d0
    movl    d0, a1
    movl    sp@(s2), a2
    movl    sp@(n), d1

1$: tstb    a1@+
    bnes    1$
    subql   #1, a1

2$: movb    a2@+, a1@+
    beqs    3$
    subql   #1, d1
    bges    2$
    clrb    a1@-

3$: rts
