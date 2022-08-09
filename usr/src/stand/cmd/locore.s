|
| Minitor starting at 0x3ff80000
| Locore routines which need to be in assembler
|

|
| some constants

|
| some addresses
__ptbr  =    0x3ffc0000
__esr   =    0x3ffe8002
__ptec  =    0x3ffc8000
__ccr   =    0x3ffe8006
__dmapt =    0x3ffd0000
__pbc   =    0x3ffe8004
__pcr   =    0x3ffe8000
_sbrpte =    0x3ffca000
promcold=    0x3ff80000
minitor =    0x3ff80100
__mfp   =    0x3ffe0000
__lru   =    0x3ffd8000
__puc   =    0x3ffd9000
SYSVA   =    0x3f800000
| reserve 0xf7000 for vectors and PTEs

| following 12K for ICC workspace
physbase=    SYSVA + 0xF7000

| following 4K for PTEs for frame buffers
ptecxxxx=   physbase + 0x0c00

| actual miniram is here
miniram =    0x3f8f8000
miniend =    0x3f900000

|
| ROM vector table
| 
    .text
_promcold:
    braw     cold                              | 0  jump into cold start (was actually SP preset)
    .long    cold                              | 1  boot hardware will supposedly map these to
    .long    buserror                          | 2  phys addr 0x00000000 on hardware reset
    .long    addresserror                      | 3
    .long    illegalopcode                     | 4
    .long    zerodivide                        | 5
    .long    chkerror                          | 6
    .long    trapverror                        | 7
    .long    priverror                         | 8
    
    .globl   _tracevec                         |    changed by minitor single step code
_tracevec:                                     |    therefore .globl
    .long    trace                             | 9

    .long    e_handler                         | 10
    .long    f_handler                         | 11
    .long    ille                              | 0
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    spurious                          | 24
    .long    interrupt1                        | 25
    .long    interrupt2                        | 26
    .long    interrupt3                        | 27
    .long    interrupt4                        | 28
    .long    interrupt5                        | 29
    .long    interrupt6                        | 30
    .long    interrupt7                        | 31
    .long    trapill                           | 44
    
    .globl _trap1vec
_trap1vec:                                     | 33 used for single step, therefore .globl
    .long    trap1

    .long    trapill                           | 44
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    trapill
    .long    ille                              | 0
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille
    .long    ille

|
| Start of code, including a few hard coded constants, such as serial number and
| mac address

    .globl   _minitor
_minitor:
    braw     _warmstart
        
serialno:
    .word    0x9179
    .word    0xFFFF

    .globl   _macaddress
_macaddress:
    .byte    0x08, 0x00, 0x27, 0x00, 0x91, 0x79

unknown:
    .word    0x71FF
    .word    0xFFFF, 0xFFFF, 0x0ECC, 0xF42A    | no idea what these do - maybe checksums
    .word    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF    | or debris

| documented in Cadmus MWS/3 Users Guide, Chp 6.10 as well known addresses
    .extern _putchar, _getchar, _haschar, _con_type, _echo
    .long    _putchar
    .long    _getchar
    .long    _haschar
    .long    _con_type
    .long    _echo

    .globl   _physbase
_physbase:    
    .long    physbase                          | points to the start of area shared with ICC

|
| some externals
    .extern  _coldinit, _main, _trap
    .extern  _pte_cxxxxx


|-----------------------------------------------------------------------------
| warmstart handler
|
    .globl  _warmstart
_warmstart:
    movw    #1,__ccr                           | PTBR=1, ATB=0, CACHE=0
    moveq   #9,d0                              | clear and enable 68020 cache
    movec   d0, cacr

    movw    #0x21, __pcr                       | PCR: RUN, DMA
    movl    #0xFFFFFFFF, __ptbr                | set first PTBR = { 0xffff, 0xffff }
    braw    warm_init

cold:
    movw    #0x2500, sr                        | set int level 5, supervisor
    movl    #promcold, d0
    movec   d0, vbr                            | set vector table in ROM

    movw    #1, __ccr                          | PTBR=1, ATB=0, CACHE=0
    moveq   #9, d0                             | clear and enable cache
    movec   d0,cacr

    movw    __esr, d0                          | read and clear ESR
    movl    #__ptbr+0x7EE, sp                  | temporarily set SP=0x3FFC07EE (we know it exists)
    clrl    __ptbr                             | set PTBR = 0 (coldstart!)

    movl    #0xA0000000, _sbrpte+0x1FF8        | protect 8k zero page, allow kernel write
                                               | via SPT entries 0x7FE000 and 0x7FF000
                                               | set PFN=0, PTE_KW into SPT[0x7FE]
                                               | Phys 0x000000-0x000FFF
    movl    #0xA0000001, _sbrpte+0x1FFC        | set PFN=1, PTE_KW into SPT[0x7FF]
                                               | Phys 0x001000-0x001FFF

    movw    #256, d0                           | 256 entries
    movl    #0xB0000000, d1                    | pte = PG_V|PTE_KW|PTE_KR + pfn
    lea     __dmapt, a0                        | DMA paging table
    jsr     fillpt                             | __dmapt[0x000-0x0FF] = Phys 0x000000-0x0FFFFF

    movw    #64, d0                            | 64 entries
    movl    #0xB0000300, d1                    | PTE = PG_V|PTE_KW|PTE_KR + 0x300 + pfn
    movl    #_sbrpte+0x1800, a0                | SPT[0x600]
    jsr     fillpt                             | fill SPT[0x600..0x63F] = Phys 0x300000-33FFFF

    movw    #2,__pcr                           | set PCR = BUSINIT to QBUS
    movw    #1000, d0                          | wait some time
1$: dbf     d0, 1$
    clrw    __pcr                              | clear PCR_RUN, BUSINIT

    movw    #256, d0                           | 256 entries
    movl    #0xB0000000, d1                    | PTE = PG_V|PTE_KW|PTE_KR + pfn
    lea     _sbrpte, a0                        | fill SPT[0x000-0x0FF] = Phys 0x000000-0x0fffff
    jsr     fillpt                             | fill SPT 1MB
    movw    #0x21, __pcr                       | PCR_RUN, PCR_PCONF
 
    bsrw    _memsize                           | calculate memory size (4,8,12,16MB)
    bsrw    mfpinit                            | initialize 68901 MFP

|
| warm start effectively starts here
warm_init:
    movl    #__ptbr+0x7EE, sp                  | reset SP = 0x3FFC07EE
    movl    #_promcold, d0                     | set vector table to ROM
    movec   d0, vbr

    movw    #8, d0                             | 8 entries
    movl    #0xB00000F8, d1                    | PTE = PG_V|PTE_KW|PTE_KR + 0xf8 + pfn
    lea     _sbrpte+0x3E0, a0                  | SPT[0xf8]
    jsr     fillpt                             | fill SPT[0x0F8..0x0FF] = Phys 0x0F8000-0xFFFFF

    movl    #0xB00000F7,_sbrpte+0x3DC          | fill SPT[0xF7] = Phys 0x0F7000-0xF7FFF

    movw    #256, d0                           | 256 entries
    movl    #0xF0000C00, d1                    | PTE = 0xc00, PG_V|PTE_UW + 0xc00 + pfn
    movl    #_pte_cxxxxx, a0                   | table    ?
    jsr     fillpt                             | fill table [0-0xff] =    Phys 0xc00000-0xCFFFFF

    movw    #255, d0                           | 255 entries
    lea     _sbrpte, a0
2$: orl     #0x20000000,a0@+                   | enable PTE_KW in SPT[0x000-0x0FE]
    dbf     d0, 2$

    movw    #8, d0                             | 8 pages
    movl    #0xB00000F8, d1                    | PTE = PG_V|PTE_KW|PTE_KR + 0xf8 + pfn
    lea     __dmapt+4, a0                      | fill DPT[1-9] = 0xf8-0xff = Phys 0xf8000-0xfffff
    jsr     fillpt
    movl    #0xB00000F7, __dmapt+0x3DC         | DPT[0xf7] = 0xf7 = Phys addr = 0xf7000

    movl    #miniend, sp                       | SP = 0x3f900000 (there is 0x322a bytes for stack)

    movl    #miniram, a0                       | clear 0x3f8f8000 - 0x3f900000
    movl    #miniend, a1
3$: clrl    a0@+
    cmpl    a1, a0
    bltw    3$                                 | loop

    movl    __ptbr, _coldinit                  | save PTBR into coldinit
                                               | used as coldstart flag, if ptbr is 0,
                                               | it is cold
    movw    #0xC3DF, __ptbr+0x600              | PTBR[0x300] = MP_VALID|0x3df
    movw    #0x2000, sr                        | set interrupt level 0
    
    subl    a6, a6                             | a6 = 0
    movw    #0xFFFF, __ccr                     | set all cached, no clear bits
    jsr     _main                              | enter minitor interactive loop
    braw    _warmstart                         | back into warmstart

|-----------------------------------------------------------------------------
| fill page table 
| d0 = #entries, a0=start of table, d1=PTE, pfn will be incremented
|
fillpt:
    subqw    #1, d0                            | decrement counter
1$: movl     d1, a0@+                          | save PTE into    table
    addql    #1, d1                            | increment PFN
    dbf      d0,1$                             | loop
    rts


|----------------------------------------------------------------------------
| initialize 68901 MFP 
|
mfpinit:
    lea        __mfp, a0
    movb    #0x04, a0@(0x2d)                   | TSR:   set end of transmission
    movb    #0x70, a0@(0x01)                   | GDPR:  set bits 7-5
    movb    #0x70, a0@(0x05)                   | DDR:   set bits 7-5 as outputs
    movb    #0x01, a0@(0x19)                   | TACR:  timer A delay mode /4 prescaler
    movb    #0x07, a0@(0x1f)                   | TADR:  timer A counter = 7
    movb    #0x01, a0@(0x1d)                   | TCDCR: timer D delay mode /4 prescaler
    movb    #0x03, a0@(0x25)                   | TDDR:  timer D counter counter = 3
    movb    #0x88, a0@(0x29)                   | UCR:   usart CLK /16, Asynch mode 1Stop
    movb    #0x00, a0@(0x07)                   | IERA:  disable interrupts
    movb    #0x00, a0@(0x0b)                   | IPRA:  no ints pending
    movb    #0x01, a0@(0x2b)                   | RSR:   receiver enable
    movb    #0x05, a0@(0x2d)                   | TSR:   transmitter enable, SO=HIGH
    movb    #0x00, a0@(0x2f)                   | UDR:   clear USART data register
    rts

|-----------------------------------------------------------------------------
| calculate memory size and set in PBC
| PBC = xx?xxxxx
|       || ||||+---- has mem at 0x000000-0x3fffff = 4MB
|       || |||+----- has mem at 0x400000-0x7fffff = 8MB
|       || ||+------ has mem at 0x800000-0xbfffff = 12MB
|       || |+------- has mem at 0xc00000-0xffffff = 16MB
|       ++-+-------- 11?0=16MB 10?0=12MB 00?1=8MB 00?0=4MB
|
    .globl _memsize
_memsize:
    movl    d7, sp@-                           | save D7
    movw    __ccr, sp@-                        | save CCR
    movw    #0xFFF3, __ccr                     | clear CA and PTBR
    clrw    d7                                 | collected pbc bits
    clrl    d0                                 | shift count
    clrl    d2                                 | memory bits 0x08=16MB, 0x04=12, 0x02=8MB 0x01=4MB
    movl    #0xB0000000, d3                    | PTE = 0, PG_V|PTE_KW|PTE_KR
    movw    #0xCF, __pbc                       | set cache control, disable cache, assume 16MB

1$: movw    #0x61, __pcr                       | PCR_RUN, DMA Grant, memory through QBUS
    movl    d3, _sbrpte                        | SPT[0] = map physaddrs 0x000000, 0x400000, 0x800000, 0xc00000
    movl    SYSVA, d1                          | get QBUS memory bits
    movw    #0x21, __pcr                       | PCR_RUN, DMA Grant
    andw    #0x0F, d1                          | get low 4 bits
    cmpw    #0x0F, d1                          | are they all set?
    beqw    2$                                 | yes, no memory there
    movw    #1, d4                             | d4 = 1 << shiftcount
    aslw    d0, d4
    orw     d4, d7                             | d7 |= 1 << d0
    cmpw    d2, d1                             | take minimum of match bits
    blew    2$
    movw    d1, d2                             | save it

2$: addqw   #1, d0                             | for all channels
    cmpw    #4, d0
    bgew    3$                                 | break loop
    addl    #0x400, d3                         | add 0x400 to PFN = next 4MB
    braw    1$                                 | loop

3$: movl    #0xB0000000, _sbrpte               | reset SPT[0]
    cmpw    #8, d2                             | 16MB?
    bnew    4$                                 | no
    orw     #0xC0, d7                          | pbc = 11001111

4$: cmpw    #4,d2                              | 12MB?
    bnew    5$                                 | no
    orw     #0x80, d7                          | pbc = 10000111

5$: cmpw    #2, d2                             | 8MB?
    bnew    6$                                 | no
    orw     #0x10, d7                          | pbc = 00010011
                                               | not matched: pbc = 00000001

6$: movw    d7, __pbc                          | save into PBC
    movw    sp@+, __ccr                        | restore CCR
    movl    sp@+, d7                           | restore D7
    rts


|-----------------------------------------------------------------------------
| clear CPU cache
|
    .globl   _clrcache020
_clrcache020:
    moveq    #9, d0                            | set C=clear cache and E=enable cache
    movec    d0,cacr
    rts

|-----------------------------------------------------------------------------
| exception handling trampoline
| see also /usr/sys/l.s

except:
    moveml   #0xfffe, sp@-                    | save all regs d0-d7, a0-a6
    movec    usp, a0                          | save USP (the supervisor SP now rules)
    movl     a0, sp@-
    jsr      _trap                            | call trap handler (C code)
    movl     sp@+, a0                         | restore USP
    movec    a0, usp
    moveml   sp@+, #0x7fff                    | restore registers
    rts                                       | return to exnorm

| common dispatcher for all exception/interrupt vectors
exnorm:
    bsrw    except                            | call exception handler
    addql   #4, sp                            | clean vector number off stack
    movl    #0xFFC0FC00, __ptec+0x1000        | pte cache
    rte                                       | return from exception

buserror:
    movl    #2, sp@-                          | exception number
    bras    exnorm                            | goto common exception handler code

addresserror:
    movl    #3, sp@-
    bras    exnorm

illegalopcode:
    movl    #4, sp@-
    bras    exnorm

zerodivide:
    movl    #5, sp@-
    bras    exnorm

chkerror:
    movl    #6, sp@-
    bras    exnorm

trapverror:
    movl    #7, sp@-
    bras    exnorm

priverror:
    movl    #8, sp@-
    bras    exnorm

trace:
    movl    #9, sp@-
    bras    exnorm

trap1:                                         | used in single step code
    movl    #33, sp@-
    bras    exnorm

trapill:
    movl    #44, sp@-
    bras    exnorm

trapold:
    movl    #45, sp@-
    bras    exnorm

trapnew:
    movl    #46, sp@-
    bras    exnorm

spurious:
    movl    #24, sp@-
    bras    exnorm

interrupt1:
    movl    #25, sp@-
    braw    exnorm

interrupt2:
    movl    #26, sp@-
    braw    exnorm

interrupt3:
    movl    #27, sp@-
    braw    exnorm

interrupt4:
    movl   #28, sp@-
    braw    exnorm

interrupt5:
    movl    #29, sp@-
    braw    exnorm

interrupt6:
    movl    #30, sp@-
    braw    exnorm

interrupt7:
    movl    #31, sp@-
    braw    exnorm

e_handler:
    movl    #10, sp@-
    braw    exnorm

f_handler:
    movl    #11, sp@-
    braw    exnorm

ille:
    movl    #0, sp@-
    braw    exnorm

|-----------------------------------------------------------------------------
| compiler helpera for switch statements
| see source file /usr/sys/os/binsw.s

    .globl binswitch
binswitch:
    movl    a3, sp@-
    movl    a0, a1
    aslw    #2, d1
    lea     a0@(,d1:w), a2
    movl    a2, a0

1$: movl    a2, d2
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

2$: movl    a3, a2
    bras    1$

3$: cmpw    a1@, d0
    bnes    4$
    lea     a1@(2), a0

4$: movw    a0@, d0
    movl    sp@+, a3
    jmp     a0@(,d0:w)

    .globl binlswitch
binlswitch:
    movl    a3, sp@-
    movl    a0, a1
    mulu    #6, d1
    lea     a0@(,d1:l), a2
    movl    a2, a0

1$: movl    a2, d2
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

2$: movl    a3, a2
    bras    1$

3$: cmpl    a1@, d0
    bnes    4$
    lea     a1@(4), a0

4$: movw    a0@, d0
    movl    sp@+, a3
    jmp     a0@(,d0:w)

|-----------------------------------------------------------------------------
| used by C code trap handler
|
invalid_trap:
    moveq   #-1, d0
    unlk    a6
    rts
    
    .data
    .globl _invalid_trap
_invalid_trap:
    .long   invalid_trap
    
    .text
|-----------------------------------------------------------------------------
| get supervisor status register (unused)
|
    .globl _getsr
_getsr:
    clrl    d0
    movw    sr, d0
    rts

|-----------------------------------------------------------------------------
| set supervisor status register
|
    .globl _setsr
_setsr:
arg =  6
    clrl    d0
    movw    sr, d0
    movw    sp@(arg), sr
    rts

|-----------------------------------------------------------------------------
| get vector base register
|
    .globl _getvbr
_getvbr:
    movec    vbr, d0
    rts

|-----------------------------------------------------------------------------
| set vector base register
|
    .globl _setvbr
_setvbr:
arg =  4
    movl    sp@(arg), d0
    movec   d0, vbr
    rts

|-----------------------------------------------------------------------------
| retexcept: return from exception handler (called from C code)
|
    .globl _retexcept
_retexcept:
var6  = -6
var2  = -2
exvec = 8
    link    a6, #0
    movl    a6@(exvec), a0                     | passed an exvec*
    movl    a0@, sp                            | restore SP exvec->exa7
    subql   #8, sp                             | reserve 2 long
    movw    a0@(0x48), sp@                     | put back processor status
    movl    a0@(0x4a), sp@(var6+8)             | put back PC
    clrw    sp@(var2+8)                        | align to long
    moveml  a0@(4), #0x7fff                    | restore all registers a0-a6, d0-d7
    movl    #0xFFC0FC00, __ptec+0x1000         | set page table cache
    rte

|-----------------------------------------------------------------------------
| setjmp(jmp_buf), longjmp(jmpbuf, ret)
| see usr/src/lib/libc/m68000/gen/setjmp.s
|
    .global _setjmp
_setjmp:
retp = 0
linkp = 4
env = 8

    link    a6, #0
    movl    a6@(env), a0                       | jmp_buf
    movl    a6@(linkp), a0@                    | jmp_buf[0] = old linkp
    movl    a6, a0@(4)                         | jmp_buf[1] = current linkp
    movl    a6@, a0@(8)                        | jmp_buf[2] = ret pc
    moveml  #0x38e0, a0@(12)                   | jmp_buf[3-8] = regs d7-5,a5-3
    clrl    d0
    unlk    a6
    rts

    .global _longjmp
_longjmp:
linkp = 4
env = 8
retval = 12

    link    a6,#0
    movl    a6@(env), a0                       | jmp_buf
    movl    a0@(4), a1                         | tmplinkp = jmp_buf[1]
    movl    a0@, a1@(linkp)                    | old linkp = jmp_buf[0]
    movl    a0@(8), a1@                        | ret pc = jmp_buf[2]
    moveml  a0@(12), #0x38e0                   | regs d7-5, a5-3 = jmp_buf[3-8]

    movl    a6@(retval), d0                    | return value
    bnes    1$
    moveq   #1, d0                             | ensure it is != 0

1$: movl    a1, a6                             | restore current linkp
    unlk    a6                                 | exit
    rts
    
|-----------------------------------------------------------------------------
| _getspecrecs(specregs*) - store the non-standard CPU registers in structure
| called from minitor C code
    .globl _getspecregs
_getspecregs:
arg =  8

    link    a6,#0
    movl    a6@(arg), a0                       | get target address
    movec   usp, d0
    movl    d0, a0@+                           | store USP
    movec   vbr, d0
    movl    d0, a0@+                           | store VBR
    movec   sfc, d0              
    movl    d0, a0@+                           | store SFC
    movec   dfc, d0
    movl    d0, a0@+                           | store DFC
    movec   cacr, d0
    movl    d0, a0@+                           | store CACR
    movec   caar, d0
    movl    d0, a0@+                           | store CAAR
    unlk    a6
    rts

|-----------------------------------------------------------------------------
| _setspecrecs(specregs*) - restore the non-standard CPU registers from structure
| called from minitor C code
    .globl _setspecregs
_setspecregs:
arg =  8

    link    a6,#0
    movl    a6@(arg), a0                       | get source address
    movl    a0@+, d0
    movec   d0, usp                            | restore USP
    movl    a0@+, d0
    movec   d0, vbr                            | restore VBR
    movl    a0@+, d0
    movec   d0, sfc                            | restore SFC
    movl    a0@+, d0
    movec   d0, dfc                            | restore DFC
    movl    a0@+, d0
    movec   d0, cacr                           | restore CAAR
    movl    a0@+, d0
    movec   d0, caar                           | restore CADR
    unlk    a6
    rts

    .even
