/* FILE cbip.h  VERSION 6.2  DATE 86/12/09  TIME 13:37:52 */

#define TRACE 1

/* Interrupt Vector Register ievec */
#define BMT_LEVEL	0x40		/* Interrupt level  - 1 */
#define SCC_VEC		0	/* Offset of SCC interrupt vector */
#define MOUSE_VEC	4	/* Offset of MOUSE_BUUTON interrupt vector */
#define VSYNC_VEC  	8	/* Offset of VSYNC interrupt vector */

/* Base of I/O          */
#define FRAME_SIZE 	0x000fffff         /* length of frame buffer in bytes */
#define CBIP_IOBASE     0xF0000000	
#define CLT_R_OFFSET    0x00000000
#define CLT_G_OFFSET    0x00000400
#define CLT_B_OFFSET    0x00000800
#define MICE_OFFSET	0x00000C00
#define SCC_OFFSET	0x00000D00
#define MON_OFFSET	0x00000E00
#define INTR_OFFSET	0x00000F00

/* SCC Constants */
#define SCCA	0
#define SCCB	1

/* Interrupt Enable Register - iereg */

#define LBUTTON_IEN	0x02	/* Left Button enabled */
#define RBUTTON_IEN	0x04	/* Right Button enabled */
#define MBUTTON_IEN	0x08	/* Middle Button enabled */
#define MOUSE_IEN 	0xe    /* Mouse interrupt enable on Frame Buffer */
#define VSYNC_IEN	0x1	/* VSYNC interrupt enable */

/*  Mouse status register  */

#define L_DONE		0x10
#define R_DONE		0x04
#define M_DONE		0x01
#define	L_UP		0x20
#define	M_UP		0x02
#define	R_UP		0x08
#define BUTTON_DONE	(L_DONE| M_DONE| R_DONE)


#define FB_LL 0x3ffff         /* length of frame buffer in longs */

struct dev_clt {              /* colour lookup tables */
	long    clt_r[256];   /* red */
	long    clt_g[256];   /* green */
	long    clt_b[256];   /* blue */
};


struct dev_mouse {
	long    mscr;           /* mouse status/control register */
	long    mcntx;          /* mouse counter x direction */
	long    mcnty;          /* mouse counter y direction */
};


struct dev_reg {
	long    cmd;        /* chanel command port */
	long    data        /* chanel data port */
};


struct dev_mon {
	long    vreg;   /* vsync register */
	long    ereg;   /* enable monitor register */
	long    hreg;   /* horicontal panning register */
};


struct dev_intr {
	long    iereg;  /* interrupt enable register */
	long    ievec;  /* interrupt vector register */
};

/*===================================================*/



/* MESSAGE FLAGS */
/*
#define REPLIED		0x1
#define NORELEASE	0x4
#define IN_QUEUE	0x8
*/

/* CONSOLE FLAG  */
#define REQUEST		01


#define CLIENT_WAITING 	1
#define SERVER_WAITING 	2
#define EVENT_SELECT	4
#define MOUSE_WAITING	8
#define KBD_WAITING	0x10
#define TAB_WAITING	0x20
#define TTY_WAITING	0x40
#define DRIVER_WAITING 	(TTY_WAITING |MOUSE_WAITING |KBD_WAITING |TAB_WAITING)
#define ENDLINK ((BMT_MESSAGE *) -1)
#define Port_empty(p) (p->first == ENDLINK)
#define spl_cbmt()	spltty()
#define PBMT 	PZERO+3
#define MSG_SIZE 	8
#define POOL_SIZE       32	/* Message Pool */
#define EPOOL_SIZE 	32	/* Event Pool */
#define WindowMask	long
typedef struct {
	short	flag;
	BMT_MESSAGE *first, *last;
	} BMT_PORT;


typedef struct{
	short	rcsr;
	short	rbuf;
	short	tcsr;
	short	tbuf;
	} CON_DEVICE;


typedef struct event_message
{
	struct	event_message *link;
	short		prio;
	EventMask	mask;
	EventRecord	event;
	} EVENT_MESSAGE;

typedef struct event_queue
{
	short	flag;
	EVENT_MESSAGE	first;
	EVENT_MESSAGE	last;
	EVENT_MESSAGE	*free;
	EVENT_MESSAGE	pool[EPOOL_SIZE];
	char		pool_sem;
	} EVENT_QUEUE;


/* **************************************************************************
 * 	For each BMT there is  a variable of BMT_DES which is located in 
 *  	BMT memory.
 * **************************************************************************/

typedef	struct
	{
	short		bmtid;
	short		pool_flag;
	short		pool_sem;
	unsigned long 	TickCount;
	short		activeWinInd;
	BMT_MESSAGE 	*free;
	BMT_MESSAGE 	msg_pool[POOL_SIZE+1];
	BMT_PORT 	msgport;
	EVENT_QUEUE	queue;
	LONG_BMT_MESSAGE long_msg;
	Kbd_msg		kbd_msg;
	BMT_MESSAGE	tty_msg;
	char		rbuf[MSG_SIZE<<2];
	struct proc	*col_proc;
	char		*pbuf;
	int		pcnt;
} BMT_DES;

/*************************** Constant Definition *************************/
#ifndef NULL
#define NULL ((char *)0L)
#endif
#ifndef NIL
#define NIL ((char *)0L)
#endif
#define SCC_PRIO INT_LEV6



