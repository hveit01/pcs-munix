    .text
    .global _strcmp
_strcmp:
s1 = 4
s2 = 8

    movl    sp@(s1), a1
    movl    sp@(s2), a2

1$: cmpmb   a1@+, a2@+
    bnes    2$
    tstb    a1@(-1)
    bnes    1$
    clrl    d0
    rts

2$: movb    a1@-, d0
    subb    a2@-, d0
    extw    d0
    extl    d0
    rts
