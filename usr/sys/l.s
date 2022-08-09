|  @(#) RELEASE:  1.6  Nov 17 1986 /usr/sys/l.s ";
|
|  1.6 SDLC interrupt vectors (IBM-3274)
|  1.5 3 vectors for first COL, 144 sccint, 148 buttonint, 14C vsint
|      reserve 3 vectors for 2nd COL, 154 sccint, 158 buttonint, 162 vsint
|  1.4 icc0 to icc3, moved mfp and COL
|  1.3 RJE interrupt vectors (DQS)
|  1.2 bmt anstelle von bip
|  1.1 X25 interrupt vectors
|
#include "conf.h"
	.globl  __entry
	.globl  _eaintr,_trap,initcode,_dump
	.globl  _sbrpte,__pcr
	.globl  minitor
	.extern __ccr
	.extern f_handler,f_mvem        | for motorola ieee fp software

#ifdef DQS
	.globl _start_of_text
#endif

	. =     0

#ifdef DQS
_start_of_text:
#endif

	bra     __entry                 |       0
	bra     dump                    |       1
	.long   buserror                |       2
	.long   addresserror            |       3
	.long   illegalopcode           |       4
	.long   zerodivide              |       5
	.long   chkerror                |       6
	.long   trapverror              |       7
	.long   priverror               |       8
	.long   trace                   |       9
	.long   e_handler               |       10
	.long   f_handler               |       11
	.long   ille                    |       12
	.long   coprocviol              |       13
	.long   ille                    |       14
	.long   ille                    |       15
	.long   ille                    |       16
	.long   ille                    |       17
	.long   ille                    |       18
	.long   ille                    |       19
	.long   ille                    |       20
	.long   ille                    |       21
	.long   ille                    |       22
	.long   ille                    |       23
	.long   spurious                |       24
	.long   interrupt1              |       25
	.long   interrupt2              |       26
	.long   interrupt3              |       27
	.long   interrupt4              |       28
	.long   interrupt5              |       29
	.long   interrupt6              |       30
	.long   interrupt7              |       31
	.long   f_mvem                  |trap 0: save/restore fp regs   32
	.long   clrcaches               |trap 1: clear cpu and ext. cache 33
	.long   trapill                 |       34
	.long   trapill                 |       35
	.long   trapill                 |       36
	.long   trapill                 |       37
	.long   trapill                 |       38
	.long   trapill                 |       39
	.long   trapill                 |       40
	.long   trapill                 |       41
	.long   trapill                 |       42
	.long   trapill                 |       43
	.long   trapill                 |       44
	.long   trapold                 |       45
	.long   trapnew                 |       46
	.long   trapmon                 |       47
	. = 0xc0                        |       48
	.long   fpex            | Branch or set on unordered condition  48
	.long   fpex            | Inexact Result                        49
	.long   fpex            | FP divide by zero                     50
	.long   fpex            | Underflow                             51
	.long   fpex            | Operand Error                         52
	.long   fpex            | Overflow                              53
	.long   fpex            | Signaling NAN                         54
|
usertrap = .
#if defined(IW) || defined(IS) || defined (IF) || defined(ICC_SCC) || defined(ICC_TCPIP)
	. = 0x104               | 0101
	.long	icc0_rint
	.long   icc0_xint
	.long	icc0_init
	. = 0x114               | 0105
	.long	icc1_rint
	.long   icc1_xint
	.long	icc1_init
	. = 0x124               |0111
	.long	icc2_rint
	.long   icc2_xint
	.long	icc2_init
	. = 0x134               |0115
	.long	icc3_rint
	.long   icc3_xint
	.long	icc3_init
#endif
#ifdef  COL
	. =     0x144           |0121
	.long   sccint1         |       J.D SCC interrupt
	.long   buttonint1      |       J.D Mouse-Button interrupt
	.long   vsint1          |       J.D Vsync. interrupt
	. =     0x154           |0121
	.long	sccint2		|	J.D SCC interrupt
	.long	buttonint2	|	J.D Mouse-Button interrupt
	.long	vsint2		| 	J.D Vsync. interrupt
#endif
#ifdef BIP
#define BIPVEC  0x16c
	. = BIPVEC              | 0133
	.long   bmt_init
	.long   bmt_intr0
	.long   bmt_intr1
	.long   bmt_intr2
	.long   bmt_intr3
#endif
#ifdef DQS
	.globl  _dqs0_vec, _dqs1_vec, _dqs2_vec, _dqs3_vec,
	.=      0x180           | 0140
_dqs0_vec:
	.long   dqsint0
	.=      0x190           | 0144
_dqs1_vec:
	.long   dqsint1
	.=      0x1a0           | 0150
_dqs2_vec:
	.long   dqsint2
	.=      0x1b0           | 0154
_dqs3_vec:
	.long   dqsint3
#endif
#ifdef HK2
	. =     0x1a0           | 0150  same as dqs1
	.long	hk2intr
#endif
	. = 0x1e8
	.long   mfpxint         | mfp transmit interrupt
	.long   mfperrint       | mfp rx error interrupt
	.long   mfprint         | mfp receive interrupt
|
#ifdef LP
	. = 0x200               | 0200
	.long	lpintr
#endif
#ifdef  HK
	. =     0x220           | 0210
	.long   hkintr
#endif
#if TS
	. = 0x250               | 0224
	.long	tapeintr
#endif
#ifdef  SDLC
	. =     0x284           | 0241
	.long   sdlcint
	. =     0x288           | 0242
	.long   sdlcfatal
#endif
#ifdef X25
	. = 0x28c		|0243
	.long  x25int0
	. = 0x294		|0245
	.long  x25int1
	. = 0x298		|0246
	.long  x25int2
	. = 0x29c		|0247
	.long  x25int3
#endif
#ifdef  SW
	. =     0x2b0           | 0254
	.long   swintr
#endif
#ifdef  ST
	. =     0x2e0           | 0270
	.long   stintr
#endif
#ifdef MUXKE2
	. = 0x380               | DH or MUXKE2 first board receive intr
	.long   dhrint          | 0340
	. = 0x384               | MUXKE2 second board receive intr
	.long   dh1rint         | 0341
	. = 0x388               | 0342
	.long   dmintr          | MUXKE2 first board modem interrupt
	. = 0x38c               | 0343
	.long   dm1intr         | MUXKE2 second board modem intr
	. = 0x390               | DH or MUXKE2 first board transmit intr
	.long   dhxint          | 0344
	. = 0x394               | MUXKE2 second board transmit interrupt
	.long   dh1xint         | 0345
#endif  MUXKE2
#ifdef  SW2
	. = 0x3e0               | 0370
	.long   sw2intr
#endif
#if LP && NLP > 1     /* second printer */
	. = 0x3f0               |0374
	.long	lp2intr
#endif
|
	. = 0x400               | start of runtime system code
				| some free space for miscellany
	. = 0x440
|
minsp = 0x3f8f0000
__entry:movw    #0x2700,sr
	movl    #0xb0000000,d0          | set up 1MB of mem
	lea     _sbrpte,a0
	movw    #255,d1
set0:   movl    d0,a0@+
	addqw   #1,d0
	dbra    d1,set0
	movl    #minsp,a7               | use this as stack (temporarily)
	lea	usertrap,a0
	movl    #0x3f800400,a1
init0:  tstl    a0@                     | initialize unused interrupt vectors
	bnes    init1
	movl	#ille,a0@
init1:	addql	#4,a0
	cmpl    a1,a0
	blts    init0
	jmp	initcode
|
dump:   jsr     _dump
	bra     __entry
|
except: moveml  #0xfffe,a7@-
	movec   usp,a0
	movl	a0,a7@-
	jsr     _trap
	movl	a7@+,a0
	movec   a0,usp
	moveml  a7@+,#0x7fff
	rts
|
buserror:
	movl    #2,a7@-
bus0:	bsr	except
	addql   #4,a7
	tstw    a7@(6)                  | * stacktype = 0 ?
	beqs    bus2
	rte                             | rte with restart
bus2:   tstw    a7@(8)                  | long bus error stack frame ?
	bnes    bus3
	movl    a7@,a7@(24)             | * copy ps,pc,stacktype to start of frame
	movl    a7@(4),a7@(28)
	addl    #24,a7                  | * pop frame
	rte                             | rte without restart
bus3:   movl    a7@,a7@(84)             | * copy ps,pc,stacktype to start of frame
	movl    a7@(4),a7@(88)
	addl    #84,a7                  | * pop frame
	rte                             | rte without restart
|
addresserror:
	movl    #3,a7@-
	bras    bus0
|
exnorm:	bsr	except
	addql   #4,a7
	movl    #0xffc0fc00,0x3ffc9000
	rte
|
illegalopcode = .
	movl    #4,a7@-
	bras    exnorm
|
zerodivide = .
	movl    #5,a7@-
	bras    exnorm
|
chkerror = .
	movl    #6,a7@-
	bras    exnorm
|
trapverror = .
	movl    #7,a7@-
	bras    exnorm
|
priverror = .
	movl    #8,a7@-
	bras    exnorm
|
trace:  movl    #9,a7@-
	bras    exnorm
|
clrcaches:
	movl    d0,a7@-
	moveq   #9,d0
	movec   d0,cacr         | clear cpu cache
	movw    __ccr,d0
	orw     #7,d0
	andw    #0xfffb,d0
	movw    d0,__ccr        | clear data cache
	movl    a7@+,d0
	rte
|
trapmon:movl    #47,a7@-
	bras    exnorm
|
trapill:movl    #44,a7@-
	bras    exnorm
|                       68010 system calls
trapold:movl    #45,a7@-
	bras    exnorm
|                       68020 system calls
trapnew:movl    #46,a7@-
	bra     exnorm
|
spurious = .
	movl    #24,a7@-
	bra     exnorm
coprocviol = .
	movl    #13,a7@-
	bra     exnorm
fpex = .
	movl    #48,a7@-
	bra     exnorm
interrupt1 = .
	movl    #25,a7@-
	bra     exnorm
interrupt2 = .
	movl    #26,a7@-
	bra     exnorm
interrupt3 = .
	movl    #27,a7@-
	bra     exnorm
interrupt4 = .
	movl    #28,a7@-
	bra     exnorm
interrupt5 = .
	movl    #29,a7@-
	bra     exnorm
interrupt6 = .
	movl    #30,a7@-
	bra     exnorm
interrupt7 = .
	movl    #31,a7@-
	bra     exnorm
|
e_handler = .
	movl    #10,a7@-
	bra     exnorm
|
ille:   movl    #0,a7@-
	bra     exnorm
|
eaint:  moveml  #0xfffe,a7@-
	movec   usp,a0
	movl	a0,a7@-
	jsr     _eaintr
	movl	a7@+,a0
	movec   a0,usp
	moveml  a7@+,#0x7fff
	addql   #8,a7
	movl    #0xffc0fc00,0x3ffc9000
	rte
|
	.globl  _mfprint
	.globl  _mfpxint
	.globl  _mfperrint
mfpxint:movl    #0,a7@-
	movl    #_mfpxint,a7@-
	bra	eaint
mfperrint:movl  #0,a7@-
	movl    #_mfperrint,a7@-
	bra	eaint
mfprint:movl    #0,a7@-
	movl    #_mfprint,a7@-
	bra	eaint
#ifdef  HK
	.globl  _hkintr
hkintr: clrl    a7@-
	movl	#_hkintr,a7@-
	bra     eaint
#endif
#ifdef	HK2
	.globl  _hk2intr
hk2intr:clrl    a7@-
	movl	#_hk2intr,a7@-
	bra	eaint
#endif
#ifdef  SW
	.extern _swintr
swintr: clrl    a7@-
	movl    #_swintr,a7@-
	bra     eaint
#endif
#ifdef  SW2
	.extern _sw2intr
sw2intr: clrl    a7@-
	movl    #_sw2intr,a7@-
	bra     eaint
#endif
#ifdef  BIP
	.globl  _bip_intr
_bip_intr:
	.word   BIPVEC + 0x4
	.word   BIPVEC + 0x8
	.word   BIPVEC + 0xc
	.word   BIPVEC + 0x10

	.extern _Bmt_init,_Bmt_intr
bmt_init: clrl    a7@-
	movl	#_Bmt_init,a7@-
	bra	eaint
bmt_intr0: clrl    a7@-
	movl	#_Bmt_intr,a7@-
	bra	eaint
bmt_intr1: movl  #1,a7@-
	movl	#_Bmt_intr,a7@-
	bra	eaint
bmt_intr2: movl #2,a7@-
	movl	#_Bmt_intr,a7@-
	bra	eaint
bmt_intr3: movl  #3,a7@-
	movl	#_Bmt_intr,a7@-
	bra	eaint
#endif
#ifdef DQS
	.extern _dqsintr
dqsint0:
	clrl   a7@-
	pea    _dqsintr
	jra    eaint
dqsint1:
	pea    1
	pea    _dqsintr
	jra    eaint
dqsint2:
	pea    2
	pea    _dqsintr
	jra    eaint
dqsint3:
	pea    3
	pea    _dqsintr
	jra    eaint
#endif
#ifdef  COL
	.globl  _sccintr,_vsync_int,_button_int
vsint1: clrl    a7@-
	movl    #_vsync_int,a7@-
	bra	eaint
buttonint1: clrl    a7@-
	movl    #_button_int,a7@-
	bra	eaint
sccint1: clrl    a7@-
	movl    #_sccintr,a7@-
	bra	eaint
vsint2: movl     #1,a7@-
	movl    #_vsync_int,a7@-
	bra	eaint
buttonint2: movl #1,a7@-
	movl    #_button_int,a7@-
	bra	eaint
sccint2: movl   #1,a7@-
	movl    #_sccintr,a7@-
	bra	eaint
#endif
#if defined(IW) || defined(IS) || defined (IF) || defined(ICC_SCC) || defined(ICC_TCPIP)
	.extern _icc_rint
	.extern _icc_xint
	.extern _icc_init_done
icc0_rint:      movl    #0,a7@-
	movl	#_icc_rint,a7@-
	bra	eaint
icc0_xint:      movl    #0,a7@-
	movl	#_icc_xint,a7@-
	bra	eaint
icc0_init:      movl    #0,a7@-
	movl	#_icc_init_done,a7@-
	bra	eaint
icc1_rint:      movl    #1,a7@-
	movl	#_icc_rint,a7@-
	bra	eaint
icc1_xint:      movl    #1,a7@-
	movl	#_icc_xint,a7@-
	bra	eaint
icc1_init:      movl    #1,a7@-
	movl	#_icc_init_done,a7@-
	bra	eaint
icc2_rint:      movl    #2,a7@-
	movl	#_icc_rint,a7@-
	bra	eaint
icc2_xint:      movl    #2,a7@-
	movl	#_icc_xint,a7@-
	bra	eaint
icc2_init:      movl    #2,a7@-
	movl	#_icc_init_done,a7@-
	bra	eaint
icc3_rint:      movl    #3,a7@-
	movl	#_icc_rint,a7@-
	bra	eaint
icc3_xint:      movl    #3,a7@-
	movl	#_icc_xint,a7@-
	bra	eaint
icc3_init:      movl    #3,a7@-
	movl	#_icc_init_done,a7@-
	bra	eaint
#endif
#ifdef MUXKE2
	.extern _dmintr
dmintr: clrl    a7@-
	movl	#_dmintr,a7@-
	bra	eaint
dm1intr:movl    #1,a7@-
	movl	#_dmintr,a7@-
	bra	eaint
	.extern _dhrint
	.extern _dhxint
dhrint: clrl    a7@-
	movl	#_dhrint,a7@-
	bra	eaint
dh1rint:movl    #1,a7@-
	movl	#_dhrint,a7@-
	bra	eaint
dhxint: clrl    a7@-
	movl	#_dhxint,a7@-
	bra	eaint
dh1xint:movl    #1,a7@-
	movl	#_dhxint,a7@-
	bra	eaint
#endif  MUXKE2
#if TS
	.extern _tapeintr
tapeintr:       clrl    a7@-
	movl	_tapeintr,a7@-
	bra	eaint
#endif
#ifdef  ST
	.globl  _stintr
stintr: clrl    a7@-
	movl    #_stintr,a7@-
	bra     eaint
#endif
#ifdef	LP
	.extern _lpintr
lpintr: clrl    a7@-
	movl	#_lpintr,a7@-
	bra	eaint
#if NLP > 1
lp2intr:movl    #1,a7@-
	movl	#_lpintr,a7@-
	bra	eaint
#endif
#endif
#ifdef X25
	 .extern  _x25intr
x25int0:  clrl  a7@-
x25i:     movl #_x25intr,a7@-
	 bra    eaint
x25int1:  movl #1,a7@-
	 bras  x25i
x25int2:  movl #2,a7@-
	 bras  x25i
x25int3:  movl #3,a7@-
	 bras  x25i
#endif
#ifdef SDLC
	.extern _sdlcint
	.extern _sdlcfatal
sdlcint:clrl  a7@-
	movl #_sdlcint,a7@-
	bra    eaint
sdlcfatal:
	clrl  a7@-
	movl #_sdlcfatal,a7@-
	bra    eaint
#endif
