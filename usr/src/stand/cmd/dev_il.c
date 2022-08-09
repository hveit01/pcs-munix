#include "data.h"
#include <sys/icc/pikitor.h>

extern ICC_PORT *icc_port;
extern char icc_versionbuf[];
char il_pool[0x322a];                   /* no idea how long this actually is */

icc_il_connect()
{
    ICC_RESULT result;
    int phys = phys_offset(il_bootfile);    /* redundant */

    icc_port->par[0] = macaddress.hi;
    icc_port->par[1] = macaddress.mi;
    icc_port->par[2] = macaddress.lo;
    icc_port->par[3] = phys_offset(il_pool);
    icc_port->par[4] = phys_offset(il_bootfile);
    icc_port->par[5] = phys_offset(icc_versionbuf);
    icc_versionbuf[0] = '\0';
    result = icc_call(LANCE_CONNECT, 1752870);

    printf(icc_versionbuf);
    if (result <= 0) {
        if (!noopenerr)
            printf("IL: cannot open\n");
        return -1;
    }
    return 0;
}

int icc_il_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    bpsecbuf_phys(bp);                  /* make b_dma */

    icc_port->par[0] = bp->b_unit;
    icc_port->par[1] = bp->b_sector;
    icc_port->par[2] = bp->b_sector >> 1;
    icc_port->par[3] = bp->b_count;
    icc_port->par[4] = bp->b_dma;

    /* if not read, fail, this driver only accepts read from net */
    if (rwcmd != 1)
        return -1;

    if (icc_call(LANCE_READ, 1752870) < 0) {
        printf("IL: cannot read bno = 0x%lx %d.\n",
            bp->b_sector, bp->b_sector);
        return -1;
    }
    
    return bp->b_count;
}
