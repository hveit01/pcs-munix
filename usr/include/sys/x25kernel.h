/*
 * This include-file describes structures and constants used only by
 * the X.25 driver.
 * Nevertheless this information migth be useful for debug-programs
 * like `dumpkernel'.
 */

/*
 * `chan' struct. One per channel.
 * MAXCHAN+1 elements are declared, but only `nchan'+1 of them used.
 * `nchan' is assigned when the user inits the X.25 controller by the
 * X25INIT ioctl.
 *
 * The rcv interrupt fills the ucmd, udata, ubuf and ulen fields.
 * The `write' and various ioctl calls from the user fill the
 * dcmd, ddata, dbuf and dlen fields.

 * Only one `cmd' field in each direction is needed for
 * CALL, NDATA, QDATA, RESET and CLEAR, because only one of these things
 * may be active at a given point of time.
 * The only exception is EDATA and EDATA+ACK: These packets may travel
 * parrallel to the NDATA/QDATA stream.
 */

#ifndef MAXCHAN
#include <sys/x25param.h>
#endif

struct svc {
	short	c_dcmd;				/* user cmd to be sent. */
	short	c_ucmd;				/* last cmd rcv'd. */
	short	c_ddata;			/* user data to be sent. */
	short	c_udata;			/* last data rcv'd. */
	char	c_dbuf[PACKETSIZE];		/* data for dcmd/ddata. */
	char	c_ubuf[PACKETSIZE];		/* data for ucmd/udata. */
	short	c_dlen;				/* bytecount in dbuf. */
	short	c_ulen;				/* bytecount in ubuf. */
	short	c_upos;				/* data read away from ubuf. */
	char	c_dedata;			/* edata to be sent. */
	char	c_uedata;			/* edata received. */
	char	c_state;			/* state of the connection. */
	char	c_channo;			/* map dev to channel. */
	char	c_ochanno;			/* last channo != 0. */
	char	c_protid;			/* listen arg. */
	char	c_clerror;			/* errno if flag&CLEARED. */
	short	c_timeo;			/* timeout counter. */
	short	c_flag;				/* Internal flags. */
	short	c_uflag;			/* User related flags. */
	short	c_pgrp;				/* Proc. group (for signals). */
	short	c_uid;				/* UID for accounting. */
	struct svcreason c_lastr;		/* Last reset rcv'd. */
	struct svcreason c_lastc;		/* Last clear rcv'd. */
} x25chans[MAXCHAN+1];

typedef struct svc *chanptr;

/* state codes: */
#define C_IDLE	 0				/* Free. */
#define C_CRIN	 1				/* Rcv'd call req. */
#define C_CROUT	 2				/* Sent call req. */
#define C_READY	 3				/* Connected. */
#define	C_RIN	 4				/* Rcv'd reset. Implies READY.*/
#define C_ROUT	 5				/* Sent reset. Implies READY. */
#define C_CLIN	 6				/* Rcv'd clear. */
#define C_CLOUT	 7				/* Sent clear. */

/* flag bits: */
#define C_OPEN		0x0001			/* Is open. */
#define C_DBUSY		0x0002			/* May not send data. */
#define C_UEDATA	0x0004			/* Rcv'd edata. */
#define C_DEDATA	0x0008			/* Sent edata. */
#define C_UEACK		0x0010			/* Rcv'd an edata ack. */
#define C_DEACK		0x0020			/* Wonna send edata ack. */
#define C_MUSTRESET	0x0040			/* Error: must reset. */
#define C_MUSTCLEAR	0x0080			/* Error: must clear. */
#define C_SIGNALLED	0x0100			/* Signal sent to pgrp. */
#define C_CLEARED	0x0200			/* restart or link down. */
#define C_LISTEN	0x0400			/* Waiting for call. */
#define C_INCOMING	0x0800			/* Is incoming call. */

/* uflag bits: */
#define C_CALLNWAIT	0x0001			/* Nonblocking call. */
#define C_READNWAIT	0x0002			/* Nonblocking read. */
#define C_READMORE	0x0004
#define C_READQ		0x0008
#define C_WRITENWAIT	0x0010			/* Nonblocking write. */
#define C_WRITEMORE	0x0020
#define C_WRITEQ	0x0040

/*
 * Mapping of channel number to minor device numbers.
 * `devnr' is the inverse of `chans[].c_channo' and is used
 * to speed up mapping of incoming packets.
 */

char x25devnr[MAXCHAN+1];

/*
 * Buffer for printf messages which go to the `x25demon' process.
 */

struct pfbuf {
	short pf_in;
	short pf_out;
	char pf_data[500];
};
