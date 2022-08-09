/* @(#)err.c    6.1 */
static char *_Version = "@(#) RELEASE:  1.0  Jan 12 1987 /usr/src/uts/io/err.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/buf.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/utsname.h"
#include "sys/elog.h"
#include "sys/systm.h"

ushort logging;

extern char *msgrp, *msgbufp;
extern char ovmsgb;
extern char nprinterr;
extern char msgbuf[];
extern int hz;

erropen(dev,flg)
{
    if(logging && (flg & FREAD) != 0) {
        u.u_error = EBUSY;
        return;
    }
    if(dev != 0 || (flg & (FREAD|FWRITE)) == (FREAD|FWRITE)) {
        u.u_error = ENXIO;
        return;
    }
    if(!suser()) {
        u.u_error = EPERM;
        return;
    }
    
    if (flg & FREAD) {
        logging++;
        msgrp = msgbufp;
        ovmsgb = 0;
    }
}

errclose(dev,flg)
{
    logging = 0;
    nprinterr = 0;
}

errread(dev)
{
    register n;
    extern char msgslp;
    
    spl7();

    for (;;) {
        if (ovmsgb > 0) {
            u.u_error = EAGAIN;
            ovmsgb = 0;
            spl0();
            return;
        }
        if (msgbufp != msgrp)
            break;
        msgslp++;
        sleep(&msgbufp, PPIPE);
    }   

    while (u.u_count > 0 && msgrp != msgbufp) {

        if ((int)msgbufp > (int)msgrp)
            n = msgbufp - msgrp;
        else
            n = &msgbuf[1024] - msgrp;
        spl0();
        n = (n > u.u_count) ? u.u_count : n;
        if (copyout(msgrp, u.u_base, n) < 0) {
            u.u_error = EFAULT;
            return;
        }
        u.u_count -= n;
        u.u_base += n;
        msgrp += n;
        if ((int)msgrp == (int)&msgbuf[1024])
            msgrp = msgbuf;
        spl7();
    }
    
    spl0();
}

#define EIOC        ('E'<<8)
#define EIODIS      (EIOC|1)
#define EIOENA      (EIOC|2)

errioctl(dev, func)
{
    if (func == EIODIS)
        nprinterr = 0;
    else if (func == EIOENA)
        nprinterr = 1;
    else
        u.u_error = EINVAL;
}

errwrite()
{
    register long n;
    register caddr_t baddr;
    struct buf *bp;
    
    if (u.u_count > (ulong)512) {
        u.u_error = E2BIG;
        return;
    }
    if (!logging) {
        u.u_error = EIO;
        return;
    }
    
    bp = geteblk();
    n = u.u_count;
    copyin(u.u_base, baddr = bp->b_un.b_addr, n);
    
    spl7();
    while (msgbufp != msgrp) {
        delay(hz / 2);
        if (!logging) {
            u.u_error = EIO;
            goto done;
        }
        spl7();
    }
    while (u.u_count != 0) {
        n = &msgbuf[1024] - msgbufp;
        n = (ulong)n > u.u_count ? u.u_count : n;
        bcopy(baddr, msgbufp, n);
        baddr += n;
        msgbufp += n;
        u.u_count -= n;
        if ((int)msgbufp >= (int)&msgbuf[1024])
            msgbufp = msgbuf;
    }
    wakeup(&msgbufp);
    
done:
    brelse(bp);
    spl0();
}

