/* @(#)bio.c    6.3 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/bio.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/seg.h"
#include "sys/var.h"

struct buf *sbuf;   /* start of buffer headers */
char *bufstart;     /* start of buffers */
int basyncnt;
int basynwait;

/*
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no one else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *  getblk
 *  bread
 *  breada
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *  bwrite
 *  bdwrite
 *  bawrite
 *  brelse
 */

/*
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface)
 */
#define notavail(bp)    \
{\
    register s;\
\
    s = spl6();\
    bp->av_back->av_forw = bp->av_forw;\
    bp->av_forw->av_back = bp->av_back;\
    bp->b_flags |= B_BUSY;\
    bfreelist.b_bcount--;\
    splx(s);\
}

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf* bread(dev, blkno)
dev_t dev;
daddr_t blkno;
{
    register struct buf* bp;

    sysinfo.lread++;
    bp = getblk(dev, blkno);
    if (bp->b_flags & B_DONE) {
        if ((bp->b_flags & B_ERROR) && (u.u_error=bp->b_error) == 0)    /*pcs*/
            u.u_error = EIO;
        return bp;
    }
    bp->b_flags |= B_READ;
    bp->b_bcount = FsBSIZE(dev);
    (*bdevsw[bmajor(dev)].d_strategy)(bp);
    u.u_ior++;
    sysinfo.bread++;
    iowait(bp);
    return bp;
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf* breada(dev, blkno, rablkno)
dev_t dev;
daddr_t blkno, rablkno;
{
    register struct buf* bp;
    register struct buf* rabp;

    bp = 0;
    if (!incore(dev, blkno)) {
        sysinfo.lread++;
        bp = getblk(dev, blkno);
        if ((bp->b_flags & B_DONE) == 0) {
            bp->b_flags |= B_READ;
            bp->b_bcount = FsBSIZE(dev);
            (*bdevsw[bmajor(dev)].d_strategy)(bp);
            u.u_ior++;
            sysinfo.bread++;
        }
    }
    if (rablkno && bfreelist.b_bcount > 1 && incore(dev,rablkno) == 0) {
        rabp = getblk(dev,rablkno);
        if (rabp->b_flags & B_DONE)
            brelse(rabp);
        else {
            rabp->b_flags |= B_READ|B_ASYNC;
            rabp->b_bcount = FsBSIZE(dev);
            (*bdevsw[bmajor(dev)].d_strategy)(rabp);
            u.u_ior++;
            sysinfo.bread++;
        }
    }
    if (bp == 0) {
        if (u.u_xablock != 0 && 
            bfreelist.b_bcount > 1 &&
            incore(dev, u.u_xablock) == 0) {
                rabp = getblk(dev, u.u_xablock);
                if (rabp->b_flags & B_DONE)
                    brelse(rabp);
                else {
                    rabp->b_flags |= B_READ|B_ASYNC;
                    rabp->b_bcount = FsBSIZE(dev);
                    (*bdevsw[bmajor(dev)].d_strategy)(rabp);
                    u.u_ior++;
                    sysinfo.bread++;
                }
        }
        return bread(dev, blkno);
    }
    iowait(bp);
    return bp;
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
bwrite(bp)
register struct buf* bp;
{
    register flag;

    sysinfo.lwrite++;
    flag = bp->b_flags;
    bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
    (*bdevsw[bmajor(bp->b_dev)].d_strategy)(bp);
    u.u_iow++;
    sysinfo.bwrite++;
    if ((flag & B_ASYNC) == 0) {
        iowait(bp);
        brelse(bp);
    } else {                                                            /*pcs*/
        basyncnt++;                                                     /*pcs*/
        if ((flag & B_DELWRI) == 0)                                     /*pcs*/
            geterror(bp);                                               /*pcs*/
    }
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
bdwrite(bp)
register struct buf* bp;
{
    sysinfo.lwrite++;
    if ((bp->b_flags & B_DELWRI) == 0)
        bp->b_start = lbolt;
    
    bp->b_flags |= B_DELWRI|B_DONE;
    bp->b_resid = 0;
    brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
bawrite(bp)
register struct buf* bp;
{
    if (bfreelist.b_bcount > 4)
        bp->b_flags |= B_ASYNC;
    bwrite(bp);
}

/*
 * release the buffer, with no I/O implied.
 */
brelse(bp)
register struct buf* bp;
{
    register struct buf** backp;
    register int s;

    if (bp->b_flags & B_WANTED) {
        wakeup((caddr_t)bp);
    }
    if (bfreelist.b_flags & B_WANTED) {
        bfreelist.b_flags &= ~B_WANTED;
        wakeup((caddr_t)&bfreelist);
    }
    if (bp->b_flags & B_ERROR) {
        bp->b_flags |= B_STALE|B_AGE;
        bp->b_flags &= ~(B_ERROR|B_DELWRI);
        bp->b_error = 0;
    }
    s = spl6();
    if ((bp->b_flags & B_AGE)) {
        backp = &bfreelist.av_forw;
        (*backp)->av_back = bp;
        bp->av_forw = *backp;
        *backp = bp;
        bp->av_back = &bfreelist;
    } else {
        backp = &bfreelist.av_back;
        (*backp)->av_forw = bp;
        bp->av_back = *backp;
        *backp = bp;
        bp->av_forw = &bfreelist;
    }
    bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_AGE);
    bfreelist.b_bcount++;
    splx(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
incore(dev, blkno)
register dev_t dev;
register daddr_t blkno;                                                 /*pcs*/
{
    register struct buf* bp;
    register struct buf* dp;

    blkno = FsLTOP(dev, blkno);
    dp = bhash(dev, blkno);
    for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
        if (bp->b_blkno == blkno && bp->b_dev == dev && (bp->b_flags & B_STALE) == 0)
            return 1;
    return 0;
}

struct buf* getblk(dev, blkno)
register dev_t dev;
daddr_t blkno;
{
    register struct buf* bp;
    register struct buf* dp;

    blkno = FsLTOP(dev, blkno);
loop:
    spl0();
    dp = bhash(dev, blkno);
    if (dp == 0)
        panic("devtab");
        
    for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
        if (bp->b_blkno != blkno || bp->b_dev != dev || (bp->b_flags & B_STALE))
            continue;
        spl6();
        if (bp->b_flags & B_BUSY) {
            bp->b_flags |= B_WANTED;
            syswait.iowait++;
            sleep((caddr_t)bp, PRIBIO+1);
            syswait.iowait--;
            goto loop;
        }
        spl0();
        notavail(bp);
        return bp;
    }
    spl6();
    if (bfreelist.av_forw == &bfreelist) {
        bfreelist.b_flags |= B_WANTED;
        sleep((caddr_t)&bfreelist, PRIBIO+1);
        goto loop;
    }
    spl0();
    bp = bfreelist.av_forw;
    notavail(bp);
    if (bp->b_flags & B_DELWRI) {
        bp->b_flags |= B_ASYNC;
        bwrite(bp);
        goto loop;
    }
    bp->b_flags = B_BUSY;
    bp->b_back->b_forw = bp->b_forw;
    bp->b_forw->b_back = bp->b_back;
    bp->b_forw = dp->b_forw;
    bp->b_back = dp;
    dp->b_forw->b_back = bp;
    dp->b_forw = bp;
    bp->b_dev = dev;
    bp->b_blkno = blkno;
    bp->b_bcount = FsBSIZE(dev);
    return bp;
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf* geteblk()
{
    register struct buf* bp;
    register struct buf* dp;

loop: 
    spl6();
    while (bfreelist.av_forw == &bfreelist) {
        bfreelist.b_flags |= B_WANTED;
        sleep((caddr_t)&bfreelist, PRIBIO+1);
    }
    spl0();
    dp = &bfreelist;
    bp = bfreelist.av_forw;
    notavail(bp);
    if (bp->b_flags & B_DELWRI) {
        bp->b_flags |= B_ASYNC;
        bwrite(bp);
        goto loop;
    }
    bp->b_flags = B_BUSY|B_AGE;
    bp->b_back->b_forw = bp->b_forw;
    bp->b_forw->b_back = bp->b_back;
    bp->b_forw = dp->b_forw;
    bp->b_back = dp;
    dp->b_forw->b_back = bp;
    dp->b_forw = bp;
    bp->b_dev = (dev_t)NODEV;
    bp->b_bcount = SBUFSIZE;
    return bp;
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
iowait(bp)
register struct buf* bp;
{
    syswait.iowait++;
    spl6();
    while ((bp->b_flags & B_DONE) == 0)
        sleep((caddr_t)bp, PRIBIO);
    spl0();
    syswait.iowait--;
    geterror(bp);
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
iodone(bp)
register struct buf* bp;
{
    bp->b_flags |= B_DONE;
    if (bp->b_flags & B_READ)
        clrcache();
    if ( bp->b_flags & B_ASYNC) {
        if ((bp->b_flags & B_READ) == 0)                                /*pcs*/
            basyncnt--;                                                 /*pcs*/
        if (basyncnt == 0 && basynwait != 0) {                          /*pcs*/
            basynwait = 0;                                              /*pcs*/
            wakeup((caddr_t)&basyncnt);                                 /*pcs*/
        }                                                               /*pcs*/
        brelse(bp);
    } else {
        bp->b_flags &= ~B_WANTED;
        wakeup((caddr_t)bp);
    }
}

/*
 * Zero the core associated with a buffer.
 */
clrbuf(bp)
struct buf* bp;
{
    bzero(bp->b_un.b_words, bp->b_bcount);
    bp->b_resid = 0;
}

bflush(dev)
register dev_t dev;                                                     /*pcs*/
{
    register struct buf* bp;
    
loop: 
    spl6();
    for (bp = bfreelist.av_forw; bp != &bfreelist; ) {
        if (bp->b_flags & B_DELWRI && (dev==NODEV || dev == bp->b_dev)) {
            bp->b_flags |= B_ASYNC;
            notavail(bp);
            bwrite(bp);
            spl0();                                                     /*pcs*/
            goto loop;                                                  /*pcs*/
        } else
            bp = bp->av_forw;                                           /*pcs*/
    }
    spl0();
}

bdflush()                                                               /*pcs*/
{                                                                       /*pcs*/
    register struct buf* bp;                                            /*pcs*/
                                                                        /*pcs*/
    register int t;                                                     /*pcs*/
    register int s;                                                     /*pcs*/
                                                                        /*pcs*/
    t = v.v_autoup * hz;                                                /*pcs*/
loop:                                                                   /*pcs*/
    spl6();                                                             /*pcs*/
    for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {  /*pcs*/
        if ((bp->b_flags & B_DELWRI) && (lbolt - bp->b_start) >= t) {   /*pcs*/
            bp->b_flags |= B_ASYNC;                                     /*pcs*/
            s = spl6();                                                 /*pcs*/
            bp->av_back->av_forw = bp->av_forw;                         /*pcs*/
            bp->av_forw->av_back = bp->av_back;                         /*pcs*/
            bp->b_flags |= B_BUSY;                                      /*pcs*/
            bfreelist.b_bcount--;                                       /*pcs*/
            splx(s);                                                    /*pcs*/
            bwrite(bp);                                                 /*pcs*/
            spl0();                                                     /*pcs*/
            goto loop;                                                  /*pcs*/
        }                                                               /*pcs*/
    }                                                                   /*pcs*/
    spl0();
    sleep((caddr_t)bdflush, PRIBIO);                                    /*pcs*/
    goto loop;                                                          /*pcs*/
}                                                                       /*pcs*/

bdwait()                                                                /*pcs*/
{                                                                       /*pcs*/
    spl6();                                                             /*pcs*/
    while (basyncnt != 0) {                                             /*pcs*/
        basynwait = 1;                                                  /*pcs*/
        sleep((caddr_t)&basyncnt, PRIBIO);                              /*pcs*/
    }                                                                   /*pcs*/
    spl0();                                                             /*pcs*/
}                                                                       /*pcs*/

binval(dev)
register dev_t dev;                                                     /*pcs*/
{
    register struct buf* dp;
    register struct buf* bp;
    register i;

    if (dev != swapdev) {                                               /*pcs*/
        for (i=0; i < v.v_hbuf; i++) {
            dp = (struct buf*)&hbuf[i];
            for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
                if (dev == bp->b_dev)
                    bp->b_flags |= B_STALE|B_AGE;
        }
    }                                                                   /*pcs*/
}

binit()
{
    register struct buf* bp;
    register struct buf* dp;
    register unsigned int i;
    register unsigned int padr;
    register char* buffers;
    int bufsiz, hdsiz;
    
    dp = &bfreelist;
    dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
    
    hdsiz = btoc(sizeof(struct buf) * v.v_buf);
    bp = (struct buf*)sptalloc(hdsiz);                                  /*pcs*/
    sbuf = bp;
    bufsiz = btoc(SBUFSIZE * v.v_buf);
    buffers = sptalloc(bufsiz);                                         /*pcs*/
    padr = map_dma(buffers, ctob(bufsiz));                              /*pcs*/
    bufstart = buffers;
    if (bp == 0 || buffers == 0)
        panic("binit");
    maxmem -= (bufsiz + hdsiz);
    
    for (i = 0; i < v.v_buf; i++, bp++, buffers += SBUFSIZE, padr += SBUFSIZE) {
        bp->b_dev = NODEV;
        bp->b_un.b_addr = buffers;
        bp->b_dmd = *svtopte(buffers);                              /*pcs*/
        bp->b_paddr = padr;                                             /*pcs*/
        bp->b_back = dp;
        bp->b_forw = dp->b_forw;
        dp->b_forw->b_back = bp;
        dp->b_forw = bp;
        bp->b_flags = B_BUSY;
        bp->b_bcount = 0;
        brelse(bp);
    }
    pfreelist.av_forw = bp = pbuf;
    for ( ; bp < &pbuf[v.v_pbuf-1]; bp++) {
        bp->av_forw = bp+1;
    }
    bp->av_forw = 0;
    for (i=0; i < v.v_hbuf; i++) {
        hbuf[i].b_forw = hbuf[i].b_back = (struct buf*)&hbuf[i];
    }
}
