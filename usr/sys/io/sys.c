/* pcs modified */
static char *_Version = "@(#) RELEASE:  1.1  Jul 09 1986 /usr/sys/io/sys.c ";

/* @(#)sys.c    6.1 */
/*
 *  indirect driver for controlling tty.
 */
#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/tty.h"

syopen(dev, flag)
{

    if (syvtty("open")==0 && sycheck())
    (*cdevsw[bmajor(u.u_ttyd)].d_open)(minor(u.u_ttyd), flag);
}

syread(dev)
{

    if (syvtty("read")==0 && sycheck())
    (*cdevsw[bmajor(u.u_ttyd)].d_read)(minor(u.u_ttyd));
}

sywrite(dev)
{

    if (syvtty("write")==0 && sycheck())
    (*cdevsw[bmajor(u.u_ttyd)].d_write)(minor(u.u_ttyd));
}

syioctl(dev, cmd, arg, mode)
{

    if (syvtty("ioctl")==0 && sycheck())
    (*cdevsw[bmajor(u.u_ttyd)].d_ioctl)(minor(u.u_ttyd), cmd, arg, mode);
}

sycheck()
{
    if (u.u_ttyp == NULL) {
        u.u_error = ENXIO;
        return(0);
    }
    if (*u.u_ttyp != u.u_procp->p_pgrp) {
        u.u_error = EIO;
        return(0);
    }
    return(1);
}

sysel(dummy, readfds, writefds, exceptfds, timeout)
fd_set *readfds, *writefds, *exceptfds;
struct timeval *timeout;
{
    dev_t dmaj, dmin;
    register res;

    if (u.u_ttyp == 0)
        return 0;

    dmaj = major(u.u_ttyd);
    dmin = minor(u.u_ttyd);

    res = 0;
    if (cdevsw[dmaj].d_ttys)
        res |= sel_tty(&cdevsw[dmaj].d_ttys[dmin], readfds, writefds, exceptfds, timeout);
    if (cdevsw[dmaj].d_sel)
        res |= (*cdevsw[dmaj].d_sel)(dmin, readfds, writefds, exceptfds, timeout);

    return res;
}