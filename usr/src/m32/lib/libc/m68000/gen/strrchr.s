    .text
    .global _strrchr
_strrchr:
s1 = 4
c = 8

    movl    sp@(s1), a0
    movl    sp@(c), d1
    clrl    d0

1$: cmpb    a0@, d1
    bnes    2$
    movl    a0, d0

2$: tstb    a0@+
    bnes    1$
    rts
