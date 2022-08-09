/* request.cmd values */
#define SC_RDCAPACITY   1
#define SC_OPEN         2
#define SC_READ         3
#define SC_WRITE        4
#define SC_IOCTL        5
#define SC_DOINIT       6
#define SC_CLOSE	7
#define SC_READBYTES	33
#define SC_WRITEBYTES   34

#define SCSI_SYNC       1
#define SCSI_ASYNC      2

#define SCSI_SHORT_REPLY        1
#define SCSI_LONG_REPLY         2

#define SCSI_RQSENSE    0x03    /* request sense for all controllers */
#define SCSI_READ	0x08    /* read for all controllers */
#define SCSI_WRITE	0x0A    /* write for all controllers */

/* the UNIX minor id for ICC devices is "drivenumber * 16 + filesystem".
   The drivenumber may be 0,1,2,3 for IW, or 0,1 for IS,IF.
   The driver adds to this number one of the I?_IDs below, then shifts
   right 4. The result is a number between 0 and 7, 0 to 3 for IW, 4 and 5
   for IS, and 6 and 7 for IF. A controller is expected to have at most
   two drives connected, so we support 2 IW controllers, and one IF and IS
   controller with this scheme
*/
#define IW_ID   (0*16)
#define IS_ID   (4*16)
#define IF_ID   (6*16)

#define NDRV_CTRLR	2	/* Number of drives / controller */
#define NUM_CTRLS	4	/* Number of controllers */
#define NUM_DRIVES	(NDRV_CTRLR*NUM_CTRLS)
#define NUM_IW_DRIVES	4	/* Number of winchester drives */

#define MFBLKSIZE	(8 * 124 + 10)

#undef paddr /* in buf.h */

union request {         /* for UNIX only: cf. struct b_req below */
	struct {
		struct rtk_header rtkhdr;
		short cmd;
		dev_t dev;
		daddr_t bno;
		paddr_t paddr;
		unsigned short count;
		struct buf *bp;
	} rw_cmd;

	struct {
		struct rtk_header rthkdr;
		short cmd;
		dev_t dev;
		short ioctl_cmd;
		paddr_t txaddr;
		paddr_t rxaddr;
		dev_t swapdev;
	} ioctl_cmd;

	struct {
		struct rtk_header rtkhdr;
		long data[ICC_NOFPAR];
	} message;
};

/* reply.code values
*/
#define SCSI_OK 0
#define UNIX_ERROR 1    /* bno too large, odd bno on 1k disk, ... */
#define SCSI_ERROR 2    /* real disk error, bad block .. */
#define SCSI_RESTART 3  /* once more for error recovery */

union reply {
	struct {
		short code;
		struct buf *bp;
	} ok_reply;

	struct {
		short code;
		struct buf *bp;
		short error_code;
		short disk;
		daddr_t blkno;
#define ERRMSGLEN 40
		char message[ERRMSGLEN];
	} err_reply;

	struct {
		short code;
		short blksize;  /* in bytes */
		short dsksize;  /* in MB */
#define CTRLRNAMELEN 28
		char ctrlrname[CTRLRNAMELEN];
	} dskname;
};

struct errlog {
	short code;
	short cnt;
	char message[ERRMSGLEN];
};

union ipc_reply {
	struct {
		struct rtk_header rtkhdr;
		short code;
		struct buf *bp;
	} scsi_mess;

	struct {
		struct rtk_header rtkhdr;
		long data[ICC_NOFPAR];
	} rtk_mess;
};

struct b_req {          /* for ICC only: cf. union request above */
	struct b_req *r_forw;
	char r_id[2];
	short r_cmd;
	dev_t r_dev;
	union {
		struct rw_cmd {
			daddr_t bno;
			paddr_t paddr;
			unsigned short count;
			struct buf *bp;
		} rw_cmd;

		struct ioctl_cmd {
			short ioctl_cmd;
			paddr_t txaddr;
			paddr_t rxaddr;
			dev_t swapdev;
		} ioctl_cmd;
	} r_req;
	struct buf *r_buf;	/* 1 K IO: buffer; special request: message */
	short r_drive;		/* physical drive */
	daddr_t r_pblk;		/* physical block on disk */
};

struct cdb {
	byte b1, b0, b3, b2, b5, b4, b7, b6, b9, b8;
};
struct rqs_cdb {
	byte b1, b0, b3, b2, b5, b4;
};
struct rqs_ddb {
	byte b1, b0, b3, b2;
};

/* scsi command execution modes
*/
#define XM_WAIT 0	/* wait for end of execution (polling) */
#define XM_INTR 1	/* end of execution signalled by interrupt */
#define XM_DISCON 2	/* controller may disconnect during execution */
#define XM_BIGBUF 3	/* bigbuf is needed for execution */
#define XM_BUSFREE -1	/* pseudo execution mode: scsi bus is free */

/* scsi hardware error flags (or'ed in)
*/
#define ERR_CHKCOND 2	/* CHECK CONDITION reported in status phase */
#define ERR_NOSTAT 1	/* no status phase, only COMMAND COMPLETE message */
#define ERR_PHCNG 4	/* phase change during DATAIN/OUT */
#define ERR_CANTSEL 8	/* cannot select controller */
#define ERR_DUMMY 0x10        /* checksta forced by software */
