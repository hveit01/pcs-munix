/* @(#)clock.c  1.4 */
static char* _Version = "@(#) RELEASE:  1.4  Oct 21 1986 /usr/sys/os/clock.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/callo.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/var.h>
#include <sys/tuneable.h>
#include <sys/sysmacros.h>
#include <sys/psl.h>
#include "sys/map.h"
#include "sys/swap.h"

/*
 * clock is called straight from
 * the real time clock interrupt.
 *
 * Functions:
 *  reprime clock
 *  implement callouts
 *  maintain user/system times
 *  maintain date
 *  profile
 *  alarm clock signals
 *  jab the scheduler
 */

#define PRF_ON  01
extern  prfstat;

time_t time, lbolt;
int switching;
int vhandcnt;
int bdflushcnt;
uint sxbrkcnt;

clock(pc, ps)
caddr_t pc;
{
    register struct proc *pp;
    register preg_t *prp;
    register reg_t  *rp;
    register a;
    register newpri;
    static short lticks;
    static rqlen, sqlen;
    swpt_t *st;
    static short xflag = 0;
    
    extern char runin;
    extern caddr_t waitloc;
    extern int freemem[]; extern vhand();
    extern void bdflush();
    
    /*
     * if any callout active, update first non-zero time
     */

    if (callout[0].c_func != NULL) {
        register struct callo *cp;
        cp = &callout[0];
        while(cp->c_time<=0 && cp->c_func!=NULL)
            cp++;
        cp->c_time--;
    }
    if (!BASEPRI(ps)) { /*pcs*/
        spl1();
        if (callout[0].c_time <= 0)
            timein();
    }
    if (prfstat & PRF_ON)
        prfintr(pc, ps);
    if (USERMODE(ps)) {
        a = CPU_USER;
        u.u_utime++;    
    } else {
        if (pc == waitloc) {
            if (syswait.iowait+syswait.swap+syswait.physio) {
                a = CPU_WAIT;
                if (syswait.iowait)
                    sysinfo.wait[W_IO]++;
                if (syswait.swap)
                    sysinfo.wait[W_SWAP]++;
                if (syswait.physio)
                    sysinfo.wait[W_PIO]++;
            } else
                if (sxbrkcnt)
                    a = CPU_SXBRK;
                else
                    a = CPU_IDLE;
        } else {
            a = CPU_KERNEL;
            u.u_stime++;
        }
    }
    sysinfo.cpu[a]++;
    pp = u.u_procp;
    if (pp->p_stat==SRUN) {

        /*  Update memory usage for the currently
         *  running process.
         */

        for(prp = pp->p_region ; (reg_t*)SYSVA < (rp = prp->p_reg) ; prp++) {
            if(rp->r_type == RT_PRIVATE)
                u.u_mem += rp->r_nvalid;
            else 
                if (rp->r_refcnt)
                    u.u_mem += rp->r_nvalid/rp->r_refcnt;
        }
    }

    if (!switching && pp->p_cpu < 80)
        pp->p_cpu++;
    lbolt++;    /* time in ticks */

    if (!BASEPRI(ps)) {
        unsigned long ofrmem;
        ofrmem = minfo.freemem[0];
        minfo.freemem[0] += freemem[0];
        if (minfo.freemem[0] < ofrmem)
            minfo.freemem[1]++;
    }
    if (xflag && (lbolt&7)==0)
        runrun++;
    if (--lticks <= 0) {
        if (BASEPRI(ps))
            return;
        lticks += hz;
        time++;
        if ((((short)time) & 31)==0 && haveclock)
            touch_clock();
        runrun++;
        
        minfo.freeswap = 0;
        for(st = swaptab; st < &swaptab[MSFILES]; st++) {
            if(st->st_ucnt == NULL)
                continue;
            minfo.freeswap += st->st_nfpgs;
        }
        xflag = 0;
        rqlen = 0;
        sqlen = 0;
        for(pp = &proc[0]; pp < (struct proc *)v.ve_proc; pp++) {
            if (pp->p_stat) {
                if (pp->p_time != SCHMAX)
                    pp->p_time++;
                if (pp->p_clktim) 
                    if (--pp->p_clktim == 0) 
                        psignal(pp, SIGALRM);
                pp->p_cpu >>= 1;
                if (pp->p_inter) {
                    pp->p_inter--;
                    xflag++;
                }
                
                if (pp->p_pri >= (PUSER-NZERO)) {
                    newpri = pp->p_cpu + (pp->p_inter ? 0 : PINTER) +
                           pp->p_nice + (PUSER-NZERO);
                    if (newpri > PIDLE)
                        newpri = PIDLE;
                    pp->p_pri = newpri;
                    curpri = pp->p_pri;
                }
                    
                if (pp->p_stat == SRUN) {
                    if (pp->p_flag & SLOAD)
                        rqlen++;
                    else
                        sqlen++;
                }
            }
        }
        if (rqlen) {
            sysinfo.runque += rqlen;
            sysinfo.runocc++;
        }
        if (sqlen) {
            sysinfo.swpque += sqlen;
            sysinfo.swpocc++;
        }
                /*
         * Wake up paging process every t_vhandr
         * if memory is running low
         */
        if (--vhandcnt <= 0) {
            vhandcnt = tune.t_vhandr;
            if(freemem[0] < tune.t_vhandl)
                wakeup((caddr_t)vhand);
        }
        
        if (--bdflushcnt <= 0) {
            bdflushcnt = tune.t_bdflushr;
            wakeup((caddr_t)bdflush);
        }

        /*
         * Wakeup sched if
         * memory is tight or someone is not loaded (runin set)
         */
        if (runin) {
            runin = 0;
            wakeup(&runin);
        }
    }
}
