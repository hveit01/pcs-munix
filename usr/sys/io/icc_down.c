/* PCS specific */
static char *_Version = "@(#) RELEASE:  2.2  May 08 1987 /usr/sys/io/icc_down.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"

#include "sys/icc/pikitor.h"
#include "sys/icc/unix_icc.h"

#define PLOAD_TMOUT 250

struct downctrl downctrl[4];
char icc_buf_d[ICC_BUFSIZE];

extern int icc_initflg[];
extern int icc_boot;
extern ICC_DES icc_des[];
extern int (*down_initprocs[])();
extern int (*icc_initprocs[]) ();

iccioctl_d(dev, cmd, arg)
short dev;
{
    register int (**iproc)();
    register iccno;
    struct downctrl *dcp;

    iccno = minor(dev); 
    dcp = &downctrl[iccno];
    switch (cmd) {
    case ICC_RESET:
        if ((dcp->dc_flags & ICC_RESET_FLAG)==0) {
            reseticc(iccno, SETINT_RESET);
            delay(hz);
            reseticc(iccno, 1);
            dcp->dc_flags |= ICC_RESET_FLAG;
        } else
            u.u_error = EBUSY;
        break;
    case ICC_RUN:
        if (dcp->dc_flags & (ICC_RESET_FLAG|ICC_WRITE_FLAG)) {
            dcp->dc_icc_port.par[0] = dcp->dc_offset;
            dcp->dc_offset = 0;
            dcp->dc_icc_port.func = ICC_START;
            delay(hz);
        } else
            u.u_error = EIO;
        break;
    case ICC_DOWN_INITPROCS:
        for (iproc = &down_initprocs[0]; *iproc; )
            (*iproc++)(iccno, arg);
        break;
    case ICC_INITPROCS:
        if (dcp->dc_flags & ICC_RESET_FLAG) {
            icc_boot = 1;
            icc_initflg[iccno] = 0;
            icc_init(iccno);
            for (iproc = &icc_initprocs[0]; *iproc; )
                (*iproc++)(iccno);
        }
        break;
    case ICC_IS_INITED:
        suword(arg, icc_initflg[iccno]);
        break;
    case ICC_VERSION:
        copyout(icc_des[iccno].version, arg, 40); /* sizeof version array */
        break;
    default:
        u.u_error = EINVAL;
    }
}

iccopen_d(dev)
short dev;
{
    register uint iccno;
    struct downctrl *dcp;

    iccno = minor(dev);
    if (iccno >= 4) {
        u.u_error = ENODEV;
        return;
    }       
    
    dcp = &downctrl[iccno];
    if (dcp->dc_flags & ICC_OPEN_FLAG) {
        u.u_error = EBUSY;
        return;
    }

    if (ssword(icc_base[iccno], 0) < 0)
        u.u_error = ENODEV;
    else
        dcp->dc_flags |= ICC_OPEN_FLAG;
}

iccwrite_d(dev)
short dev;
{
    register iccno;
    register ulong cnt;
    register off_t off;
    int tmout = PLOAD_TMOUT;
    ICC_PORT *icp;
    struct downctrl *dcp;

    iccno = minor(dev);
    dcp = &downctrl[iccno];
    icp = &dcp->dc_icc_port;
    
    if (dcp->dc_flags & ICC_RESET_FLAG) {
        if ((dcp->dc_flags & ICC_WRITE_FLAG)==0) {
            dcp->dc_offset = u.u_offset;
            dcp->dc_flags |= ICC_WRITE_FLAG;
        }

        if (u.u_count > ICC_BUFSIZE) {
            u.u_error = EINVAL;
            return;
        }

        cnt = u.u_count;
        off = u.u_offset;
        iomove(icc_buf_d, u.u_count, 0);
        if (u.u_error)
            return;

        icp->par[0] = logtophys(icc_buf_d);
        icp->par[1] = cnt;
        icp->par[2] = off;
        icp->func = ICC_PLOAD;
        do {
            tmout--;
            delay(1);
            clrcache();
        } while (icp->func != 0 && tmout > 0);
        
        if (tmout <= 0 || icp->result != cnt) {
            u.u_error = EIO;
            return;
        }
    } else
        u.u_error = EIO;
}

iccclose_d(dev)
short dev;
{
    struct downctrl *dcp;
    register iccno;
    
    iccno = minor(dev);
    dcp = &downctrl[iccno];
    if ((dcp->dc_flags & ICC_OPEN_FLAG)==0) {
        u.u_error = ENOENT;
        return;
    }
    dcp->dc_flags = 0;
}

reseticc(iccno, intr)
register iccno;
{
    register ushort *base;
    register ulong paddr;
    struct downctrl *dcp = &downctrl[iccno];

    paddr = logtophys(dcp);
    dcp->dc_icc_port.func = ICC_READY;
    dcp->dc_icc_port.result = 0;

    base = icc_base[iccno];
    *base++ = (paddr >> 16) & 0xff;
    *base++ = (paddr >> 8) & 0xff;
    *base++ = paddr & 0xff;
    *base++ = intr;
}
