|
| @(#)dbxxx.s
|
    .data
    .comm   __dbargs, 512

    .text
    .globl  __dbsubc
__dbsubc:
    movl    __dbargs, a0    | get address of routine
    movl    __dbargs+4, d0  | get arg count
    movl    d0, d1          | into D1
    asll    #2, d0          | make long index
    lea     __dbargs+8, a1  | get arg table
    addl    d0, a1          | index into table

    subql   #1, d1          | adjust counter
1$: movl    a1@-, sp@-      | push argument
    dbf     d1, 1$          | D1 times
    movl    sp, a6          | build frame pointer
    jsr     a0@             | call routine
    addw    #0, sp          | caller pulls args

    .globl __dbsubn
__dbsubn:
    trap    #0x0f           | halt
    .word   0

