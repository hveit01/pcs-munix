    .text
    .global _strcat
_strcat:
s1 = 4
s2 = 8
    movl    sp@(s1), d0
    movl    d0, a1
    movl    sp@(s2), a2

1$: tstb    a1@+
    bnes    1$
    subql   #1, a1
    
2$: movb    a2@+, a1@+
    bnes    2$
    rts
