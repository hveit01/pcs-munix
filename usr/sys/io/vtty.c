/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.1  Aug 22 1986 /usr/sys/io/vtty.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/tty.h"
#include "sys/ioctl.h"
#undef FIOCLEX
#undef FIONCLEX
#undef FIONREAD
#include "sys/ttold.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/inode.h"
#include "sys/conf.h"
#include "sys/termio.h"
#include "sys/sysinfo.h"
#include "sys/sel.h"

#define NVTTY   NOFILE
#define VBUFSZ  CANBSIZ

/* flags */
#define VT_XOFF     0x0001
#define VT_EOF      0x0002
#define VT_ESCAPE   0x0004
#define VT_IN1      0x0008
#define VT_OUT1     0x0010
#define VT_IN0      0x0020
#define VT_OUT0     0x0040
#define VT_USED     0x0080  /* is in use */
#define VT_WAIT     0x0100  /* someone waits for me */
#define VT_SELWT    0x0200  /* someone waits on nselect */ 

struct vtty { /*size = 302 */
    char v_buf[VBUFSZ];
    int v_count;                /*256*/
    int v_pgrp;                 /*260*/
    int v_line;                 /*264*/
    ushort v_flags;             /*268*/
    struct file *v_filep0;      /*270*/
    struct file *v_filep1;      /*274*/
    struct file *v_filep2;      /*278*/
    struct termio v_tio;        /*280*/
/*  short c_iflag;              /*282 tty iflag: IGNBRK, IXON, ICRNL */
/*  short c_oflag;              /*284 tty oflag: OPOST, ONLCR, OCRNL */
/*  short c_cflag;              /*286*/
/*  short c_lflag;              /*288*/
/*  char c_line;                /*290*/
/*  char c_cc[NCC];             /*291*/
} vtty[NVTTY]; 
struct vtty *endvtty;

extern fake_select(), select();

extern char ttcchar[NCC];

init_vtty()
{
    register struct vtty *vp;

    endvtty = &vtty[NVTTY];
    for (vp = vtty; vp < endvtty; vp++) {
        vp->v_flags = 0;
    }
}

get_vtty()
{
    register struct vtty *vp;

    for (vp = vtty; vp < endvtty; vp++) {
        if ((vp->v_flags & VT_USED)==0) {
            vp->v_flags = VT_USED;
            return vp - vtty;
        }
    }
    return -1;
}

free_vtty(func, n)
{
    register struct vtty *vp;

    if (n >= 0 && n < NVTTY) {
        vp = &vtty[n];
        switch (func) {
        case 'i':
            vp->v_flags |= VT_IN1;
            break;
        case 'o':
            vp->v_flags |= VT_OUT1;
            break;
        case 'I':
            vp->v_flags |= VT_IN0;
            break;
        case 'O':
            vp->v_flags |= VT_OUT0;
            break;
        }

        if ((vp->v_flags & (VT_OUT0|VT_IN0|VT_OUT1|VT_IN1)) == 
                (VT_OUT0|VT_IN0|VT_OUT1|VT_IN1)) {
            if (vp->v_flags & VT_WAIT)
                wakeup(&vp->v_flags);
            vp->v_flags = 0;
        }       
    }
}

pwait(vp)
register struct vtty *vp;
{
    register struct inode *ip;

    ip = vp->v_filep0->f_inode;
    spltty();
      while (ip->i_size != 0) {
          vp->v_flags |= VT_WAIT;
          sleep(&vp->v_flags, NINTER);
      }
    spl0();
}

rele_vtty(fp)
register struct file *fp;
{
    register struct inode *ip;
    register struct vtty *vp;

    if (fp->f_vtty1==F_UNUSED)
        return;

    vp = &vtty[fp->f_vtty2];
    ip = vp->v_filep0->f_inode;
    if ((vp->v_flags & VT_WAIT) && ip->i_size == 0) {
        vp->v_flags &= ~VT_WAIT;
        wakeup(&vp->v_flags);
    }
}

pflush(vp, which)
struct vtty *vp;
{
    struct inode *ip;

    u.u_offset = 0;
    if (which & (VT_IN0|VT_IN1)) {
        ip = vp->v_filep1->f_inode;
        ip->i_size = ip->i_frptr = ip->i_fwptr = 0;
        if (ip->i_fflag & IFIW) {
            ip->i_fflag &= ~IFIW;
            curpri = PPIPE;
            wakeup(&ip->i_fwcnt);
        }
    }

    if (which & (VT_OUT0|VT_OUT1)) {
        ip = vp->v_filep0->f_inode;
        ip->i_size = ip->i_frptr = ip->i_fwptr = 0; 
        if (ip->i_fflag & IFIW) {
            ip->i_fflag &= ~ IFIW;
            curpri = PPIPE;
            wakeup(&ip->i_fwcnt);
        }

        if (vp->v_flags & VT_WAIT) {
            vp->v_flags &= ~VT_WAIT;
            wakeup(&vp->v_flags);
        }
    }
}

wr_vtty(fp)
register struct file *fp;
{
    if (fp->f_vtty1 == 'o')
        o_vtty(fp, u.u_base, u.u_count);
    else
        i_vtty(fp, u.u_base, u.u_count);
}

eof_vtty(fp)
struct file *fp;
{
    int iseof;

    if (fp->f_vtty1 != 'I')
        return 0;

    iseof = vtty[fp->f_vtty2].v_flags & VT_EOF;
    vtty[fp->f_vtty2].v_flags &= ~VT_EOF;
    return iseof;
}

syvtty(cp)
char* cp;
{
    register struct vtty *vp;
    register i;
    struct a {
        int fd;
        caddr_t base;
        ulong count;
    } ap;

    if (u.u_ttyp==0 || *u.u_ttyp != u.u_procp->p_pgrp)
        return 0;

    if (u.u_ttyp < (short*)vtty || u.u_ttyp >= (short*)endvtty)
        return 0;

    for (vp = vtty; vp < endvtty && u.u_ttyp >= (short*)(vp+1); vp++);

    switch (*cp) {
    case 'i':
        vtty_ioctl(vp->v_filep1);
        break;

    case 'r':
        ap.fd = NVTTY;
        for (i=0; i<NVTTY; i++) {
            if (u.u_ofile[i] && u.u_ofile[i] == vp->v_filep2) {
                ap.fd = i;
                break;
            }
        }
        if (ap.fd == NVTTY) {
            u.u_error = EIO;
            return 1;
        }
        
        ap.base = u.u_base;
        ap.count = u.u_count;
        u.u_ap = (long*)&ap;
        rdwr(1, 1);
        break;

    case 'w':
        o_vtty(vp->v_filep0, u.u_base, u.u_count);
        break;
    }
    return 1;
}

stderr_vtty(fpp, ipp)
struct file **fpp;
struct inode **ipp;
{
    register struct file *fp;
    register caddr_t unused;

    fp = *fpp;
    if (fp->f_vtty1 == 'o') {
        fp = vtty[fp->f_vtty2].v_filep1;
        *fpp = fp;
        *ipp = fp->f_inode;
        u.u_fmode = fp->f_flag;
    }
}

vtty_ioctl(fp)
struct file *fp;
{
    register struct termio *vtiop;
    register struct vtty *vp;
    register int sgflags;
    struct file *fp1;
    struct file *fp2;
    struct file *fp3;
    struct inode *ip;
    int num;
    struct termio tio; /*68 c_iflag*/
    struct sgttyb sgttyb;
    struct tchars tchars; /*80*/
    struct ltchars lchars; /*86*/
    int dummy;
    struct a { /* ioctl args passed */
        int fd;         /*96-93*/
        int func;       /*92-89*/
        caddr_t arg;    /*88-85*/
    } *ap;

    ap = (struct a*)u.u_ap;
    ip = fp->f_inode;
    
    if (ap->func == FIONSELECT) {
        fake_select(ap->arg);
        return;
    } else if (ap->func == FIONREAD) {
        int isize = ip->i_size;
        if (fp->f_vtty1 && (vtty[fp->f_vtty2].v_flags & VT_XOFF))
            isize = 0;
        if (copyout(&isize, ap->arg, sizeof(int)) != 0)
            u.u_error = EFAULT;
        return;
    } else if (ap->func == PIOCVTTY) {
            struct vttyb vttyb;
        if (copyin(ap->arg, &vttyb, sizeof(struct vttyb)) != 0) {
            u.u_error = EFAULT;
            return;
        }
        if ((fp = getf(vttyb.inpipe[1]))  == 0 ||
            (fp1= getf(vttyb.outpipe[1])) == 0 ||
            (fp2= getf(vttyb.inpipe[0]))  == 0 ||
            (fp3= getf(vttyb.outpipe[0])) == 0)
                return;

        if ((fp->f_inode->i_mode & IFMT)  != IFIFO ||
            (fp1->f_inode->i_mode & IFMT) != IFIFO ||
            (fp2->f_inode->i_mode & IFMT) != IFIFO ||
            (fp3->f_inode->i_mode & IFMT) != IFIFO) {
                u.u_error = EBADF;
                return;
        }

        if ((fp1->f_vtty2 = fp->f_vtty2 = 
             fp3->f_vtty2 = fp2->f_vtty2 = 
             (num = get_vtty())) == -1) {
                u.u_error = ENFILE;
                return;
        }
        
        fp1->f_vtty1 = 'o'; /* outpipe[1] */
        fp->f_vtty1  = 'i'; /* inpipe[1] */
        fp2->f_vtty1 = 'I'; /* inpipe[0] */
        fp3->f_vtty1 = 'O'; /* outpipe[0] */
        
        vp = &vtty[num];
        vp->v_count = 0;
        vp->v_filep0 = fp1;
        vp->v_filep1 = fp;
        vp->v_filep2 = fp2;
        vp->v_tio.c_iflag = IXON|ICRNL|IGNBRK;
        vp->v_tio.c_oflag = OPOST|ONLCR;
        vp->v_tio.c_lflag = 59;
        vp->v_line = 2;
        bcopy(ttcchar, vp->v_tio.c_cc, NCC);
        u.u_ttyp = (short*)&vp->v_pgrp;
        vp->v_pgrp = u.u_procp->p_pid;
        return;
    } else if (fp->f_vtty1 == F_UNUSED) {
        u.u_error = ENOTTY;
        return;
    } else {
        vp = &vtty[fp->f_vtty2];
        vtiop = &vp->v_tio;
        switch (ap->func) {
        case LDOPEN:
        case LDCLOSE:
        case LDCHG:
        case LDSETT:
        case TCSETA:
        case TCSETAW:
        case TCSETAF:
        case TCSBRK:
        case TCXONC:
        case TCFLSH:
        case TIOCSETD:
        case TIOCHPCL:
        case TIOCSETP:
        case TIOCSETN:
        case TIOCSETC:
        case TIOCSLTC:
        case TIOCSPGRP:
            while (vp->v_line == 2 && vp->v_pgrp != u.u_procp->p_pgrp &&
                (short*)&vp->v_pgrp == u.u_ttyp && 
                (u.u_procp->p_sigignore & (1<<(SIGTTOU-1)))==0 && 
                (u.u_procp->p_sigmask & (1<<(SIGTTOU-1)))==0) {
                    gsignal(u.u_procp->p_pgrp, SIGTTOU);
                    sleep(&lbolt, NINTER-1);
            }
            break;
        }

        switch (ap->func) {
        case IOCTYPE:
            u.u_rval1 = TIOC;
            break;
        case TCSETAW:
        case TCSETAF:
            pwait(vp);
            if (ap->func == TCSETAF)
                pflush(vp, VT_IN1|VT_OUT1);
            /*FALLTHRU*/
        case TCSETA:
            if (copyin(ap->arg, &tio, sizeof(struct termio)) != 0) {
                u.u_error = EFAULT;
                return;
            }
            if (tio.c_line != vtiop->c_line && tio.c_line >= (uint)linecnt) {
                u.u_error = EINVAL;
                return;
            }

            vtiop->c_line = tio.c_line;
            vtiop->c_iflag = tio.c_iflag;
            vtiop->c_oflag = tio.c_oflag;
            vtiop->c_cflag = tio.c_cflag;
            vtiop->c_lflag = tio.c_lflag;
            bcopy(tio.c_cc, vtiop->c_cc, NCC);
            if ((vtiop->c_iflag & (IXON|ICRNL|IGNCR|INLCR|ISTRIP))==0 &&
                (vtiop->c_lflag & (ECHOE|ECHO|ICANON|ISIG))==0 &&
                vp->v_count != 0) {
                    pump_fp(vp->v_filep1, vp, vp->v_count, 1);
                    vp->v_count = 0;
            }
            break;
        
        case TCGETA:
            if (copyout(vtiop, ap->arg, sizeof(struct termio)) != 0)
                u.u_error = EFAULT;
            break;
                
        case TCSBRK:
            pwait(vp);
            break;
            
        case TCXONC:
            switch (*(short*)&ap->arg) {
            case 0:
            case 2:
                vp->v_flags |= VT_XOFF;
                break;
            case 1:
            case 3:
                vp->v_flags &= ~VT_XOFF;
                break;
            default:
                u.u_error = EINVAL;
                break;
            }
            break;

        case TCFLSH:
            switch (*(short*)&ap->arg) {
            case 0:
                pflush(vp, VT_IN1);
                break;
            case 1:
                pflush(vp, VT_OUT1);
                break;
            case 2:
                pflush(vp, VT_IN1|VT_OUT1);
                break;
            default:
                u.u_error = EINVAL;
                break;
            }
            break;

        case TIOCSPGRP:
            if ((num = fuword(ap->arg)) < 0)
                u.u_error = EFAULT;
            else
                vp->v_pgrp = num;
            break;

        case TIOCGPGRP:
            if (suword(ap->arg, vp->v_pgrp) < 0)
                u.u_error = EFAULT;
            break;

        case TIOCSETD:
            if (copyin(ap->arg, &num, sizeof(int)) != 0)
                u.u_error = EFAULT;
            else if (num >= (uint)linecnt)
                u.u_error = EINVAL;
            else
                vp->v_line = num;
            break;

        case TIOCGETD:
            if (copyout(&vp->v_line, ap->arg, sizeof(int)) != 0)
                u.u_error = EFAULT;
            break;
            
        case TIOCSETC:
            if (copyin(ap->arg, &tchars, sizeof(struct tchars)) != 0)
                u.u_error = EFAULT;
            else {
                vtiop->c_cc[VINTR] = tchars.t_intrc;
                vtiop->c_cc[VQUIT] = tchars.t_quitc;
                vtiop->c_cc[VEOF] = tchars.t_eofc;
                vtiop->c_cc[VEOL] = tchars.t_brkc;
            }
            break;
            
        case TIOCGETC:
            tchars.t_intrc = vtiop->c_cc[VINTR];
            tchars.t_quitc = vtiop->c_cc[VQUIT];
            tchars.t_startc = CSTART;
            tchars.t_stopc = CSTOP;
            tchars.t_eofc = vtiop->c_cc[VEOF];
            tchars.t_brkc = vtiop->c_cc[VEOL];
            if (copyout(&tchars, ap->arg, sizeof(struct tchars)) != 0)
                u.u_error = EFAULT;
            break;

        case TIOCSLTC:
            if (copyin(ap->arg, &lchars, sizeof(struct ltchars)) != 0)
                u.u_error = EFAULT;
            else {
                vtiop->c_cc[VSUSP] = lchars.t_suspc;
                vtiop->c_cc[VDSUS] = lchars.t_dsuspc;
            }
            break;

        case TIOCGLTC:
            lchars.t_suspc  = vtiop->c_cc[VSUSP];
            lchars.t_dsuspc = vtiop->c_cc[VDSUS];
            lchars.t_rprntc = 0x12;
            lchars.t_flushc = 0x0f;
            lchars.t_werasc = 0x17;
            lchars.t_lnextc = CESC;
            if (copyout(&lchars, ap->arg, sizeof(struct ltchars)) != 0)
                u.u_error = EFAULT;
            break;

        case TIOCHPCL:
            vtiop->c_cflag |= HUPCL;
            break;

        case TIOCSETP:
            pwait(vp);
            pflush(vp, VT_IN1|VT_OUT1);
            /*FALLTHRU*/
        case TIOCSETN:
            if (copyin(ap->arg, &sgttyb, sizeof(struct sgttyb)) != 0) {
                u.u_error = EFAULT;
                return;
            }

            vtiop->c_iflag = vtiop->c_oflag = vtiop->c_lflag = 0;
            vtiop->c_cflag = CREAD|B9600;
            vtiop->c_cc[VERASE] = sgttyb.sg_erase;
            vtiop->c_cc[VKILL] = sgttyb.sg_kill;
            
            sgflags = sgttyb.sg_flags;

            if (sgflags & O_TANDEM)
                vtiop->c_cflag |= TOSTOP;

            if (sgflags & O_XTABS)
                vtiop->c_oflag |= TAB3;
            else if (sgflags & (O_TB1|O_TB2))
                vtiop->c_oflag |= TAB1;

            if (sgflags & O_LCASE) {
                vtiop->c_iflag |= IUCLC;
                vtiop->c_oflag |= OLCUC;
                vtiop->c_lflag |= XCASE;
            }

            if (sgflags & O_ECHO)
                vtiop->c_lflag |= (ECHO|ECHOE|ECHOK);

            if (sgflags & O_CRMOD) {
                vtiop->c_iflag |= ICRNL;
                vtiop->c_oflag |= ONLCR;
                if (sgflags & O_CR1)
                    vtiop->c_oflag |= CR1;
                if (sgflags & O_CR2)
                    vtiop->c_oflag |= (CR2|ONOCR);
            } else {
                vtiop->c_oflag |= ONLRET;
                if (sgflags & O_NL1)
                    vtiop->c_oflag |= CR1;
                if (sgflags & O_NL2)
                    vtiop->c_oflag |= CR2;
            }

            if (sgflags & (O_RAW|O_CBREAK)) {
                vtiop->c_cc[VEOL] = 0;
                vtiop->c_cc[VEOF] = 1;
                vtiop->c_cflag |= CS8;
                if (sgflags & O_CBREAK) {
                    vtiop->c_iflag |= (IXON|ISTRIP|IGNPAR|BRKINT);
                    vtiop->c_lflag |= ISIG;
                    vtiop->c_oflag |= OPOST;
                } else
                    vtiop->c_iflag &= ~(IUCLC|ICRNL);
            } else {
                vtiop->c_cc[VEOF] = CEOF;
                vtiop->c_cc[VEOL] = 0;
                vtiop->c_cc[VEOL2] = 0;
                vtiop->c_iflag |= (IXON|ISTRIP|IGNPAR|BRKINT);
                vtiop->c_oflag |= OPOST;
                vtiop->c_cflag |= 
                    ((sgflags & O_ODDP) && (sgflags & O_EVENP)) ? 
                        CS8 : (PARENB|CS7);
                vtiop->c_lflag |= (ISIG|ICANON);
            }
            
            vtiop->c_iflag |= INPCK;
            if (sgflags & O_ODDP) { 
                if (sgflags & O_EVENP)
                    vtiop->c_iflag &= ~INPCK;
                else
                    vtiop->c_cflag |= PARODD;
            }

            if (sgflags & O_VTDELAY)
                vtiop->c_oflag |= FF1;

            if (sgflags & O_BSDELAY)
                vtiop->c_oflag |= BS1;
            break;

        case TIOCGETP:
            sgttyb.sg_ospeed = sgttyb.sg_ispeed = B9600;
            sgttyb.sg_erase = vtiop->c_cc[VERASE];
            sgttyb.sg_kill = vtiop->c_cc[VKILL];

            sgflags = 0;
            if ((vtiop->c_lflag & ICANON)==0)
                sgflags |= (vtiop->c_lflag & ISIG) ? O_CBREAK : O_RAW;

            if (vtiop->c_lflag & XCASE)
                sgflags |= O_LCASE;
            
            if (vtiop->c_lflag & ECHO)
                sgflags |= O_ECHO;
            
            if (vtiop->c_cflag & PARODD)
                sgflags |= O_ODDP;
            else if (vtiop->c_iflag & INPCK)
                sgflags |= O_EVENP;
            else
                sgflags |= (O_EVENP|O_ODDP);

            if (vtiop->c_oflag & ONLCR) {
                sgflags |= O_CRMOD;
                if (vtiop->c_oflag & CR1)
                    sgflags |= O_CR1;
                if (vtiop->c_oflag & CR2)
                    sgflags |= O_CR2;
            } else {
                if (vtiop->c_oflag & CR1)
                    sgflags |= O_NL1;
                if (vtiop->c_oflag & CR2)
                    sgflags |= O_NL2;
            }
            
            if ((vtiop->c_oflag & TAB3) == TAB3)
                sgflags |= O_XTABS; 
            else if (vtiop->c_oflag & TAB1)
                sgflags |= O_XTABS;

            if (vtiop->c_oflag & FF1)
                sgflags |= O_VTDELAY;

            if (vtiop->c_oflag & BS1)
                sgflags |= O_BSDELAY;

            sgttyb.sg_flags = sgflags;
            if (copyout(&sgttyb, ap->arg, sizeof(struct sgttyb)) != 0)
                u.u_error = EFAULT;
            
            break;

        default:
            if ((ap->func & IOCTYPE) != LDIOC)
                u.u_error = EINVAL;
            break;
        }
    }
}

pump_fp(fp, abase, acount, asegflg)
struct file *fp;
caddr_t abase;
{
    caddr_t sbase;
    ulong scount;
    short ssegflg;
    short sfmode;
    off_t soffset;
    ushort spboff;
    ushort spbsize;
    struct inode *sip;
    struct vtty *vp = &vtty[fp->f_vtty2];
    off_t sisize;

    /* save fields of u */
    sbase = u.u_base;
    scount= u.u_count;
    ssegflg = u.u_segflg;
    sfmode = u.u_fmode;
    soffset = u.u_offset;
    spboff = u.u_pboff;
    spbsize = u.u_pbsize;

    u.u_base = abase;
    u.u_count = acount;
    u.u_segflg = asegflg;
    u.u_fmode = fp->f_flag;
    sip = fp->f_inode;
    sisize = sip->i_size;
    plock(sip);
    u.u_offset = 0;
    fp->f_offset = 0;
    writei(sip);
    prele(sip);
    acount -= u.u_count;
    fp->f_offset += acount;
    u.u_base = sbase;
    u.u_count = scount;
    u.u_segflg = ssegflg;
    u.u_fmode = sfmode;
    u.u_offset = soffset;
    u.u_pboff = spboff;
    u.u_pbsize = spbsize;

    if (sisize == 0 && (vp->v_flags & VT_SELWT)) {
        vp->v_flags &= ~VT_SELWT;
        wakeup(&nselect);
    }
}

o_vtty(fp, base, cnt)
struct file *fp;
caddr_t base;
{
    char *cp;                           
    char *cpend;                        
    int unused;
    char buf[VBUFSZ];                   
    char b;                             
    ushort oflag;                       
    struct vtty *vp = &vtty[fp->f_vtty2];
    
    oflag = vp->v_tio.c_oflag;

    if (cnt == 0) return;

    while (vp->v_line == 2 && vp->v_pgrp != u.u_procp->p_pgrp &&
        (short*)&vp->v_pgrp == u.u_ttyp && 
        (vp->v_tio.c_cflag & TOSTOP) &&
        (u.u_procp->p_sigignore & (1<<(SIGTTOU-1)))==0 && 
        (u.u_procp->p_sigmask & (1<<(SIGTTOU-1)))==0) {
            gsignal(u.u_procp->p_pgrp, SIGTTOU);
            sleep(&lbolt, NINTER-1);
    }

    /* transfer RAW */
    if ((oflag & OPOST)==0)
        return pump_fp(fp, base, cnt, 0);

    /* need to do primitive OPOST processing */
    cpend = &buf[VBUFSZ-2];
    cp =    buf;
    while (cnt > 0) {
        b = fubyte(base);
        if (u.u_error) return;
        switch (b) {
        case '\n':
            if (oflag & ONLCR)
                *cp++ = '\r';
            *cp++ = '\n';
            break;
        case '\r':
            if (oflag & OCRNL)
                *cp++ = '\n';
            else
                *cp++ = '\r';
            break;
        default:
            *cp++ = b;
            break;
        }
        base++;
        cnt--;
        if (cnt == 0 || cp > cpend) {
            pump_fp(fp, buf, cp - buf, 1);
            cp = buf;
        }
    }
}

i_vtty(fp, base, cnt)
struct file *fp;
caddr_t base;
{
    char ch;
    caddr_t vbufp;
    caddr_t cbufp;
    caddr_t cendp;
    char buf[VBUFSZ];
    struct file *filep0;
    struct vtty *vp;
    ushort iflag;
    ushort lflag;
    ushort oflag;
    ushort vflags;
    short count;
    int sigflg;

    if (cnt == 0) return;

    vp = &vtty[fp->f_vtty2];
    filep0 = vp->v_filep0;
    iflag = vp->v_tio.c_iflag;
    lflag = vp->v_tio.c_lflag;
    oflag = vp->v_tio.c_oflag;
    vflags = vp->v_flags;
    count = vp->v_count;
    vbufp = &vp->v_buf[count];
    cbufp = buf;
    cendp = &buf[VBUFSZ-3];
    
    if (fp->f_inode->i_size > VBUFSZ)
        pflush(vp, VT_IN1);

    if ((iflag & (IXON|ICRNL|IGNCR|INLCR|ISTRIP))==0 &&
        (lflag & (ECHO|ECHOE|ICANON|ISIG))==0) {
            pump_fp(fp, base, cnt, 0);
            return;
    }

    while (cnt > 0) {

        if (count >= VBUFSZ) {
            vbufp = vp->v_buf;
            count = 0;
        }

        lflag = vp->v_tio.c_lflag;
        ch = fubyte(base);
        if (u.u_error != 0) return;

        if (iflag & ISTRIP)
            ch &= 0x7f;
        if (lflag & ISIG) {
            sigflg = 0;
            
            if (ch == vp->v_tio.c_cc[VINTR]) {
                signal(vp->v_pgrp, SIGINT);
                sigflg = 1;
            }
            if (ch == vp->v_tio.c_cc[VQUIT]) {
                signal(vp->v_pgrp, SIGQUIT);
                sigflg = 1;
            }

            if (vp->v_line == 2 && 
                (ch == vp->v_tio.c_cc[VSUSP] || ch == vp->v_tio.c_cc[VDSUS])) {
                    gsignal(vp->v_pgrp, SIGTSTP);
                    sigflg = 1;
            }

            if (sigflg && (lflag & NOFLSH)==0) {
                count = 0;
                vbufp = vp->v_buf;
                pflush(vp, VT_IN1|VT_OUT1);
            }
        
            if (sigflg)
                goto flushbuf;
        }
        
        if (iflag & IXON) {
            if (ch == CSTOP) {
                vflags |= VT_XOFF;
                goto flushbuf;
            }
            if (ch == CSTART) {
                vflags &= ~VT_XOFF;
                goto flushbuf;
            }
            
            if ((vflags & VT_XOFF) && (iflag & IXANY))
                vflags &= ~VT_XOFF;
        }

        if (ch == '\n' && (iflag & INLCR))
            ch = '\r';
        else if (ch ==  '\r') {
            if (iflag & IGNCR)
                goto flushbuf;
            if (iflag & ICRNL)
                ch = '\n';
        }

        if (lflag & ICANON) {
            if (ch == '\n' && (lflag & ECHONL))
                lflag |= ECHO;
            if ((vflags & VT_ESCAPE)==0) {
                if (ch == vp->v_tio.c_cc[VERASE])
                    ch = 8; /*BS*/
                if (ch == '\\') {
                    vflags |= VT_ESCAPE;
                    count++;
                    *vbufp++ = ch;
                } else if (ch == 8 && (lflag & ECHOE)) {
                    if (lflag & ECHO)
                        *cbufp++ = 8;
                    lflag |= ECHO;
                    *cbufp++ = ' ';
                    if (count != 0) {
                        vbufp--;
                        count--;
                    }
                } else if (ch == vp->v_tio.c_cc[VKILL]) {
                    vbufp = vp->v_buf;
                    count = 0;
                    if (lflag & ECHOK) {
                        if (lflag & ECHO)
                            *cbufp++ = '@';
                        lflag |= ECHO;
                        ch = '\n';
                    }
                } else if (ch == vp->v_tio.c_cc[VEOF]) {
                    lflag &= ~ECHO;
                    ch = '\n';
                    count++;
                    *vbufp++ = ch;
                    vp->v_flags |= VT_EOF;
                    vflags |= VT_EOF;
                } else {
                    count++;
                    *vbufp++ = ch;
                }
            } else {
                if (ch == vp->v_tio.c_cc[VERASE] ||
                    ch == vp->v_tio.c_cc[VKILL] ||
                    ch == vp->v_tio.c_cc[VEOF]) {
                        count--;
                        vbufp--;
                }
                count++;
                *vbufp++ = ch;
                if (ch != '\\')
                    vflags &= ~VT_ESCAPE;
            }
            
            if (ch == '\n' || 
                ch == vp->v_tio.c_cc[VEOF] ||
                ch == vp->v_tio.c_cc[VEOL] ||
                ch == vp->v_tio.c_cc[VEOL2]) {
                if (count != 0)
                    pump_fp(fp, vp, count, 1);
                vbufp = vp->v_buf;
                count = 0;
                
                if (ch == vp->v_tio.c_cc[VEOF] && 
                    fp->f_inode->i_fflag & IFIR) {
                        fp->f_inode->i_fflag &= ~IFIR;
                        curpri = PPIPE;
                        wakeup(&fp->f_inode->i_waite);
                }
            }
        } else {
            count++;
            *vbufp++ = ch;
            if (cnt == 1 || vbufp >= &vp->v_buf[VBUFSZ]) {
                pump_fp(fp, vp, count, 1);
                vbufp = vp->v_buf;
                count = 0;
            }
        }

        if (lflag & ECHO) {
            if (ch == '\n') {
                if (oflag & ONLCR)
                    *cbufp++ = '\r';
                *cbufp++ = '\n';
            } else if (ch == '\r') {
                if (oflag & OCRNL)
                    *cbufp++ = '\n';
                else
                    *cbufp++ = '\r';
            } else {
                *cbufp++ = ch;
            }
        }

flushbuf:
        base++;
        cnt--;
        if (cnt == 0 || cbufp > cendp) {
            if ((vflags & VT_OUT1)==0 && cbufp > buf)
                pump_fp(filep0, buf, cbufp - buf, 1);

            cbufp = buf;
        }
    }

    vp->v_count = count;
    vp->v_flags = vflags;
}

fake_select(arg)
int *arg;
{
    static int x;
    register i, k;
    struct a {
        int nfds;
        fd_set *readfds;
        fd_set *writefds;
        fd_set *exceptfds;
        struct timeval *tmout;
    } *uap;
    int var36;
    int var40;

    var36 = fulong(arg);
    if (u.u_error != 0)
        return;

    /* prepare system call */
    uap = (struct a*)u.u_ap;
    uap->readfds = (fd_set*)arg;
    uap->nfds = 32;
    uap->writefds = uap->exceptfds = 0;
    uap->tmout = 0;
    /* do system call */
    select();
    /* obtain result */
    var40 = fulong(arg);

    sulong(arg++, var36);
    if (u.u_error != 0 || var40 == 0) {
        suword(arg, -1);
        return;
    }

    if (var40 & SEL_IN) {
        suword(arg, 0);
        return;
    }

    for (i= x; i < (x+32); i++) {
        k = i % 32;
        if (var40 & (1<<k)) {
            suword(arg, k);
            x = k+1;
            break;
        }
    }
}

sel_vtty(fp, wait, flg)
register struct file *fp;
{
    register struct vtty *vp;
    register ret;

    ret = 0;
    vp = &vtty[fp->f_vtty2];

    if (wait == -1) {
        vp->v_flags &= ~VT_SELWT;
    } else {
        if (flg && (vp->v_flags & VT_XOFF)==0 &&
            fp->f_inode->i_size != 0)
                ret |= SEL_IN;
        if (ret==0 && wait)
            vp->v_flags |= VT_SELWT;
    }
    return ret;
}
