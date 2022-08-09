/* PCS specific */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/signal.h"
#include "sys/user.h"

#define ON_UNIX 1
#include "sys/icc/unix_icc.h"
#include "tcpip/types.h"
#include "tcpip/buffers.h"
#include "tcpip/sockemu.h"
#include "tcpip/message.h"
#include "sys/socket.h"
#include "netinet/in.h"

struct sock_info sockets[NR_OF_SOCKETS];
char so_ioctl_psbl;

extern Ulong arpa_ip_address;
extern char sodriver_ready;
extern short install_exit();
extern boolean prot_is_busy();


UNIX_MESSAGE *create_msg(id2, socknum, fsm)
register int id2;
register int socknum;
register struct sock_cmd_fsm *fsm;
{
    register UNIX_MESSAGE *up;
    register struct so_open_msg *msg;
    
    tracew(0x1000);
    fsm->so_pid = u.u_procp->p_pid;
    fsm->so_state = (Uchar)SOCMD_REQUEST;
    up = Request_msg();
    fsm->so_pendmsg = up;
    fsm->so_pid = -1;
    fsm->so_errno = 0;
    up->link = 0;
    up->id[0] = ARPA_SOCKET;
    up->id[1] = id2;
    msg = (struct so_open_msg*)up->data;
    msg->so_number = socknum;
    msg->so_seqno = fsm->so_seqno;
    msg->so_retcode = 0;
    return up;
}

boolean socmd_sleep_ok(fsm, sstat)
register struct sock_cmd_fsm *fsm;
int sstat;
{
    register int s;
    
    tracew(0x1001);
    switch (sstat) {
    case SSTAT_UNUSED:
        tracew(0x1002);
        s = spldisk();
        while (fsm->so_state != (Uchar)SOCMD_IDLE && 
               fsm->so_state != (Uchar)SOCMD_INHIB) {
            if (sleep((caddr_t)fsm, PCATCH|LOPRI) != 0) {
                u.u_error = EINTR;
                return FALSE;
            }
        }
        splx(s);
        if (fsm->so_state != (Uchar)SOCMD_IDLE) {
            tracew(0x1003);
            u.u_error = fsm->so_errno;
            return FALSE;
        }
        tracew(0x1004);
        break;
        
    case SSTAT_OPENED:
        s = spldisk();
        tracew(0x1005);
        fsm->so_state = (Uchar)SOCMD_WAIT;
        fsm->so_pid = u.u_procp->p_pid;
        while (!fsm->so_ready && fsm->so_state == (Uchar)SOCMD_WAIT) {
            if (sleep((caddr_t)fsm, PCATCH|LOPRI) != 0) {
                fsm->so_state = (Uchar)SOCMD_INTRUP;
                fsm->so_errno = EINTR;
            }
        }
        
        splx(s);
        tracew(0x1006);
        fsm->so_pid = -1;
        
        switch (fsm->so_state) {
        case SOCMD_WAIT:
            fsm->so_state = (Uchar)SOCMD_OK;
            tracew(0x1007);
            break;
        case SOCMD_INTRUP:
            tracew(0x1107);
            /*FALLTHRU*/
        case SOCMD_ERROR:
            tracew(0x1008);
            u.u_error = fsm->so_errno;
            fsm->so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)fsm);
            return FALSE;
        case SOCMD_INHIB:
            tracew(0x1009);
            u.u_error = fsm->so_errno;
            return FALSE;
        case SOCMD_KILLED:
            tracew(0x100a);
            u.u_error = EDEADLK;
            fsm->so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)fsm);
            return FALSE;
        default:
            tracew(0x100b);
            u.u_error = ENOEXEC;
            return FALSE;
        }
        break;

    default:
        tracew(0x100c);
        u.u_error = ENOEXEC;
        return FALSE;
    }
    return TRUE;
}

boolean sosend_cmd(iccno, up, fsm)
Uchar iccno;
UNIX_MESSAGE *up;
register struct sock_cmd_fsm *fsm;
{
    int s;
    
    tracew(0x100d);
    fsm->so_state = (Uchar)SOCMD_SENT;
    fsm->so_pid = u.u_procp->p_pid;

    up->flag |= NORELEASE;
    Send_icc((dev_t)iccno, up);
    Wait_for_reply(up, ICC_WAIT_TIME);
    fsm->so_pendmsg = 0;
    Release_msg(up);
    
    s = spldisk();
    while (fsm->so_state < (Uchar)SOCMD_OK) {
        if (sleep((caddr_t)fsm, PCATCH|LOPRI) != 0)
            fsm->so_state = (Uchar)SOCMD_INTRUP;
    }
    splx(s);
    tracew(0x100e);
    fsm->so_pid = -1;
    switch (fsm->so_state) {
    case SOCMD_OK:
        tracew(0x100f);
        break;
    case SOCMD_INTRUP:
        tracew(0x110f);
        u.u_error = EINTR;
        fsm->so_errno = EINTR;
        wakeup((caddr_t)fsm);
        return FALSE;
    case SOCMD_ERROR:
        tracew(0x1010);
        u.u_error = fsm->so_errno;
        fsm->so_state = (Uchar)SOCMD_IDLE;
        wakeup((caddr_t)fsm);
        return FALSE;
    case SOCMD_INHIB:
        tracew(0x1011);
        u.u_error = fsm->so_errno;
        wakeup((caddr_t)fsm);
        return FALSE;
    case SOCMD_KILLED:
        tracew(0x1012);
        u.u_error = EDEADLK;
        fsm->so_state = (Uchar)SOCMD_IDLE;
        wakeup((caddr_t)fsm);
        break;
    default:
        tracew(0x1013);
        u.u_error = ENOEXEC;
        return FALSE;
    }
    return TRUE;
}

init_sofsm(fsm, ready)
register struct sock_cmd_fsm *fsm;
boolean ready;
{
    fsm->so_state = (Uchar)SOCMD_IDLE;
    fsm->so_ready = ready;
    fsm->so_errno = 0;
    fsm->so_seqno = 0;
    fsm->so_pendmsg = 0;
    fsm->so_pendpack = 0;
    fsm->so_pid = -1;
}

boolean soopen_psbl(sockno)
register int sockno;
{
    register struct sock_info *sip;

    if (!sodriver_ready) {
        u.u_error = ENXIO;
        return 0;
    }
    if (sockno >= NR_OF_SOCKETS) {
        u.u_error = ENODEV;
        return 0;
    }

    sip = &sockets[sockno];
    if (sip->so_state == SSTAT_UNUSED) {
        u.u_error = ENOENT;
        return 0;
    }
    if (sip->so_state != SSTAT_MARKED) {
        u.u_error = EBUSY;
        return 0;
    }
    
    return 1;
}

int tcp_soopen(sonum)
short sonum;
{
    register struct sock_info *sip;
    register int socknum = sonum & 0xff;
    
    if (soopen_psbl(socknum) == 0)
        return 1;
    
    tracew(0x1014);
    sip = &sockets[socknum];
    sip->so_state = SSTAT_OPENED;
    sip->so_icc = ARPA_ICC;
    sip->so_type = -1;
    sip->so_number = socknum;
    sip->so_pendread = -1;
    sip->so_readbytes = 0;
    sip->so_writebytes = 0;
    sip->so_thisaddr = arpa_ip_address;
    sip->so_lingertime = -1;
    sip->so_firstexit = -1;
    init_sofsm(&sip->so_open, 0);
    init_sofsm(&sip->so_read, 0);
    init_sofsm(&sip->so_write, 0);
    init_sofsm(&sip->so_ioctl, 1);
    init_sofsm(&sip->so_close, 1);
    u.u_error = 0;
    return 0;
}

Uchar soclose_psbl(socknum)
register int socknum;
{
    register struct sock_info *sip;
    
    tracew(0x1015);
    if (socknum >= NR_OF_SOCKETS) {
        tracew(0x1016);
        u.u_error = ENODEV;
        return 0;
    }
    
    sip = &sockets[socknum];
    if (sip->so_state <= SSTAT_MARKED) {
        tracew(0x1017);
        u.u_error = ENOTSOCK;
        return 0;
    }

    tracew(0x1018);
    return 1;
}

cancel_sock(sip)
register struct sock_info *sip;
{
    if (sip->so_open.so_pendmsg) {
        Release_msg(sip->so_open.so_pendmsg);
        sip->so_open.so_pendmsg = 0;
    }
    if (sip->so_read.so_pendmsg) {
        Release_msg(sip->so_read.so_pendmsg);
        sip->so_read.so_pendmsg = 0;
    }
    if (sip->so_write.so_pendmsg) {
        Release_msg(sip->so_write.so_pendmsg);
        sip->so_write.so_pendmsg = 0;
    }
    if (sip->so_ioctl.so_pendmsg) {
        Release_msg(sip->so_ioctl.so_pendmsg);
        sip->so_ioctl.so_pendmsg = 0;
    }
    if (sip->so_read.so_pendpack) {
        Rel_kbuffer(sip->so_read.so_pendpack);
        sip->so_read.so_pendpack = 0;
    }
    if (sip->so_write.so_pendpack) {
        Rel_kbuffer(sip->so_write.so_pendpack);
        sip->so_write.so_pendpack = 0;
    }
}

int tcp_soclose(sonum)
short sonum;
{
    register UNIX_MESSAGE *up;
    register int *unused;
    register struct sock_info *sip; 
    register int socknum = sonum & 0xff;
    
    if (soclose_psbl(socknum) == 0)
        return 1;
    
    sip = &sockets[socknum];
    cancel_user(sip);
    tracew(0x1019);
    
    up = create_msg(SOCK_CLOSE, socknum, &sip->so_close);
    sosend_cmd(sip->so_icc, up, &sip->so_close);
    cancel_sock(sip);
    sip->so_state = SSTAT_UNUSED;
    tracew(0x101b);
    u.u_error = 0;
    return 0;
}

boolean soioctl_psbl(socknum, cmd)
register int socknum;
int cmd;
{
    register struct sock_info *sip;
    
    tracew(0x101c);
    
    if (socknum >= NR_OF_SOCKETS) {
        tracew(0x101d);
        u.u_error = ENODEV;
        return FALSE;
    }
    sip = &sockets[socknum];
    if (sip->so_state < SSTAT_OPENED) {
        tracew(0x101e);
        u.u_error = EUNATCH;
        return FALSE;
    }
    if (sip->so_state == SSTAT_INTERR || sip->so_state == SSTAT_CLOSNG) {
        tracew(0x101f);
        u.u_error = EPIPE;
        return FALSE;
    }

    if (sip->so_ioctl.so_state == (Uchar)SOCMD_INTRUP)
        return TRUE;
    
    tracew(0x1020);
    return socmd_sleep_ok(&sip->so_ioctl, SSTAT_UNUSED);
}

int soioc_getaddr(sip, cmd, arg)
register struct sock_info *sip;
int cmd;    /* ioctl cmd */
caddr_t arg;
{
    struct sockaddr_in sa;
    
    if (sip->so_state < SSTAT_TYPED) {
        u.u_error = ENXIO;
        return 1;
    }

    sa.sin_family = AF_INET;
    if (cmd == SOIOC_SOCKADDR) {
        sa.sin_port = sip->so_thisport;
        sa.sin_addr.s_addr = sip->so_thisaddr;
    } else {
        if (sip->so_state < SSTAT_CONNCT) {
            u.u_error = ENOTCONN;
            return 1;
        }
        if (cmd == SOIOC_PEERADDR) {
            sa.sin_port = sip->so_destport;
            sa.sin_addr.s_addr = sip->so_destaddr;
        } else {
            sa.sin_port = sip->so_lastport;
            sa.sin_addr.s_addr = sip->so_lastip;
        }
    }
    
    if (copyout(&sa, arg, sizeof(struct sockaddr_in)) != 0) {
        u.u_error = EFAULT;
        return 1;
    }
    
    u.u_error = 0;
    return 0;
}

int soioc_exits(sip, cmd, arg)
register struct sock_info *sip;
int cmd;
caddr_t arg;
{
    register UNIX_MESSAGE *up;
    register struct so_ioctl_msg *msg;
    struct exit_struct ex;
    
    
    switch (cmd) {
    case 0xf00:
        /* pass in only type, low_bound, high_bound */
        if (copyin(arg, &ex, 3*sizeof(Ushort)) != 0) {
            u.u_error = EFAULT;
            return 1;
        }
        
        if (ex.low_bound > ex.high_bound) {
            register Ushort swp = ex.low_bound;
            ex.low_bound = ex.high_bound;
            ex.high_bound = swp;
        }
        if (prot_is_busy(&ex)) {
            u.u_error = EBUSY;
            return 1;
        }
        
        ex.ident = sip->so_number;
        ex.link = -1;
        if (install_exit(sip, &ex) < 0)
            return 1;
        
        up = create_msg(SOCK_IOCTL, sip->so_number, &sip->so_ioctl);
        msg = (struct so_ioctl_msg*)up->data;
        msg->so_ioctl = cmd;
        msg->so_port = ex.low_bound;
        msg->so_high_bound = ex.high_bound;
        msg->so_type = ex.ip_or_lance;
        
        if (sosend_cmd(sip->so_icc, up, &sip->so_ioctl) == 0)
            return 1;
        break;
    default:
        u.u_error = EINVAL;
        return 1;
    }

    sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
    wakeup((caddr_t)&sip->so_ioctl);
    u.u_error = 0;
    return 0;
}

int tcp_soioctl(sonum, cmd, arg, arg2)
short sonum;
int cmd;
caddr_t arg;
caddr_t arg2; /* unused - only general ioctl */
{
    register UNIX_MESSAGE *up;
    register struct so_ioctl_msg *msg;
    register struct sock_info *sip;
    union {
        struct sock_ioctl i;
        struct sockaddr_in sa;
    } ioc;
    Ushort rdready;
    register int socknum = sonum & 0xff;
    
    if (soioctl_psbl(socknum, cmd) == 0)
        return 1;
    
    sip = &sockets[socknum];
    if (sip->so_ioctl.so_state == (Uchar)SOCMD_INTRUP)
        return soredo_ioctl(sonum, cmd, arg, arg2, sip);

    if (cmd == SOIOC_SOCKADDR || cmd == SOIOC_PEERADDR || 
            cmd == SOIOC_READADDR)
        return soioc_getaddr(sip, cmd, arg);

    if (sip->so_type == SOCK_RAW || sip->so_type == SOCK_ETH)
        return soioc_exits(sip, cmd, arg);
    
    if (cmd == SOIOC_READPSBL) {
        rdready = sip->so_read.so_ready;
        if (copyout((caddr_t)&rdready, arg, sizeof(Ushort)) != 0) {
            u.u_error = EFAULT;
            return 1;
        }
        u.u_error = 0;
        return 0;
    }

    if (copyin(arg, (caddr_t)&ioc,
            cmd == SOIOC_SOCKET ? 
                sizeof(struct sock_ioctl) : sizeof(struct sockaddr_in)) != 0) {
        tracew(0x1021);
        u.u_error = EFAULT;
        return 1;
    }

    if (cmd == SOIOC_SOCKET) {
        tracew(0x1022);
        up = create_msg(SOCK_OPEN, socknum, &sip->so_open);
        msg = (struct so_ioctl_msg*)up->data;
        msg->so_ioctl = cmd;
        msg->so_ip_adr = arpa_ip_address;
        msg->so_port = ioc.i.so_port;
    } else {
        tracew(0x1023);
        up = create_msg(SOCK_IOCTL, socknum, &sip->so_ioctl);
        msg = (struct so_ioctl_msg*)up->data;
        msg->so_ioctl = cmd;
        msg->so_ip_adr = ioc.sa.sin_addr.s_addr;
        msg->so_port = ioc.sa.sin_port;
    }
    if (msg->so_ip_adr == 0)
        msg->so_ip_adr = arpa_ip_address;

    switch (cmd) {
    case SOIOC_SOCKET:
        tracew(0x1024);
        sip->so_type = msg->so_type = ioc.i.so_type;
        sip->so_options = msg->so_options = ioc.i.so_options;
        if (sosend_cmd(sip->so_icc, up, &sip->so_open) == 0) {
            tracew(0x1025);
            return 1;
        }
        tracew(0x1026);
        /* return thisport, destaddr, destport */
        copyout((caddr_t)&sip->so_thisport, arg, 3*sizeof(Ushort));
        sip->so_lingertime = -1;
        if (sip->so_type == SOCK_ETH || sip->so_type == SOCK_RAW) {
            sip->so_state = SSTAT_CONNCT;
            sip->so_write.so_ready = 1;
        } else
            sip->so_state = SSTAT_TYPED;
        sip->so_open.so_state = (Uchar)SOCMD_INHIB;
        break;

    case SOIOC_CONNECT:
    case SOIOC_ACCEPT:
        tracew(0x1027);
        if (sip->so_state >= SSTAT_CONNCT) {
            tracew(0x1028);
            sip->so_ioctl.so_pendmsg = 0;
            Release_msg(up);
            u.u_error = EALREADY;
            sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)&sip->so_ioctl);
            return 1;
        }
        if (sosend_cmd(sip->so_icc, up, &sip->so_ioctl) == 0) {
            tracew(0x1029);
            return 1;
        }
        ioc.sa.sin_family = AF_INET;
        ioc.sa.sin_addr.s_addr = sip->so_destaddr;
        ioc.sa.sin_port = sip->so_destport;
        if (copyout((caddr_t)&ioc, arg, sizeof(struct sockaddr_in)) != 0) {
            tracew(0x102a);
            sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)&sip->so_ioctl);
            u.u_error = EFAULT;
            return 1;
        }
        sip->so_state = SSTAT_CONNCT;
        tracew(0x102b);
        break;

    case SOIOC_RECEIVE:
    case SOIOC_SEND:
        tracew(0x102c);
        if (sip->so_type == SOCK_STREAM && sip->so_state >= SSTAT_CONNCT) {
            tracew(0x102d);
            sip->so_ioctl.so_pendmsg = 0;
            Release_msg(up);
            u.u_error = EALREADY;
            sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)&sip->so_ioctl);
            return 1;
        }
        if (sosend_cmd(sip->so_icc, up, &sip->so_ioctl) == 0) {
            tracew(0x102e);
            return 1;
        }
        sip->so_write.so_ready = 1;
        sip->so_state = SSTAT_CONNCT;
        tracew(0x102f);
        break;

    default:
        tracew(0x1030);
        tracew(cmd);
        sip->so_ioctl.so_pendmsg = 0;
        Release_msg(up);
        sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
        u.u_error = EINVAL;
        wakeup((caddr_t)&sip->so_ioctl);
        return 1;
    }

    sip->so_ioctl.so_state = (Uchar)SOCMD_IDLE;
    wakeup((caddr_t)&sip->so_ioctl);
    u.u_error = 0;
    return 0;
}

boolean soread_psbl(socknum)
register int socknum;
{
    register struct sock_info *sip;

    tracew(0x1031);
    if (socknum >= NR_OF_SOCKETS) {
        tracew(0x1032);
        u.u_error = ENODEV;
        return FALSE;
    }

    sip = &sockets[socknum];
    if (sip->so_state < SSTAT_CONNCT) {
        tracew(0x1033);
        u.u_error = ENOTCONN;
        return FALSE;
    }

    if (sip->so_state == SSTAT_HLFOPN)
        return 1;

    if (sip->so_state != SSTAT_CONNCT && sip->so_state != SSTAT_LINGER) {
        tracew(0x1034);
        u.u_error = ESHUTDOWN;
        return FALSE;
    }
    
    if (sip->so_read.so_state == (Uchar)SOCMD_INTRUP)
        return TRUE;
    
    tracew(0x1035);
    return socmd_sleep_ok(&sip->so_read, SSTAT_UNUSED);
}

int tcp_soread(sonum)
short sonum;

{
    register UNIX_MESSAGE *up;
    register struct so_read_msg *msg;
    register struct sock_info *sip;
    struct kbuffer *buf;
    caddr_t base;
    register int socknum = sonum & 0xff;
    
    if (soread_psbl(socknum) == 0)
        return 1;

    sip = &sockets[socknum];
    if (sip->so_state == SSTAT_HLFOPN)
        return 0;
    
    if (sip->so_pendread != -1 || sip->so_read.so_state == (Uchar)SOCMD_INTRUP)
        return soredo_read(sonum, sip);

    base = u.u_base;
    do {
        tracew(0x1036);
        if (socmd_sleep_ok(&sip->so_read, SSTAT_OPENED) == 0) {
            tracew(0x1037);
            return 1;
        }
        if (sip->so_state == SSTAT_HLFOPN)
            return 0;

        sip->so_read.so_ready = 0;
        up = create_msg(SOCK_READ, socknum, &sip->so_read);
        msg = (struct so_read_msg*)up->data;
        sip->so_read.so_pid = u.u_procp->p_pid;
        sip->so_read.so_state = (Uchar)SOCMD_REQUEST;
        buf = Req_kbuffer();
        sip->so_read.so_pendpack = (struct packet *)buf;
        sip->so_read.so_pid = -1;
        msg->so_data = (caddr_t)logtophys(buf);
        msg->so_len = (Ushort)(KBUF_SIZE < (short)u.u_count) != FALSE ? KBUF_SIZE : (short)u.u_count;
        msg->so_lastblock = msg->so_len == u.u_count;
        if (sosend_cmd(sip->so_icc, up, &sip->so_read) == 0) {
            tracew(0x1038);
            if (sip->so_read.so_state == (Uchar)SOCMD_INTRUP)
                return /*0*/;
            if (sip->so_read.so_pendpack)
                Rel_kbuffer(buf);
            sip->so_read.so_pendpack = 0;
            return 1;
        }
    
        if (copyout(buf, base, sip->so_lastcount) != 0) {
            tracew(0x1039);
            u.u_error = EFAULT;
            if (sip->so_read.so_pendpack)
                Rel_kbuffer(buf);
            sip->so_read.so_pendpack = 0;
            sip->so_read.so_state = (Uchar)SOCMD_IDLE;
            wakeup((caddr_t)&sip->so_read);
            return 1;
        }

        tracew(0x103a);
        u.u_error = 0;
        u.u_count -= sip->so_lastcount;
        base += sip->so_lastcount;
        sip->so_readbytes += sip->so_lastcount;
        sip->so_read.so_pendpack = 0;
        Rel_kbuffer(buf);
    } while (u.u_error==0 && sip->so_type == SOCK_STREAM && u.u_count > 0 &&
            sip->so_read.so_ready);
    
    sip->so_read.so_state = (Uchar)SOCMD_IDLE;
    wakeup((caddr_t)&sip->so_read);
    tracew(0x103b);
    return 0;
}

boolean sowrite_psbl(socknum)
register int socknum;
{
    register struct sock_info *sip;
    
    tracew(0x103c);
    if (socknum >= NR_OF_SOCKETS) {
        tracew(0x103d);
        u.u_error = ENODEV;
        return FALSE;
    }
    
    sip = &sockets[socknum];
    if (sip->so_state < SSTAT_CONNCT) {
        tracew(0x103e);
        u.u_error = ENOTCONN;
        return FALSE;
    }

    if (sip->so_state != SSTAT_CONNCT && sip->so_state != SSTAT_HLFOPN) {
        tracew(0x103f);
        u.u_error = ESHUTDOWN;
        return FALSE;
    }

    tracew(0x1040);
    return socmd_sleep_ok(&sip->so_write, SSTAT_UNUSED);
}

int tcp_sowrite(sonum)
short sonum;
{
    register UNIX_MESSAGE *up;
    register struct so_write_msg *msg;
    register struct sock_info *sip;
    struct kbuffer *buf;
    caddr_t base;
    register int socknum = sonum & 0xff;

    if (u.u_count == 0) {
        u.u_error = 0;
        return 0;
    }

    if (sowrite_psbl(socknum) == FALSE)
        return 1;

    sip = &sockets[socknum];
    if (u.u_count > KBUF_SIZE && sip->so_type != SOCK_STREAM) {
        tracew(0x1041);
        u.u_error = E2BIG;
        return 1;
    }

    base = u.u_base;
    do {
        tracew(0x1042);
        if (socmd_sleep_ok(&sip->so_write, SSTAT_OPENED) == 0) {
            tracew(0x1043);
            return 1;
        }

        if (sip->so_type == SOCK_STREAM)
            sip->so_write.so_ready = 0;
        
        up = create_msg(SOCK_WRITE, socknum, &sip->so_write);
        msg = (struct so_write_msg*)up->data;
        sip->so_write.so_state = (Uchar)SOCMD_REQUEST;
        sip->so_write.so_pid = u.u_procp->p_pid;
        buf = Req_kbuffer();
        sip->so_write.so_pendpack = (struct packet*)buf;
        sip->so_write.so_pid = -1;
        msg->so_data = (caddr_t)logtophys(buf);
        msg->so_len = (Ushort)(KBUF_SIZE < (short)u.u_count) != FALSE ? KBUF_SIZE : (short)u.u_count;
        if (copyin(base, buf, msg->so_len) != 0) {
            tracew(0x1044);
            u.u_error = EFAULT;
            sip->so_write.so_pendpack = 0;
            Rel_kbuffer(buf);
            sip->so_write.so_pendmsg = 0;
            Release_msg(up);
            sip->so_write.so_state = (Uchar)SOCMD_IDLE;
            sip->so_write.so_ready = 1;
            wakeup((caddr_t)&sip->so_write);
            return 1;
        }

        u.u_error = 0;
        u.u_count -= msg->so_len;
        sip->so_writebytes += msg->so_len;
        base = &base[msg->so_len];
        
        msg->so_lastblock = u.u_count == 0;
        if (sosend_cmd(sip->so_icc, up, &sip->so_write) == 0) {
            tracew(0x1045);
            if (sip->so_write.so_state == (Uchar)SOCMD_INTRUP) {
                u.u_error = 0;
                return 0;
            }
            if (sip->so_write.so_pendpack)
                Rel_kbuffer(buf);
            sip->so_write.so_pendpack = 0;
            return 1;
        }
        sip->so_write.so_pendpack = 0;
        Rel_kbuffer(buf);
        tracew(0x1046); 
    } while (u.u_error==0 && u.u_count > 0);
    
    sip->so_write.so_state = (Uchar)SOCMD_IDLE;
    wakeup((caddr_t)&sip->so_write);
    tracew(0x1047);
    return 0;
}

reset_sofsm(fsm, err)
register struct sock_cmd_fsm *fsm;
register short err;
{
    fsm->so_errno = err;
    fsm->so_state = (Uchar)SOCMD_INHIB;
    if (fsm->so_pid != (Ushort)-1)
        wakeup((caddr_t)fsm);
}

tell_soerror(sip, fsm, err)
register struct sock_info *sip;
register struct sock_cmd_fsm *fsm;
register short err;
{
    if (err == 207) {   /*synthetic */
        tracew(0x2001);
        sip->so_state = SSTAT_HLFOPN;
        sip->so_read.so_ready = 1;
        if (sip->so_read.so_state != (Uchar)SOCMD_WAIT)
            sip->so_read.so_state = (Uchar)SOCMD_OK;
        wakeup(&sip->so_read);
        return;
    }

    if (fsm) {
        tracew(0x2000);
        fsm->so_errno = err;
        fsm->so_state = (Uchar)SOCMD_ERROR;
        wakeup((caddr_t)fsm);
        return;
    }

    sip->so_state = SSTAT_INTERR;
    tracew(0x2003);
    reset_sofsm(&sip->so_open, err);
    reset_sofsm(&sip->so_read, err);
    reset_sofsm(&sip->so_write, err);
    reset_sofsm(&sip->so_ioctl, err);
    reset_sofsm(&sip->so_close, err);
}

socket_intr(dev, up)
int dev;
register UNIX_MESSAGE *up;
{
    register struct sock_info *sip;
    register short retcode;
    register Ushort seqno;
    register short flags;
    struct so_status_msg *msg;
    short socknum;

    tracew(0x2100);
    if (up->id[0] != ARPA_SOCKET)
        return;
    msg = (struct so_status_msg*)up->data;
    socknum = msg->so_number;
    seqno = msg->so_seqno;
    retcode = msg->so_retcode;
    sip = &sockets[socknum];
    tracew(0x2101);
    if (socknum > NR_OF_SOCKETS)    /* bug? allows one off */
        return;

    if (sip->so_state < SSTAT_MARKED)
        return;

    switch (up->id[1]) {
    case SOCK_OPEN:
        tracew(0x2102);
        if (retcode) {
            tell_soerror(sip, &sip->so_open, retcode);
            tracew(0x2103);
        } else if (seqno != sip->so_open.so_seqno) {
            tracew(0x2104);
            tell_soerror(sip, 0, EPIPE);
        } else {
            tracew(0x2105);
            sip->so_open.so_seqno = (sip->so_open.so_seqno+1) & 0x7f;
            sip->so_thisport = ((struct so_ioctl_msg*)msg)->so_port;
            sip->so_open.so_state = (Uchar)SOCMD_OK;
            wakeup((caddr_t)&sip->so_open);
        }
        break;

    case SOCK_READ:
        tracew(0x2106);
        if (retcode) {
            tracew(0x2107);
            tell_soerror(sip, &sip->so_read, retcode);
        } else if (seqno != sip->so_read.so_seqno) {
            tracew(0x2108);
            tell_soerror(sip, 0, EPIPE);
        } else {
            tracew(0x2109);
            sip->so_read.so_seqno = (sip->so_read.so_seqno+1) & 0x7f;
            sip->so_lastport = ((struct so_read_msg*)msg)->so_port;
            sip->so_lastip = ((struct so_read_msg*)msg)->so_ipaddr;
            sip->so_lastcount = ((struct so_read_msg*)msg)->so_len;
            sip->so_read.so_ready |= ((struct so_read_msg*)msg)->so_lastblock == 0;
            if (sip->so_read.so_state == (Uchar)SOCMD_SENT)
                sip->so_read.so_state = (Uchar)SOCMD_OK;
            else
                sip->so_pendread = 0;
            wakeup((caddr_t)&sip->so_read);
        }
        break;

    case SOCK_WRITE:
        tracew(0x210a);
        if (retcode) {
            tracew(0x210b);
            tell_soerror(sip, &sip->so_write, retcode);
        } else if (seqno != sip->so_write.so_seqno) {
            tracew(0x210c);
            tell_soerror(sip, 0, EPIPE);
        } else {
            tracew(0x210d);
            sip->so_write.so_seqno = (sip->so_write.so_seqno+1) & 0x7f;
            if (sip->so_write.so_state == (Uchar)SOCMD_SENT)
                sip->so_write.so_state = (Uchar)SOCMD_OK;
            else {
                Rel_kbuffer(sip->so_write.so_pendpack);
                sip->so_write.so_pendpack = 0;
                sip->so_write.so_state = (Uchar)SOCMD_IDLE;
            }
            wakeup((caddr_t)&sip->so_write);
        }
        break;

    case SOCK_IOCTL:
        tracew(0x210e);
        if (retcode) {
            tracew(0x210f);
            tell_soerror(sip, &sip->so_ioctl, retcode);
        } else if (seqno != sip->so_ioctl.so_seqno) {
            tracew(0x2110);
            tell_soerror(sip, 0, EPIPE);
        } else {
            sip->so_ioctl.so_seqno = (sip->so_ioctl.so_seqno+1) & 0x7f;
            switch (((struct so_ioctl_msg*)msg)->so_ioctl) {
            case 0xf00:
                break;
            case SOIOC_RECEIVE:
            case SOIOC_SEND:
                tracew(0x2111);
                break;
            case SOIOC_CONNECT:
            case SOIOC_ACCEPT:
                tracew(0x2112);
            default:
                tracew(0x2113);
                tell_soerror(sip, 0, EPIPE);
                return;
            }
            sip->so_ioctl.so_state = (Uchar)SOCMD_OK;
            wakeup((caddr_t)&sip->so_ioctl);
        }
        break;

    case SOCK_CLOSE:
        tracew(0x2114);
        if (retcode) {
            tracew(0x2115);
            tell_soerror(sip, &sip->so_close, retcode);
        } else if (seqno != sip->so_close.so_seqno) {
            tracew(0x2116);
            tell_soerror(sip, 0, EPIPE);
        } else {
            tracew(0x2117);
            sip->so_close.so_seqno = (sip->so_close.so_seqno+1) & 0x7f;
            sip->so_close.so_state = (Uchar)SOCMD_OK;
            wakeup((caddr_t)&sip->so_close);
        }
        break;

    case SOCK_STATUS:
        tracew(0x2118);
        flags = msg->so_flags;
        if (flags & SOSTAT_ERROR) {
            tracew(0x2119);
            tell_soerror(sip, 0, retcode);
        }
        if (flags & SOSTAT_CONNDOWN) {
            tracew(0x211a);
            sip->so_close.so_errno = retcode;
            sip->so_close.so_state = retcode == 0 ? (Uchar)SOCMD_OK : (Uchar)SOCMD_ERROR;
            wakeup((caddr_t)&sip->so_close);
        }
        if (flags & SOSTAT_CONNUP) {
            tracew(0x211b);
            sip->so_destport = msg->so_for_port;
            sip->so_destaddr = msg->so_for_ip_addr;
            sip->so_write.so_ready = 1;
            sip->so_ioctl.so_errno = retcode;
            sip->so_ioctl.so_state = retcode == 0 ? (Uchar)SOCMD_OK : (Uchar)SOCMD_ERROR;
            wakeup((caddr_t)&sip->so_ioctl);
        }
        if (flags & SOSTAT_RECVPSBL) {
            tracew(0x211c);
            sip->so_read.so_ready = 1;
            wakeup((caddr_t)&sip->so_read);
        }
        if (flags & SOSTAT_SENDCONT) {
            tracew(0x211d);
            sip->so_write.so_ready = 1;
            wakeup((caddr_t)&sip->so_write);
        }
        break;

    default:
        tracew(0x211e);
        dump_msg(up, 1000);
        tell_soerror(sip, 0, EPIPE);
        break;
    }
}

prot_intr()
{
    printf("prot intr illegally called\n");
}

dump_msg(msg, n)
register UNIX_MESSAGE *msg;
short n;
{
    printf("\r%d:Msg %08lx ", n, (int)msg);
    printf("id=%d,%d", msg->id[0], msg->id[1]);
    printf(" %08lx %08lx %08lx %08lx %08lx %08lx\n",
        msg->data[0], msg->data[1], msg->data[2],
        msg->data[3], msg->data[4], msg->data[5]);
}

dump_addr(p, n)
short *p;
short n;
{
    register int i;
    
    printf("\r");
    i = 0;
    while (i < (n+1) / 2) {
        printf("%04x ", p[i]);
        i++;
    }
    printf("\n");
}

soredo_ioctl(socknum, cmd, arg, arg2, sip)
int socknum;
int cmd;
caddr_t arg;
caddr_t arg2;
register struct sock_data *sip;
{
    printf("REDO IOCTL, OOPS OOPS OOPS OOPS OOPS, abgewuergt!\n");
    tell_soerror(sip, 0, 999);
    u.u_error = 999;
    return 1;
}

soredo_read(socknum, sip)
int socknum;
register struct sock_info *sip;
{
    register int n;
    register struct packet *pack;
    int s = spldisk();
    
    while (sip->so_pendread == -1) {
        if (sleep((caddr_t)&sip->so_read, PCATCH|LOPRI) != 0) {
            u.u_error = EINTR;
            return 1;
        }
    }
    splx(s);
    pack = sip->so_read.so_pendpack;
    n = (sip->so_lastcount - sip->so_pendread) > u.u_count ?
        u.u_count : sip->so_lastcount - sip->so_pendread;

    if (copyout((int)pack + sip->so_pendread, u.u_base, n) != 0) {
        u.u_error = EFAULT;
        Rel_kbuffer(pack);
        sip->so_read.so_pendpack = 0;
        sip->so_pendread = -1;
        sip->so_read.so_state = (Uchar)SOCMD_IDLE;
        wakeup((caddr_t)&sip->so_read);
        return 1;
    }

    u.u_error = 0;
    u.u_count -= n;
    sip->so_readbytes += n;
    sip->so_pendread += n;
    if (sip->so_pendread == sip->so_lastcount) {
        sip->so_pendread = -1;
        sip->so_read.so_pendpack = 0;
        Rel_kbuffer(pack);
        sip->so_read.so_state = (Uchar)SOCMD_IDLE;
        wakeup((caddr_t)&sip->so_read);
    }
    return 0;
}
