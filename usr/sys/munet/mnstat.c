/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.1  Nov 06 1986 /usr/virt/src/sys/mnstat.c ";

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
#include "rbq.h"

#define STERR_NONE      0
#define STERR_CAT       1
#define STERR_PROTO     2
#define STERR_SNDRCV    3
#define STERR_PKTSZ     4
#define STERR_INTERNAL  5

short st_error = STERR_NONE;
short st_enable = 0;

char  *st_estr[] = {
    "Error 0",
    "Illegal count cathegory",
    "Illegal protocol type",
    "Illegal snd/rcv flag",
    "Illegal packet size",
    "Stat programming error"
};

struct stdata {
    int eth2[19];   /* ETH counts */
    int eth1[19];

    int rpc2[12];   /* RPC counts */
    int rpc1[12];

    int nfs2[158];  /* NFS counts */
    int nfs1[158];

    int buf[2];     /* BUF, CON, PORT counts */
    int con[2];
    int port;
} st;

St_nfs(bufno, cnt, cat, uutype) 
char bufno;
short cnt;
short cat;
int uutype;
{
    register int *nfsp;
    register short i;
    
    if (bufno != 1 && bufno != 2) {     /* select nfs1 or nfs2 buffer */
        st_error = STERR_SNDRCV;
        return;
    }
    if (bufno == 1)
        nfsp = st.nfs1;
    else
        nfsp = st.nfs2;
    
    if (cat != 0x0f) {                  /* must be category 0x0f */
        st_error = STERR_CAT;
        return;
    }

    switch (uutype) {                   /* collect stats depending on type */
    case UUTYPE:                        /* MUNIX/NET RPC packets */
        if (cnt < 0) {
            i = (ushort)(-cnt + 98);                /* offset x...98 */
            nfsp[2]++;                  /* negative count */
        } else {
            i = cnt;                    /* offset 0...x */
            nfsp[1]++;                  /* positive count */
        }
        break;
    case RTTYPE:                        /* DISKLESS packets */
        i = cnt + 132;                  /* offset 132...153 */
        nfsp[3]++;                      /* diskless packet */
        break;
    case UPTYPE:                        /* ignore dlupdate sync */
        return;
    case BCTYPE:                        /* cerberus packets */
        if (cnt == 1) {
            i = cnt;                    /* offset 1 */
            nfsp[1]++;                  /* positive count */
        } else {
            st_error = STERR_PROTO;
            return;
        }
        break;
    default:                            /* UXNAME packets */
        if (cnt == -20) {
            i = -cnt + 98;              /* offset 78 */
            nfsp[2]++;                  /* negative count */
        } else {
            st_error = STERR_PROTO;
            return;
        }
        break;
    }

    if (i < 0 || i > 154) { /* valid offset ? */
        st_error = STERR_INTERNAL;
        return;
    }
    nfsp[0]++;                          /* packet count */
    (nfsp+4)[i]++;                          /* increment stats */
}

St_rpc(bufno, uutype, cat)
char bufno;
short uutype;
short cat;
{
    register int *rp, *rpc2, *rpc1;
    char flag = 0;
    
    if (bufno != 1 && bufno != 2) {     /* select buffer rpc1 or rpc2 */
        st_error = STERR_SNDRCV;
        return;
    }
    if (bufno == 1) {
        rpc1 = rp = st.rpc1;
        switch (cat) {
        case 0x0f:
            flag  =1;
            rpc1[0]++;
            break;
        case 0x13:
            rpc1[1]++;
            break;
        case 0x14:
            rpc1[2]++;
            break;
        default:
            st_error = STERR_CAT;
        }
    } else {
        rpc2 = rp = st.rpc2;
        switch (cat) {
        case 0x0f:
            flag = 1;
            rpc2[0]++;
            break;
        case 0x15:
            rpc2[1]++;
            break;
        case 0x16:
            rpc2[2]++;
            break;
        case 0x17:
            rpc2[3]++;
            break;
        case 0x19:
            rpc2[4]++;
            break;
        case 0x14:
            rpc2[5]++;
            break;
        default:
            st_error = STERR_CAT;
            break;
        }
    }
    if (!flag)
        return;

    switch (uutype) {
    case UUTYPE:
        rp[6]++;
        break;
    case BCTYPE:
        rp[7]++;
        break;
    case RTTYPE:
        rp[8]++;
        break;
    case UPTYPE:
        rp[9]++;
        break;
    case UNTYPE:
        rp[10]++;
        break;
    default:
        rp[11]++;
        break;
    }
}

St_eth(bufno, proto, cat, sz)
char bufno;
short proto;
short cat;
short sz;
{
    register int *eth, *eth2, *eth1;
    char flag = 0;
    short unused;
    short pktsz;
    int unused2;

    if (bufno != 1 && bufno != 2) {
        st_error = STERR_SNDRCV;
        return;
    }
    if (bufno == 1) {
        eth1 = eth = st.eth1;
        switch (cat) {
        case 0x0f:
            flag = 1;
            eth1[0]++;
            break;
        case 0x13:
            eth1[1]++;
            break;
        case 0x14:
            eth1[2]++;
            break;
        case -1:
            flag = 1;
            break;
        default:
            st_error = STERR_CAT;
            break;
        }
    } else {
        eth2 = eth = st.eth2;
        switch (cat) {
        case 0x0f:
            flag = 1;
            eth2[0]++;
            break;
        case 0x10:
            eth2[2]++;
            break;
        case 0x11:
            eth2[3]++;
            break;
        case 0x18:
            eth2[4]++;
            break;
        case 0x19:
            eth2[1]++;
            break;
        case -1:
            flag = 1;
            break;
        default:
            st_error = STERR_CAT;
            break;
        }
    }
    
    if (flag==1) {
        if (cat == 0x0f) {
            switch (proto) {
            case ETHERPUP_IPTYPE:
                eth[5]++;
                break;
            case ETHERPUP_ARPTYPE:
                eth[6]++;
                break;
            default:
                eth[7]++;
                break;
            }
        } else {
            switch (proto & 0xff) {
            case IP_PROTO_MUNET:
                eth[8]++;
                break;
            case IP_PROTO_ICMP:
                eth[9]++;
                break;
            default:
                eth[10]++;
            }
        }
    }
    
    if (cat != 0x0f)
        return;

    pktsz = sz;
    if (pktsz < 60 || pktsz > 1514) {
        st_error = STERR_PKTSZ;
        return;
    }
    if (pktsz > 1024)
        eth[11]++;
    else if (pktsz > 512)
        eth[12]++;
    else if (pktsz > 256)
        eth[13]++;
    else if (pktsz > 128)
        eth[14]++;
    else
        eth[15]++;
}

St_buf(cat)
char cat;
{
    register int *bp = st.buf;
    if (cat == 0x1a)
        bp[0]++;
    else if (cat == 0x1b)
        bp[1]++;
    else
        st_error = STERR_SNDRCV;
}

St_con(cat)
char cat;
{
    register int *cp = st.con;

    if (cat == 0x1c)
        cp[0]++;
    else if (cat == 0x01d)
        cp[1]++;
    else
        st_error = STERR_SNDRCV;
}

St_port(cat)
char cat;
{
    register int *pp = &st.port;
    
    if (cat == 0x1e)
        (*pp)++;
    else
        st_error = STERR_SNDRCV;
}

St_msg()
{
    st_error = 0;
}

St_enable()
{
    st_enable = 1;
}

St_disable()
{
    st_enable = 0;
}

St_clear()
{
    register short i = 0;
    register char* s = (char*)&st;
    while (i < sizeof(struct stdata)) {
        *s++ = 0; i++;
    }
}

St_read(outbuf)
char *outbuf;
{
    static char *err = "Copyout error";
    
    if (copyout(&st, outbuf, sizeof(struct stdata)) == -1)
        printf("St_read : %s\n", &err);     /* BUG: & is wrong here */
}

St_write(msg, param, bufno, arg1, arg2, arg3, arg4)
char *msg;
unsigned char param;
unsigned char bufno;
short arg1;
ushort arg2;
short arg3;
int arg4;
{
    if (!st_enable)
        return;

    switch (param) {
    case 7:
        St_nfs(bufno, arg1, arg2, arg3);
        break;
    case 8:
        St_rpc(bufno, arg1, arg2);
        break;
    case 9:
        St_eth(bufno, arg1, arg2, arg3, arg4);
        break;
    case 10:
        St_buf(bufno);
        break;
    case 11:
        St_con(bufno);
        break;
    case 12:
        St_port(bufno);
        break;
    default:
        st_error = STERR_INTERNAL;
        St_msg("St_write");
        break;
    }

    if (st_error)
        St_msg(msg);
}
