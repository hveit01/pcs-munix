    .text
    .extern _ftrap 
    .extern _main 
    .extern _end 
    .extern _have881 
    .extern _vadrspace 
    .extern _edata 
    .extern _m68341             | motorola FP emulator code
                                | this is not 68881 compatible!
    .extern _con_type 
    .extern __entry
    
__ptbr  =   0x3ffc0000
__esr   =   0x3ffe8002
__ptec  =   0x3ffc8000
__ccr   =   0x3ffe8006
__dmapt =   0x3ffd0000
__pbc   =   0x3ffe8004
__pcr   =   0x3ffe8000
_sbrpte =   0x3ffca000
promcold=   0x3ff80000
minitor =   0x3ff80100
__mfp   =   0x3ffe0000
__lru   =   0x3ffd8000
__puc   =   0x3ffd9000
SYSVA   =   0x3f800000
userarea=   0x3f7ff000          | point to currently mapped user area

    .globl initcode
initcode:
    movl    #SYSVA, d0          | SYSVA is mapped start of system RAM
    movec   d0, vbr             | Vector table points here
    moveq   #1, d0 
    movec   d0, cacr            | enable instruction cache
    movec   d0, sfc             | set SFC and DFC to user space (unused)
    movec   d0, dfc
    movw    #1, __ccr           | set control reg
    movw    #0x19, __ccr        | set control reg start
    movw    SYSVA+0xffffe, _con_type+2 | get console type from last
                                | word of minitor RAM
    lea     _edata, a0          | clear data area
    lea     _end, a1
1$: 
    clrl    a0@+
    cmpl    a1, a0
    bltw    1$

    subl    a6, a6              | clear frame pointer
    movl    #_waitloc, sp       | point to top of temp stack
    movw    #0x2000, sr         | supervisor mode and spl0
    jsr     check68881          | set _have881 if copro present (required)
    jsr     _vadrspace          | initialize virtal address space
    movl    #SYSVA, sp          | point to below SYSVA for stack
    orw     #0x29, __pcr        | set PCR start mode
    jsr     _main               | enter UNIX
    movl    d0, a0              | return init routine
    jsr     a0@                 | call it
    .word   0xabcd              | should never return

    .globl _idle
_idle: 
    link    a6, #0
    stop    #0x2000
sub_80:
    unlk    a6
    rts 

    | find out whether real 68881 exists
check68881: 
    link    a6, #0
    movl    SYSVA+0x2c, d2      | save line-F handler address
    movl    #linef, SYSVA+0x2c  | disable F-handler
    clrl    d0                  | set copro present = 0
                                | try a 68881 instruction
    .long   0xF2005f8f          | fmovecr #0xF, fp7
                                | if copro present, no line-F trap
    moveq   #1, d0              | copro is present
linef:                          | will return here if line-F trap
    movl    d2, SYSVA+0x2c      | restore Line-F handler
    movl    d0, _have881        | and set 881 flag
    unlk    a6
    rts 

    .globl _icode 
_icode: 
    movw    #0, sr 
    movl    #0x1000, sp
    clrl    sp@-
    pea     0x3C:l
    pea     0x2A:l 
    clrl    sp@-
    link    a6, #-0x24
    movl    #0x3B, a6@(-0x20)
    trap    #0xE
    .word   0xabcd

    .globl progname 
progname:
    .asciz  "/etc/init"
    .even
    .long   0
    .long   0
    .long   0x2A
    .long   0

    .globl _szicode 
_szicode:
    .long   0x44

    .globl _storacc 
_storacc: 
arg_0 = 8 
    link    a6, #0 
    movl    #0xB0000000, _sbrpte
    movl    a6@(arg_0), a0
    movl    SYSVA+8, d2
    movl    #loc_120, SYSVA+8 
    clrl    d0
    movw    a0@, d1
    addql   #1, d0
loc_120:
    movl    d2, SYSVA+8
    tstw    d0
    unlk    a6
    rts
    
    .globl _swab 
_swab: 
arg_0 = 4 
arg_4 = 8 
    movl    sp@(arg_0), a0
    movl    sp@(arg_4), d0
    lsrw    #1, d0
1$: 
    movw    a0@, d1
    rolw    #8, d1
    movw    d1, a0@+
    subql   #1, d0 
    bgts    1$
    rts
    
    .globl _setjmp 
_setjmp: 
    movl    sp@+, a1
    movl    sp@, a0
    moveml  #0xfae0, a0@ 
    clrl    d0 
    jmp     a1@

    .globl _longjmp 
_longjmp: 
arg_0 = 4 
    movl    sp@(arg_0), a0
    moveml  a0@, #0xfae0
    moveq   #1, d0
    jmp     a1@

    .globl _resume 
_resume: 
arg_0 = 4 
arg_4 = 8 
    movl    sp@(arg_0), d0
    movl    sp@(arg_4), a0
    movw    #0x2700, sr
    orw     #0xC000, d0 
    movw    d0, 0x3ffc07ee
    movw    0x3ffe8006, d0
    orw     #7, d0
    andw    #0xFFF9, d0
    movw    d0, __ccr
    moveq   #9, d0
    movec   d0, cacr
    moveml  a0@, #0xfae0
    moveq   #1, d0
    movw    #0x2000, sr
    jmp     a1@

    .globl _currpl 
_currpl: 
    clrl    d0
    movw    sr, d0
    andw    #0xFF00, d0
    rts 

    .globl _splx 
_splx: 
arg_2 = 6 
    clrl    d0
    movw    sr, d0
    movw    sp@(arg_2), sr
    rts

    .globl _spl0
_spl0: 
    clrl    d0
    movw    sr, d0
    movw    #0x2000, sr
    rts

.globl _spl1 
    _spl1:
    clrl    d0
    movw    sr, d0
    movw    #0x2100, sr
    rts
    
    .globl  _splhi
_splhi: 
    clrl    d0
    movw    sr, d0
    movw    #0x2700, sr
    rts
    
    .globl _spldisk 
_spldisk: 
    clrl    d0
    movw    sr, d0
    movw    #0x2500, sr
    rts
    
    .globl _clrca
_clrca: 
    movw    __ccr, d0
    orw     #7, d0
    andw    #0xFFFB, d0
    movw    d0, __ccr
    moveq   #9, d0
    movec   d0, cacr
    rts
    
    .globl _clratb 
_clratb: 
    movw    __ccr, d0
    orw     #7, d0
    andw    #0xFFFD, d0
    movw    d0, __ccr
    rts
    
    .globl _clrptbr 
_clrptbr: 
    movw    __ccr, d0
    orw     #7, d0
    andw    #0xFFFE, d0
    movw    d0, __ccr
    rts
    
    .globl _clrcache 
_clrcache: 
    movw    __ccr, d0
    orw     #7, d0
    andw    #0xFFFB, d0
    movw    d0, __ccr
    rts
    
    .globl _min 
_min: 
arg_0 = 4 
arg_4 = 8 
    movl    sp@(arg_0), d0
    cmpl    sp@(arg_4), d0
    bles    1$
    movl    sp@(arg_4), d0
1$: 
    rts
    
.globl _max 
_max: 
arg_0 = 4 
arg_4 = 8 
    movl    sp@(arg_0), d0
    cmpl    sp@(arg_4), d0
    bges    1$
    movl    sp@(arg_4), d0
1$:
    rts
    
    .globl _searchdir 
_searchdir: 
arg_0 = 8 
arg_4 = 0xC 
arg_8 = 0x10
    link    a6, #0
    movl    a6@(arg_0), a0 
    movl    a6@(arg_4), d1
    moveq   #0x10, d2
    clrl    d0
loc_260: 
    cmpl    d2, d1
    blts    loc_290
    tstw    a0@
    beqs    loc_280
    movl    a6@(arg_8), a2
    lea     a0@(2), a1
    moveq   #0xD, d3 
loc_272: 
    cmpmb   a1@+, a2@+
    dbne    d3, loc_272
    beqs    loc_288
loc_27A: 
    addl    d2, a0
    subl    d2, d1
    bras    loc_260
loc_280: 
    tstl    d0
    bnes    loc_27A
    movl    a0, d0
    bras    loc_27A
loc_288: 
    subl    a6@(arg_0), a0
    movl    a0, d0
    bras    loc_29C
loc_290: 
    tstl    d0
    beqs    loc_29A
    subl    a6@(arg_0), d0
    bras    loc_29C
loc_29A: 
    moveq   #-1, d0
loc_29C: 
    unlk    a6
    rts

.globl _setspace 
_setspace: 
arg_0 = 4 
    movl    sp@(arg_0), d0
    movec   d0, sfc
    movec   d0, dfc
    rts
    
    .globl _prom_cold
_prom_cold: 
    movw    #0x2700, sr
    reset
    movw    #0x29, 0x3ffe8000
    movw    #0, __ccr
    moveq   #9, d0
    movec   d0, cacr
    jmp     promcold

    .globl _prom_warm
_prom_warm: 
    movw    #0x2700, sr
    movw    #0x29, 0x3ffe8000
    movw    #1, __ccr
    moveq   #9, d0
    movec   d0, cacr
    movl    #0xB00000F7, __dmapt + 0x03DC 
    movl    #0xB00000F8, __dmapt + 0x03E0
    movl    #0xB00000F9, __dmapt + 0x03E4
    movl    #0xB00000FA, __dmapt + 0x03E8
    movl    #0xB00000FB, __dmapt + 0x03EC 
    movl    #0xB00000FC, __dmapt + 0x03F0 
    movl    #0xB00000FD, __dmapt + 0x03F4 
    movl    #0xB00000FE, __dmapt + 0x03F8 
    movl    #0xB00000FF, __dmapt + 0x03FC
    jmp     minitor


u_fpvec = userarea + 0x2a2

    .globl f_handler                | floating point handler
f_handler: 
uspsave = -4 
    moveml  #0xffff, sp@-           | save all regs
    movl    usp, a0
    movl    a0, sp@(0x40+uspsave)   | save USP to temporary
    movl    sp, a5                  | a5 is current SP, pointing to fpe_vec
    lea     u_fpvec, a3             | a3 points to u.u_fpvec
    movl    #_f_start, a4           | a4 points to _f_start
    movl    #ftraps, a1             | a1 point to trap vectors
    jsr     _m68341                 | call emulator
    movl    sp@(0x40+uspsave), a0   | restore USP
    movl    a0, usp
    moveml  sp@+, #0xffff           | restore regs
    rte                             | return from FP trap
    
ftraps:                             | floating trap table
    .long   ftrap               
    .long   ftrap 
    .long   ftrap 
    .long   ftrap 
    .long   ftrap 
    .long   ftrap
ftrap: 
    movl    a5, sp@-                | point to stack frame
    jsr     _ftrap                  | call floating trap handler
    addql   #4, sp                  | clean stack
    rts                             | exit
    
    | calling convention
    | d7 = #bytes to move, if positive, move from a3 to a2
    |                      if negative, move from a2 to a3
    | a3 = pointer1
    | a2 = pointer2
    .globl _f_start 
_f_start: 
    cmpl    #SYSVA, a3              | within user area?
    bgew    4$                      | yes, skip
    movl    d0, sp@-                | save d0 register
    tstw    d7                      | is d7 < 0?
    blts    2$                      | yes, skip
    subqw   #1, d7                  | adjust byte count
1$: 
    movesb  a3@+, d0                | move from a3 to a2
    movb    d0, a2@+
    dbf     d7, 1$
    movl    sp@+, d0                | restore d0
    rts
2$:
    negw    d7                      | make byte count positive
    subqw   #1, d7                  | adjust count
3$:
    movb    a2@+, d0                | move from a2 to a3
    movesb  d0, a3@+
    dbf     d7, 3$
    movl    sp@+, d0                | restore d0
    rts 
4$: 
    tstw    d7                      | count negative?
    blts    6$                      | yes skip
    subqw   #1, d7                  | adjust count
5$: 
    movb    a3@+, a2@+              | move from a3 to a2
    dbf     d7, 5$
    rts
6$:
    negw    d7                      | make byte count positive
    subqw   #1, d7                  | adjust count
7$:
    movb    a2@+, a3@+              | move from a2 to a3
    dbf     d7, 7$
    rts

    | this is for floating point moves of motorola IEEE-FP
    | This is not compatible to 68881!
    .globl f_mvem 
f_mvem: 
    moveml  #0x8080, sp@-           | save A0, D0
    movl    sp@(0x0a), a0           | get PC
    movw    a0@+, d0                | read word following trap
    movl    a0, sp@(0x0a)           | and advance PC
    movl    usp, a0                 | save USP in A0
    tstw    d0                      | D0 < 0?
    blts    loc_450                 | yes, other direction
    btst    #0, d0                  | Bit 0 set?
    beqs    loc_408 
    .long   0xF0601008              | move FP0
loc_408: 
    btst    #1, d0                  
    beqs    loc_412 
    .long   0xF0601009              | Move FP1
loc_412: 
    btst    #2, d0 
    beqs    loc_41C 
    .long   0xF060100A              | Move FP2
loc_41C: 
    btst    #3, d0 
    beqs    loc_426 
    .long   0xF060100B              | Move FP3
loc_426: 
    btst    #4, d0 
    beqs    loc_430 
    .long   0xF060100C              | Move FP4
loc_430:
    btst    #5, d0 
    beqs    loc_43A 
    .long   0xF060100D              | Move FP5
loc_43A:
    btst    #6, d0 
    beqs    loc_444 
    .long   0xF060100E              | Move FP6
loc_444: 
    btst    #7, d0 
    beqs    loc_4A0
    .long   0xF060100F              | Move FP7
    bras    loc_4A0 
loc_450:                            | other direction
    btst    #7, d0       
    beqs loc_45A 
    .long   0xF058000F              | restore FP7
loc_45A: 
    btst    #6, d0 
    beqs    loc_464 
    .long   0xF058000E              | restore FP6
loc_464: 
    btst    #5, d0 
    beqs    loc_46E 
    .long   0xF058000D              | Restore FP5
loc_46E: 
    btst    #4, d0 
    beqs    loc_478 
    .long   0xF058000C              | Restore FP4
loc_478: 
    btst    #3, d0 
    beqs    loc_482 
    .long   0xF058000B              | Restore FP3
loc_482: 
    btst    #2, d0 
    beqs    loc_48C 
    .long   0xF058000A              | Restore FP2
loc_48C: 
    btst    #1, d0
    beqs    loc_496
    .long   0xF0580009              | Restore FP1
loc_496: 
    btst    #0, d0 
    beqs    loc_4A0 
    .long   0xF0580008              | Restore FP0
loc_4A0: 
    movl    a0, usp                 | restore USP
    moveml  sp@+, #0x0101           | restore D0, A0
    rte                             | exit from trap

    .data
    .globl _f_end 
_f_end: 
    . =     .+1024                  | 1K temporary space space
    
    .globl _waitloc 
_waitloc:
    .long   sub_80 
