static char* _Version = "@(#) RELEASE:  1.2  Nov 09 1987 /usr/sys/io/tty.c ";

/* @(#)tty.c    6.1 */
/*
 * general TTY subroutines
 */
#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/tty.h"
#include "sys/ttold.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/termio.h"
#include "sys/sysinfo.h"

/*
 * tty low and high water marks
 * high < TTYHOG
 */
int tthiwat[16] = {
    0, 60, 60, 60,
    60, 60, 60, 120,
    120, 180, 180, 240,
    240, 240, 240, 240,
};
int ttlowat[16] = {
    0, 20, 20, 20,
    20, 20, 20, 40,
    40, 60, 60, 80,
    80, 80, 80, 80,
};

char    ttcchar[NCC] = {
    CINTR,
    CQUIT,
    CERASE,
    CKILL,
    CEOF,
    0,
    0,
    0,  
    CSUSP,  /*pcs*/
    CDSUSP  /*pcs*/
};

/* null clist header */
struct clist ttnulq;

/* canon buffer */
char    canonb[CANBSIZ];
/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
char    maptab[] = {
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,'|',000,000,000,000,000,'`',
    '{','}',000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,000,000,
    000,000,000,000,000,000,'~',000,
    000,'A','B','C','D','E','F','G',
    'H','I','J','K','L','M','N','O',
    'P','Q','R','S','T','U','V','W',
    'X','Y','Z',000,000,000,000,000,
};

/*pcs to access lower word part, used for ttold ioctls */
#define WARG(arg) (((short*)&arg)[1])
#define UARG(arg) (((ushort*)&arg)[1])

/*
 * common ioctl tty code
 */
ttiocom(tp, cmd, arg, mode)
register struct tty *tp;
{
    register flag;
    struct termio cb;
    struct sgttyb tb;
    int cc; /*pcs*/
    extern int sxtline;

    switch(cmd) {
    case IOCTYPE:
        u.u_rval1 = TIOC;
        break;

    case TCSETAW:
    case TCSETAF:
        ttywait(tp);
        if (cmd == TCSETAF)
            ttyflush(tp, (FREAD|FWRITE));
    case TCSETA:
        if (copyin(arg, &cb, sizeof cb)) {
            u.u_error = EFAULT;
            break;
        }
        if (tp->t_line != cb.c_line) {
            if (cb.c_line < 0 || cb.c_line >= linecnt || cb.c_line == sxtline) {
                u.u_error = EINVAL;
                break;
            }
            (*linesw[tp->t_line].l_ioctl)(tp, LDCLOSE, 0, mode);
        }
        flag = tp->t_lflag;
        tp->t_iflag = cb.c_iflag;
        tp->t_oflag = cb.c_oflag;
        
        tp->t_cflag = cb.c_cflag | (tp->t_cflag & (TTSEL|TTYSEL)); /*pcs*/
        
        tp->t_lflag = cb.c_lflag;
        bcopy(cb.c_cc, tp->t_cc, NCC);
        if (tp->t_line != cb.c_line) {
            tp->t_line = cb.c_line;
            (*linesw[tp->t_line].l_ioctl)(tp, LDOPEN, 0, mode);
        } else if (tp->t_lflag != flag) {
            (*linesw[tp->t_line].l_ioctl)(tp, LDCHG, flag, mode);
        }
        return(1);

    case TCGETA:
        cb.c_iflag = tp->t_iflag;
        cb.c_oflag = tp->t_oflag;
        cb.c_cflag = tp->t_cflag & ~(TTSEL|TTYSEL); /*pcs*/
        cb.c_lflag = tp->t_lflag;
        cb.c_line = tp->t_line;
        bcopy(tp->t_cc, cb.c_cc, NCC);
        if (copyout(&cb, arg, sizeof cb))
            u.u_error = EFAULT;
        break;

    case TCSBRK:
        ttywait(tp);
        if (WARG(arg) == 0) /*pcs*/
            (*tp->t_proc)(tp, T_BREAK);
        break;

    case TCXONC:
        switch (WARG(arg)) { /*pcs*/
        case 0:
            (*tp->t_proc)(tp, T_SUSPEND);
            break;
        case 1:
            (*tp->t_proc)(tp, T_RESUME);
            break;
        case 2:
            (*tp->t_proc)(tp, T_BLOCK);
            break;
        case 3:
            (*tp->t_proc)(tp, T_UNBLOCK);
            break;
        default:
            u.u_error = EINVAL;
        }
        break;

    case TCFLSH:
        switch (WARG(arg)) { /*pcs*/
        case 0:
        case 1:
        case 2:
            ttyflush(tp, (UARG(arg) - FOPEN)&(FREAD|FWRITE)); /*pcs*/
            break;

        default:
            u.u_error = EINVAL;
        }
        break;
        
    case TIOCHPCL:              /*pcs*/
        tp->t_cflag |= HUPCL;   /*pcs*/
        break;                  /*pcs*/

/* conversion aide only */
    case TIOCSETP:
        ttywait(tp);
        ttyflush(tp, (FREAD|FWRITE));
    case TIOCSETN: /*pcs*/
        if (copyin(arg, &tb, sizeof(tb))) {
            u.u_error = EFAULT;
            break;
        }
        tp->t_iflag = 0;
        tp->t_oflag = 0;
        tp->t_lflag = 0;
        tp->t_cflag = (tb.sg_ispeed&CBAUD)|CREAD|(tp->t_cflag&CLOCAL); /*pcs*/
        if ((tb.sg_ispeed&CBAUD)==B110)
            tp->t_cflag |= CSTOPB;
        tp->t_cc[VERASE] = tb.sg_erase;
        tp->t_cc[VKILL] = tb.sg_kill;
        flag = tb.sg_flags;
        if (flag&O_TANDEM)          /*pcs*/
            tp->t_cflag |= TOSTOP;  /*pcs*/
        if (flag&O_XTABS)
            tp->t_oflag |= TAB3;
        else if (flag&O_TBDELAY)
            tp->t_oflag |= TAB1;
        if (flag&O_LCASE) {
            tp->t_iflag |= IUCLC;
            tp->t_oflag |= OLCUC;
            tp->t_lflag |= XCASE;
        }
        if (flag&O_ECHO)
            tp->t_lflag |= ECHO|ECHOE|ECHOK;    /*pcs*/
/* !pcs if (!(flag&O_NOAL))
            tp->t_lflag |= ECHOK;*/
        if (flag&O_CRMOD) {
            tp->t_iflag |= ICRNL;
            tp->t_oflag |= ONLCR;
            if (flag&O_CR1)
                tp->t_oflag |= CR1;
            if (flag&O_CR2)
                tp->t_oflag |= ONOCR|CR2;
        } else {
            tp->t_oflag |= ONLRET;
            if (flag&O_NL1)
                tp->t_oflag |= CR1;
            if (flag&O_NL2)
                tp->t_oflag |= CR2;
        }
        if (flag&(O_RAW|O_CBREAK)) {    /*pcs*/
            tp->t_cc[VTIME] = 0;        /*pcs*/
            tp->t_cc[VMIN] = 1;
            tp->t_cflag |= CS8;
            if (flag & O_CBREAK) {      /*pcs*/
                tp->t_iflag |= BRKINT|IGNPAR|ISTRIP|IXON; /*pcs*/
                tp->t_lflag |= ISIG;    /*pcs*/
                tp->t_oflag |= OPOST;   /*pcs*/
            } else                      /*pcs*/
                tp->t_iflag &= ~(ICRNL|IUCLC); /*pcs
                
/* !pcs         tp->t_cc[VTIME] = 1;
            tp->t_cc[VMIN] = 6;
            tp->t_iflag &= ~(ICRNL|IUCLC);
            tp->t_cflag |= CS8;*/
        } else {
            tp->t_cc[VEOF] = CEOF;
            tp->t_cc[VEOL] = 0;
            tp->t_cc[VEOL2] = 0;
            tp->t_iflag |= BRKINT|IGNPAR|ISTRIP|IXON;   /*pcs*/
            tp->t_oflag |= OPOST;
            tp->t_cflag |= ((flag&O_ODDP) && (flag&O_EVENP)) ? /*pcs*/
                CS8 : (CS7|PARENB);                     /*pcs*/
            tp->t_lflag |= ICANON|ISIG;                 /*pcs*/
/* !pcs     tp->t_cflag |= CS7|PARENB;
            tp->t_lflag |= ICANON|ISIG;*/
        }
        tp->t_iflag |= INPCK;
        if (flag&O_ODDP)
            if (flag&O_EVENP)
                tp->t_iflag &= ~INPCK;
            else
                tp->t_cflag |= PARODD;
        if (flag&O_VTDELAY)
            tp->t_oflag |= FFDLY;
        if (flag&O_BSDELAY)
            tp->t_oflag |= BSDLY;
        return(1);

    case TIOCGETP:
        tb.sg_ispeed = tp->t_cflag&CBAUD;
        tb.sg_ospeed = tb.sg_ispeed;
        tb.sg_erase = tp->t_cc[VERASE];
        tb.sg_kill = tp->t_cc[VKILL];
        flag = 0;
/* !pcs if (tp->t_cflag&HUPCL)
            flag |= O_HUPCL;*/
        if (!(tp->t_lflag&ICANON))
            flag |= (tp->t_lflag & ISIG) ? O_CBREAK : O_RAW; /*pcs*/
/* !pcs     flag |= O_RAW; */
        if (tp->t_lflag&XCASE)
            flag |= O_LCASE;
        if (tp->t_lflag&ECHO)
            flag |= O_ECHO;
/* !pcs if (!(tp->t_lflag&ECHOK))
            flag |= O_NOAL;*/
        if (tp->t_cflag&PARODD)
            flag |= O_ODDP;
        else if (tp->t_iflag&INPCK)
            flag |= O_EVENP;
        else
            flag |= O_ODDP|O_EVENP;
        if (tp->t_oflag&ONLCR) {
            flag |= O_CRMOD;
            if (tp->t_oflag&CR1)
                flag |= O_CR1;
            if (tp->t_oflag&CR2)
                flag |= O_CR2;
        } else {
            if (tp->t_oflag&CR1)
                flag |= O_NL1;
            if (tp->t_oflag&CR2)
                flag |= O_NL2;
        }
        if ((tp->t_oflag&TABDLY)==TAB3)
            flag |= O_XTABS;
        else if (tp->t_oflag&TAB1)
            flag |= O_TBDELAY;
        if (tp->t_oflag&FFDLY)
            flag |= O_VTDELAY;
        if (tp->t_oflag&BSDLY)
            flag |= O_BSDELAY;
        tb.sg_flags = flag;
        if (copyout(&tb, arg, sizeof(tb)))
            u.u_error = EFAULT;
        break;
    case FIONREAD: /*pcs*/
        u.u_procp->p_inter = 0;
        if (!(tp->t_lflag & ICANON)) {
            cc = tp->t_rawq.c_cc + tp->t_canq.c_cc;
        } else {
            if (tp->t_delct)
                canon(tp);
            cc = tp->t_canq.c_cc;
        }
        if (copyout(&cc, arg, sizeof(int)))
            u.u_error = EFAULT;
        break;

    default:
        if ((cmd&IOCTYPE) == LDIOC)
            (*linesw[tp->t_line].l_ioctl)(tp, cmd, arg, mode);
        else
            u.u_error = EINVAL;
        break;
    }
    return(0);
}

ttinit(tp)
register struct tty *tp;
{
    tp->t_line = 0;
    tp->t_term = 0;                                     /*pcs*/
    if (tp->t_cflag == 0) {                             /*pcs*/
        tp->t_iflag = BRKINT|IGNPAR|ISTRIP|ICRNL|IXON;  /*pcs*/
        tp->t_oflag = OPOST|ONLCR;                      /*pcs*/
        tp->t_cflag = CLOCAL|HUPCL|CREAD|CS8|B9600;     /*pcs*/
        tp->t_lflag = ECHO|ECHOE|ECHOK|ICANON|ISIG;     /*pcs*/
/* !pcs tp->t_iflag = 0;
    tp->t_oflag = 0;
    tp->t_cflag = SSPEED|CS8|CREAD|HUPCL;
    tp->t_lflag = 0; */
        bcopy(ttcchar, tp->t_cc, NCC);
    }                                                   /*pcs*/
}

ttywait(tp)
register struct tty *tp;
{
    register s; /*pcs*/
    
    s = spltty();   /*pcs*/
    while (tp->t_outq.c_cc || (tp->t_state&(BUSY|TIMEOUT))) {
        tp->t_state |= TTIOW;
        sleep((caddr_t)&tp->t_oflag, TTOPRI);
    }
    splx(s);    /*pcs*/
    delay(HZ/15);
}

/*
 * flush TTY queues
 */
ttyflush(tp, cmd)
register struct tty *tp;
{
    register struct cblock *cp;
    register s;
    extern nselect; /*pcs*/

    if (cmd&FWRITE) {
        while ((cp = getcb(&tp->t_outq)) != NULL)
            putcf(cp);
        (*tp->t_proc)(tp, T_WFLUSH);
        if (tp->t_state&OASLP) {
            tp->t_state &= ~OASLP;
            wakeup((caddr_t)&tp->t_outq);
        }
        if (tp->t_state&TTIOW) {
            tp->t_state &= ~TTIOW;
            wakeup((caddr_t)&tp->t_oflag);
        }
        if (tp->t_cflag&TTYSEL) {       /*pcs*/
            tp->t_cflag &= ~TTYSEL;     /*pcs*/
            wakeup((caddr_t)&nselect);  /*pcs*/
        }
    }
    if (cmd&FREAD) {
        while ((cp = getcb(&tp->t_canq)) != NULL)
            putcf(cp);
        s = spltty(); /*pcs*/
        while ((cp = getcb(&tp->t_rawq)) != NULL)
            putcf(cp);
        tp->t_delct = 0;
        splx(s);
        (*tp->t_proc)(tp, T_RFLUSH);
        if (tp->t_state&IASLP) {
            tp->t_state &= ~IASLP;
            wakeup((caddr_t)&tp->t_rawq);
        }
        if (tp->t_cflag&TTYSEL) {       /*pcs*/
            tp->t_cflag &= ~TTYSEL;     /*pcs*/
            wakeup((caddr_t)&nselect);  /*pcs*/
        }
    }
}

/*
 * Transfer raw input list to canonical list,
 * doing erase-kill processing and handling escapes.
 */
canon(tp)
register struct tty *tp;
{
    register char *bp;
    register struct cblock *cp;
    register c, esc;
    register s; /*pcs*/

    s = spltty(); /*pcs*/
    if (tp->t_rawq.c_cc == 0)
        tp->t_delct = 0;
    while (tp->t_delct == 0) {
        if (!(tp->t_state&CARR_ON) || (u.u_fmode&FNDELAY)) {
            splx(s); /*pcs*/
            return;
        }
        if (!(tp->t_lflag&ICANON) && tp->t_cc[VMIN]==0) {
            if (tp->t_cc[VTIME]==0)
                break;
            tp->t_state &= ~RTO;
            if (!(tp->t_state&TACT))
                tttimeo(tp);
        }
        tp->t_state |= IASLP;
        sleep((caddr_t)&tp->t_rawq, TTIPRI);
    }
    if (!(tp->t_lflag&ICANON)) {
        while (cp=getcb(tp))        /*pcs*/
            putcb(cp, &tp->t_canq); /*pcs*/
/* !pcs tp->t_canq = tp->t_rawq;
        tp->t_rawq = ttnulq; */
        tp->t_delct = 0;
        splx(s); /*pcs*/
        return;
    }
    splx(s);
    bp = canonb;
    esc = 0;
    while ((c=getc(&tp->t_rawq)) >= 0) {
        if (!esc) {
            if (c == '\\') {
                esc++;
            } else if (c == tp->t_cc[VERASE] || c == '\b') { /*pcs*/
                if (bp > canonb)
                    bp--;
                continue;
            } else if (c == tp->t_cc[VKILL]) {
                bp = canonb;
                continue;
            } else if (c == tp->t_cc[VEOF]) {
                break;
            }
        } else {
            esc = 0;
            if (c == tp->t_cc[VERASE] ||
                c == tp->t_cc[VKILL] ||
                c == tp->t_cc[VEOF])
                bp--;
            else if (tp->t_lflag&XCASE) {
                if ((c < 0200) && maptab[c]) {
                    bp--;
                    c = maptab[c];
                } else if (c == '\\')
                    continue;
            } else if (c == '\\')
                esc++;
        }
        *bp++ = c;
        if (c == '\n' || c == tp->t_cc[VEOL] || c == tp->t_cc[VEOL2])
            break;
        if (bp >= &canonb[CANBSIZ])
            bp--;
    }
    tp->t_delct--;
    c = bp - canonb;
    sysinfo.canch += c;
    bp = canonb;
/* faster copy ? */
    while (c--)
        putc(*bp++, &tp->t_canq);
    return;
}

/*
 * Restart typewriter output following a delay timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
ttrstrt(tp)
register struct tty *tp;
{

    (*tp->t_proc)(tp, T_TIME);
}

