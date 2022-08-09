/* @(#)main.c   6.6 */
static char* _Version = "@(#) RELEASE:  1.1  Jul 16 1986 /usr/sys/os/main.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/filsys.h>
#include <sys/mount.h>
#include <sys/page.h>
#include <sys/inode.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/var.h>
#include <sys/ipc.h>
#include <sys/seg.h>
#include <sys/utsname.h>            /*pcs*/
#include <sys/munet/munet.h>        /*pcs*/
#include <sys/munet/diskless.h>     /*pcs*/

struct inode *rootdir;
int maxmem, physmem;
int maxumem, maxupts;

/*
 * Initialization code.
 * Called from cold start routine as soon as a stack and segmentation
 * have been established.
 * Proc 0 has been partially setup to accomplish above.
 *
 * Functions:
 *  machine dependent device initialization
 *  set process 0 proc[0].p_ and u.u_ values
 *  call all initialization routines
 *  fork - process 0 to schedule
 *       - process 1 execute bootstrap
 *       - process 2 execute pageout (vhand)
 *
 *  proc 0 loops forever
 *  proc 1 returns to caller (start) its start address
 *  proc 2 returns to caller (start) its start address
 */

main()
{
    register int (**initptr)();
    extern int (*init_tbl[])();
    extern icode[], szicode;
    extern icc_boot;                    /*pcs*/
    extern int (*icc_initprocs[])();    /*pcs*/
    extern schar();                     /*pcs*/
    register int i;

    startup();

#if NOTPCS
    printf("\nCopyright (c) 1984 AT&T Technologies, Inc.\n");
    printf("    All Rights Reserved\n\n");
    printf("THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T TECHNOLOGIES, INC.\n");
    printf("The copyright notice above does not evidence any actual or intended\n");
    printf("publication of such source code.\n\n");
#endif
    /*
     * set up system process
     */

    curproc = &proc[0];

    proc[0].p_stat = SRUN;
    proc[0].p_flag |= SLOAD|SSYS;
    proc[0].p_nice = NZERO;

    u.u_cmask = CMASK;
    u.u_limit = CDLIMIT;
    u.u_rdir = NULL;
    u.u_cdirdev = NODEV;        /*pcs*/
    u.u_crootdev = NODEV;       /*pcs*/
    
    
    /*pcs
     *pcs initialize the ICC coprocessor
     *pcs*/
    if (icc_boot) {
        for (initptr = &icc_initprocs[0]; *initptr; initptr++)
            (**initptr)(0);
    }
/* loc_7C: */

    /*
     * initialize system tables and resources
     */

    for (initptr= &init_tbl[0]; *initptr; initptr++) {
        (**initptr)();
    }
/* loc_8A: */
    
    /*
     * get root inode for proc 0 and others
     */
    rootdir = iget(&mount[0], ROOTINO);
    prele(rootdir);                     /*pcs*/
    u.u_cdir = iget(&mount[0], ROOTINO);
    prele(u.u_cdir);                    /*pcs*/
    u.u_rdir = 0;                       /*pcs*/
    u.u_start = time;

    /*
     *  This call should return 0 since the first slot
     *  of swaptab should be used.
     */

    if((comswapsmi = swapadd(swapdev, swplo, nswap)) != 0) /*pcs*/
        panic("startup - swapadd failed");
/* loc_120: */

    
    /*pcs newcastle connection */
    if (master == 0) {
        static char clrootname[17]; /*pcs client node name */
        clrootname[0] = '/';
        bcopy(utsname.nodename, &clrootname[1], 9);
        u.u_locate_flag = 0;
        u.u_dirp = clrootname;
        rootdir = namei(schar, 0);
        if (rootdir == 0) {
            printf("%s e=%d, rd=%4x, crd=%4x\n",
                clrootname, (unsigned)u.u_error,
                rootdev, rmtrootdev);
        }
/* loc_1B0: */

        if (rootdir == 0)
            panic("No root subtree\n");
/* loc_1C6: */

        rootdir->i_flag &= ~ILOCK;
        u.u_cdir = iget(mount, (unsigned)rootdir->i_number);
        u.u_cdir->i_flag &= ~ILOCK;
    }
/* loc_1FA: */


    /*
     * create initial processes
     * start scheduling task
     */

    if (newproc(0)) {
        register preg_t *prp;
        register reg_t *rp;

        rp = allocreg(0, RT_PRIVATE);
        prp = attachreg(rp, &u, 0, PT_DATA, SEG_RW);        
        growreg(&u, prp, btop(szicode), btop(szicode), DBD_DZERO);
        regrele(rp);
        copyout((caddr_t)icode, (caddr_t)0, szicode);
        return 0;
    }
    
    if (newproc(0)) {
        register struct proc *p;
        extern vhand();

/* loc_298: */
        maxmem -= (u.u_ssize + 1);
        p = u.u_procp;
        p->p_flag |= SLOAD|SSYS;
        u.u_utime = 0;                      /*pcs*/
        u.u_cutime = 0;                     /*pcs*/
        u.u_stime = 0;                      /*pcs*/
        u.u_cstime = 0;                     /*pcs*/
        bcopy("vhand", u.u_psargs, 6);      /*pcs*/
        bcopy("vhand", u.u_comm, 5);        /*pcs*/
        return (int)vhand;
    }
/* loc_31A: */

    if (newproc(0)) {                       /*pcs*/
        register struct proc *p;            /*pcs*/
        extern bdflush();                   /*pcs*/

        maxmem -= (u.u_ssize + 1);          /*pcs*/
        p = u.u_procp;                      /*pcs*/
        p->p_flag |= SLOAD|SSYS;            /*pcs*/
        u.u_utime = 0;                      /*pcs*/
        u.u_cutime = 0;                     /*pcs*/
        u.u_stime = 0;                      /*pcs*/
        u.u_cstime = 0;                     /*pcs*/
        bcopy("bdflush", u.u_psargs, 8);    /*pcs*/
        bcopy("bdflush", u.u_comm, 7);      /*pcs*/
        return (int)bdflush;                /*pcs*/
    }                                       /*pcs*/
/* loc_39C: */

    if (newproc(0)) {                       /*pcs*/
        register struct proc *p;            /*pcs*/
        extern reqproc();                   /*pcs*/

        maxmem -= (u.u_ssize + 1);          /*pcs*/
        p = u.u_procp;                      /*pcs*/
        p->p_flag |= SLOAD|SSYS;            /*pcs*/
        p->p_ppid = 1;                      /*pcs*/
        u.u_utime = 0;                      /*pcs*/
        u.u_cutime = 0;                     /*pcs*/
        u.u_stime = 0;                      /*pcs*/
        u.u_cstime = 0;                     /*pcs*/
        bcopy("dlserver", u.u_psargs, 9);   /*pcs*/
        bcopy("dlserver", u.u_comm, 9);     /*pcs*/
        return (int)reqproc;                /*pcs*/
    }                                       /*pcs*/
/* loc_422: */

    bcopy("sched", u.u_psargs, 6);          /*pcs*/
    bcopy("sched", u.u_comm, 5);            /*pcs*/
    sched();
    
    /* FOR LINT */
    return 0;
}
