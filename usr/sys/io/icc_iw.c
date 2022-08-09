/* PCS specific */
static char* _Version = "@(#) RELEASE:  2.0  Oct 20 1986 /usr/sys/io/icc_iw.c";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/conf.h"
#include "sys/fssizes.h"

#include "sys/icc/types.h"
#include "sys/icc/pikitor.h"
#include "sys/icc/unix_icc.h"
#include "sys/icc/icc_scsi.h"
#include "sys/icc/iwctrl.h"

static int icc = 0; /* dev# of icc to serve */

#define IW_WAITLOCK 0x08
#define IW_LOCK     0x40
extern int iw_flag;

union request iw_request;
union reply iw_reply;
short iw_initflg;

union iw_iocb iocb;
paddr_t iocb_paddr;

/* note this is also defined as global in icc_is.c and icc_if.c */
char mfblk[MFBLKSIZE];
paddr_t mfblk_paddr;

#define IW_SETERR   0x02
#define IW_PRERR    0x01
short iw_softctrl;

extern int icc_initflg[];
extern struct fs_sizes iw_sizes[];
extern iw_open();

iw_lock()
{
    while (iw_flag & IW_WAITLOCK) {
        iw_flag |= IW_LOCK;
        sleep(&iw_flag, 1);
    }
    iw_flag |= IW_WAITLOCK;
}

iw_rele()
{
    if (iw_flag & IW_LOCK) {
        wakeup(&iw_flag);
        iw_flag &= ~IW_LOCK;
    }
    iw_flag = 0;
}

iw_init()
{
    int i;

    if (icc_initflg[0] == 0)
        return;

    iocb_paddr = logtophys(&iocb);
    mfblk_paddr = logtophys(mfblk);

    iw_request.ioctl_cmd.cmd = SC_DOINIT;
    iw_request.ioctl_cmd.dev = IW_ID;
    
    iw_request.ioctl_cmd.swapdev = 
        bdevsw[bmajor(swapdev)].d_open == iw_open ? minor(swapdev) : -1;
    iw_request.ioctl_cmd.txaddr = logtophys(iw_sizes);
    iw_request.ioctl_cmd.rxaddr = logtophys(&iw_reply);
    iw_reply.ok_reply.code = SCSI_SHORT_REPLY;
    Rpc(icc, SCSI_IW, SCSI_SYNC, &iw_request, WAIT);
    if (iw_reply.ok_reply.code != SCSI_OK) {
        printf("Could not send size table to ICC\n");
        return;
    }

    for (i=0; i < 4; i++) {
        iw_request.rw_cmd.cmd = SC_RDCAPACITY;
        iw_request.rw_cmd.dev = i << 4;
        iw_request.rw_cmd.bp = 0;
        Rpc(icc, SCSI_IW, SCSI_SYNC, &iw_request, WAIT);
        if (iw_reply.ok_reply.code == SCSI_OK && iw_reply.dskname.dsksize != 0) {
            printf("IW disk %d formatted with %d MB\n", i, iw_reply.dskname.dsksize);
            iw_initflg |= 1<<i;
        }
    }
}

iw_open(dev)
{
    if (icc_initflg[0] == 0) {
        u.u_error = ENXIO;
        return;
    }

    iw_lock();
    iw_request.rw_cmd.cmd = SC_OPEN;
    iw_request.rw_cmd.dev = dev + IW_ID;
    iw_request.rw_cmd.bp = 0;
    Rpc(icc, SCSI_IW, SCSI_SYNC, &iw_request, WAIT);
    if (iw_getsta(&iw_reply))
        u.u_error = ENXIO;
    iw_rele();
}

iw_strategy(bp)
register struct buf *bp;
{
    iw_lock();
    bp->av_forw = 0;
    iw_request.rw_cmd.bp = bp;
    iw_request.rw_cmd.dev = minor(bp->b_dev) + IW_ID;
    iw_request.rw_cmd.bno = bp->b_blkno;
    iw_request.rw_cmd.count = bp->b_bcount;
    iw_request.rw_cmd.paddr = bp->b_paddr;
    iw_request.rw_cmd.cmd = (bp->b_flags & B_READ) ? SC_READ : SC_WRITE;
    Rpc(icc, SCSI_IW, SCSI_ASYNC, &iw_request, WAIT);
    iw_rele();
}

iw_getsta(rp)
union reply *rp;
{
    register struct buf *bp = rp->ok_reply.bp;
    short ret = 0;

    if (rp->ok_reply.code == SCSI_OK)
        goto done;

    if (rp->err_reply.code == SCSI_ERROR) {
        if (rp->err_reply.error_code == 0x18 || rp->err_reply.error_code == 0x1f) {
            if (iw_softctrl & IW_SETERR)
                ret++;
            if (iw_softctrl & IW_PRERR)
                iw_prerr(rp);
        } else {
            ret++;
            iw_prerr(rp);
        }
    } else {
        if (rp->err_reply.code == 1)
            ret++;
        else
            printf("bad iw_reply %d\n", rp->err_reply.code);
    }
        
done:
    if (bp) {
        if (ret) {
            bp->b_flags |= B_ERROR;
            bp->b_resid = bp->b_bcount;
        } else
            bp->b_resid = 0;
        iodone(bp);
    }
    return ret;
}

iw_prerr(rp)
union reply *rp;
{
    printf("\nIW error on disk %d: ", rp->err_reply.disk);
    printf("code: 0x%x = ", rp->err_reply.error_code);
    printf("'%s' ", rp->err_reply.message);
    if (rp->err_reply.blkno >= 0)
        printf("block: %ld", rp->err_reply.blkno);
    printf("\n");
}

iw_read(dev)
dev_t dev;
{
    physio(iw_strategy, NULL, dev, /*read*/1);
}

iw_write(dev)
dev_t dev;
{
    physio(iw_strategy, NULL, dev, /*write*/0);
}

iw_ioctl(dev, cmd, arg)
{
    int ret; /* returned size */

    iw_lock();
    if (copyin(arg, &iocb, sizeof(union iw_iocb)) != 0) {
        u.u_error = EFAULT;
        iw_rele();
        return;
    }
    switch (cmd) {
    case IW_SOFTERR:
        iw_softctrl = iocb.a.level;
        break;
    default:
        iw_request.ioctl_cmd.cmd = SC_IOCTL;
        iw_request.ioctl_cmd.dev = dev + IW_ID;
        iw_request.ioctl_cmd.ioctl_cmd = cmd;
        iw_request.ioctl_cmd.txaddr = iocb_paddr;
        iw_request.ioctl_cmd.rxaddr = mfblk_paddr;
        Rpc(icc, SCSI_IW, SCSI_SYNC, &iw_request, WAIT);
        if (iw_getsta(&iw_reply))
            u.u_error = EIO;
    }

    ret = 0;
    switch (cmd) {
    case IW_RDCAPACITY:
        ret = 8;
        break;
    case IW_READBBL:
    case IW_ERRREPORT:
        ret = MFBLKSIZE;
        break;
    case IW_MODESENSE:
        ret = 0x4c; /* != MODESENSELENGTH */
        break;
    case IW_CTRLRNAME:
        ret = CTRLRNAMELEN;
    }
    
    if (ret != 0 && copyout(mfblk, arg, ret) != 0)
        u.u_error = EFAULT;
    iw_rele();
}

iw_intr(dev, up)
UNIX_MESSAGE *up;
{
    if (up->id[1] == 1)
        iw_getsta((union reply*)&(up->data[0]));
    else if (up->id[1] == 2)
        iw_getsta(&iw_reply);
    else
        printf("iw_intr: strange id[1]\n");
}