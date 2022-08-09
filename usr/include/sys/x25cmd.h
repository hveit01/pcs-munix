/*
 * This include-file defines structures and constants for the communication
 * between the X.25 driver and the X.25 controller.
 */

#ifndef MAXCHAN
#include <sys/x25param.h>
#endif

/*
 * Common definitions for the message interface
 * between host and controller.
 * `csr' must be last. READ THE COMMENT IN mif.c.
 *  (If you have the sources.)
 *
 *   - irp:	Set by the controller when a Q-Bus interupt is raised.
 *		Reset by the host when the interrupt was handled.
 *		Its purpose it to allow the controller to wait for the
 *		completion of a Q-Bus interrupt by polling this word.
 *   - rcvena:	Used in by the host in conjunction with `rcv.csr&GO' to
 *		tell the controller which channels have room to accept
 *		data packets. One bit per channel. The LSB in rcvena[0]
 *		corresponds to channel #0 (which is unused because channel
 *		numbers start at 1).
 *		Its actual size is (maxchan+1+7)/8.
 *   - trc:	Address of the buffer where the controller leaves some
 *		information about what happens on the message interface
 *		and on the line.
 *   - pfb:     Address of a printf buffer.
 */

struct win {
	short	cmd;			/* function code. */
	char   *bar;			/* buffer address. */
	short	bc;			/* byte count. */
	short	chan;			/* channel number */
	short 	csr;			/* control/status reg. */
};
#define err cmd				/* error code will be in `cmd' field. */

struct device {				/* layout of the common storage. */
	struct win   xmt;
	struct win   rcv;
	short        irp;		/* The host's ackn. of a Q-bus intr. */
	short	     isdown;		/* Controller->Host, if down. */
	struct cbuf *trc;
	struct cbuf *pfb;
	char	     rcvena[16];	/* Enable rcv on this chan. */
					/* maxchan will not be > 16*8.*/
	short	     h1, h2;
	long	     lh1, lh2;		/* help fields for debugging. */
};
#define xuid h1

/*
 * CSR bits.
 * No more then the lower 8 Bit can be defined, because the trace info
 * transfers only the lower bytes.
 */

#define XGO     01
#define XDONE   02
#define XERROR  04
#define XWAIT  010			/* only in xmt.csr. */
#define XINIT  020			/* only in xmt.csr. */

/*
 * Cmd codes.
 * No cmd may be 0. 0 means <empty>.
 *
 * For internal reasons RESET, CLEAR and RESTART must be sorted
 * numerically and must have the highest numbers of all.
 * This is used to allow the following programming technique:
 * A reset, clear, or restart command flushes everything with
 * a lower command code.
 */

#define CALL	 001
#define NDATA    002
#define QDATA    003
#define EDATA    004
#define RRP	 005			/* For l3 internal use only. */
#define RNRP	 006			/* For l3 internal use only. */
#define BADP	 007			/* For l3 internal use only. */
#define RESET    010
#define CLEAR	 011
#define RESTART  012
#define DIAGP	 013			/* For l3 internal use only. */
#define ACCOUNT	 014			/* For ICC->UNIX comm. only. */
#define CMDCODE  017			/* Mask for cmd code. */
#define ACK	 020
#define CMD      037			/* Mask for cmd code + ack bit. */
#define MOREDATA 040			/* only in NDATA+QDATA cmd's. */

/*
 * Error-codes.
 * Error-codes bigger then 255 can't be defined, because the trace info
 * contains only the lower byte.
 */

/* After the INIT phase: */
#define M_EBADCHAN   1			/* Bad channel number. */
#define M_EBADCMD    2			/* Unknown cmd field. */
#define M_ESIZE      3			/* Byte-count too big. */
#define M_EOVERFLOW  4			/* Too many data, too many reset...*/
#define M_EMUSTRESET 5			/* X25 protocol error detected. */
#define M_EMUSTCLEAR 6			/* X25 protocol error detected. */
/* In the INIT phase: */
#define M_ENOSTOR    7			/* Not enough storage. */
#define M_ENOLINE    8			/* No modem signals seen. */
#define M_EL2ADDR    9			/* Data received, but bad HDLC address*/
					/* Might be `dce' is set wrong. */
/* In both phases: */
#define M_EL2DOWN   10                  /* Link down. */
#define M_EL2FAIL   11			/* Link went down, but is up again. */

/*
 * When the host initializes the controller the data buffer is filled with 
 * a `struct conf', which parameterizes the controller.
 * The field `cf_errcode' is only used as return value.
 */

struct conf {
	short cf_errcode;		/* Error code, if INIT fails. */
	short cf_maxchan;		/* Number of L3 channels. */
	short cf_l3dq_len;		/* L3 xmit buffering. */
	short cf_dce;			/* Act as DCE. Also afftects L2. */
	short cf_T1;			/* HDLC timeout time in seconds. */
	short cf_N2;			/* HDLC retry counter. */
	short cf_genclock;		/* Generate Clock. */
	short cf_baud;			/* Baudrate (if genclock). */
	short cf_modem;			/* Modem lines protocol. */
	short cf_testloop;		/* Testing with a testloop connector. */
	short cf_debug1;
};

#define cf_qvec cf_errcode

/* cf_modem bits. */

#define MODEM_WAIT	0x01		/* Wait forever until DCD+CTS seen. */
					/* Otherwise INIT fails after timeout.*/
#define MODEM_IGNORE	0x02		/* Don't care for DCD+CTS at all. */

/* cf_debug1 bits. */

#define DEBUG1_MODEM	0x01		/* print DCD,CTS status. */
#define DEBUG1_BUFFER	0x02		/* Do a bufcheck() every 0.5 sec. */
#define DEBUG1_TRACE	0x04		/* Generate trace stream. */
#define DEBUG1_ACCOUNT	0x08		/* Generate account info on mif. */
#define DEBUG1_WAITRDB	0x10		/* On abort: no exit but wait for rdb.*/

/*
 * Communication FIFO for diagnostic data streams from the controller
 * to the host.
 * Current streams are: printf info and trace of incoming/outgoing frames.
 */

struct cbuf {
	char   *c_addr;
	short	c_len;
	short	c_in;
	short	c_out;
};

/*
 * Definitions for the header bytes of the "trace" diagnostic stream.
 */

#define TRC_MIF 	0x10
#define TRC_FRIN	0x20
#define TRC_FROUT	0x30
#define TRC_OVERFLOW	0x40

/*
 * Account info:
 * The accounting info is a stream of pairs of the form:
 *		len, struct acntrec.
 * `ac_data' is partially filled, according to `len'.
 * If ac_cmd&CMDCODE is CALL, CLEAR and RESTART, ac_data contains
 * the actual packet and `len' can be used to compute how long it is.
 * If ac_cmd&CMDCODE is NDATA, QDATA or EDATA, ac_dlen just tells how
 * long the data-packet was.
 * A sequence number (modulo 256) is included to detect losses.
 */

struct acntrec {
	char	ac_seqnr;
	char	ac_chan;
	short	ac_uid;
	char	ac_cmd;
	char	ac_dir;
	union {
		char	ac_d[PACKETSIZE];
		short	ac_dl;
	} ac_info;
};

#define ac_data ac_info.ac_d
#define ac_dlen ac_info.ac_dl

#define ACNT_OVERFLOW 0177		/* mustn't collide with "cmd codes". */
