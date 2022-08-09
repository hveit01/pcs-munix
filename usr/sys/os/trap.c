/* @(#)trap.c   6.3 */
static char *_Version = "@(#) RELEASE:  2.0  May 14 1987 /usr/src/uts/os/trap.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/reg.h"
#include "sys/psl.h"
#include "sys/seg.h"
#include "sys/sysinfo.h"
#include "sys/sysmacros.h"

#include "sys/debug.h"

static int mina7 = 0x7fffffff;

typedef void (*inthndlr)();

extern char nprinterr;

/* Called with the current USP value which points to the complete register set
 * (struct exvec).
 * The members exret and exno point to the real int handler and its vector#
 */
eaintr(usp)
{
    register struct exvec *regs = (struct exvec *)&usp;
    register inthndlr inth = (inthndlr)regs->exret;
    int dummy; /*pcs unused*/

    /* call the int handler */
    (*inth)(regs->exno);
    
    /* if in user mode, do context switching */
    if (USERMODE(regs->exu.ex1.exstatreg) && runrun) {

        /* save current registers */
        u.u_exvec = regs;
        
        /* clear FP save flag */
        u.u_fpsaved = 0;
        
        /* switch context */
        qswtch();
        
        /* has the new process used FP registers? yes, restore them */
        if (u.u_fpsaved)
            restfp();
    }
}

/* since the EXSSW bits seem to be defined nowhere, make handy names */
#define SSW_FC      0x8000
#define SSW_FB      0x4000
#define SSW_RC      0x2000
#define SSW_RB      0x1000
#define SSW_DF      0x0100
#define SSW_RM      0x0080
#define SSW_RW      0x0040
#define SSW_SZMASK  0x0030
#define     SSW_SZ0 0x0000
#define     SSW_SZ1 0x0010
#define     SSW_SZ2 0x0020
#define     SSW_SZ3 0x0030
#define SSW_FCMASK  0x0007
#define     SSW_FCS 0x0004
#define     SSW_FCU 0x0000
#define     SSW_FCD 0x0001
#define     SSW_FCP 0x0002
#define     SSW_FCC 0x0007

/*
 * Called from the trap handler when a processor trap occurs.
 */
trap(usp)
{ 
    register struct exvec *regs = (struct exvec *)&usp;
    register struct proc *p;
    register int pc;
    register int usermode;
    register short oprio;
    int dummy; /*pcs unused */
    int reply;
    int esrsave;
    int exssw;

    if (usermode = USERMODE(regs->exu.ex1.exstatreg)) {
        u.u_fpsaved = 0;
        u.u_exvec = regs;
    }
    switch (regs->exno) {
    case 2: /* bus error*/
        esrsave = (int)_esr;
        exssw = regs->exu.ex2.exssw;
        if ((esrsave & 0x7100) != 0) {
            systemtrap(regs);   /*pcs bug? called with 1 arg only! */
            return;
        }
        if (usermode || (exssw & SSW_FCS) == 0) {
            ulong fltaddr;
            int isflt;
            int dummy2; /*pcs unused */
            long expc;
            long expc2;
        
            if ((exssw & (SSW_FC|SSW_FB)) != 0) {
                if (regs->exu.ex2.exfmtvec == 0xA008) {
                    expc = regs->exu.ex2.expc + 4;
                    expc2 = regs->exu.ex2.expc + 2;
                } else {
                    expc = regs->exu.ex2.stageb;
                    expc2 = expc - 2;
                }
                if ((exssw & (SSW_FC|SSW_FB)) == SSW_FC)
                    fltaddr = expc2;
                else if ((exssw & (SSW_FC|SSW_FB)) == SSW_FB)
                    fltaddr = expc;
                else {
                    if (btotp(expc) != btotp(expc2))
                        fsword(expc);
                    fltaddr = expc2;
                }
            } else
                fltaddr = regs->exu.ex2.exaccessaddr;

            if (fltaddr == 0xEFFFFC) {
                sulong(USRSTACK-4, regs->exu.ex2.dob);
                regs->exu.ex2.exssw &= ~SSW_DF;
                return;
            }
            if ((esrsave & 0x4) != 0)       /* page not present */
                isflt = pfault(fltaddr);
            else if ((esrsave & 0x1) != 0)
                isflt = vfault(fltaddr);
            else if ((esrsave & 0x28) != 0 || fltaddr >= (ulong)&u)
                isflt = 0;
            else
                isflt = 0;
            if (isflt > 0) {
                if (usermode && regs->exu.ex2.expc < SYSVA)
                    goto setpri;
                return;
            }
            if (fltaddr > 0x3f500000 && fltaddr < (SYSVA - ptob(u.u_ssize))) {
                if (grow(fltaddr) != 0) {
                    if (usermode)
                        goto setpri;
                    return;
                }
            }
        }
        /*FALLTHRU*/
    case 3: /* address error */
        if (usermode)
            usertrap(regs, esrsave);
        else {
            systemtrap(regs, esrsave);
            return;
        }
        break;

    case 45:    /* synthetic: trapold = 68010 system calls */
    case 46:    /* synthetic: trapnew = 68020 system calls */
        if (usermode) {
            syscall(regs);
            goto done;
        }
        /*FALLTHRU*/

    case 4:     /* illegal instr */
    case 5:     /* division by zero */
    case 6:     /* chk instruction */
    case 7:     /* trapv instruction */
    case 8:     /* privilege violation */
    case 9:     /* trace exception */
    case 10:    /* A emulator trap */
    case 11:    /* F emulator trap */
    case 12:    /* reserved */  
    case 44:    /* synthetic: trapill */
    case 47:    /* synthetic: trapmon */
    case 48:    /* FP exception */
        if (usermode) {
            usertrap(regs, 0);
            break;
        }
        /*FALLTHRU*/

    case 0:     case 1:     case 13:    case 14:
    case 15:    case 16:    case 17:    case 18:
    case 19:    case 20:    case 21:    case 22:
    case 23:    case 24:    case 25:    case 26:
    case 28:    case 29:    case 31:    case 32:
    case 33:    case 34:    case 35:    case 36:
    case 37:    case 38:    case 39:    case 40:
    case 41:    case 42:    case 43:
    default:
        systemtrap(regs, _esr);
        return;

    case 27: /* interrupt lvl 3 (clock) */
        pc = regs->exu.ex1.expc;
        clock(pc, regs->exu.ex1.exstatreg);
        if (usermode) {
            if (u.u_prof.pr_scale) {
                spl0();
                addupc(pc, &u.u_prof, 1);
            }
            if (runrun)
                break;
        }
        return;

    case 30: /* interrupt lvl 6 */

        {   char nprt = nprinterr;
            nprinterr = 0;
            printf("\nhalt");
            while ((_pcr &0x100) != 0);
            systemtrap(regs, _esr);
            do {
                printf("\ncontinue (y/n) ? ");
                splhi();
                reply = getchar();
                putchar('\n');
                if (reply == 'y') {
                    nprinterr = nprt;
                    return;
                }
            } while (reply != 'n');
            dump();
            prom_warm();
        }
        break;
    }

    p = u.u_procp;
    if (p->p_sig && issig())
        psig(regs->exno);

setpri:
    p = u.u_procp;
    oprio = p->p_pri;
    calcppri(p);
    if (oprio != p->p_pri)
        runrun++;
    
    if (runrun)
        qswtch();
done:
    if (u.u_fpsaved)
        restfp();
} 

static int stkovflo;    /* seems to be unused */

systemtrap(regs, esr)
register struct exvec *regs;
{
    register long pc;
    extern long beginuser, enduser, erret;
    extern long f_start, f_end;
    int dummy1, dummy2; /*pcs unused */
    static int dblflt;
    
    if (regs->exno==2 || regs->exno == 3) {
        pc = regs->exu.ex2.expc;
        if (pc >= (long)&beginuser && pc < (long)&enduser) {
            regs->exu.ex2.expc = erret;
            regs->exu.ex2.dummy0 =
                (regs->exu.ex2.exfmtvec & 0xf000)== 0xb000;
            regs->exu.ex2.exfmtvec = 0;
            if ((regs->exu.ex2.exssw & SSW_FCS) == 0)
                u.u_error = EFAULT;
            return;
        } if (pc >= (long)&f_start && pc < (long)&f_end) {
            usertrap(regs, esr);
            return;
        }
    } 
    if (regs->exno==9)
        return 0;

    if (dblflt++ > 1) {
        printf("error in pmd\n");
        prom_warm();
    }

    nprinterr = 0;
    printmessage(regs, esr);
    spl0();
    update();
    smallpmd(regs);
    dblflt = 0;

    if (regs->exno != 30) {
        dump();
        prom_warm();
    }
}

usertrap(regs, esr)
register struct exvec *regs;
{
    register signo;
    register long pc;
    int instr;
    register preg_t *prp;
    
    
    switch (regs->exno) {
    case 2:
    case 3:
        regs->exu.ex2.dummy0 =
            (regs->exu.ex2.exfmtvec & 0xf000)== 0xb000;
        regs->exu.ex2.exfmtvec = 0;
        pc = regs->exu.ex2.expc;
        signo = regs->exno==3 ? SIGADDR :
            ((esr & 8) != 0) ? SIGBUS : SIGSEGV;
        break;

    case 4:
    case 10:
    case 11:
        signo = SIGILL; 
        break;

    case 12:
    case 48:
        signo = SIGFPE;
        break;

    case 5:
        signo = SIGZERO;
        break;

    case 6:
        signo = SIGCHK;
        break;

    case 7:
        signo = SIGOVER;
        break;

    case 8:
        pc = regs->exu.ex2.expc;
        if (((instr = fuword(pc)) & 0xFFC0) == 0x40c0) {    /* MOVE FROM SR */
            if ((prp = findpreg(u.u_procp, 1)) != 0 &&
                ((prp->p_flags & PF_RDONLY) != 0)) {
                if (prp->p_reg->r_refcnt != 1 ||
                    (prp->p_reg->r_flags & RG_NOFREE) != 0)
                    goto priverr;
                chgprot(prp, 3);
            } else
                    prp = 0;

            signo = suword(pc, instr | 0x200); /* patch instruction to MOVE FROM CCR */
            if (prp != 0)
                chgprot(prp, 1);

            if (signo >= 0) {   /* signo was abused for return code of suword */
                dirtypage(pc);
                clrca();
                return;
            }
        }
priverr:
        signo = SIGPRIV;
        break;

    default:
        if (regs->exno == 44) {
            nosys();
            return;
        } else if (regs->exno != 47) {
            printf("undefined user exception\n");
            printmessage(regs, esr);
            return;
        }
        /*FALLTHRU*/

    case 9:
        signo = SIGTRAP;
        regs->exu.ex2.exstatreg &= ~(PS_T0|PS_T1);
        break;
    }

    psignal(u.u_procp, signo);
} 

smallpmd(regs)
struct exvec *regs;
{
    register long *fp;
    register dummy; /*pcs unused*/
    register short k, n;
    ulong retpc;

    printf("\nprocedure return addresses:\n");
    fp = (long*)regs->exa6;
    for (k = 0, retpc = 0, n = 20; n > 0; n--) {
        if (fp == 0)
            break;
        if (((long)fp & 1) != 0) {
            printf("\nframe pointer error:%X\n", fp);
            break;
        }
        retpc = fslong(fp+1);
        if (retpc >= (USRSTACK-0x400000) && retpc < USRSTACK) {
            fp++;
            retpc = fslong(fp+1);
        }
        if (retpc == 0)
            break;
        if ((retpc & 1) != 0) {
            printf("\nreturn address error:%8lx\n", retpc);
            break;
        }
        printf("%8lx ", retpc);
        if (retpc == 0xffffffff)
            break;
        if (++k == 8) {
            printf("\n");
            k = 0;
        }

        fp = (long*)fslong(fp);
    }

    if (n == 0)
        printf("\npmd aborted after 20 calls");
    printf("\n");
} 

printmessage(regs, esr) 
register struct exvec *regs;
{ 
    char *errmsg;
    short s;

    s = splhi();
    
    if ((regs->exu.ex2.exstatreg & PS_S) != 0)
        regs->exa7 = (long)regs + veclen(regs);
    
    printf("\nregisters:\n");
    printf("A0 %8x  A1 %8x  A2 %8x  A3 %8x\n",
        regs->exa0, regs->exa1, regs->exa2, regs->exa3);
    printf("A4 %8x  A5 %8x  A6 %8x  US %8x\n",
        regs->exa4, regs->exa5, regs->exa6, regs->exa7);
    printf("D0 %8x  D1 %8x  D2 %8x  D3 %8x\n",
        regs->exd0, regs->exd1, regs->exd2, regs->exd3);
    printf("D4 %8x  D5 %8x  D6 %8x  D7 %8x\n",
        regs->exd4, regs->exd5, regs->exd6, regs->exd7);

    switch (regs->exno) {
    case 2:
    case 3:
        printf("%s error, access address = %8x, pc = %8x fmt=%4x ssw = %4x,\nsr = %4x, esr=%4x ",
            regs->exno == 2 ? "bus" : "address",
            regs->exu.ex2.exaccessaddr,
            regs->exu.ex2.expc,
            regs->exu.ex2.exfmtvec,
            regs->exu.ex2.exssw,
            regs->exu.ex2.exstatreg,
            esr);
        printesr(esr);
        splx(s);
        return;

    case 31:
        printf("Interrupt 7 (NMI), pc = %8x, sr = %4x, esr=%4x ",
            regs->exu.ex1.expc,
            regs->exu.ex1.exstatreg,
            esr);
        printesr(esr);
        splx(s);
        return;

    case 4:
        errmsg = "illegal opcode";
        break;

    case 10:
        errmsg = "illegal 1010 opcode";
        break;

    case 11:
        errmsg = "illegal 1111 opcode";
        break;

    case 13:
        errmsg = "coprocessor protocol violation";
        break;

    case 5:
        errmsg = "zero divide";
        break;

    case 6:
        errmsg = "check error";
        break;

    case 7:
        errmsg = "trapv error";
        break;

    case 8:
        errmsg = "privileged opcode";
        break;

    case 45:
    case 46:
        errmsg = "trap instr";
        break;

    case 0:
        printf("illegal vector interrupt:exception %d., exfmtvec=%x\n",
            regs->exu.ex2.exfmtvec >> 2, regs->exu.ex2.exfmtvec);
        errmsg = "";
        break;

    default:
        printf("exception %d", regs->exno);
        errmsg = "";
        break;
    }

    printf("%s, status reg =%x, pc = %X\n",
        errmsg, regs->exu.ex2.exstatreg, regs->exu.ex2.expc);

    splx(s);
}

printesr(esr)
register int esr;
{
    printf("(");
    if (esr & 1)
        printf(" NOT_VALID");
    if (esr & 2)
        printf(" NOT_EXEC");
    if (esr & 4)
        printf(" NOT_WRITE");
    if (esr & 8)
        printf(" INVALID_ADDRESS");
    if (esr & 0x10)
        printf(" NOT_USER");
    if (esr & 0x20)
        printf(" MEGAPAGEFAULT");
    if (esr & 0x40)
        printf(" CPU_TIMEOUT");
    if (esr & 0x100)
        printf(" P_BUS_PARITY");
    if (esr & 0x1000)
        printf(" DMA_TIMEOUT");
    if (esr & 0x2000)
        printf(" DMA_MMU_ERROR");
    if (esr & 0x4000)
        printf(" Q_BUS_PARITY");
    printf(" )\n");
}

/* expects a exvec stack frame */
whereami(fmt, a1, a2, a3)
char *fmt;
{
    struct exvec regs;
    long *cf; /*c4*/
    
    cf = (long*)&fmt;       /* point to address of 1st arg on stack */
    cf -= 2;                /* skip over retpc and saved fp */
    regs.exa6 = (long)cf;   /* set the frame pointer */

    printf(fmt, a1, a2, a3); /* print arguments */
    smallpmd(&regs);        /* write registers */
}

ftrap(fpv)
struct fpe_vec *fpv;
{ 
    struct exvec regs;
    
    cpfvec(fpv, &regs);
    regs.exret = (long)fpv;
    regs.exno = 11;

    u.u_exvec = &regs;
    if (fpv->exstatreg & PS_S)
        u.u_signal[SIGFPE] = 0;

    psignal(u.u_procp, SIGFPE);
    if (issig())
        psig();
}

cpfvec(from, to)
register struct fpe_vec *from;
register struct exvec *to;
{
    to->exd0 = from->exd0;
    to->exd1 = from->exd1;
    to->exd2 = from->exd2;
    to->exd3 = from->exd3;
    to->exd4 = from->exd4;
    to->exd5 = from->exd5;
    to->exd6 = from->exd6;
    to->exd7 = from->exd7;
    to->exa0 = from->exa0;
    to->exa1 = from->exa1;
    to->exa2 = from->exa2;
    to->exa3 = from->exa3;
    to->exa4 = from->exa4;
    to->exa5 = from->exa5;
    to->exa6 = from->exa6;
    to->exa7 = from->exa7;
    to->exu.ex1.exstatreg = from->exstatreg;
    to->exu.ex1.expc = from->expc;
} 

veclen(regs)
register struct exvec *regs;
{ 
    if ((regs->exu.ex2.exfmtvec & 0xe000) == 0)
        return 80;
    if ((regs->exu.ex2.exfmtvec & 0xf000) == 0x2000)
        return 84;
    if ((regs->exu.ex2.exfmtvec & 0xf000) == 0x9000)
        return 92;
    if ((regs->exu.ex2.exfmtvec & 0xf000) == 0xA000)
        return 104;
    if ((regs->exu.ex2.exfmtvec & 0xf000) == 0xB000)
        return 164;
    printf("veclen fmtvec=%x ??\n",regs->exu.ex2.exfmtvec);
    
    /*pcs bug: does not return a value */
} 
