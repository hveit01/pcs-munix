    .text
    .global _rindex
_rindex:
s = 4
c = 8

    movl    sp@(s), a0
    movl    sp@(c), d1
    clrl    d0

1$: cmpb    a0@, d1
    bnes    2$
    movl    a0, d0

2$: tstb    a0@+
    bnes    1$
    rts
