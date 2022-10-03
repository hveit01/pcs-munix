/* FILE bmt.h  VERSION 6.7  DATE 87/01/08  TIME 13:39:59 */

#ifndef CWS1

/*
Modifications
vers    when    who     what
1.6     04 oct  uh      ??????????????????????????
*/


#define STOP 	0
#define RUN 	1
#define READY	1
#define GO	2
#define INT_ENABLE 	0x40 	/* interrupt enable mask in CSW register */
#define REFRESH_VEC	0x7c	/* interrupt vector for refresh routine */
#define QBUS_VEC	0x64	/* interrupt vector for Qbus interrupt */
#define ENTRY_ADDR	0x400	/* start address of down-loaded program */
#define INTR_BASE	0x1f4f0	/* address of redirected interrupt table */

#define RAM_MAP		2	/* RAM memory mapping
				   -- RAM_OFFSET below depends on RAM_MAP */
#define ROM_MAP		0	/* ROM memory mapping */

#define RAM_SIZE	0x01ffe0L
#define BOOT_SIZE	0x140L
#define COMM_SIZE	8L
#define SCC_BASE	0xfffff0
#define SCC_VEC		0x300

#define HOST_ADDR(m) ((long)unix_base + (long) m)
#define BMT_ADDR(m)  ((0x1ffffL & (long)m) + 0x40000L)
  
/* Dennis Bzowy compatible memory structure (for prom talking)  */
typedef long (*Funcp)();

#define idle	0
#define rintr	1
#define	wintr	2

typedef struct {
	char    ram[ 0x3e000 ],
		bss[  0x1000 ];
	long    func; /*  host -> bitmap call box  */
	long    args[6],
		funcret;
	char    wbuf[ 1024 ],
		keys[   32 ],
		funcs[  32 ];
	short   intr_state;
		/*  <-> unix bip tty driver /usr/src/sys/dev/bip.c:
		 *  biprint sets intr_state = 0   => keyb can qintr again
		 */
	short     rintno, wintno; /*  E0/4  =>  qbus interrupt on key  */
	char    skip[ 0x3ffe0 - 0x3f466 ];
/*  3ffe0: bip ctl:  */
	short   go,
		intr_local68,
		map0,
		map1,
		mousereg,
		mregs[  3 ],
		sio[    4 ],
		pixctl[ 4 ]; /*  sizeof( struct device) == 0x40000  */
} Bip_device;

/*   Layout of a Bitmap memory      */

typedef struct bip_device {
	char    ram[ (RAM_SIZE - COMM_SIZE - BOOT_SIZE - 12L) ];
	char	boot[BOOT_SIZE];
	short	host_intr;	/* Host interrupt vector set by  the Host  */
	short	bmt_id;		/* Bitmap id set by the Host or dowload    */
	long	host_base;	/* Start address of BMT ram for the Host   */
	long	bmt_desp;	/* Pointer to BMT_DES structure set by Bmt */
	char	dbg[COMM_SIZE]; /* Communication Arrea for RTK Debugger    */
	short   go, 		/* Bitmap controll registers               */
		intr_local68,
		map0,
		map1,
		mousereg,
		mregs[  3 ],
		sio[    4 ],
		pixctl[ 4 ]; /*  sizeof( struct device) == 0x40000  */
} BIP_DEVICE;


/* CONSOLE FLAG  */
#define REQUEST		01

#define BMT_MAGIC	(0x01031944)	/* J.D */
/*   FIFO SIGNALS */
#define F_MSG_NOT_FULL		1
#define F_MSG_NOT_EMPTY		2
#define F_INTR_NOT_EMPTY	3
#define F_INTR_NOT_FULL		4
#define F_POOL_NOT_EMPTY	5
#define F_CONS_REQUEST		6
#define F_REPLY_MSG		7
#define F_TTY_TREADY		8
#define F_TTY_RREADY		9
#define F_EPOOL_NOT_EMPTY	10
#define F_EPORT_NOT_EMPTY	11
#define F_WPS_WAKEUP		12
#define F_LONG_SEND		13	/* Start Long Send 	*/
#define F_LONG_RECEIVE		14	/* Start Long Receive 	*/
#define F_SEND_READY		15	/* End of Long Send 	*/
#define F_RECEIVE_READY		16	/* End of  Long Receive */
#define F_LONG_RELEASE		17	/* End of  Long Receive */


#define BMT_PRIO	5		/* Process Priority */
#define UNIX_WAITING 	1
#define BMT_WAITING 	2
#define EVENT_SELECT	4		/* J.D for select call */
#define spl_port() spl1()
#define NOF_BMT	2
#define Clear_lock(l) l=0
#define MSG_SIZE 	8
#define F_MAX  		32			/* Fifo Size 		*/
#define P_MAX		(F_MAX/2)		/* Message Pool size 	*/


typedef struct {
	short	flag;
	short	lock;
	short	usr;
	short	bsr;
	SEMAPHORE sem;
	BMT_MESSAGE *first, *last;
	} BMT_PORT;

typedef struct {
	short	sig;
	long	val;
	} FIFO_ITEM;

typedef struct {
	short		flag;		/* Fifo Flag		*/
	short		bmt_id;		/* Bitmap Identifier	*/
	SEMAPHORE 	sem;		/* Fifo	Semaphor	*/
	FIFO_ITEM 	*prod;		/* Producer Pointer	*/
	FIFO_ITEM 	*cons;		/* Consumer Pointer	*/
	FIFO_ITEM 	fifo[F_MAX];	/* Fifo Buffer		*/
	} BMT_FIFO;

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

#define EPOOL_SIZE 32
typedef struct event_queue
{
	short	lock;
	short	usr;
	short	bsr;
	short	flag;
	short	qflag;
	SEMAPHORE	pool_sem;
	BMT_FIFO	*pofifo;
	EVENT_MESSAGE	first;
	EVENT_MESSAGE	last;
	EVENT_MESSAGE	*free;
	EVENT_MESSAGE	pool[EPOOL_SIZE];
	} EVENT_QUEUE;

typedef	struct	
{
	Point	pos;		/* Mouse Position	*/
	short	keys;		/* Mouse Buttons	*/
	} WPS_BLOCK;
/* **************************************************************************
 * 	For each BMT there is  a variable of BMT_DES which is located in 
 *  	BMT memory.
 * **************************************************************************/

typedef	struct
	{
	CON_DEVICE con_reg;
	BMT_FIFO bmt_intr;		/* bmt interrupt input fifo 	*/
	BMT_FIFO unix_intr;		/* unix interrupt input fifo 	*/
	BMT_FIFO bmt_msg;		/* bmt message fifo		*/
	BMT_FIFO unix_msg;		/* unix message fifo		*/
	BMT_FIFO bmt_pool;		/* bmt message pool fifo	*/
	BMT_FIFO unix_pool;		/* unix message pool fifo	*/
	BMT_MESSAGE msg_pool[P_MAX];	/* Message Pool 		*/
	/********************* Event & Event Objects ************************/
	EVENT_QUEUE	queue;
	/* **************** WPS Objects *********************************/
	short	  wps_flag;		/* WPS Flag (1 - WPS is waiting */
	WPS_BLOCK wps_new;		/* Current WPS Record	*/
	WPS_BLOCK wps_old;		/* Previous WPS Record	*/
	/* **************** Long Messages Objects  **********************/
	LONG_BMT_MESSAGE long_msg;
} BMT_DES;


#endif




