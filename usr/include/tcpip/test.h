/* some test defines */

#define IDLE_TIME       2000L   /* in milliseconds */

/* just a bit like picasso */
extern void box();
extern void nbox();
extern void cbox();
extern char *nbr();

#undef  BOX

#define SENDERCOL       1
#define LOOPBACKCOL     16
#define RECVCOL         31
#define MGMTCOL         46
#define PROTCOL         61

#define LANCELINE       22
#define ARPLINE         18
#define IPLINE          12
#define TCPUDPLINE       6
#define HOSTLINE         1

#define HOSTPROTLINE     8
#define PROTHOSTLINE    16


/* some packet-traceing stuff */

#define SENDTRACE(send,msg,dest,pack) /*  Send_trace(send,msg,dest,pack) */

#define LANCESND        0x10
#define LANCERCV        0x11
#define LANCEINTR       0x12
#define ARPSND          0x20
#define ARPRCV          0x21
#define ARPTIM          0x22
#define IPSND           0x30
#define IPRCV           0x31
#define ICMPMODULE      0x32
#define TCPSND          0x40
#define TCPRCV          0x41
#define TCPRECV         0x401
#define TCPSTATUS       0x402
#define TCPOPEN         0x403
#define CONNOPEN        0x410   /* these routines taken from tcp_act.c */
#define DELIVER         0x411
#define DISPATCH        0x412
#define GENSYN          0x413
#define OPENFAIL        0x414
#define RESET           0x415
#define RESETSELF       0x416
#define SENDACK         0x417
#define SENDNEWDATA     0x418
#define SETFIN          0x419
#define UPDATE          0x41A
#define REPORTTIMEOUT   0x41B
#define UDPSND          0x48
#define UDPRCV          0x49
#define UDPSAVE         0x4a    /* the routine that enqueues packets */
#define UDPOPEN         0x4b
#define UDPCLOSE        0x4c
#define HOSTRCV         0x51    /* watch for this intended misplacing */
#define L5RCV           0x50    /* also look at this */
#define CMDREAD         0x501
#define CMDWRITE        0x502
#define IOCSOCKET       0x503
#define IOCCONN         0x504
#define SENDDATA        0x505
#define L5TIM           0x506
#define LANCEXMIT       0x601
#define OLDLANCE        0x602
#define LANCEFWD        0x603
#define IPFWD           0x604
#define HANDPROT        0x605
#define REMOVE          0x606

#define FREEPACKET      0xFFFF
#define GETPACKET       0xFFEE
#define ARPLANCEPORT    0x10    /* rcv-port for Lance-send */
#define ARPWPORT        0x11
#define IPARPPORT       0x20
#define LANCEARPPORT    0x21
#define TCPIPPORT       0x30
#define UDPIPPORT       TCPIPPORT
#define LANCEIPPORT     0x31
#define L5TCPPORT       0x40
#define IPTCPPORT       0x41
#define L5UDPPORT       0x48
#define IPUDPPORT       0x49
#define UDPWPORT        0x4a
#define HOSTRCVPORT     0x51
#define TCPL5PORT       0x50
#define UDPL5PORT       0x58
#define PROTHOSTPORT    0x59
#define EXITPORT        0x60
