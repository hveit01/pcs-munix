/* @(#)msv.h	1.2 */

#ifdef M32
typedef unsigned char byte;
#endif

/* This constant defines the number of stations or msv-subdevices that
are implemented. It is used to define several arrays and is used
to check the value of the dev parameter on all calls from system level */

#define MAXSTAT		16
#define NSCC		2		/* Number of devices:SCC-channels */

#define MSV_DEV(x)	(x >> 6)	/* Maximal 64	SUBDEVICES,
					   look MAXSTAT */
#define MSV_CHAN(x)	(x & 0x3f)	/* Maximal 2	DEVICES */

#define SBCOUNT		5		/* number of buffers for sender */
#if (SBCOUNT > (ICC_NOFPAR-1))
"error SBCOUNT must be less than" ICC_NOFPAR
#endif

#define REC_SIZE	(4096+2)	/* number of bytes for receiver */
#define TX_SIZE		SBCOUNT*1024	/* number of bytes for sender */

typedef struct {
	ushort	base_vektor;
	byte	*b_paddr;
} open_cmd;

typedef struct {
	short	count;
	byte	*b_paddr;
	short	rest;
}read_cmd;

typedef struct {
	short	count;
	byte	*b_paddr[SBCOUNT];
} write_cmd;

typedef struct {
	ushort	cmd;
	byte	arg[8];
} ioctl_cmd;

typedef struct {
	RTK_HEADER header;
	byte	chan_error[2];
	union {
		open_cmd	open;
		ioctl_cmd	ioctl;
		read_cmd	read;
		write_cmd	write;
	} u;
}MSV_MESSAGE;

#define M_CHAN chan_error[0]
#define M_ERROR chan_error[1]

/* ICC-commands */

#define MSV_OPEN		0
#define MSV_READ		1
#define MSV_WRITE		2
#define MSV_IOCTL		4
#define MSV_CLOSE		5
#define MSV_WAIT_RCV_BUF	6

/*****************************************************************************/
/* IOCTL - COMMANDS */

#if XXXX
/* FUER CONWARE */
#include <sys/termio.h>
#define MSV_CMD			TCIO 
#else
/* FUER UNS */
#define MSV_CMD			('M'<<8) 
#endif

#define MSV_GET_FLAGS		(MSV_CMD|1)
#define MSV_SET_FLAGS		(MSV_CMD|2)
#define MSV_SET_FLAGSW		(MSV_CMD|3)
#define MSV_GET_DEVICE_STATISTICS (MSV_CMD|4)
#define MSV_GET_SUBDEVICE_STATISTICS (MSV_CMD|5)
#define MSV_RESET_STATISTICS	(MSV_CMD|6)
#define MSV_GET_SNOOP		(MSV_CMD|7)
#define MSV_SEND_NAK		(MSV_CMD|8)
#define MSV_SEND_EOT		(MSV_CMD|9)
#define MSV_SEND_ENQ		(MSV_CMD|10)
#define MSV_SEND_ACK0		(MSV_CMD|11)
#define MSV_SEND_ACK1		(MSV_CMD|12)
#define MSV_SEND_POLL		(MSV_CMD|13)
#define MSV_SEND_SELECT		(MSV_CMD|14)
#define MSV_SET_LOOP_MODE	(MSV_CMD|15)
#define MSV_GET_LAST_STATUS	(MSV_CMD|16)
#define MSV_SET_MODEM_SIGNALS	(MSV_CMD|17)
#define MSV_SET_SLEEP_FLAG	(MSV_CMD|18)

/*----------------------------------------------------------------------------*/
/* Argument struct for ioctl-cmd: MSV_GET_FLAGS */

typedef char MSV_FLAGS[8];

/* Argument struct for ioctl-cmd: MSV_SET_FLAGS, MSV_SET_FLAGSW */

typedef	struct {
	ushort	poll,
		select,
		escape;
	byte	pads,
		syncs;
	} MSV_ADDRESS;
		
/* Argument struct for ioctl-cmd: MSV_GET_DEVICE_STATISTICS */

typedef struct {
	long	parity_errors,
		etx_with_parity_errors,
		lrc_errors,
		framing_errors,
		overrun_errors,
		underrun_errors,
		sw_overrun_errors,
		timeout_3_secs_errors,
		timeout_24_secs_errors,
		timeout_sender_errors,
		acks_timeout_errors,
		sender_retry_errors,
		sender_drop_errors,
		enqs_transmitted,
		enqs_received,
		eots_transmitted,
		eots_received,
		acks0_transmitted,
		acks0_received,
		acks1_transmitted,
		acks1_received,
		naks_transmitted,
		naks_received,
		wabts_transmitted,
		wabts_received,
		packets_received,
		packets_transmitted,
		address_cmds_transmitted,
		wrong_acks_received,
		unknowns_received,
		poll_cmds_received,
		select_cmds_received,
		invalid_cmds_received,
		bytes_transmitted,
		bytes_received,
		opens,
		bad_opens,
		closes,
		reads,
		bad_reads,
		ioctls,
		bad_ioctls,
		writes,
		bad_writes,
		read_bytes,
		write_bytes;
	} MSV_DEVICE_STATISTICS;

/* Argument struct for ioctl-cmd: MSV_GET_SUBDEVICE_STATISTICS */

typedef struct {
	long	sw_overrun_errors,
		timeout_sender_errors,
		sender_retry_errors,
		sender_drop_errors,
		packets_received,
		packets_transmitted,
		bytes_transmitted,
		bytes_received,
		ioctls,
		bad_ioctls,
		reads,
		bad_reads,
		writes,
		bad_writes,
		read_bytes,
		write_bytes;
	} MSV_SUBDEVICE_STATISTICS;

/* Argument struct for ioctl- cmd: MSV_GET_SNOOP */

typedef struct {
	long	rel_time;	/* Relative time in millisec. */
	short	bytes;		/* Total bytes or EVENT_SEND/EVENT_RECV */
	byte	dev,		/* Device ((dev<<6) | chan) */
		event,		/* Event of line */
		state,		/* Main_state of device in icc-program */
		retry,		/* Retry counter for transmitter */
		scc_r0,		/* SCC-Reg. r0 */
		scc_r1;		/* SCC-Reg. r1 */
	} MSV_SNOOP;


#define MSV_SNOOP_SIZE	60	/* (SBUFSIZE-2)/sizeof(MSV_SNOOP) */

typedef struct  {
	ushort		index;	/* index: next to fill snoop-info */ 
	ushort		overrun;/* overrun since last snoop-read  */
	MSV_SNOOP	snoop[MSV_SNOOP_SIZE];
	} MSV_SNOOP_VAR;

/* Argument struct for ioctl-cmd: MSV_SEND_POLL, MSV_SEND_SELECT */

typedef ushort MSV_CMD_ADDRESS;

/* Argument struct for ioctl-cmd: MSV_SET_LOOP_MODE */

typedef struct {
		byte clock_mode,
		     baud_rate;
		} MSV_CLOCK;

/* Argument struct for ioctl-cmd: MSV_GET_LAST_STATUS */

typedef	struct {
	byte 	two_last_char[2],
		receive_mode,
		send_data_count, /* 255,0 */
		pre_buf_count,	
		dcd_signal,	/* != 0, 0 */
		cts_signal;	/* != 0, 0 */
	} MSV_STATUS;
		
/* Argument struct for ioctl-cmd: MSV_SET_MODEM_SIGNALS */

typedef	struct {
	byte 	dtr_signal,	/* != 0, 0 */
		rts_signal;	/* != 0, 0 */
	} MSV_MODEM_SIGNAL;
		
/* Argument struct for ioctl-cmd: MSV_SET_SLEEP_FLAG */

typedef ushort MSV_SLEEP_FLAG;

/*--------------------------------------------------------------------------*/
typedef union {
		byte			mode;
		MSV_FLAGS		flags;
		MSV_ADDRESS		msv_addr;
		MSV_DEVICE_STATISTICS	msv_dev_stat;
		MSV_SUBDEVICE_STATISTICS msv_subdev_stat;
		MSV_SNOOP_VAR		msv_snoop_var;
		MSV_CMD_ADDRESS		msv_cmd_addr;
		MSV_CLOCK		msv_clock_mode;
		MSV_STATUS		msv_status;
		MSV_MODEM_SIGNAL	msv_modem_signals;
		MSV_SLEEP_FLAG		msv_sleep_flag;
} msv_io;

/***************************************************************************/

/* ICC-return code: */

#define  BAD_OPEN	EBUSY
#define	 BAD_CARD	ESRCH	/* no such process */
#define	 DEV_CLOSE	EIO
#define	 BAD_CHAN	ECHRNG
#define	 BAD_UNIT	ENXIO
#define	 BAD_COMMAND	EFAULT

typedef struct {
	long		state;
	long		flags;
	int		mode;
	struct buf	*rx_buf;	/* a receiver buffer for a subdevices */
	MSV_MESSAGE	rx_msg;
	byte		last_error;
	byte		icc_dev;
	} MSV_CHANNEL;

/* state bits */

#define MSV_OPEN_SLEEP	(long) 1
#define MSV_CLOSE_SLEEP	(long) 2
#define MSV_IOCTL_SLEEP	(long) 4
#define MSV_READ_SLEEP	(long) 8
#define MSV_WRITE_SLEEP (long) 0x10
#define MSV_ISOPEN	(long) 0x20

/* flags bits */

#define B_READ_BUSY	(long) 0x40	/* receiver- Puffer belegt */
#define B_READ_WANTED	(long) 0x80

/* the following definitions (mode) are used to control
read,write,sleep... at the interface to the unix kernel */

#define MSV_NORMAL  0	/* this state is initialized: return immediately
			on LTG error in read+write return sender errors
			return immediately if no msg available in read
			sleep in write if buffer full only for one POLL
			cycle */

#define MSV_RDAC    1	/* this is the special mode used by RDAC:
			don't look for LTG errors, just wait wait on read
			with sleep for next message to arrive wait in write
			with sleep until buffer free remove STX and ETX in
			READ don't check STX in write        */

#define MSV_SPECIAL 2	/* special mode for some application programs:
			return immediately on LTG error in read+write
			wait on read with sleep for next message to
			arrive sleep in write if buffer full only for
			one POLL cycle */

#define MSV_LOOP_M  3	/* LOOP- MODE */
/***************************************************************/

typedef struct {
	long		flags;
	long		icc_flags;
	MSV_MESSAGE	tx_msg,
			ioctl_msg;
	struct buf	*ioctl_buf,	/* a ioctl buffer for a device */
			*tx_buf[SBCOUNT];/* 5 transmit buffer for a device */
	byte		opens;
	byte		error;
	} MSV_UNIX_DEVICE;

/*--------------------------------------------------------------------------*/
/* flags bits */

#define B_IOCTL_BUSY	(long) 1	/* ioctl- Puffer belegt */
#define B_WRITE_BUSY	(long) 2	/* write- Puffer belegt */
#define B_IOCTL_WANTED	(long) 4	
#define B_WRITE_WANTED	(long) 8

/* the next bits are at the same like icc_flags */

/*#define POLLACT	(long) 0x2000 /* SET if a POLL or SELECT was received
				   during the last 24 secs, gets false after
				   24 sec of silence on the line */

/*#DEFINE SEND_BUFFER_IN_USE (long) 0x4000 */

/*#define STATLTG	(long) 0x10000 /* SET if a POLL or SELECT was received
				   during the last 24 secs, gets false after
				   24 sec of silence on the line */

/*--------------------------------------------------------------------------*/
/* Bits for variable DEVICE.icc_flags */

#define STATCH		1l	/* SET after every major line status change,
				   CLEARED after read of line status via
				   IOCTL */
#define MSV_LOOP	2l	/* LOOP-MODUS for Tests only */

#define STATPOLL	4l	/* SET before first POLL received,
				   CLEARED all the time after*/

#define PREVIOUS_SEND_IN_ERROR 8l /* this BIT is used to remember an error
                                   condition during send to the host.
                                   There is no direct way to indicate this
                                   error to the user programm. It will be
                                   reported on the next read */

#define DUEACT		0x10	/* SET during a send block to or a receive
				   block from the host */

#define SNDACT		0x20	/* SET during a send block to the receive
				   block from the host */

#define RECACT		0x40	/* SET during a receive block from the host */

#define STATSAN		0x80	/* SET if a message waits to be sended to the
				   host */
#define STATDUE		0x100	/* SET during a send block to or a receive
				   block from the host, stays true if an
				   error was found during transmit */

#define ANSER_RCVD	0x200	/* this BIT is set when any ACK
				   (NAK,DLE 0, DLE 1 or DLE ? (WABT)) is
				   received from the host */

#define PARITY_ERROR_IN_ETX 0x400 /* this BIT is set if a parity error
				   in an ETX or ETB was found */

#define WABT_RECEIVED	0x800	/* this BIT is set when a WABT was
				   received from the host */

#define LAST_ACK_WAS_NAK 0x1000 /* this variable is used to remember
				   that the last ack sended to the host
				   was a NAK */

#define POLLACT		0x2000	/* SET if a POLL or SELECT was received
				   during the last 24 secs, gets false after
				   24 sec of silence on the line */

#define SEND_BUFFER_IN_USE 0x4000

#define LTGMODE		(long) 0x8000	/* switching variable to control the
				disconnecting of a switched telefone line */

#define STATLTG		(long) 0x10000 /* SET if a POLL or SELECT was received
				   during the last 24 secs, gets false after
				   24 sec of silence on the line */

#define MSV_DCD		(long) 0x20000 /* DCD signal active */

#define MSV_SLEEP_TX	(long) 0x40000 /* Sleep 40 ms bevor sending */
#define MSV_HUNT	(long) 0x80000
#define MSV_WAKE_UNIX	(long) 0x80000000 /* wake all unix-procs */
/***************************************************************/

/* character constants */

#define  STX    2
#define  ETX    3
#define  EOT    4
#define  ENQ    5
#define  DLE    0x10
#define  NAK    0x15
#define  SYN    0x16
#define  ETB    0x17
#define  ITB    0x1F
#define  PAD	0xff

/* Constants for snoop */

/* for bytes field */
#define EVENT_SEND	1
#define EVENT_RECV	0

/* for event field */
#define EVENT_ACK0	'0'
#define EVENT_ACK1	'1'
#define EVENT_WABT	'?'
#define EVENT_NAK	NAK
#define EVENT_EOT	EOT
#define EVENT_ENQ	ENQ
#define EVENT_RX_DATA	ETX
#define EVENT_SEND_COMPLETE 0x80-4
#define EVENT_TX_DATA	0x80-3
#define EVENT_POLL	0x80-2
#define EVENT_SELECT	0x80-1
#define EVENT_ERROR	0x80
#define EVENT_ETX_P_ERR (EVENT_ERROR|1)
#define EVENT_LRC_ERR	(EVENT_ERROR|2)
#define EVENT_INV_ADDR  (EVENT_ERROR|3)
#define EVENT_DROP_ERR	(EVENT_ERROR|4)
#define EVENT_TIMEOUT	(EVENT_ERROR|5)
#define EVENT_CHAN_RES  (EVENT_ERROR|6)
#define EVENT_OPEN	(EVENT_ERROR|7)
#define EVENT_CLOSE	(EVENT_ERROR|8)
#define EVENT_WRITE	(EVENT_ERROR|9)
#define EVENT_READ	(EVENT_ERROR|10)
#define EVENT_UNKNOWN	(EVENT_ERROR|11)
#define EVENT_ACK_TIMEOUT (EVENT_ERROR|12)
#define EVENT_SENDER_TIMEOUT (EVENT_ERROR|13)

/* Constants for IOCTL-cmd.: MSV_SET_FLAGS */

#define M_RDAC		1
#define M_NORMAL	2
#define M_SPECIAL	3

/***************************************************************/

