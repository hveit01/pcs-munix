/* @(#)syscall.c    1.1 */
static char *_Version = "@(#) RELEASE:  1.3  Nov 11 1986 /usr/sys/os/syscall.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/reg.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/sysinfo.h"
#include "sys/psl.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/inode.h" /*pcs*/
#include "sys/tty.h" /*pcs*/
#include "sys/termio.h" /*pcs*/
#include "sys/sxt.h" /*pcs*/
#include "sys/bmt/gdi.h" /*pcs*/
#include "sys/bmt/gdisys.h" /*pcs*/
#include "fcntl.h" /*pcs*/

/*
 * Called from the trap handler when a processor trap occurs.
 */
syscall(regs)
struct exvec *regs;
{
    register struct user *up;
    register struct proc *pp;
    register long *a;
    register uint i;
    
    struct sysent *callp;
    int syst;

    up = &u;
    syst = up->u_stime;
    up->u_exvec = regs;
    a = (long*)(regs->exa6); /* frame pointer */
    sysinfo.syscall++;
    up->u_error = 0;
    if (regs->exno == 46) { /*pcs trapnew 32 bit syscalls */
        i = fulong(a - 8);
        if (i >= sysentlen)
            i = 0;

        callp = &sysent[i];
        if (callp->sy_narg==0 || 
                copyin(a + 2, up->u_arg, callp->sy_narg) == 0) {
                    
            up->u_ap = up->u_arg;
            up->u_dirp = (caddr_t)up->u_ap[0];
            up->u_callno = i;
            up->u_rval1 = 0;

            if (callp->sy_setjmp == 0) {
                if (setjmp(up->u_qsav)) {
                    spl0();
                    if (up->u_error==0) 
                        up->u_error = EINTR;
                } else {
                    if (locate(i)==0)       /* PCS handle MUNIX syscall redirection */
                        (*callp->sy_call)();
                }
            } else {
                if (locate(i)==0)           /* PCS handle MUNIX syscall redirection */
                    (*callp->sy_call)();
            }
        } else
            up->u_error = EFAULT;
        
        if ((i != 59 && u.u_callno != 59) || up->u_error != 0) /*exece*/
                sulong(a - 9, up->u_error);

        if (up->u_error && ++up->u_errcnt > 16) {
            up->u_errcnt = 0;
            runrun++;
        }
        regs->exd0 = up->u_rval1;

    } else {    /* old 16 bit syscalls */

        i = fuword(a - 7);
        if (i >= sysentlen)
            i = 0;

        up->u_callno = i;
        callp = &sysent[i];
        if (callp->sy_narg==0 || 
                sys_convin((short*)a + 4, up->u_arg, callp->sy_fmt) == 0) {

            up->u_ap = up->u_arg;
            up->u_dirp = (caddr_t)up->u_ap[0];
            up->u_rval1 = 0;
            if (callp->sy_setjmp == 0) {
                if (setjmp(up->u_qsav)) {
                    spl0();
                    if (up->u_error==0) 
                        up->u_error = EINTR;
                } else {
                    if (locate(i)==0)
                        (*callp->sy_call)();
                }
            } else {
                if (locate(i)==0)
                    (*callp->sy_call)();
            }
        } else 
            up->u_error = EFAULT;

        if (i != 59 || up->u_error != 0) /*exece*/
            suword(((short*)a) - 15, up->u_error);

        if (up->u_error && ++up->u_errcnt > 16) {
            up->u_errcnt = 0;
            runrun++;
        }
        regs->exd0 = up->u_rval1;
    }

    pp = up->u_procp;
    resetpri(pp, i);

    if (runrun != 0)
        qswtch();
    
    if (pp->p_sig && issig())
        psig(regs->exno);
    if (up->u_prof.pr_scale)
        addupc((caddr_t)regs->exu.ex1.expc, &up->u_prof, (int)(up->u_stime-syst));
}

nosys()
{
    psignal(u.u_procp, SIGSYS);
}

/*
 * Ignored system call
 */
nullsys()
{ 
} 

/*
 * Routine which sets a user error; placed in
 * illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{

    u.u_error = ENODEV;
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{
}

/* convert old 16 bit calls to new API */
sys_convin(arg16, ap, form)
register short *arg16;
int *ap;
char *form;
{ 
    register char *fmt;
    register int  *arg32;
    int rdev;
    int mode;
    int fd;
    struct file *fp;
    struct inode *ip;

    arg32 = ap;
    for (fmt = form; *fmt != 0; fmt++) {
        if (*fmt == 'l') {
            *arg32++ = fulong(arg16);
            arg16 += 2;
        } else if (*fmt == 's') {
            *arg32++ = (short)fuword(arg16);
            arg16 += 1;
        } else if (*fmt == 'u') {
            *arg32++ = (ushort)fuword(arg16);
            arg16 += 1;
        } else if (*fmt == 'e') {
            *arg32++ = fulong(arg16);
            arg16 += 2;
            *arg32++ = fulong(arg16);
            arg16++;
        } else
        switch(u.u_callno) {
        case 45:    /* fcntl */
            switch (arg32[-1]) {
            case F_GETLK:
            case F_SETLK:
            case F_SETLKW:
                fmt = "l"; break;
            default:
                fmt = "s";
            }
            fmt--;
            break;

        case 79:    /* shmsys */
            switch (arg32[-1]) {
            case 0: /* shmat */
                fmt = "sls"; break;
            case 1: /* shmctl */
                fmt = "ssl"; break;
            case 2: /* shmdt */
                fmt = "l"; break;
            case 3: /* shmget */
                fmt = "lls";
            }
            fmt--;
            break;

        case 54:    /*ioctl*/
            rdev = -1;
            fd = u.u_arg[0];
            if (fd >= 0 && fd < 50) {
                fp = u.u_ofile[fd];
                if (fp != 0) {
                    ip = fp->f_inode;
                    mode = ip->i_mode & IFMT;
                    if (mode == IFCHR)
                        rdev = bmajor(ip->i_rdev);
                }
            }
            switch(arg32[-1]) {
            case TCSBRK:
            case TCXONC:
            case TCFLSH:
            case MIOCMOUSE:
            case MIOCTIMEOUT:
            case MIOC_MAP1:
                fmt = "s"; break;
            case SXTIOCLINK:
            case SXTIOCSWTCH:
            case SXTIOCWF:
            case SXTIOCBLK:
            case SXTIOCUBLK:
                if (rdev == 36 || rdev == 8) /* sxt or mxt device */
                    fmt = "s";
                else
                    fmt = "l";
                break;
            default:
                fmt = "l";
            }
            fmt--;
            break;
        
        case 77:    /* msgsys */
            switch (arg32[-1]) {
            case 0: /* msgget */
                fmt = "ls"; break;
            case 1: /* msgctl */
                fmt = "ssl"; break;
            case 2: /* msgrcv */
                fmt = "slsls"; break;
            case 3: /* msgsnd */
                fmt = "slss";
            }
            fmt--;
            break;

        case 78: /*semsys*/
            switch (arg32[-1]) {
            case 0: /* semctl */
                fmt = "sssl"; break;
            case 1: /* semget */
                fmt = "lss"; break;
            case 2: /* semop */
                fmt = "sls";
            }
            fmt--;
            break;

        default:
            panic("sys_convin");
        }
    }

    return u.u_error;
} 
