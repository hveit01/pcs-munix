/* @(#)sys3.c   6.4 */
static char* _Version = "@(#) RELEASE:  1.4  Nov 05 1986 /usr/sys/os/sys3.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/mount.h"
#include "sys/ino.h"
#include "sys/page.h"
#include "sys/buf.h"
#include "sys/filsys.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/flock.h"
#include "sys/conf.h"
#include "sys/stat.h"
#include "sys/ttold.h"
#include "sys/var.h"
#include "sys/swap.h"
#include "sys/munet/munet.h"    /*pcs*/
#include "sys/munet/mnnode.h"   /*pcs*/
#include "sys/munet/diskless.h" /*pcs*/

extern stat0();

/*
 * the fstat system call.
 */
fstat()
{
    register struct file *fp;
    register struct a {
        int fdes;
        struct stat *sb;
    } *uap;

    uap = (struct a *)u.u_ap;
    fp = getf(uap->fdes);
    if(fp == NULL)
        return;
    stat1(fp->f_inode, uap->sb);
}

/*
 * the stat system call.
 */
stat()
{
    stat0(0);
}

/*
 * the lstat system call.
 */
lstat()
{
    stat0(DONT_FOLLOW); /* don't follow symlinks */
}

/*pcs*/
stat0(flag) /* flag passed to namei */
{
    register struct inode *ip;
    register struct a {
        char* path;
        struct stat *sb;
        unsigned *rdev;
    } *uap;
    int rdev;
    unsigned int *addr;

    uap = (struct a *)u.u_ap;

    rdev = (short)fuword(&uap->sb->st_rdev);
    if ((ip = namei(uchar, flag)) == 0)
        return;

    stat1(ip, uap->sb);

    if ((ip->i_mode & IFMT) == IFLAN && rdev == 0xFFFFDEAD) {
        addr = (unsigned int*)&ip->i_netx;
        if (sulong(uap->rdev, GET_IPADDR(addr)) != 0)
            u.u_error = EFAULT;
    }
    iput(ip);
} 
/*pcs*/

/*
 * the dup system call.
 */
dup() {
    register struct file *fp;
    register i;
    register fbits;
    int dummy; /* pcs unused variable */

    register struct a {
        int fdes;
        int fdes2;                      /*pcs*/
    } *uap;

    uap = (struct a *)u.u_ap;
    fbits = uap->fdes & ~077;           /*pcs*/
    
    fp = getf(uap->fdes &= 077);        /*pcs*/
    if(fp == NULL)
        return;

    if ((fbits & 0x40) == 0) {
        if ((i = ufalloc(0)) < 0)
            return;
    } else {                            /*pcs*/
        i = uap->fdes2;                 /*pcs*/
        if (i < 0 || i >= NOFILE) {     /*pcs*/
            u.u_error = EBADF;          /*pcs*/
            return;                     /*pcs*/
        }                               /*pcs*/
        u.u_rval1 = i;                  /*pcs*/
    }

    if (i == uap->fdes)
        return;

    if (u.u_ofile[i])
        closef(u.u_ofile[i]);

    u.u_ofile[i] = fp;
    u.u_pofile[i] = (u.u_pofile[uap->fdes] & ~1); /*pcs*/
    fp->f_count++;
}

/*
 * the file control system call.
 */
fcntl()
{
    register struct file *fp;
    register struct a {
        int fdes;
        int cmd;
        int arg;
    } *uap;
    struct flock bf;
    register i;

    uap = (struct a *)u.u_ap;
    fp = getf(uap->fdes);
    if (fp == NULL)
        return;
    switch(uap->cmd) {
    case 0:
        i = uap->arg;
        if (i < 0 || i > NOFILE) {
            u.u_error = EINVAL;
            return;
        }
        if ((i = ufalloc(i)) < 0)
            return;
        u.u_ofile[i] = fp;
        fp->f_count++;
        u.u_pofile[i] = u.u_pofile[uap->fdes] & ~1; /*pcs*/
        break;

    case 1:
        u.u_rval1 = u.u_pofile[uap->fdes] & 1;
        break;

    case 2:
        u.u_pofile[uap->fdes] &= ~1;                /*pcs*/
        u.u_pofile[uap->fdes] |= (uap->arg & 1);    /*pcs*/
        break;

    case 3:
        u.u_rval1 = fp->f_flag+FOPEN;
        break;

    case 4:
        fp->f_flag &= (FREAD|FWRITE);
        fp->f_flag |= (uap->arg-FOPEN) & ~(FREAD|FWRITE);
        break;

    case 5:
        /* get record lock */
        if (copyin(uap->arg, &bf, sizeof bf))
            u.u_error = EFAULT;
        else if ((i=getflck(fp, &bf)) != 0)
            u.u_error = i;
        else if (copyout(&bf, uap->arg, sizeof bf))
            u.u_error = EFAULT;
        break;

    case 6:
        /* set record lock and return if blocked */
        if (copyin(uap->arg, &bf, sizeof bf))
            u.u_error = EFAULT;
        else if ((i=setflck(fp, &bf, 0)) != 0)
            u.u_error = i;
        break;

    case 7:
        /* set record lock and wait if blocked */
        if (copyin(uap->arg, &bf, sizeof bf))
            u.u_error = EFAULT;
        else if ((i=setflck(fp, &bf, 1)) != 0)
            u.u_error = i;
        break;

    default:
        u.u_error = EINVAL;
    }
}

/*
 * character special i/o control
 */
ioctl()
{
    register struct file *fp;
    register struct inode *ip;
    register struct a {
        int fdes;
        int cmd;
        int arg;
    } *uap;
    register dev_t dev;

    uap = (struct a *)u.u_ap;
    if ((fp = getf(uap->fdes)) == NULL)
        return;
    
    if (uap->cmd == FIOCLEX) {
        u.u_pofile[uap->fdes] |= 1;
    } else if (uap->cmd == FIONCLEX) {
        u.u_pofile[uap->fdes] &= ~1;
    } else {
        ip = fp->f_inode;
        switch (ip->i_mode & IFMT) {
        case IFCHR:
            break;
        case IFIFO:
            vtty_ioctl(fp);
            return;
        default:
            u.u_error = ENOTTY;
            return;
        }
        dev = (dev_t)ip->i_rdev;
        if (cdevsw[major(dev)].d_str)
            strioctl(ip, uap->cmd, uap->arg, fp->f_flag);
        else
            (*cdevsw[major(dev)].d_ioctl)(minor(dev), uap->cmd, uap->arg, fp->f_flag);
    }
}

struct inode* mgetdev()
{
    register struct inode *ip;
    register dev_t dev;

    if ((ip = namei(uchar, 0)) == 0)
        return ip;
    dev = ip->i_rdev;

    switch (ip->i_mode & IFMT) {
    case IFBLK:
        if (bmajor(dev) >= bdevcnt)
            u.u_error = ENXIO;
        break;
    case IFLAN:
        if (bmajor(dev) >= nlandev)
            u.u_error = ENXIO;
        break;
    default:
        u.u_error = ENOTBLK;
        break;
    }

    if (u.u_error) {
        iput(ip);
        ip = 0;
    }
    return ip;
} 

/*
 * the mount system call.
 */
smount()
{
    dev_t dev;
    register struct inode *ip;
    register struct mount *mp;
    struct mount *smp;
    register struct filsys *fp;
    struct inode *bip;
    register struct a {
        char    *fspec;
        char    *freg;
        int ronly;
    } *uap;
    int i;
    unsigned int *addr;

    uap = (struct a*)u.u_ap;
    if (!suser())
        return;

    bip = mgetdev();
    if (u.u_error)
        return;
    dev = bip->i_rdev;
    u.u_dirp = (caddr_t)uap->freg;
    prele(bip);
    ip = namei(uchar, 0);
    if (ip == 0) {
        plock(bip);
        iput(bip);
        return;
    }
    if (ip == bip) {
        ip->i_count--;
        iput(ip);
        u.u_error = ENOTDIR;
        return;
    }
    plock(bip);
    if ((ip->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        goto out;
    }
    if (ip->i_count != 1)
        goto out;
    if (ip->i_number == ROOTINO)
        goto out;
    if (master != 0) {
        for (i = 0; i < maxdlnactv; i++) {
            if (dltable[i].dl_flags & DL_CONNECTED) {
                if ((dev & ~Fs2BLK) == (dltable[i].dl_rootdev & ~Fs2BLK))
                    goto out;
            }
        }
    }
    smp = NULL;
    for (mp = &mount[0]; mp < (struct mount *)v.ve_mount; mp++) {
        if (mp->m_flags != MFREE) {
            if (brdev(dev) == brdev(mp->m_dev))
                goto out;
        } else
        if (smp == NULL)
            smp = mp;
    }
    mp = smp;
    if (mp == NULL)
        goto out;
    mp->m_flags = MINTER;
    mp->m_dev = brdev(dev);

    if ((bip->i_mode & IFMT) == IFLAN) {
        if (uap->ronly) {
            u.u_error = EACCES;
            goto out1;
        }
        addr = (unsigned int*)&bip->i_netx;
        uimount(mp->m_dev, GET_IPADDR(addr));
        if (u.u_error != 0)
            goto out1;
        mp->m_bufp = 0;
        mp->m_mount = 0;
        mp->m_inodp = ip;
        mp->m_flags = MINUSE;
        mp->m_count = 1;
        mp->m_omnt = ip->i_mount;
        ip->i_flag |= ILAND|IMOUNT;
        ip->i_mount = mp;
        iput(bip);
        prele(ip);
        return;
    }
    
    (*bdevsw[bmajor(dev)].d_open)(minor(dev), uap->ronly ? 1 : 3, 1);
    if(u.u_error)
        goto out1;
    mp->m_bufp = geteblk();
    fp = mp->m_bufp->b_un.b_filsys;
    u.u_offset = SUPERBOFF;
    u.u_count = sizeof(struct filsys);
    u.u_base = (caddr_t)fp;
    u.u_segflg = 1;
    readi(bip);
    if (u.u_error != 0)
        goto fserr;
    fp->s_fmod = 0;
    fp->s_ilock = 0;
    fp->s_flock = 0;
    fp->s_ninode = 0;
    fp->s_inode[0] = 0;
    fp->s_ronly = uap->ronly & 1;
    if (fp->s_magic != FsMAGIC) {
        u.u_error = EINVAL;
        goto fserr;
    }
    if (fp->s_ronly == 0) {
        if ((fp->s_state + fp->s_time) == FsOKAY) {
            fp->s_state = FsACTIVE;
            u.u_offset = SUPERBOFF;
            u.u_count = sizeof(struct filsys);
            u.u_base = (caddr_t)fp;
            u.u_segflg = 1;
            u.u_fmode = FWRITE|FSYNC;
            writei(bip);
            if (u.u_error != 0) {
                u.u_error = EROFS;
                goto fserr;
            }
        } else {
            u.u_error = ENOSPC;
            goto fserr;
        }
    }
    if (fp->s_type == Fs2b) {
        binval(mp->m_dev);
        mp->m_dev |= Fs2BLK;
    }
    if (brdev(pipedev) == brdev(mp->m_dev))
        pipedev = mp->m_dev;
    if ((mp->m_mount = iget(mp, ROOTINO)) != NULL)
        prele(mp->m_mount);
    else
        goto fserr;
    mp->m_inodp = ip;
    mp->m_flags = MINUSE;
    mp->m_count = 1;
    mp->m_omnt = ip->i_mount;
    ip->i_flag |= IMOUNT;
    ip->i_mount = mp;
    iput(bip);
    prele(ip);
    return;

fserr:
    (*bdevsw[bmajor(dev)].d_close)(minor(dev), !uap->ronly, 1);
    brelse(mp->m_bufp);
out1:
    mp->m_flags = MFREE;
out:
    if (bip != NULL)
        iput(bip);
    if (u.u_error == 0)
        u.u_error = EBUSY;
    iput(ip);
}

/*
 * the umount system call.
 */
sumount()
{
    dev_t dev, odev;
    register struct inode *ip;
    register struct inode *bip;
    register struct mount *mp;
    register struct a {
        char    *fspec;
    };
    struct filsys *fp;

    if(!suser())
        return;

    bip = mgetdev();
    if (u.u_error)
        return;
    
    dev = bip->i_rdev;
    for (mp = &mount[0]; mp < (struct mount*)v.ve_mount; mp++)
        if (mp->m_flags == MINUSE && brdev(dev) == brdev(mp->m_dev))
            goto found;
    iput(bip);
    u.u_error = EINVAL;
    return;

found:
    odev = dev;
    dev = mp->m_dev;
    xumount(dev);
    if (mp->m_mount) {
        plock(mp->m_mount);
        iput(mp->m_mount);
        mp->m_mount = NULL;
        u.u_error = 0;
    }
    if (iflush(dev) < 0) {
        iput(bip);
        u.u_error = EBUSY;
        return;
    }
    if ((bip->i_mode & IFMT) == IFLAN) {
        uiumount(dev);
        if (u.u_error) {
            iput(bip);
            return;
        }
        ip = mp->m_inodp;
        plock(ip);
        ip->i_flag &= ~(IMOUNT|ILAND);
        ip->i_mount = mp->m_omnt;
        mp->m_omnt = NULL;
        iput(ip);
        iput(bip);
        mp->m_bufp = NULL;
        mp->m_flags = MFREE;
        return;
    }
    mp->m_flags = MINTER;
    ip = mp->m_inodp;
    ip->i_flag &= ~IMOUNT;
    ip->i_mount = mp->m_omnt;
    mp->m_omnt = NULL;
    plock(ip);
    iput(ip);
    fp = mp->m_bufp->b_un.b_filsys;
    if (!fp->s_ronly) {
        bflush(dev);
        fp->s_time = time;
        fp->s_state = FsOKAY - fp->s_time;
        u.u_error = 0;
        u.u_offset = SUPERBOFF;
        u.u_count = sizeof(struct filsys);
        u.u_base = (caddr_t)fp;
        u.u_segflg = 1;
        u.u_fmode = FWRITE|FSYNC;
        writei(bip);
        u.u_error = 0;
    }
    iput(bip);
    (*bdevsw[bmajor(dev)].d_close)(minor(dev), 0, 1);
    punmount(mp);
    binval(dev);
    if (dev != odev)
        binval(odev);
    brelse(mp->m_bufp);
    mp->m_bufp = NULL;
    mp->m_flags = MFREE;
} 

srmount(flag)
{ 
    register struct mount *mp;
    register struct buf *bp;
    register struct filsys *fp;
    struct inode itmp;
    extern struct mount *pipemnt;

    mp = &mount[0];

    if (flag) {
        bp = geteblk();
        mp->m_bufp = bp;
        fp = bp->b_un.b_filsys;
        pipemnt = &mount[0];
    } else {
        bp = mp->m_bufp;
        fp = bp->b_un.b_filsys;
        if (fp->s_state == FsACTIVE) {
            u.u_error = EINVAL;
            return;
        }
    }
    itmp.i_mode = IFBLK;
    itmp.i_rdev = rootdev;
    u.u_error = 0;
    u.u_offset = SUPERBOFF;
    u.u_count = sizeof(struct filsys);
    u.u_base = (caddr_t)fp;
    u.u_segflg = 1;
    readi(&itmp);
    if (u.u_error)
        panic("cannot mount root");
    mp->m_flags = MINTER;
    mp->m_dev = brdev(rootdev);
    mp->m_omnt = mp;
    fp->s_fmod = 0;
    fp->s_ilock = 0;
    fp->s_flock = 0;
    fp->s_ninode = 0;
    fp->s_inode[0] = 0;
    fp->s_ronly = 0;
    if (fp->s_magic != FsMAGIC)
        panic("not a valid root file system");
    if (master) {
        if ((fp->s_state+fp->s_time) == FsOKAY)
            fp->s_state = FsACTIVE;
        else
            fp->s_state = FsBAD;
        u.u_offset = SUPERBOFF;
        u.u_count = sizeof(struct filsys);
        u.u_base = (caddr_t)fp;
        u.u_segflg = 1;
        u.u_fmode = FWRITE|FSYNC;
        writei(&itmp);
    }
    if (u.u_error) {
        fp->s_state = FsBAD;
        u.u_error = 0;
    }
    if (fp->s_type == Fs2b) {
        binval(mp->m_dev);
        mp->m_dev |= Fs2BLK;
    }
    mp->m_flags = MINUSE;
    rootdev = mp->m_dev;
    if (brdev(pipedev) == brdev(rootdev))
        pipedev = rootdev;
}

/*
 *  manipulate swap files.
 */

swapfunc()
{
    register int        i;
    register struct inode   *ip;
    register swpi_t     *uap;

    uap = (swpi_t *)(*u.u_ap);

    switch(uap->si_cmd){
        case SI_LIST:
            i = sizeof(swpt_t) * MSFILES;
            if(copyout(swaptab, uap->si_buf, i) < 0)
                u.u_error = EFAULT;
            break;

        case SI_ADD:
        case SI_DEL:
            if(!suser()) {
                break;
            }
            u.u_dirp = uap->si_buf;
            ip = namei(uchar, 0);
            if(ip == NULL) {
                break;
            }
            if((ip->i_mode & IFMT)  !=  IFBLK){
                u.u_error = ENOTBLK;
                break;
            }

            if(uap->si_cmd == SI_DEL) {
                swapdel(ip->i_rdev, uap->si_swplo);
            } else {
                swapadd(ip->i_rdev, uap->si_swplo,
                    uap->si_nblks);
            }
            iput(ip);
            break;
    }
}
