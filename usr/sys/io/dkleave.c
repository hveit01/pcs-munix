/*pcs only */
static char *_Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/io/dkleave.c";

/* this looks like a relic from former systems */

/* It is pure assumption that this belongs to a former optimizer
 * for disk blocks to be read from or written to disk
 * similar to dsort.c
 *
 * Therefore it looks plausible that these routines operate on buffers
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/buf.h"

dkblock(bp)
register struct buf *bp;
{
    register int n = minor(bp->b_dev);
    
    if ((n & 0x40) == 0) /* bit 6 is 0 */
        return bp->b_blkno;
    else {
        n >>= 3;
        n &= 7;
        n++;
        return bp->b_blkno / n;
    }
}

dkunit(bp)
register struct buf *bp;
{
    register int n = minor(bp->b_dev);

    n >>= 3;
    if ((n & 8) == 0) /* bit 6 is 0 */
        return n;
    else {
        n &= 7;
        n++;
        return bp->b_blkno % n;
    }
}
