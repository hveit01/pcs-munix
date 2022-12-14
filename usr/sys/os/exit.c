/* @(#)exit.c   1.3 */
static char* _Version = "@(#) RELEASE:  1.3  Aug 20 1987 /usr/sys/os/exit.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/var.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/debug.h>

/*
 * Release resources.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
exit(rv)
{
    register int i;
    register struct proc *p, *q;
    register int (**fptr)();
    register preg_t *prp;
    register reg_t  *rp;

    p = u.u_procp;
    p->p_flag &= ~(STRC);
    p->p_clktim = 0;
    for (i=0; i<NSIG; i++)
        u.u_signal[i] = 1;
    if ((p->p_pid == p->p_pgrp)
     && (u.u_ttyp != NULL)
     && (*u.u_ttyp == p->p_pgrp)) {
        *u.u_ttyp = 0;
        signal(p->p_pgrp, SIGHUP);
    }
    p->p_pgrp = 0;
    for (i=0; i<NOFILE; i++) {
        if (u.u_ofile[i] != NULL)
            closef(u.u_ofile[i]);
    }
/*pcs*/
    punlock();
/*pcs*/
    plock(u.u_cdir);
    iput(u.u_cdir);
    if (u.u_rdir) {
        plock(u.u_rdir);
        iput(u.u_rdir);
    }

    semexit();
    shmexit();

    acct(rv);
    
/*pcs*/
    if (p->p_munetinfo)
        uiexit(p->p_munetinfo);
/*pcs*/
    /* 
     * free data, stack, text
     * free U block in swtch
     */

    prp = p->p_region;
    while(rp = prp->p_reg){
        reglock(rp);
        if (prp->p_type == PT_STACK) {
            growreg(&u, prp, -(rp->r_pgsz - USIZE), 0, DBD_DZERO);
            regrele(rp);
            prp++;
        } else
            detachreg(&u, prp);
    }

/*pcs*/
    if (u.u_nsegs)
        listfree(u.u_pmap,u.u_nsegs);
/*pcs*/

    p->p_stat = SZOMB;
    ((struct xproc *)p)->xp_xstat = rv;
    ((struct xproc *)p)->xp_utime = u.u_cutime + u.u_utime;
    ((struct xproc *)p)->xp_stime = u.u_cstime + u.u_stime;
    for (q = &proc[1]; q < (struct proc *)v.ve_proc; q++) {
        if (p->p_pid == q->p_ppid) {
            q->p_ppid = 1;
            if (q->p_stat == SZOMB)
                psignal(&proc[1], SIGCLD);
            if (q->p_stat == SSTOP)
                setrun(q);
        } else
        if (p->p_ppid == q->p_pid)
            psignal(q, SIGCLD);
        if (p->p_pid == q->p_pgrp)
            q->p_pgrp = 0;
    }
    swtch();
    /* no deposit, no return */
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 */
wait()
{
    register f;
    register struct proc *p;
/*pcs*/
    register long *uap = u.u_ap;
/*pcs*/
    
loop:
    f = 0;
    for (p = &proc[1]; p < (struct proc *)v.ve_proc; p++)
    if (p->p_ppid == u.u_procp->p_pid) {
        f++;
        if (p->p_stat == SZOMB) {
            freeproc(p, 1);
            return;
        }
        if (p->p_stat == SSTOP) {
            if ((p->p_flag&SWTED) == 0) {
                p->p_flag |= SWTED;
                u.u_rval1 = p->p_pid;
/*pcs*/
                if (*uap) {
                    if (u.u_exvec->exno == EXPATH)
                        sulong((long*)*uap,(fsig(p->p_sig)<<8) | 0177);
                    else
                        suword((short*)*uap, (fsig(p->p_sig)<<8) | 0177);
                }
/*pcs*/             
                return;
            }
            continue;
        }
    }
    if (f) {
        sleep((caddr_t)u.u_procp, PWAIT);
        goto loop;
    }
    u.u_error = ECHILD;
}

/*
 * Remove zombie children from the process table.
 */
freeproc(p, flag)
register struct proc *p;
{
/*pcs*/
    register long* uap = u.u_ap;
/*pcs*/
    if (flag) {
        register n;

        n = u.u_procp->p_cpu + p->p_cpu;
        if (n > 80)
            n = 80;
        u.u_procp->p_cpu = n;
        u.u_rval1 = p->p_pid;
/*pcs*/
        if (*uap) {
            if (u.u_exvec->exno == EXPATH)
                sulong((long*)*uap, *(short*)&p->p_sig);
            else
                suword((short*)*uap, *(short*)&p->p_sig);
        }
/*pcs*/
    }
    u.u_cutime += ((struct xproc *)p)->xp_utime;
    u.u_cstime += ((struct xproc *)p)->xp_stime;
    p->p_stat = NULL;
    p->p_pid = 0;
    p->p_ppid = 0;
    p->p_sig = 0L;
    p->p_flag = 0;
    p->p_wchan = 0;
}

/*pcs*/
pexit()
{
    register i;
    
    for (i=0; i < 50; i++)
        if (u.u_ofile[i])
            closef(u.u_ofile[i]);

    /* Don't include punlock() since not needed for newproc() clean. */

    plock(u.u_cdir);
    iput(u.u_cdir);
    
    if (u.u_rdir) {
        plock(u.u_rdir);
        iput(u.u_rdir);
    }
    
    semexit();
    shmexit();
}
/*pcs*/
