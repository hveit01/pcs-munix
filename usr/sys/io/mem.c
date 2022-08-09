static char *_Version = "@(#) RELEASE:  1.1  Aug 22 1986 /usr/sys/io/mem.c ";

/* @(#)mem.c    6.1 */
/*
 *  Memory special file
 *  minor device 0 is physical memory
 *  minor device 1 is kernel memory
 *  minor device 2 is EOF/NULL
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/systm.h"
#include "sys/page.h"
#include "sys/var.h"    /*pcs*/
#include "sys/proc.h"   /*pcs*/
#include "sys/file.h"   /*pcs*/
#include "sys/inode.h"  /*pcs*/
#include "sys/mount.h"  /*pcs*/
#include "sys/callo.h"  /*pcs*/

extern int *mmpte;
extern char *mmvad;

mmread(dev)
{
    register unsigned n;
    register c;

    while(u.u_error==0 && u.u_count!=0) {
        n = min(u.u_count, ctob(4));                /*pcs*/
        if (dev == 0) {
            c = btotp(u.u_offset) & PG_PFNUM;
            *mmpte = c | PG_V | PG_KR;
            clrcache();                             /*pcs*/
            c = u.u_offset & (NBPP-1);              /*pcs*/
            n = min(n, NBPP-c);                     /*pcs*/
            if (copyout(&mmvad[c], u.u_base, n))
                u.u_error = ENXIO;
        } else if (dev == 1) {
            if (copyout(u.u_offset, u.u_base, n))   /*pcs*/
                u.u_error = ENXIO;
        } else
            return;
        u.u_offset += n;
        u.u_base += n;
        u.u_count -= n;
    }
}

mmwrite(dev)
{
    register unsigned n;
    register c;

    while(u.u_error==0 && u.u_count!=0) {
        n = min(u.u_count, ctob(4));                /*pcs*/
        if (dev == 0) {
            c = btotp(u.u_offset) & PG_PFNUM;       /*pcs*/
            *mmpte = c | PG_V | PG_KW;
            clrcache();                             /*pcs*/
            c = u.u_offset & (NBPP-1);              /*pcs*/
            n = min(n, NBPP-c);                     /*pcs*/
            if (copyin(u.u_base, &mmvad[c], n))
                u.u_error = ENXIO;
        } else if (dev == 1) {
            if (copyin(u.u_base, u.u_offset, n))    /*pcs*/
                u.u_error = ENXIO;
        }
        u.u_offset += n;
        u.u_base += n;
        u.u_count -= n;
    }
}

#define KIOC        ('K'<<8)
#define KIOCVAR     (KIOC|0)
#define KIOCPROC    (KIOC|1)
#define KIOCSBUF    (KIOC|3)
#define KIOCFILE    (KIOC|4)
#define KIOCINODE   (KIOC|5)
#define KIOCMOUNT   (KIOC|6)
#define KIOCCALLO   (KIOC|7)
#define KIOCROOT    (KIOC|8)
#define KIOCSWAP    (KIOC|9)
#define KIOCNSWAP   (KIOC|10)
#define KIOCSWPLO   (KIOC|11)
#define KIOCHZ      (KIOC|12)

extern struct buf *sbuf;

mmioctl(dev, func, arg)
dev_t dev;
caddr_t arg;
{
    struct a {
        caddr_t addr;   /* address of item */
        short sz;       /* sizeof(item) */
        short cnt;      /* # of items */
    } result;

    if (minor(dev) != 1) {
        u.u_error = ENODEV;
        return;
    }

    switch (func) {
    case KIOCVAR:   /* get _v */
        result.addr = (caddr_t)&v;
        result.sz = sizeof(struct var);
        result.cnt = 1;
        break;
    case KIOCPROC:
        result.addr = (caddr_t)&proc[0];
        result.sz = sizeof(struct proc);
        result.cnt = v.v_proc;
        break;
    case KIOCSBUF:
        result.addr = (caddr_t)&sbuf[0];
        result.sz = sizeof(struct buf);
        result.cnt = v.v_buf;
        break;
    case KIOCFILE:
        result.addr = (caddr_t)&file[0];
        result.sz = sizeof(struct file);
        result.cnt = v.v_file;
        break;
    case KIOCINODE:
        result.addr = (caddr_t)&inode[0];
        result.sz = sizeof(struct inode);
        result.cnt = v.v_inode;
        break;
    case KIOCMOUNT:
        result.addr = (caddr_t)&mount[0];
        result.sz = sizeof(struct mount);
        result.cnt = v.v_mount;
        break;
    case KIOCCALLO:
        result.addr = (caddr_t)&callout[0];
        result.sz = sizeof(struct callo);
        result.cnt = v.v_call;
        break;
    case KIOCROOT:
        result.addr = (caddr_t)&rootdev;
        result.sz = sizeof(dev_t);
        result.cnt = 1;
        break;
    case KIOCSWAP:
        result.addr = (caddr_t)&swapdev;
        result.sz = sizeof(dev_t);
        result.cnt = 1;
        break;
    case KIOCNSWAP:
        result.addr = (caddr_t)&nswap;
        result.sz = sizeof(int);
        result.cnt = 1;
        break;
    case KIOCSWPLO:
        result.addr = (caddr_t)&swplo;
        result.sz = sizeof(int);
        result.cnt = 1;
        break;
    case KIOCHZ:
        result.addr = (caddr_t)&hz;
        result.sz = sizeof(int);
        result.cnt = 1;
        break;
    default:
        u.u_error = EINVAL;
        return;
    }

    copyout((caddr_t)&result, arg, sizeof(struct a));
}
