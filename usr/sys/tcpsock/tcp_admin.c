/* PCS specific */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
/*#include "sys/systm.h"   see below */
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
#include "tcpip/protexit.h"
#include "sys/socket.h"
#include "netinet/in.h"

static char* Vers_ = "@(#) RELEASE:  1.0  May 11 1987 /usr/sys/tcpsock/tcp_admin.c";

static boolean first_call = TRUE;
boolean sodriver_ready = FALSE;
struct admin_reader adm_rdrs[NR_ADMIN_READERS];
Ulong arpa_ip_address = 0;


extern short arpa_icc;
extern struct sock_info sockets[];
extern struct exit_struct exits[];
extern short ffree_exit;
/*forward*/ struct admin_reader *Get_admr_by_pid();
extern time_t time; /* can't include sys/systm.h, because this would define nlandev */
                    /* which is not in original objfile */


tcp_adopen()
{
    int s = spldisk();
    
    if (!first_call) {
        splx(s);
        while (!sodriver_ready)
            sleep((caddr_t)&sodriver_ready, PPIPE);
        u.u_error = 0;
        return 0;
    }

    first_call = 0;
    splx(s);
    if (arpa_icc)
        icc_init(arpa_icc);
    return 0;
}

int tcp_init(/*void*/)
{
    init_admin();
    init_sockets();
    sodriver_ready = TRUE;
    wakeup((caddr_t)&sodriver_ready);
    return 0;
}

int tcp_adclose(/*void*/)
{
    int dummy;                          /* unused */
    
    cleanup_sockets();
    cleanup_admin();
    u.u_error = 0;
    return 0;
}

int send_no_param(id2, msg_code)
int id2;
Ushort msg_code;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;

    if (!suser()) {
        u.u_error = EPERM;
        return 1;
    }
    
    up = Request_msg();
    up->link = 0;
    msg = (struct mgmt_data*)up->data;
    msg->ret_code = 0;
    
    up->id[0] = ARPA_MGMT;
    up->id[1] = id2;
    up->flag |= NORELEASE;
    msg->msg_code = msg_code;
    
    Send_icc(arpa_icc, up);
    Wait_for_reply(up, ICC_WAIT_TIME);
    u.u_error = msg->ret_code;

    Release_msg(up);
    return u.u_error != 0;
}

/* TRUE=error */
int send_one_output_param(id2, msg_code, off, size, out)
int id2;
Ushort msg_code;
int off, size;
caddr_t out;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;

    if (!suser()) {
        u.u_error = EPERM;
        return TRUE;
    }
    
    up = Request_msg();
    up->link = 0;
    msg = (struct mgmt_data*)up->data;
    msg->ret_code = 0;
    
    up->id[0] = ARPA_MGMT;
    up->id[1] = id2;
    up->flag |= NORELEASE;
    msg->msg_code = msg_code;

    Send_icc(arpa_icc, up);
    Wait_for_reply(up, ICC_WAIT_TIME);
    if (msg->ret_code) {
        u.u_error = msg->ret_code;
        Release_msg(up);
        return TRUE;
    }
    
    if (copyout((int)msg + off, out, size) != 0) {
        Release_msg(up);
        u.u_error = EFAULT;
        return TRUE;
    }
    
    Release_msg(up);
    return FALSE;
}

int send_one_input_param(id2, msg_code, off, size, in)
int id2;
Ushort msg_code;
int off, size;
caddr_t in;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;

    if (!suser()) {
        u.u_error = EPERM;
        return TRUE;
    }
    
    up = Request_msg();
    up->link = 0;
    msg = (struct mgmt_data*)up->data;
    msg->ret_code = 0;
    
    if (copyin(in, (int)msg + off, size) != 0) {
        Release_msg(up);
        u.u_error = EFAULT;
        return 1;
    }
    up->id[0] = ARPA_MGMT;
    up->id[1] = id2;
    up->flag |= NORELEASE;
    msg->msg_code = msg_code;
    
    Send_icc(arpa_icc, up);
    Wait_for_reply(up, ICC_WAIT_TIME);
    u.u_error = msg->ret_code;
    Release_msg(up);
    return u.u_error != 0;
}

int send_buf_one_out(id2, msg_code, off, size, arg)
int id2;
Ushort msg_code;
int off, size;
caddr_t arg;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;
    register struct kbuffer *kp;

    if (!suser()) {
        u.u_error = EPERM;
        return TRUE;
    }
    
    up = Request_msg();
    up->link = 0;
    msg = (struct mgmt_data*)up->data;
    msg->ret_code = 0;
    up->id[0] = ARPA_MGMT;
    up->id[1] = id2;
    up->flag |= NORELEASE;
    msg->msg_code = msg_code;

    kp = Req_kbuffer();
    *(Ulong*)((int)msg + off) = logtophys(kp);

    Send_icc(arpa_icc, up);
    Wait_for_reply(up, ICC_WAIT_TIME);
    if (msg->ret_code) {
        Rel_kbuffer(kp);
        Release_msg(up);
        u.u_error = msg->ret_code;
        return TRUE;
    }
    
    if (copyout(kp, arg, size) != 0) {
        Rel_kbuffer(kp);
        Release_msg(up);
        u.u_error = EFAULT;
        return TRUE;
    }

    Rel_kbuffer(kp);
    Release_msg(up);
    return FALSE;
}


#define IOCTLA(x) (0xa00|x)
#define IOCTLB(x) (0xb00|x)
#define IOCTLC(x) (0xc00|x)
#define IOCTLD(x) (0xd00|x)
#define IOCTLE(x) (0xe00|x)

int tcp_adioctl(sonum, cmd, arg)
int sonum;
int cmd;
caddr_t arg;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;
    int dummy;                          /*unused*/
    int i;
    
    if (cmd != IOCTLC(IP_GET_ADDR)) {
        if (!suser() && cmd != SOIOC_GETSOCKET) {
            u.u_error = EPERM;
            return TRUE;
        }
    }

    u.u_error = 0;
    cleanup_sockets();
    
    msg = 0;
    
    switch (cmd) {
    case SOIOC_GETSOCKET:
        for (i=0; i < NR_OF_SOCKETS; i++) {
            if (sockets[i].so_state == SSTAT_UNUSED) break;
        }
        if (i >= NR_OF_SOCKETS) {
            u.u_error = ENODEV;
            return 1;
        }
        if (copyout(&i, arg, sizeof(int)) != 0) {
            u.u_error = EFAULT;
            return 1;
        }
        sockets[i].so_rqtime = time;
        sockets[i].so_state = SSTAT_MARKED;
        break;

    case SOIOC_FORCE_INIT:
        icc_initflg[arpa_icc] = 0;
        /*FALLTHRU*/

    case SOIOC_REINIT:
        if (arpa_icc==0) {
            u.u_error = EBUSY;
            return 1;
        }
        return icc_init(arpa_icc);

    case SOIOC_KILLER:
        return icc_killer(arg);

    case SOIOC_SETREADER:
        if (adm_rdrs[SO_LOGGING_READER].state != ADRD_UNUSED) {
            u.u_error = EALREADY;
            return 1;
        }
        adm_rdrs[SO_LOGGING_READER].state = ADRD_IDLE;
        adm_rdrs[SO_LOGGING_READER].type = 0;
        adm_rdrs[SO_LOGGING_READER].iccs = 0;
        adm_rdrs[SO_LOGGING_READER].allowed = 0xf;
        adm_rdrs[SO_LOGGING_READER].lasticc = 0;
        adm_rdrs[SO_LOGGING_READER].pid = u.u_procp->p_pid;
        Send_reader_msg(&adm_rdrs[SO_LOGGING_READER], ADRD_RSP, -1);
        adm_rdrs[SO_LOGGING_READER].data[1] = 0;
        u.u_error = 0;
        return 0;

    case SOIOC_CLRREADER:
        if (adm_rdrs[SO_LOGGING_READER].state == ADRD_UNUSED) {
            u.u_error = ENXIO;
            return 1;
        }
        adm_rdrs[SO_LOGGING_READER].state = ADRD_UNUSED;
        if (adm_rdrs[SO_LOGGING_READER].buffer)
            Rel_kbuffer(adm_rdrs[SO_LOGGING_READER].buffer);
        break;

    case 0x6106:    /*undocumented, is SETREADER for second slot */
        if (adm_rdrs[SO_MEMDUMP_READER].state != ADRD_UNUSED) {
            u.u_error = EALREADY;
            return 1;
        }
        if (copyin(arg, adm_rdrs[SO_MEMDUMP_READER].data, 2*sizeof(Ulong)) != 0) {
            u.u_error = EACCES;
            return 1;
        }
        adm_rdrs[SO_MEMDUMP_READER].state = ADRD_IDLE;
        adm_rdrs[SO_MEMDUMP_READER].type = 1;
        adm_rdrs[SO_MEMDUMP_READER].lasticc = 0;
        adm_rdrs[SO_MEMDUMP_READER].buffer = 0;
        adm_rdrs[SO_MEMDUMP_READER].pid = u.u_procp->p_pid;
        adm_rdrs[SO_MEMDUMP_READER].allowed = adm_rdrs[SO_MEMDUMP_READER].iccs = 
            1 << adm_rdrs[SO_MEMDUMP_READER].data[0];
        u.u_error = 0;
        return 0;

    case 0x610b: /*undocumented, is CLRREADER for 2nd slot */
        if (adm_rdrs[SO_MEMDUMP_READER].state == ADRD_UNUSED) {
            u.u_error = ENXIO;
            return 1;
        }
        adm_rdrs[SO_MEMDUMP_READER].state = ADRD_UNUSED;
        if (adm_rdrs[SO_MEMDUMP_READER].buffer)
            Rel_kbuffer(adm_rdrs[SO_MEMDUMP_READER].buffer);
        break;

    case IOCTLA(LANCE_SET_ADR):
        return send_one_input_param(MAN_LANCE, LANCE_SET_ADR, &msg->u, 6, arg);

    case IOCTLA(LANCE_GET_ADR):
        return send_one_output_param(MAN_LANCE, LANCE_GET_ADR, &msg->u, 6, arg);

    case IOCTLA(LANCE_CONNECT):
        return send_no_param(MAN_LANCE, LANCE_CONNECT);

    case IOCTLA(LANCE_DISCONNECT):
        return send_no_param(MAN_LANCE, LANCE_DISCONNECT);

    case IOCTLA(LANCE_HW_INIT):
        return send_no_param(MAN_LANCE, LANCE_HW_INIT);

    case IOCTLA(LANCE_GET_STAT):
        return send_buf_one_out(MAN_LANCE, LANCE_GET_STAT,
            (int)&msg->u.lance_mgmt_msg.stat_msg, 56, arg);

    case IOCTLA(LANCE_LOG_ON):
        return send_no_param(MAN_LANCE, LANCE_LOG_ON);

    case IOCTLA(LANCE_LOG_OFF):
        return send_no_param(MAN_LANCE, LANCE_LOG_OFF);

    case IOCTLA(LANCE_PROT_ON):
        return send_no_param(MAN_LANCE, LANCE_PROT_ON);

    case IOCTLA(LANCE_PROT_OFF):
        return send_no_param(MAN_LANCE, LANCE_PROT_OFF);

    case IOCTLB(ARP_INIT):
        return send_one_input_param(MAN_ARP, ARP_INIT, 
            &msg->u.arp_mgmt_msg, 10, arg);

    case IOCTLB(ARP_GET_ENTRY):
        up = Request_msg();
        up->link = 0;
        msg = (struct mgmt_data*)up->data;
        msg->ret_code = 0;
        up->id[0] = ARPA_MGMT;
        up->id[1] = MAN_ARP;
        msg->msg_code = ARP_GET_ENTRY;
        if (copyin(arg, &msg->u.arp_mgmt_msg, sizeof(Ulong)) != 0) {
            Release_msg(up);
            u.u_error = EFAULT;
            return 1;
        }
        Send_icc(arpa_icc, up);
        Wait_for_reply(up, ICC_WAIT_TIME);
        if (msg->ret_code) {
            u.u_error = msg->ret_code;
            Release_msg(up);
            return 1;
        }
        if (copyout(msg->u.arp_mgmt_msg.ether_addr, arg, 6*sizeof(Uchar)) != 0) {
            Release_msg(up);
            u.u_error = EFAULT;
            return 1;
        }
        Release_msg(up);
        return 0;

    case IOCTLB(ARP_SET_ENTRY):
        return send_one_input_param(MAN_ARP, ARP_SET_ENTRY, 
            &msg->u.arp_mgmt_msg, 10, arg);

    case IOCTLB(ARP_KILL_ENTRY):
        return send_one_input_param(MAN_ARP,ARP_KILL_ENTRY, 
            &msg->u.arp_mgmt_msg, sizeof(Ulong), arg);

    case IOCTLB(ARP_GET_STAT):
        return send_buf_one_out(MAN_ARP, ARP_GET_STAT, 
            &msg->u.arp_mgmt_msg.stat_msg, 20, arg);

    case IOCTLB(ARP_LOG_OFF):
        return send_no_param(MAN_ARP, ARP_LOG_OFF);

    case IOCTLB(ARP_LOG_ON):
        return send_no_param(MAN_ARP, ARP_LOG_ON);
        
    case IOCTLB(ARP_REQUEST_ENTRY):
        return send_one_input_param(MAN_ARP, ARP_REQUEST_ENTRY, 
            &msg->u.arp_mgmt_msg.ip_addr, sizeof(Ulong), arg);

    case IOCTLC(IP_INIT):
        return send_one_input_param(MAN_IP, IP_INIT, 
            &msg->u.ip_mgmt_msg.ip_addr, sizeof(Ulong), arg);

    case IOCTLC(IP_GET_ADDR):
        if (arpa_ip_address) {
            if (copyout(&arpa_ip_address, arg, sizeof(Ulong)) != 0)
                u.u_error = EIO;
        } else
            u.u_error = EFAULT;
        return; /* bug! return code not always set */
        
    case IOCTLC(IP_GET_NUMBER):
        i = arpa_icc;
        if (copyout(&i, arg, sizeof(int)) != 0)
            u.u_error = EIO;
        return; /*bug! return code not always set */

    case IOCTLC(IP_SET_ROUTE):
        return send_one_input_param(MAN_IP, IP_SET_ROUTE,
            &msg->u.ip_mgmt_msg, 2*sizeof(Ulong), arg);

    case IOCTLC(IP_GET_ROUTE):
        up = Request_msg();
        up->link = 0;
        msg = (struct mgmt_data*)up->data;
        msg->ret_code = 0;
        up->id[0] = ARPA_MGMT;
        up->id[1] = MAN_IP;
        msg->msg_code = IP_GET_ROUTE;
        if (copyin(arg, &msg->u.ip_mgmt_msg, sizeof(Ulong)) != 0) {
            Release_msg(up);
            u.u_error = EFAULT;
            return 1;
        }
        Send_icc(arpa_icc, up);
        Wait_for_reply(up, ICC_WAIT_TIME);
        if (msg->ret_code) {
            Release_msg(up);
            u.u_error = msg->ret_code;
            return 1;
        }
        if (copyout(&msg->u.ip_mgmt_msg.path, arg, sizeof(Ulong)) != 0) {
            Release_msg(up);
            u.u_error = EFAULT;
            return 1;
        }
        Release_msg(up);
        return 0;
        
    case IOCTLC(IP_KILL_ROUTE):
        return send_one_input_param(MAN_IP, IP_KILL_ROUTE,
            &msg->u.ip_mgmt_msg, sizeof(Ulong), arg);

    case IOCTLC(IP_LOG_ON):
        return send_no_param(MAN_IP, IP_LOG_ON);

    case IOCTLC(IP_LOG_OFF):
        return send_no_param(MAN_IP, IP_LOG_OFF);

    case IOCTLC(IP_ICMP_ON):
        return send_no_param(MAN_IP, IP_ICMP_ON);

    case IOCTLC(IP_ICMP_OFF):
        return send_no_param(MAN_IP, IP_ICMP_OFF);

    case IOCTLC(IP_GET_STAT):
        return send_buf_one_out(MAN_IP, IP_GET_STAT,
            (int)&msg->u.ip_mgmt_msg.stat_msg, 24, arg);

    case IOCTLC(IP_PROT_ON):
        return send_no_param(MAN_IP, IP_PROT_ON);

    case IOCTLC(IP_PROT_OFF):
        return send_no_param(MAN_IP, IP_PROT_OFF);

    
    case IOCTLD(TCP_INIT):
        arpa_ip_address = *(Ulong*)arg;
        return send_one_input_param(MAN_TCP_UDP, TCP_INIT,
            &msg->u.tcp_udp_mgmt_msg.ip_addr, sizeof(Ulong), arg);

    case IOCTLD(TCP_LOG_ON):
        return send_no_param(MAN_TCP_UDP, TCP_LOG_ON);

    case IOCTLD(TCP_LOG_OFF):
        return send_no_param(MAN_TCP_UDP, TCP_LOG_OFF);

    case IOCTLD(TCP_CANCEL):
        return send_one_input_param(MAN_TCP_UDP, TCP_CANCEL,
            &msg->u.tcp_udp_mgmt_msg.dest_port, sizeof(Ushort), arg);

    case IOCTLD(TCP_GET_STAT):
        return send_buf_one_out(MAN_TCP_UDP, TCP_GET_STAT,
            (int)&msg->u.tcp_udp_mgmt_msg.stat_msg, 24, arg);

    case IOCTLD(TCP_FEATURE):
        u.u_error = ENODEV;
        return 1;

    case IOCTLD(UDP_CANCEL):
        return send_one_input_param(MAN_TCP_UDP, UDP_CANCEL,
            &msg->u.tcp_udp_mgmt_msg.dest_port, sizeof(Ushort), arg);

    case IOCTLD(UDP_GET_STAT):
        return send_buf_one_out(MAN_TCP_UDP, UDP_GET_STAT,
            (int)&msg->u.tcp_udp_mgmt_msg.stat_msg, 16, arg);

    case IOCTLE(L5_INIT):
        return send_one_input_param(MAN_L5, L5_INIT,
            &msg->u.l5_mgmt_msg.nr_sockets, 2*sizeof(Ushort), arg);

    case IOCTLE(L5_GET_STAT):
        return send_buf_one_out(MAN_L5, L5_GET_STAT,
            (int)&msg->u.l5_mgmt_msg.stat_msg, 52, arg);

    case IOCTLE(L5_ABORT):
        return send_no_param(MAN_L5, L5_ABORT);

    default:
        u.u_error = EINVAL;
        return 1;
    }
    return 0;
}

icc_killer(arg)
caddr_t arg;
{
    register struct sock_info *sip;
    register struct sock_cmd_fsm *fsm;
    register short i;
    register short flag;
    struct sock_killer sk;
    
    if (copyin(arg, &sk, sizeof(struct sock_killer)) != 0) {
        u.u_error = EFAULT;
        return 1;
    }

    for (i = 0; i < NR_OF_SOCKETS; i++) {
        sip = &sockets[i];
        if (sip->so_state < SSTAT_OPENED) continue;

        flag = SLEEP_OFF;
        if (sip->so_open.so_pid == sk.so_pid) {
            fsm = &sip->so_open;
            flag = SLEEP_ON_OPEN;
        } else if (sip->so_read.so_pid == sk.so_pid) {
            fsm = &sip->so_read;
            if (sip->so_read.so_state == SSTAT_OPENED)
                flag = SLEEP_ON_READ;
            else
                flag = SLEEP_IN_READ;
        } else if (sip->so_write.so_pid == sk.so_pid) {
            fsm = &sip->so_write;
            if (sip->so_write.so_state == SSTAT_OPENED)
                flag = SLEEP_ON_WRITE;
            else
                flag = SLEEP_IN_WRITE;
        } else if (sip->so_ioctl.so_pid == sk.so_pid) {
            fsm = &sip->so_ioctl;
            flag = SLEEP_ON_IOCTL;
        } else if (sip->so_close.so_pid == sk.so_pid) {
            fsm = &sip->so_close;
            flag = SLEEP_ON_CLOSE;
        }
        
        if (flag) break;
    }
    
    if (i >= NR_OF_SOCKETS) {
        u.u_error = ESRCH;
        return 1;
    }

    if (flag) {
        sk.so_sleep = flag;
        if (sk.so_killit) {
            fsm->so_state = (Uchar)SOCMD_KILLED;
            wakeup((caddr_t)fsm);
        }
    } else
        sk.so_sleep = SLEEP_OFF;

    if (copyout(&sk, arg, sizeof(struct sock_killer)) != 0) {
        u.u_error = EFAULT;
        return 1;
    }

    return 0;
}

int tcp_adread()
{
    register struct admin_reader *ap;
    register int i;
    int icc;
    int s;
    
    if (!suser()) {
        u.u_error = EPERM;
        return TRUE;
    }

    cleanup_sockets();
    ap = Get_admr_by_pid(u.u_procp->p_pid);
    if (ap == 0) {
        u.u_error = EACCES;
        return TRUE;
    }

    while (ap->iccs == 0 && ap->state == ADRD_IDLE) {
        if (sleep((caddr_t)ap, PCATCH|PPIPE) != 0) {
            ap->state = ADRD_IDLE;
            u.u_error = EINTR;
            return TRUE;
        }
    }

    if (ap->state != ADRD_IDLE) {
        ap->state = ADRD_IDLE;
        u.u_error = EPIPE;
        return TRUE;
    }

    icc = arpa_icc;
    i = 1;
    ap->lasticc = 1;
    ap->iccs = 1;
    ap->lasticc = (icc+1) % 1;

    s = spldisk();
    ap->state = ADRD_REQ;
    
    ap->iccs &= ~i;
    switch (ap->type) {
    case 0:
        Send_reader_msg(ap, ADRD_IDLE, icc);
        break;
    case 1:
        Send_reader_msg(ap, ADRD_ERROR, icc);
        break;
    }
    
    while (ap->state == ADRD_REQ) {
        if (sleep((caddr_t)ap, PCATCH|PPIPE) != 0) {
            ap->state = ADRD_ERROR;
            u.u_error = EINTR;
            break;
        }
    }
    splx(s);
    
    if (ap->state != ADRD_RSP) {
        if (ap->state != ADRD_ERROR)
            u.u_error = EPIPE;
        ap->state = ADRD_IDLE;
        Rel_kbuffer(ap->buffer);
        ap->buffer = 0;
        return TRUE;
    }
    if (copyout((caddr_t)ap->buffer, u.u_base, 
        (int)(ap->length > u.u_count ? u.u_count : ap->length)) != 0)
            u.u_error = EFAULT;
    else {
        u.u_error = 0;
        u.u_count -= ap->length > u.u_count ? u.u_count : ap->length;
        if (ap->type == 1)
            adm_rdrs[SO_MEMDUMP_READER].data[1] += ap->length;
    }
    ap->state = ADRD_IDLE;
    Rel_kbuffer(ap->buffer);
    ap->buffer = 0;
    return u.u_error != 0;
}

boolean tcp_adwrite(/*void*/)
{
    cleanup_sockets();
    u.u_error = EACCES;
    return TRUE;
}

init_admin(/*void*/)
{
    register int i;

    for (i=0; i < NR_ADMIN_READERS; i++) {
        adm_rdrs[i].state = ADRD_UNUSED;
        adm_rdrs[i].iccs = 0;
        adm_rdrs[i].buffer = 0;
    }
}

cleanup_admin(/*void*/)
{
    register int i;
    for (i=0; i < NR_ADMIN_READERS; i++) {
        if (adm_rdrs[i].state != ADRD_UNUSED) {
            switch (i) {
            case SO_LOGGING_READER:
                Send_reader_msg(&adm_rdrs[i], ADRD_REQ, -1);
                break;
            case SO_MEMDUMP_READER:
                break;
            }
            
            if (adm_rdrs[i].buffer)
                Rel_kbuffer(adm_rdrs[i].buffer);
            adm_rdrs[i].state = ADRD_UNUSED;
        }
    }
}

init_sockets(/*void*/)
{
    register int i;
    for (i=0; i < NR_OF_SOCKETS; i++)
        sockets[i].so_state = SSTAT_UNUSED;

    for (i=0; i < (NR_EXITS-1); i++)
        exits[i].link = i+1;
    exits[NR_EXITS-1].link = -1;
    ffree_exit = 0;
}

cleanup_sockets(/*void*/)
{
    register int i;
    for (i=0; i< NR_OF_SOCKETS; i++) {
        if (sockets[i].so_state == SSTAT_MARKED &&
                (time - sockets[i].so_rqtime) >= MAX_REQUEST_TIME)
            sockets[i].so_state = SSTAT_UNUSED;
    }
}

struct admin_reader *Get_admr_by_pid(pid)
int pid;
{
    register int i;
    
    for (i = 0; i < NR_ADMIN_READERS; i++) {
        if (adm_rdrs[i].state && adm_rdrs[i].pid == (short)pid)
            return &adm_rdrs[i];
    }
    return 0;
}

Send_reader_msg(rdr, cmd, icc)
register struct admin_reader *rdr;
int cmd;
register int icc;
{
    register UNIX_MESSAGE *up;
    register struct mgmt_data *msg;
    caddr_t kbuffer;
    int dummy;                          /*unused*/
    
    up = Request_msg();
    up->link = 0;
    up->id[0] = ARPA_MGMT;
    up->id[1] = MAN_LOGGING;
    msg = (struct mgmt_data*)up->data;
    msg->ret_code = 0;
    msg->msg_code = cmd;
    switch (cmd) {
    case LOGGING_REQ:
        kbuffer = (caddr_t)Req_kbuffer();
        msg->u.log_mgmt_msg.buffer = (caddr_t)logtophys(kbuffer);
        msg->u.log_mgmt_msg.length = KBUF_SIZE;
        msg->u.log_mgmt_msg.reader_no = rdr->pid;
        rdr->buffer = kbuffer;
        Send_icc(icc, up);
        Wait_for_reply(up, ICC_WAIT_TIME);
        break;
        
    case LOGGING_CAN:
    case LOGGING_ON:
        Send_icc(arpa_icc, up);
        Wait_for_reply(up, ICC_WAIT_TIME);
        break;
        
    case MEMDUMP_REQ:
        kbuffer = (caddr_t)Req_kbuffer();
        msg->u.log_mgmt_msg.buffer = (caddr_t)logtophys(kbuffer);
        msg->u.log_mgmt_msg.length = KBUF_SIZE;
        msg->u.log_mgmt_msg.reader_no = rdr->pid;
        msg->u.log_mgmt_msg.startaddr = (caddr_t)rdr->data[1];
        rdr->buffer = kbuffer;
        Send_icc(icc, up);
        Wait_for_reply(up, ICC_WAIT_TIME);
        break;

    default:
        break;
    }
    Release_msg(up);
}

mgmt_intr(icc, up)
int icc;
register UNIX_MESSAGE *up;
{
    register struct admin_reader *ap;
    register struct mgmt_data *msg;
    
    if (up->id[1] != MEMDUMP_RSP)
        return;
    msg = (struct mgmt_data*)up->data;

    ap = &adm_rdrs[SO_LOGGING_READER];
    switch (msg->msg_code) {
    case LOGGING_IND:
        if (ap->state == ADRD_UNUSED) {
            msg->msg_code = ADRD_REQ;
            up->flag |= NORELEASE;
            Send_icc(icc, up);
            Wait_for_reply(up, ICC_WAIT_TIME);
            return;
        } else {
            ap->iccs |= (1 << icc);
            wakeup((caddr_t)ap);
            return;
        }

    case LOGGING_RSP:
        if (msg->ret_code)
            ap->state = ADRD_ERROR;
        else {
            ap->state = ADRD_RSP;
            ap->length = msg->u.log_mgmt_msg.length;
            if (msg->u.log_mgmt_msg.more_flg)
                ap->iccs |= (1 << icc);
            else
                ap->iccs &= ~(1 << icc);
        }
        wakeup((caddr_t)ap);
        break;

    case MEMDUMP_RSP:
        ap = &adm_rdrs[SO_MEMDUMP_READER];
        if (ap->state == ADRD_UNUSED)
            return;
        
        if ((ap->allowed & (1 << icc)) == 0)
            return;
        if (msg->ret_code)
            ap->state = ADRD_ERROR;
        else {
            ap->state = ADRD_RSP;
            ap->length = msg->u.log_mgmt_msg.length;
            ap->iccs |= (1 << icc);
        }
        wakeup((caddr_t)ap);
    }
}
