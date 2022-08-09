| this is the assembler part of the memtest helpers

| format strings and texts in DATA
    .data  
aD:      .asciz  "%d"
a8x:     .asciz  "%8x"
a4x:     .asciz  "%4x"
a2x:     .asciz  "%2x"
a1x:     .asciz  "%1x"
    
| externals
    .extern _haschar, _getchar, _putchar, _printf

    .text
| clear processor cache and enable it
| this is unused, but in PROM
    .globl _clrca_en
_clrca_en:
    moveq   #0x09, d0                  | clear processor cache, enable cache
    movec   d0, cacr
    rts

| clear processor cache and disable it
    .globl _clrca_dis
_clrca_dis:
    moveq   #0x08, d0
    movec   d0,cacr
    rts

| convert ASCII hex char into binary value (0-15)
    .globl  _hex2bin
_hex2bin:
    subb    #0x30, d0                   | subtract '0'
    cmpb    #16, d0                     | less then 16
    blsw    1$                          | no
    subql   #7, d0                      | adjust
1$: rts

| the following stuff are _sa (saveall) wrappers to the original monitor routines
| they effectively save all registers, except a7, and possibly d0, to stack

| save all regs; if char available, read/echo, if not, return 0

    .globl  sa_inkey
sa_inkey:
    moveml  #0x7f00, sp@-               | save d1-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    jsr     _haschar
    tstl    d0
    beqw    1$                          | a char pending?
    jsr     _getchar                    | return char in d0

1$: moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00fe               | restore d1-d7
    rts


| emit CRLF 
    .globl  sa_crlf
sa_crlf:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    #0x0d, sp@-                 | send CR
    jsr     _putchar
    addql   #4, sp
    movl    #0x0a, sp@-                 | send LF
    jsr     _putchar
    addql   #4, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


| do printf
    .globl  sa_printf
sa_printf:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    a6, sp@-                    | put up argument address
    jsr     _printf
    addql   #4, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


    .text
| print decimal in d1
    .globl sa_print_dec
sa_print_dec:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    d1, sp@-
    movl    #aD, sp@-                   | printf("%d", d1)
    jsr     _printf
    addql   #8, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


| print 8 hex digits in d1
    .globl  sa_print_hex8
sa_print_hex8:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    d1, sp@-
    movl    #a8x, sp@-                  | printf("%8x", d1)
    jsr     _printf
    addql   #8, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


| print 4 hex digits in d1
    .globl  sa_print_hex4
sa_print_hex4:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    d1, sp@-
    movl    #a4x,  sp@-                 | printf("%4x", d1)
    jsr     _printf
    addql   #8, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


| print 2 hex digits in d1
    .globl  sa_print_hex2
sa_print_hex2:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    d1, sp@-
    movl    #a2x,  sp@-                 | printf("%2x", d1)
    jsr     _printf
    addql   #8, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts


| print 1 hex digit in d1
    .globl  sa_print_hex1
sa_print_hex1:
    moveml  #0xff00, sp@-               | save d0-d7
    moveml  #0x00fe, sp@-               | save a0-a6
    movl    d1, sp@-
    movl    #a1x,  sp@-                 | printf("%1x", d1)
    jsr     _printf
    addql   #8, sp
    moveml  sp@+, #0x7f00               | restore a0-a6
    moveml  sp@+, #0x00ff               | restore d0-d7
    rts
