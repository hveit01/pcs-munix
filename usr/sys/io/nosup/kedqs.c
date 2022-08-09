/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.7  Oct 8 1986 /usr/sys/io/kedqs.c ";

/* this is not yet fully understood - possibly it is broken */
/* the 1.2 kernel does not contain this device */
/* KEDQS is a variant of the Dilog DQ-130 device, a tape formatter */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/buf.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"

/* number of devices */
#define NDQS 4

/* physical registers */
struct dqregs {
	short mtc;		/* command register r/w */
	short mts;		/* status register */
	short mtctrl;	/* control register r/w */
	short mtcma;	/* low 15-0 bits of transfer address */
	short mtbrc;	/* complement of byte cnt */
	char  mtvec;	/* vector number */
	char  mtspd;	/* transfer speed*/
	short reg12;	/* reg12 not used here*/
	char  mtcmah;	/* contains bits 22-17 !IS_REL0() */
	char  reg15;	/* reg15 not used here */
	char  reg16;	/* reg16 not used here */
	char  mtclk;	/* clock modifier !IS_REL0() */
};

/* mtc register bits/commands */
#define MTC_GO		0x0001	/* execute command */
#define MTC_FUMASK	0x000e	/* function code mask */	
#define  MFU_OFFREW	0x0000	/* function 0: offline/rewind */
#define  MFU_READ	0x0002	/* function 1: read */
#define  MFU_WRITE	0x0004	/* function 2: write */
#define  MFU_WREOF	0x0006	/* function 3: write EOF */
#define  MFU_SFWD	0x0008	/* function 4: space forward */
#define  MFU_SREV	0x000a	/* function 5: space backward */
#define  MFU_WREXT	0x000c	/* function 6: write with extended gap */
#define  MFU_REWND	0x000e	/* function 7: rewind */
#define MTC_ADMASK	0x0030	/* address mask bits 17-16 for rev0 */
#define MTC_INTENB	0x0040	/* enable interrupt */
#define MTC_CURDY	0x0080	/* CU ready cleared on cmd, set on EOT */
#define MTC_USMASK	0x0300	/* Unit select mask */
#define  MUS_0		0x0000	/* Unit 0 */
#define  MUS_1		0x0100	/* Unit 1 */
#define  MUS_2		0x0200	/* Unit 2 */
#define  MUS_3		0x0300	/* Unit 3 */

/* higher level commands */
#define CMD_OFFREW	MUS_3|MFU_OFFREW
#define CMD_TSTREAD	MUS_3|MFU_READ
#define CMD_TSTWRIT	MUS_3|MFU_WRITE
#define CMD_TSTCTRL	MUS_3|MFU_WREOF
#define CMD_READ	MUS_3|MTC_INTENB|MFU_READ
#define CMD_WRITE	MUS_3|MTC_INTENB|MFU_WRITE
#define CMD_CTRL	MUS_3|MTC_INTENB|MFU_WREOF
	
/* macros to set memory address */
#define SET_ADDRH_V0(regs, addr, cmd)\
	regs->mtc = cmd | ((((int)addr >> 16) & 3) << 4) | MTC_GO
#define SET_ADDRH_V1(regs, addr, cmd)\
	regs->mtcmah = ((int)addr >> 16) & 0x3f;\
	regs->mtc = cmd | MTC_GO

/* mts register bits */
#define MTS_TUR		0x0001	/* tape unit ready, set if tape stopped */
#define MTS_RWS		0x0002	/* rewind state, rewinding=1, rewound=0 */
#define MTS_WRL		0x0004	/* write protect=1 */
#define MTS_SDWN	0x0008	/* tape is slowing down=1 */
#define MTS_7CHN	0x0010	/* 7chan tape=1, 9chan=0 */
#define MTS_BOT		0x0020	/* begin of tape seen=1 */
#define MTS_SELR	0x0040	/* no tape unit, offline or off=0 */
#define MTS_NXM		0x0080	/* non-existing memory */
#define MTS_8		0x0100	/* unused */
#define	MTS_RLE		0x0200	/* record length error=1 */
#define MTS_EOT		0x0400	/* end of tape=1 */
#define MTS_BGL		0x0800	/* bus grant late=1 */
#define MTS_HE		0x1000	/* hard error=1 */
#define MTS_13		0x2000	/* unused */
#define MTS_EOF		0x4000	/* end of file=1 */
#define MTS_ILL		0x8000	/* ill command, write protect, SELR */

/* known control commands.
 * These may be actually control bits, but they are undocumented */
#define MC_START	0x7010	/* start tape */
#define MC_SEEK		0x2d01	/* seek block header */




/* Returned from getke_reg.*/
struct keregs {
	char mtcmah;	/* register high bits address (mtcma), rev=1 */
	char reg1;		/* reg1 (unused)*/
	short mtc;		/* register command (mtc) */
	short mts;		/* register status (mts) */
	short mtbrc;	/* register count (mtbrc) */
	short mtcma;	/* register address (mtcma) */
	short mtctrl;	/* register ctrl (mtctrl) */
};

extern struct dqstr { /*size=80*/
	char idx;		/* index ito dqstr array, point to current dqstr */
	char dev;		/* device */
	char time;		/* time of status (byte) */
	char rphase;	/* subphase of read */
	char wphase;	/* subphase of write */
	char iphase;	/* interrupt state machine */
	struct keregs keregs;
	short wcnt;		/* number of bytes written */
} dqstr[NDQS];

struct dqbuf {
#define BF_EMPTY	0
#define BF_DATA		1
#define BF_BUSY		2
	char rstate;	/* read buffer status 0=empty, 1=has data, 2=busy */
	char wstate;	/* write buffer status 0=empty, 1=has data, 2=busy */

	int rsize;
	int wsize;
	paddr_t rxaddr;
	paddr_t txaddr;
	struct dqbuf *rxnext; /* rx next link */
	struct buf *rxbuf;
	struct dqbuf *txnext; /* tx next link */
	struct buf *txbuf;
};

struct dqbuffer {
	struct keregs keregs;	/* register set for ioctl */
	short sz;				/* size of data */
	union {
		char b[512];		/* binary payload */
		struct {			/* set speed (ioctl 77) */
			ushort clkmod;
			ushort speed;
		} s;
		short mtctrl;		/* pass control word to formatter */
	} u;
};

/* per device info structure */
struct dqdevice {
#define RP_IDLE	0			/* unit available */
#define RP_1	1
#define RP_2	2
#define RP_3	3
	short rphase;			/* read subphase */

#define WP_IDLE	 0			/* unit available */
#define WP_1	-1			/* start unit for write */
#define WP_2	-2			/* find block header for write */
#define	WP_3	-3			/* writing data */
#define WP_4	-4
#define WP_5	-5			/* write error: set u.u_error */
#define WP_6	-6			/* write error: terminal state */
	short wphase;			/* write subphase */ 

#define ST_IDLE		0		/* no interrupts in progress */
#define ST_READ1	1		/* position to block header */
#define ST_STATE2	2
#define ST_STATE3	3
#define ST_STATE4	4
#define ST_STATE5	5
#define ST_WRITE1	7		/* start tape successful, find block header */
#define ST_WRITE2	8		/* found block header, writing */
#define ST_WRITE3	9

#define ST_TOSTATES	10		/* added to state when dqstout timer expired */

/* timeout states: a dqsintr was not received when dqstout timer expired */
#define ST_TOREAD1	11		/* timeout starting tape move */
#define ST_TOSTATE2	12		/* timeout find block header */
#define ST_TOSTATE3	13
#define ST_TOSTATE4	14
#define ST_TOSTATE5	15
#define ST_TOWRITE1	17
#define ST_TOWRITE2	18
#define ST_TOWRITE3	19
	char iphase;		/* interrupt state machine*/

	char off5;

#define OC_ISOPEN	1
#define OC_CLOSING	2
	char ocflag;				/* isopen=1, isclosing=2*/	
	char polling;				/* polling mode: 1=testmode, 2=set in interrupt */
	struct dqregs *regs;		/* ptr to registers of KE */
	struct dqregs *xregs;		/* copy of ptr to registers, used for wait on writers */
	char rwait;					/* set if reader waiting for data */
	char wwait;					/* set if writer waiting for buffer */
	char timeout;				/* delay count (tick=83ms/hz=60 100ms/hz=50) */
								/* will switch istate+=10 after expiry */
								/* note: wait times below are base on 60Hz */
	char retries;				/* retry counter after some operation failed */
	char off20;	/*used*/
	char off21;
	short lastctrl;				/* last ctrl issued, set at start of dqintr */
	short curctrl;				/* ctrl cmd to be executed */
	int rrecoff;				/* offset into read rrecbuf */
#define incseq() dp->reqseq++; dp->reqseq &= 15
#define decseq() dp->reqseq--; dp->reqseq &= 15
	short reqseq;				/* sequence# of record 0..15 */
	short off32;
	short off34;	/*used*/
	char off36;
	char off37;
	char errflg;				/* when set, write 0x9N error block */
	char closerew;				/* rewind on close dev !IS_REV0() */
	char rechdr[8]; 			/* record header */
	short off48;
	struct dqbuf *sbuflist; 	/* linked list of received sbufs */
	struct dqbuf *sbufcur;		/* ptr to a current sbuf */
	struct dqbuf *rrecbuf;		/* ptr to first received buf*/
	struct dqbuf *xbuflist; 	/* linked list of xbufs */
	struct dqbuf *xbufcur;		/* ptr to a current xbuf */
	struct dqbuf sbufs[4];		/* sbufs */
	struct dqbuf xbufs[4];		/* xbufs */
} dqsx[NDQS] = { 0, }; 

/* for byte swapping */
static int byte0 = 0;
static int byte1 = 1;
static int byte2 = 2;
static int byte3 = 3;
static int byte4 = 4;
static int byte5 = 5;
static int byte6 = 6;

/* vector addresses, absolute addrs of 68k vectors */
extern int dqs0_vec, dqs1_vec, dqs2_vec, dqs3_vec;
int *ke_vec[NDQS] = { &dqs0_vec, &dqs1_vec, &dqs2_vec, &dqs3_vec };
extern int start_of_text;

static unsigned char clkmod = 8;
static unsigned char speed1 = 134;
static unsigned char speed2 = 16;

#define IS_REL0(rel) ((rel)==0)
static char ke_rel;

static struct dqstr *snoopp[NDQS];

static struct dqbuffer tx_buffer;
static struct dqbuffer rx_buffer;

extern paddr_t rx_buffer_phys;	/* receive buffer for polling mode */
extern paddr_t tx_buffer_phys;	/* send buffer for polling */

/* from c.c */
extern struct dqregs *dqs_addr[NDQS]; /* base addrs 0x3fff220, 250, 280, 2b0 */
extern char *dqs_swp; /*DQSADDR+2*/
extern char *dqs_rel; /*DQSADDR+5*/

extern dqstout();
extern wakeup();
extern struct dqbuf *dqsget(), *dqxget();

dqsopen(dev)
{
	register struct dqdevice *dp;

	if (dev >= NDQS || fsword(dqs_addr[dev]) == -1 || 
		((dp = &dqsx[dev])->ocflag) != 0) {
		u.u_error = ENXIO;
		return;
	}

	dp->regs = (struct dqregs*)dqs_addr[dev];
	dp->xregs = (struct dqregs*)dqs_addr[dev];

	if (dp->regs->mtc < 0) {	/* bit15 set */
		u.u_error = ENXIO;		/* yes, no device */
		return;
	}

	/* get vector */
	dp->regs->mtvec = ke_vec[dev] - &start_of_text;
	*dqs_swp = 1;	/* set swap bit */
	
	keinit_line(dp->regs);	/* initialize unit */
	dp->sbufcur = 0;		/* clear receiver buffer ring */
	dp->reqseq = 0;			/* clear sequence number */
	dp->off34 = 0;
	dp->errflg = 0;			/* write DQSERR packet */
	dp->rrecbuf = 0;		/* clear ptr to read buffer */
	dp->off20 = 3;
	dp->rphase = RP_IDLE;	/* rphase: device available */
	dp->wphase = WP_IDLE;	/* wphase: device available */
	dp->ocflag = OC_ISOPEN;	/* mark device as open */
	dp->timeout = 0;		/* no delay, will run thread after 50ms/60ms */
							/* see below */
	dp->polling = 0;		/* switch off test mode */

	timeout(dqstout, dp, hz/3);	/* start timer thread 50ms/60Hz 60ms/50Hz */
}

dqsclose(dev)
{
	register struct dqdevice *dp = &dqsx[dev];

	dp->regs = (struct dqregs*)dqs_addr[dev];
	dp->iphase = ST_IDLE;
	
	/* reset default values, if speed was changed */
	if (dp->polling & 1) {
		clkmod = 8;
		speed1 = 134;
		speed2 = 16;
	}

	/* set flag close */
	dp->ocflag = OC_CLOSING;
	
	/* discard pending buffers */
	dqsfin(dp);
	dqxfin(dp);
	
	/* handle close ion exit */
	if (!IS_REL0(ke_rel) && dp->closerew) {
		dp->closerew = 0;
		
		/* rewind */
        dp->regs->mtc = CMD_OFFREW|MTC_GO;
		dqsdelay(1);
		
		/* set write with extended interrecord gap */
		dp->regs->mtc = MFU_WREXT|MTC_GO;
		dqsdelay(1);
	}
}

dqsdelay(n)
{
	timeout(wakeup, (caddr_t)dqsdelay, n * hz);
	sleep((caddr_t)dqsdelay, PWAIT);
}

dqsread(dev)
{
	register struct dqdevice *dp = &dqsx[dev];
	register struct dqbuf *bp;
	register cnt;

	dp->regs = (struct dqregs*)dqs_addr[dev];

	/* is phase != IDLE */
	spltty();
	  while (dp->rphase < RP_IDLE)
		  sleep((caddr_t)dp, PWAIT);	/* is busy, so sleep */
	spl0();

	/* is it really a read? yes, intitialize for read
	 * otherweise we come here to position for write */
	
	if (dp->wphase == WP_IDLE) {		/* unit is not writing? */
		dp->sbufcur = 0;				/* no current buffer to read */
		dp->curctrl = MC_START;			/* start unit */
        dp->retries = 7;				/* set retries to start unit */
		dp->rphase++;					/* goto read phase RP_1 */
		dp->iphase = ST_READ1;			/* interrupt phase READ1 */
		keget_ctrl(dp->regs);			/* initiate CMD_READ */
		dp->timeout = 20;				/* wait 1,6sec */
		dp->rrecbuf = 0;				/* clear current read buffer ptr */
	}

	/* no work buffer yet? */
	if (dp->rrecbuf == 0) {
		dp->rrecoff = 0;	/* offset = 0 */
		spltty();
		while ((cnt = dp->rphase) > 0) {	/* read in progress */
			/* obtain a buffer */
			if ((dp->rrecbuf = dqsget(1, dp)) != 0)
				break;

			/* didn't get a free buffer */
			if (cnt == RP_2 && dp->sbuflist==0) {
				dqsbeg(dp);	/* initialize the receiver list */
				spltty();
				if (dp->iphase == ST_STATE5)	
					dp->timeout = 1;	/* wait 83ms */
				continue;
			} else if (cnt >= RP_3) {
				spl0();
				dqsfin(dp);			/* discard packets */
				return;
			}
			dqsleep(dp); /* wait until buffer available */
		}
		spl0();

		
		if (dp->rphase <= RP_IDLE)		/* no more read in progress? */
			return;						/* exit */
	}

	/* got a buffer with received data */
	bp = dp->rrecbuf;
	cnt = min(bp->rsize, u.u_count);	/* take minimum of size vs request */
	/* align to even number, if possible */
	if ((cnt & 1) && cnt < u.u_count) {	
		cnt++;
		u.u_count++;
	}

	/* swap data for release 0 */
	if (IS_REL0(ke_rel))
		swab(bp->rxbuf->b_un.b_addr, cnt);

	/* transfer data to receiving process */
	iomove(&bp->rxbuf->b_un.b_addr[dp->rrecoff], cnt, 1);
	dp->rrecoff += cnt;	/* advance offset */
	if ((bp->rsize -= cnt) <= 0) {		/* all data in this buffer done? */
		bp->rstate = BF_EMPTY;			/* yes, mark empty */
		dp->rrecbuf = 0;				/* release this buffer */
		spltty();
		  if (dp->iphase == ST_STATE5)
			  dp->timeout = 1;			/* wait 83ms and schedule dqsintr */
		spl0();
	}
}

/* called by the OS to write something to tape */
dqswrite(dev)
{
	register struct dqdevice *dp = &dqsx[dev];
	register struct dqbuf *bp;
	register tmp;

	spltty();

	dp->regs = (struct dqregs*)dqs_addr[dev];

	/* BUG: who sets wphase > 0? should never block */
	while (dp->wphase > 0)						/* are we in seek? */
		sleep((caddr_t)dp, PWAIT);				/* yes, sleep */
		
	/* OS wants to write */

	/* nothing more to write, but possibly still finishing */
	if (u.u_count == 0) {

		/* are we in write phases WP1 to WP4 ? */
		while ((tmp = dp->wphase) < WP_IDLE && tmp > WP_5) { /* yes */
			if (tmp == WP_2) {					/* are we in phase WP2 ? */
				dp->wphase--; 					/* advance to WP3 */
				if (dp->iphase == ST_WRITE3)	/* at end of write? */
					dp->timeout = 1;			/* wait 83ms */
			}
			dqxleep(dp);						/* sleep until done */
		}
	} else {
		spl0();
		
		/* there is data to write, and the unit is available */
		if (dp->wphase == WP_IDLE) {
			dp->sbufcur = 0;					/* discard any read buffers */
			dp->curctrl = MC_START;				/* ensure a timeout will result in error */
			dp->retries = 3;					/* retries to start unit */
			dp->wphase--; 						/* start write phase WP_1 */
			dp->iphase = ST_WRITE1;				/* goto WRITE1 state */
			dqspoke(MC_SEEK, dp);				/* issue cmd and wait 1,5sec */
		}

		spltty();

		/* while in WP1 to WP4 */
		while ((tmp = dp->wphase) < WP_IDLE && tmp > WP_5) {
			bp = dqxget(0, dp);					/* obtain a free buffer */
			if (bp != 0) break;					/* got one, break */
			if (tmp == WP_2 && dp->xbuflist == 0) { /* WP2, and no buf */
				dqxbeg(dp);						/* reinit buf ring */
				spltty();						
			} else
				dqxleep(dp);					/* wait until buf ready */
			/* loop: retry getting a free buffer */
		}
	}
	
	/* have a buffer for data, but are we in error states WP5/WP6? */
	spl0();
	if ((tmp = dp->wphase) <= WP_5) {			/* error phases WP5..WP6 ? */
		if (tmp == WP_5)						/* exactly WP5? */
			u.u_error = EIO;					/* error: this is data overrun */
		dqxfin(dp);								/* phase WP5 or WP6 */
		return;									/* flush buffers, and exit */
	}

	/* we are not in error yet */
	if (dp->wphase < WP_IDLE) {
		bp->wsize = tmp = min(512, u.u_count);	/* write at most 512 bytes */
		if (tmp & 1) {							/* if odd count, make even */
			tmp++;
			u.u_count++;
		}

		/* transfer this data into the tx buffer */
		iomove(bp->txbuf->b_un.b_addr, tmp, 0);
		
		/* for REL0 of KEDQS, swap bytes */
		if (IS_REL0(ke_rel))
			swab(bp->txbuf->b_un.b_addr, tmp);
		
		/* lock buffer - this needs to be written */
		bp->wstate = BF_DATA; 	/* mark as contains data */
		spltty();
		  /* if finishing write, wait 83ms */
		  if (dp->iphase == ST_WRITE3)
			  dp->timeout = 1;	/* wait 83 ms */
		spl0();
	}
}

/* this is the combined hardware interrupt as well as the
 * timeout handler, triggered by dqstout when some wait delay expired
 * without a hardware interrupt (ST_TOxxxxx states */
dqsintr(dev)
{
	register struct dqdevice *dp = &dqsx[dev];
	register struct dqbuf *bp;
	register struct dqstr *dsp;
	register short d7;
	short error;
	char* bufp;

	/* get status block */
	dsp = &dqstr[dqstr[0].idx];
	
	/* ignore state machine in test mode */
	if (dp->polling & 1)
		return;

	/* were we in polling mode */
	if ((dp->polling & 2) && (dsp->keregs.mtc & MTC_CURDY))
		clrcache();

	/* clear poll flag */
	dp->polling &= ~2;
	
	/* fill status */
	dsp->dev = dev;						/* write device# */
	getke_reg(dp->regs, &dsp->keregs);	/* write current register state */
	snoopp[dev] = dsp;					/* put last status */
	dsp->wcnt = 0;						/* # of bytes written */
	dsp->iphase = dp->iphase;			/* interrupt processing phase */
	dsp->rphase = dp->rphase;			/* read processing phase */
	dsp->wphase = dp->wphase;			/* write processing phase */
	dsp->time = time;					/* write time tick (low byte) */
	error = dsp->keregs.mts & (MTS_ILL|MTS_13|MTS_HE|MTS_BGL|MTS_EOT|MTS_RLE|MTS_8);

										/* save error status bits status bits */

	/* obtain last ctrl issued */
	if (dsp->keregs.mts & MTS_TUR)		/* unit stopped? */
		dp->lastctrl = dsp->keregs.mtctrl;	/* read last ctrl cmd */
	else
		dp->lastctrl = 0;

	/* we arrive here either if the device interrupted us
	 * or dqstout triggered us with a timeout (ST_TOxxxx states */

	dp->timeout = 0; 					/* kill timer - don't retrigger */
	switch (dp->iphase) {
	case ST_READ1:						/* arrive here after dqsread started */
        d7 = dsp->keregs.mtctrl;		/* last ctrl word */
		if (error)						/* error status? */
			d7 = 0;						/* ignore cmd */
		if (d7 == MC_SEEK) {
			if (dp->rphase == RP_1) {	/* RP_1 phase */
				dp->rphase++;			/* goto RP_2 phase */
				dqswake(dp);			/* wakeup receiver process */
				dp->retries = 7;
			}
			goto lbl_920;				/* go get data */
		} else {
			dp->iphase = ST_STATE2;		/* go into seek mode */
			goto lbl_966;
		}

	case ST_STATE2:
	case ST_TOSTATE2:
        dp->iphase = ST_READ1;
		keget_ctrl(dp->regs);			/* initiate CMD_READ */
        dp->timeout = 25;				/* wait 2sec */
		break;

	case ST_STATE3:
	case ST_TOSTATE3:
        dp->iphase = ST_STATE4;
        dp->polling |= 2;
		if (dp->sbufcur == 0) {
			/* read 8 bytes record header */
			keget_daten(dp->regs, logtophys(dp->rechdr), -8);
			dsp->wcnt = 8;
			dp->timeout = 25;			/* wait 2sec */
			break;
		} else {
			/* receive a data block */
			keget_daten(dp->regs, dp->sbufcur->rxaddr, -512);
			dsp->wcnt = 512;
			dp->timeout = 50;			/* wait 4sec */
			break;
		}

	case ST_STATE4:
        dp->iphase = ST_STATE3;
		if (error != 0)
			goto lbl_966;

		if (dsp->keregs.mts & 1) {
			if ((d7 = dsp->keregs.mtctrl) == MC_START)
				goto lbl_920;
			else if (d7==0x3d)
				goto tapeerror;
			else
				goto lbl_966;
		} else if (dp->sbufcur == 0) {
			if ((short)(dp->rechdr[byte0] & 0xf0) == 0xa0) {
				dp->off34 = dp->rechdr[byte0] & 15;
				if (dp->rechdr[byte1] & 64)
					dp->off36 = 1;
				goto lbl_920;
			} else if ( (short)((dp->rechdr[byte0] & 0xf0)==0x80) == 0) 
				goto lbl_966;
			if (dp->rechdr[byte3] == 0) {
		        dp->off32 = dp->rechdr[byte0] & 15;
				if (dp->rechdr[byte1] & 64)
					dp->off36 = 1;
				if (dp->off32 == dp->off34) {
					dp->off34++;
					dp->off34 &= 15;
					goto lbl_920;
				}
			}
			goto lbl_966;
		}

        dp->off37 = 0;
		bp = dp->sbufcur;
		bufp = bp->rxbuf->b_un.b_addr;
		if (bufp[byte1] & 64)
			dp->off36 = 1;

		d7 = (unsigned char)bufp[byte0];
		if ((short)(d7 & 0xf0) != 0x80) { 

			if ((short)(d7 & 0xf0) == 0x90) {
				if ((unsigned char)bufp[byte3] == 0xe0) { 
lbl_868:
					bufp[byte0] = 2;
					goto lbl_8f8;
				} else
lbl_878:
					goto lbl_8ea;
			} else if ((short)(d7 & 0xf0) == 0xa0) { 
lbl_88a:
				dp->off34 = d7 & 15;
				goto lbl_8ea;
			} else {
lbl_89a:
				bufp[byte0] = 2;
				goto lbl_8f8;
			}
		} else {
			dp->off32 = d7 & 15;
			if (dp->off32 != dp->off34) {
				dp->errflg = 1;
				bufp[byte0] = 2;
				goto lbl_8f8;
			} else {
				dp->errflg = 0;
				dp->off34++;
				dp->off34 &= 15;
			}
		}
lbl_8ea:
		bufp[byte0] = 0;

lbl_8f8:
        bp->rsize = dsp->keregs.mtbrc + 512;
        bp->rstate = BF_DATA;
        dp->sbufcur = 0;
		dqswake(dp);
		if (dp->errflg != 0)
			goto tapeerror;

lbl_920:
        dp->off37 = 0;
		if (--dp->retries < 0)
				goto retryfailed;
		
	case ST_STATE5:
	case ST_TOSTATE5:
		if (dp->sbufcur == 0) {
			if ((dp->sbufcur = dqsget(0, dp))==0 && dp->iphase != ST_TOSTATE5) {
				dp->iphase = ST_STATE5;
				dp->timeout = 2;		/* wait 160ms */
				break;
			}
		}
		dp->iphase = ST_WRITE3;
		dp->retries = 7;
		goto writedone;

lbl_966:
		if (--dp->retries >= 0) {
			dqspoke(0x3d, dp);
			break;
		}
		/*FALLTHRU*/
	case ST_TOREAD1:
	case ST_TOSTATE4:
retryfailed:
		goto finalfailure;

/********************************************************/
/*            writer part of int processor              */
/********************************************************/

	case ST_WRITE1:						/* arrive here after dqswrite started */
	case ST_TOWRITE1:					/* arrive here after timeout */
										/* happens if MC_SEEK did not reply 
										 * is not an error: tape might be empty */

        dp->iphase = ST_WRITE2;			/* assume we have started tape */
		keget_ctrl(dp->regs);			/* initiate CMD_READ (find header) */
        dp->timeout = 25;				/* wait 2sec */
		break;

	case ST_TOWRITE2:					/* arrive here if status read did not reply */
        error += MTS_TUR;				/* mark tape stop */
		/*FALLTHRU*/

	case ST_WRITE2:						/* arrive here by IRQ that header seen */
		if (error != 0) {				/* if error */
			d7 = 0;						/* op = 0 */
			goto tapeerror;				/* handle error */
		} else if ((d7 = dp->lastctrl) != dp->curctrl)
			goto tapeerror;				/* different ctrl than issued
										 * = error condition (overrun?) */
		else if (dp->wphase == WP_1)	/* advance sub phase */
			dp->wphase--; 				/* become WP_2 */

        dp->rphase = RP_2;				/* set rphase = 2 */
		dqswake(dp);					/* wakeup readers */

writedone:
		bp = dp->xbufcur;				/* get current write buffer */
		if (bp != 0) {					/* stuff was sent */
			bp->wstate = BF_EMPTY; 		/* mark buffer as empty */
			dp->xbufcur = 0;			/* discard it */
		}

		dqxwake(dp);					/* wakeup writers */
        dp->retries = 7;				/* setup retries write */
        dp->off20 = 7;
		/*FALLTHRU*/

	case ST_WRITE3:
	case ST_TOWRITE3:
		if (dp->off36) {
			dp->off36 = 0;
			dqsubr(dp);
			break;
		}

		/* current buffer sent and no further buffer to send? */
		if (dp->xbufcur==0 && (dp->xbufcur = dqxget(1, dp))==0) {
			if (dp->wphase == WP_3)		/* were we in error state? */
				goto finalfailure;		/* yes, exit */
			if (dp->iphase == ST_TOWRITE3) {	/* got timeout? */
				dp->off20 = 7;
				dqsubr(dp);
				break;
			}
			dp->iphase = ST_WRITE3;		/* stay in streaming phase */
			dp->timeout = 2;			/* wait 160ms */
			break;
		}

		/* there is more to write */
        bp = dp->xbufcur;
        dp->iphase = ST_STATE3;			/* stay in state 3 */
		bufp = bp->txbuf->b_un.b_addr;	/* get buffer status */
		if ((bufp[byte0] & 0xf0) == 0x80)	/* busy? */
			bufp[byte0] += dp->reqseq;	/* increment sequence number for record */

		if (dp->sbufcur==0)				/* no receive buf? */
			bufp[byte1] |= 64;			/* set bit 6 */

writebuffer:
		/* write a record of bp->wsize bytes */
		kesend_daten(dp->regs, bp->txaddr, (short)-bp->wsize);
        dsp->wcnt = bp->wsize;			/* set # bytes written into status */
        incseq();						/* advance sequence number */
        dp->off37 = 1;
        dp->timeout = 25;				/* wait 2sec */
		break;


/* arrive here in case of error */
tapeerror:
        if (--dp->retries < 0)			/* retry in case of error? */
			goto retryfailed;			/* no, too many retries */

		if (--dp->off20 >= 0) {			/* seek retries? */
			if (dp->wphase != WP_1) {	/* didn't we find header? */
				dp->iphase = ST_STATE3;	/* go to STATE3 */
				if (dp->errflg != 0) {	/* write error record? */
					dp->errflg = 0;		/* reset error flag */
					dqserr(dp);			/* write an error header 0x9N */
					break;
				}
				if (d7 == 0x3d) {		/* lastctrl was 0x3d? */
					if (dp->off37 != 0) {
						decseq();		
						bp = dp->xbufcur;
						goto writebuffer;
					}
					dqspoke(MC_START, dp);
					break;
				}
				dqspoke(0x3d, dp);
				break;
			}
			dp->iphase = ST_WRITE1;
			dqspoke(MC_SEEK, dp);
			break;
		}

		if (dp->wphase != WP_1)
			dp->wphase = WP_4;

finalfailure:
		dp->wphase = (dp->wphase > WP_4) ? WP_6 : WP_5;
        dp->iphase = ST_IDLE;
        dqxwake(dp);
		dp->rphase = RP_3;
		dqswake(dp);
		break;
	}

	/* advance to next status buffer */
	if (++dqstr[0].idx >= 4)
		dqstr[0].idx = 0;
}

/* timer thread */
dqstout(dp)
register struct dqdevice *dp;
{
	if (dp->ocflag != OC_ISOPEN) {	/* if not open, close explicitly */
		dp->ocflag = 0;
		return;						/* and do not restart */
	}
	
	/* device is open and timer is called */

	/* do we have to wait for something? */
	if (dp->timeout > 0) {
		
		/* yes, decrement timeout counter */
		if (--dp->timeout == 0) {
			if (dp->polling & 1)		/* in non-interrupt mode? */
				return;					/* yes, exit */
			
			dp->iphase += ST_TOSTATES;	/* set state after timeout */
			dqsintr(dp - dqsx);			/* call int processor for this device */
		}
	}
	
	/* restart timer thread, in contrast to open, it now ticks at 83ms */
	timeout(dqstout, dp, hz/5); 		/* 1/12sec = 83ms */
}

dqserr(dp)
register struct dqdevice *dp;
{
	register char *bp = dp->rechdr;
	
	/* write an error record 0x90+N 0xcf/0x8f 0xcf 0xe0 0x80+M 0x00 0x00 */
	bp[byte0] = dp->off32 | 0x90;
	if (dp->sbufcur == 0)
		bp[byte1] = 0xcf;
	else
		bp[byte1] = 0x8f;

	bp[byte2] = 0xcf;
	bp[byte3] = 0xe0;
	bp[byte4] = dp->off34 | 0x80;
	bp[byte5] = 0;
	bp[byte6] = 0;
	kesend_daten(dp->regs, logtophys(dp->rechdr), -7);
	snoopp[dp - dqsx]->wcnt = 7;	/* 7 bytes written */
	dp->timeout = 15;				/* wait 1.25sec */
}

dqsubr(dp)
register struct dqdevice *dp;
{
	register char *bp;

	dp->iphase = ST_STATE3;
	if (dp->sbufcur) {
		dqspoke(MC_START, dp);
		return;
	}

	/* write a record marker 0x80+N 0xcf 0xcf 0x00 0x00 */
	bp = dp->rechdr;
	bp[byte0] = dp->reqseq | 0x80;
	bp[byte1] = 0xcf;
	bp[byte2] = 0xcf;
	bp[byte3] = 0;
	bp[byte4] = 0;
	kesend_daten(dp->regs, logtophys(dp->rechdr), -5);
	snoopp[dp - dqsx]->wcnt = 5;	/* 5 bytes written */
	incseq();						/* advance sequence number, modulo 16 */
	dp->timeout = 15;				/* wait 1.25 sec */
}

/* send control data word */
dqspoke(data, dp)
short data;
register struct dqdevice *dp;
{
	register struct dqregs *regs = dp->regs;
	regs->mtctrl = data;
	regs->mtbrc = 0;
	regs->mtc = CMD_CTRL|MTC_GO;
	dp->timeout = 15;				/* wait 1.25 sec */
}

/* send reader to sleep if no data received */
dqsleep(dp)
register struct dqdevice *dp;
{
	dp->rwait = 1;
	sleep((caddr_t)dp->regs, PWAIT);
}

/* send writer to sleep if no free write buffer */
dqxleep(dp)
register struct dqdevice *dp;
{
	dp->wwait = 1;
	sleep((caddr_t)dp->xregs, PWAIT);
}

/* wakeup readers waiting for received data */
dqswake(dp)
register struct dqdevice *dp;
{
	if (dp->rwait)
		wakeup((caddr_t)dp->regs);
	dp->rwait = 0;
}

/* wake up writers waiting for a free send buffer */
dqxwake(dp)
register struct dqdevice *dp;
{
	if (dp->wwait)
		wakeup((caddr_t)dp->xregs);
	dp->wwait = 0;
}

dqsbeg(dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp;
	register i;

	dp->sbuflist = (struct dqbuf*)dp;	/* mark end of list */
	bp = &dp->sbufs[3];	/* point to last buffer */
	
	/* connect buffers as a ring */
	for (i = 0; i < 4; i++) {	/* foreach buffer */
		bp = bp->rxnext = &dp->sbufs[i];	/* link to next */
		bp->rstate = BF_EMPTY;	/* mark empty */
		bp->rxbuf = geteblk();				/* obtain a buffer */
		bp->rxaddr = bp->rxbuf->b_paddr;	/* save physical addr */
	}
	dp->sbuflist = bp;	/* head of ring */
}

dqxbeg(dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp;
	register i;

	dp->xbuflist = (struct dqbuf*)dp;	/* mark end of list */
	bp = &dp->xbufs[3];					/* point to last */
	
	/* connect buffers as a ring */
	for (i = 0; i < 4; i++) {	/* foreach buffer */
		bp = bp->txnext = &dp->xbufs[i];	/* link to next */
		bp->wstate = BF_EMPTY;	/* mark empty */
		bp->txbuf = geteblk();				/* obtain a buffer */
		bp->txaddr = bp->txbuf->b_paddr;	/* save physical addr */
	}
	dp->xbuflist = bp;	/* head of ring */
}

/* obtain a receive buffer
/* if flag = 1, get the first one that was received */
struct dqbuf *dqsget(flag, dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp = dp->sbuflist;

	/* is there something in buffer list ? */
	if ((caddr_t)bp <= (caddr_t)dp)
		return 0;	/*no*/

	/* obtain the first available one */
	if (flag) {
		if (bp->rstate != BF_DATA) /* does it contain data? */
			return 0;	/*no*/
		dp->sbuflist = bp->rxnext; /*advance to the next*/
	} else {
		/* find a free buffer */
		while (bp->rstate != BF_EMPTY) {
			bp = bp->rxnext;
			if (bp == dp->sbuflist) /* at start of ring? */
				return 0;	/* yes, none available */
		}
	}

	bp->rstate = BF_BUSY;	/* mark buffer busy */
	return bp;	/* return it */
}

/* obtain a receive buffer
/* if flag = 1, get the first one that contains data */
struct dqbuf *dqxget(flag, dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp = dp->xbuflist;

	/* somthing in buffer list? */
	if ((caddr_t)bp <= (caddr_t)dp)
		return 0;	/*no*/

	/* obtain the first one with data */
	if (flag) {
		if (bp->wstate != BF_DATA)	/* has one? */
			return 0;	/*no*/
		dp->xbuflist = bp->txnext;	/*advance to next one */
	} else {
		/* find a free buffer */
		while (bp->wstate != BF_EMPTY) {
			bp = bp->txnext;
			if (bp == dp->xbuflist)	/*again at start of ring? */
				return 0;	/*yes, none to send */
		}
	}
	bp->wstate = BF_BUSY;	/* mark as busy */
	return bp;	/* return it */
}

/* flush receiver buffer overflow */
dqsfin(dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp = dp->sbuflist;
	
	/* nothing to process ?*/
	if ((caddr_t)bp == (caddr_t)dp)		/* no, exit */
		return;

	if (bp) {							/* got a buffer */
		do {
			brelse(bp->rxbuf);			/* release buffer */
			bp = bp->rxnext;			/* advance to next */
		} while (bp != dp->sbuflist);	/* until end */
	}
	dp->sbuflist = 0;					/* clear buffer list */
	dp->rphase = RP_IDLE;				/* disable reader */
	wakeup((caddr_t)dp);				/* wakeup waiting readers */
}

/* flush xmit buffer overflow */
dqxfin(dp)
register struct dqdevice *dp;
{
	register struct dqbuf *bp = dp->xbuflist;

	if ((caddr_t)bp == (caddr_t)dp)
		return;

	if (bp) {
		do {
			brelse(bp->txbuf);
			bp = bp->txnext;
		} while (bp != dp->xbuflist);
	}
	dp->xbuflist = 0;
	dp->wphase = WP_IDLE;
	wakeup((caddr_t)dp);
}

dqsioctl(dev, func, arg)
dev_t dev;
caddr_t arg;
{
	register struct dqdevice *dp = &dqsx[dev];
	int s = currpl();

	/* if not initialized, don't allow test modes */
	if (!(dp->polling & 1) && func > 77 && func < 83) {
		u.u_error = EINVAL;
		return;
	}
	
	spltty();
	switch (func) {

	/* disable rewind on close */
	case 75:
        dp->closerew = 0;
		break;

	/* enable rewind on close */
	case 76:
		dp->closerew = 1;
		break;

	/* initialize for test modes
	 * set speed, vector, init line, buffers */
	case 77:
		dp->polling = 1;
		
		/* read ioctl data, only 2 shorts are relevant here */
		copyin(arg, &tx_buffer, 26);
		tx_buffer_phys = logtophys(tx_buffer.u.b);
		rx_buffer_phys = logtophys(rx_buffer.u.b);
		clkmod = tx_buffer.u.s.clkmod;
		if (!IS_REL0(ke_rel))
			speed2 = tx_buffer.u.s.speed;
		else
			speed1 = tx_buffer.u.s.speed;

		/* set vector */
		dp->regs->mtvec = ke_vec[dev] - &start_of_text;
		keinit_line(dp->regs);
		
		/* read data */
		testget_daten(dp->regs);
		break;

	/* test mode: set mtctrl from payload (u.mtctrl)
	 * then read data into buffer u.b */
	case 78:
		/* read in register set, sz of buffer and mtctrl */
		copyin(arg, &tx_buffer, sizeof(struct keregs)+2*sizeof(short));
		
		/* set ctrl */
		testsend_ctrl(dp->regs);
		
		/* read data int rc_buffer */
		testget_daten(dp->regs);
		break;

	/* test mode: send data, if not failed, get data */
	case 79:
		/* get complete send buffer */
		copyin(arg, &tx_buffer, sizeof(struct dqbuffer));

		/* try to send */
		if (testsend_daten(dp->regs)==0)
			break;					/* failed */
		/* read into rx_buffer */
		testget_daten(dp->regs);
		break;

	/* test mode: return received data to user */
	case 80:
		/* copy current registers into rx */
		getke_reg(dp->regs, &rx_buffer);
		
		/* was last cmd a successful read? */
		if ((rx_buffer.keregs.mtc & (MTC_CURDY|MTC_FUMASK)) == (MTC_CURDY|MFU_READ)) {
			clrcache();	/* yes, clear cache */
			if (IS_REL0(ke_rel))	/* swap byte in rx_buffer, if REL0 */
				swab(rx_buffer.u.b, rx_buffer.keregs.mtbrc+512);
		}
		/* pass buffer back to user */
		copyout(&rx_buffer, arg, sizeof(struct dqbuffer));
		break;

	/* test mode: execute WRITE/WREOF command from buffer, cmd = mtc */
	case 81:
		/* copy buffer from user */
		copyin(arg, &tx_buffer, sizeof(struct dqbuffer));

		/* execute command */
		testke_cmd(dp->regs, &tx_buffer);
		break;

	/* test mode: execute READ cmd from buffer */
	case 82:
		/* get registers from user */
		copyin(arg, &rx_buffer, sizeof(struct keregs)+sizeof(short));
		
		/* execute command */
		testke_cmd(dp->regs, &rx_buffer);
		break;

	/* test mode: get HW RAM */
	case 83:
		/* copy HW RAM into buffer, 128 words */
        testget_HW_RAM(rx_buffer.u.b);
		
		/* transfer data back to user */
		copyout(&rx_buffer, arg, sizeof(struct keregs)+sizeof(short)+128*sizeof(short));
		break;
	
	default:
        u.u_error = EINVAL;
		break;
	}
	splx(s);
}

static testget_HW_RAM(buf)
register short *buf;
{
	
	register short *regs = (short*)dqs_addr[0];
	register i;

	regs = &regs[-16];

	for (i = 0; i < 128; i++) {
		*buf++ = *regs++;
	}
}

static waitms(dly)
register dly;
{
	register i;

	do {
		i=2075;
		while (--i >0);
	} while (--dly > 0);
}

/* execute a formatter command from a given buffer */
testke_cmd(regs, buf)
register struct dqregs *regs;
register struct dqbuffer *buf;
{
	register caddr_t bp;
	register i;

	bp = 0;
	
	i = buf->keregs.mtc;
	i &= ~MTC_ADMASK;	/* drop the address bits 18-17 */
	if ((buf->keregs.mtbrc+512) < 0) {
		u.u_error = EINVAL;
		return 0;
	}

	/* allow READ, WRITE, WRITEEOF only, select appropriate buffer */
	switch (i & MTC_FUMASK) {
	case MFU_READ:
		bp = (caddr_t)rx_buffer_phys;
		break;

	case MFU_WRITE:
	case MFU_WREOF:
		bp = (caddr_t)tx_buffer_phys;
		break;

	case MFU_SFWD:
	case MFU_SREV:
        u.u_error = EINVAL;
		return 0;
	}
	
	regs->mtbrc = buf->keregs.mtbrc;
	regs->mtctrl = buf->keregs.mtctrl;
	regs->mtcma = (short)bp;
	
	if (!IS_REL0(ke_rel)) {
		SET_ADDRH_V1(regs, bp, i);
	} else {
		SET_ADDRH_V0(regs, bp, i);
	}
	return 1;
}

/* wait some time for CU becoming ready, then read data (non-interrupt) */
static testget_daten(regs)
register struct dqregs *regs;
{
	register i = 4000;

	while (--i != 0 && (regs->mtc & MTC_CURDY)==0)
		waitms(1);

	testget(regs);
}

/* do a non-interrupt read of buffer */
testget(regs)
register struct dqregs *regs;
{
	regs->mtbrc = -512;
	regs->mtcma = rx_buffer_phys;
	if (IS_REL0(ke_rel)) {
		SET_ADDRH_V0(regs, rx_buffer_phys, CMD_TSTREAD);
	} else {
		SET_ADDRH_V1(regs, rx_buffer_phys, CMD_TSTREAD);
	}
}

/* set mtctrl from tx_buffer ioctl, then execute a non-interrupt ctrl cmd */
static testsend_ctrl(regs)
register struct dqregs *regs;
{
	regs->mtctrl = tx_buffer.u.mtctrl;
	regs->mtbrc = 0;
	regs->mtc = CMD_TSTCTRL|MTC_GO;
}

/* write non-interrupt data from tx_buffer */
static testsend_daten(regs)
register struct dqregs *regs;
{
	register struct dqbuffer *bp = &tx_buffer; 	/* payload address */
	register uint cnt = bp->sz;				   	/* size of payload */

	if (cnt > 512) {							/* too large? */
		u.u_error = EINVAL;
		return 0;
	}

	if (IS_REL0(ke_rel))
		swab(tx_buffer.u.b, cnt + (cnt & 1));		/* REL0 swap bytes */

	regs->mtbrc = -cnt;							/* set bytes to send */
	regs->mtcma = tx_buffer_phys;				/* set transfer addr */
	if (IS_REL0(ke_rel)) {						/* also high address bits */
		SET_ADDRH_V0(regs, tx_buffer_phys, CMD_TSTWRIT);
	} else {
		SET_ADDRH_V1(regs, tx_buffer_phys, CMD_TSTWRIT);
	}											/* execute write */
	return 1;
}

/* initialize the formatter unit */
keinit_line(regs)
register struct dqregs *regs;
{
	/* release 0:
	 * - swaps bytes
	 * - uses speed1
	 * - 18bit address
	 */
	if (IS_REL0(ke_rel = *dqs_rel)) {
		regs->mtspd = speed1;
		byte0 = 1;	byte1 = 0;
		byte2 = 3;	byte3 = 2;
		byte4 = 5;	byte5 = 4;
		byte6 = 7;
	} else {
	/* release 1:
	 * - no byte swap 
	 * - uses speed 1 and clkmod
	 * - 22 bit address
	 */
		regs->mtspd = speed2;
		regs->mtclk = clkmod;
	}
	
	/* force rewind */
	regs->mtc = CMD_OFFREW;		 
	regs->mtc = CMD_OFFREW|MTC_GO;
}

/* execute a read */
static keget_ctrl(regs)
register struct dqregs *regs;
{
	regs->mtbrc = 0;				/* no bytes to transfer */
	regs->mtc = CMD_READ|MTC_GO;	/* do a 0-read */
}

/* read data from position */
static keget_daten(regs, addr, ncnt)
register struct dqregs *regs;
register addr;
short ncnt;
{
	regs->mtbrc = ncnt; 			/* # of bytes to read */
	regs->mtcma = addr;				/* address of read buffer */
	if (IS_REL0(ke_rel)) {			/* also high address bits */
		SET_ADDRH_V0(regs, addr, CMD_READ);
	} else {
		SET_ADDRH_V1(regs, addr, CMD_READ);
	}								/* execute a read command */
}

/* write data at position */
static kesend_daten(regs, addr, ncnt)
register struct dqregs *regs;
register addr;
{
	regs->mtbrc = ncnt;				/* # of bytes to write */
	regs->mtcma = addr;				/* address of write buffer */
	if (IS_REL0(ke_rel)) {			/* also high bits */
		SET_ADDRH_V0(regs, addr, CMD_WRITE);
	} else {
		SET_ADDRH_V1(regs, addr, CMD_WRITE);
	}								/* execute a write command */
}

/* get status into keregs at given address */
getke_reg(regs, kp)
register struct dqregs *regs;
register struct keregs *kp;
{
	kp->mtc = regs->mtc;
	kp->mts  = regs->mts;
	kp->mtbrc  = regs->mtbrc;
	kp->mtcma = regs->mtcma;
	kp->mtctrl  = regs->mtctrl;
	kp->mtcmah = regs->mtcmah;
}

