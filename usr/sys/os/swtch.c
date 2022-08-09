/* @(#)swtch.c  1.1 */
static char* _Version = "@(#) RELEASE:  1.2  Oct 21 1986 /usr/sys/os/swtch.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include <sys/region.h>
#include "sys/proc.h"
#include "sys/sysinfo.h"

struct proc *lastproc;
extern int switching;

/*
 * put the current process on
 * the Q of running processes and
 * call the scheduler.
 */
qswtch()
{
    setrq(u.u_procp);
    swtch();
}

/*
 * This routine is called to reschedule the CPU.
 * if the calling process is not in RUN state,
 * arrangements for it to restart must have
 * been made elsewhere, usually by calling via sleep.
 * There is a race here. A process may become
 * ready after it has been examined.
 * In this case, idle() will be called and
 * will return in at most 1HZ time.
 * i.e. its not worth putting an spl() in.
 */
swtch()
{
    register n;
    register struct proc *p, *q, *pp, *pq; /*pcs pq not register*/
    preg_t *prp;
    extern struct user *uservad;

    sysinfo.pswitch++;
    if (save(u.u_rsav)) {
        p = lastproc;
        switch (p->p_stat) {
        case SZOMB:
            /* loc_4A: */
            if ((prp = findpreg(p, PT_STACK)) == 0)
                panic("swtch: findpreg");
            /* loc_6C: */
            reglock(prp->p_reg);
            uaccess(p, prp);
            detachreg(uservad, prp);
            break;
        }
        /* loc_9C: */
        /* loc_A4: */
        if (lastproc != curproc)
            setptbr(u.u_rsav);
        /* loc_C0: */
        return;
    }

    /* loc_C4: */
    if (u.u_m881 && u.u_fpsaved==0) {
        savfp();
        u.u_fpsaved = 1;
    }   
    
    /* loc_E2: */
    switching = 1;
    lastproc = u.u_procp;
loop:
    /* loc_F6: */
    spl6();
    runrun = 0;
    pp = 0;
    q = 0;
    n = 256;
    /*
     * Search for highest-priority runnable process
     */
    if (p = runq) {
        if (panicstr) {
            /* loc_11E: */
            do {
                if ((p->p_flag & SLOAD) && p == curproc) {
                    pp = p;
                    pq = q;
                    n = p->p_pri;
                    break;
                } else
                    /* loc_140: */
                    q = p;
            } while (p = p->p_link);
            /* loc_14A: */
        } else {
            do {
                /* loc_14C: */
                if ((p->p_flag & SLOAD) && p->p_pri <= n) {
                    pp = p;
                    pq = q;
                    n = p->p_pri;
                }
                /* loc_16E: */
                q = p;
            } while (p = p->p_link);
        }
    }
    /*
     * If no process is runnable, idle.
     */ 
    /* loc_178: */
    p = pp;
    if (p == 0) {
        curpri = PIDLE;
        idle();
        goto loop;
    }
    
    /* loc_190: */
    q = pq;
    if (q == 0)
        runq = p->p_link;
    else
        /* loc_1A2: */
        q->p_link = p->p_link;

    /* loc_1A8: */
    curpri = n;
    curproc = p; 
    spl0();
    /*
     * The rsav contents are interpreted in the new address space
     */
    switching = 0;
    p->p_flag &= ~SSWAP;
    if (lastproc == curproc) {
        longjmp(u.u_rsav);
        return;
    }
    /* loc_1E6: */
    resume(p->p_addr, u.u_rsav);
}
