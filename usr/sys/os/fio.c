/* @(#)fio.c    6.4 */
static char* _Version = "@(#) RELEASE:  1.1  Aug 20 1986 /usr/sys/os/fio.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/errno.h"
#include "sys/filsys.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/inode.h"
#include "sys/mount.h"
#include "sys/var.h"
#include "sys/acct.h"
#include "sys/sysinfo.h"

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.
 * Only task is to check range of the descriptor.
 */
struct file* getf(f)
register int f;
{
    register struct file *fp;
    
    if (u.u_locate_flag == 1) {         /*pcs*/
        u.u_locate_flag = 0;            /*pcs*/
        return u.u_getf_rv;             /*pcs*/
    }
    
    if (f >= 0 && f < NOFILE) {
        fp = u.u_ofile[f];
        if (fp != 0) {
            if (u.u_rofile[f] == 0)
                return fp;
            if (fp->f_inode->i_flag & ILAND) {
                uidobig(fp->f_inode);
                return fp;
            }
        }
    }
    u.u_error = EBADF;
    return 0;
} 
closef(fp)
register struct file* fp;
{ 
    register struct inode *ip;
    int flag, fmt;
    int vtty1, vtty2;   /*pcs*/
    dev_t dev;
    int opt;            /*pcs*/
    register int (*cfunc)();

    if (fp == 0 || fp->f_count <= 0)
        return;
    cleanlocks(fp);
    unlock(fp->f_inode);
    if ((unsigned)fp->f_count > 1) {
        fp->f_count--;
        return;
    }
    ip = fp->f_inode;
    plock(ip);
    flag = fp->f_flag;
    fp->f_count = 0;
    fp->f_next = ffreelist;         /*pcs*/
    ffreelist = fp;                 /*pcs*/
    dev = (dev_t)ip->i_rdev;
    fmt = ip->i_mode & IFMT;
    vtty1 = fp->f_vtty1;            /*pcs*/ 
    vtty2 = fp->f_vtty2;            /*pcs*/
    fp->f_vtty1 = fp->f_vtty2 = 0;  /*pcs*/

    switch (fmt) {
        
    case IFCHR:
        cfunc = cdevsw[bmajor(dev)].d_close;
        opt = 2;                    /*pcs*/
        break;

    case IFBLK:
        cfunc = bdevsw[bmajor(dev)].d_close;
        opt = 0;                    /*pcs*/
        break;

    case IFLAN:                     /*pcs*/
        break;

    case IFIFO:
        if (vtty1 != F_UNUSED)      /*pcs*/
            free_vtty(vtty1, vtty2); /*pcs*/
        closep(ip, flag);

    default:
        iput(ip);
        return;
    }
    for (fp = file; fp < (struct file*)v.ve_file; fp++) {
        register struct inode *tip;

        if (fp->f_count) {
            tip = fp->f_inode;
            if (dev == tip->i_rdev &&
                (tip->i_mode & IFMT) == fmt)
                goto out;
        }
    }
    if (setjmp(u.u_qsav)) {     /* catch half close */
        plock(ip);                  /*pcs*/
        goto out;
    }
    if (fmt == IFBLK) {
        register struct mount *mp;
        
        for (mp = mount; mp < (struct mount*)v.ve_mount; mp++) {
            if (mp->m_flags == MINUSE &&
                brdev(mp->m_dev) == brdev(dev)) {   /*pcs*/
                (*cfunc)(minor(dev), flag, opt);    /*pcs*/
                goto out;
            }
        }
        bflush(dev);
        (*cfunc)(minor(dev), flag, opt);
        binval(dev);
    } else {
        prele(ip);
        if (cdevsw[major(dev)].d_str)               /*pcs*/
            strclose(ip, flag);                     /*pcs*/
        else                                        /*pcs*/
        if (fmt != IFLAN) 
            (*cfunc)(minor(dev), flag, opt);
        plock(ip);
    }
out:
    iput(ip);
} 

/*
 * openi called to allow handler of special files to initialize and
 * validate before actual IO.
 */
openi(ip, flag) 
register struct inode *ip;
{ 
    dev_t dev;
    register unsigned int maj;

    dev  = (dev_t)ip->i_rdev;
    switch (ip->i_mode & IFMT) {

    case IFCHR:
        maj = bmajor(dev);
        if (maj >= cdevcnt)
            goto bad;
        if (u.u_ttyp == 0)
            u.u_ttyd = dev;
        if (cdevsw[maj].d_str)          /*pcs*/
            stropen(dev, flag, ip);     /*pcs*/
        else                            /*pcs*/
            (*cdevsw[maj].d_open)(minor(dev), flag, 2);
        break;

    case IFBLK:
        maj = bmajor(dev);
        if (maj >= bdevcnt)
            goto bad;
        (*bdevsw[maj].d_open)(minor(dev), flag, 0);
        break;

    case IFLAN:                                 /*pcs*/
        if (bmajor(dev) >= nlandev) goto bad;   /*pcs*/
        break;                                  /*pcs*/

    case IFIFO:
        openp(ip, flag);
        break;
    }
    return;
bad:
    u.u_error = ENXIO;
} 

/*
 * Check mode permission on inode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file
 * system is checked. Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select the owner/group/other fields.
 * The super user is granted all permissions.
 */

access(ip, mode)
register struct inode *ip;
{ 
    register m;

    m = mode;
    if (m == IWRITE) {
        if (getfs(ip->i_mount)->s_ronly) {
            u.u_error = EROFS;
            return 1;
        }
        if (ip->i_flag & ITEXT)
            xrele(ip);
        if (ip->i_flag & ITEXT) {
            u.u_error = ETXTBSY;
            return 1;
        }
    }
    if (u.u_uid == 0)
        return 0;
    if (u.u_uid != ip->i_uid) {
        m >>= 3;
        if (u.u_gid != ip->i_gid)
            m >>= 3;
    }
    if ((ip->i_mode & m) != 0)
        return 0;
    u.u_error = EACCES;
    return 1;
} 

/*
 * Look up a pathname and test if the resultant inode is owned by the
 * current user. If not, try for super-user.
 * If permission is granted, return inode pointer.
 */
struct inode* owner(opt)
{
    register struct inode *ip;
    
    ip = namei(uchar, opt);
    if (ip == 0)
        return 0;
/*loc_5A0: */
    if (u.u_uid == ip->i_uid || suser())
/*loc_5B4: */
        if (getfs(ip->i_mount)->s_ronly)
            u.u_error = EROFS;
/*loc_5CE: */
    if (u.u_error == 0)
        return ip;
/*loc_5DA: */
    iput(ip);
    return 0;
}

/*
 * Test if the current user is the super user.
 */
suser()
{ 
    if (u.u_uid == 0) {
        u.u_acflag |= ASU;
        return 1;
    }
/*loc_608: */
    u.u_error = EPERM;
    return 0;
} 

/*
 * Allocate a user file descriptor.
 */
ufalloc(i)
register i;
{ 
    for (; i < NOFILE; i++) {
/*loc_626: */
        if (u.u_ofile[i] == 0) {
            u.u_rval1 = i;
            u.u_pofile[i] = 0;
            return i;
        }
    }
/*loc_646: */
/*loc_648: */
    u.u_error = EMFILE;
    return -1;
/*loc_65A: */
} 

/*
 * Allocate a user file descriptor and a file structure.
 * Initialize the descriptor to point at the file structure.
 *
 * no file -- if there are no available file structures.
 */
struct file *
falloc(ip, flag)
struct inode *ip;
{
    register struct file *fp;
    register i;
    i = ufalloc(0);
    if (i < 0)
        return 0;
/*loc_680: */
    if ((fp = ffreelist) == 0) {
        printf("file table overflow\n");
        syserr.fileovf++;
        u.u_error = ENFILE;
        return 0;
    }
/*loc_6AA: */
    ffreelist = fp->f_next;
    u.u_ofile[i] = fp;
    fp->f_count++;
    fp->f_flag = flag;              /*pcs*/
    fp->f_inode = ip;
    fp->f_offset = 0;
    fp->f_vtty1 = fp->f_vtty2 = 0;  /*pcs*/
    return fp;
} 

struct file *ffreelist;
finit()
{ 
    register struct file *fp;
    
    for (ffreelist = fp = &file[0]; fp < &file[v.v_file-1]; fp++)
/*loc_6FA: */
        fp->f_next = fp+1;
} 

unfalloc(fp)
register struct file *fp;
{
    if (--fp->f_count <= 0) {
        fp->f_next = ffreelist;
        ffreelist = fp;
    }
/*loc_750: */
} 
