/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.2  Nov 10 1986 /usr/sys/io/isdump.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/icc/pikitor.h"
#include "sys/icc/unix_icc.h"


/* this is @ addr ICC0_
 * does this correspond to RDCR0-2,RDSINT of icc.h? 
 * not yet fully understood
 */
struct qbusicc {
    short addrhi;
    short addrmi;
    short addrlo;
    short cmd;
};

int con_type;
ICC_PORT i_port;
ICC_PORT *icc_port = &i_port;
char *pik_version;
int icc_pikprint;

static int haveicc;

extern int icc_noprint; /*cons.c*/
extern char end[];
extern int *dumppte;
extern short *dumpvad;

#define IS_TIMEOUT  116858 /* where does this come from? */

remap_dma(base, endaddr)
{
    register int *src;
    register int *dst;
    int npages;

    npages = btotp(endaddr-1 + base) - btotp(base) + 1;
    src = &sbrpte[svtop(base)];
    dst = &_dmapt[0x40];
    while (npages-- != 0)
        *dst++ = *src++;
}

icc_connect()
{
    register struct qbusicc *icc;
    register caddr_t dummy1; /*unused*/
    int dummy2; /*unused*/
    int i;
    long paddr;
    int var_2c;
    
    icc = (struct qbusicc*)ICC0_BASE;
    haveicc = !(ssword(icc, 0) == -1);

    if (haveicc==0)
        return -1;

    remap_dma(SYSVA, &end[-SYSVA]);
    paddr = logtophys(icc_port);
    bzero(icc_port, sizeof(ICC_PORT));
    icc_noprint++;
    icc_port->result = 0;

    icc->addrhi = paddr >> 16;
    icc->addrmi = paddr >> 8;
    icc->addrlo = paddr;
    icc->cmd = SETINT_RESET;

    i = IS_TIMEOUT;
    while (icc_port->result == 0 && --i)
        clrcache();

    icc->addrhi = paddr >> 16;
    icc->addrmi = paddr >> 8;
    icc->addrlo = paddr;
    icc->cmd = 1;
    icc_pikprint++;
    icc_noprint = 0;
    return 0;
}

ICC_RESULT icc_call(func, timeout)
short func;
{
    icc_port->func = func;
    do {
        if (--timeout == 0)
            return E_TIMEOUT;
        clrcache();
    } while (icc_port->func != ICC_READY);
    return icc_port->result;
}

is_dump()
{
    register short *pt;
    register i;
    register k;
    register basept;
    ICC_RESULT res;
    
    icc_port->par[0] = 0;
    res = icc_call(IS_MODE, 60*IS_TIMEOUT); /* long timeout */
    if (res >= 0) {
        icc_port->par[0] = 0;
        res = icc_call(IS_SKIP, 200*IS_TIMEOUT); /* long timeout */
    }
    
    if (res < 0) {
        printf("cannot open IS\n");
        return -1;
    }
    
    for (basept=60, k=0; ; k += basept) {
        clrcache();
        *dumppte = ((k + basept) >> 2) | (PG_V|PG_KW);
        pt = &dumpvad[ctob(k + basept) & POFFMASK];
        if (storacc(pt)== 0) {
            basept = btoc((ctob(k + basept) & ~SOFFMASK)) - k;
            if (basept == 0)
                break;
        }
    
        for (i = 0; i < ((basept+3)>>2); i++)
            _dmapt[i] = ((k >> 2) + i) | (PG_V|PG_KW);

        icc_port->par[0] = ctob(basept);
        icc_port->par[1] = ctob(k) & POFFMASK;
        icc_port->par[2] = 0;
        if (icc_call(IS_WRITE, 20*IS_TIMEOUT) < 0) {
            printf("write error on IS\n");
            return -1;
        }
    }
    
    icc_port->par[0] = 0;
    icc_port->par[1] = 0;
    icc_port->par[2] = 1;
    icc_call(IS_WRITE, 20*IS_TIMEOUT); /* long timeout */

    icc_port->par[0] = 0;
    icc_call(IS_SKIP, 200*IS_TIMEOUT);
    return 0;
}

icc_putchar(ch)
char ch;
{
    if (icc_noprint==0) {
        icc_port->par[5] = ch;
        icc_call(PUTC, 10*IS_TIMEOUT); /*long timeout = ? */
    }
}

icc_getchar()
{
    if (icc_noprint) 
        return 0;

    icc_call(GETC, -1); /* timeout = 65535 */
    return icc_port->result;
}
