    .text
    .global _bcopy
_bcopy:
s1 = 4
s2 = 8
n = 12

    movl    sp@(s1), a0
    movl    sp@(s2), a1
    movl    sp@(n), d1

1$: movb    a0@+, a1@+
    subql   #1, d1
    bnes    1$
    rts
