/* @(#) mnport.h        1.0     Apr. 25 1986    */

/*
 *  Include file for MUNIX/NET - ports
 *
 *  Packets are received from the network via ports. Each process has
 *  a unique input port number set by free_mnptno. There are well-known
 *  and dynamic port numbers. A process gets a dynamic port number
 *  either by systemcall or at the first access to remote nodes.
 *  Dynamic port numbers are in the range PTNOMIN..PTNOMAX.
 *  Server processes need well-known port numbers to listen to
 *  incoming requests. Well-known port numbers are set only by systemcall.
 *  They should be in the range 1..PTNOMIN-1.
 *
 *  Processes doing IPC have to agree on port numbers.
 *
 */


/* MUNIX/NET - port structure : */

struct mnport {
	short  pt_sflags;               /* state of this port */
	short  pt_myptno;               /* my input port number */
	short  pt_yourptno;             /* input port number of sender */
	short  pt_yournode;             /* device of sender */
	short  pt_mrc;                  /* max. # of read aheads */
	short  pt_rc;                   /* current # of read aheads */
	struct rbq *pt_rbufd;           /* descriptor of received packet */
	struct mnport *pt_next;         /* to construct linked lists */
};


/* MUNIX/NET - ports : */

extern struct mnport mnport[];          /* MUNIX/NET ports (two per process) */
short  free_mnptno;                     /* next dynamic port number */
struct mnport *freeports;               /* list of free ports */
struct mnport *openports;               /* list of open ports */

/* definition concerning read aheads */

#define RQLENGTH        4               /* max. # of packets to be queued */

/* definitions concerning port numbers */

#define PTNOMIN       1001              /* first dynamic port number */
#define PTNOMAX       9999              /* last dynamic port number */

/* definition of bits in pt_sflags field */
#define PT_FREE   0        /* port is available (i.e., all bits off) */
#define PT_USED   0x0001   /* port has been allocated */
#define PT_WAIT   0x0002   /* process "ownpid" is sleeping on this port */
#define PT_RQTO   0x0008   /* time out has been requested for port */
#define PT_TIMO   0x0010   /* time out has occurred for this port */

/* definition of flags for get_port() */

#define ANYNODE         (-1)    /* accept packets from everywhere */

#define UPORT           1       /* indicates user port */
#define KPORT           2       /* indicates kernel port */
