/* @(#)ptrace.c 1.2 */
static char* _Version = "@(#) RELEASE:  1.2  Oct 01 1986 /usr/sys/os/ptrace.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include <sys/region.h>
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/seg.h"
#include "sys/var.h"
#include "sys/psl.h"
#include "sys/pfdat.h"

/*
 * Priority for tracing
 */
#define IPCPRI  PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
    int ip_lock;
    int ip_req;
    int *ip_addr;
    int ip_data;
} ipc;

/* pcs: data structures for ptrace cmds -10 and -12 */
struct oexvec ipcvec;
struct fp_vec fpvec;
struct corefil corefil;


/*
 * Enter the tracing STOP state.
 * In this state, the parent is
 * informed and the process is able to
 * receive commands from the parent.
 */
stop()
{
    register struct proc *pp;
    register struct proc *cp;
    register preg_t      *prp;

    /*  Put the region sizes into the u-block for the
     *  parent to look at if he wishes.
     */

    pp = u.u_procp;
    if (prp = findpreg(pp, PT_TEXT))
        u.u_tsize = prp->p_reg->r_pgsz;
    else    
        u.u_tsize = 0;
    if (prp = findpreg(pp, PT_DATA))
        u.u_dsize = prp->p_reg->r_pgsz;
    else
        u.u_dsize = 0;
    if (prp = findpreg(pp, PT_STACK))
        u.u_ssize = prp->p_reg->r_pgsz;
    else
        u.u_ssize = 0;
loop:
    cp = u.u_procp;
    if (cp->p_ppid != 1)
    for (pp = &proc[0]; pp < (struct proc *)v.ve_proc; pp++)
        if (pp->p_pid == cp->p_ppid) {
            wakeup((caddr_t)pp);
            cp->p_stat = SSTOP;
            swtch();
            if ((cp->p_flag&STRC)==0 || procxmt())
                return;
            goto loop;
        }
    exit(fsig(u.u_procp->p_sig));
}

/*
 * sys-trace system call.
 */
ptrace()
{
    register struct proc *p;
    register struct a {
        int req;
        int pid;
        int *addr;
        int data;
    } *uap;

    uap = (struct a *)u.u_ap;
    if (uap->req <= 0) {
        u.u_procp->p_flag |= STRC;
        return;
    }
    for (p=proc; p < (struct proc *)v.ve_proc; p++) 
        if (p->p_stat==SSTOP
         && p->p_pid==uap->pid
         && p->p_ppid==u.u_procp->p_pid)
            goto found;
    u.u_error = ESRCH;
    return;

    found:
    while (ipc.ip_lock)
        sleep((caddr_t)&ipc, IPCPRI);
    ipc.ip_lock = p->p_pid;
    ipc.ip_data = uap->data;
    ipc.ip_addr = uap->addr;
    ipc.ip_req = uap->req;
    p->p_flag &= ~SWTED;
    setrun(p);
    while (ipc.ip_req > 0)
        sleep((caddr_t)&ipc, IPCPRI);
    u.u_rval1 = ipc.ip_data;

    if (ipc.ip_req == -10) /*pcs*/
        copyout(&ipcvec, ipc.ip_addr, VECSIZE); /*pcs*/
    else if (ipc.ip_req == -12) /*pcs*/
/*loc_216:*/
        copyout(&fpvec, ipc.ip_addr, FPVECSIZE); /*pcs*/
    else /*pcs*/
/*loc_23E:*/    
    if (ipc.ip_req < 0)
        u.u_error = EIO;
/*loc_24E:*/
    
    ipc.ip_lock = 0;
    wakeup((caddr_t)&ipc);
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
    register int i;
    register *p;
    int errsave;

    if (ipc.ip_lock != u.u_procp->p_pid)
        return(0);
    i = ipc.ip_req;
    ipc.ip_req = 0;

    errsave = u.u_error; /*pcs*/
    u.u_error = 0; /*pcs*/

    switch (i) {

    /* read user I or D (same here - relic from VAX which had I/D separation */
    case 1:
    case 2:
        ipc.ip_data = fuword((caddr_t)ipc.ip_addr); /*pcs*/
        break;

    /* read user space */
    case 3:
        i = (int)ipc.ip_addr;
        if (i<0 || i >= ptob(USIZE))
            goto error;
        ipc.ip_data = ((sphysadr)&u)->r[i>>1]; /*pcs*/
        
        break;

    /* write user I */
    /* Must set up to allow writing */
    case 4: {
        register preg_t *prp;

        prp = findpreg(u.u_procp, PT_TEXT);
        if (prp && (prp->p_flags & PF_RDONLY)) {
            if (prp->p_reg->r_refcnt != 1 ||
                prp->p_reg->r_flags & RG_NOFREE)
                goto error;
            chgprot(prp, SEG_RW);
        } else
            prp = NULL;
        i = suword((caddr_t)ipc.ip_addr, ipc.ip_data); /*pcs*/
        if (prp)
            chgprot(prp, SEG_RO);
        if (i >= 0) {
            /*
             * The write worked.  Break the disk association
             * of the page since it has been modified.
             * Note: the word can straddle two pages.
             */
            dirtypage((caddr_t)ipc.ip_addr);
            dirtypage(((caddr_t)ipc.ip_addr)+sizeof(short)-1); /*pcs*/
        } else
            goto error;
        break;

    }
    /* write user D */
    case 5:
        if (suword((caddr_t)ipc.ip_addr, ipc.ip_data) < 0)
            goto error;
        break;

    /* write user registers A0-7,D0-7,PC,PS in u */
    case 6: /*pcs*/
        i = (int)ipc.ip_addr;
        if (i >= 0 && i <= 15) /*0..15 = a7,d0-d7,a1-a6*/
            ((long*)u.u_exvec)[i] = ipc.ip_data;
        else if (i==19) /* 19 = ps */
            u.u_exvec->exu.ex1.exstatreg = ipc.ip_data & 0xff;
        else if (i==20) /* 20 = pc */
            u.u_exvec->exu.ex1.expc = ipc.ip_data;
        else
            goto error;
        break;

    /* set signal and continue */
    /* one version causes a trace-trap */
    case 9: /*pcs*/
        u.u_exvec->exu.ex1.exstatreg |= PS_T1;
        goto trace;

    /* set T0 bit and resume child */
    case 11: /*pcs*/
        u.u_exvec->exu.ex1.exstatreg |= PS_T0;
        /*FALLTHRU*/
    /* resume child */
    case 7: /*pcs*/
trace:
        if (((int)ipc.ip_addr & 1) == 0)
            u.u_exvec->exu.ex1.expc = (long)ipc.ip_addr;
        u.u_procp->p_sig = 0L;
        if (ipc.ip_data)
            psignal(u.u_procp, ipc.ip_data);
        wakeup((caddr_t)&ipc);
        return(1);

    /* force exit */
    case 8:
        wakeup((caddr_t)&ipc);
        exit(fsig(u.u_procp->p_sig));

    /* write CPU registers */
    case 10: /*pcs*/
        veccopy(&ipcvec);
        ipc.ip_req = -10;
        break;
    /* write fp registers */
    case 12: /*pcs*/
        if (!u.u_fpsaved) {
            savfp();
            u.u_fpsaved = 1;
        }
        fpveccopy(&fpvec);
        ipc.ip_req = -12;       
        break;

    default:
    error:
        ipc.ip_req = -1;
    }
    wakeup((caddr_t)&ipc);
    return(0);
}

/*pcs*/
veccopy(p)
struct oexvec *p;
{
    bcopy(u.u_exvec, p, 17*sizeof(long)); /* copy a7,d0-d7,a0-a6, exret */
    p->exno = u.u_exvec->exno;
    p->expc = u.u_exvec->exu.ex1.expc;
    p->exstatreg = u.u_exvec->exu.ex1.exstatreg;
    if (u.u_exvec->exno != 2 && u.u_exvec->exno != 3) {
        p->exaccessaddr = 0;
        p->excpustate = 0;
        p->exinstrreg = 0;
    } else {
        p->exaccessaddr = u.u_exvec->exu.ex2.exaccessaddr;
        p->excpustate = u.u_exvec->exu.ex2.exssw;
        p->exinstrreg = _esr; 
    }
}

/*pcs*/
fpveccopy(p)
struct fp_vec *p;
{
    bcopy(&u.u_fpvec, p, FPVECSIZE);
}

/*pcs*/
makecorefil(sig)
{
    veccopy(&corefil.d_exvec);
    fpveccopy(&corefil.d_fpvec);
    
    corefil.d_tsize = ptob(u.u_tsize);
    corefil.d_dsize = ptob(u.u_dsize);
    corefil.d_ssize = ptob(u.u_ssize-1);
    corefil.d_stacktop = SYSVA - NBPP;
    corefil.d_stackbas = corefil.d_stacktop - corefil.d_ssize;
    corefil.d_exdata.dx_mag   = u.u_exdata.ux_mag;
    corefil.d_exdata.dx_tsize = u.u_exdata.ux_tsize;
    corefil.d_exdata.dx_dsize = u.u_exdata.ux_dsize;
    corefil.d_exdata.dx_bsize = u.u_exdata.ux_bsize;
    bcopy(u.u_comm, corefil.d_comm, DIRSIZ);
    bcopy(u.u_psargs, corefil.d_psargs, PSARGSZ);
    corefil.d_start = u.u_start;
    corefil.d_ticks = u.u_ticks; 
    corefil.d_signal = sig;
}

/*
 * Break the disk association of a page based on the
 * virtual address for the current running process.
 * Note: code assumes address is legit.
 */
dirtypage(vaddr)
register caddr_t vaddr;
{
    register tmp;
    register pfd_t *pfd;
    register dbd_t *dbd;
    register preg_t *prp;
    register pte_t *pt;

    prp = findreg(u.u_procp, vaddr);
    tmp = btotp(vaddr - prp->p_regva);
    pt = &prp->p_reg->r_list[tmp/NPGPT][tmp%NPGPT];
    dbd = (dbd_t *)pt + NPGPT;
    pfd = &pfdat[pt->pgm.pg_pfn];
    if (pfd->pf_flags & P_HASH)
        premove(pfd);
    if (dbd->dbd_type == DBD_SWAP)
        swfree1(dbd);
    dbd->dbd_type = DBD_NONE;
}
