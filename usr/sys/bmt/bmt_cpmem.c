/* PCS specific */

/* for monochrome BMT device */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/var.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/tty.h>
#include <sys/ttold.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/termio.h>
#include <sys/sysinfo.h>
#include <sys/cadmaus.h>

#include <sys/bmt/gdi.h>
#include <sys/bmt/gdisys.h>
#include <sys/bmt/rtk.h>
#include <sys/bmt/bmt.h>
#include <sys/bmt/list.h>
#include <sys/bmt/EventMgr.h>
#include <sys/bmt/Layer.h>
#include <sys/bmt/font.h>
#include <sys/bmt/Window.h>

static char *_Version = "@(#) RELEASE: 8.1 87/05/22 /usr/sys/bmt/bmt_cpmem.c";

extern Bip_device *bip_addr[];

#define RAMOFF (RAM_MAP*0x10000)



copy_in(dev, from, to, cnt)
int dev;
register char *from, *to;
int cnt;
{
    register char *p;
    Bip_device *bip = bip_addr[dev];
    char* end = to + cnt;
    int sz;
    
    while (to < end) {
        bip->map0 =((ulong)to) >> 17;
        p = ((char*)(((ulong)to) & -RAMOFF)) + RAMOFF;
        if (p < end)
            sz = p - to;
        else
            sz = end - to;

        if (copyin(from, &bip->ram[((ulong)to) & (RAMOFF-1)], sz) != 0)
            return -1;

        from += sz;
        to += sz;
    }
    return 0;
}

copy_out(dev, from, to, cnt)
int dev;
register char *from, *to;
int cnt;
{
    register char *p;
    Bip_device *bip = bip_addr[dev];
    char *end = from + cnt;
    int sz;

    while (from < end) {
        bip->map0 =((ulong)from) >> 17;
        p = ((char*)(((ulong)from) & -RAMOFF)) + RAMOFF;
        if (p < end)
            sz = p - from;
        else
            sz = end - from;

        if (copyout(&bip->ram[((ulong)from) & (RAMOFF-1)], to, sz) != 0)
            return -1;

        from += sz;
        to += sz;
    }
    return 0;
}

bmt_swap(dev, func, arg)
dev_t dev;
short func;
caddr_t arg;
{
    struct ap {
        int cnt;
        caddr_t from;
        caddr_t to;
    } ap;
    register struct ap *p = &ap;

    if (copyin(arg, p, sizeof(ap)) != 0) {
        u.u_error = EFAULT;
        return;
    }

    if (func == MIOCOPYIN) {
        if (copy_in(dev, p->from, p->to, p->cnt) != 0) {
            u.u_error = EFAULT;
            return;
        }
    } else if (func == MIOCOPYOUT) {
        if (copy_out(dev, p->from, p->to, p->cnt) != 0) {
            u.u_error = EFAULT;
            return;
        }
    } else
        u.u_error = EFAULT;
}
