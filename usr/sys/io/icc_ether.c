/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.5  Aug 03 1987 /usr/sys/io/icc_ether.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/sysinfo.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/utsname.h"

#include "rbq.h" /* this is missing from sys/* */
#include "sys/port.h"
#include "sys/ether.h"
#include "sys/ethpt.h"
#include "sys/icc/unix_icc.h"
#include "sys/icc/icc_lance.h"
#include "sys/munet/munet.h"
#include "sys/munet/mnbuf.h"
#include "fcntl.h"

struct port e_port[NPORT];
struct portctrl portctrl = { 0, 0, FREEMIN };

static struct port *txport[64];
static struct port *lastport;
static struct port *maxbbport;

extern icc_lance_hwinit(), icc_lance_hwclose(),
    icc_lance_there(), icc_lance_transmit(),
    icc_ethxint(), icc_ethrint();

extern struct rbq icc_rbq[];
extern dev_t majordev;
extern short stnaddr;

icc_ptctrl(cmd)
{
    switch (cmd) {
    case 0:
        if (portctrl.openct++ == 0) {
            if (icc_lance_hwinit(BBPTYPE, icc_ethrint) != 0) {
                portctrl.openct = 0;
                return 1;
            }
            portctrl.flag |= ROPEN;
            lastport =  &e_port[NPORT];
            maxbbport = &e_port[MAXPORTS];
        }
        break;

    case 1:
        if (--portctrl.openct <= 0) {
            portctrl.flag &= ~ROPEN;
            icc_lance_hwclose();
        }
    }
    return 0;
}

icc_ethopen(dev, mode)
dev_t dev;
{
    register struct port *ep;

    if (icc_lance_there() == -1 || dev >= NPORT) {
        u.u_error = ENXIO;
        return;
    }

    ep = &e_port[dev];
    if ((ep->pt_flag & ROPEN) || icc_ptctrl(0) != 0) {
        u.u_error = EACCES;
        return;
    }
    
    if (ISBBP(dev)) {
        ep->pt_flag = ((mode & O_RDWR) ? (RWRITE|RREAD) : RREAD) | ROPEN;
        ep->pt_mrc = 1;
    } else {
        ep->pt_flag = ((mode & O_RDWR) ? RWRITE : RREAD) | ROPEN;
        ep->pt_type = 0;
        ep->pt_mrc = 1;
    }
    ep->pt_rc = 0;
}

icc_ethread(dev)
dev_t dev;
{
    register struct port *ep;
    register caddr_t unused; /* relic from former code */
    register struct ethpacket *ethp;
    short datalen;

    if (u.u_count <= 0) {
        u.u_error = EINVAL;
        return;
    }

    ep = &e_port[dev];
    if ((ep->pt_flag & P_CONFIG)==0) {
        u.u_error = EACCES;
        return;
    }

    if (ISETHPT(dev) && (ep->pt_flag & RWRITE)) {
        u.u_error = EACCES;
        return;
    }

    spltimer();
      while (ep->pt_rc <= 0) {
          ep->pt_flag |= P_READSLEEP;
          sleep(ep, PPIPE);
      }
    spl0();

    ethp = (struct ethpacket*)ep->pt_rbufd->r_pkptr;

    if (ISBBP(dev)) {
        datalen = ethp->datalen;
        if (datalen <= u.u_count)
            iomove(&ethp->data, datalen, 1);
        else
            u.u_error = EIO;
    } else {
        if (ep->pt_flag & P_SWAP)
            swab(&ethp->sourceid, MAXETHDATA);
        datalen = ethp->type; /* type abused for datalen - pkt without header */
        if (datalen >= 0 && datalen <= u.u_count && datalen <= MAXETHDATA)
            iomove(&ethp->sourceid, datalen, 1);
        else 
            u.u_error = EIO;
    }
    
    spltimer();
    rq_relfirst(&ep->pt_rbufd);
    ep->pt_rc--;
    spl0();
}

icc_ethrint(qp, rawsz, wake)
struct rbq *qp;
{
    register struct ethpacket *ethp;
    register struct port *ep;
    register ushort destport;
    register ushort type;
    short id = qp->r_bufid;

    spldisk();
    
    /* defragment packet */
    switch (fr_reas(&qp)) {
    case -1:
        buf_rel(id);
        return 0;
    case IP_FMORE:
        return 0;
    case IP_FLAST:
        break;
    }

    id = qp->r_bufid;
    ethp = (struct ethpacket*)qp->r_pkptr;  /*ethpacket*/
    type = ethp->type;      /*type*/
    if (type==0 || type==BBPTYPE) {
        destport = ethp->destport;
        for (ep = &e_port[0]; ep < maxbbport; ep++) {
            if ((ep->pt_flag & P_CONFIG) && destport == ep->pt_inport)
                break;
        }
        if (ep < maxbbport && ep->pt_rc < ep->pt_mrc && 
                (ep->pt_flag & (ROPEN|RREAD)) == (ROPEN|RREAD) &&
                (ep->pt_accept == ethp->sourceid || ep->pt_accept == ANYONE) &&
                (wake || (ep->pt_flag & P_READSLEEP))) {
            rq_append(&ep->pt_rbufd, qp);
            ep->pt_rc++;
            ep->pt_sendadr = ethp->source;
            ep->pt_sender = ethp->sourceid;
            ep->pt_sendport = ethp->sourceport;
            if (ep->pt_flag & P_READSLEEP) {
                wakeup(ep);
                ep->pt_flag &= ~P_READSLEEP;
            }
        } else {
            buf_rel(id);
            return 0;
        }
    } else {
        for (ep = maxbbport; ep < lastport; ep++) {
            if ((ep->pt_flag & (ROPEN|RREAD|P_CONFIG)) == (ROPEN|RREAD|P_CONFIG) &&
              type == ep->pt_type)
                break;
        }
        if (ep < lastport && ep->pt_rc < ep->pt_mrc) {
            if (wake || (ep->pt_flag & P_READSLEEP)) {
                rq_append(&ep->pt_rbufd, qp);
                ep->pt_rc++;
                ep->pt_sendadr = ethp->source;
                ep->pt_ethadr = ethp->dest;
                ethp->type = rawsz - (ETH_HDR_SIZE+CRC_SIZE); /* type abused for datalen */
                if (ep->pt_flag & P_READSLEEP) {
                    wakeup(ep);
                    ep->pt_flag &= ~P_READSLEEP;
                }
                return 0;
            }
        }
        
        buf_rel(id);
        return 0;
    }
    return 0;
}

icc_ethwrite(dev)
dev_t dev;
{
    register struct port *ep;   /* current port to write */
    register struct rbq *rq;    /* buffer que entry for icc */
    struct ethpacket *ethp;     /* network buffer to be built */
    ushort bufid;               /* buf# of system */
    uint payload;               /* net size to transmit */
    int totalsz;                /* total size of network buffer */
    int bufno;                  /* buf index for icc */
    
    ep = &e_port[dev];
    
    if (ISBBP(dev))
        payload = u.u_count + BBP_HDR_SIZE;
    else
        payload = u.u_count;
    
    if (payload > MAXETHDATA) {
        u.u_error = EINVAL;
        return;
    }
    
    if ((ep->pt_flag & P_CONFIG)==0) {
        u.u_error = EACCES;
        return;
    }

    spltimer();
      totalsz = (payload < MINETHDATA ? MINETHDATA : payload) + ETH_HDR_SIZE;
      buf_get(majordev, totalsz, sizeof(struct ethpacket), &bufid);
      bufno = bufnum(bufid);
      rq = &icc_rbq[bufno];
      ethp = (struct ethpacket*)rq->r_pkptr;
    spl0();

    ethp->dest = ep->pt_ethadr;
    ethp->source = utsname.uiname;
    if (ISBBP(dev)) {
        ethp->type = BBPTYPE;
        ethp->sourceid = stnaddr;
        ethp->destid = ep->pt_station;
        ethp->sourceport = ep->pt_inport;
        ethp->destport = ep->pt_outport;
        ethp->datalen = payload - BBP_HDR_SIZE;
        iomove(&ethp->data, payload - BBP_HDR_SIZE, 0);
    } else {
        ethp->type = ep->pt_type;
        iomove(&ethp->sourceid, payload, 0);
        if (ep->pt_flag & P_SWAP)
            swab(&ethp->sourceid, EVEN(payload));
    }

    /* send to itself? */
    if (ETHAEQ(ep->pt_ethadr, utsname.uiname)) {
        register struct port *p;

        spltimer();
        if (ISBBP(dev)) {
            for (p = e_port; p < maxbbport; p++) {
                if ((p->pt_flag & P_CONFIG) && p->pt_inport == ep->pt_outport)
                    break;
            }
            if (p < maxbbport && (p->pt_flag & (ROPEN|RREAD)) == (ROPEN|RREAD) &&
                    p->pt_rc < p->pt_mrc &&
                    (p->pt_accept == stnaddr || p->pt_accept == ANYONE)) {
                rq_append(&p->pt_rbufd, rq);
                p->pt_rc++;
                p->pt_sendadr = utsname.uiname;
                p->pt_sender = stnaddr;
                p->pt_sendport = ep->pt_inport;
                if (p->pt_flag & P_READSLEEP) {
                    wakeup(p);
                    p->pt_flag &= ~P_READSLEEP;
                }
            } else
                buf_rel(bufid);
        } else {
            for (p = maxbbport; p < lastport; p++) {
                if ((p->pt_flag & (P_CONFIG|RREAD|ROPEN))==(P_CONFIG|RREAD|ROPEN) &&
                        p->pt_type == ep->pt_type)
                    break;
            }
            if (p < lastport && p->pt_rc < p->pt_mrc) {
                rq_append(&p->pt_rbufd, rq);
                p->pt_rc++;
                p->pt_sendadr = utsname.uiname;
                p->pt_ethadr = utsname.uiname;
                ethp->type = totalsz - ETH_HDR_SIZE; /* type abused for datalen */
                if (p->pt_flag & P_READSLEEP) {
                    wakeup(p);
                    p->pt_flag &= ~P_READSLEEP;
                }
            } else
                buf_rel(bufid);
        }
        spl0();
        return;
    }
    
    txport[bufno] = ep;
    ep->pt_flag &= ~(P_WTDONE|P_WTERR);
    icc_lance_transmit(bufno, icc_ethxint);

    spltimer();
      while ((ep->pt_flag & (P_WTDONE|P_WTERR)) == 0) {
          ep->pt_flag |= P_WRITESLEEP;
          sleep(&ep->pt_xrslt, PRIBIO-1);
      }
    spl0();

    if (ep->pt_flag & P_WTERR)
        u.u_error = EIO;
}

icc_ethxint(dev, count)
register ushort dev;
{
    register ushort mdev = minor(dev);
    register struct port *txp = txport[mdev];

    if (txp != 0) {
        if (count < 0) {
            txp->pt_xrslt = BB_ERROR;
            txp->pt_flag |= P_WTERR;
        } else {
            txp->pt_xrslt = BB_ACCEPTED;
            txp->pt_flag |= P_WTDONE;
        }
        
        if (txp->pt_flag & P_WRITESLEEP) {
            wakeup(&txp->pt_xrslt);
            txp->pt_flag &= ~P_WRITESLEEP;
        }
    }
    
    txport[mdev] = 0;
    buf_rel(dev);
    return 0;
}

icc_ethclose(dev)
dev_t dev;
{
    register struct port *ep = &e_port[dev];

    spltimer();
      rq_relall(&ep->pt_rbufd);
      ep->pt_mrc = ep->pt_rc = 0;
      ep->pt_flag = 0;
    spl0();
    
    icc_ptctrl(1);
}

icc_ethioctl(dev, cmd, arg, flag)
dev_t dev;
{
    if (ISBBP(dev))
        icc_bbpioctl(dev, cmd, arg, flag);
    else
        icc_ethptioctl(dev, cmd, arg, flag);
}

icc_bbpioctl(dev, cmd, arg, flag)
dev_t dev;
{
    int port;
    register struct port *ep = &e_port[dev];

    switch (cmd) {
    case BBPENQ:
        if ((ep->pt_flag & P_CONFIG)==0) {
            u.u_error = EACCES;
            return;
        }
        ep->pt_blkavail = (ep->pt_rc > 0);
        if (copyout(&ep->pt_enq, arg, sizeof(struct portenq)) != 0)
            u.u_error = EFAULT;
        break;

    case BBPGET:
        if ((ep->pt_flag & P_CONFIG)==0) {
            u.u_error = EACCES;
            return;
        }
        if (copyout(&ep->pt_info, arg, sizeof(struct portinfo)) != 0)
            u.u_error = EFAULT;
        break;

    case BBPSET:
        port = ep->pt_inport;
        spltimer();
        ep->pt_flag &= ~P_CONFIG;
        if (copyin(arg, &ep->pt_info, sizeof(struct portinfo)) != 0) {
            u.u_error = EFAULT;
            return;
        }
        
        if (ep->pt_inport != port && ep->pt_rc > 0) {
            ep->pt_rc = 0;
            rq_relall(&ep->pt_rbufd);
        }
        
        if (ep->pt_inport == DYNAMIC) {
            do {
                ep->pt_inport = portctrl.bb_free++;
                if (portctrl.bb_free > FREEMAX)
                    portctrl.bb_free = FREEMIN;
            } while (icc_bbpclash(ep->pt_inport));
        } else {
            if (icc_bbpclash(ep->pt_inport)) {
                u.u_error = EACCES;
                return;
            }
        }
        
        ep->pt_flag |= P_CONFIG;
        spl0();
        break;
    default:
        u.u_error = EINVAL;
        break;
    }
}

icc_bbpclash(port)
register port;
{
    register struct port *ep;
    
    for (ep = &e_port[0]; ep < maxbbport; ep++) {
        if ((ep->pt_flag & P_CONFIG) && ep->pt_inport == port)
            return 1;
    }
    return 0;
}

icc_ethptioctl(dev, cmd, arg, mode)
dev_t dev;
{
    register struct port *ep = &e_port[dev];
#define ETH(ep) ((struct ethptinfo*)&(ep->pt_info))

    struct ethptinfo einfo;

    switch (cmd) {
    case ETHPTSET:
        spltimer();
        ep->pt_flag &= ~P_CONFIG;
        if (copyin(arg, &einfo, sizeof(struct ethptinfo)) != 0) {
            u.u_error = EFAULT;
            return;
        }

        if (einfo.ei_proto == 0 || einfo.ei_proto == BBPTYPE ||
          einfo.ei_proto == 0x800 || einfo.ei_proto == 0x806) {
            u.u_error = EACCES;
            return;
        }
        
        if (ETH(ep)->ei_proto != einfo.ei_proto) {
            if (icc_ethptclash(einfo.ei_proto, OPENMODE(ep->pt_flag)) != 0) {
                u.u_error = EACCES;
                return;
            }
            ETH(ep)->ei_proto = einfo.ei_proto;
        }

        ep->pt_flag |= P_CONFIG;
        if (einfo.ei_swapflag == 1)
            ep->pt_flag |= P_SWAP;
    
        if (ep->pt_flag & RWRITE)
            ETH(ep)->ei_locadr = einfo.ei_remadr;
        spl0();
        break;
        
    case ETHPTGET:
        if ((ep->pt_flag & P_CONFIG) == 0) {
            u.u_error = EACCES;
            return;
        }

        einfo.ei_proto = ETH(ep)->ei_proto;
        if (ep->pt_flag & P_SWAP)
            einfo.ei_swapflag = 1;
        else
            einfo.ei_swapflag = 0;
        einfo.ei_rfuflag = 0;

        spltimer();
        if (ep->pt_flag & RWRITE) {
            einfo.ei_remadr = ETH(ep)->ei_locadr;
            einfo.ei_locadr = utsname.uiname;
        } else {
            einfo.ei_remadr = ep->pt_sendadr;
            einfo.ei_locadr = ETH(ep)->ei_locadr;
        }
        spl0();
        if (copyout(&einfo, arg, sizeof(struct ethptinfo)) != 0)
            u.u_error = EFAULT;
        break;

    case ETHPTSETBUF:
        if ((ep->pt_flag & P_CONFIG) == 0) {
            u.u_error = EACCES;
            return;
        }
        ep->pt_mrc = MAXETHPT_RDAHEAD;
        break;
        
    default:
        u.u_error = EINVAL;
        break;
    }
}

icc_ethptclash(proto, mode)
ushort proto;
{
    register struct port *ep;

    for (ep = maxbbport; ep < lastport; ep++) {
        if ((ep->pt_flag & P_CONFIG) && ETH(ep)->ei_proto == proto &&
          (ep->pt_flag & RREAD) == mode)
            return 1;
    }
    return 0;
}
