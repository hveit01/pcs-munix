|
| asm helper to fix exponent byte
| this is unused in compiler code

    .text
    .globl __PCS_asmfixexp
__PCS_asmfixexp:
    nop
    addl    d7, d7      | * 2
    beqs    1$          | exist if number is zero
    eorb    #0x80, d7   | negate sign of exp
    asrb    #1, d7      | divide by 2
    subi    #0x82, d7   | make bias for exponent
    swap    d7          | put exponent into upper word
    roll    #7, d7      | put into highest byte
1$: rts
    
|
| asm helper to convert a DBL in d7 into FLT
|
    .globl __PCS_asmflt
__PCS_asmflt:
    swap    d7          | put exp into lower word
    rorl    #7, d7      | put exponent into lower byte
    movl    #0xffffff80, d7 | mask
    eorb    d5, d7      | negate sign of exp
    addb    d7, d7      | * 2
    bvss    2$          | overflow?
    addb    #5, d7      | would overflow when shifted 32 bit
    bvss    8$          | yes
    eorb    d5, d7      | restore sign
    rorl    #1, d7      | divide 2
1$: rts                 | done

2$: bccs    7$          | no carry, skip
    cmpb    #0x7c, d7   | exponent was 124?
    beqs    3$          | yes
    cmpb    #0x7e, d7   | exponent was 126
    bnes    4$          | no, skip

3$: addb    #-0x7b      | was 124 or 126, adjust
    rorl    #1, d7      | divide 2
    tstb    d7          | check exp
    bnes    1$          | okay, done
    bras    5$          | result is 0

4$: andw    #0xfeff,d7  | was a carry into bit 8, clear it
    tstl    d7          | total result now 0?
    beqs    1$          | yes, done
    tstb    d7          | only exp now 0?
    beqs    6$          | underflow, result is 0

5$: movl    #0, d7      | force result 0
    bras    1$          | done
    
6$  movl    #0, d7      | underflow 0
    bras    1$
    
7$: cmpb    #0xfe, d7   | exp is -2
    bnes    8$          | no
    lsrl    #8, d7      | shift 9 bit right
    lsrl    #1, d7
    bnes    9$          | not yet 0
    movl    #0xffffffff, d7 | make INF
    roxrb   #1, d7
    orw     #2, ccr     | set overflow bit
    bras    1$          | done

8$: lslw    #8, d7      | shift 8 bit back (sign)
    movl    #0xffffffff, d7 | make INF/-INF
    roxrb   #1, d7
    orw     #2, ccr     | set overflow bit
    bras    1$          | done
    
9s: movl    #0, d7      | result is 0
    bras    1$

