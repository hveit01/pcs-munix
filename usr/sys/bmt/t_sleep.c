/* PCS specific */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>

static char *_Version = "@(#) RELEASE: 1.2 86/12/09 /usr/sys/bmt/t_sleep.c";

static Waker(msg)
register BMT_MESSAGE *msg;
{
    msg->flag |= PROD_WAITING;
    wakeup((caddr_t)msg);
}

/* BUG: this really assumes a BMT_MESSAGE because it will change a flag.
 * However, in kevent.c and bmt_mpp.c it is called with different structures
 * which will probably overwrite data */
T_sleep(msg, tmout)
BMT_MESSAGE *msg;
long tmout;
{
    if (tmout == 0)
        return 1;

    msg->flag &= ~PROD_WAITING;
    cancelto((caddr_t)Waker, msg);
    timeout((caddr_t)Waker, msg, tmout);
    sleep((caddr_t)msg, PZERO);
    return (msg->flag & PROD_WAITING) ? 1 : 0;
}