/* Message flags */
#define RECEIVE 0x01
#define SEND    0x02
#define REPLY   0x04
#define ABORT   0x08
#define NORELEASE 0x10
#define TIME_OUT 0x20
#define ICC_SIG	0x40
#define U_MSG   0x80
#define OVER    0x100
#define REPLIED 0x200
#define SCC_CLR 0x400

/* Message ids */
#define SCSI_IW         1
#define SCSI_IF         2
#define SCSI_IS         3
#define LANCE		4
#define SCCA		5
#define SCCB		6
#define HOST_PRINT	7
#define ARPA_SOCKET	8
#define ARPA_PROT	9
#define ARPA_MGMT	10

#define MAX_ID          10
#define NUM_SCSI_IDS	3

/* Message: number of (long) parameters. The maximum number is limited by
	    hardware and depends on the size of a block transferred by DMA
	    ( ICC_NOFPAR * 2 + 4  <=  MAXCYCLE (defined in qbi.h)  ). */
#define ICC_NOFPAR	6	

/* Interrupt values */
#define ICC_INTR	0x40
#define ICC_INIT	0x80
#define ICC_INIT_VERS	0x81	/* write version of ICC software into icc_des */
#define ICC_RESTART	0x87	/* jump into Pikitor */

#define REQUEST 1

#define ICC_ABORT 0xa7b8

/*
 * OWNER_FLAG indicates the new icc protocol version
 */
#define OWNER_FLAG      53      /* means from "icc version 5.3" on */

/*
 * ICC_OWNER_FLAG and HOST_OWNER_FLAG used in new version to do a
 * new icc protocol to avoid too fasts interrupts.
 */
#define ICC_OWNER_FLAG  0x1000


#ifndef ASSEMBLER
typedef struct unix_message
{
	struct unix_message *link;
	long    sender;
	short 	flag;
	char 	id[2];
	long	data[ICC_NOFPAR];
} UNIX_MESSAGE;

typedef struct {
	short rcsr;
	short rbuf;
	short tcsr;
	short tbuf;
} CON_DEVICE;

typedef struct {
	short magic;	/* 0 or ICC_ABORT */
	short code;	/* see deb_par of icckernel */
	long pc;	/* program counter of ICC */
} ABORT_MESSAGE;

typedef	struct {
	UNIX_MESSAGE rr;
	UNIX_MESSAGE tr;
	CON_DEVICE con_reg;
	short rr_val;
	short tr_val;
	short tr_empty;
	short *icc_cr0;
	short *icc_cr1;
	short *icc_cr2;
	short *icc_setint;
	UNIX_MESSAGE *first, *last;
	ABORT_MESSAGE abo_msg;
	char version[40];
	short owner_flag;      /* in new version we're polling on icc side */
			       /* on this flag instead of waiting on       */
			       /* tr_empty (to avoid too fast interrupts). */
} ICC_DES;
#endif
