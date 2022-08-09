/************************************************************************
*                           S T A T I S T I C                           *
*                                                                       *
* This include file describes the structure of all statistics of the    *
* processes on the ICC                                                  *
*                                                                       *
* change history:                                                       *
*       09/06/86 jh     actual version with tcp                         *
************************************************************************/


struct lance_statistic
{
unsigned long   transmitcount;          /* number of transmitted packets */
unsigned long   receivecount;           /* number of received packets */
unsigned long   trans_timeout;          /* number of timeouted packets */
unsigned long   collision;              /* number of collisions on cable */
unsigned long   missed_packets;         /* number of lost packets */
unsigned long   mem_error;              /* number of memory access errors */
unsigned long   underflow;              /* number of packets too short */
unsigned long   late;                   /* number of packets with late colli */
unsigned long   lost_carrier;           /* number of lost carriers */
unsigned long   retry;                  /* number of failed retries */
unsigned long   crc_err;                /* packets received with crc_error */
unsigned long   frame_err;              /* packets received with framing err */
unsigned long   buf_err;                /* could not get buffer in time */
unsigned long   destroy;                /* packets without STP and ENP */
};

struct arp_statistic
{
unsigned long   ill_pack;               /* number of illegal arp packets */
unsigned long   legal;                  /* number of legal arp packets */
unsigned long   num_request;            /* number of arp requests sent */
unsigned long   destroyed;              /* number of destroyed packets */
unsigned long   response;               /* number of responses to others */
}; /* struct arp_statistic */

struct ip_statistic
{
unsigned long   invalid;                /* number of invalid ip packets */
unsigned long   valid;                  /* number of   valid ip packets */
unsigned long   illsvc;                 /* invalid svc call from above */
unsigned long   send;                   /* number of packets sent */
unsigned long   ill_ip_addr;            /* packets with a wrong ip address */
unsigned long   reroute;                /* packets which have been rerouted */
}; /* struct ip_statistic */

struct icmp_statistic
{
unsigned long   invalid;                /* number of invalid ip packets */
unsigned long   valid;                  /* number of   valid ip packets */
unsigned long   illsvc;                 /* invalid svc call from above */
unsigned long   send;                   /* number of packets sent */
unsigned long   discard;                /* number of requests discarded */
}; /* struct icmp_statistic */

struct udp_statistic
{
unsigned long   invalid;                /* number of invalid packets */
unsigned long   valid;                  /* number of valid packets */
unsigned long   send;                   /* number of packets sent */
unsigned long   discard;                /* number of undeliverable packets */
}; /* struct udp_statistic */

struct tcp_statistic
{
unsigned long   invalid;                /* number of invalid packets */
unsigned long   valid;                  /* number of valid packets */
unsigned long   send;                   /* number of packets sent */
unsigned long   illsvc;                 /* illegal service call */
unsigned long   invalid_state;          /* invalid states in state machine */
unsigned long   invalid_event;          /* invalid events in state machine */
}; /* struct tcp_statistic */

struct l5_statistic
{
unsigned long   opens;                  /* # of 'open' calls */
unsigned long   closes;                 /* # of 'close' calls */
unsigned long   reads;                  /* # of 'read' calls */
unsigned long   writes;                 /* # of 'write' calls */
unsigned long   ioctls;                 /* # of 'ioctl' calls */
unsigned long   cmd_errors;             /* # of illegal calls to socketdrv */
unsigned long   mgmt_errors;            /* # of errors in management */
unsigned long   host_errors;            /* # of host/icc communication errs */
unsigned long   host_signals;           /* # signals sent to host */
unsigned long   readbytes;              /* # of 'read' bytes */
unsigned long   writebytes;             /* # of 'write' bytes */

unsigned long   noidle;                 /* #ticks not in idle */
unsigned long   idle;                   /* #ticks in idle */
}; /* struct l5_statistic */

struct prot_stat
{
unsigned long   send;                   /* # of protentry-msgs sent */
unsigned long   receive;                /* # of non-discarded rcvd msgs */
unsigned long   installs;               /* # of calls to install */
unsigned long   errors;                 /* # errors somewhere */
}; /* struct protstat */
