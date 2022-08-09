
    .text
    .global binswitch
binswitch:
    movl    a3, sp@-
    movl    a0, a1
    aslw    #2, d1
    lea     a0@(,d1:w), a2
    movl    a2, a0
1$:
    movl    a2, d2
    subql   #4, d2
    cmpl    a1, d2
    beqs    3$
    movl    a2, d2
    subl    a1, d2
    asrl    #3, d2
    asll    #2, d2
    lea     a1@(,d2:l), a3
    cmpw    a3@, d0
    blts    2$
    movl    a3, a1
    bras    1$
2$:
    movl    a3, a2
    bras    1$
3$:     
    cmpw    a1@, d0
    bnes    4$
    lea     a1@(2), a0

4$:
    movw    a0@, d0
    movl    sp@+, a3
    jmp     a0@(,d0:w)



    .global binlswitch
binlswitch:
    movl    a3, sp@-
    movl    a0, a1
    mulu    #6, d1
    lea     a0@(,d1:l), a2
    movl    a2, a0
1$:
    movl    a2, d2
    subql   #6, d2
    cmpl    a1, d2
    beqs    3$
    movl    a2, d2
    subl    a1, d2
    divu    #12, d2
    mulu    #6, d2
    lea     a1@(,d2:l), a3
    cmpl    a3@, d0
    blts    2$
    movl    a3, a1
    bras    1$

2$:
    movl    a3, a2
    bras    1$

3$:
    cmpl    a1@, d0
    bnes    4$
    lea     a1@(4), a0

4$:
    movw    a0@, d0
    movl    sp@+, a3
    jmp     a0@(,d0:w)
