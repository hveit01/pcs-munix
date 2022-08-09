/* PCS specific */
static char* _Version = "@(#) RELEASE:  2.1  Feb 05 1987 /usr/sys/io/icc_lance.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/buf.h"
#include "sys/ino.h"
#include "sys/inode.h"
#include "sys/mount.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/seg.h"
#include "sys/var.h"
#include "sys/utsname.h"

#include "rbq.h"
#include "sys/port.h"
#include "sys/ether.h"
#include "sys/munet/munet.h"
#include "sys/munet/diskless.h"
#include "sys/munet/mnbuf.h"
#include "sys/munet/mnnode.h"
#include "sys/munet/uarp.h"
#include "sys/icc/unix_icc.h"
#include "sys/icc/icc_lance.h"
#include "sys/icc/lance.h"

#define ICCNO   0           /* hardcoded only ICC 0 */
#define INITMUNIX   0x5555  /* init munix recv function */
                            /* if BBPTYPE use nc_etc_recv */

unsigned char transretrycount[NUM_TRANS_BUFS];
struct icc_lance_request transmitmsg[NUM_TRANS_BUFS];

/* allows to trace ethernet headers */
struct monbuf {
    char data[UIMINPACKET];
}; /* first 60 bytes of a data packet */

int         u_monptr = 0;
int         u_moncnt = 0;
struct monbuf *u_monbuf = 0;

uint            u_namptr = 0;
int         u_namcnt = 0;
struct utsname *u_nambuf = 0;

static int  initflg = 0;

dev_t       majordev = 0;

int         unknownip = 0;

int         icctranstest = 0;

static struct icc_lance_request freemsg[HOSTBUFNO];
static int seq;

struct rbq icc_rbq[HOSTBUFNO];
int icc_free_recbufs;

int xname_bufsiz;
uint xname_count;
struct utsname *xname_buf;

short htransmitbufs;
short htfreecnt;
short htfreemask;
short htsleep;

int (*munet_irdone)();
int (*nc_eth_rdone)();

extern short icc_lance_hbno;
#if 0
extern struct uinode uinode[];
extern enetaddr noname;
#endif

extern caddr_t logtophys();

free_recbuf(hbno)
short hbno;
{
    if (hbno >= (ushort)icc_lance_hbno || hbno < (ushort)htransmitbufs) {
        printf("free_recbuf: invalid buffer %d\n", hbno);
        return;
    }

    freemsg[hbno].rtkhdr.link = 0;
    Send_icc(0, &freemsg[hbno]);
    icc_free_recbufs++;
}

/* return 1 if ok
 * 0 if not enought tranmit buffers
 * -1 if failed
 */
icc_lance_init(rcvfunc)
int (*rcvfunc)();
{
    register i;

    if (rcvfunc == 0) {
        printf("icc_lance_init: invalid receive routine address\n");
        return -1;
    }

    for (i=0; i < nlandev; i++) {
        if (ldevsw[i].lan_init == icc_lance_init)
            break;
    }
    if (i >= nlandev) {
        printf("icc_lance_init: can't find self in ldevsw table\n");
        return -1;
    }

    majordev = makedev(i, 0);
    if (icc_lance_hwinit(INITMUNIX, rcvfunc) != 0)
        return -1;
    
    if (htransmitbufs > 10 && (icc_lance_hbno+1) >= (htransmitbufs * 3))
        return 1;
    return 0;
}

/*
 * code=INITMUNIX will initialize for MUNIX, BBPTYPE for ethbp sockets
 *
 * return -1 if no icc or not yet initialized
 * return 0 if okay
 * return 1 if invalid code
 * return ENOSPC if # of transmit buffers not okay
 */
icc_lance_hwinit(code, rcvfunc)
short code;
int (*rcvfunc)();
{
    short i;

    if (icc_there(ICCNO) == 0)
        return -1;

    if (icc_initflg[ICCNO] == 0)    /* note, only one ICC supported! */
        return -1;

    initflg++;
    if (code == INITMUNIX)
        munet_irdone = rcvfunc;
    else if (code == BBPTYPE)
        nc_eth_rdone = rcvfunc;
    else
        return 1;
    
    if (munet_irdone && initflg != 1)
        return 0;
    
    if (icc_lance_hbno <= NUM_TRANS_BUFS) {
        printf("icc_lance_hwinit: invalid number of transmit buffers\n");
        return ENOSPC;
    }
    
    htfreecnt = htransmitbufs = NUM_TRANS_BUFS;
    htsleep = 0;
    htfreemask = 0;

    for (i=0; i < htransmitbufs; ) {
        htfreemask |= (1 << i++);
    }

    if (munet_irdone)
        ldevsw[majordev].lan_bufid = -1;

    icc_rq_init();

    freemsg[0].rtkhdr.link = 0;
    freemsg[0].rtkhdr.id[0] = LANCE;
    freemsg[0].cmdtype = INIT_LANCE;
    freemsg[0].lance_un.init.etheraddress = utsname.uiname;
    Send_icc(ICCNO, &freemsg[0]); /* hardcoded ICC0 */
    Wait_for_reply(&freemsg[0], WAIT);
    utsname.uiname = freemsg[0].lance_un.init.etheraddress;

    for (i = htransmitbufs; i < icc_lance_hbno; i++) {
        freemsg[i].rtkhdr.link = 0;
        freemsg[i].rtkhdr.id[0] = LANCE;
        freemsg[i].cmdtype = READBUFREADY;
        freemsg[i].lance_un.rec.rbufphysadr = logtophys(hostbuf[i]);
        freemsg[i].lance_un.rec.bufno = i;
        Send_icc(ICCNO, &freemsg[i]); /* hardcoded ICC0 */

        transmitmsg[i].rtkhdr.id[0] = LANCE;
        transmitmsg[i].cmdtype = TRANSMIT;
        transmitmsg[i].lance_un.trans.tbufphysadr = logtophys(hostbuf[i]);
        transmitmsg[i].lance_un.trans.tbufno = i;
        transmitmsg[i].lance_un.trans.tmsg = &transmitmsg[i];
    }

    icc_free_recbufs = icc_lance_hbno - htransmitbufs;
    return 0;
}

/* ICC number is ignored */
icc_lance_there(iccno)
{
    if (icc_there(0) == 0)  /* hardcoded ICC0 */
        return -1;
    else
        return 0;
}

/* ICC number is ignored */
icc_lance_hwclose(iccno)
{   
    if (munet_irdone == 0) {
        freemsg[0].rtkhdr.link = 0;
        freemsg[0].rtkhdr.id[0] = LANCE;
        freemsg[0].cmdtype = CLOSE;
        Send_icc(ICCNO, &freemsg[0]);   /* hardcoded ICC0 */
        Wait_for_reply(&freemsg[0], WAIT);
    }
}

/* 
 * sz = requested size
 * prio = waiting priority,
 * bno = buffer number returned
 * bp = buffer address returned
 */
icc_lance_getbuf(sz, prio, bno, bp)
int sz;
short *bno;
caddr_t *bp;
{
    
    register struct icc_lance_request *rp;
    register caddr_t unused;
    register short bufno;
    register short bit;
    register short s;
    short needed;   /* buffers needed */

    s = splnet();

    if (sz > (uint)8312)
        needed = 3;
    else if (sz > (uint)4186)
        needed = 2;
    else
        needed = 1;

    /* wait for buffer */
    while (htfreemask == 0 || needed > htfreecnt) {
        St_write("lance_getbuf_BUF3", 10, 0x1b);
        htsleep++;
        sleep(&htfreemask, prio);
    }

    htfreecnt--;
    
    for (bufno = 0, bit = 1; (htfreemask & bit) == 0; bufno++, bit <<= 1)
        ;
    htfreemask &= ~bit;
    splx(s);

    if (sz < UIMINPACKET)
        sz = UIMINPACKET;
    else if (sz > UIMAXPACKET)
        sz = UIMAXPACKET;
    else
        sz += (sz&1); /*make even */

    rp = &transmitmsg[bufno];
    rp->rtkhdr.link = 0;
    rp->rtkhdr.id[0] = LANCE;
    rp->cmdtype = TRANSMIT;
    rp->lance_un.trans.tmsg = rp;
    rp->lance_un.trans.tbufphysadr = logtophys(hostbuf[bufno]);
    rp->lance_un.trans.tbufno = bufno;
    rp->lance_un.trans.tsize = sz;
    rp->lance_un.trans.tseq = seq++;
    rp->lance_un.trans.xdone = 0;

    *bno = majordev | bufno;
    *bp = hostbuf[bufno];
    return sz;
}

/* return 28 if invalid bufno bno, else return size
 */
icc_lance_sizbuf(bno, sz, bp)
ushort bno;
register sz;
caddr_t *bp;
{
    register short bufid = bufnum(bno);

    if (bufid < 0 || bufid >= htransmitbufs) {
        printf("icc_lance_sizbuf: invalid buffer id = %x\n", bno);
        return EINVAL;
    }

    if (sz < UIMINPACKET)
        sz = UIMINPACKET;
    else if (sz > UIMAXPACKET)
        sz = UIMAXPACKET;
    else
        sz += (sz&1);

    transmitmsg[bufid].lance_un.trans.tsize = sz;
    *bp = hostbuf[bufid];
    return sz;
}

icc_lance_relbuf(bno)
register ushort bno;
{
    register short s;
    
    bno &= 0xff;
    
    if (bno < 0 || bno >= icc_lance_hbno) {
        printf("icc_lance_relbuf: invalid buffer id = %x\n", bno);
        return EINVAL;
    }
    
    s = splnet();
    
    if (bno < htransmitbufs) {
        if (htsleep) {
            htsleep = 0;
            wakeup(&htfreemask);
        }
        htfreecnt++;
        htfreemask |= (1 << bno);
    } else
        free_recbuf(bno);
    
    splx(s);
    return 0;
}

icc_lance_transmit(bno, donefunc)
ushort bno;
int (*donefunc)();
{
    register struct mnpacket *mnp;
    register bufno = bno;
    bufno = bufno & 0xff;
    
    if (bufno < 0 || bufno >= htransmitbufs) {
        printf("icc_lance_transmit: invalid buffer id = %x\n", bno);
        return EINVAL;
    }
    
    transmitmsg[bufno].rtkhdr.flag = 0;
    transmitmsg[bufno].lance_un.trans.xdone = donefunc;
    if (transmitmsg[bufno].lance_un.trans.tsize > UIMAXPACKET)
        printf("transmit: size to big\n");
    mnp = (struct mnpacket*)hostbuf[bufno];
    mnp->e_source = utsname.uiname;
    if (icctranstest) {
        printf("icc_trans:bid=%d id=%d word4=%x cllno=%d\n",
            bufno, mnp->ip_id, mnp->ip_word4, mnp->uu.uu_callno);
    }
    St_write("lance_transmit_ETH13", ARPA_PROT, 1, mnp->e_type, 15, 
        transmitmsg[bufno].lance_un.trans.tsize,
        mnp->e_dest);
    if (mnp->e_type == ETHERPUP_IPTYPE) {
        St_write("lance_transmit_ETH14", ARPA_PROT, 1, mnp->ip_word5,
            -1, -1, -1);
    }

    transretrycount[bufno] = 0;
    icc_lance_tmon(mnp, transmitmsg[bufno].lance_un.trans.tseq);
    Send_icc(ICCNO, &transmitmsg[bufno]);
    return 0;
}

icc_lance_wakenam(arg)
{
    u_namcnt = -1;
    wakeup(arg);
}

/* nbuf = address of external receive buffer
 * nbufsiz = # of entries buffer needs to be 48*nbufsiz
 */
icc_lance_xname(nbuf, nbufsiz)
struct utsname *nbuf;
uint nbufsiz;
{
    /* retrieve collected xname data */
    if (u_nambuf == nbuf) {
        if (nbufsiz) {
            timeout(icc_lance_wakenam, &u_nambuf, (int)nbufsiz * hz);
            sleep(&u_nambuf, 0x11a);
            if (u_namcnt != -1)
                cancelto(icc_lance_wakenam, &u_nambuf);
        }
        
        copyout(xname_buf, u_nambuf, xname_count * sizeof(struct utsname));
        sptfree(xname_buf, xname_bufsiz, 0);
        
        xname_buf = 0;
        xname_bufsiz = 0;
        u_nambuf = 0;
        u_namcnt = 0;
        return 0;
    }
    
    /* allocate an xname buffer, unless we have one */
    if (u_nambuf == 0) {
        xname_bufsiz = btoc(nbufsiz * sizeof(struct utsname));
        if ((xname_buf = (struct utsname*)sptalloc(xname_bufsiz)) == 0)
            return ENOMEM;
        xname_count = 0;
        u_nambuf = nbuf;
        u_namcnt = nbufsiz;
        u_namptr = 0;
        return 0;
    } else
        return EBUSY;
}

icc_lance_monitor(mbuf, mcnt)
struct monbuf *mbuf;
{
    if (u_monbuf)
        return EBUSY;

    u_monbuf = mbuf;
    u_moncnt = mcnt;
    u_monptr = 0;
    
    sleep(&u_monbuf, 0x11a);

    u_moncnt = 0;
    u_monbuf = 0;
    return 0;
}

lance_intr(dev, rp)
struct icc_lance_request *rp;
{
    if (dev != ICCNO)       /* only one ICC acceptable */
        return;

    if (rp->cmdtype == TRANSMIT) {
        icc_lance_tint(rp);
        return;
    }
    if (rp->cmdtype == RECEIVE_LANCE) {
        icc_lance_rint(rp);
        return;
    }

    if (rp->cmdtype == PRINT_LANCE)
        prmsg(rp);
    else
        printf("lance_intr (host): cryptic message received \n");
}

icc_lance_rint(rp)
struct icc_lance_request *rp;
{
    register struct mnpacket *mnp;
    register bufno;
    register dummy1, dummy2;
    
    
    splnet();
    
    bufno = rp->lance_un.reply.rbno;
    mnp = (struct mnpacket*)icc_rbq[bufno].r_pkptr;
    if (bufno >= (uint)icc_lance_hbno) {
        printf("icc_lance_rint: rbno %u wrong ! \n", bufno);
        return;
    }
    icc_free_recbufs--;
    
    St_write("lance_rint_ETH3", ARPA_PROT, 2, mnp->e_type, 15, 
        rp->lance_un.reply.rlength - 4, mnp->e_dest);
    if (rp->lance_un.reply.err) {

        printf("icc_lance_rint: FCS error in received packet -- ");
        printf("%4x %4x %4x %4x %4x %4x %4x\n",
            mnp->e_dest.hi, mnp->e_dest.mi, mnp->e_dest.lo, 
            mnp->e_source.hi, mnp->e_source.mi, mnp->e_source.lo, 
            mnp->e_type);
        St_write("lance_rint_ETH2", ARPA_PROT, 2, -1, 0x10, -1, -1);
        free_recbuf(bufno);
        return;
    }
    if (u_moncnt > 0) {
        register caddr_t mp;
        u_moncnt--;
        mp = u_monbuf[u_monptr++].data;
        bcopy(mnp, mp, UIMINPACKET);
        switch (mnp->uu.uu_type) {
        case UUTYPE:
            sslong(&mp[0x24], lbolt);
            ssword(&mp[0x32], -1);
            break;
        case RTTYPE:
            sslong(&mp[0x34], lbolt);
            break;
        }
        if (u_moncnt == 0)
            wakeup(&u_monbuf);
    }
    
    if (mnp->e_type != ETHERPUP_IPTYPE && mnp->e_type != ETHERPUP_ARPTYPE) {
        if (nc_eth_rdone) {
            if ( (*nc_eth_rdone)(&icc_rbq[bufno], rp->lance_un.reply.rlength, 
                    icc_free_recbufs >= 4))
                free_recbuf(bufno);
            return;
        }
        free_recbuf(bufno);
        return;
    }
    
    /* handle ARP request/REPLY */
    if (mnp->e_type == ETHERPUP_ARPTYPE) {
        if (uarp_recv(&mnp->ip_word1) == -1) { /* pass only IP HEADER */
            /* no need to do anything: ARP not for me */
            free_recbuf(bufno);
            return;
        }
        /* build ARP reply */
        mnp->e_dest = mnp->e_source;
        mnp->e_source = utsname.uiname;
        transmitmsg[bufno].rtkhdr.link = 0;
        transmitmsg[bufno].lance_un.trans.xdone = 0;
        transmitmsg[bufno].lance_un.trans.tsize = UIMAXDATA_OLD;
        transmitmsg[bufno].lance_un.trans.tseq = -3;
        icc_lance_tmon(mnp, -3);
        St_write("lance_transmit_ETH13", ARPA_PROT, 1, mnp->e_type, 15,
            transmitmsg[bufno].lance_un.trans.tsize, mnp->e_dest);
        Send_icc(ICCNO, &transmitmsg[bufno]);
        return;
    }
    
    St_write("lance_rint_ETH4", ARPA_PROT, 2, mnp->ip_word5, -1, -1, -1);
    if (mnp->e_type != ETHERPUP_IPTYPE ||
            (mnp->ip_destaddr != utsname.ipname && mnp->ip_destaddr != -1)) {
        St_write("lance_rint_ETH5", ARPA_PROT, 2, -1, 0x19, -1, -1);
        free_recbuf(bufno);
        return;
    }

    if ((mnp->ip_word5 & 0xff) == 1) {
        if (uicmp_input(mnp) == 1) {
            mnp->e_dest = mnp->e_source;
            mnp->e_source = utsname.uiname;
            transmitmsg[bufno].rtkhdr.link = 0;
            transmitmsg[bufno].lance_un.trans.xdone = 0;
            transmitmsg[bufno].lance_un.trans.tsize = UIMAXDATA_OLD;
            transmitmsg[bufno].lance_un.trans.tseq = -3;
            St_write("lance_transmit_ETH13", ARPA_PROT, 1, mnp->e_type,
                0x0f, transmitmsg[bufno].lance_un.trans.tsize,
                mnp->e_dest);
            St_write("lance_transmit_ETH14", ARPA_PROT, 1, mnp->ip_word5,
                -1, -1, -1);
            icc_lance_tmon(mnp, -3);
            Send_icc(ICCNO, &transmitmsg[bufno]);
            return;
        }
        free_recbuf(bufno);
        return;
    }

    if ((mnp->ip_word5 & 0xff) != 0x44) {
        unknownip++;
        St_write("lance_rint_ETH6", ARPA_PROT, 2, -1, 0x19, -1, -1);
        free_recbuf(bufno);
        return;
    }
    
    if (mnp->uu.uu_type == UNTYPE) {
        St_write("lance_rint_RPC19", ARPA_SOCKET, 2, mnp->uu.uu_type, 0x0f);
        if (UISAME(&mnp->e_dest,&bcname)) {
            mnp->e_dest = mnp->e_source;
            mnp->e_source = utsname.uiname;
            bcopy(&utsname, mnp->uu.uu_data, sizeof(struct utsname));
            transmitmsg[bufno].rtkhdr.link = 0;
            transmitmsg[bufno].lance_un.trans.xdone = 0;
            transmitmsg[bufno].lance_un.trans.tsize = UIMAXDATA_OLD;
            transmitmsg[bufno].lance_un.trans.tseq = -2;
            icc_lance_tmon(mnp, -2);
            St_write("lance_rint_RPC20", ARPA_SOCKET, 1, mnp->uu.uu_type, 0x0f);
            St_write("lance_transmit_ETH13", ARPA_PROT, 1, ETHERPUP_IPTYPE,
                0x0f, transmitmsg[bufno].lance_un.trans.tsize,
                mnp->e_dest);
            St_write("lance_transmit_ETH14", ARPA_PROT, 1, mnp->ip_word5,
                -1, -1, -1);
            Send_icc(ICCNO, &transmitmsg[bufno]);
            return;
        }
        
        if (u_namcnt > 0) {
            u_namcnt--;
            xname_count++;
            bcopy(mnp->uu.uu_data, &xname_buf[u_namptr], sizeof(struct utsname));
            u_namptr++;
            if (u_namcnt == 0)
                wakeup(&u_nambuf);
        }
        free_recbuf(bufno);
        return;
    }
    
    if (munet_irdone) {
        if ((*munet_irdone)(&icc_rbq[bufno], rp->lance_un.reply.rlength, 
                icc_free_recbufs >= 4))
            free_recbuf(bufno);
        return;
    }
    free_recbuf(bufno);
}

icc_lance_tmon(mnp, code)
register struct mnpacket *mnp;
{
    register caddr_t mp;
    register short s = splnet();
    
    if (u_moncnt > 0) {
        u_moncnt--;
        mp = u_monbuf[u_monptr++].data;
        bcopy(mnp, mp, UIMINPACKET);

        switch (mnp->uu.uu_type) {
        case UUTYPE:
            sslong(&mp[0x24], lbolt);
            ssword(&mp[0x32], code);
            break;
        case RTTYPE:
            sslong(&mp[0x34], lbolt);
            break;
        }
        if (u_moncnt == 0)
            wakeup(&u_monbuf);
    }
    splx(s);
}

icc_lance_tint(rp)
struct icc_lance_request *rp;
{
    register struct icc_lance_request *tp;
    register ushort tbufno;

    if (rp->lance_un.trans.terr) {
        icc_lance_jint(rp);
        return;
    }

    splnet(); /* no splx()? */
    
    tp = rp->lance_un.trans.tmsg;
    tbufno = tp->lance_un.trans.tbufno;
    if (tp->lance_un.trans.xdone) {
        if ((*tp->lance_un.trans.xdone)(majordev | tbufno,
                tp->lance_un.trans.tsize))
            icc_lance_relbuf(tbufno);
        return;
    }
    
    icc_lance_relbuf(tbufno);
}

icc_lance_jint(rp)
struct icc_lance_request *rp;
{
    register struct icc_lance_request *tp;
    register ushort tbufno;
    register short terr;
    
    tp = rp->lance_un.trans.tmsg;
    tbufno = tp->lance_un.trans.tbufno;
    terr = rp->lance_un.trans.terr;

    if ((terr & LCOL) || (terr & LCAR)) { /* late collision || loss of carrier */
        if (transretrycount[tbufno]++ < 3) {
            St_write("lance_jint_ETH11", ARPA_PROT, 1, -1, 0x13, -1, -1);
            tp->rtkhdr.link = 0;
            Send_icc(ICCNO, tp);
            return;
        }
    }

    splnet();
    if (tp->lance_un.trans.xdone) {
        if ((*tp->lance_un.trans.xdone)(majordev | tbufno, -1))
            icc_lance_relbuf(tbufno);
        return;
    }
    
    icc_lance_relbuf(tbufno);
}

uarp_recv(ap)
register struct uarp_pkt *ap;
{
    register idx;
    
    if (ap->arp_op == ARP_OP_REQUEST && ap->arp_tpa == utsname.ipname) {
        if ((idx = uifindnode3(ap->arp_spa)) >= 0) {
            if (UISAME(&ap->arp_sha, &utsname.uiname))
                uinode[idx].nd_etaddr = noname;
            else {
                uinode[idx].nd_etaddr = ap->arp_sha;
                uinode[idx].nd_time = time;
            }
        }
        ap->arp_hrd = ARP_HRD_E;
        ap->arp_pro = ARP_PRO_IP;
        ap->arp_hln = ARP_LEN_E;
        ap->arp_pln = ARP_LEN_IP;
        ap->arp_op  = ARP_OP_REPLY;
        ap->arp_tha = ap->arp_sha;
        ap->arp_tpa = ap->arp_spa;
        ap->arp_sha = utsname.uiname;
        ap->arp_spa = utsname.ipname;
        return 0;
    } else if (ap->arp_op == ARP_OP_REPLY) {
        if ((idx = uifindnode3(ap->arp_spa)) >= 0) {
            if (UISAME(&ap->arp_sha, &utsname.uiname))
                uinode[idx].nd_etaddr = noname;
            else {
                uinode[idx].nd_etaddr = ap->arp_sha;
                uinode[idx].nd_time = time;
            }
            wakeup(&uinode[idx].nd_ipaddr);
        }
        return -1;
        
    } else
        return -1;
}

prmsg(rp)
struct icc_lance_request *rp; 
{
    static short retrycnt = 3;
    
    register i;
    
    for (i = 0; i < 31; i++) {
        if (rp->lance_un.bitmap & (1<<i)) {
            switch(i) {
            case 1:
                printf("lance: transmit message too long\n");
                break;
            case 2:
                if (retrycnt) {
                    retrycnt--;
                    printf("lance: Warning - Collision test failed\n");
                }
                break;
            case 3:
                printf("lance: memory error detected\n");
                break;
            case 4:
                printf("lance_server_proc: cryptic msg\n");
                break;
            case 5:
                printf("lance: data late, UFLO occurred\n");
                break;
            case 6:
                printf("lance: late collision detected\n");
                break;
            case 7:
                printf("lance: loss of carrier detected (cable ?)\n");
                break;
            case 8:
                printf("lance: OFLO receive condition occurred\n");
                break;
            case 9:
                printf("lance: BUFFR condition detected\n");
                break;
            case 10:
                printf("lance: ENP and STP not present\n");
                break;
            case 11:
                printf("lance: CRC error detected by receive packet\n");
                break;
            default:
                printf("lance: msg #%d\n", i);
                break;
            }

            switch(i) {
            case 8:
                St_write("prmsg_ETH7", ARPA_PROT, 2, -1, 0x11, -1, -1);
                St_write("prmsg_BUF2", ARPA_MGMT, 0x1a);
                break;
            case 11:
                St_write("prmsg_ETH8", ARPA_PROT, 2, -1, 0x10, -1, -1);
                break;
            case 3:
            case 9:
                St_write("prmsg_BUF1", ARPA_MGMT, 0x1a);
                /*FALLTHRU*/
            case 7:
                St_write("prmsg_ETH9", ARPA_PROT, 2, -1, 0x18, -1, -1);
                break;
            case 5:
            case 6:
                St_write("prmsg_ETH10", ARPA_PROT, 1, -1, 0x14, -1, -1);
                break;
            default:
                break;
            }
        }
    }
}

icc_rq_init()
{
    register i;

    for (i=0; i < icc_lance_hbno; i++) {
        icc_rbq[i].r_bufid = majordev | i;
        icc_rbq[i].r_pkptr = hostbuf[i];
        icc_rbq[i].r_next = 0;
    }
}
