    .text
    .global _strcpy
_strcpy:
s1 = 4
s2 = 8

    movl    sp@(s1), d0
    movl    d0, a1
    movl    sp@(s2), a2

1$: movb    a2@+, a1@+
    bnes    1$
    rts
