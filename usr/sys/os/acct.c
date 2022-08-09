/* @(#)acct.c   6.3 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/acct.c ";  /*pcs*/

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/acct.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/inode.h"
#include "sys/file.h"
#include "sys/munet/munet.h"        /*pcs*/
#include "sys/munet/uisw.h"         /*pcs*/

/*
 * Perform process accounting functions.
 */

sysacct()
{
    register struct inode *ip;
    register struct a {
        char* fname;
    } *uap;
    static aclock;
    
    uap = (struct a*)u.u_ap;
    if (aclock || !suser())
        return;
    aclock++;
    switch ((int)uap->fname) {              /*gcc*/
    case 0:                                 /*gcc*/
        if (acctp) {
            plock(acctp);
            iput(acctp);
            acctp = (struct inode*)NULL;    /*gcc*/
        }
        break;
    default:
        if (acctp) {
            u.u_error = EBUSY;
            break;
        }
        ip = namei(uchar, 0);
        if (ip == (struct inode*)NULL)      /*gcc*/
            break;
        if ((ip->i_mode & IFMT) == IFDIR)   /*pcs*/
            u.u_error = EISDIR;             /*pcs*/
        else                                /*pcs*/
            if ((ip->i_mode & IFMT) != IFREG)
                u.u_error = EACCES;
            else
                access(ip, IWRITE);

        if (u.u_error) {
            iput(ip);
            break;
        }
        acctp = ip;
        prele(ip);
    }
    aclock--;
}
        
/*
 * On exit, write a record on the accounting file.
 */
void acct(st)
{
    register struct inode* ip;
    off_t siz;

    if ((ip=acctp) == (struct inode*)NULL)  /*gcc*/
        return;

    if (acctdev != NODEV) {                 /*pcs*/
        u.u_callno = UIIACCTW;              /*pcs*/
        u.u_munet = st;                     /*pcs*/
        uisend(acctdev, RPC);               /*pcs*/
        return;                             /*pcs*/
    }                                       /*pcs*/

    plock(ip);
    bcopy((char*)u.u_comm, (char*)acctbuf.ac_comm, sizeof(acctbuf.ac_comm));
    acctbuf.ac_btime = u.u_start;
    acctbuf.ac_utime = compress(u.u_utime);
    acctbuf.ac_stime = compress(u.u_stime);
    acctbuf.ac_etime = compress(lbolt - u.u_ticks);
    acctbuf.ac_mem = compress(u.u_mem);
    acctbuf.ac_io = compress(u.u_ioch);
    acctbuf.ac_rw = compress(u.u_ior + u.u_iow);
    acctbuf.ac_uid = u.u_ruid;
    acctbuf.ac_gid = u.u_rgid;
    acctbuf.ac_tty = u.u_ttyp ? u.u_ttyd : NODEV;
    acctbuf.ac_stat = st;
    acctbuf.ac_flag = u.u_acflag;
    siz = ip->i_size;
    u.u_offset = siz;
    u.u_base = (caddr_t)&acctbuf;
    u.u_count = sizeof(acctbuf);
    u.u_segflg = 1;
    u.u_error = 0;
    u.u_limit = (daddr_t)5000;
    u.u_fmode = FWRITE;
    writei(ip);
    if (u.u_error) {
        ip->i_size = siz;
    }
    prele(ip);
}

int compress(t)
register time_t t;
{
    register exp = 0;
    register round = 0;

    while (t >= 8192) {
        exp++;
        round = t & 4;
        t >>= 3;
    }
    
    if (round) {
        t++;
        if (t >= 8192) {
            t >>= 3;
            exp++;
        }
    }
    
    return (exp << 13) + t;
}       
