/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.6  Feb 26 1987 /usr/sys/munet/mnport.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/ino.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/var.h"
#include <sys/utsname.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uarp.h>
#include <sys/munet/mnport.h>
#include <sys/munet/mninfo.h>
#include "rbq.h"

extern struct munetinfo *get_minfo();

int init_ports()
{
    register struct mnport *mp;
    register struct mnport *mp1 = 0;

    mp = &mnport[v.v_proc*2-1];
    while (mp >= mnport) {
        mp->pt_sflags = 0;
        mp->pt_myptno = 0;
        mp->pt_yourptno = 0;
        mp->pt_yournode = 0;
        mp->pt_mrc = mp->pt_rc = 0;
        mp->pt_rbufd = 0;
        mp->pt_next = mp1;
        mp1 = mp;
        mp--;
    }
    
    freeports = mnport;
    openports = 0;
    return 0;
}

struct mnport *get_freeport()
{
    register struct mnport *mp;
    register short s;

    if (freeports == 0) {
        printf("mnport: out of free ports\n");
        return 0;
    }

    s = spldisk();
    mp = freeports;
    freeports = mp->pt_next;
    mp->pt_next = 0;
    splx(s);
    St_write("uigetport_PORT1", 0x0c, 0x1e);
    return mp;
}

int put_freeport(mp)
register struct mnport *mp;
{
    register short s = spldisk();
    mp->pt_sflags = 0;
    mp->pt_myptno = 0;
    mp->pt_yourptno = 0;
    mp->pt_yournode = 0;
    mp->pt_mrc = mp->pt_rc = 0;
    
    mp->pt_next = freeports;
    rq_relall(&mp->pt_rbufd);
    freeports = mp;
    splx(s);
    return 0;
}

int put_openport(mp)
register struct mnport *mp;
{
    register struct mnport *mp1;
    register short s;
    
    s = spldisk();
    mp1 = openports;
    openports = mp;
    mp->pt_sflags = PT_USED;
    mp->pt_next = mp1;
    splx(s);
    return 0;
}

int rem_openport(mp)
register struct mnport *mp;
{
    register struct mnport *mp1, *mp2;
    register short s = spldisk();
    
    mp1 = mp2 = openports;
    while (mp2 != 0 && mp2 != mp) {
        mp1 = mp2;
        mp2 = mp2->pt_next;
    }

    if (mp2 == 0) {
        printf("mnport: cannot remove port from openlist\n");
        u.u_error = EINVAL;
        splx(s);
        return -1;
    }

    if (mp2 == openports) {
        openports = mp->pt_next;
        mp->pt_next = 0;
        splx(s);
        return 0;
    } else {
        mp1->pt_next = mp->pt_next;
        mp->pt_next = 0;
        splx(s);
        return 0;
    }
}

int conf_port(mp, yourptno, yournode)
register struct mnport *mp;
register short yourptno, yournode;
{
    register short s;
    if (mp == 0) {
        printf("mnport: port is not allocated\n");
        u.u_error = EINVAL;
        return -1;
    }

    s = spldisk();
    mp->pt_sflags = PT_USED;
    mp->pt_mrc = 4;
    mp->pt_yourptno = yourptno;
    mp->pt_yournode = yournode;
    mp->pt_rc = 0;
    rq_relall(&mp->pt_rbufd);
    splx(s);
    return 0;
}

struct mnport *find_port(myptno)
register short myptno;
{
    register short s = spldisk();
    register struct mnport *mp = openports;
    while (mp != 0 && myptno != mp->pt_myptno)
        mp = mp->pt_next;
    splx(s);
    return mp;
}

struct proc *find_ptproc(myptno)
register short myptno;
{
    register short s = spldisk();
    register struct proc *pp;
    register struct munetinfo *mnp;

    for (pp = &proc[1]; pp < (struct proc*)v.ve_proc; pp++) {
        mnp = pp->p_munetinfo;
        if (mnp == 0) continue;
        if (mnp->mi_uport && myptno == mnp->mi_uport->pt_myptno) {
            splx(s);
            return pp;
        }
        if (mnp->mi_kport && myptno == mnp->mi_kport->pt_myptno) {
            splx(s);
            return pp;
        }
    }

    printf("mnport: no process for portnumber %hd\n", myptno);
    splx(s);
    return 0;
}

/* port=1 make user port, =2 port kernel port */
struct mnport* pt_swtch(pp, port)
register struct proc *pp;
register short port;
{
    register struct mnport *mp;
    register short s = spldisk();
    register struct munetinfo *mnp = pp->p_munetinfo;

    if (mnp == 0) {
        mnp = get_minfo();
        if (mnp == 0) {
            splx(s);
            return 0;
        }
        pp->p_munetinfo = mnp;
    }
    
    switch (port) {
    case 1:
        if (mnp->mi_uport == 0) {
            mp = get_freeport();
            if (mp == 0) {
                splx(s);
                return 0;
            }
            put_openport(mp);
            mnp->mi_uport = mp;
        } 
        splx(s);
        return mnp->mi_uport;
    case 2:
        if (mnp->mi_kport == 0) {
            mp = get_freeport();
            if (mp == 0) {
                splx(s);
                return 0;
            }
            put_openport(mp);
            mnp->mi_kport = mp;
        } 
        splx(s);
        return mnp->mi_kport;
    default:
        splx(s);
        return 0;
    }
}

/* newport=1 user port, newport=2 kernel port */
struct mnport *get_port(pp, newport, port, yourptno, yournode)
register struct proc *pp;
register short newport, port, yourptno;
short yournode;
{
    register struct mnport *mp;
    register struct munetinfo *mnp;
    
    
    short s = spldisk();
    if (port == 0) {
        mnp = pp->p_munetinfo;
        if (mnp==0) {
            u.u_error = EINVAL;
            splx(s);
            return 0;
        }
        switch (newport) {
        case 1:
            splx(s);
            return mnp->mi_uport;
        case 2:
            splx(s);
            return mnp->mi_kport;
        default:
            u.u_error = EINVAL;
            splx(s);
            return 0;
        }
    }
    
    mp = pt_swtch(pp, newport);
    if (mp == 0) {
        splx(s);
        u.u_error = ENOMEM;
        return 0;
    }

    switch (port) {
    case -1:
        if (mp->pt_myptno) {
            conf_port(mp, yourptno, yournode);
            splx(s);
            return mp;
        }
        break;
    case -2:
        break;
    default:
        if (port <= 0) {
            splx(s);
            u.u_error = EINVAL;
            return 0;
        }
        break;
    }
    
    mp->pt_myptno = 0;
    if (port <= 0) {
        do {
            free_mnptno++;
            if (free_mnptno > PTNOMAX)
                free_mnptno = PTNOMIN;
        } while (find_port(free_mnptno) != 0);
        mp->pt_myptno = free_mnptno;
    } else {
        if (find_port(port) != 0) {
            printf("mnport: portnumber %hd not free\n", port);
            splx(s);
            u.u_error = EINVAL;
            return 0;
        }
        mp->pt_myptno = port;
    }

    conf_port(mp, yourptno, yournode);
    splx(s);
    return mp;
}

int clean_port(mp)
register struct mnport *mp;
{
    register short s = spldisk();
    mp->pt_sflags = PT_USED;
    if (mp->pt_rc) {
        mp->pt_rc--;
        rq_relfirst(&mp->pt_rbufd);
    }
    splx(s);
    return 0;
}

int close_port(pp, port)
register struct proc *pp;
register short port;
{
    register short s = spldisk();
    register struct mnport *mp;
    register struct munetinfo *mnp = pp->p_munetinfo;
    
    if (mnp == 0) {
        printf("mnport: no munetinfo for this process\n");
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }
    switch (port) {
    case 1:
        mp = mnp->mi_uport;
        break;
    case 2:
        mp = mnp->mi_kport;
        break;
    default:
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }
    if (mp == 0) {
        printf("mnport: port was not allocated\n");
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }

    if (cl_info_pt(mnp, port) == -1) {
        printf("mnport: port of munetinfo cannot be cleared\n");
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }

    if (rem_openport(mp) == -1) {
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }
    
    put_freeport(mp);
    splx(s);
    return 0;
}

int send_port(myport, rq, yourptno, yournode, enqflag)
register short myport;
register struct rbq *rq;
register short yourptno, yournode;
int enqflag;
{
    register struct mnport *mp;
    register struct proc *pp;
    short s, flag;
    int xxx;
    struct mnpacket *mup;
    
    s = spldisk();
    flag = 0;
    mp = find_port(myport);
    if (mp != 0) {
        flag = 1;
        if (mp->pt_sflags & PT_USED) {
            if (mp->pt_rc < mp->pt_mrc) {
                flag = 2;
                if (mp->pt_yourptno== -1 || yourptno == mp->pt_yourptno) {
                    flag = 3;
                    if (mp->pt_yournode== -1 || yournode == mp->pt_yournode)
                        flag = 4;
                }
            }
        }
    }
    
    mup = (struct mnpacket*)rq->r_pkptr;
    if (flag > 0) {
        if (mup->uu.uu_signal < 0) {
            pp = find_ptproc(myport);
            if (pp != 0 && pp->p_stat == SSLEEP && 
                    pp->p_pri > PZERO && pp->p_wchan != (caddr_t)mp) {
                psignal(pp, -mup->uu.uu_signal);
            }
            mup->uu.uu_signal = 0;
        }
    }
    
    if (flag != 4) {
        if (flag == 1)
            St_write("uireceive_RPC17", 8, 2, -1, 0x16);
        else
            St_write("uireceive_RPC18", 8, 2, -1, 0x15);
        splx(s);
        return 1;
    }
    
    if (mp->pt_sflags & PT_WAIT) {
        wakeup((caddr_t)mp);
        mp->pt_sflags &= ~PT_WAIT;
    } else if (enqflag == 0) {
        splx(s);
        return 1;
    }
    
    rq_append(&mp->pt_rbufd, rq);
    mp->pt_rc++;
    splx(s);
    return 0;
}

int rdptwake(mp)
register struct mnport *mp;
{
    register short s = spldisk();
    
    if (mp == 0) {
        printf("mnport: can't wake up this port\n");
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }

    if (mp->pt_rc <= 0) {
        mp->pt_sflags |= PT_TIMO;
        wakeup((caddr_t)mp);
    }
    mp->pt_sflags &= ~PT_RQTO;
    splx(s);
    return 0;
}

int read_port(mp, port, prio)
register struct mnport *mp;
register short port;
int prio;
{
    register short s = spldisk();

    if (mp == 0) {
        printf("mnport: port not allocated\n");
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }

    if (mp->pt_rc <= 0) {
        mp->pt_sflags |= PT_RQTO;
        timeout(rdptwake, mp, port);
    
        while (mp->pt_rc <= 0) {
            if (mp->pt_sflags & PT_TIMO) {
                mp->pt_sflags &= ~(PT_TIMO|PT_WAIT);
                splx(s);
                return -1;
            } else {
                mp->pt_sflags |= PT_WAIT;
                prio = prio ? PCATCH|PPIPE : BPRI;
                if (sleep((caddr_t)mp, prio) != 0 && mp->pt_rc <= 0) {
                    mp->pt_sflags &= ~PT_WAIT;
                    if (mp->pt_sflags & PT_RQTO) {
                        cancelto(rdptwake, mp);
                        mp->pt_sflags &= ~PT_RQTO;
                    }
                    splx(s);
                    return -2;
                }
            }
        }
        mp->pt_sflags &= ~PT_WAIT;
        if (mp->pt_sflags & PT_RQTO) {
            cancelto(rdptwake, mp);
            mp->pt_sflags &= ~PT_RQTO;
        }
    }
    splx(s);
    return 0;
}

exam_ports()
{
    register struct mnport *mp;
    
    printf("Examine Ports : \n");

    for (mp = openports; mp; mp = mp->pt_next) {
        printf("\tmy ptno %hd\tyour ptno %hd\tyour node %hd\n",
            mp->pt_myptno, mp->pt_yourptno, mp->pt_yournode);
    }
}

clearport(mp)
register struct mnport *mp;
{
    register short s = spldisk();
    rq_relall(&mp->pt_rbufd);
    mp->pt_rc = 0;
    splx(s);
}
