/* pcs only */
static char *_Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/io/dsort.c ";

/* this seems to be part of a former optimizer to sort disk blocks in the
 * buffer queue
 * and while I am sure that this works on the buf structure, I have my doubts
 * whether it really refers to an older struct buf definition. I guess it
 * is somehow defective.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/errno.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/fssizes.h"

disksort(bp1, bp2)
register struct buf *bp1, *bp2;
{
    register struct buf* next;
    register unsigned start;
    struct buf *bp3;

    next = bp1->av_forw;
    start = bp2->b_resid;
    bp2->b_start = lbolt;

    if (next == NULL) {
        bp1->av_forw = bp2;
        bp1->av_back = bp2;
        bp2->av_forw = NULL;
        return;
    }

    while ((bp3 = next->av_forw) != NULL) {
        if ( (start >= (unsigned)next->b_resid) == (start < (unsigned)bp3->b_resid) &&
             (bp2->b_start - bp3->b_start) < 30)
                break;
        next = bp3;
    }

    next->av_forw = bp2;
    bp2->av_forw = bp3;
    if (next == bp1->av_back)
        bp1->av_back = bp2;
}

realblk(bp, fsp)
register struct buf *bp;
struct fs_sizes *fsp;
{
    if (fsp == NULL)
        return bp->b_resid;
    return fsp[fsys(bp->b_dev)].offset + bp->b_blkno;
}

newsort(bp1, bp2, fsp)
register struct buf *bp1, *bp2;
struct fssizes *fsp;
{
    register struct buf *next;
    struct buf *bp3;
    register blk2, blk3, blkn;

    next = bp1->av_forw;
    if (next == NULL) {
        bp1->av_forw = bp2;
        bp1->av_back = bp2;
        bp2->av_forw = NULL;
        return;
    }

    bp3 = next;
    next = next->av_forw;
    blk2 = realblk(bp2, fsp);
    if (next) {
        blkn = realblk(next, fsp);
        if (blkn > blk2) {
            do {
                bp3 = next;
                blk3 = blkn;
                next = next->av_forw;
                if (next == 0)
                    break;
                blkn = realblk(next, fsp);
            } while (blk3 <= blkn);
        }
    }

    while (next && realblk(next, fsp) <= blk2) {
        bp3 = next;
        next = next->av_forw;
    }

    bp2->av_forw = bp3->av_forw;
    bp3->av_forw = bp2;
    if (bp3 == bp1->av_back)
        bp1->av_back = bp2;
}
