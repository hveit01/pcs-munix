static char * _Version = "@(#) RELEASE:  2.2  May 26 1987 /usr/sys/c.c ";
/*
Modifications
vers    when    who     what
2.2     260587  rob     new iw_sizes for WD52 and WD53
2.1     240487  rob     new iw_sizes for WD51,WD52,WD53 (and WD54)
2.0     171186  nb      sdlc driver (IBM-3274); cdev: 50; SDLC.
1.9     131186  jh      tty structure for PTY
1.8     071186  bl      color update
1.7     20.Oct  iw      ifdef MUXKE, x25pty.c = pty.c
1.6     011086  wild    non-standard Ethernet address for DECNet emulation
1.5      8.Oct  rb, iw  rje driver (kedqs); cdev: 21; DQS
1.4      9.Sep  bl      bmt instead of bip, mxt=8
1.3      4.Sep  bmz	Declare x25pt_tty[] here instead of in x25pty.c.
1.2     27.Aug  iw      x25, BOOTCON
1.1     160786  jh/nm   diskless node support
*/


#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/conf.h"
#include "sys/signal.h"
#include "sys/dir.h"
#include "sys/user.h"
#include "sys/fssizes.h"
#include "sys/errno.h"

#include "conf.h"


#if defined(ICC_ETH) || defined(ICC_UNISON) || defined(IW) || defined(IS) || defined(IF) || defined(ICC_SCC) || defined(ICC_TCPIP) || defined(X25) || defined(SDLC)
#       define ICC 1
#endif

#if defined(ICC_ETH) || defined(ICC_UNISON) || defined(IW) || defined(IS) || defined(IF) || defined(ICC_SCC) || defined(ICC_TCPIP)
#       define ICC_BOOT 1 /*???*/
#endif

#include "sys/space.h"



int     nodev(), nulldev();
int     sel_true ();

#ifdef  BIP
int     bipopen(), bipclose(), bipread(), bipwrite(), bipioctl();
int     bip_sel ();
extern  struct tty bip_tty[];
#endif

#if !defined(DLSUPPORT) || !defined(MASTERNODE)
#undef MAXDLNACTV
#define MAXDLNACTV 0
#endif

#if defined(DLSUPPORT) /*&& ! defined(MASTERNODE)*/
int     dsklinit(), rmtopen(), rmtclose(), rmtstrategy();
#endif


#ifdef  COL
int     cbip_open(), cbip_close(), cbip_read(), cbip_write(), cbip_ioctl();
int     cbip_sel ();
extern  struct tty cbip_tty[];
#endif

#ifdef  HK
int     hkopen(), hkstrategy(), hkread(), hkwrite();
#endif

#ifdef  HK2
int     hk2open(), hk2strategy(), hk2read(), hk2write();
#endif

#ifdef  ICC
int     iccopen_d(), iccclose_d(), iccwrite_d(), iccioctl_d();
#endif

#ifdef ICC_ETH
int     icc_ethopen(), icc_ethclose(), icc_ethread(), icc_ethwrite(),
	icc_ethioctl();
#endif

#ifdef  ICC_SCC
int     sccopen(), sccclose(), sccread(), sccwrite(), sccioctl();
extern  struct tty scc_tty[];
#endif

#ifdef ICC_TCPIP
		/* the admin device for direct icc access/ socket admin */
int	tcp_adopen(), tcp_adclose(), tcp_adread(), tcp_adwrite(), tcp_adioctl();

		/* now the driver for all those little sockets */
int	tcp_soopen(), tcp_soclose(), tcp_soread(), tcp_sowrite(), tcp_soioctl();
short arpa_icc=ARPA_ICC;
#endif

#ifdef ICC_UNISON
int     icc_lance_init(), icc_lance_getbuf(), icc_lance_sizbuf();
int     icc_lance_relbuf(), icc_lance_transmit(), icc_lance_monitor();
int     icc_lance_xname();
int     uiinit();
int     (*uxreceive)() = NULL;
#endif

#ifdef  IF
int     if_open(), if_strategy();
int     if_read(), if_write(), if_ioctl();
#endif

#ifdef  IS
int     is_open(), is_close(), is_strategy();
int     is_read(), is_write(), is_ioctl();
#undef ICC_BOOT
#define ICC_BOOT 1
#endif

#ifdef  IW
int     iw_open(), iw_strategy();
int     iw_read(), iw_write(), iw_ioctl();
#endif

#ifdef  LP
int     lpopen(), lpclose(), lpwrite(), lpioctl();
#endif

#ifdef	DQS
int	dqsopen(), dqsclose(), dqsread(), dqswrite(), dqsioctl();
#endif

#ifdef  MUXKE2
int     dhopen(),  dhclose(),  dhread(),  dhwrite(),  dhioctl();
extern  struct tty dh_tty[];
#endif

/* Serial interface on CPU board, called mfp or SCO..                   */
int     mfpopen(),mfpclose(),mfpread(),mfpwrite(),mfpioctl();
extern struct tty mfp_tty[];

#ifdef PTY
int	ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcioctl();
int	ptsopen(), ptsclose(), ptsread(), ptswrite(), ptsioctl();
int     ptc_sel();
int ptc_dev = 39;
extern struct  tty pt_tty[];
#endif

#ifdef  SBP
int     sbpopen(), sbpclose(), sbpread(), sbpwrite(), sbpioctl();
#endif

#ifdef  SDLC
int     sdlcopen(), sdlcclose(), sdlcread(), sdlcwrite(), sdlcioctl();
int	sdlc_check();
#endif

#ifdef  ST
int     ststrategy(), stopen(), stclose(), stread(), stwrite(), stioctl();
#endif

#ifdef  SW
int     swopen(), swstrategy(), swread(), swwrite(), swioctl();
int     swinit();
#endif

#ifdef  SW2
int     sw2open(), sw2strategy(), sw2read(), sw2write(), sw2ioctl();
int     sw2init();
#endif

#ifdef  SXT
int     sxtopen(), sxtclose(), sxtread(), sxtwrite(), sxtioctl();
int     sxt_sel();
#endif

#ifdef  TS
int     tsopen(), tsclose(), tsstrategy(), tsread(), tswrite(), tsioctl();
int     tsintr();
int     tsbaemask = 0;          /* would be 0x2400 for Emulex Controller */
int     (*tapeintr)() = tsintr;
#endif

#ifdef SDLC
int sdlcopen(), sdlcclose(), sdlcread(), sdlcwrite(), sdlcioctl();
#endif

#ifdef X25
int	x25open(), x25close(), x25read(), x25write(), x25ioctl();
#endif


/* Munix/Net special lan device switch */
struct	ldevsw	ldevsw[] =
{
#ifdef ICC_UNISON
	icc_lance_init, icc_lance_getbuf, icc_lance_sizbuf, icc_lance_relbuf,
	icc_lance_transmit, icc_lance_monitor, icc_lance_xname, 0, 0,
#else
	nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,
#endif
	NULL
};


/* block device switch */

struct	bdevsw	bdevsw[] =
{
	nodev,   nodev,   nodev,                /* rm = 0               */
	nodev,   nodev,   nodev,                /* rl = 1               */
	nodev,   nodev,   nodev,                /* rx2= 2               */
	nodev,   nodev,   nodev,                /* tm = 3               */

#ifdef  HK
	hkopen, nulldev, hkstrategy,            /* hk = 4               */
#else
	nodev,  nodev,  nodev,
#endif

	nodev,  nodev,  nodev,                  /* rk = 5               */
	nodev,  nodev,  nodev,                  /* hp = 6               */
	nodev,  nodev,  nodev,                  /* td = 7               */

#if defined(DLSUPPORT) /*&& !defined(MASTERNODE)*/
	rmtopen, rmtclose,rmtstrategy,          /* rmt = 8 remote disk  */
#else
	nodev,   nodev,   nodev,
#endif

	nodev,  nodev,  nodev,                  /* hl = 9               */
	nodev,  nodev,  nodev,                  /* ot = 10              */
	nodev,  nodev,  nodev,                  /* sasi = 11            */

#ifdef  HK2
	hk2open,  nulldev, hk2strategy,         /* hk2 = 12             */
#else
	nodev,  nodev,  nodev,
#endif

#ifdef  TS
	tsopen, tsclose, tsstrategy,            /* ts = 13              */
#else
	nodev,  nodev,  nodev,
#endif

	nodev,  nodev,  nodev,                  /* ras_m1 = 14          */
	nodev,  nodev,  nodev,                  /* fd = 15              */
	nodev,  nodev,  nodev,                  /* eagle = 16           */

#ifdef  IW                            /* icc winchester = 17 & 17+64=81 */
	iw_open, nulldev, iw_strategy,
#else
	nodev,  nodev,  nodev,
#endif

#ifdef  IF
	if_open, nulldev, if_strategy,          /* icc floppy = 18      */
#else
	nodev,  nodev,  nodev,
#endif

#ifdef  SW
	swopen, nulldev, swstrategy,            /* sw disk = 19         */
#else
	nodev,  nodev,  nodev,
#endif

#ifdef  SW2
	sw2open, nulldev, sw2strategy,          /* sw2 disk = 20        */
#else
	nodev,  nodev,  nodev,
#endif
};

int     bdevcnt = sizeof(bdevsw) / sizeof(struct bdevsw);


int     mmread(), mmwrite();
int     prfread(), prfwrite(), prfioctl();
int     syopen(), syread(), sywrite(), syioctl();
int     erropen(), errclose(), errread(), errwrite(), errioctl();


/* format of cdevsw: open, close, read, write, ioctl, ttys */

struct	cdevsw	cdevsw[] =
{
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL, /* console, set dyn.*/
	nulldev, nulldev, mmread,  mmwrite,  nodev,    sel_true, NULL, NULL, /* mem = 1  */
	syopen,  nulldev, syread,  sywrite,  syioctl,  NULL,     NULL, NULL, /* tty = 2  */
	nulldev, nulldev, prfread, prfwrite, prfioctl, NULL,     NULL, NULL, /* prof = 3 */

	erropen, errclose,errread, errwrite, errioctl, NULL,     NULL, NULL, /* err = 4; was rrl = 4      */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,   /* rrm = 5      */

#ifdef COL                                              /* colour = 6   */
	cbip_open,cbip_close,cbip_read,cbip_write,cbip_ioctl,
	cbip_sel, cbip_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,   nodev,     NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,   nodev,     NULL,     NULL, NULL,  /* rrx = 7      */

	nodev,   nodev,   nodev,   nodev,   nodev,     NULL,     NULL, NULL,  /* was mxt = 8  */
	nodev,   nodev,   nodev,   nodev,   nodev,     NULL,     NULL, NULL,  /* tm = 9       */

#ifdef LP                                               /* lp = 10      */
	lpopen,  lpclose, nodev,  lpwrite,  lpioctl,   NULL,     NULL, NULL,
#else
	nodev,   nodev,   nodev,  nodev,     nodev,    NULL,     NULL, NULL,
#endif

#ifdef HK                                               /* rhk = 11     */
	hkopen,  nulldev, hkread, hkwrite,   nodev,    sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,  nodev,     nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,  nodev,     nodev,    NULL,     NULL, NULL,  /* dz = 12      */

#ifdef MUXKE2                                           /* dh = 13      */
	dhopen,  dhclose, dhread, dhwrite,   dhioctl,  NULL,     dh_tty,NULL,
#else
	nodev,   nodev,   nodev,  nodev,     nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rhp = 14     */
							/* SCO console = 15 */
	mfpopen, mfpclose,mfpread, mfpwrite, mfpioctl, NULL,     mfp_tty,NULL,
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dlv11-j =16  */

#ifdef ST                                               /* rst = 17     */
	stopen,  stclose, stread,  stwrite,  stioctl,  sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* lbp = 18     */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rhl = 19     */

#ifdef HK2                                              /* rhk2 = 20    */
	hk2open, nulldev, hk2read, hk2write, nodev,    sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef DQS
	dqsopen, dqsclose,dqsread, dqswrite, dqsioctl, NULL,     NULL, NULL,  /* dqs = 21 */
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dqs   = 21   */
#endif

#ifdef ICC_ETH                                          /* ether = 22   */
  icc_ethopen,icc_ethclose,icc_ethread,icc_ethwrite,icc_ethioctl,NULL,NULL,NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef SBP                                              /* sbp = 23     */
	sbpopen, sbpclose,sbpread, sbpwrite, sbpioctl, NULL,     NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rot = 24     */

#ifdef BIP                                              /* bmd = 25     */
	bipopen, bipclose,bipread, bipwrite, bipioctl, bip_sel,  bip_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* slu = 26     */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* sasi = 27    */

#ifdef  TS                                             /* ts = 28      */
	tsopen,  tsclose, tsread,  tswrite,  tsioctl,  sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dlv11-e = 29 */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* raster = 30  */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rfd = 31     */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rampage = 32 */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* rwt = 33     */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* mbus = 34    */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* reagle = 35  */
  
#ifdef SXT                                              /* sxt = 36     */
	sxtopen, sxtclose,sxtread, sxtwrite, sxtioctl, sxt_sel,  NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* nibbp = 37   */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* expack = 38  */

#ifdef PTY
	ptcopen,ptcclose, ptcread, ptcwrite, ptcioctl, ptc_sel,  NULL, NULL,  /* ptc = 39 */
	ptsopen,ptsclose, ptsread, ptswrite, ptsioctl, NULL,     pt_tty, NULL,/* pts = 40 */
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /*41:exos admin */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /*42:exos download */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /*43:exos socket*/
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dhv = 44     */

#ifdef  IW                                      /* icc winchester = 45  */
	iw_open, nulldev, iw_read, iw_write, iw_ioctl, sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef  IF                                      /* icc floppy = 46      */
	if_open, nulldev, if_read, if_write, if_ioctl, sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef  IS                                      /* icc streamer = 47    */
	is_open, is_close,is_read, is_write, is_ioctl, sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef  ICC_SCC                                 /* icc scc = 48         */
	sccopen, sccclose,sccread, sccwrite, sccioctl, NULL, scc_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL, NULL,    NULL,
#endif

#ifdef  ICC                                     /* icc download = 49    */
	iccopen_d, iccclose_d, nodev, iccwrite_d, iccioctl_d, NULL, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,           NULL, NULL, NULL,
#endif

#ifdef SDLC                                     /* sdlc = 50            */
	sdlcopen, sdlcclose, sdlcread, sdlcwrite, sdlcioctl, NULL, NULL, NULL,
#else
	nodev, nodev, nodev, nodev, nodev,                   NULL, NULL, NULL,
#endif

#ifdef X25
						/* x25 = 51             */
	x25open, x25close,x25read, x25write, x25ioctl, NULL,     NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,   /* x25pts=52 */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,   /* x25ptc=53 */

#ifdef  SW                                              /* sw disk = 54 */
	swopen,  nulldev, swread,  swwrite,  swioctl,  sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

#ifdef  SW2                                             /* sw2 disk = 55 */
	sw2open, nulldev, sw2read, sw2write, sw2ioctl, sel_true, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* 56 = gpib interf.*/
#ifdef ICC_TCPIP                                /* full TCP/IP admin devc 57 */
   tcp_adopen,tcp_adclose,tcp_adread,tcp_adwrite,tcp_adioctl,NULL, NULL, NULL,
   tcp_soopen,tcp_soclose,tcp_soread,tcp_sowrite,tcp_soioctl,NULL, NULL, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif
};

int     cdevcnt = sizeof(cdevsw) / sizeof(struct cdevsw);


struct cdevsw conssw[] = {

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* 0            */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dlv11-j      */
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dz           */

#ifdef  MUXKE2                                          /* dh           */
	dhopen,  dhclose, dhread,  dhwrite,  dhioctl,  NULL,     dh_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* slu          */

#ifdef  BIP                                             /* bmd          */
	bipopen, bipclose,bipread, bipwrite, bipioctl, bip_sel,  bip_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,  /* dlv11-e      */

#ifdef  ICC_SCC                                         /* icc scc      */
	sccopen, sccclose,sccread, sccwrite, sccioctl, NULL,     scc_tty, NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif
							 /* SCO console */
	mfpopen, mfpclose,mfpread, mfpwrite, mfpioctl, NULL,     mfp_tty, NULL,

#ifdef  COL                                             /* colour       */
	cbip_open, cbip_close, cbip_read, cbip_write, cbip_ioctl,
	cbip_sel, &cbip_tty[0], NULL,
#else
	nodev,   nodev,   nodev,   nodev,    nodev,    NULL,     NULL, NULL,
#endif

};


dev_t   rootdev = makedev(ROOT, ROOTUNIT);
dev_t   swapdev = makedev(SWAP, SWAPUNIT) | Fs2BLK;
dev_t   pipedev = makedev(ROOT, ROOTUNIT);
dev_t   rmtrootdev;     /* device numbers for remote root, swap, pipe */
dev_t   rmtswapdev;
dev_t   rmtpipedev;
daddr_t swplo   = SWAPLO;
int     nswap   = NSWAP;
int     hz = HERTZ;             /* line frequency, normally 50 or 60    */
int     haveclock = 1;
time_t  bootime;
int bootconsole = BOOTCON;

dev_t   comrootdev = NODEV;     /* device number of common root fs      */
int     comswapsmi;             /* common swaptable entry  */

struct  filsys  *rootfp;        /* pointer to superblock */


/*
 * diskless/master node information
 */
#ifdef MASTERNODE
	int master  = 1;                /* 0 = diskless, 1 = master     */
	int ownswap = 1;                /* 0 = don't own swap, 1 = do   */
#ifdef DLSUPPORT
	int maxdlnactv = MAXDLNACTV;    /* max. number of active dlnodes */
#else DLSUPPORT
	int maxdlnactv = 0;             /* max. number of active dlnodes */
#endif DLSUPPORT
#else MASTERNODE
	int master = 0;                 /* 0 = diskless, 1 = master      */
	int ownswap = 0;                /* 0 = don't own swap, 1 = do    */
	int maxdlnactv = 0;             /* max. number of active dlnodes */
#endif MASTERNODE

#ifdef ICC_ETH
int     nonstdea = NONSTDEA;            /* standard/nonstandard Ethernet address */
#else
int     nonstdea = 0;                   /* standard Ethernet address */
#endif

ipnetaddr master_ip = 0L;               /* master's ip address           */
enetaddr master_id = {0, 0, 0};         /* master's ethernet address     */
long req_q;				/* bit map of pending dl requests */



#ifdef  ICC_BOOT                /* icc_boot = 1 => icc will be initialized */
	int icc_boot = 1;       /* and used during UNIX startup            */
#else
	int icc_boot = 0;       /* icc will not be initialized and used    */
#endif                          /* during startup                          */

#ifdef BIP
int     bip_cnt  = NBIP;        /* number of bitmap displays            */
struct tty bip_tty[NBIP];
char    *bip_addr[] =
{
#define BIADDR 0x300000
	(char *)(BIADDR+0),
	(char *)(BIADDR+0x40000),
	(char *)(BIADDR+0x80000),
	(char *)(BIADDR+0xc0000)
};
#else
int     bip_cnt = 0;
char    *bip_addr[1];
#endif

#ifdef COL
int     cbip_cnt  = NCBIP;     /* number of colour bitmap displays            */
struct tty cbip_tty[NCBIP];
char    *cbip_addr[] =
{
#define CBIPADDR 0xb0000000
      (char *)(CBIPADDR+0),
      (char *)(CBIPADDR+0x100000),
};
unsigned long    cbip_io_base[] =
{
#define CBIP_IOBASE 0xf0000000
      (CBIP_IOBASE),
      (CBIP_IOBASE+0x100000),
};
int	cbip_intr_vec[] =
{
#define CBIP_INTR_VEC	0x144
	(CBIP_INTR_VEC),
	(CBIP_INTR_VEC + 0x10),
};
#else
int     cbip_cnt = 0;
char    *cbip_addr[1];
unsigned long cbip_io_base[1];
int	cbip_intr_vec[1];
#endif


/*  DH has standard addresses.  MUXKE2 differs a bit from DH:
 *  modem interrupt is between  rcv and tx interrupt, and address of
 *  second controller = address of first + 0x100.
 *  MUXKE2 always comes with "DM"
 */
#ifdef  MUXKE2
int     dh_timo[NDH11];
int     dh_cnt  = NDH11;                /* number of DH11 lines         */
struct  tty dh_tty[NDH11];
char *dh_addr[] =
{
#define DHADDR  ((char *)0x3FFFE010)
	(char *)(DHADDR+000),
	(char *)(DHADDR+0x100),
};

char    *dm_addr[] =
{
#define DMADDR  ((char *)0x3FFFE020)
	(char *)(DMADDR+000),
	(char *)(DMADDR+0x100)
};
#endif

#ifdef DQS
char *dqs_addr[] =
{
#define DQSADDR 0x3FFFF200          /* BSC-KE/BSC-KE2 baseaddress */
	(char *)(DQSADDR+0x20),
	(char *)(DQSADDR+0x50),
	(char *)(DQSADDR+0x80),
	(char *)(DQSADDR+0xB0)
};
char *dqs_swp = (char *)(DQSADDR+0x02);  /* byte swab yes/no */
char *dqs_rel = (char *)(DQSADDR+0x05);  /* release number */
#endif

#ifdef LP
int     lp_cnt  = NLP;          /* number of parallel printers, <= 2    */
char *lp_addr[] =
{
	(char *)0x3FFFFF4C,     /* very funny addresses ?!?             */
	(char *)0x3FFFE7F8      /* for second printer                   */
};
#endif

#ifdef PTY
struct	tty pt_tty[NPTY];
struct	pt_ioctl {
	int	pt_flags;
	struct  proc *pt_selr;
} pt_ioctl[NPTY];
int pt_cnt = NPTY;
#endif

struct	size
{
	daddr_t	nblocks;
	int     cyloff;
};


/*
 *      The sizes structures defines the logical separation of a disk pack
 *	into mountable file systems.
 *
 *	The nblocks entry specifies the number of blocks (512 bytes) in
 *	the file system.
 *
 *	The cyloff entry specifies the offset+1 from the physical beginning
 *	of the pack to the beginning of the file system in cylinders
 *
 *      The minor device number is computed as
 *
 *              (physical_drive_number * 8) + row_number_in_array
 *
 *      e.g. if you have two rk07s, the device files with size 10560
 *      starting on cylinder 248 would have the numbers 0*8+1=1 and 1*8+1=9.
 */

#if defined(HK) || defined(HK2)

#define RK07BL  53790           /* number of sectors on RK07 disk pack  */
#define RK06BL  27126           /* number of sectors on RK06 disk pack  */
#define SPARE   154             /* number of sectors reserved for bad sector handling (last tracks) */

struct  size hk_sizes[8] =
{
	16368,          0,      /* 8 MB for root, or medium swap        */
	37224,          248,    /* 18 MB, e.g  for usr                  */
	10560,          248,    /* 5 MB for small swap                  */
	26664,          408,    /* 13 MB for 2nd half of RK07           */
	18612,          248,    /* 9 MB for 2nd third of RK07           */
	18612,          530,    /* 9 MB for 3rd third of RK07           */
	26928,          0,      /* 13 MB for entire RK06  or big swap
						  or 1st half of RK07   */
	RK07BL-SPARE,   0,      /* 26 MB for entire RK07                */
};
#endif  HK

#if defined(IW)
/*
 *      The struct iw_sizes defines the logical separation of a disk pack
 *      into mountable file systems.
 *
 *      The minimal sizes of the open disks are for the Fujitsu 86.
 *
 *      The file systems are numbered from 0 to 15. File system 1
 *      is for diskless node roots.
 *
 */

struct  fs_sizes iw_sizes[16] =
{
	 16384,             0,  /* 0    root                        8 Mb */
	 10240,         16384,  /* 1    dn                          5 Mb */
	 30720,         26624,  /* 2    swap                       15 Mb */
	999999,         57344,  /* 3    usr   rest of data pack    ?? Mb */
	 69184,             0,  /* 4    first 1/2 of wd51b         35 Mb */
	 69192,         69184,  /* 5    second 1/2 of wd51b        35 Mb */
	999999,        138376,  /* 6    rest of wd52               ?? Mb */
	999999,        285040,  /* 7    rest of wd53               ?? Mb */
	999999,        494904,  /* 8    rest of wd54               ?? Mb */
	 20480,         16384,  /* 9    medium swap space          10 Mb */
	999999,         36864,  /* 10   rest of data pack          ?? Mb */
	104928,        285040,  /* 11   first 1/2 of rest of wd53  52 Mb */
	999999,        389968,  /* 12   second 1/2 of rest of wd53 ?? Mb */
	     0,             0,  /* 13   free for expansion               */
	     0,             0,  /* 14   free for expansion               */
	999999,             0,  /* 15   whole disk                ??? Mb */
};
#endif IW

#if defined(SW) || defined(SW2)
/*
 *      The number of elements in the sw_sizes[]
 *      eg: 823 cyl * 10 heads * 32 sec/trk = 263360 secs
 *          total = 263360 - 2(for bad block list) - SW_SPARE = 263148 secs
 *      WD81  131724   84/65  Fujitsu 8", model 2312
 *      WD82  263148  168/131 Fujitsu 8", model 2322
 *      WD83  534738  337/267 Fujitsu 8", model 2333
 *      WD41  774428  474/387 Fujitsu 10 1/2", model 2351 (eagle)
 *      WD42 1094388  689/547 Fujitsu 10 1/2", model 2361 (super eagle)
 */

#define SW_SPARE 210L                   /* alternative sectors          */
					/* corresponds to SW_SPARE in sw.h */
#define WD81 (589L*7L*32L-SW_SPARE-2L)  /* 84/     Fuj. 8", model 2312  */
#define WD82 (823L*10L*32L-SW_SPARE-2L) /* 168/131 Fuj. 8", model 2322  */
#define WD83 (823L*10L*65L-SW_SPARE-2L) /* 337/267 Fuj. 8", model 2333  */
#define WD84 (624L*27L*65L-SW_SPARE-2L) /* 690/547 Fuj. 8", model 2344  */
#define WD41 (842L*20L*46L-SW_SPARE-2L) /* 474/387 Fuj. 10 1/2", model 2351 (eagle)       */
#define WD42 (842L*20L*65L-SW_SPARE-2L) /* 689/547 Fuj. 10 1/2", model 2361 (super eagle) */

struct  fs_sizes sw_sizes[16] =
{
	16384,      0,           /* 0  root                          8 MB  */
	10240,  16384,           /* 1  dn root, /tmp, small swap     5 MB  */
	30720,  26624,           /* 2  big swap                     15 MB  */
     99999999,  57344,           /* 3  rest of data pack            ?? MB  */
       205804,  57344,           /* 4  rest of WD82                102 MB  */
	68601,  57344,           /* 5  first 1/3  of rest of WD82   34 MB  */
	68601, 125945,           /* 6  second 1/3 of rest of WD82   34 MB  */
	68601, 194546,           /* 7  third 1/3  of rest of WD82   34 MB  */
       102902,  57344,           /* 8  first 1/2  of rest of WD82   51 MB  */
       102902, 160246,           /* 9  second 1/2 of rest of WD82   51 MB  */
       271590, 263148,           /* 10 up to WD83                  135 MB  */
     99999999, 534738,           /* 11 anything after WD83          ?? MB  */
				 /*    WD41   236990               118 MB  */
				 /*    WD42   556952               278 MB  */
				 /*    WD84   560170               278 MB  */
       279825, 534738,           /* 12 third 1/4 of WD42           138 MB  */
     99999999, 814563,           /* 13 last 1/4 of WD42(279824)/WD84(280344)    138 MB  */
	    0,      0,           /* 14 free for expansion                  */
     99999999,      0,           /* 15 all of data pack                    */
};
#endif

#ifdef X25
/*
 * The MUNIX X.25 driver informs the ICC about the interrupt vectors
 * it should use.
 */
short  x25vector[] = { 0243, 0245, 0246, 0247 };
/* 1.7
struct tty x25pt_tty[NPTY];
int x25pt_cnt = NPTY;
*/
#endif X25


/* aunix questions */

#ifdef DIALOG

struct devs { char *devnam; int devmaj} devs[] =
{
	"hk",4,
	"iw",81,
	"sw",19,
	0
};

atoi(p)
register char *p;
{
	register int n;

	n = 0;
	while (*p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
	return(n);
}

dialog()
{       struct devs *dp;
	char line[40];
	int  maj, min;

l1:     printf("please enter type of root device (");
	for (dp = devs; dp->devnam; dp++)
		if (dp->devmaj >= 0) printf("%s ",dp->devnam);
	printf("): ");
	gets(line);
	maj = -1;
	for (dp = devs; dp->devnam; dp++) {
		if (strcmp(dp->devnam, line) == 0) {
			maj = dp->devmaj;
			break;
		}
	}
	if (maj == -1) goto l1;
	printf("what unit (i.e. minor device): ");
	gets(line);
	min = atoi(line);
	rootdev = pipedev = makedev(maj, min);
	prdev("rootdev and pipedev is", rootdev);
l2:     printf("please enter type of swap device (");
	for (dp = devs; dp->devnam; dp++)
		if (dp->devmaj >= 0) printf("%s ",dp->devnam);
	printf("): ");
	gets(line);
	maj = -1;
	for (dp = devs; dp->devnam; dp++) {
		if (strcmp(dp->devnam, line) == 0) {
			maj = dp->devmaj;
			break;
		}
	}
	if (maj == -1) goto l2;
	printf("what unit (i.e. minor device): ");
	gets(line);
	min = atoi(line);
	swapdev = makedev(maj, min);
	prdev("swapdev is",swapdev);
	printf("where does swap area start (swplo): ");
	gets(line);
	swplo = atoi(line);
	printf("how many blocks in swap area (nswap): ");
	gets(line);
	nswap = atoi(line);
}

dialog1 ()
{
	sa_main ();
}
#else
	dialog() {}
	dialog1 (){}
#endif
