    .text
    .global _strncmp
_strncmp:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), a1
    movl    sp@(s2), a2
    movl    sp@(n), d1

1$: subql   #1, d1
    blts    2$
    cmpmb   a1@+, a2@+
    bnes    3$
    tstb    a1@(-1)
    bnes    1$

2$: clrl    d0
    rts

3$:
    movb    a1@-, d0
    subb    a2@-, d0
    extw    d0
    extl    d0
    rts
