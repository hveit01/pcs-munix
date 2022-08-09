/* @(#)utssys.c 6.3 */
static char* _Version = "@(#) RELEASE:  1.3  Jan 21 1986 /usr/src/uts/os/utssys.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/page.h"
#include "sys/buf.h"
#include "sys/file.h"
#include "sys/filsys.h"
#include "sys/inode.h"
#include "sys/mount.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/var.h"
#include "sys/utsname.h"
#include "sys/systm.h"
#include "sys/proc.h"       /*pcs*/
#include "sys/uadmin.h"     /*pcs*/
#include "sys/console.h"    /*pcs*/

extern short stnaddr;

utssys()
{
    register i;
    register struct a {
        int type;       /*pcs*/
        int mv;         /*pcs*/
        char    *cbuf;  /*pcs*/
    } *uap;
    struct {
        daddr_t f_tfree;
        ino_t   f_tinode;
        char    f_fname[6];
        char    f_fpack[6];
    } ust;

    uap = (struct a *)u.u_ap;
    switch(uap->type) {

    case 0:     /* uname */
        if (copyout(&utsname, uap->cbuf, sizeof(struct utsname)))
            u.u_error = EFAULT;
        return;

/* case 1 was umask */

    case 2:     /* ustat */
    {
        for(i=0; i<v.v_mount; i++) {
            register struct mount *mp;
            register struct filsys *fp;

            mp = &mount[i];
            if (mp->m_flags == MINUSE && brdev(mp->m_dev) == brdev(uap->mv)) {
                if (mp->m_bufp == 0) continue;

                fp = mp->m_bufp->b_un.b_filsys;
                ust.f_tfree = FsLTOP(mp->m_dev, fp->s_tfree);
                ust.f_tinode = fp->s_tinode;
                bcopy(fp->s_fname, ust.f_fname, sizeof(ust.f_fname));
                bcopy(fp->s_fpack, ust.f_fpack, sizeof(ust.f_fpack));
                if (copyout(&ust, uap->cbuf, 18))
                    u.u_error = EFAULT;
                return;
            }
        }
        u.u_error = EINVAL;
        return;
    }
/*pcs*/
    case 3: { /* copy eth addr */
            struct ethname  ueth;       /*pcs*/
            ueth.ethaddr = utsname.uiname;
            ueth.stnaddr = stnaddr;
            if (copyout(&ueth, uap->cbuf, sizeof(struct ethname)))
                u.u_error = EFAULT;
            return;
    }
    case 4: /* return u_cdirdev */
        suword(uap->cbuf, u.u_cdirdev);
        return;
    case 5: /* return u_crootdev */
        suword(uap->cbuf, u.u_crootdev);
        return;
    case 6: /* set utsname structure */
        if (suser())
            copyin(uap->cbuf, &utsname, sizeof(struct utsname));
        return;
    case 7:
        suword(uap->cbuf, 0);
        return;
    case 9:
        chconsdev(uap->mv);
        return;
    case 10:
        suword(uap->cbuf, con_type);
        return;
    case 11:
        return;
    case 12:
        if (suser()) {
            u.u_base = (caddr_t)uap->cbuf;
            u.u_count = uap->mv;
            errwrite(0);
        }
        return;
    /*pcs*/
    
    default:
        u.u_error = EFAULT;
    }
}

/*pcs*/
uadmin()
{
    register struct a {
        int cmd;
        int fcn;
        int mdep;
    } *uap;
    static int ualock;
    
    if (ualock || !suser())
        return;
    ualock = 1;
    uap = (struct a*)u.u_ap;
    switch(uap->cmd) {
    case A_SHUTDOWN:
        {
            register struct proc *p = &proc[1]; /*pcs*/
            for (; p < (struct proc*)v.ve_proc; p++) {
                if (p->p_stat == 0)
                    continue;
                if (p != u.u_procp)
                    psignal(p, SIGKILL);
            }
        }
        delay(hz * 2);
        
        panicstr = "SHUTDOWN";
        {
            register struct file *fp;
            register struct inode *ip;
            for (fp = &file[0]; fp < (struct file*)v.ve_file; fp++) {
                if (fp->f_count) {
                    ip = fp->f_up.f_uinode;
                    if ((ip->i_mode & IFMT)==IFREG || (ip->i_mode & IFMT) == IFIFO)
                        closef(fp);
                }
            }
        }
        {
            register struct mount *mp = &mount[0];
            struct inode itmp;
            
            itmp.i_mode = IFBLK;            
            update();

            for (mp = &mount[0]; mp < (struct mount*)v.ve_mount; mp++) {
                if (mp->m_flags == MINUSE) {
                    if (mp->m_bufp == 0)
                        uiumount(mp->m_dev);
                    else if (master==NULL && mp == &mount[0])
                        dlwcsup();
                    else {
                        register struct filsys *fp = getfs(mp);
                        fp->s_fmod = 0;
                        fp->s_time = time;
                        if (fp->s_state == FsACTIVE)
                            fp->s_state = FsOKAY - fp->s_time;
                        itmp.i_rdev = mp->m_dev;
                        u.u_error = 0;
                        u.u_offset = 512;
                        u.u_count = 512;
                        u.u_base = (caddr_t)fp;
                        u.u_segflg = 1;
                        u.u_fmode = FSYNC|FWRITE;
                        writei(&itmp);
                    }
                }
            }
            bdwait();
        }
    case A_REBOOT:
        printf("system shutdown successful\n");
        {   register int dly = 0x170000;
            while (dly-- != 0);
        }
        if (uap->fcn ==AD_HALT)
            prom_warm();
        else
            prom_cold();
        break;
    case A_REMOUNT:
        {
            register struct mount *mp = &mount[0];
            iflush(mp->m_dev);
            binval(mp->m_dev);
            srmount(0);
        }
        break;
    default:
        u.u_error = EINVAL;
    }
    ualock = 0;
}
/*pcs*/
