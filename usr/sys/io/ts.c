static char* _Version = "@(#) RELEASE:  1.2  Oct 16 1986 /usr/sys/io/ts.c ";

/* @(#)ts.c 6.1 */
/*
 * TS11 tape driver
 * Handles one TS11. Minor device classes:
 * bit 2 - off: rewind on close; on: position after first TM.
 */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"

#include "sys/buf.h"
#include "sys/dir.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/signal.h"
#include "sys/systm.h"
#include "sys/user.h"
#include "sys/mtio.h"
#include "fcntl.h"

struct device {
    short tsdb;
    short tssr;
};

/* Bits in status register tssr */
#define SC  0100000
#define SSR 0200
#define OFL 0100
#define NBA 02000

#define DEVADDR(dev) ((struct device*)0x3ffff550)+(dev&7)       /*7777772520*/
#define TSDEV(dev) (dev & 0x7f)
#define TSERROR     -1
#define TSOFFLINE   -2

/* status message */
struct  sts {
    short   s_sts;
    short   len;
    short   rbpcr;  /* residual frame count register */
    short   sx0;    /* extended status registers (0-3) */
    short   sx1;
    short   sx2;
    short   sx3;
    /*ushort tssr;*/    /* !PCS */
};

/* Error codes in sx0 */
#define TMK 0100000
#define RLS 040000
#define RLL 010000
#define WLE 04000
#define ONL 0100
#define EOT 01

/* command message */
struct cmd {
    short   cmd;
    short   loba;
    short   hiba;
    short   size;
};

/* command packet header word */
#define ACK 0100000
#define CVC 040000
#define SWB 010000
#define IE  0200

/* commands, also in command packet header word */
#define SETCHR  04
#define GSTAT   017
    /* data transfer */
#define READ    01
#define REREAD  01001
#define WRITE   05
#define REWRITE 01005
#define WTM 011
    /* positioning */
#define SFORW   010
#define SKIPF   01010
#define SREV    0410
#define REW 02010
    /* init */
#define TSINIT  013

#define NOP 0
#define TCC 0xe /* Termination Class Code mask */

/* PCS device bit to enforce NO REWIND, different from VAx */
#define NOREW   0x80

/* this is to ensure that cmd is 32-byte aligned */
short cmdpkt[72] = { 0, };
#define CMDBUF ((struct cmd*)(((int)&cmdpkt[64]) & 0x3fffffe0))

/* characteristics data */
struct charac {
    ushort  loba;
    ushort  hiba;
    ushort  size;
    ushort  mode;
} chrbuf;

struct sts mesbuf;

struct buf tstab;
struct buf ctsbuf;
struct buf rtsbuf;

/*PCS flags in b_active, uses bits whereas VAX uses ints */
#define SIDLE   0
#define SIO     1
#define SMOVE   2
#define SRETRY  8
#define SCOM    16
#define SMARK   32
#define SINIT   128

char ts_flags;
int ts_openf;
int ts_stray;
daddr_t ts_blkno;
int ts_spblk = 1;

long ts_nxrec;
int ts_swap;
ushort cbufaddr;
short mbufaddr;
short mbufaddrext;
short cxbufaddr;
short cxbufaddrext;

extern int tsbaemask;

tsopen(dev, flag)
{
    register struct device *rp;
    register i;
    caddr_t cmdaddr;
    daddr_t dma;
    
    rp  = DEVADDR(dev);
    if (TSDEV(dev) > 7 || ts_openf > 0 || fsword(rp) == -1) {
        u.u_error = ENXIO;
        return;
    }
    
    if (cbufaddr == 0) {
        cmdaddr = (caddr_t)CMDBUF;
        dma = map_18dma(cmdaddr, 8);
        cbufaddr = (dma & 0xffff);
        cbufaddr |= (dma>>16) & 3; /*tsdb is bit order A15...A2,A17-A16 */
        mbufaddr = logtophys(&mesbuf);
        mbufaddrext = logtophys(&mesbuf) >> 16;
        cxbufaddr = logtophys(&chrbuf);
        cxbufaddrext = logtophys(&chrbuf) >> 16;
    }
    
    if (tstab.b_active & SINIT) {
        for (i=0; i<125; i++) {
            if (rp->tssr & SSR) {
                i = 0;
                break;
            }
            delay(hz * 4);
        }
        if (i) {
            u.u_error = ENXIO;
            return;
        }
        tstab.b_active = SIDLE;
    }
    
    if (tsinit(rp, 0)) {
        u.u_error = ENXIO;
        return;
    }

    ts_blkno = 0;
    ts_nxrec = 1000000;
    ts_flags = 0;
    rtsbuf.b_flags |= B_DMA22;
    i = tscommand(dev, NOP);
    
    if (rp->tssr & OFL) {
        printf("Magtape %d offline\n", TSDEV(dev));
        u.u_error = ENXIO;
    }

    if ((flag & O_RDWR) && (i & 4)) {
        printf("Magtape %d write locked\n", TSDEV(dev));
        u.u_error = ENXIO;
    }
    
    if (u.u_error==0)
        ts_openf = 1;
    else
        ts_openf = 0;
}

tsclose(dev, flag)
{
    register struct device *rp = DEVADDR(dev);
    if (tstab.b_active & SINIT) {
        ts_openf = 0;
        return;
    }
    
    if ((flag & 2) && ts_openf != TSOFFLINE) {
        tscommand(dev, WTM);
        tscommand(dev, WTM);
    }
    
    if (dev & NOREW) {
        if ((flag & 2) && ts_openf != TSOFFLINE)
            tscommand(dev, SREV);
    } else
        tscommand(dev, REW);

    ts_openf = 0;
}

tscommand(dev, com)
{
    register struct buf *bp;
    
    bp = &ctsbuf;
    
    spldisk();
      while (bp->b_flags & B_BUSY) {
          bp->b_flags |= B_WANTED;
          sleep((caddr_t)bp, PRIBIO);
      }
    spl0();
    bp->b_dev = dev;
    bp->b_resid = com;
    bp->b_blkno = 0;
    bp->b_flags = B_BUSY|B_READ;
    tsstrategy(bp);
    iowait(bp);
    if (bp->b_flags & B_WANTED)
        wakeup((caddr_t)bp);
    if (bp->b_flags & B_ERROR)
        u.u_error = EIO;
    bp->b_flags = 0;
    return bp->b_resid;
}

tsstrategy(bp)
register struct buf *bp;
{
    register daddr_t *p;
    
    if (bp != &ctsbuf) {
        bp->b_flags |= B_DMA22;
        if (ts_openf < 0) {
            bp->b_flags |= B_ERROR;
            iodone(bp);
            return;
        }
        p = &ts_nxrec;
        if (FsPTOL(bp->b_dev, bp->b_blkno) > *p) {
            bp->b_flags |= B_ERROR;
            bp->b_error = ENXIO;
            iodone(bp);
            return;
        }
        if (FsPTOL(bp->b_dev, bp->b_blkno) == *p && bp->b_flags & B_READ) {
            clrbuf(bp);
            bp->b_resid = bp->b_bcount;
            iodone(bp);
            return;
        }
        if ((bp->b_flags & B_READ)==0)
            *p = FsPTOL(bp->b_dev, bp->b_blkno) + 1;
    }
    
    bp->av_forw = 0;
    spldisk();
    if (tstab.b_actf == 0)
        tstab.b_actf = bp;
    else
        tstab.b_actl->av_forw = bp;
    tstab.b_actl = bp;
    if (tstab.b_active == SIDLE)
        tsstart();
    spl0();
}

tsstart()
{
    register struct buf *bp;
    register struct device *rp;
    register daddr_t blkno;
    int unused;

loop:
    
    if ((bp = tstab.b_actf) == 0)
        return;
    rp = DEVADDR(bp->b_dev);
    blkno = ts_blkno;
    if (rp->tssr & OFL) {
        printf("\nMagtape %x offline\n", TSDEV(bp->b_dev));
        ts_openf = TSOFFLINE;
        bp->b_flags |= B_ERROR;
    } else {
        if (bp == &ctsbuf) {
            if (bp->b_resid==0) {
                bp->b_resid = mesbuf.sx0;
                goto next;
            } else {
                tstab.b_active = SCOM;
                CMDBUF->loba = ts_spblk;
                CMDBUF->size = 1;
                CMDBUF->cmd = bp->b_resid | ACK | IE;
                rp->tsdb = cbufaddr;
                return;
            }
        } else if (ts_openf < 0) {
            bp->b_error = ENXIO;
            bp->b_flags |= 4;
            goto next;
        } else if (FsPTOL(bp->b_dev, bp->b_blkno) > ts_nxrec) {
            bp->b_flags |= B_ERROR;
            goto next;
        } else if (blkno != FsPTOL(bp->b_dev, bp->b_blkno)) {
            tstab.b_active = SMOVE;
            if (blkno < FsPTOL(bp->b_dev, bp->b_blkno)) {
                CMDBUF->cmd = SFORW|ACK|IE;
                CMDBUF->loba = FsPTOL(bp->b_dev, bp->b_blkno) - blkno;
            } else {
                CMDBUF->cmd = SREV|ACK|IE;
                CMDBUF->loba = blkno - FsPTOL(bp->b_dev, bp->b_blkno);
            }
            rp->tsdb = cbufaddr;
            return;
        }
        tstab.b_active = SIO;
        CMDBUF->loba = bp->b_paddr;
        CMDBUF->hiba = (bp->b_paddr >> 16) | tsbaemask;
        CMDBUF->size = bp->b_bcount;
        CMDBUF->cmd = ts_swap | ACK|IE;
        if (bp->b_flags & B_READ)
            CMDBUF->cmd |= READ;
        else
            CMDBUF->cmd |= WRITE;
        rp->tsdb = cbufaddr;
        return;
    }

next:
    tstab.b_actf = bp->av_forw;
    iodone(bp);
    goto loop;
}

tsintr()
{
    register struct buf *bp;
    register struct device *rp;
    register state;

    if ((bp=tstab.b_actf) == 0) {
        ts_stray++;
        return;
    }
    
    rp = DEVADDR(bp->b_dev);
    state = tstab.b_active;
    
    if ((rp->tssr & SC) || (state == SIO && (mesbuf.sx0 & EOT))) {
        switch ((rp->tssr & TCC) >> 1) {
        case 6: /* unrecoverable, tape pos lost */
        case 7: /* fatal */
            state = SIDLE;
            ts_openf = TSOFFLINE;
            break;

        case 5: /* recoverable, tape not moved */
            tstab.b_errcnt++;
            state = SRETRY;
            break;

        case 4: /* recoverable, tape pos one rec down from start of func*/
            if (++tstab.b_errcnt < 10) {
                if (bp->b_flags & B_READ)
                    CMDBUF->cmd = ts_swap|ACK|REREAD|IE;
                else
                    CMDBUF->cmd = ts_swap|ACK|REWRITE|IE;
                rp->tsdb = cbufaddr;
                return;
            }
            break;

        case 3: /* function reject */
            state = SIDLE;
            ts_openf = TSOFFLINE;
            break;

        case 2: /* status alert */
            if (mesbuf.sx0 & TMK) { /*tape mark*/
                ts_nxrec = FsPTOL(bp->b_dev, bp->b_blkno);
                state = SMARK;
                break;
            }
            if (mesbuf.sx0 & EOT) {
                state = SIDLE;
                ts_openf = TSOFFLINE;
                break;
            }
            if (mesbuf.sx0 & RLS) {
                state = SIO;
                break;
            }
            if (mesbuf.sx0 & RLL) {
                state = SIO;
                mesbuf.rbpcr = 0;
                break;
            }
            break;

        case 1: /* attention */
            if (rp->tssr & OFL) {
                printf("\nMagtape %x offline\n", TSDEV(bp->b_dev));
                state = SIDLE;
            }
            break;

        case 0: /* normal termination */
            if (state == SIO && (mesbuf.sx0 & EOT))
                state = SIDLE;
            break;

        default:
            break;
        }

        if (tstab.b_errcnt > 4 || state == SIDLE) {
            if (mesbuf.sx0 & EOT)
                printf("TS: end of tape reached\n");
            else
                printf("\nTS error: sr %4x sx0 %4x sx1 %4x sx2 %4x sx3 %4x errcnt %d\n",
                    rp->tssr, mesbuf.sx0, mesbuf.sx1,
                    mesbuf.sx2, mesbuf.sx3, tstab.b_errcnt);
        }
        if (tstab.b_errcnt >= 10 || state == SIDLE) {
            tstab.b_errcnt = 0;
            bp->b_flags |= B_ERROR;
            if (ts_openf >= 0)
                ts_openf = TSERROR;
            if (state != 0)
                tsinit(rp, 0);
            state = SIDLE;
        }
    }
    if ((tstab.b_active & SINIT)==0)
        tstab.b_active = SIDLE;
        
    switch (state) {
    case SRETRY:
        break;
        
    case SIO:
    case SMARK:
        ts_blkno++;
        /*FALLTHRU*/

    case SIDLE:
    case SCOM:
        tstab.b_errcnt = 0;
        tstab.b_actf = bp->b_actf;
        if (CMDBUF->cmd & 8) /* 2 words result, VAX doesn't check this */
            bp->b_resid = 0;
        else
            bp->b_resid = mesbuf.rbpcr;
        iodone(bp);
        break;
    case SMOVE:
        ts_blkno = FsPTOL(bp->b_dev, bp->b_blkno);
        break;
    default:
        break;
    }
    
    tsstart();
}

tsinit(rp, flag)
register struct device *rp;
{
    struct buf *cbuf; /*32*/
    int var36;
    int i;

    if (flag) {
        rp->tssr = 0;
        tstab.b_active = SINIT;
        return 0;
    }

    cbuf = &ctsbuf;
    cbuf->b_flags |= B_DMA22;

    CMDBUF->cmd = ACK|CVC|GSTAT;
    rp->tsdb = cbufaddr;

    for (i=0; i < 20000; i++)
        if (rp->tssr & SSR) break;
    if (i >= 20000) {
        printf("TS11 init failed (GSTAT)\n");
        printf("sr %4x sx0 %4x\n", rp->tssr, mesbuf.sx0);
        return 1;
    }

    CMDBUF->cmd = ACK|CVC|TSINIT;
    rp->tsdb = cbufaddr;
    
    for (i=0; i < 20000; i++)
        if (rp->tssr & SSR) break;
    if (i >= 20000) {
        printf("TS11 init failed (transport initialize)\n");
        printf("sr %4x sx0 %4x\n", rp->tssr, mesbuf.sx0);
        return 1;
    }

    chrbuf.loba = mbufaddr;
    chrbuf.hiba = mbufaddrext;
    chrbuf.size = sizeof(struct sts);
    chrbuf.mode = 0;
    CMDBUF->cmd = ACK|CVC|SETCHR;
    CMDBUF->loba = cxbufaddr;
    CMDBUF->hiba = cxbufaddrext;
    CMDBUF->size = sizeof(struct charac);
    rp->tsdb = cbufaddr;

    for (i=0; i < 10000; i++)
        if (rp->tssr & SSR) break;
    if (i >= 10000) {
        printf("TS11 init failed (set characteristics)\n");
        printf("sr %4x sx0 %4x\n", rp->tssr, mesbuf.sx0);
        return 1;
    }
    
    if (((rp->tssr & TCC) >> 1) > 1) {
        printf("TS11 init failed (termination class)\n");
        printf("sr %4x sx0 %4x\n", rp->tssr, mesbuf.sx0);
        return 1;
    }

    return 0;
}

tsread(dev)
{
    tsphys(dev);
    physio(tsstrategy, &rtsbuf, dev, B_READ);
}

tswrite(dev)
{
    tsphys(dev);
    physio(tsstrategy, &rtsbuf, dev, B_WRITE);
}

tsphys(dev)
{
    daddr_t a;
    
    if (TSDEV(dev) < 8) {
        a = u.u_offset >> 9;
        ts_blkno = a;
        ts_nxrec = a+1;
    }
}

tsioctl(dev, func, arg)
{
    register unit = dev & 3;
    register cmd;

    struct mtop mop;
    int flag;

    /* cmds for ioctl function */
    static int cmds[] = {
        WTM, SKIPF, SKIPF|SREV, SFORW, SREV, REW, SREV|2, NOP 
    };

    if (copyin(arg, &mop, sizeof(struct mtop)) != 0) {
        u.u_error = EFAULT;
        return 1;
    }
    
    if (func != MTIOCTOP) {
err:
        u.u_error = ENXIO;
        return 0;
    }

    switch (mop.mt_op) {
    case MTWEOF:
        flag = mop.mt_count;
        break;

    case MTFSF:
    case MTBSF:
    case MTFSR:
    case MTBSR:
        flag = 1;
        ts_spblk = mop.mt_count;
        break;

    case MTREW:
    case MTOFFL:
        flag = 1;
        break;

    case MTSWAP:
        ts_swap = SWB;
        return 0;

    case MTNOSWAP:
        ts_swap = 0;
        return 0;

    default:
        goto err;
    }

    if (flag <= 0)
        goto err;

    cmd = cmds[mop.mt_op];
    while (--flag >= 0)
        tscommand(unit, cmd);
    ts_spblk = 1;
    return 0;
}
