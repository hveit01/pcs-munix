/* @(#)sys2.c   6.1 */
static char* _Version = "@(#) RELEASE:  1.4  Jan 27 1987 /usr/sys/os/sys2.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/dir.h"
#include "sys/dirent.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/file.h"
#include "sys/inode.h"
#include "sys/mount.h"
#include "sys/sysinfo.h"
#include "sys/var.h"
#include "sys/conf.h"

/*
 * read system call
 */
read()
{
    sysinfo.sysread++;
    rdwr(FREAD);
}

/*
 * write system call
 */
write()
{
    sysinfo.syswrite++;
    rdwr(FWRITE);
}

/*
 * common code for read and write calls:
 * check permissions, set base, count, and offset,
 * and switch out to readi or writei code.
 */
rdwr(mode)
register mode;
{
    register struct file *fp;
    register struct inode *ip;
    register struct a {
        int fdes;
        char    *cbuf;
        unsigned count;
    } *uap;
    int type;
    int count;                  /*pcs*/
    struct file *fpsave;        /*pcs*/
    struct inode *ipsave;       /*pcs*/

    uap = (struct a*)u.u_ap;
    fp = getf(uap->fdes);
    if (fp == 0)
        return;
    if ((fp->f_flag & mode) == 0 && 
        (fp->f_vtty1 == F_UNUSED || mode == FWRITE)) {  /*pcs*/
        u.u_error = EBADF;
        return;     
    }
    u.u_base = (caddr_t)uap->cbuf;
    count = uap->count;
    u.u_count = count;
    u.u_segflg = 0;
    u.u_fmode = fp->f_flag;
    ip = fp->f_inode;
    type = ip->i_mode & IFMT;
    if (type == IFREG || type == IFDIR) {
        plock(ip);
        if ((u.u_fmode & FAPPEND) && mode==FWRITE)
            fp->f_offset = ip->i_size;
        if (ip->i_locklist) {
            prele(ip);
            if (locked(1, ip, fp->f_offset, fp->f_offset + u.u_count))
                return;
            plock(ip);
        }
    } else if (type == IFIFO) {
        if (fp->f_vtty1) {                  /*pcs*/
            if (mode == FWRITE) {           /*pcs*/
                wr_vtty(fp);                /*pcs*/
                goto done;                  /*pcs*/
            }                               /*pcs*/
            fpsave = fp;                    /*pcs*/
            ipsave = ip;                    /*pcs*/
            stderr_vtty(&fpsave, &ipsave);  /*pcs*/
            fp = fpsave;                    /*pcs*/
            ip = ipsave;                    /*pcs*/
            if (eof_vtty(fp)) {             /*pcs*/
                count = 0;                  /*pcs*/
                goto done;                  /*pcs*/
            }                               /*pcs*/
        }
        plock(ip);
        fp->f_offset = 0;
    }
    u.u_offset = fp->f_offset;
    if (mode==FREAD)
        readi(ip);
    else
        writei(ip);

    if (type==IFREG || type==IFDIR || type==IFIFO)
        prele(ip);
    if (type==IFIFO && mode==FREAD && fp->f_vtty1 != F_UNUSED)  /*pcs*/
        rele_vtty(fp);                                          /*pcs*/
    count -= u.u_count;
    fp->f_offset += count;

done:
    u.u_rval1 = count;
    u.u_ioch += (unsigned)count;
    if (mode == FREAD)
        sysinfo.readch += count;
    else
        sysinfo.writech += count;
}

/*
 * open system call
 */
open()
{
    register struct a {
        char    *fname;
        int mode;
        int crtmode;
    } *uap;

    uap = (struct a *)u.u_ap;
    copen(uap->mode-FOPEN, uap->crtmode);
}

/*
 * creat system call
 */
creat()
{
    register struct a {
        char    *fname;
        int fmode;
    } *uap;

    uap = (struct a *)u.u_ap;
    copen(FWRITE|FCREAT|FTRUNC, uap->fmode);
}

/*
 * common code for open and creat.
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
copen(mode, arg)
register mode;
{
    register struct inode *ip;
    register struct file *fp;
    int i;

    if ((mode & (FREAD|FWRITE)) == 0) {
        u.u_error = EINVAL;
        return;
    }
    if (mode & FCREAT) {
        ip = namei(uchar, 1);
        if (ip == 0) {
            if (u.u_error)
                return;
            ip = maknode(arg & 07777 & ~ISVTX);
            if (ip == 0)
                return;
            mode &= ~FTRUNC;
        } else {
            if (mode & FEXCL) {
                u.u_error = EEXIST;
                iput(ip);
                return;
            }
            mode &= ~FCREAT;
        }
    } else {
        /* v pcs*/
        if (mode & FINOD) {
            union di {
                caddr_t c;
                struct {
                    dev_t dev;
                    ino_t ino;
                } d;
            } dirp;
            dev_t dev;
            register struct mount *mp;
            
            dirp.c = u.u_dirp;
            dev = dirp.d.dev;
            for (mp = mount; mp < (struct mount*)v.ve_mount; mp++)
                if (mp->m_flags == MINUSE &&
                    brdev(mp->m_dev) == brdev(dev))
                        break;
            if (mp == (struct mount*)v.ve_mount) {
                u.u_error = ENXIO;
                return;
            }
            ip = iget(mp, dirp.d.ino);
            if (ip == 0)
                return;
        } else {
            ip = namei(uchar, 0);
            if (ip == 0)
                return;
        }
        /* ^ pcs */
    }
    
    if ((mode & FCREAT) == 0) {
        if (mode & FREAD)
            access(ip, IREAD);
        if (mode & (FWRITE|FTRUNC)) {
            access(ip, IWRITE);
            if ((ip->i_mode & IFMT) == IFDIR)
                u.u_error = EISDIR;
        }
    }
    
    /* v pcs */
    if (ip->i_locklist && (mode & FTRUNC) && u.u_error==0 &&    /*pcs*/
        locked(2, ip, 0, 0x40000000))                           /*pcs*/
            u.u_error = EAGAIN;                                 /*pcs*/
    
    if ((ip->i_mode & IFMT)==IFCHR && ip->i_rdev == 0) {
        register j;
        register struct inode* xp;
        
        for (j=1; j < cdevcnt; j++)
            if (cdevsw[j].d_open == cdevsw[0].d_open)
                break;
        if (j < cdevcnt) {
redo:
            for (xp = &inode[0]; xp < &inode[v.v_inode]; xp++) {
                if ((xp->i_mode & IFMT)==IFCHR && makedev(j,0) == xp->i_rdev) {
                    if (xp->i_flag & ILOCK) {
                        xp->i_flag |= IWANT;
                        sleep((caddr_t)xp, 10);
                        goto redo;
                    }
                    iput(ip);
                    ip = xp;
                    if (ip->i_count == 0) {
                        ip->av_back->av_forw = ip->av_forw;
                        ip->av_forw->av_back = ip->av_back;
                        ip->i_mount->m_count++;
                    }
                    ip->i_flag |= ILOCK;
                    ip->i_count++;
                    break;
                }
            }
        }
    }
    /* ^ pcs */

    if (u.u_error || (fp = falloc(ip, mode & FMASK)) == 0) {
        iput(ip);
        return;
    }
    if (mode & FTRUNC)
        itrunc(ip);

    prele(ip);
    i = u.u_rval1;
    if (setjmp(u.u_qsav)) {
        if (u.u_error == 0)
            u.u_error = EINTR;
        u.u_ofile[i] = 0;
        closef(fp);
    } else {
        openi(ip, mode);
        if (u.u_error==0)
            return;
        u.u_ofile[i] = 0;
        unfalloc(fp);
        plock(ip);
        iput(ip);
    }
}

/*
 * close system call
 */
close()
{
    register struct file *fp;
    register struct a {
        int fdes;
    } *uap;

    uap = (struct a *)u.u_ap;
    fp = getf(uap->fdes);
    if (fp == NULL)
        return;
    u.u_ofile[uap->fdes] = NULL;
    closef(fp);
}

/*
 * seek system call
 */
seek()
{
    register struct file *fp;
    register struct inode *ip;
    register struct a {
        int fdes;
        off_t   off;
        int sbase;
    } *uap;

    uap = (struct a *)u.u_ap;
    fp = getf(uap->fdes);
    if (fp == NULL)
        return;
    ip = fp->f_inode;
    if ((ip->i_mode&IFMT)==IFIFO) {
        u.u_error = ESPIPE;
        return;
    }
    if (uap->sbase == 1)
        uap->off += fp->f_offset;
    else if (uap->sbase == 2)
        uap->off += fp->f_inode->i_size;
    else if (uap->sbase != 0) {
        u.u_error = EINVAL;
        psignal(u.u_procp, SIGSYS);
        return;
    }
    if (uap->off < 0) {
        u.u_error = EINVAL;
        return;
    }
    fp->f_offset = uap->off;
    u.u_roff = uap->off;
}

/*
 * link system call
 */
link()
{
    register struct inode *ip, *xp;
    register struct a {
        char    *target;
        char    *linkname;
    } *uap;

    uap = (struct a *)u.u_ap;
    ip = namei(uchar, 0);
    if (ip == NULL)
        return;
    if (ip->i_nlink >= MAXLINK) {
        u.u_error = EMLINK;
        goto outn;
    }

    if ((ip->i_mode&IFMT)==IFDIR && !suser())
        goto outn;
    /*
     * Unlock to avoid possibly hanging the namei.
     * Sadly, this means races. (Suppose someone
     * deletes the file in the meantime?)
     * Nor can it be locked again later
     * because then there will be deadly
     * embraces.
     * Update inode first for robustness.
     */
    ip->i_nlink++;
    ip->i_flag |= ICHG|ISYN;
    iupdat(ip, &time, &time);
    prele(ip);
    u.u_dirp = (caddr_t)uap->linkname;
    xp = namei(uchar, DONT_FOLLOW | 1);
    if (xp != NULL) {
        iput(xp);
        u.u_error = EEXIST;
        goto out;
    }
    if (u.u_error)
        goto out;
    if (u.u_pdir->i_dev != ip->i_dev) {
        iput(u.u_pdir);
        u.u_error = EXDEV;
        goto out;
    }
    wdir(ip);
out:
    plock(ip);
    if (u.u_error) {
        ip->i_nlink--;
        ip->i_flag |= ICHG;
    }
outn:
    iput(ip);
    return;
}

/*
 * mknod system call
 */
mknod()
{
    register struct inode *ip;
    register struct a {
        char    *fname;
        int fmode;
        int dev;
        int ndev;
    } *uap;
    daddr_t* netxp;                     /*pcs*/

    uap = (struct a *)u.u_ap;
    if ((uap->fmode&IFMT) != IFIFO && !suser())
        return;
    ip = namei(uchar, DONT_FOLLOW | 1);
    if (ip != NULL) {
        iput(ip);
        u.u_error = EEXIST;
        return;
    }
    if (u.u_error)
        return;
    ip = maknode(uap->fmode);
    if (ip == NULL)
        return;
    switch(ip->i_mode&IFMT) {
    case IFCHR:
    case IFBLK:
        ip->i_rdev = (dev_t)uap->dev;
        ip->i_flag |= ICHG;
        break;
    case IFLAN:                             /*pcs*/
        ip->i_rdev = (dev_t)uap->dev;       /*pcs*/
        ip->i_flag |= ICHG;                 /*pcs*/
        netxp = &ip->i_netx;                /*pcs*/
        *netxp++ = major(uap->ndev);        /*pcs*/
        *netxp = minor(uap->ndev) << 16;    /*pcs*/
    }

    iput(ip);
}

/*
 * access system call
 */
saccess()
{
    register svuid, svgid;
    register struct inode *ip;
    register struct a {
        char    *fname;
        int fmode;
    } *uap;

    uap = (struct a *)u.u_ap;
    svuid = u.u_uid;
    svgid = u.u_gid;
    u.u_uid = u.u_ruid;
    u.u_gid = u.u_rgid;
    ip = namei(uchar, 0);
    if (ip != NULL) {
        if (uap->fmode&(IREAD>>6))
            access(ip, IREAD);
        if (uap->fmode&(IWRITE>>6))
            access(ip, IWRITE);
        if (uap->fmode&(IEXEC>>6))
            access(ip, IEXEC);
        iput(ip);
    }
    u.u_uid = svuid;
    u.u_gid = svgid;
}

/*
 * mkdir system call
 */
mkdir() 
{
    register struct a {
        char    *fname;
        int mode;
    } *uap;
    register struct inode *ip;
    register struct inode *xp;
    static struct direct dir[2];

    uap = (struct a *)u.u_ap;
    xp = namei(uchar, 1);
    if (xp == 0) {
        if (u.u_error)
            return;
        ip = u.u_pdir;
        ip->i_count++;
        xp = maknode(uap->mode & 07777 & ~ISVTX | IFDIR);
        plock(ip);
        if (xp == 0) {
            iput(ip);
            return;
        }
        prele(xp);
        bzero(dir, 2*sizeof(struct direct));
        dir[0].d_ino = xp->i_number;
        dir[1].d_ino = ip->i_number;
        dir[0].d_name[0] = dir[1].d_name[0] = '.';
        dir[1].d_name[1] = '.';
        u.u_offset = 0;
        u.u_base = (caddr_t)dir;
        u.u_count = 2*sizeof(struct direct);
        writei(xp);
        if (u.u_error) {
            int errsave = u.u_error;
            u.u_error = 0;
            u.u_dirp = uap->fname;
            plock(xp);
            iput(xp);
            iput(ip);
            rmdir();
            u.u_error = errsave;
            return;
        } else {
            ip->i_nlink++;
            ip->i_flag |= (ICHG|IUPD);
            xp->i_nlink++;
            xp->i_flag |= (ICHG|IUPD);
            iput(ip);
            plock(xp);
            iput(xp);
        }
    } else {
        u.u_error = EEXIST;
        iput(xp);
    }
} 

/*
 * rmdir system call
 */
rmdir()
{
    register struct inode *ip;
    register struct inode *xp;
    register struct direct *dent;
    register daddr_t blk;
    register off_t i, sz;

    struct buf *bp;
    off_t off;
    int ient, xent, d;

    xp = namei(uchar, DONT_FOLLOW | 2);
    if (xp == 0)
        return;
    
    off = u.u_offset;
    if (xp->i_flag & ILAND) goto exit_x;
    if (xp->i_number == u.u_dent.d_ino) {
        u.u_error = EINVAL;
        goto exit_x;
    }
    ip = iget(xp->i_mount, (unsigned)u.u_dent.d_ino);
    if (ip == 0) 
        goto exit_x;

    if ((ip->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        goto exit_ix; 
    }
    if (ip->i_dev != xp->i_dev) {
        u.u_error = EBUSY;
        goto exit_ix;
    }

    xent = ient = 0;
    for (i = 0; i < ip->i_size; ) {
        u.u_offset = i;
        blk = bmap(ip, 1);
        if (u.u_error)
            goto exit_ix;
            
        if (blk < 0) {
            u.u_error = EIO;
            goto exit_ix;
        }
        bp = bread(ip->i_dev, blk);
        dent = (struct direct*)bp->b_un.b_addr;
        sz = ip->i_size-i;
        if (FsBSIZE(ip->i_dev) < sz)
            sz = FsBSIZE(ip->i_dev);
        i + = sz;
        sz /= sizeof(struct direct);
        for (d = 0; d < sz; d++) {
            if (dent[d].d_ino != 0) {
                if (dent[d].d_ino == ip->i_number) {
                    if (dent[d].d_name[0] == '.' &&
                        dent[d].d_name[1] == '\0')
                            ient++;
                    else
                        u.u_error = EINVAL;
                } else {
                    if (dent[d].d_ino == xp->i_number) {
                        if (dent[d].d_name[0] == '.' &&
                            dent[d].d_name[1] == '.' &&
                            dent[d].d_name[2] == '\0')
                                xent++;
                        else
                            u.u_error = EINVAL;
                    } else
                        u.u_error = EEXIST;
                }
            }
        }
        brelse(bp);
    }
    if (u.u_error == 0) {
        if (ient)
            ip->i_nlink--;
        if (xent)
            xp->i_nlink--;
        u.u_offset = off;
        u.u_offset -= sizeof(struct direct);
        u.u_base = (caddr_t)&u.u_dent;
        u.u_count = sizeof(struct direct);
        u.u_dent.d_ino = 0;
        u.u_segflg = 1;
        u.u_fmode = (FWRITE|FSYNC);
        writei(xp);
        if (u.u_error == 0) {
            ip->i_nlink--;
            ip->i_flag |= ICHG;
            xp->i_flag |= ICHG;
        }
    }
exit_ix:
    iput(ip);

exit_x:
    iput(xp);
}



/*
 * getdents system call
 */
getdents() 
{ 
    register struct inode *ip;
    register struct file *fp;
    register struct buf *bp;
    int num; /* the total number of bytes returned */
    struct buf *xbp;
    int bn, i, j;
    int k; /* the total number of bytes to be read from disk */
    int m; /* the total number of bytes returned in temp block */
    int n; /* number of bytes left to be read from disk block */
    long ino;
    int dirsz;
    unsigned short reclen;
    struct dirent *dirent = 0;
    struct direct *dir;
    struct a {
        int fd;
        char *buf;
        unsigned bufsz;
    } *uap;
    char *bufpt;
    unsigned bufsz;
    
    uap = (struct a*)u.u_ap;
    if ((fp = getf(uap->fd)) == 0)
        return;
    ip = fp->f_inode;   
    if ((ip->i_mode & IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        return;
    }
    u.u_offset = fp->f_offset;
    num = 0;
    bufpt = uap->buf;       /* returned buffer */
    bufsz = uap->bufsz;     /* size of returned buffer */

    /* check if on directory entry boundary */
    if ((u.u_offset % sizeof(struct direct)) != 0) {
        u.u_error = ENOENT;
        goto fail;
    }
    u.u_count = ip->i_size; 
    u.u_base = bufpt;
    xbp = geteblk();
    if (u.u_error) {
        brelse(xbp);
        goto fail;
    }
    dirsz = dirent->d_name - (char*)dirent;
    while (u.u_count > 0) {
        bn = bmap(ip, B_READ);
        if (u.u_error) {
            brelse(xbp);
            goto fail;
        }
        if ((n = (unsigned)u.u_pbsize) == 0)
            break;
        k = n;  /* save number to be read */
        bp = bread(ip->i_dev, bn);
        if (u.u_error) {
            brelse(bp);
            brelse(xbp);
            goto fail;
        }
        dir = (struct direct*)(bp->b_un.b_addr + u.u_pboff);
        dirent = (struct dirent*)xbp->b_un.b_addr;
        m = 0;
        /*
         * The following loop fills a block with directory entries.
         */     
        while (n > 0) {
            if ((ino = (long)dir->d_ino) == 0) {
                n -= sizeof(struct direct);
                dir++;
                continue;
            }
            for (i = 0; i < DIRSIZ; i++)
                if (dir->d_name[i] == '\0')
                    break;
            /*
             * Save the name length.
             */
            j = i;
            /*
             * The following rounds the size of a
             * file system independent directory entry up to
             * the next fullword boundary, if it is
             * not already on a fullword boundary.
             */
            reclen = (dirsz + i + NBPW) & ~(NBPW-1);
            if (m + reclen > SBUFSIZE || (num + m + reclen > bufsz)) {
                if (copyout((caddr_t)xbp->b_un.b_addr, u.u_base, m)) {
                    u.u_error = EFAULT;
                    brelse(bp);
                    brelse(xbp);
                    goto fail;
                }
                num += m;
                if ((num + reclen) > bufsz) {
                    u.u_offset += k - n;
                    brelse(bp);
                    /*
                     * Check if there is enough 
                     * room to return at least 
                     * one entry.
                     */
                    if (num == 0) {
                        u.u_error = EINVAL;
                        brelse(xbp);
                        goto fail;
                    }
                    goto out;
                }
                u.u_base += m;
                m = 0;
                dirent = (struct dirent*)xbp->b_un.b_addr;
            } else { /* put_another entry in buffer */
                m += reclen;
                dirent->d_reclen = reclen;
                dirent->d_ino = ino;
                dirent->d_off = u.u_offset + k - n + sizeof(struct direct);
                n -= sizeof(struct direct);
                for (i = 0; i < j; i++)
                    dirent->d_name[i] = dir->d_name[i];
                dirent->d_name[j] = '\0';
                dir++;
                dirent = (struct dirent*)(xbp->b_un.b_addr + m);
            }
        }
        u.u_count -= k - n;
        if (copyout((caddr_t)xbp->b_un.b_addr, u.u_base, m)) {
            u.u_error = EFAULT;
            brelse(bp);
            brelse(xbp);
            goto fail;
        }
        u.u_offset += k - n;
        num += m;
        u.u_base += m;
        brelse(bp);
    }
 
out:
    brelse(xbp);
    if (num > 0)
        fp->f_offset = u.u_offset;
    u.u_rval1 = num;
    return;
    
fail:
    u.u_rval1 = -1;
} 

symlink()
{
    register struct inode *ip;
    register struct a {
        char *target;
        char *linkpath;
    } *uap;
    register char *tgt;
    register i, ch;
    
    uap = (struct a*)u.u_ap;

    if (suser() == 0)
        return;
    
    u.u_dirp = uap->linkpath;
    ip = namei(uchar, DONT_FOLLOW | 1);
    if (ip != 0) {
        iput(ip);
        u.u_error = EEXIST;
        return;
    }
    i = 0;
    tgt = uap->target;
    if (fubyte(tgt) != '/')
        u.u_error = EINVAL;
    for ( ; (ch = fubyte(tgt)) > 0; ++tgt, ++i) {
        if (ch <= ' ' || ch >= 0x7f) {
            u.u_error = EINVAL;
            break;
        }
    }
    if (ch == -1)
        u.u_error = EFAULT;
    if (u.u_error != 0) {
            if (u.u_pdir != 0)
                iput(u.u_pdir);
            return;
    }
    ip = maknode( 0777 | IFLNK);
    if (ip == 0)
        return;
    u.u_offset = 0;
    u.u_base = uap->target;
    u.u_count = i;
    u.u_segflg = 0;
    u.u_fmode = FWRITE;
    writei(ip);
    iput(ip);
}

/*
 * readlink system call
 */
readlink() {
    
    register struct inode *ip;
    register struct a {
        char *fname;
        char *buf;
        unsigned bufsiz;
    } *uap;
    
    uap = (struct a *)u.u_ap;
    ip = namei(uchar, DONT_FOLLOW | 0);
    if (ip == 0)
        return;
    
    if ((ip->i_mode & IFMT) != IFLNK)
        u.u_error = ENXIO;
    else {
        u.u_offset = 0;
        u.u_base = uap->buf; 
        u.u_count = uap->bufsiz;
        u.u_segflg = 0;
        u.u_fmode = FREAD;
        readi(ip);
    }
    iput(ip);
    u.u_rval1 = uap->bufsiz - u.u_count;
}
