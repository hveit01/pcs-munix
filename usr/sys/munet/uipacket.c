/* PCS specific */

/* This is the central MUNIX system call entry */

static char *_Version = "@(#) RELEASE:  1.9  May 18 1987 /usr/sys/munet/uidoit.c";

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

short my_version = 4;

extern int mn_seqn;
extern struct munetinfo *get_minfo();
extern struct mnport *get_port();
extern struct proc *find_ptproc();

/* uipacket system call
 * uipacket(caddr_t, short)
 */
uipacket()
{
    register struct mnport *portp;
    register struct uinode *uin;
    register int i, k, err;
    
    struct uupacket *up;                /* payload packet */
    struct mnpacket *mnp1, *mnp2;       /* munix packet */
    struct ldevsw *lanp;
    struct a {                          /* args passed from syscall dispatcher */
        caddr_t arg;
        int cmd;
    } *a;

    int seqn;
    int pktlen;                         /* length of entire packet */
    int datalen;                        /* length of uu_data */
    int sig;                            /* uu_signal */
    int retries;                        
    short callno;                       /* uu_callno */
    short pid;                          
    short unused1;                      /* unused slack */
    short remport;
    short fromport;
    short killport;
    ipnetaddr uunode;                   /* uu_node */
    enetaddr unused2;                   /* unused slack */
    ushort bufid;
    int uucount;                        /* uu_count */
    struct mount *mp;                   /* mount point of diskless swap */
    short rstime;
    /* local vars exist within case blocks */

    u.u_rval1 = 0;                      /* return value */
    pid = u.u_procp->p_pid;             

    a = (struct a*)u.u_ap;
    up = (struct uupacket*)a->arg;  /* cmd specific argument */
    
    switch (a->cmd) {
    case UIREAD:
    case UIREADT:
        if (u.u_procp->p_munetinfo==0) {
            u.u_error = EIO;
            return;
        }
        portp = u.u_procp->p_munetinfo->mi_uport;
        if (portp == 0) {
            u.u_error = EIO;
            return;
        }
        spldisk();
        if (portp->pt_rc <= 0) {
            if (a->cmd == UIREADT) {
                u.u_rval1 = 0;
                spl0();
                return;
            }
            portp->pt_sflags |= PT_WAIT;
            while (portp->pt_rc <= 0)
                sleep((caddr_t)portp, RPRI);
        }
        spl0();
        if (portp->pt_rbufd)
            mnp2 = (struct mnpacket*)portp->pt_rbufd->r_pkptr;
        else 
            return;
        
        up = &mnp2->uu;
        up->uu_node = mnp2->ip_srcaddr;
        u.u_error = 0;
        St_write("uipacket_NFS3", 7, 2, up->uu_callno, 0xf, up->uu_type);
        if (up->uu_callno == UIICONN || up->uu_callno == UIICONNVERS)
            St_write("uipacket_CON4", 0x0b, 0x1c);
        buf_copy(portp->pt_rbufd->r_bufid, up, a->arg, up->uu_datalen, 0, 1);
        clean_port(portp);
        u.u_rval1 = 1;
        break;

    case UIRDSW:
        uiprdsw();
        break;

    case UIWTSW:
        uipwtsw();
        break;

    case UIWRITE:
    case UIWRITER:

        retries = 0;
        uunode = fulong(&up->uu_node);
        callno = fuword(&up->uu_callno);
        fromport = fuword(&up->uu_fromport);
        if (u.u_error)
            return;

        switch (callno) {
        case UIIXONN:
            sulong(&up->uu_timestamp, time);
            break;
        
        case UIIREAD:
        case UIIURUN:
            uucount = u.u_count = fuword(&up->uu_count);
            break;
        
        case UISREAD:
        case UISWRITE:
        case UISLREAD:
        case UISLWRITE:
        case UISGETDENTS:
            {
                short fd = fuword(&up->uu_fd);
                int off;
                uucount = fuword(&up->uu_count);
                
                if (uucount == -1)
                    uucount = 0;
                off = u.u_ofile[fd]->f_offset;
                sulong(&up->uu_offset, off);
                break;
            }
        default:
            uucount = 0;
            break;
        }

        /* k is index into uinode */
        if ((k = uifindnode3(uunode)) < 0) {
            u.u_error = ENOENT;
            return;
        }

        St_write("uipacket_NFS2", 7, 1, callno, 0x0f, up->uu_type);
        if (u.u_procp->p_munetinfo == 0)
            u.u_procp->p_munetinfo = get_minfo();

        if (callno == UIIUWHO || callno == UIIURUN) {
            if ((remport = u.u_procp->p_munetinfo->mi_remport[k]) == 0) {
                uisend(uinode[k].nd_dev, 1);
                if (u.u_error)
                    return;
                remport = u.u_procp->p_munetinfo->mi_remport[k];
            }
            suword(&up->uu_toport, remport);
            if (u.u_error)
                return;

            if (callno == UIIURUN) {
                St_write("uisend_CON2", 0x0b, 0x1d);
                ins_minfo(u.u_procp->p_munetinfo, k, 0);
            }
        } else {
            if ((err = getether(k)) != 0) {
                u.u_error = err;
                return;
            }
            remport = fuword(&up->uu_toport);
        }
        lanp = &ldevsw[major(uinode[k].nd_dev)];
        if ((err = lanp->lan_error) != 0) {
            u.u_error = err;
            return;
        }
        if (a->cmd == UIWRITER) {
            if ((portp = get_port(u.u_procp, 1, 0, 0, 0)) == 0)
                return;
            INC_SEQN(mn_seqn);
            seqn = mn_seqn;
        }
        
        pktlen = uisizeswx (callno, k, uucount);
        datalen = pktlen - EIP_HEADER_SIZE;
retry:
        if ((mnp1 = (struct mnpacket*)buf_get(
                uinode[k].nd_dev, pktlen, RT_HEADER_SIZE, &bufid))==0)
            return;

        up = &mnp1->uu;
        if (buf_copy(bufid, up, a->arg, datalen, 0, 0) == -1) {
            buf_rel(bufid);
            return;
        }
            
        mnp1->e_dest = uinode[k].nd_etaddr;
        if (a->cmd == UIWRITER) {
            up->uu_stamp = seqn;
            up->uu_timestamp = bootime;
        }
            
        up->uu_type = UUTYPE;
        up->uu_euid = u.u_uid;
        up->uu_egid = u.u_gid;
        up->uu_datalen = datalen;
        St_write("uipacket_RPC6", 8, 1, up->uu_type, 0x0f);
        if (retries)
            St_write("uipacket_RPC7", 8, 1, -1, 0x13);
        if ((err = uiwrites(bufid, uunode, ETHERPUP_IPTYPE)) != 0) {
            St_write("uipacket_RPC5", 8, 1, -1, 0x14);
            u.u_error = err;
        }
        if (u.u_error != 0 || a->cmd == UIWRITE) {
            u.u_rval1 = 1;
            break;
        }

        rstime = RSTIME + (RSTIME * (time%16)/16);
        uucount = read_port(portp, rstime, 1);
        if (uucount < 0) {
            if (uucount == -2)
                u.u_error = EINTR;
            if (u.u_error == 0) {
                if (retries++ < 4)
                    goto retry;
                u.u_error = EIO;
            }
            St_write("uipacket_RPC13", 8, 2, -1, 0x14);
            return;
        }
        
        if (portp->pt_rbufd)
            mnp2 = (struct mnpacket*)portp->pt_rbufd->r_pkptr;
        else {
            u.u_error = EIO;
            return;
        }

        if (mnp2->uu.uu_stamp == seqn) {
            if (mnp2->uu.uu_signal > 0)
                psignal(u.u_procp, mnp2->uu.uu_signal);
            if (u.u_error == 0) {
                up = &mnp2->uu;
                up->uu_node = mnp2->ip_srcaddr;
                St_write("uipacket_NFS5", 7, 2, up->uu_callno, 0x0f, up->uu_type);
                buf_copy(portp->pt_rbufd->r_bufid, up, a->arg, up->uu_datalen, 0, 1);
            }
            clean_port(portp);
            u.u_rval1 = 1;
            return;
        }
        St_write("uipacket_RPC14", 8, 2, -1, 0x17);
        clean_port(portp);
        goto retry;
        
        /*NOTREACHED*/
        
    case UIURUNC:
        uunode = fulong(&up->uu_node);
        remport = fuword(&up->uu_altport);
        if (u.u_error)
            return;
        
        if ((k = uifindnode3(uunode)) < 0) {
            u.u_error = ENOENT;
            return;
        }
        if (remport <= 0) {
            u.u_error = EINVAL;
            return;
        }
        
        if (u.u_procp->p_munetinfo == 0)
            u.u_procp->p_munetinfo = get_minfo();
        ins_minfo(u.u_procp->p_munetinfo, k, remport);

        for (mp = &mount[1]; mp < (struct mount*)v.ve_mount; mp++) {
            if (mp->m_flags == MINUSE && FILE_IS_REMOTE(mp->m_inodp) &&
                    mp->m_dev == uinode[k].nd_dev)
                break;
        }
        if (mp == (struct mount*)v.ve_mount)
            u.u_error = ENOENT;
        else {
            closef(u.u_ofile[0]);
            for (i = 1; i < 4; i++) {
                if (u.u_ofile[i]==0) continue;
                if (u.u_ofile[i]->f_flag==1) {
                    changefd(i, 1);
                    break;
                }
            }
            for (i = 1; i < 4; i++) {
                if (u.u_ofile[i]==0) continue;
                if (u.u_ofile[i]->f_flag==2) {
                    changefd(i, 2);
                    break;
                }
            }
            for (i = 1; i < 4; i++) {
                if (u.u_ofile[i]==0) continue;
                if (u.u_ofile[i]->f_flag==3) {
                    changefd(i, 3);
                    break;
                }
            }
            if (u.u_ofile[1] && !u.u_rofile[1]) {
                u.u_ofile[0] = u.u_ofile[1];
                u.u_pofile[0] = u.u_pofile[1];
                u.u_rofile[0] = 0;
            } else {
                if ((u.u_ofile[0] = falloc(mp->m_inodp, 1)) == 0)
                    goto fail;
                mp->m_inodp->i_count++;
                u.u_rofile[0] = 1;
            }
            if (u.u_ofile[2] && !u.u_rofile[2]) {
                u.u_ofile[1] = u.u_ofile[2];
                u.u_pofile[1] = u.u_pofile[2];
                u.u_rofile[1] = 0;
            } else {
                if ((u.u_ofile[1] = falloc(mp->m_inodp, 2)) == 0)
                    goto fail;
                mp->m_inodp->i_count++;
                u.u_rofile[1] = 2;
            }
            if (u.u_ofile[3] && !u.u_rofile[3]) {
                u.u_ofile[2] = u.u_ofile[3];
                u.u_pofile[2] = u.u_pofile[3];
                u.u_rofile[2] = 0;
            } else {
                if ((u.u_ofile[2] = falloc(mp->m_inodp, 3)) == 0)
                    goto fail;
                mp->m_inodp->i_count++;
                u.u_rofile[2] = 3;
            }
            u.u_ofile[3] = 0;
            u.u_rofile[3] = 0;
            u.u_pofile[3] = 0;
            u.u_rval1 = 1;
            return;
        }

fail:
        ins_minfo(u.u_procp->p_munetinfo, k, 0);
        for (i=0; i < 2; i++) {
            register struct file *fp = u.u_ofile[i];
            if (fp) {
                plock(fp->f_inode);
                iput(fp->f_inode);
                fp->f_count = 0;
                fp->f_next = ffreelist;
                ffreelist = fp;
                u.u_ofile[i] = 0;
            }
        }
        u.u_rofile[0] = 0;
        u.u_rofile[1] = 0;
        u.u_rofile[2] = 0;
        break;

    case UIMONITOR:
        {
            int cnt;                    /* 108-105 */

            i = ((short*)(a->arg))[0];  /* LAN device */
            cnt = ((short*)(a->arg))[1];    /* entries to return */
            if (cnt <= 0) {
                u.u_error = EINVAL;
                return;
            }
            *(int*)(a->arg) = 0;        /* clear returned bufptr */
            *(short*)(a->arg) = 0;      /* clear count (BUG?) */

            if (i < 0 || i >= nlandev) {
                u.u_error = ENODEV;
                return;
            }
            spldisk();
            if ((err = (*ldevsw[i].lan_monitor)(a->arg, cnt)) != 0)
                u.u_error = err;
            spl0();
            u.u_rval1 = 0;
            break;
        }
        
    case UIVERS:
        {
            int dummy;
            int flag;
            ipnetaddr ipaddr = fulong(&up->uu_node);
            
            flag = 0;
retry2:
            if ((i = uifindnode3(ipaddr)) < 0)
                return;
            if (WHAT_VERSION(i)==0 && flag==0 && uinode[i].nd_time!=0) {
                flag++;
                uisend(uinode[i].nd_dev, 1);
                goto retry2;
            }
            suword(&up->uu_argi[0], WHAT_VERSION(i));
            return;
        }

    case UINDINFO:
        {
            int n;
            ipnetaddr ipaddr;
            ipnetaddr *arg = (ipnetaddr*)a->arg;
    
            ipaddr = fulong(&arg[0]);
            if (ipaddr == utsname.ipname) {
                sulong(&arg[1], time);
                return;
            } else if ((n = uifindnode3(ipaddr)) != -1) {
                sulong(&arg[1], uinode[n].nd_time);
                return;
            } else {
                u.u_error = ENODEV;
                return;
            }
        }

    case UIXNAME:
        {
            int sz;

            i = ((short*)(a->arg))[0];      /* LAN device */
            sz = ((short*)(a->arg))[1]; /* size of buf */
            if (sz <= 0) {
                u.u_error = EINVAL;
                return;
            }
            if (i < 0 || i >= nlandev) {
                u.u_error = ENODEV;
                return;
            }

            *(int*)(a->arg) = 0;    /* clear returned bufptr */
            lanp = &ldevsw[i];

            if ((mnp1 = (struct mnpacket*)buf_get(
                    i<<8, UIMIN_MNPKT, RT_HEADER_SIZE, &bufid)) == 0)
                return;
            
            spldisk();
            if ((err = (*lanp->lan_xname)(a->arg, sz)) != 0) {
                buf_rel(bufid);
                u.u_error = err;
                spl0();
                return;
            }
            
            mnp1->e_dest = bcname;
            mnp1->e_source = utsname.uiname;
            mnp1->uu.uu_type = UNTYPE;
            St_write("uipacket_RPC8", 8, 1, mnp1->uu.uu_type, 0x0f);
            if ((err = uiwrites(bufid, -1, ETHERPUP_IPTYPE)) != 0) {
                u.u_error = err;
                St_write("uipacket_RPC22", 8, 1, -1, 0x14);
                spldisk();
                (*lanp->lan_xname)(a->arg, 0);
                spl0();
                return;
            }
            
            spldisk();
            if ((err=(*lanp->lan_xname)(a->arg, sz)) != 0)
                u.u_error = err;
            spl0();
            u.u_rval1 = 0;
            return;
        }
        
    case UIWATCHDOG:
        for (k = 0; k < UIMAXNODES; k++) {
            uin = &uinode[k];
            if (uin->nd_dev == NODEV) continue;
            if ((time - uin->nd_time) < WDOGT) continue;
            if (uin->nd_flag & 2)
                uicleanup(k);
            uin->nd_flag &= ~3;
            uin->nd_time = 0;
        }

        for (i=0; i < maxdlnactv; i++) {
            if ((dltable[i].dl_flags & DL_CONNECTED) && 
                    (time - dltable[i].dl_lastin) >= WDOGT)
                rmdlnode(i);
        }
        /*FALLTHRU*/

    case UICERBERUS:
        for (i=0; i < nlandev; i++) {
            if (ldevsw[i].lan_error) continue;
            if ((mnp1 = (struct mnpacket*)buf_get(
                    i << 8, UIMIN_MNPKT, RT_HEADER_SIZE, &bufid)) != 0) {
                mnp1->e_dest = bcname;
                mnp1->uu.uu_type = BCTYPE;
                mnp1->uu.uu_callno = 0;
                mnp1->uu.uu_timestamp = time;
                St_write("uipacket_RPC9", 8, 1, mnp1->uu.uu_type, 0x0f);
                if (uiwrites(bufid, -1, ETHERPUP_IPTYPE) != 0)
                    St_write("uipacket_RPC23", 8, 1, -1, 0x14);
            }
        }
        u.u_rval1 = 1;
        break;

    case UIBPDATA:
        {
            register struct bpdata *bpp;
            if (master && maxdlnactv) {
                copyin(a->arg, bpdata, sizeof(struct bpdata)*MAXDLNODES);
                for (bpp = bpdata; bpp < &bpdata[MAXDLNODES]; bpp++) {
                    if (bpp->bp_swapdev == NODEV) {
                        bpp->bp_nswap  = nswap;
                        bpp->bp_swplo = swplo;
                    }
                }
            } else
                u.u_error = EINVAL;
            return;
        }

    case UIOCMINO:
        if (!master)
            copyin(a->arg, minolist, NMINUMS*sizeof(ino_t));
        else
            u.u_error = EINVAL;
        break;

    case UIGETPORT:
        {
            struct upt_info upt;
            if (copyin(a->arg, &upt, sizeof(struct upt_info)) != 0) {
                u.u_error = EFAULT;
                return;
            }
            if (upt.acc_ipaddr == ipbcname)
                k = -1;
            else if ((k = uifindnode3(upt.acc_ipaddr)) < 0) {
                u.u_error = ENOENT;
                return;
            }
            if ((portp = get_port(u.u_procp, 1, upt.portno, upt.acc_port, k)) == 0) {
                if (upt.portno == 0)
                    u.u_error = ENOENT;
                break;
            }
            if (upt.flag == RTHERE)
                u.u_procp->p_flag |= SFSERV;
            upt.portno = portp->pt_myptno;
            copyout(&upt, a->arg, sizeof(struct upt_info));
            break;
        }

    case UICLOSEPORT:
        close_port(u.u_procp, 1);
        break;

    case UIGUARD:
    case UIKILL:
        {
            struct proc *procp;
        
            killport = fuword(&up->uu_altport);
            err = 0;
            if (find_port(killport) != 0) {
                if (a->cmd == UIKILL) {
                    sig = fuword(&up->uu_signal);
                    if ((procp = find_ptproc(killport)) != 0) 
                        psignal(procp, sig);
                    else
                        err = ESRCH;
                }
                return;
            }
            err = ESRCH;
            suword(&up->uu_error, err);
            break;
        }
        
    case UISTEN:
        St_enable();
        break;

    case UISTDIS:
        St_disable();
        break;

    case UISTCLR:
        St_clear();
        break;

    case UISTREAD:
        St_read(a->arg);
        break;

    case UIVERSION:
        if (fuword(&up->uu_argi[0]) < my_version)
            suword(&up->uu_error, EINVAL);
        suword(&up->uu_argi[1], my_version);
        return;

    case UINETTIME:
        {
            int n;
            ipnetaddr ipaddr;
            long *arg = (long*)a->arg;
            
            ipaddr = fulong(&arg[0]);
            if (!suser()) {
                u.u_error = EPERM;
                return;
            }
            if (ipaddr == utsname.ipname) {
                suword(&arg[1], 1);
                return;
            }
            
            if ((n = uifindnode3(ipaddr)) != -1) {
                if (WHAT_VERSION(n) >= OPTIMIZED_NOFR) {
                    spl7();
                    time += uinode[n].nd_timediff;
                    bootime += uinode[n].nd_timediff;
                    spl0();
                    suword(&arg[1], 1);
                    return;
                } else {
                    suword(&arg[1], -1);
                    return;
                }
            } else {
                u.u_error = ENODEV;
                return;
            }
        }
    case UIOMITAB:
    case UIGROUTE:
    default:
        u.u_error = EINVAL;
        break;
    }
}

changefd(infd, outfd)
int infd, outfd;
{
    u.u_ofile[0] = u.u_ofile[infd];
    u.u_ofile[infd] = u.u_ofile[outfd];
    u.u_ofile[outfd] = u.u_ofile[0];
    u.u_rofile[0] = u.u_rofile[infd];
    u.u_rofile[infd] = u.u_rofile[outfd];
    u.u_rofile[outfd] = u.u_rofile[0];
    u.u_pofile[0] = u.u_pofile[infd];
    u.u_pofile[infd] = u.u_pofile[outfd];
    u.u_pofile[outfd] = u.u_pofile[0];
}