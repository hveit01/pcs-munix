/* encoded ICC functions */
typedef short ICC_FUNC;
#define ICC_READY       0
#define ICC_INITIAL     1
#define IW_OPEN         2
#define IW_READ         3
#define IW_WRITE        4
#define IS_SKIP         5
#define IS_READ_1BLK    6
#define LANCE_CONNECT   7
#define LANCE_READ      8
#define GETC            9
#define GETC0           10
#define PUTC            11
#define CONS_INIT       12
#define ICC_LOAD        13
#define ICC_PLOAD       14
#define ICC_START       15
#define ICC_DEBUG       16
#define ICC_TEST        17
#define OBD_CALL        18
#define ICC_IDLE        19
#define IS_MODE         20
#define IS_READ         21
#define IS_WRITE        22
#define IF_OPEN         23
#define IF_READ         24
#define IF_WRITE        25

/* maximum number of (long) parameters of an ICC call */
#define ICC_PAR 6

/* type of return value of an ICC call */
typedef long ICC_RESULT;
#define E_NONEXIST	-10000
#define E_TIMEOUT	-10001

/* communication port */
typedef struct
{	ICC_RESULT result;	/* return value */
	ICC_FUNC func;		/* encoded function or READY */
	long par[ICC_PAR];	/* parameters */
} ICC_PORT;

#define SETINT_RESET 0x87       /* interrupt value resetting ICC */

#define MAXICC  4               /* number of available ICCs */

#define ICC_BUFSIZE 128         /* length of buffer for download-I/O */

/* download ioctl commands */
#define ICC_RESET		0x1
#define ICC_RUN			0x2
#define ICC_DOWN_INITPROCS      0x3
#define ICC_INITPROCS           0x4
#define ICC_IS_INITED           0x5
#define ICC_VERSION             0x6

/* download control structure */
struct downctrl {
	ICC_PORT        dc_icc_port;         /* communication board of icc */
	int             dc_flags;            /* see below                  */
	unsigned long   dc_offset;           /* startaddress of program    */
};

/* download flags */
#define ICC_OPEN_FLAG   01
#define ICC_RESET_FLAG  02
#define ICC_WRITE_FLAG  04


extern unsigned short *icc_base[]; /* Base addresses of ICCs */

extern ICC_PORT *icc_port;
extern ICC_RESULT icc_call();
