#include "data.h"

static int st_delay;                    /* st device: delay loop count */
static int st_eof;                      /* st device: EOF flag */
static int st_lastcmd;                  /* st device: last command executed */
static int st_stat[3];                  /* st device: status from device */

/* see usr/sys/io/dump.c */
st_readstatus()
{
    register short *iocst = (short*)STADDR;
    register short *iodata = (short*)STDATA;
    register int i;

    for (st_delay = 0; (*iocst & 0x82)==0; st_delay++); /* wait for ready */

    *iocst = 0xc005;                    /* status command */    
    for (st_delay = 0; *iocst & 0x81; st_delay++);  /* wait for ready */
    for (i=0; i<3; i++) {
        for (st_delay = 0; (*iocst & 0x80)==0; st_delay++); /* wait for ready */
        st_stat[i] = *iodata;
    }

    return st_stat[0] & 0x100 ? 1 : 0;
}

st_write(buf)
register short *buf;
{
    register int i = 256;
    register short *iocst = (short*)STADDR;
    register short *iodata = (short*)STDATA;

    do {
        for (st_delay = 0; (*iocst & 0x03)==0; st_delay++); /* wait for buffer rdy */
        *iodata = *buf++;
    } while (--i != 0);
    for (st_delay = 0; (*iocst & 0x03)==0; st_delay++); /* wait for buffer done */
}

st_read(buf)
register short *buf;
{
    register int i = 256;
    register short *iocst = (short*)STADDR;
    register short *iodata = (short*)STDATA;

    do {
        for (st_delay = 0; (*iocst & 0x03)==0; st_delay++); /* wait for buffer rdy */
        *buf++ = *iodata;
    } while (--i != 0);
    for (st_delay = 0; (*iocst & 0x03)==0; st_delay++); /* wait for buffer done */
}


st_readwrite(rwcmd, buf, ncount)
int rwcmd;
char *buf;
register int ncount;
{
    register short *iocst = (short*)STADDR;
    int nbytes = ncount;
    
    if (ncount %512) {
        printf("streamer: count not a multiple of 512\n");
        exit();
    }

    ncount /= ncount;                   /* #blocks */
    while (ncount-- != 0) {
        if (st_eof)                     /* EOF? */
            return -1;                  /* yes, but still stuff to read, error */

        if (rwcmd == 2)                 /* do read/write block */
            st_write((short*)buf);
        else
            st_read((short*)buf);

        buf += 512;                     /* advance buffer */

        if ((*iocst & 2)==0) {          /* EOF? */
            if (st_readstatus() == 0)   /* error? */
                return -1;              /* premature EOF */
            st_eof++;                   /* set end of I/O */
        }
    }                                   /* return transferred bytes if EOF on last block */
    return nbytes;
}

/* see /usr/sys/io/dump.c */
st_open(bp)
register struct iobuf *bp;
{
    register short *iocst = (short*)STADDR;
    register int fnum;

    *iocst = 3;                         /* reset */
    for (st_delay = 0; *iocst != 3; st_delay++);    /* wait for cmd rdy */
    st_readstatus();
    *iocst = 0x2101;                    /* rewind? */
    for (st_delay = 0; *iocst != 0x81; st_delay++); /* wait for cmd rdy */
    st_readstatus();
    
    st_eof = 0;
    fnum = bp->b_fnum;                  /* file number */
    while (fnum-- != 0) {
        *iocst = 0xa005;                /* forward file */
        for (st_delay = 0; (*iocst & 2)==0; st_delay++);    /* wait for EOF */
        st_readstatus();
    }
}

st_close(bp)
register struct iobuf *bp;
{
    register short *iocst = (short*)STADDR;

    st_lastcmd = -1;                    /* invalidate last command */

    *iocst = 0x2101;                    /* rewind? */
    for (st_delay = 0; *iocst != 0x81; st_delay++); /* wait for cmd rdy */
    st_readstatus();
}

st_strategy(bp, rwcmd)
register struct iobuf *bp;
int rwcmd;
{
    register short *iocst = (short*)STADDR;
    if (st_lastcmd != rwcmd) {
        *iocst = (rwcmd==2 ? 0x4000 : 0x8000) | 4 | 1;
        for (st_delay = 0; (*iocst & 0x82)==0; st_delay++); /* wait for cmd rdy */
        if (*iocst & 2) {               /* EOF? */
            read_status();
            return -1;
        }
        st_lastcmd = rwcmd;             /* set current command */
    }

    st_readwrite(rwcmd, bp->b_secbuf, bp->b_count);
}

