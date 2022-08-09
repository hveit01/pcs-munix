/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.1  Nov 21 1986 /usr/sys/munet/mnbuf.c";

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

#include "sys/munet/munet.h"
#include "sys/munet/mnbuf.h"
#include "rbq.h"

/* FR_HEADER_SIZE = 60 = 0x3c 
   E_HEADER_SIZE = 14 = 0x0e
   IP_HEADER_SIZE = 20 = 0x14
   EIP_HEADER_SIZE = 34 = 0x22
   UIMAXDATA_NEW_FRAG = 4096 = 0x1000
   struct mnpacket = 4186 = 0x105a
   struct dlpacket = 4154 = 0x103a
   RT_HEADER_SIZE = 24 = 0x18
   MN_HEADER_SIZE = 54 = 0x36
*/


bufque   bufq[NLANDEV][MAXHOSTBUFNO];
int   (*bdone[NLANDEV][MAXHOSTBUFNO])();
ushort bufqid[NLANDEV][MAXHOSTBUFNO];
frque frq;

extern buf_it(), fr_done();


caddr_t buf_get(dev, sz, hdsz, bufid)
dev_t dev;
ushort *bufid;
{
    register bufque *bq;
    register bufque *found;
    register bufque *bq2 = 0;
    struct ldevsw *lanp;
    caddr_t  pkptr;
    long szreq;             /*requested size */
    long szgot;             /*received size of fragment */
    ushort id;              /*bufid*/

    szreq = sz;
    if (szreq <= 0) {
        u.u_error = EINVAL;
        return 0;
    }

    lanp = &ldevsw[major(dev)];
    
    while (szreq > 0) {
        szgot = lanp->lan_getbuf(szreq, hdsz, &id, &pkptr);
        bq = buf_pointer(id);
        bq->bq_flag = BQ_ALLOC|BQ_SEND;
        bq->bq_bufid = id;
        bq->bq_frsiz = min(szgot, szreq);
        bq->bq_pkptr = pkptr;
        if (bq2 == 0)
            found = bq;
        else
            bq2->bq_nextbuf = bq2->bq_nextque = bq;
        
        bufqid[bufmaj(id)][bufnum(id)] = found->bq_bufid;
        szreq -= szgot;
        if (szreq > 0)
            szreq += FR_HEADER_SIZE;
        bq2 = bq;
    }
    bq->bq_nextbuf = bq->bq_nextque = 0;
    *bufid = found->bq_bufid;
    return found->bq_pkptr;
}

buf_copy(id, pkpos, buf, bufsz, inkernel, dir)
ushort id;
caddr_t pkpos;
caddr_t buf;
short inkernel;
{
    register bufque *bq;
    register caddr_t bufp = buf; /* current write position in buf */
    register caddr_t rdp = 0;    /* current read pos in lan buffer */
    caddr_t pkptr;               /* pointer to lan buffer */
    uint free = bufsz;           /* free size in buffer */
    ushort nbytes;               /* size that is readable */
    ushort offset;               /* offset into lan_buffer */
    
    for (bq = buf_pointer(id); bq && free != 0; bq = bq->bq_nextbuf) {
        pkptr = bq->bq_pkptr;
        if (rdp == 0) {
            rdp = pkpos;
            offset = rdp - pkptr;
        } else {
            offset = FR_HEADER_SIZE;
            rdp = &pkptr[offset];
        }

        if (dir & 1) {
            nbytes = bq->bq_frsiz - (ushort)(rdp - pkptr);
            nbytes = min(nbytes, free);
            if (inkernel == 1) {
                /*if*/(bcopy(rdp, bufp, nbytes) == -1) /*does not fail*/
                    /*return -1 */;
            } else {
                if (copyout(rdp, bufp, nbytes) == -1)
                    return -1;
            }
        } else {
            nbytes = min( (int)(bq->bq_frsiz - (ushort)(rdp - pkptr)), free);
            bq->bq_frsiz = nbytes + (ushort)(rdp - pkptr);
            if (inkernel == 1) {
                /*if*/(bcopy(bufp, rdp, nbytes) == -1)  /*does not fail*/
                    /*return -1*/;
            } else {
                if (copyin(bufp, rdp, nbytes) == -1)
                    return -1;
            }
        }
        free -= nbytes;
        bufp += nbytes;
    }

    return bufsz - free;
}

caddr_t buf_siz(id, reqsz)
ushort id;

{
    register bufque *bq;
    register caddr_t pkptr = 0;
    register uint sz = reqsz;

    for (bq = buf_pointer(id); bq; bq = bq->bq_nextque) {
        bq->bq_frsiz = (*ldevsw[bufmaj(bq->bq_bufid)].lan_sizbuf)
                            (bq->bq_bufid, sz, &bq->bq_pkptr);
        if (pkptr==0)
            pkptr = bq->bq_pkptr;
        if (sz > bq->bq_frsiz) {
            bq->bq_nextbuf = bq->bq_nextque;
            sz -= bq->bq_frsiz;
            sz += FR_HEADER_SIZE;
        } else {
            bq->bq_nextbuf = 0;
            return pkptr;
        }
    }
    return 0;
}

buf_rel(id)
ushort id; /*buffer id*/
{
    register bufque *bq;
    register s = spltimer();

    bq = buf_pointer(bufqid[bufmaj(id)][bufnum(id)]);
    buf_relque(bq);

    splx(s);
}

buf_it(q)
bufque *q;
{
    register bufque *bq;
    register bufque *bq2;
    int s = spltimer();

    for (bq2 =0, bq = frq.fq_first; bq; bq2=bq, bq=bq->bq_nextque) {
        if (bq == q) {
            if (bq2)
                bq2->bq_nextque = bq->bq_nextque;
            else
                frq.fq_first = bq->bq_nextque;
            
            bq->bq_nextque = 0;
            frq.fq_count--;
            buf_relque(bq);

            splx(s);
            return 0;
        }
    }

    splx(s);
    return -1;
}

buf_relque(bq)
register bufque *bq;
{
    register bufque *bq2;
    register struct ldevsw *lanp = &ldevsw[bufmaj(bq->bq_bufid)];

    while (bq) {
        (*lanp->lan_relbuf)(bq->bq_bufid);
        bq->bq_flag = 0;
        bq2 = bq;
        bq = bq->bq_nextbuf;
        bq2->bq_nextbuf = 0;
    }
}

bufque *buf_pointer(id)
register ushort id; /*buffer id */
{
    if (bufmaj(id) >= NLANDEV || bufnum(id) >= MAXHOSTBUFNO) {
        u.u_error = EINVAL;
        return 0;
    }
    return &bufq[bufmaj(id)][bufnum(id)];
}

int fr_trans(id, ipaddr, type, donefunc)
ushort id;
ipnetaddr ipaddr;
ushort type;
int (*donefunc)();
{
    register bufque *bq;
    register bufque *bq2;
    register caddr_t pkptr;
    struct ldevsw *lanp;
    ushort fragflg;
    char first = 1;
    
    bq = buf_pointer(id);
    lanp = &ldevsw[bufmaj(id)];

    if (type == 0x806) {
        bufqid[bufmaj(id)][bufnum(id)] = id;
        (*lanp->lan_transmit)(bq->bq_bufid, donefunc);
        return bq->bq_bufid;
    } else {
        for (;;) {
            bq2 = bq->bq_nextbuf;
            if (bq2 == 0)
                fragflg = IP_FLAST;
            else
                fragflg = IP_FMORE;
            
            bufqid[bufmaj(bq->bq_bufid)][bufnum(bq->bq_bufid)] = id;
            bdone [bufmaj(bq->bq_bufid)][bufnum(bq->bq_bufid)] = donefunc;

            pkptr = bq->bq_pkptr;
            if (first) {
                first = 0;
                ip_fill(pkptr, ipaddr, (ushort)(bq->bq_frsiz-E_HEADER_SIZE), fragflg);
            } else
                ip_fmore(pkptr, (ushort)(bq->bq_frsiz-E_HEADER_SIZE), fragflg);
            (*lanp->lan_transmit)(bq->bq_bufid, fr_done);
            if (bq2)
                bcopy(pkptr, bq2->bq_pkptr, FR_HEADER_SIZE);
            else
                return bq->bq_bufid;
            
            bq = bq2;
        }
    }
}


fr_reas(rq)
struct rbq **rq;
{
    register struct mnpacket *pkt;
    register struct mnpacket *fragpkt;
    register bufque *curbq;
    struct rbq *rbqp = *rq;
    ushort curid;   /*bufid*/
    ushort curoff;  /*fragment offset*/
    ushort curflg;  /*fragment flags*/
    ushort fragoff; /*fragment offset*/
    ushort fragflg; /*fragment flags*/
    short ocnt;
    short fcnt;
    short off;
    short fragcnt = 0;
    bufque *curfrag;
    bufque *nxfrag; 
    bufque *frag;
    bufque *fraghead;
    int s;
    int unused;

    curid = rbqp->r_bufid;
    pkt = (struct mnpacket*)rbqp->r_pkptr;
    curbq = buf_pointer(curid);
    curbq->bq_rbq = rbqp;
    
    curoff = pkt->ip_word4 & IP_OMASK;
    curflg = pkt->ip_word4 & IP_FMASK;
    curbq->bq_flag = BQ_ALLOC|BQ_RCVE;
    curbq->bq_bufid = curid;
    curbq->bq_frsiz = pkt->ip_len + E_HEADER_SIZE;
    curbq->bq_pkptr = (caddr_t)pkt;
    curbq->bq_nextbuf = curbq->bq_nextque = 0;
    
    bufqid[bufmaj(curid)][bufnum(curid)] = curid;
    if (pkt->e_type != ETHERPUP_IPTYPE)
        return 0;
    
    if ((curoff==0 && curflg==0) || pkt->ip_word4 == IP_FFLAG)
            return 0;

    s = spltimer();
    
    for (fraghead = nxfrag = frq.fq_first; nxfrag != 0; 
           fraghead = nxfrag, nxfrag = nxfrag->bq_nextque) {

        curfrag = nxfrag;
        if (fr_cmp(curfrag, curbq) == 0) {
            ocnt = fcnt = off = 0;
            while (curfrag != 0) {
                fragpkt = (struct mnpacket*)(curfrag->bq_pkptr);
                fragoff = fragpkt->ip_word4 & IP_OMASK;
                fragflg = fragpkt->ip_word4 & IP_FMASK;
                if (curoff == fragoff && fragcnt==0) {
                    splx(s);
                    return -1;
                }
                if (curoff < fragoff && fragcnt==0) {
                    if (curfrag == nxfrag) {
                        curbq->bq_nextbuf = nxfrag;
                        curbq->bq_nextque = nxfrag->bq_nextque;
                        if (fraghead == nxfrag) {
                            frq.fq_first = curbq;
                            fraghead = curbq;
                        } else
                            fraghead->bq_nextque = curbq;
                        nxfrag->bq_nextque = 0;
                        cancelto(buf_it, nxfrag);
                        timeout(buf_it, curbq, FRTIME);
                        nxfrag = curbq;
                    } else {
                        curbq->bq_nextbuf = frag->bq_nextbuf;
                        frag->bq_nextbuf = curbq;
                    }
                    fragcnt++;
                    curfrag = curbq;
                    fragoff = curoff;
                    fragflg = curflg;
                }
                if (off < fragoff)
                    ocnt++;
                if (fragflg == 0)
                    fcnt++;
                off = fragoff+1;
                frag = curfrag;
                curfrag = curfrag->bq_nextbuf;
            }
            if (fragcnt == 0) {
                if (curoff > off)
                    ocnt++;
                frag->bq_nextbuf = curbq;
                if (curflg==0)
                    fcnt++;
                fragcnt++;
            }
            bufqid[bufmaj(curid)][bufnum(curid)] = nxfrag->bq_bufid;
            if (ocnt==0 && fcnt != 0) {
                frq.fq_count--;
                if (fraghead == nxfrag)
                    frq.fq_first = nxfrag->bq_nextque;
                else
                    fraghead->bq_nextque = nxfrag->bq_nextque;
            
                nxfrag->bq_nextque = 0;
                cancelto(buf_it, nxfrag);
                *rq = nxfrag->bq_rbq;
                splx(s);
                return 0;
            }
            splx(s);
            return IP_FMORE;
        }
    }
    
    if (frq.fq_count >= (FR_MAX-1)) {
        splx(s);
        return -1;
    }
    frq.fq_count++;
    curbq->bq_nextque = frq.fq_first;
    frq.fq_first = curbq;
    timeout(buf_it, curbq, FRTIME);
    splx(s);
    return IP_FMORE;
}

fr_cmp(q1, q2)
bufque *q1, *q2;
{
    /* assume it is an IP packet */
    register struct mnpacket *pp1 = (struct mnpacket*)q1->bq_pkptr;
    register struct mnpacket *pp2 = (struct mnpacket*)q2->bq_pkptr;

    return !(pp1->ip_srcaddr == pp2->ip_srcaddr && pp1->ip_id == pp2->ip_id);
}

fr_done(id, arg)
register short id;
short arg;
{
    register bufque *bq;
    register short qid;
    int (*donefunc)();
    
    bq = buf_pointer(id);
    bq->bq_flag |= BQ_SENDOK;
    
    qid = bufqid[bufmaj(id)][bufnum(id)];
    
    for (bq = buf_pointer(qid); bq; bq = bq->bq_nextbuf) {
        if ((bq->bq_flag & BQ_SENDOK)==0)
            return 0;
        qid = bq->bq_bufid;
    }

    donefunc = bdone[bufmaj(qid)][bufnum(qid)];
    if (donefunc == 0)
        buf_rel(qid);
    else
        (*donefunc)(qid, arg);
    return 0;
}
