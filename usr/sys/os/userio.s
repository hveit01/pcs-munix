
    .text
    .globl  _beginuser
_beginuser: 
    link    a6, #0 
    movl    a6@(8), a1
    movl    a6@(0xc), a0
    movw    a0, d0
    andw    #1, d0 
    movw    a1, d1 
    andw    #1, d1 
    cmpw    d0, d1 
    beqs    1$
    movl    a6@(0x10), d1
    bras    5$
1$: 
    movl    a6@(0x10), d1
    movw    a0, d0
    andw    #3, d0
    beqs    3$
    negw    d0
    addqw   #4, d0
    extl    d0
    cmpl    d1, d0
    bges    5$
    subl    d0, d1
    subql   #1, d0
2$: 
    movesb  a1@+, d2
    movb    d2, a0@+
    dbf     d0, 2$
3$: 
    movl    d1, d0
    andl    #0xFFFFFFFC, d0
    beqs    5$ 
    subl    d0, d1
    asrl    #2, d0 
4$: 
    movesl  a1@+, d2 
    movl    d2, a0@+ 
    subql   #1, d0 
    bgts    4$ 
5$: 
    tstl    d1 
    beqs    7$ 
6$: 
    movesb  a1@+, d2 
    movb    d2, a0@+ 
    subql   #1, d1
    bgts    6$
7$: 
    clrl    d0
    unlk    a6
    rts

    .globl _copyout
_copyout: 
    link    a6, #0
    movl    a6@(8), a1
    movl    a6@(0xc), a0
    movw    a0, d0
    andw    #1, d0
    movw    a1, d1
    andw    #1, d1
    cmpw    d0, d1
    beqs    1$
    movl    a6@(0x10), d1
    bras    5$
1$: 
    movl    a6@(0x10), d1
    movw    a0, d0
    andw    #3, d0
    beqs    3$
    negw    d0
    addqw   #4, d0
    extl    d0
    cmpl    d1, d0
    bges    5$
    subl    d0, d1
    subql   #1, d0
2$: 
    movb    a1@+, d2
    movesb  d2, a0@+
    dbf     d0, 2$
3$: 
    movl    d1, d0
    andl    #0xFFFFFFFC, d0
    beqs    5$
    subl    d0, d1
    asrl    #2, d0
4$: 
    movl    a1@+, d2
    movesl  d2, a0@+
    subql   #1, d0
    bgts    4$
5$: 
    tstl    d1
    beqs    7$
6$: 
    movb    a1@+, d2
    movesb  d2, a0@+
    subql   #1, d1
    bgts    6$
7$: 
    clrl    d0 
    unlk    a6 
    rts 

    .globl _fubyte
_fubyte: 
    link    a6, #0
    movl    a6@(8), a0
    clrl    d0
    movesb  a0@, d0
    unlk    a6
    rts
    
    .globl _fuword
_fuword: 
    link    a6, #0
    movl    a6@(8), a0
    clrl    d0
    movesw  a0@, d0
    unlk    a6
    rts
    
    .globl _fulong
_fulong: 
    link    a6, #0
    movl    a6@(8), a0
    movesl  a0@, d0
    unlk    a6
    rts
    
    .globl _subyte
_subyte: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a6@(0xc), d0
    movesb  d0, a0@
    clrl    d0
    unlk    a6
    rts

    .globl _suword
_suword: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a6@(0xc), d0
    movesw  d0, a0@
    clrl    d0
    unlk    a6
    rts

    .globl _sulong
_sulong: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a6@(0xc), d0
    movesl  d0, a0@
    clrl    d0
    unlk    a6
    rts
    
    .globl _fsbyte
_fsbyte:
    link    a6, #0
    movl    a6@(8), a0
    clrl    d0
    movb    a0@, d0
    unlk    a6
    rts
    
    .globl _fsword
_fsword:
    link    a6, #0
    movl    a6@(8), a0
    clrl    d0
    movw    a0@, d0
    unlk    a6
    rts
    
    .globl _fslong
_fslong:
    link    a6, #0
    movl    a6@(8), a0
    movl    a0@, d0
    unlk    a6
    rts
    
    .globl _ssbyte
_ssbyte: 
    link    a6, #0
    movl    a6@(8), a0
    movb    a6@(0xf), a0@
    clrl    d0
    unlk    a6
    rts
    
    .globl _ssword
_ssword: 
    link    a6, #0
    movl    a6@(8), a0
    movw    a6@(0xe), a0@
    clrl    d0
    unlk    a6
    rts
    
    .globl _sslong
_sslong: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a6@(0xc), a0@
    clrl    d0
    unlk    a6
    rts
    
    .globl _bcopy
_bcopy: 
    link    a6, #0
    movl    a6@(8), a1 
    movl    a6@(0xc), a0
    movw    a0, d0
    andw    #1, d0
    movw    a1, d1
    andw    #1, d1
    cmpw    d0, d1
    beqs    1$
    movl    a6@(0x10), d1
    bras    5$
1$: 
    movl    a6@(0x10), d1
    movw    a0, d0
    andw    #3, d0
    beqs    3$
    negw    d0
    addqw   #4, d0
    extl    d0
    cmpl    d1, d0
    bges    5$
    subl    d0, d1
    subql   #1, d0
2$: 
    movb    a1@+, a0@+
    dbf     d0, 2$
3$: 
    movl    d1, d0
    andl    #0xFFFFFFFC, d0
    beqs    5$
    subl    d0, d1
    asrl    #2, d0
4$: 
    movl    a1@+, a0@+
    subql   #1, d0
    bgts    4$
5$: 
    tstl    d1
    beqs    7$
6$: 
    movb    a1@+, a0@+
    subql   #1, d1
    bgts    6$
7$: 
    clrl    d0
    unlk    a6
    rts
    
    .globl _bzero
_bzero: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a6@(0xc), d1
    movw    a0, d0
    andw    #3, d0
    beqs    2$
    negw    d0
    addqw   #4,d0
    extl    d0
    cmpl    d1, d0
    bges    4$
    subl    d0, d1
    subql   #1, d0
1$: 
    clrb    a0@+
    dbf     d0, 1$
2$: 
    movl    d1, d0
    andl    #0xFFFFFFFC, d0
    beqs    4$
    subl    d0, d1
    asrl    #2, d0
3$: 
    clrl    a0@+
    subql   #1, d0
    bgts    3$
4$: 
    tstl    d1
    beqs    6$
5$: 
    clrb    a0@+
    subql   #1, d1
    bgts    5$
6$: 
    clrl    d0
    unlk    a6
    rts
    
    .globl _spath
_spath: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a0, d0
    beqw    2$
    tstb    a0@
    beqw    2$
    movl    a6@(0xc), a1
    movl    a6@(0x10), d1
    subql   #1, d1
    clrl    d0
1$: 
    movb    a0@+, a1@+
    beqw    3$
    addql   #1, d0
    cmpl    d0, d1
    bnew    1$
    moveq   #0xFFFFFFFE, d0
    braw    3$
2$: 
    moveq   #0xFFFFFFFF, d0
3$: 
    unlk    a6
    rts
    
    .globl _upath
_upath: 
    link    a6, #0
    movl    a6@(8), a0
    movl    a0, d0
    beqw    2$
    tstb    a0@
    beqw    2$
    movl    a6@(0xc), a1
    movl    a6@(0x10), d1
    subql   #1, d1
    clrl    d0
1$: 
    movesb  a0@+, d2
    movb    d2, a1@+
    beqw    3$
    addql   #1, d0
    cmpl    d0, d1
    bnew    1$
    moveq   #0xFFFFFFFE, d0
    braw    3$
2$: 
    moveq   #0xFFFFFFFF, d0
3$: 
    unlk    a6
    rts
    
    .globl _enduser
_enduser: 
    moveq   #0xFFFFFFFF, d0
    unlk    a6
    rts
    
    .even
    .data
    .globl _erret
_erret:
    .long _enduser
