    .text
    .global _strncpy
_strncpy:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), d0
    movl    d0, a1
    movl    sp@(s2), a2
    movl    sp@(n), d1

1$:
    subql   #1, d1
    blts    3$
    movb    a2@+, a1@+
    bnes    1$

2$: subql   #1,d1
    blts    3$
    clrb    a1@+
    bras    2$

3$: rts
