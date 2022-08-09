/* @(#)sys1.c   6.3 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/sys1.c ";

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
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/file.h"
#include "sys/inode.h"
#include "sys/seg.h"
#include "sys/acct.h"
#include "sys/sysinfo.h"
#include "sys/var.h"
#include "sys/ipc.h"
#include "sys/shm.h"

/*
 * exit system call:
 * pass back caller's arg
 */
rexit()
{
    register struct a {
        int rval;
    } *uap;

    uap = (struct a *)u.u_ap;
    exit((uap->rval & 0377) << 8);
}
/*
 * fork system call.
 */
fork()
{
    register a;

    sysinfo.sysfork++;
    /*
     * Disallow if
     *  No processes at all;
     *  not su and too many procs owned; or
     *  not su and would take last slot; or
     *  not su and no space on swap.
     * Part of check done in newproc().
     */
    switch( newproc(1) ) {
        case 1: /* child  -- successful newproc */
            u.u_rval1 = u.u_procp->p_ppid;
            u.u_start = time;
            u.u_ticks = lbolt;
            u.u_mem = u.u_procp->p_size;
            u.u_ior = u.u_iow = u.u_ioch = 0; /**/
            u.u_cstime = 0; /**/
            u.u_stime = 0; /**/
            u.u_cutime = 0; /**/
            u.u_utime = 0;  /**/
            u.u_acflag = AFORK; /**/
            return;
        case 0: /* parent -- successful newproc */
            /* u.u_rval1 = pid-of-child; */
            break;
        default: /* unsuccessful newproc */
            u.u_error = EAGAIN;
            break;
    }
out:
    /* fix PC for child */
    u.u_exvec->exu.ex1.expc += 2; /*pcs*/
}
