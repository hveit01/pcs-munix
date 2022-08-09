/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.4  Dec 17 1986 /usr/sys/munet/locate.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/reg.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uisw.h>
#include <fcntl.h>
#include "rbq.h"

/* sysc = systemcall number */
int locate(sysc)
int sysc;
{
    u.u_locate_flag = 0;
    switch(sysc) {
    case UISCHMOD:          /* chmod */
    case UISSTAT:           /* stat */
    case UISUTIME:          /* utime */
    case UISACCES:          /* access */
        return checkpath(0);

    case UISCHOWN:          /* chown */
    case UISLSTAT:          /* lstat */
        return checkpath(DONT_FOLLOW);

    case UISREAD:           /* read */
    case UISLREAD:          /* lread */
    case UISGETDENTS:       /* getdents */
        {
            struct a { int fdes; caddr_t buf; int count; } *uap = (struct a*)u.u_ap;
            if (checkfdes(uap->fdes) == 1) {
                u.u_r.r_val = uap->count - u.u_count;
                u.u_ioch += u.u_rval1;
                sysinfo.readch += u.u_rval1;
                return 1;
            }
            return 0;
        }

    case UISWRITE:          /* write */
    case UISLWRITE:         /* lwrite */
        {
            struct a { int fdes; caddr_t buf; int count; } *uap = (struct a*)u.u_ap;
            if (checkfdes(uap->fdes) == 1) {
                u.u_r.r_val = uap->count - u.u_count;
                u.u_ioch += u.u_rval1;
                sysinfo.writech += u.u_rval1;
                return 1;
            }
            return 0;
        }

    case UISOPEN:           /* open */
        {
            struct a { char *path; int mode; } *uap = (struct a*)u.u_ap;
            return glopen(uap->mode + O_WRONLY);
        }

    case UISCREAT:          /* creat */
        return glopen(O_CREAT|O_TRUNC|O_RDWR);

    case UISCLOS:           /* close */
        return 0;

    case UISSEEK:           /* seek */
        {
            register long *fdes = u.u_ap;
            return checkfdes(*fdes);
        }
 
    case UISLINK:           /* link */
        {
            struct inode *ip;
            register struct inode *ip2;
            register struct a { char *path1; char *path2; } *uap = (struct a*)u.u_ap;
            if ((u.u_namei_rv = namei(uchar, 0)) != 0 && 
                    (u.u_namei_rv->i_flag & ILAND)) {
                ip = u.u_namei_rv;
                iput(ip);
                uap->path1 = u.u_dirp;
                u.u_dirp = uap->path2;
                ip2 = namei(uchar, DONT_FOLLOW | 1);
                if (ip == ip2) {
                    uap->path2 = u.u_dirp;
                    uisend(ip2->i_mount->m_dev, 0);
                    if (u.u_error == EXPATH)
                        u.u_error = EXDEV;
                    iput(ip2);
                    return 1;
                }
                u.u_error = EXDEV;
                if (ip2)
                    iput(ip2);
                else if (u.u_pdir)
                    iput(u.u_pdir);
                return 1;
            } 
            if (u.u_namei_rv != 0)
                prele(u.u_namei_rv);
            u.u_locate_flag = 1;
            return 0;
        }

    case UISMKNOD:          /* mknod */
        {
            register struct a { char *path; int mode; int dev; } *uap 
                = (struct a*)u.u_ap;
            if (checkpath(DONT_FOLLOW | 1) == 1)
                return 1;
            if ((uap->mode & IFMT) != IFIFO && suser()==0) {
                if (u.u_namei_rv)
                    iput(u.u_namei_rv);
                if (u.u_pdir)
                    iput(u.u_pdir);
                return -1;
            }
            if (u.u_namei_rv)
                prele(u.u_namei_rv);
            return 0;
        }
        
    case UISMKDIR:          /* mkdir */
        {
            register struct a { char *path; } *uap = (struct a*)u.u_ap;
            if (checkpath(DONT_FOLLOW | 1) == 1)
                return 1;
            if (u.u_namei_rv)
                prele(u.u_namei_rv);
            return 0;
        }

    case UISSYMLINK:        /* symlink */
        {
            register struct { char *path1; } *dummy;
            register struct a { char *path1; char *path2; } *uap = (struct a*)u.u_ap;

            u.u_dirp = uap->path2;
            if ((u.u_namei_rv = namei(uchar, DONT_FOLLOW | 1)) != 0 &&
                    (u.u_namei_rv->i_flag & ILAND)) {
                uap->path2 = u.u_dirp;
                uisend(u.u_namei_rv->i_mount->m_dev, 0);
                iput(u.u_namei_rv);
                return 1;
            }
            if (suser() == 0) {
                if (u.u_pdir)
                    iput(u.u_pdir);
                if (u.u_namei_rv)
                    iput(u.u_namei_rv);
                return -1;
            }
            if (u.u_namei_rv)
                prele(u.u_namei_rv);
            u.u_locate_flag = 1;
            return 0;
        }
        
    case UISREADLINK:       /* readlink */
        return checkpath(DONT_FOLLOW | 0);

    case UISFSTAT:          /* fstat */
        {
            long *fdes = u.u_ap;
            return checkfdes(*fdes);
        }

    case UISDUP:            /* dup */
        {
            struct a { int fd, fd2; } *uap = (struct a*)u.u_ap;
            int newfd;
            int fd = uap->fd;
            int err;
            int dupflgs = uap->fd & ~0x3f;  /* DUP flags */
            
            uap->fd &= 0x3f;                /* file handle */
            uikilllseek(uap->fd);
            if (checkfdes(uap->fd) == 1) {
                if (u.u_error)
                    return -1;
                if ((dupflgs & 0x40)==0) {
                    if ((newfd = ufalloc(0)) < 0) {
                        err = u.u_error;
                        u.u_r.r_val = u.u_rrarg;
                        close_remfile(uap->fd);
                        u.u_error = err;
                        return -1;
                    }
                } else {
                    newfd = uap->fd2;
                    if (newfd < 0 || newfd > NOFILE) {
                        u.u_error = EBADF;
                        err = u.u_error;
                        u.u_r.r_val = u.u_rrarg;
                        close_remfile(uap->fd);
                        u.u_error = err;
                        return -1;
                    }
                    u.u_r.r_val = newfd;
                }
                
                if (newfd != uap->fd) {
                    if (u.u_ofile[newfd] != 0) {
                        if (u.u_rofile[newfd]) {
                            int rval = u.u_rval1;
                            u.u_rval1 = u.u_rofile[newfd];
                            close_remfile(newfd);
                            u.u_rval1 = rval;
                            u.u_rofile[newfd] = 0;
                        }
                        closef(u.u_ofile[newfd]);
                    }
                    u.u_ofile[newfd] = u.u_getf_rv;
                    u.u_pofile[newfd] = u.u_pofile[uap->fd] & ~1;
                    u.u_getf_rv->f_count++;
                    u.u_rofile[newfd] = u.u_rrarg;
                }
                return 1;
            }
            uap->fd = fd;
            return 0;
        }

    case UISFCNTL:          /* fcntl */
        {
            register struct a { int fd; int cmd; int arg; } *uap 
                = (struct a*)u.u_ap;
            register int newfd;
            int err;

            uikilllseek(uap->fd);
            if (checkfdes(uap->fd) == 0)
                return 0;

            switch (uap->cmd) {
            case F_DUPFD:
                newfd = uap->arg;
                if (newfd < 0 || newfd >= NOFILE) {
                    u.u_error = EINVAL;
                    err = u.u_error;
                    u.u_rval1 = u.u_rrarg;
                    u.u_callno = 254;
                    getf(uap->fd);
                    u.u_error = err;
                    return -1;
                }
                
                if ((newfd = ufalloc(newfd)) < 0) {
                    err = u.u_error;
                    u.u_rval1 = u.u_rrarg;
                    u.u_callno = 254;
                    getf(uap->fd);
                    u.u_error = err;
                    return -1;
                }
                if (u.u_error == 0) {
                    u.u_ofile[newfd] = u.u_getf_rv;
                    u.u_getf_rv->f_count++;
                    u.u_pofile[newfd] = u.u_pofile[uap->fd] & ~1;
                    u.u_rofile[newfd] = u.u_rrarg;
                }
                break;
            case F_GETFD:
            case F_GETFL:
                u.u_rval1 = u.u_rrarg;
                break;
            case F_SETFD:
            case F_SETFL:
            case F_GETLK:
            case F_SETLK:
            case F_SETLKW:
                break;
            default:
                u.u_error = EINVAL;
                break;
            }
            return 1;
        }

    case UISIOCTL:          /* ioctl */
        {
            long *fdes = u.u_ap;
            return checkfdes(*fdes);
        }

    case UISULINK:          /* unlink */
    case UISRMDIR:          /* rmdir */
        return checkpath(DONT_FOLLOW | 2);

    case UISCHDIR:          /* chdir */
        return gldirec(&u.u_cdir);

    case UISCHROT:          /* chroot */
        if (suser() == 0)
            return -1;
        return gldirec(&u.u_rdir);

    case UISSACCT:          /* sysacct */
        {
            register long *arg = u.u_ap;
            static int flag;
            
            if (flag || suser() == 0)
                return -1;
            flag++;

            if (*arg == 0 && acctp != 0) {
                if (acctdev != -1) {
                    uisend(acctdev, 0);
                    acctp = 0;
                    acctdev = -1;
                    flag--;
                    return 1;
                }
            } else {
                if (acctp) {
                    u.u_error = EBUSY;
                    flag--;
                    return -1;
                }
                
                if ((u.u_namei_rv = namei(uchar, 0)) != 0) {
                    if (u.u_namei_rv->i_flag & ILAND) {
                        if (u.u_error) {
                            iput(u.u_namei_rv);
                            flag--;
                            return -1;
                        }
                        acctdev = u.u_namei_rv->i_mount->m_dev;
                        prele(u.u_namei_rv);
                        flag--;
                        return 1;
                    }
                    prele(u.u_namei_rv);
                }
            }
            flag--;
            u.u_locate_flag = 1;
            return 0;
        }
        
    case UISLOCKF:          /* lockf */
        {
            long *fdes = u.u_ap;
            uikilllseek(*fdes);
            return checkfdes(*fdes);
        }
    case UISALLOTHER:
    default:
        return 0;
    }
}   

int checkpath(flag)
int flag;
{
    if ((u.u_namei_rv = namei(uchar, flag)) != 0) {
        if (u.u_namei_rv->i_flag & ILAND) {
            iput(u.u_namei_rv);
            return 1;
        }
    }
    u.u_locate_flag = 1;
    if (u.u_namei_rv)
        prele(u.u_namei_rv);
    return 0;
}

int checkfdes(fd)
int fd;
{
    if ((u.u_getf_rv = getf(fd)) != 0) {
        if (u.u_getf_rv->f_inode->i_flag & ILAND)
            return 1;
    }
    u.u_locate_flag = 1;
    return 0;
}

int glopen(mode)
int mode;
{
    struct file *fp;
    int rval;
    
    if ((mode & (O_WRONLY|O_RDWR)) == 0) {
        u.u_error = EINVAL;
        return -1;
    }

    if (mode & O_CREAT)
        u.u_namei_rv = namei(uchar, 1);
    else if (mode & 040)                /* ??? */
        return 0;
    else
        u.u_namei_rv = namei(uchar, 0);

    if (u.u_namei_rv && (u.u_namei_rv->i_flag & ILAND)) {
        if (u.u_error || (fp = falloc(u.u_namei_rv, mode & 0xff))==0) {
            iput(u.u_namei_rv);
            return -1;
        }
        
        prele(u.u_namei_rv);
        rval = u.u_rval1;
        u.u_pofile[rval] |= u.u_rrflag;
        u.u_rofile[rval] = u.u_rrarg;
        return 1;
    }
    
    if (u.u_namei_rv)
        prele(u.u_namei_rv);
    
    u.u_locate_flag = 1;
    return 0;
}

int gldirec(dir)
struct inode **dir;
{
    if ((u.u_namei_rv = namei(uchar, 0)) && (u.u_namei_rv->i_flag & ILAND)) {
        if (u.u_error)
            iput(u.u_namei_rv);
        else {
            prele(u.u_namei_rv);
            if (*dir) {
                plock(*dir);
                iput(*dir);
            }
            *dir = u.u_namei_rv;
        }
        return 1;
    }
    
    if (u.u_namei_rv)
        prele(u.u_namei_rv);

    u.u_locate_flag = 1;
    return 0;
}

close_remfile(fd)
int fd;
{
    u.u_callno = 254;
    uisend(u.u_ofile[fd]->f_inode->i_mount->m_dev, 0);
}
