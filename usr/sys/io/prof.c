/*pcs only */
static char *_Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/io/prof.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/errno.h"
#include "sys/sysmacros.h"
#include "sys/dir.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/psl.h"

#define PRFSIZE 1024
int prfstat, prfmax;
unsigned prfctr[PRFSIZE+1];
unsigned prfsym[PRFSIZE];

prfread()
{
    if ((prfstat & 2) == 0) {
        u.u_error = ENXIO;
        return;
    }

    iomove(prfsym, min(u.u_count, prfmax*sizeof(int)), 1);
    iomove(prfctr, min(u.u_count, (prfmax+1)*sizeof(int)), 1);
}

prfwrite()
{
    register unsigned *ct;
    
    if (u.u_count > PRFSIZE*sizeof(int))
        u.u_error = ENOSPC;
    else if ((u.u_count & 3) != 0 || u.u_count < 12)
        u.u_error = E2BIG;
    else if (prfstat & 1)
        u.u_error = EBUSY;
    
    if (u.u_error)
        return;

    for (ct = prfctr; ct != &prfctr[PRFSIZE+1]; *ct++ = 0) ;

    prfmax = u.u_count >> 2;
    iomove(prfsym, u.u_count, 0);

    for (ct = &prfsym[1]; ct != &prfsym[prfmax]; ct++) {
        if (ct[0] < ct[-1]) {
            u.u_error = EINVAL;
            break;
        }
    }

    if (u.u_error) {
        prfstat = 0;
        return;
    }
    
    prfstat = 2;
}
        
prfioctl(dev, func, arg)
{
    switch (func) {
    case 1:
        u.u_rval1 = prfstat;
        break;
    case 2:
        u.u_rval1 = prfmax;
        break;
    case 3:
        if (prfstat & 2) {
            prfstat = (arg & 1) | 2;
            break;
        }
        /*FALLTHRU*/
    default:
        u.u_error = EINVAL;
        break;
    }
}

prfintr(pc, ps)
register unsigned pc;
{
    register int mx, i;
    int slot;

    if ((ps  & PS_S)==0) {
        prfctr[prfmax]++;
        return;
    }
    
    for (i = 0, mx = prfmax; (slot = (i+mx) / 2) != i; ) {
        if (pc >= prfsym[slot])
            i = slot;
        else
            mx = slot;
    }
    prfctr[slot]++;
}
