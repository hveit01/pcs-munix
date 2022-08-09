/* @(#)exec.c   1.3 */
static char* _Version = "@(#) RELEASE:  1.5  Apr 27 1987 /usr/sys/os/exec.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/buf.h>
#include <sys/inode.h>
#include <sys/acct.h>
#include <sys/sysinfo.h>
#include <sys/seg.h>
#include <sys/reg.h>
#include <sys/var.h>
#include <sys/proc.h>

#include <filehdr.h>
#include <scnhdr.h>
#include <aouthdr.h>

#include <sys/debug.h>

#define NPARGS  btop(NCARGS)

/*
 * exec system call, with and without environments.
 */
struct execa {
    char    *fname;
    char    **argp;
    char    **envp;
};

exece()
{
    execeplus(0);
}

forkexec()
{
    execeplus(1);
}

#define SYSCALL_FORKEXEC    56
#define SYSCALL_EXECE       59
#define SYSCALL_DE          0xDE

execeplus(flag)
int flag;
{
    register unsigned nc;   /* d7 */
    register char* cp;      /* a5 */
    register struct execa* uap; /* a4 */
    register char* psap;
    int na, ne, ucp, ap, c;
    char* args;
    int saveargs;
    int dummy;
    struct inode* ip;
    extern struct inode *gethead();
    caddr_t dsave;                                                      /*pcs*/

    sysinfo.sysexec++;
    u.u_callno = SYSCALL_EXECE;
    u.u_rmt_ino = 0;
    dsave = u.u_dirp;                                                   /*pcs*/
    if (!master)
        u.u_callno = SYSCALL_DE;                                        /*pcs*/

redo:
    if ((ip = gethead()) == 0) {
        if (u.u_error == EINVAL &&
            (unsigned char)u.u_callno == SYSCALL_DE) {
            u.u_error = 0;
            u.u_callno = SYSCALL_EXECE;                                 /*pcs*/
            u.u_rmt_ino = 0;
            u.u_dirp = dsave;
            goto redo;
        } else return;
    }
    
    na = nc = ne = 0;
    uap = (struct execa*)u.u_ap;
    /* collect arglist */
    if ((saveargs = (int)sptalloc(10)) == 0) {                          /*gcc*/
        fixuplan(ip);                                                   /*pcs*/
        iput(ip);
        u.u_error = ENOMEM;
        return;
    }
    args = (char*)saveargs;
    if (uap->argp) {
        for (;;) {
            ap = 0;
            if (uap->argp) {
                ap = fulong((caddr_t)uap->argp);                        /*pcs*/
                uap->argp++;
            }
            if (ap == 0 && uap->envp) {
                uap->argp = 0;
                if ((ap = fulong((caddr_t)uap->envp)) == 0)
                    break;
                uap->envp++;
                ne++;
            }
            if (ap == 0) 
                break;
            na++;
            if (ap == -1) 
                u.u_error = EFAULT;
            do {
                if (nc >= NCARGS-1)
                    u.u_error = E2BIG;
                if ((c = fubyte((caddr_t)ap++)) < 0) 
                    u.u_error = EFAULT;
                if (u.u_error)
                    goto bad;
                nc++;
                *args++ = c;
            }while (c > 0);
        }
    }
    if (flag) {                                                         /*pcs*/
        switch (newproc(2)) {                                           /*pcs*/
        case 1:                                                         /*pcs*/
            u.u_r.r_val = u.u_procp->p_ppid;                            /*pcs*/
            u.u_start = time;                                           /*pcs*/
            u.u_ticks = lbolt;                                          /*pcs*/
            u.u_mem = u.u_procp->p_size;                                /*pcs*/
            u.u_ioch = 0;                                               /*pcs*/
            u.u_iow = 0;                                                /*pcs*/
            u.u_ior = 0;                                                /*pcs*/
            u.u_cstime = 0;                                             /*pcs*/
            u.u_stime = 0;                                              /*pcs*/
            u.u_cutime = 0;                                             /*pcs*/
            u.u_utime = 0;                                              /*pcs*/
            u.u_acflag = 1;                                             /*pcs*/
            break;                                                      /*pcs*/
        case 0:                                                         /*pcs*/
            u.u_callno = SYSCALL_FORKEXEC;                              /*pcs*/
            return;                                                     /*pcs*/
        default:                                                        /*pcs*/
            u.u_error = EAGAIN;                                         /*pcs*/
            goto bad;                                                   /*pcs*/
        }                                                               /*pcs*/
    }
    u.u_callno = SYSCALL_EXECE;                                         /*pcs*/
    nc = (nc + NBPW - 1) & ~(NBPW-1);
    getxfile(ip, nc + NBPW * (na+NBPW));
    if (u.u_error) {
        psignal(u.u_procp, SIGKILL);
        goto bad;
    }
    
    /* copy back arglist */
    
    args = (char*)saveargs;
    psap = u.u_psargs;
    sulong(USRSTACK-NBPW, 0);                                           /*pcs*/
    ucp = USRSTACK-2*NBPW - nc;
    ap = ucp - (na+NBPW)*NBPW;                                          /*pcs*/
    u.u_exvec->exa7 = ap;                                               /*pcs*/
    sulong((caddr_t)ap, 0);                                             /*pcs*/
    ap += NBPW;                                                         /*pcs*/
    suword((caddr_t)ap, na-ne);
    ap -= (NBPW-sizeof(short));                                         /*pcs*/
    nc = 0;
    for (;;) {
        ap += NBPW;
        if (na == ne) {
            sulong((caddr_t)ap, 0);
            while (psap < &u.u_psargs[PSARGSZ])
                *psap++ = 0;
            ap += NBPW;
        }
        if (--na < 0)
            break;
        sulong((caddr_t)ap, ucp);
        do {
            subyte((caddr_t)ucp++, (c = *args++));
            if (psap < &u.u_psargs[PSARGSZ])
                *psap++ = c ? c : ' ';
            nc++;
        } while (c);
    }
    sulong((caddr_t)ap, 0);                                             /*pcs*/
    while (psap < &u.u_psargs[PSARGSZ])
        *psap++ = 0;

    setregs();
    /*fallthru*/                                                        /*pcs*/
bad: 
    fixuplan(ip);                                                       /*pcs*/
    iput(ip);
    sptfree(saveargs, 10, 1);
    u.u_callno = SYSCALL_EXECE;                                         /*pcs*/
}

struct inode* gethead()
{
    register struct inode* ip;
    register unsigned ds, ts;
    
    struct  naout   {
        short   magic;
        short   vstamp;
        long    tsize,
            dsize,
            bsize,
            entry,
            ts,
            ds;
    };

    struct  filhd   {
        unsigned short  magic;
        unsigned short  nscns;
        long        timdat,
                symptr,
                nsyms;
        unsigned short  opthdr,
                flags;
    };

    struct  scnhdr  {
        char        s_name[8];
        long        s_paddr,
                s_vaddr,
                s_size,
                s_scnptr,
                s_relptr,
                s_lnnoptr;
        unsigned short  s_nreloc,
                s_nlnno;
        long        s_flags;
    };
    struct  ehd {
        struct filhd ef;
        struct naout af;
        struct scnhdr sf[4];
    } *ep;
    struct buf* bp;
    daddr_t bn;
    struct oexec {                                                      /*pcs*/
        short a_magic;                                                  /*pcs*/
        long a_text;                                                    /*pcs*/
        long a_data;                                                    /*pcs*/
        long a_bss;                                                     /*pcs*/
        long a_syms;                                                    /*pcs*/
        long a_entry;                                                   /*pcs*/
        long a_stksiz;                                                  /*pcs*/
        short a_flag;                                                   /*pcs*/
    } *ep411;                                                           /*pcs*/
    extern struct buf* readehd();                                       /*pcs*/
    
    if ((ip = namei(uchar, 0)) == 0)
        return 0;
    if ((ip->i_flag & ILAND) == 0 && 
        (access(ip, IEXEC) || 
         (ip->i_mode&IFMT) != IFREG ||
         (ip->i_mode & (IEXEC|(IEXEC>>3)|(IEXEC>>6))) == 0)) {
            u.u_error = EACCES;
            goto bad;
    }
    /*
     * read in first few bytes of file for segment sizes
     * ux_mag = 407/410/413
     *  407 is plain executable
     *  410 is RO text
     *  411 is old MUNIX 1.x file                                         pcs
     *  413 is page-aligned text and data
     *  570 Common object
     *  575 "
     *  set ux_tstart to start of text portion
     */
    /*
     * Read file header assuming new post-5.0 format
     */
    u.u_count = sizeof(struct ehd);
    u.u_offset = 0;
    bp = 0;
    if ((ip->i_flag & ILAND) != 0) {
        if ((bp = readehd(ip)) == 0) 
            goto bad;
    } else {
        bn = bmap(ip, B_READ);
        if (u.u_error) 
            goto bad;
        if (bn < 0) {
            u.u_error = ENOEXEC;
            goto bad;
        }
        bp = bread(u.u_pbdev, bn);
    }
    
    ep = (struct ehd*)bp->b_un.b_addr;
    if (ep->ef.magic == 0570 || ep->ef.magic == 0575) {
        bcopy((caddr_t)&ep->af, (caddr_t)&u.u_exdata, sizeof(struct ufhd));
        u.ux_tstart = sizeof(struct naout) +
            sizeof(struct filhd) + ep->ef.nscns * sizeof(struct scnhdr);
    } else if (ep->ef.magic == 0411) {                                  /*pcs*/
        /* load "old 16 bit executable (MUNIX 1.x) */                   /*pcs*/ 
        ep411 = (struct oexec*)ep;                                      /*pcs*/
        u.u_exdata.ux_mag = 0411;                                       /*pcs*/
        u.u_exdata.ux_txtorg = 0x600000;                                /*pcs*/
        if (ep411->a_text <= 0x200000)                                  /*pcs*/
            u.u_exdata.ux_datorg = 0x800000;                            /*pcs*/
        else                                                            /*pcs*/
            u.u_exdata.ux_datorg = stob(btos(ep411->a_text)) + 0x600000;/*pcs*/
        u.u_exdata.ux_entloc = ep411->a_entry;                          /*pcs*/
        u.u_exdata.ux_bsize = ep411->a_bss;                             /*pcs*/
        u.u_exdata.ux_dsize = ep411->a_data;                            /*pcs*/
        u.u_exdata.ux_tsize = ep411->a_text;                            /*pcs*/
        u.ux_tstart = sizeof(struct oexec);                             /*pcs*/
    } else {
        bcopy((caddr_t)&ep->ef, (caddr_t)&u.u_exdata, sizeof(struct ufhd));
        u.u_exdata.ux_txtorg = 0;
        u.u_exdata.ux_datorg = (u.u_exdata.ux_tsize + NBPP - 1) & ~(NBPP - 1);
        u.ux_tstart = sizeof(u.u_exdata);
    }
    brelse(bp);
    u.u_segflg = 0;
    /*
     * Nonshared 407's: A vestige from the past
     */
    if (u.u_exdata.ux_mag == 0407) {
        ds = btop((long)u.u_exdata.ux_tsize +
             (long)u.u_exdata.ux_dsize +
             (long)u.u_exdata.ux_bsize);
        ts = 0;
        u.u_exdata.ux_dsize += u.u_exdata.ux_tsize;
        u.u_exdata.ux_tsize = 0;
        u.u_exdata.ux_datorg = u.u_exdata.ux_txtorg;
    } else if (u.u_exdata.ux_mag == 0410 || u.u_exdata.ux_mag == 0411 ||
               u.u_exdata.ux_mag == 0413) {                             /*pcs*/
        ts = btop(u.u_exdata.ux_tsize);
        ds = btop(u.u_exdata.ux_dsize + u.u_exdata.ux_bsize);
        
        if ((ip->i_flag & ILAND) != 0) {
            if (u.u_exdata.ux_mag == 0413) 
                u.u_exdata.ux_mag = 0410;
        } else {
            if ((ip->i_flag & ITEXT) == 0 && ip->i_count != 1) {
                register struct file* fp;
            
                for (fp = &file[0]; fp < (struct file*)v.ve_file; fp++) {
                    if (fp->f_count != 0 && fp->f_inode == ip &&
                        (fp->f_flag & FWRITE)) {
                        u.u_error = ETXTBSY;
                        goto bad;
                    }
                }
            }
            ip->i_flag |= ITEXT;
        }
    } else {
        u.u_error = ENOEXEC;
        goto bad;
    }
    chksize(ts, ds, SSIZE + btop(NCARGS - 1));
bad: 
    if (u.u_error) {
        fixuplan(ip);                                                   /*pcs*/
        ip->i_flag &= ~ITEXT;                                           /*pcs*/
        iput(ip);
        ip = 0;
    }
    return ip;
}

/*
 * Read in and set up memory for executed file.
 */
getxfile(ip, nargc)
register struct inode* ip;
{
    register size, npgs;
    register t;                                                         /*pcs unused*/
    int base;                                                           /*pcs*/
    register reg_t* rp;
    register preg_t* prp;
    struct proc* p;
    dbd_t*  dbd;                                                        /*pcs unused*/
    int rgva;
    int offset;
    int (**fptr)();                                                     /*pcs unused*/
    pte_t* pt;                                                          /*pcs unused*/
    long* ptr;                                                          /*pcs*/
    
    shmexec();
    punlock();
    u.u_prof.pr_scale = 0;
    u.u_m881 = 1;                                                       /*pcs*/
    p = u.u_procp;
    
    /*  Free the processes current regions.  Note that
     *  detachreg compacts the region list in the proc
     *  table.
     */
    
    prp = p->p_region;
    while (rp = prp->p_reg) {
        reglock(rp);
        if (prp->p_type == PT_STACK) {
            growreg(&u, prp, -(rp->r_pgsz - USIZE), 0, DBD_DZERO);
            regrele(rp);
            prp++;
        } else
            detachreg(&u, prp);
    }
    clratb();                                                           /*pcs*/

    /*
     *  the process can no longer deal for itself
     */
    for (ptr = &u.u_signal[0]; ptr < &u.u_signal[NSIG]; ptr++)
        *ptr &= 1;                                                      /*pcs*/
    
    u.u_datorg = u.u_exdata.ux_datorg;                                  /*pcs*/
    if (u.u_exdata.ux_mag == 0407) {                                    /*pcs*/
        if (get407(ip) < 0)                                             /*pcs*/
            goto out;
        else
            goto stack;
    }
    
    /*  Load text region
     */

    if (xalloc(ip) == -1)
        goto out;
        
    base = u.u_exdata.ux_datorg;
    size = u.u_exdata.ux_dsize;
    rgva = base & ~SOFFMASK;
    
    if ((rp = allocreg(ip, RT_PRIVATE)) == 0)
        goto out;

    /*  Attach the data region to this process.
     */

    if ((prp = attachreg(rp, &u, rgva, PT_DATA, PTE_KR|PTE_KW)) == 0) {
        freereg(rp);
        goto out;
    }
    
    if (size) {
        offset = u.ux_tstart + u.u_exdata.ux_tsize;
        if (u.u_exdata.ux_mag == 0413) {
            if (mapreg(prp, base, ip, offset, size) < 0) {
                detachreg(&u, prp);
                goto out;
            }
        } else {
            if (loadreg(prp, base, ip, offset, size) < 0) {
                detachreg(&u, prp);
                goto out;
            }
        }
    }
    npgs = btop(soff(base) + size + u.u_exdata.ux_bsize) - btop(soff(base) + size);
    if (npgs) {
        if (growreg(&u, prp, npgs, 0, DBD_DZERO) < 0) {
            detachreg(&u, prp);
            goto out;
        }
    }
    regrele(rp);
    
    /*  Grow the stack.
     */
    
stack:
    prp = findpreg(p, 3);
    reglock(prp->p_reg);
    npgs = SSIZE + btop(nargc);
    if (growreg(&u, prp, npgs, 0, DBD_DZERO) < 0) {
        regrele(prp->p_reg);
        goto out;
    }
    regrele(prp->p_reg);

    /*
     * set SUID/SGID protections, if no tracing
     */
    
    if ((u.u_procp->p_flag & STRC) == 0) {
        if (ip->i_flag & IREAD) {                                       /*pcs*/
            if (readowner(ip) < 0)                                      /*pcs*/
                return;                                                 /*pcs*/
        } else {                                                        /*pcs*/
            if (ip->i_mode & ISUID)
                u.u_uid = ip->i_uid;
            if (ip->i_mode & ISGID)
                u.u_gid = ip->i_gid;
        }                                                               /*pcs*/
        u.u_procp->p_suid = u.u_uid;
    } else
        psignal(u.u_procp, SIGTRAP);
    
    return;
    
out:
    ip->i_flag &= ~ITEXT;
    u.u_error = ENOEXEC;
}

get407(ip)
struct inode* ip;
{
    int size, org, npgs;
    preg_t* prp;
    reg_t* rp;

    /*
     *  load the text
     */
    size = u.u_exdata.ux_dsize;
    if ((rp = allocreg(ip, RT_PRIVATE)) == 0)
        return -1;
        
    org = u.u_exdata.ux_datorg;
    if ((prp = attachreg(rp, &u, 0, PT_DATA, SEG_RW)) == 0) {
        freereg(rp);
        return -1;
    }
    
    if (loadreg(prp, org, ip, u.ux_tstart, size) < 0) {
        detachreg(&u, prp);
        return -1;
    }
    
    /*
     *  allocate bss as demand zero
     */
    npgs = btop(size + u.u_exdata.ux_bsize) - btop(size);
    if (npgs) {
        if (growreg(&u, prp, npgs, 0, DBD_DZERO) < 0) {
            detachreg(&u, prp);
            return -1;
        }
    }
    regrele(rp);
    return 0;
}
