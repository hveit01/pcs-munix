|
| calculate IP header checksum
| checksum returned in d0
    .globl _calc_check
_calc_check:
addr = 4
size = 8

    movl    sp@(addr), a0               | start address of header
    movl    sp@(size), d1               | size in bytes
    moveml  #0x1800, sp@-               | save d3, d4

    clrl    d0                          | collects 32 bit checksum
    clrl    d3                          | constant 0
    movw    d1, d4                      | save #bytes
    lsrw    #2, d1                      | convert # of words 
    beqs    2$                          | only single word?, skip

    subqw   #1, d1                      | adjust for DBF
1$: movl    a0@+, d2                    | get next long
    addxl   d2, d0                      | add to checksum
    dbf     d1, 1$                      | loop

    movl    d0, d1                      | swap words
    swap    d1                          | reduction to 16 bit
    addxw   d1, d0                      | add words
    addxw   d3, d0                      | add potential carry
     
2$: btst    #1, d4                      | odd # of word?
    beqs    3$                          | no, skip

    movw    a0@+, d1                    | get final word
    addw    d1, d0                      | add into checksum
    addxw   d3, d0                      | add potential carry

3$: btst    #0, d4                      | odd # of bytes
    beqs    4$                          | no, skip

    movw    a0@, d1                     | get final word
    andw    #0xff00, d1                 | only 1st byte is relevant
    addw    d1, d0                      | add into checksum
    addxw   d3, d0                      | add potential carry

4$: notw    d0                          | build complement and return

    moveml  sp@+, #0x18                 | restore d3, d4
    rts
