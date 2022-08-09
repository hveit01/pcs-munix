/* PCS specific */
static char *_Version = "@(#) RELEASE:  2.6  May 18 1987 /usr/sys/munet/munet.c";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/filsys.h>
#include <sys/reg.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/swap.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/conf.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/mnport.h>
#include <sys/munet/mninfo.h>
#include <sys/munet/bpdata.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uisw.h>
#include <fcntl.h>
#include "rbq.h"

struct uixmitport {
    short flag;
    short bufid;
} uixmitport[NXPORTS];
int mn_seqn;
short ipid;
short childrecv_ptno;

extern short my_version;
extern nodev(), uireceive();
extern struct mnport *get_port();
extern struct munetinfo *get_minfo();

uiinit()
{
    register int i;
    register int ret;

    mn_seqn = 0;
    ipid = 0;
    init_nodes();
    init_ports();
    init_minfos();
    free_mnptno = PTNOMIN;

    for (nlandev = 0; ldevsw[nlandev].lan_init != 0; nlandev++);

    for (i = 0; i < nlandev; i++) {
      lbl_5c:
        if (ldevsw[i].lan_init == nodev) {
            ldevsw[i].lan_error = ENODEV;
            continue;
        }

        ret = (*ldevsw[i].lan_init)(uireceive);
        if (ret < 0)
            ldevsw[i].lan_error = ret;
        else if (ret == 0)
            my_version = OPTIMIZED_NOFR;
    }

    u.u_error = 0;
}

uimount(dev, ipaddr)
dev_t dev;
ipnetaddr ipaddr;
{
    register struct ldevsw *lp;
    register int maj = major(dev);
    struct mnpacket *mnp;
    ushort bufid;
    int node;
    
    if (dev == NODEV || (maj = major(dev)) < 0 || maj >= nlandev) {
        u.u_error = ENODEV;
        return;
    }
    if (ipaddr == utsname.ipname || ipaddr == ipnoname || ipaddr == ipbcname) {
        u.u_error = EINVAL;
        return;
    }
    if (utsname.ipname == ipnoname || utsname.ipname == ipbcname) {
        u.u_error = EINVAL;
        return;
    }
    if (UISAME(&utsname.uiname, &noname) || UISAME(&utsname.uiname, &bcname)) {
        u.u_error = EINVAL;
        return;
    }
    if (uifindnode1(dev) >= 0 || uifindnode3(ipaddr) >= 0) {
        u.u_error = EBUSY;
        return;
    }

    lp = &ldevsw[maj];
    if ((maj = lp->lan_error) != 0) {
        u.u_error = maj;
        return;
    }

    if ((node = uiopennode(dev, ipaddr)) < 0) { /* open node */
        u.u_error = ENOSPC;
        return;
    }

    if ((mnp = (struct mnpacket*)buf_get(dev, UIMIN_MNPKT, RT_HEADER_SIZE, &bufid)) == 0) {
        uiclosenode(node);
        return;
    }
    mnp->e_dest = bcname;
    mnp->uu.uu_type = BCTYPE;
    mnp->uu.uu_argi[0] = my_version;
    mnp->uu.uu_timestamp = time;
    mnp->uu.uu_callno = UIGUARD;

    St_write("uimount_NFS6", 7, 1, 1, 0x0f, mnp->uu.uu_type);
    St_write("uimount_RPC25", 8, 1, mnp->uu.uu_type, 0x0f);
    if (uiwrites(bufid, uinode[node].nd_ipaddr, ETHERPUP_IPTYPE) != 0) {
        uinode[node].nd_dev = NODEV;
        u.u_error = EIO;
    }
}

uiumount(dev)
register dev_t dev;
{
    register struct proc *pp;
    register int node;
    register int i;
    struct mnpacket *mnp;
    ushort bufid;

    if ((node = uifindnode1(dev)) < 0) {
        u.u_error = EINVAL;
        return;
    }
    
    for (pp = &proc[1], i = 0; pp < (struct proc*)v.ve_proc; ++pp, i++) {

#define MIP (pp->p_munetinfo)
        if (MIP) {
            if (MIP->mi_remport[node] != 0 ||
                    (MIP->mi_uport && MIP->mi_uport->pt_yournode == node) ||
                    (MIP->mi_kport && MIP->mi_kport->pt_yournode == node)) {
                u.u_error = EBUSY;
                return;
            }
        }
    }

    if ((mnp = (struct mnpacket*)buf_get(dev, UIMIN_MNPKT, RT_HEADER_SIZE, &bufid)) != 0) {
        mnp->e_dest = uinode[node].nd_etaddr;
        mnp->uu.uu_type = BCTYPE;
        mnp->uu.uu_callno = UIREAD;
        mnp->uu.uu_timestamp = time;
        
        St_write("uiumount_NFS7", 7, 1, mnp->uu.uu_callno, 0x0f, mnp->uu.uu_type);
        St_write("uiumount_RPC26", 8, 1, mnp->uu.uu_type, 0x0f);
        if (uiwrites(bufid, uinode[node].nd_ipaddr, ETHERPUP_IPTYPE) != 0)
            St_write("uisend_RPC2", 8, 1, -1, 0x14);
    }
    
    uinode[node].nd_dev = NODEV;
}

uireceive(qp, unused, enqflag)
struct rbq *qp;
int unused;
int enqflag;
{
    register struct uupacket *up;
    register struct mnpacket *mnp;
    register short toport, fromport;
    register int i;
    int node;
    ushort bufid = qp->r_bufid;
    int s = spldisk();
    
    switch (fr_reas(&qp)) {
    case -1:
        buf_rel(bufid);
        return 0;
    case IP_FMORE:
        return 0;
    case IP_FLAST:
        break;
    default:
        buf_rel(bufid);
        return 0;
    }

    bufid = qp->r_bufid;
    mnp = (struct mnpacket*)qp->r_pkptr;
    if ((mnp->ip_word5 & 0xff) != IP_PROTO_MUNET) {
        splx(s);
        St_write("uireceive_ETH1", 9, 2, -1, 0x19, -1, -1);
        buf_rel(bufid);
        return 0;
    }

    up = &mnp->uu;
    St_write("uireceive_RPC10", 8, 2, up->uu_type, 0x0f);

    node = uifindnode3(mnp->ip_srcaddr);
    if (node >= 0)
        uinode[node].nd_time = time;
    if (up->uu_type == RTTYPE) {
        dlreceive(qp->r_bufid, qp->r_pkptr, 0);
        buf_rel(qp->r_bufid);
        return 0;
    } else if (up->uu_type == UUTYPE ||
            (master==0 && up->uu_type == UPTYPE && UISAME(&mnp->e_source, &master_id))) {
        toport = up->uu_toport;
        fromport = up->uu_fromport;
        if (node < 0 || toport <= 0) {
            splx(s);
            St_write("uireceive_RPC16", 8, 2, -1, 0x15);
            buf_rel(qp->r_bufid);
            return 0;
        }

        uinode[node].nd_time = time;
        switch (up->uu_callno) {
        case UIICONNVERS:
            uinode[node].nd_vers = my_version < up->uu_argi[0] ? my_version : up->uu_argi[0];
            break;
        case UIICONN:
            uinode[node].nd_vers = OLD_VERSION;
            break;
        default:
            break;
        }

        splx(s);
        if (send_port(toport, qp, fromport, node, enqflag) != 0)
            buf_rel(qp->r_bufid);
        return 0;
    } else if (up->uu_type == BCTYPE) {
        if (node >= 0) {
            uinode[node].nd_flag |= 3;
            uinode[node].nd_time = time;
            uinode[node].nd_etaddr = mnp->e_source;
            if (up->uu_callno != 0) {
                uicleanup(node);
                switch (up->uu_callno) {
                case UIIREMVERS:
                    uinode[node].nd_vers = my_version < up->uu_argi[0] ? my_version : up->uu_argi[0];
                    break;
                case UIIREM:
                    uinode[node].nd_vers = OLD_VERSION;
                    break;
                default:
                    break;
                }
            }
            if (WHAT_VERSION(node) >= OPTIMIZED_NOFR)
                uinode[node].nd_timediff = up->uu_timestamp - time;
        }
        if (master) {
            for (i = 0; i < maxdlnactv; i++) {
                if ((dltable[i].dl_flags & DL_CONNECTED) && 
                        dltable[i].dl_ipaddr == mnp->ip_srcaddr) {
                    dltable[i].dl_lastin = time;
                    break;
                }
            }
        }
    } else if (up->uu_type == UPTYPE)
        St_write("uireceive_RPC18", 8, 2, -1, 0x15);
    else
        St_write("uireceive_RPC15", 8, 2, -1, 0x19);
    
    splx(s);
    buf_rel(qp->r_bufid);
    return 0;
}

uiioctl(fd, cmd, arg)
int fd;
short cmd;
caddr_t arg;
{
    switch(cmd) {
    default:
        u.u_error = EINVAL;
        break;
    }
}

uisend(dev, flag)
dev_t dev;
int flag;
{
    register struct uupacket *up;
    register struct mnport *mnp;
    register struct uinode *uip;
    
    struct mnpacket *mup, *mup2;
    struct ldevsw *lanp;
    int node; 
    int etherr;
    int mayintr;
    int retries;
    int uisz;
    int payload;
    int hasproc;
    int phase;
    int var76;
    short rstime;
    int seqn1, seqn2;
    caddr_t dirp;
    ushort bufid;
    short remport;
    short vers;
    int prio;
    int count;
    int callno;


    phase = 0;                          /* =0 means first run */
    hasproc = 0;                        /* =1 means 'check process (UIIUARD) */
    var76 = 0;

    node = uifindnode1(dev);
    if (node < 0) {
        u.u_error = ENOENT;
        return;
    }
    
    uip = &uinode[node];
    if (uip->nd_time == 0) {
        u.u_error = EIO;
        return;
    }

    if ((etherr = getether(node)) != 0) {
        u.u_error = etherr;
        return;
    }

    if (u.u_procp->p_munetinfo == 0)
        u.u_procp->p_munetinfo = get_minfo();

    remport = u.u_procp->p_munetinfo->mi_remport[node];
    if (remport == 0) {
        if (uifirst(u.u_callno))
            phase = 1;
        else {
            u.u_error = EIO;
            return;
        }
    }
    
    lanp = &ldevsw[(int)(dev>>(uint)8)];
    mnp = get_port(u.u_procp, 2, -1, phase != 0 ? 1 : remport, node);
    if (mnp == 0)
        return;

mainloop:
    INC_SEQN(mn_seqn);
    clearport(mnp);
    if (hasproc == 0)
        seqn1 = mn_seqn;
    else
        seqn2 = mn_seqn;

    retries = 0;
    dirp = u.u_dirp;                /* save dirp */
    rstime = RSTIME + RSTIME * (time % 16) / 16;

    /* retry loop */
retry:
    if (remport <= 0 && phase == 0) {
        u.u_error = EIO;
        return;
    }
    if (hasproc==0)
        uisz = phase != 0 ? uisizesw(UIICONNVERS, node) : uisizesw(u.u_callno, node);
    else
        uisz = uisizesw(UIIGUARD, node);

    if ((mup = (struct mnpacket*)buf_get(dev, uisz, RT_HEADER_SIZE, &bufid)) == 0)
        return;
        
    up = &mup->uu;
    if (hasproc == 0) {
        vers = WHAT_VERSION(node);
        switch (vers) {
        case NOT_KNOWN:
        case OLD_VERSION:
            payload = UIMAXDATA_OLD;
            break;
        case OPTIMIZED_NOFR:
            payload = UIMAXDATA_NEW_NOFR;
            break;
        case OPTIMIZED_FRAG:
            payload = UIMAXDATA_NEW_FRAG;
            break;
        default:
            printf("uisend (2): unknown version type %hd\n", vers);
            uip->nd_vers = 0;
            u.u_error = EINVAL;
            break;
        }
            
        mayintr = phase != 0 ? 0 : uisinsw(bufid, dev, payload, node);
    }
    if (u.u_error) {
        buf_rel(bufid);
        return;
    }

    mup->e_dest = uip->nd_etaddr;
    up->uu_type = UUTYPE;
    up->uu_node = ipnoname;
    up->uu_toport = (phase != 0 || hasproc) != 0 ? 1 : remport;
    up->uu_fromport = mnp->pt_myptno;
    if (hasproc)
        up->uu_callno = UIIGUARD;
    else {
        vers = WHAT_VERSION(node);
        switch (vers) {
        case NOT_KNOWN:
        case OPTIMIZED_NOFR:
        case OPTIMIZED_FRAG:
            if (phase != 0) {
                up->uu_callno = UIICONNVERS;
                up->uu_argi[0] = my_version;
            } else
                up->uu_callno = u.u_callno;
            break;
        case OLD_VERSION:
            up->uu_callno = phase != 0 ? UIICONN : u.u_callno;
            break;
        default:
            printf("uisend (3): unknown version type %hd\n", vers);
            uip->nd_vers = 0;
            u.u_error = EINVAL;
            break;
        }
    }

    up->uu_euid = u.u_uid;
    up->uu_egid = u.u_gid;
    up->uu_stamp = hasproc ? seqn2 : seqn1;
    up->uu_timestamp = bootime;
    up->uu_datalen = uisz - EIP_HEADER_SIZE;
    up->uu_error = 0;
    callno = up->uu_callno;

    if (var76 && !hasproc) {
        up->uu_signal = -fsig(u.u_procp->p_sig);
        if (up->uu_signal == SIGKILL)
            up->uu_signal = SIGTERM;
    } else 
        up->uu_signal = 0;
    if (hasproc) {
        up->uu_altport = remport;
        conf_port(mnp, 1, node);
    }
        
    St_write("uisend_RPC1", 8, 1, up->uu_type, 0x0f);

    if (retries && !hasproc)
        St_write("uisend_RPC3", 8, 1, -1, 0x13);
    else {
        if (callno == UIICONNVERS)
            St_write("uisend_NFS1", 7, 1, -1, 0x0f, UUTYPE);
        else
            St_write("uisend_NFS1", 7, 1, callno, 0x0f, UUTYPE);
    }
    if (uiwrites(bufid, uip->nd_ipaddr, ETHERPUP_IPTYPE) != 0) {
        u.u_error = EIO;
        St_write("uisend_RPC2", 8, 1, -1, 0x14);
        return;
    }
        
    prio = (mayintr || uip->nd_vers<=OLD_VERSION ||
            (callno >= UIICONNVERS && callno <= UIIINCR) ||
            callno == UIICONN || callno == UIIUWHO);

readmore:
    count = read_port(mnp, rstime, prio && !var76);
    if (count < 0) {
        if (count == -2) {
            if ((!mayintr && hasproc==0) || uip->nd_vers <= OLD_VERSION)
                u.u_error = EINTR;
            else {
                var76 = 1;
                hasproc = 0;
                rstime = RSTIME;
            }
        }
        if (u.u_error == 0) {
            if (retries++ < 4) {
                u.u_dirp = dirp;
                goto retry;
            }
            /* too many retries */
            if (hasproc==0 && mayintr) {
                retries = 0;
                u.u_dirp = dirp;
                hasproc = 1;
                rstime = WSTIME;
                goto mainloop;
            }

            if (hasproc);                   /*bug?*/
            if (up->uu_toport != 1)
                St_write("uisend_CON1", 0x0b, 0x1d);
            conf_port(mnp, 0, 0);
            u.u_error = EIO;
            ins_minfo(u.u_procp->p_munetinfo, node, 0);
            return;
        }
    }   
        
    if (u.u_error == 0) {
        if (mnp->pt_rbufd) {
            mup2 = (struct mnpacket*)mnp->pt_rbufd->r_pkptr;
            bufid = mnp->pt_rbufd->r_bufid;
        } else {
            u.u_error = EIO;
            return;
        }
        if (hasproc && mup2->uu.uu_stamp == seqn2) {
            St_write("uisend_NFS4", 7, 2, UIIGUARD, 0x0f, UUTYPE);
            if (mup2->uu.uu_error) {
                St_write("uisend_CON2", 0x0b, 0x1d);
                conf_port(mnp, 0, 0);
                u.u_error = EIO;
                ins_minfo(u.u_procp->p_munetinfo, node, 0);
                return;
            }
            conf_port(mnp, remport, node);
            rstime = LGTIME;
            hasproc = 0;
            retries = 3;
            goto retry;
        }
        if (mup2->uu.uu_stamp == seqn1) {
            if (phase != 0) {
                if (callno == UIICONNVERS) {
                    if (mup2->uu.uu_error) {
                        uip->nd_vers = OLD_VERSION;
                        clean_port(mnp);
                        goto mainloop;
                    }
                    uip->nd_vers = my_version < mup2->uu.uu_argi[0] ? my_version : mup2->uu.uu_argi[0];
                    uip->nd_timediff = mup2->uu.uu_timestamp - time;
                }
                remport = mup2->uu.uu_altport;
            } else
                uisoutsw(bufid, dev, payload, node);
            if (mup2->uu.uu_signal > 0)
                psignal(u.u_procp, mup2->uu.uu_signal);
        } else {
            St_write("uisend_RPC11", 8, 2, -1, 0x17);
            clean_port(mnp);
            u.u_dirp = dirp;
            rstime = RSTIME;
            goto readmore;
        }
    }

    if (u.u_error == EINTR || u.u_error == EIO)
        St_write("uisend_RPC12", 8, 2, -1, 0x14);
    else
        St_write("uisend_NFS4", 7, 2, mup2->uu.uu_callno, 0x0f, mup2->uu.uu_type);
        
    clean_port(mnp);
    if (u.u_error) return;

    if (phase) {
        uip->nd_flag |= 3;
        ins_minfo(u.u_procp->p_munetinfo, node, remport);
        conf_port(mnp, remport, node);
        St_write("uisend_CON3", 0x0b, 0x1c);
        if (!flag) {
            phase = 0;
            goto mainloop;
        }
    }
    return 0;
}

uixmitdone(bufid, arg)
register ushort bufid;
int arg;
{
    register struct uixmitport *xp;
    register int s = spldisk();
    
    for (xp = &uixmitport[0]; xp < &uixmitport[NXPORTS]; xp++) {
        if ((xp->flag & XPTAKEN) && bufid == xp->bufid && !(xp->flag & XPDONE)) {
            if (arg <= 0)
                xp->flag |= XPERROR;
            xp->flag |= XPDONE;
            wakeup((caddr_t)xp);
            buf_rel(bufid);
            splx(s);
            return 0;
        }
    }
    
    buf_rel(bufid);
    splx(s);
    return 0;
}

uiwriteswake(xp)
register struct uixmitport *xp;
{
    register int s = spldisk();
    if (xp->flag & XPTAKEN) {
        xp->flag |= XPTIMEO;
        wakeup((caddr_t)xp);
    }
    splx(s);
}

uiwrites(bufid, ipaddr, pktype)
ushort bufid;
ipnetaddr ipaddr;
short pktype;
{
    register struct uixmitport *xp;
    register int ret;

    spldisk();

    for (xp = &uixmitport[1]; xp < &uixmitport[NXPORTS]; xp++) {
        if (xp->flag == 0) break;
    }
    if (xp >= &uixmitport[NXPORTS])
        xp = &uixmitport[0];

    xp->flag = XPTAKEN;
    xp->bufid = fr_trans(bufid, ipaddr, pktype, uixmitdone);
    timeout(uiwriteswake, xp, WSTIME);
    spl0();
    while ((xp->flag & (XPDONE|XPTIMEO))==0)
        sleep((caddr_t)xp, WPRI);

    spldisk();
    if ((xp->flag & XPDONE)==0)
        xp->flag |= XPERROR;
    else if ((xp->flag & XPTIMEO)==0)
        cancelto(uiwriteswake, xp);
    
    ret = (xp->flag & XPERROR) ? 1 : 0;
    if (ret)
        St_write("uiwrites_ETH12", 9, 1, -1, 0x14, -1, -1);
    
    xp->flag = 0;
    spl0();
    return ret;
}

uicleanup(n)
register int n;
{
    register struct proc *pp;

    for (pp = &proc[1]; pp < (struct proc*)v.ve_proc; pp++) {
        if (pp->p_munetinfo == 0) continue;
        if (pp->p_munetinfo->mi_remport[n]) {
            St_write("uisend_CON2", 0x0b, 0x1d);
            ins_minfo(pp->p_munetinfo, n, 0);
        }
        if ((pp->p_flag & SFSERV) && pp->p_munetinfo->mi_uport &&
                pp->p_munetinfo->mi_uport->pt_yournode == n && pp->p_stat < SZOMB)
            psignal(pp, SIGKILL);
    }
}

uiupdate() 
{
    register int i, k;
    struct mnpacket *mnp;
    int bufid;
    
    for (i = 0; i < nlandev; i++) {
        if (ldevsw[i].lan_error) continue;
        for (k = 0; k < maxdlnactv; k++) {
            if ((dltable[k].dl_flags & DL_CONNECTED) && bufmaj(dltable[k].dl_bufid) == i)
                break;
        }
        
        if (k < maxdlnactv) {
            if ((mnp = (struct mnpacket*)buf_get(bufdev(dltable[k].dl_bufid), 
                    UIMIN_MNPKT, RT_HEADER_SIZE, &bufid))) {
                mnp->e_dest = bcname;
                mnp->uu.uu_type = UPTYPE;
                mnp->uu.uu_node = ipnoname;
                mnp->uu.uu_toport = 2;
                mnp->uu.uu_fromport = 0;
                mnp->uu.uu_callno = 0;
                mnp->uu.uu_datalen = 0x38; /*sizeof(?)*/
                mnp->uu.uu_error = 0;
                St_write("uiupdate_RPC24", 8, 1, mnp->uu.uu_type, 0x0f);
                if (uiwrites(*(ushort*)&bufid, ipbcname, ETHERPUP_IPTYPE))
                    St_write("uipacket_RPC23", 8, 1, -1, 0x14);
            }
        }
    }
}

copypath(path)
register caddr_t path;
{
    register caddr_t cp;
    register int i;
    register ch;
    
    for (cp = --u.u_dirp, i = 0; i<UIMAXPATH; i++) {
        ch = fsbyte(cp++);
        if (ch == -1) {
            u.u_error = EFAULT;
            *path = '\0';
            return;
        }
        path[i] = ch;
        if (ch == '\0') break;
    }
    
    if (path[0] == '\0') {
        path[0] = '/';
        path[1] = '\0';
    }
    if (i >= UIMAXPATH) {
        u.u_error = EINVAL;
        path[UIMAXPATH-1] = '\0';
    }
}

uidobig(ip)
struct inode *ip;
{
    register char *pfp;
    register int node;
    int cnt;
    dev_t dev;
    
    dev = ip->i_mount->m_dev;
    if ((node = uifindnode1(dev)) < 0) {
        u.u_error = ENOENT;
        return;
    }
    
    switch (u.u_callno) {
    case UISREAD:
    case UISWRITE:
    case UISLREAD:
    case UISLWRITE:
    case UISGETDENTS:
        {
            register struct a {
                int fd;
                caddr_t buf;
                int cnt;
            } *uap = (struct a*)u.u_ap;
            pfp = &u.u_pofile[uap->fd];
            u.u_munet = uap->fd;
            u.u_base = uap->buf;
            u.u_count = uap->cnt;
            u.u_segflg = 0;
            cnt = u.u_count;
            *pfp &= ~8;
            *pfp |= 4;
            do {
                uisend(dev, 0);
                *pfp &= ~4;
            } while (u.u_error==0 && (*pfp & 8)==0);
            u.u_rval1 = cnt - u.u_count;
            break;
        }

    case UISSEEK:
        {
            register struct file *fp;
            register struct a {
                int fd;
                int off;
                int whence;
            } *uap = (struct a*)u.u_ap;
            register short vers;
            register int off;
            
            pfp = &u.u_pofile[uap->fd];
            vers = WHAT_VERSION(node);
            switch (vers) {
            case NOT_KNOWN:
            case OLD_VERSION:
                uisend(dev, 0);
                break;
            case OPTIMIZED_NOFR:
            case OPTIMIZED_FRAG:
                if (*pfp & 0x10) {
                    u.u_error = ESPIPE;
                    return;
                }
                fp = u.u_ofile[uap->fd];
                if (uap->whence == 2 || fp->f_count != 1) {
                    uisend(dev, 0);
                    break;
                }
                off = uap->off;
                if (uap->whence == 1) {
                    off += fp->f_offset;
                } else if (uap->whence != 0) {
                    u.u_error = EINVAL;
                    psignal(u.u_procp, SIGSYS);
                    return;
                }
                if (off < 0) {
                    u.u_error = EINVAL;
                    return;
                }
                *pfp |= 0x20;
                fp->f_offset = off;
                u.u_rval1 = off;
                return;
            default:
                printf("uidobig (2): unknown version type %hd\n", vers);
                u.u_error = EINVAL;
            }
            break;
        }
    default:
        uisend(dev, 0);
        break;
    }
}

uiexit(mup)
register struct munetinfo *mup;
{
    register struct mnport *mnp;
    register int i;
    register int s = spldisk();

    for (i=0; i<UIMAXNODES; i++) {
        if (mup->mi_remport[i]) {
            u.u_error = 0;
            u.u_callno = UIIDECR;
            uisend(uinode[i].nd_dev, 0);
        }
    }

    mnp = mup->mi_uport;
    if (mnp) {
        rem_openport(mnp);
        put_freeport(mnp);
    }
    mnp = mup->mi_kport;
    if (mnp) {
        rem_openport(mnp);
        put_freeport(mnp);
    }

    put_minfo(mup);
    splx(s);
    u.u_error = 0;
}

wcore(ip)
register struct inode *ip;
{
    if (ip->i_flag & ILAND) {
        while (u.u_count > 0 && u.u_error==0) {
            u.u_callno = UIIWRITE;
            uisend(u.u_cdirdev, 0);
        }
    } else
        writei(ip);
}

uifork(pp, cp)
register struct proc *pp; /*parent*/
register struct proc *cp; /*child*/
{
    register struct file *fp;
    register int i;
    char nodes[UIMAXNODES];
    struct mnport *mnp;
    int errsave;
    dev_t ndev;
    
    for (i=0; i<UIMAXNODES; i++) nodes[i] = 0;
    errsave = u.u_error;
    
    if (u.u_cdirdev != NODEV)
        nodes[uifindnode1(u.u_cdirdev)] = 1;
    if (u.u_crootdev != NODEV)
        nodes[uifindnode1(u.u_crootdev)] = 1;

    for (i=0; i<NOFILE; i++) {
        if (u.u_rofile[i] && (fp=u.u_ofile[i]) && (fp->f_inode->i_flag & ILAND)) {
            nodes[uifindnode1(fp->f_inode->i_mount->m_dev)] = 1;
            uikilllseek(i);
        }
    }

    for (i=0; i < UIMAXNODES; i++) {
        if (!nodes[i]) continue;
        ndev = uinode[i].nd_dev;
        u.u_error = 0;
        mnp = get_port(cp, 2, -1, -1, uifindnode1(ndev));
        if (mnp) {
            u.u_callno = UIIINCR;
            u.u_munet = mnp->pt_myptno;
            uisend(ndev, 0);
        } else
            childrecv_ptno = 0;
        
        ins_minfo(cp->p_munetinfo, i, childrecv_ptno);
        childrecv_ptno = 0;
    }
    u.u_error = errsave;
}

fixuplan(ip)
struct inode *ip;
{
    int errsave;
    if (ip->i_flag & ILAND) {
        errsave = u.u_error;
        u.u_callno = UIIICLO;
        u.u_error = 0;
        uisend(ip->i_mount->m_dev, 0);
        u.u_error = errsave;
    }
}

struct buf *readehd(ip)
struct inode *ip;
{
    struct buf *bp = geteblk();
    
    u.u_base = bp->b_un.b_addr;
    u.u_segflg = 1;

    while (u.u_count > 0) {
        u.u_callno = UIIREAD;
        uisend(ip->i_mount->m_dev, 0);
        if (u.u_error) {
            u.u_error = ENOEXEC;
            brelse(bp);
            return 0;
        }
    }
    return bp;
}

readowner(ip)
struct inode *ip;
{
    int node = uifindnode1(ip->i_mount->m_dev);
    short vers = uinode[node].nd_vers;

    if (vers <= OLD_VERSION) {
        u.u_callno = UIIGUID;
        uisend(ip->i_mount->m_dev, 0);
        if (u.u_error)
            return -1;
    } else {
        u.u_rrflag = u.u_idflag;
        u.u_munet = u.u_isid;
    }
    
    if (u.u_rrflag & 8)
        u.u_uid = u.u_munet >> 16;
    if (u.u_rrflag & 4)
        u.u_gid = u.u_munet & 0xffff;
    return 0;
}

int uireadi(ip)
struct inode *ip;
{
    while (u.u_count > 0) {
        u.u_callno = UIIREAD;
        uisend(ip->i_mount->m_dev, 0);
        if (u.u_error)
            return -1;
    }
    return 0;
}

uikilllseek(fd)
int fd;
{
    struct a {      /* args for lseek syscall */
        int fd;
        int off;
        int whence;
    };
    
    register long *uapsave;
    register struct file *fp;
    register struct a *ap;
    int callnosave, errorsave;
    dev_t dev; short dummy;
    struct a args;

    if (fd >= 0 && fd <= NOFILE && (u.u_pofile[fd] & 0x20)) {
        callnosave = u.u_callno;
        errorsave = u.u_error;

        args.fd = fd;
        args.off = u.u_ofile[fd]->f_offset;
        args.whence = 0;
        uapsave = u.u_ap;
        ap = &args;
        u.u_ap = (long*)ap;

        u.u_callno = UISSEEK;
        fp = u.u_ofile[fd];
        dev = fp->f_inode->i_mount->m_dev;
        uisend(dev, 0);
    
        u.u_ap = uapsave;
        u.u_pofile[fd] &= ~0x20;
        u.u_callno = callnosave;
        u.u_error = errorsave;
    }
}

newversion(ver)
int ver;
{
    register struct munetinfo *mip;
    register struct mnport *mnp;

    mip = u.u_procp->p_munetinfo;
    if ((mip && (mnp=mip->mi_uport)) != 0)
        return uinode[mnp->pt_yournode].nd_vers >= ver;
    else
        return 0;

}
