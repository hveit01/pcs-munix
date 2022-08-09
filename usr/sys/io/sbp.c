/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/io/sbp.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/sbp.h"
#include "fcntl.h"

int sbpfree = DYNAMIC+1;

#define SBP_MAXNUM  4095

/* flags */
#define SBP_OPEN    0x0001  /*open*/
#define SBP_RD      0x0002  /*readable*/
#define SBP_WR      0x0004  /*writeable*/
#define SBP_RWAIT   0x0008  /*reader waiting for data*/
#define SBP_RBUSY   0x0010  /*reader is busy, has not yet read data*/
#define SBP_BUF     0x0080  /*buffer allocated*/
#define SBP_CONF    0x0100  /*is configured*/

struct sbpinfo {
    ushort flags;
    struct portinfo info;
    struct portenq enq;
    short  unused;
} sbpport[MAXPORTS];

struct sbpbuf {
    short dev;
    struct buf *bp;
} sbpbuf[MAXPORTS];

sbpopen(dev, mode)
dev_t dev;
{
    register struct sbpinfo *sbp;
    
    if (dev >= MAXPORTS) {
        u.u_error = ENXIO;
        return;
    }
    
    sbp = &sbpport[dev];
    if (sbp->flags & SBP_OPEN) {
        u.u_error = EACCES;
        return;
    }
    
    sbp->flags = (mode != O_RDONLY ? (SBP_RD|SBP_WR) : SBP_RD) | SBP_OPEN;
}

sbpread(dev)
dev_t dev;
{
    register struct sbpinfo *sbp;
    register struct buf *bp;
    register cnt;

    if ((cnt = u.u_count) < 0 || cnt > MSGBUFS) {
        u.u_error = EINVAL;
        return;
    }

    sbp = &sbpport[dev];
    if ((sbp->flags & SBP_CONF)==0) {
        u.u_error = EACCES;
        return;
    }

    /* wait for buffer */
    while ((sbp->flags & SBP_BUF)==0) {
        sbp->flags |= SBP_RWAIT;
        sleep(sbp, PPIPE);
    }

    bp = sbpbuf[dev].bp;
    sbp->enq.pn_sendport = sbpbuf[dev].dev;
    
    cnt = bp->b_bcount;
    if (cnt <= u.u_count)
        iomove(bp->b_un.b_addr, cnt, 1);
    else {
        u.u_error = EIO;
    }
    
    brelse(bp);
    sbp->flags &= ~SBP_BUF;
    
    /* no longer busy, accept another buffer */
    if (sbp->flags & SBP_RBUSY) {
        sbp->flags &= ~SBP_RBUSY;
        wakeup(sbp);
    }
}

sbpwrite(dev)
dev_t dev;
{
    register struct sbpinfo *sbp; /* sending dev */
    register struct sbpinfo *rbp; /* receiving dev */
    register struct buf *bp;
    uint cnt;
    
    sbp = &sbpport[dev];
    cnt = u.u_count;

    if (cnt < 0 || cnt > MSGBUFS) {
        u.u_error = EINVAL;
        return;
    }

    if ((sbp->flags & SBP_CONF)==0) {
        u.u_error = EACCES;
        return;
    }

    for (rbp = &sbpport[0]; rbp < &sbpport[MAXPORTS]; rbp++) {
        if ((rbp->flags & SBP_CONF) && 
          rbp->info.pi_inport == sbp->info.pi_outport) break;
    }

    if (rbp < &sbpport[MAXPORTS] && 
      (rbp->flags & (SBP_RD|SBP_OPEN))==(SBP_RD|SBP_OPEN)) {
        while (rbp->flags & SBP_BUF) {
            rbp->flags |= SBP_RBUSY;
            sleep(rbp, PPIPE);
        }

        bp = geteblk();
        bp->b_bcount = cnt;
        iomove(bp->b_un.b_addr, cnt, 0);

        dev = rbp - sbpport; 
        sbpbuf[dev].bp = bp;
        sbpbuf[dev].dev = sbp->info.pi_inport;
        sbp->enq.pn_xrslt = BB_ACCEPTED;
        rbp->flags |= SBP_BUF;

        /* wakeup pending reader */
        if (rbp->flags & SBP_RWAIT) {
            wakeup(rbp);
            rbp->flags &= ~SBP_RWAIT;
        }
    } else {
        u.u_error = EIO;
        sbp->enq.pn_xrslt = BB_ABSENT;
    }
}

sbpclose(dev)
dev_t dev;
{
    register struct sbpinfo *sbp;

    u.u_count = 0;
    sbpwrite(dev);

    sbp = &sbpport[dev];
    if (sbp->flags & SBP_BUF)
        brelse(sbpbuf[dev].bp);

    sbp->flags = 0;
}

sbpioctl(dev, cmd, arg)
dev_t dev;
{
    uint inport;
    register struct sbpinfo *sbp = &sbpport[dev];
    
    switch (cmd) {
    case BBPENQ:
        if ((sbp->flags & SBP_CONF)==0) {
            u.u_error = EACCES;
            return;
        }
        
        sbp->enq.pn_blkavail = (sbp->flags & SBP_BUF) != 0;
        if (copyout(&sbp->enq, arg, sizeof(struct portenq)) != 0) {
            u.u_error = EFAULT;
        }
        break;

    case BBPGET:
        if ((sbp->flags & SBP_CONF)==0) {
            u.u_error = EACCES;
            return;
        }
        if (copyout(&sbp->info, arg, sizeof(struct portinfo)) != 0)
            u.u_error = EFAULT;
        break;

    case BBPSET:
        inport = sbp->info.pi_inport;
        sbp->flags &= ~SBP_CONF;
        if (copyin(arg, &sbp->info, sizeof(struct portinfo)) != 0) {
            u.u_error = EFAULT;
            return;
        }

        if (sbp->info.pi_inport != inport && (sbp->flags & SBP_BUF)) {
            brelse(sbpbuf[dev].bp);
            sbp->flags &= ~SBP_BUF;
        }
        
        if (sbp->info.pi_inport == DYNAMIC) {
            do {
                sbp->info.pi_inport = sbpfree++;
                if (sbpfree > SBP_MAXNUM)
                    sbpfree = DYNAMIC+1;
            } while (sbpclash(sbp->info.pi_inport));
        } else {
            if (sbpclash(sbp->info.pi_inport)) {
                u.u_error = EACCES;
                return;
            }
        }
        sbp->flags |= SBP_CONF;
        break;
    default:
        u.u_error = EINVAL;
    }       
}

sbpclash(port)
register port;
{
    register struct sbpinfo *sbp;

    for (sbp = &sbpport[0]; sbp < &sbpport[MAXPORTS]; sbp++) {
        if ((sbp->flags & SBP_CONF) && sbp->info.pi_inport == port)
            return 1;
    }
    return 0;
}
