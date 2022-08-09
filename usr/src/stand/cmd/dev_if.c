#include "data.h"
#include <sys/icc/pikitor.h>

extern ICC_PORT *icc_port;

int icc_if_open(bp)
register struct iobuf *bp;
{
    ICC_RESULT result;
    
    if (!hasicc) {
        printf("Missing ICC 0\n");
        return -1;
    }
    
    icc_port->par[0] = bp->b_unit;
    result = icc_call(IF_OPEN, 7011480);
    if (result <= 0) {
        printf("IF: cannot open\n");
        return -1;
    }
    return 0;
}

int icc_if_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    bpsecbuf_phys(bp);                  /* make b_dma */
    if (bp->b_count % 512) {
        printf("IW: count not a multiple of 512\n");    /*BUG: IF, not IW */
        exit();
    }

    icc_port->par[0] = bp->b_sector;
    icc_port->par[1] = bp->b_count;
    icc_port->par[2] = bp->b_dma;
    if (icc_call(rwcmd==1 ? IF_READ : IF_WRITE, 467432) < 0) {
        printf("IF: cannot %s bno = 0x%lx %d.\n",
            rwcmd==1 ? "read" : "write", bp->b_sector, bp->b_sector);
        return -1;
    }
    
    return bp->b_count;
}
