/* @(#)subr.c   6.3 */
static char *_Version = "@(#) RELEASE:  1.1  Aug 18 1986 /usr/sys/os/subr.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/page.h"
#include "sys/buf.h"
#include "sys/mount.h"
#include "sys/var.h"
#include "sys/file.h" /*pcs*/

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in u.u_rablock
 * for use in read-ahead.
 */
daddr_t
bmap(ip, readflg)
register struct inode *ip;
{
    register i;
    register dev;
    daddr_t bn;
    daddr_t nb, *bap;
    int raflag;
    register struct mount *mp;  /*pcs*/

    u.u_rablock = 0;
    u.u_xablock = 0;            /*pcs*/
    raflag = 0;
    {
        register sz, type, rem;

        type = ip->i_mode&IFMT;
        if (type == IFBLK) {
            dev = (dev_t)ip->i_rdev;
            for (mp = &mount[0]; mp < (struct mount*)v.ve_mount; mp++)  /*pcs*/
                if ((mp->m_flags==MINUSE) &&                            /*pcs*/
                    (brdev(mp->m_dev)==brdev(dev))) {                   /*pcs*/
                    dev = mp->m_dev;                                    /*pcs*/
                    break;
                }
        } else {
            mp = ip->i_mount;                                           /*pcs*/
            dev = ip->i_dev;
        }
        u.u_pbdev = dev;
        bn = FsBNO(dev, u.u_offset);
        if (bn < 0) {
            u.u_error = EFBIG;
            return((daddr_t)-1);
        }
        if ((ip->i_lastr + 1) == bn)
            raflag++;
        u.u_pboff = FsBOFF(dev, u.u_offset);
        sz = FsBSIZE(dev) - u.u_pboff;
        if (u.u_count < sz) {
            sz = u.u_count;
            raflag = 0;
        } else
            ip->i_lastr = bn;

        u.u_pbsize = sz;
        if (type == IFBLK) {
            if (raflag) {
                u.u_rablock = bn + 1;
                u.u_xablock = bn + 2;   /*pcs*/
            }
            return(bn);
        }
        if (readflg) {
            if (type == IFIFO) {
                raflag = 0;
                rem = ip->i_size;
            } else
                rem = ip->i_size - u.u_offset;
            if (rem < 0)
                rem = 0;
            if (rem < sz)
                sz = rem;
            if ((u.u_pbsize = sz) == 0)
                return((daddr_t)-1);
        } else if (type == IFREG) {     /*pcs*/
            if (bn >= FsPTOL(dev, u.u_limit)) {
                u.u_error = EFBIG;
                return((daddr_t)-1);
            }
        }
    }
    {
    register struct buf *bp;
    register j, sh;


    /*
     * blocks 0..NADDR-4 are direct blocks
     */
    if (bn < NADDR-3) {
        i = bn;
        nb = ip->i_addr[i];
        if (nb == 0) {
            if (readflg || (bp = alloc(mp))==NULL)  /*pcs*/
                return((daddr_t)-1);
            nb = FsPTOL(dev, bp->b_blkno);
            if ((ip->i_mode&IFMT) == IFDIR)         /*pcs*/
                bwrite(bp);                         /*pcs*/
            else                                    /*pcs*/
                bdwrite(bp);
            ip->i_addr[i] = nb;
            ip->i_flag |= IUPD|ICHG;
        }
        if ((i < NADDR-4) && raflag) {
            u.u_rablock = ip->i_addr[i+1];
            if (i < NADDR-5)                        /*pcs*/
                u.u_xablock = ip->i_addr[i+2];      /*pcs*/
        }
        return(nb);
    }

    /*
     * addresses NADDR-3, NADDR-2, and NADDR-1
     * have single, double, triple indirect blocks.
     * the first step is to determine
     * how many levels of indirection.
     */
    sh = 0;
    nb = 1;
    bn -= NADDR-3;
    for(j=3; j>0; j--) {
        sh += FsNSHIFT(dev);
        nb <<= FsNSHIFT(dev);
        if (bn < nb)
            break;
        bn -= nb;
    }
    if (j == 0) {
        u.u_error = EFBIG;
        return((daddr_t)-1);
    }

    /*
     * fetch the address from the inode
     */
    nb = ip->i_addr[NADDR-j];
    if (nb == 0) {
        if (readflg || (bp = alloc(mp))==NULL)
            return((daddr_t)-1);
        nb = FsPTOL(dev, bp->b_blkno);
        bwrite(bp);
        ip->i_addr[NADDR-j] = nb;
        ip->i_flag |= IUPD|ICHG;
    }

    /*
     * fetch through the indirect blocks
     */
    for(; j<=3; j++) {
        bp = bread(dev, nb);
        if (u.u_error) {
            brelse(bp);
            return((daddr_t)-1);
        }
        bap = bp->b_un.b_daddr;
        sh -= FsNSHIFT(dev);
        i = (bn>>sh) & FsNMASK(dev);
        nb = bap[i];
        if (nb == 0) {
            register struct buf *nbp;

            if (readflg || (nbp = alloc(mp))==NULL) {
                brelse(bp);
                return((daddr_t)-1);
            }
            nb = FsPTOL(dev, nbp->b_blkno);
            if (j < 3 || (ip->i_mode&IFMT)==IFDIR)
                bwrite(nbp);
            else
                bdwrite(nbp);
            bap[i] = nb;
            if (u.u_fmode & FSYNC)  /*pcs*/
                bwrite(bp);         /*pcs*/
            else                    /*pcs*/
                bdwrite(bp);
        } else
            brelse(bp);
    }

    /*
     * calculate read-ahead.
     */
    if ((i < FsNINDIR(dev)-1) && raflag) {
        u.u_rablock = bap[i+1];
        if (i < FsNINDIR(dev)-2)    /*pcs*/
            u.u_xablock = bap[i+2]; /*pcs*/
    }
    return(nb);
    }
}

addupc(pc, pr, incr)
caddr_t pc;
register struct prof *pr;
register incr;
{
    register short *p;
    register long idx;

    if (incr) {
        idx = ((long)pc - pr->pr_off) >> pr->pr_scale;
        if (idx < 0 || idx >= pr->pr_size)
            return;

        p = &pr->pr_base[idx];
        incr += fuword(p);
        suword(p, incr);
    }
} 

extern spte_t *copypte; /* in startup.c */
extern caddr_t copyvad; /* in startup.c */

copypage(srcpg, dstpg)
{
    register spte_t *pt;
    register n, s;
    
    pt = copypte;

    s = splhi();
    clrcache();
    
    pt++->pgi.pg_pte = srcpg | PG_V|PG_KR;
    pt->pgi.pg_pte   = dstpg | PG_V|PG_KW;
    n = bcopy(copyvad, copyvad+NBPP, NBPP);

    splx(s);
    return n;
} 

clearpage(pgnum)
{
    register spte_t *pt;
    register n, s;

    pt = copypte;

    s = splhi();
    clrcache();
    
    pt->pgi.pg_pte = pgnum | PG_V|PG_KW;
    n = bzero(copyvad, NBPP);

    splx(s);
    return n;
}

_swab(addr, sz)
caddr_t addr;
long sz;
{
    register long n;
    int npgs;
    register pte_t *pt, *pt1;
    caddr_t vaddr;

    pt = (pte_t*)&_dmapt[btotp(addr)];
    n = btotp((long)sz - 1l + (long)addr) - btotp(addr) + 1;
    
    vaddr = pgalloc(n, PG_V|PG_KW, -1);
    if (vaddr == 0) {
        printf("Could not get %d system page table entries\n", n);
        return;
    }

    pt1 = (pte_t*)svtopte(vaddr);
    npgs = n;
    while (--n >= 0)
        *pt1++ = *pt++;

    addr = vaddr + ((int)addr & POFFMASK);
    swab(addr, sz);
    pgfree(vaddr, npgs, 0);
} 
