/*
 *
 *	The following  structure defines the order of the Control
 *	and Status registers for the SCSI CONTROLLER 
 *
 */
#define HOST_ADDR1      (struct device1*)0xe84200
#define HOST_ADDR2      (struct device2*)0xec0010

/* typedef unsigned char byte; */

struct device1 {
	short dma_cmd;          /* CREG @ 0xe84200 */
	short dma_bcnt;         /* BCNT */
	long  dma_addr;         /* HAR, LAR */
	short dma_reinit;       /* REINIT, write only */
	short dma_clock;        /* writable only */
	short o_dat;            /* SODAT */
	short i_dat;            /* SIDAT */
	long  sobir23;          /* SOBIR23 */
	short sevctl;           /* SEVCTL, write only */
	short dummya;
	short sibir0;           /* SIBIR0 */
	short sibir1;           /* SIBIR1 */
	long  sibir23;          /* SIBIR23 */
	short dummyb[8];
	short sobir1;           /* SOBIR1 */
};
struct device2 {
	short cmd;              /* CMD @ 0xec0010 */
	short cstat;            /* CSTAT */
	short bus_event_reg;    /* SBEV, read only */
	short intr1;            /* INTR1 */
	short dummyd[12];
	short tcr;		/* TCR */
	short tsr0;
	short intr0;            /* INTR0 */
	short tsr1;
	short i_req_ack_reg;	/* RACKI */
};

#define SCSI_CMD        host_addr2->cmd
#       define SCSI_IDLE        0
#       define SCSI_SW          1
#       define SCSI_DIN         2
#       define SCSI_DOUT        3

#define SCSI_CSTAT      host_addr2->cstat
#       define SCSI_DIS         0
#       define SCSI_INIT        1
#       define SCSI_TARG        2

#define SCSI_INTR0      host_addr2->intr0
#       define SCSI_I_RACKINT     1
#       define SCSI_I_INTEN       2
#       define SCSI_O_CLRRACKI    null
#       define SCSI_O_SETRACKI    1
#       define SCSI_O_CLRSINT     2
#       define SCSI_O_SETSINT     3

#define SCSI_INTR1      host_addr2->intr1
#       define SCSI_NOINTR      null
#       define SCSI_SBEVENT     1
#       define SCSI_SERROR      2
#       define SCSI_CMDDONE     3

#define SCSI_SODAT      host_addr1->o_dat

#define SCSI_SOBIR1     host_addr1->sobir1
#       define  SCSI_O_SEL      1
#       define  SCSI_O_BSY      2
#       define  SCSI_O_RST      4
#       define  SCSI_O_ATN      8

#define compr(a) ((loword(a) & 1) | ((hiword(a) & 3) << 2))

#define SCSI_SOBIR23    host_addr1->sobir23
/* ?? */

#define SCSI_TCR        host_addr2->tcr
#       define  SCSI_I_ACKIN    1
#       define  SCSI_I_REQIN    2
#       define  SCSI_O_SETREQ   null
#       define  SCSI_O_SETACK   1

#define SCSI_SEVCTL     host_addr1->sevctl
#       define  SCSI_O_CLRSEV   null
#       define  SCSI_O_SETSEV   1

#define SCSI_SIDAT      host_addr1->i_dat

#define SCSI_SIBIR0     host_addr1->sibir0
#       define  SCSI_I_RST      1
#       define  SCSI_I_ATN      2
#define SCSI_SIBIR1     host_addr1->sibir1
#       define  SCSI_I_SEL      1
#       define  SCSI_I_BSY      2

#define SCSI_SIBIR23    host_addr1->sibir23
#define CMP_SIBIR23 compr(SCSI_SIBIR23)
#       define  SCSI_I_IO       1
#       define  SCSI_I_SELEN    2
#       define  SCSI_I_CD       4
#       define  SCSI_I_MSG      8
#       define  IO              SCSI_I_IO
#       define  CD              SCSI_I_CD
#       define  MSG             SCSI_I_MSG
#       define  CMD_PHASE       CD
#       define  MESSAGE_IN      (IO|CD|MSG)
#       define  MESSAGE_OUT     (CD|MSG)
#       define  STATUS          (IO|CD)
#       define  DATAIN          IO
#       define  DATAOUT         0

#define SCSI_SBEV       host_addr2->bus_event_reg
#       define  SCSI_NEVENT     0
#       define  SCSI_PHCNG      1
#       define  SCSI_SRAB       2	/* SEL, RESEL, ATN, or BUSFREE */
#       define  SCSI_RESET      3

#define SCSI_RACKI      host_addr2->i_req_ack_reg
#       define  SCSI_I_ACK      1
#       define  SCSI_I_REQ      2

#define SCSI_CREG       host_addr1->dma_cmd
#       define  DMA_DECR        0

#define SCSI_REINIT     host_addr1->dma_reinit
#define SCSI_BCNT       host_addr1->dma_bcnt
#define SCSI_ADDR       host_addr1->dma_addr
#define SCSI_HAR      hiword(SCSI_ADDR)
#define SCSI_LAR      loword(SCSI_ADDR)

/* Interrupt Vectors */
#define SCSI_DUMMY1_VCT         0xf8
#define SCSI_DUMMY2_VCT         0xf9
#define SCSI_RACK_VCT           0xfa
#define SCSI_SBEVENT_VCT        0xfb
#define SCSI_ERROR_VCT          0xfc
#define SCSI_CMD_DONE_VCT       0xfd
#define SCSI_BOTH_VCT           0xfe
