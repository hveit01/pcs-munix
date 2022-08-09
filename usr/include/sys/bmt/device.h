/* FILE device.h  VERSION 6.1  DATE 86/11/28  TIME 10:47:22 */

/* ========================= device counts BL ***************************/
#define NCBMT	2
#define MAXBIP  8       /* maximum number of BMT in the system  */
			/* if changed, ALL sources of kernel    */
			/* which deal with Bitmaps, have to     */
			/* be changed                           */

/* =========================================*/
#define	CHERRY 1	/* keyboard */
#define	VIDEO  2	/* Tablet */
#define	VG2    2	/* Tablet */
#define	PREH   3	/* Keyboard PREH */
/********************* KEYBOARD ****************************************/
/*	Flags */
#define KF_KEYDOWN 	1
#define KF_IGNORE  	2
#define KF_VTTY		4 
#define KF_AUTOREPEAT	8 
#define KF_APPLICATION	0x10 
#define KF_EVENT	0x20
#define KF_LOCAL	0x40

/* Ioctl Command */
#define KC_CHECK	1
#define KC_KEYUP 	2
#define KC_NORMAL	3
#define KC_RESET 	4
#define KC_VT100 	5
#define KC_EVENT 	6
#define KC_GIN		7
#define KC_KEYPAD_APP  	8
#define KC_KEYPAD_NOR 	9
#define KC_ON_LINE 	10
#define KC_LOCAL 	11
#define KC_DEVICE 	12
#define KC_BELL 	13
#define KC_AUTOREP_ON 	14
#define KC_AUTOREP_OFF 	15
#define KC_SET_LED 	16
#define KC_RESET_LED 	17

/* mode */
#define KM_IDLE		0
#define KM_TAB10	1
#define KM_VT100	2
#define KM_EVENT	3
#define KM_GIN		4

#define KEY_UP_MASK	0x80
#define BELL		0x7

#define LEFT_LED	('1')
#define MIDDLE_LED	('2')
#define RIGHT_LED	('3')

/************************* TABLET *************************************/

/**** Ioctl commands   *****/
#define TC_EVENT	1
#define TC_SPEED 	2
#define TC_FLUSH 	3
#define TC_POINT 	4
#define TC_STREAM 	5
#define TC_SWITCH 	6
#define TC_STOP 	7
#define TC_GIN		8
#define TC_TRANSFORM	9
#define TC_RESET	11
#define TC_GETLOC	12
#define TC_LINK		13
#define TC_DISCRETE	14

/* Table modes  */

#define TM_IDLE 	1
#define TM_TAB10	2
#define TM_TEK4014	3
#define TM_TABLET	4

/**** Flags ****/
#define T_IDLE		00001
#define T_LINK 		00002
#define T_PEN_DOWN 	00004
#define T_PEN_WAS_DOWN	00010
#define T_GIN		00020
#define T_KEY_DOWN	00040
#define T_KEY_UP	00100
/********      CMD	*************/
#define T_PRESENCE	00001


/*  TEK4014 Constants */
#define MouseRight	0xf1
#define MouseMid	0xf2
#define MouseLeft	0xf4

#define T_TIMEOUT	20	/* T_TIMEOUT x 15 Ms. */

/* Mouse modes */
#define MM_IDLE		1
#define MM_GIN		2
#define MM_NORMAL	4


/**** Mouse Local Flags */
#define MF_IN_RECT	0x8000
#define MF_GIN		0x4000

/* Mouse Commands */
#define MC_LINK		1
#define MC_TRANSFORM	2
#define MC_GETLOC	3
#define MC_SETLOC	4
#define MC_RESET	5
#define MC_DISCRETE	6
#define MC_GIN		7
#define MC_SETPOS	8
/*************************** Constant Definition *************************/
#define SCC_PRIO INT_LEV6
#define SCCA	0	/* SCC Channel A  1. SCC */
#define SCCB	1	/* SCC Channel B  1. SCC */
#define SCCC	2	/* SCC Channel A  2. SCC */
#define SCCD	3	/* SCC Channel B  2. SCC */


#ifdef	CWS1
#define TAB_CHANNEL	SCCB
#define KBD_CHANNEL	SCCA
#define TICK_SLICE	17	/* ~ 16,6 Ms. */
#else
#define TAB_CHANNEL	SCCA
#define KBD_CHANNEL	SCCB
#define TICK_SLICE	2	/* ~ 2 Ms. */
#endif
#define NOFSCC	4
/******************************* Scc_ioctl commands ************************/
#define SCC_SPEED	1
#define SCC_PARITY	2
#define SCC_EMPTY	3
#define SCC_RESET	4
#define SCC_GET		5
#define SCC_PUT		6
#define SCC_IPROC	7
#define SCC_OPROC	8
#define SCC_EPROC	9
#define SCC_SPROC	10
#define SCC_FLUSH	11
#define SCC_STATE	12

#define SCC_ERROR	0x30 /*(Rx Overrun | Parity )*/
#define scc_empty(dev) (scc_ioctl(dev,SCC_EMPTY))

/* B50 .. B9600 can be defined in termio.h */
#ifndef B50
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14	/* With 5 MHz Clock ->19531 Bd  critical !!! */
#define B38400  15	/* With 5 MHz Clock ->38400 Bd  critical !!! */
#endif

#define KEYBOARD_RESET	0x7fff	/* This code reset keyboard to VT100 Mode */


typedef struct 
{
	short	flag;
	short	mode;		/* Mouse mode */
	short	window;
	short	event;
	short	buttons;	/* Current state of mouse buttons */
	short	oldbuttons;	/* Previous state of mouse buttons */
	short	prev_reg;	/* Previous Mouse register value */
	EventMask mask;		/* Current Event mask */
	long    time;           /* Mouse update time */
	Point	curpos;		/* Current position of the mouse */
	Point	oldpos;		/* Previous position of the mouse */
	Point	delta;
	Point	curdelta;
	Rectangle devicerect;
	Rectangle cliprect;
	Rectangle cagerect;
} MOUSE;


typedef struct 
{
	short	flag;		/* tablet flag */
	short	mode;		/* Mode of operation */
	short	event;
	Point	curpos;		/* Current Pen position */
	Point	oldpos;		/* Previous Pen position */
	Point	delta;		/* delta x-y */
	Point	deltapos;	/* Last delta x-y */
	short	buttons;	/* Tablet buttons	*/
	short	window;		/* Window id.		*/
	unsigned long time;	/* Time of last the message */
	EventMask mask;		/* Current Event Mask */
	Rectangle cliprect;	/* Tablet clipping rectangle. */
	Rectangle devrect;	/* Tablet Working rectangle. */
	Rectangle cagerect;	/* Tablet Working rectangle. */
} TABLET;


#define CNTRL_C	0x3		/* Control & c */
typedef struct keyboard 
{
	unsigned char 	lastkey;	/* last pressed key */
	unsigned char 	prevkey;	/* previeous pressed key */
	short	code;		/* Internal Code of last character */
	short	mode;
	short	flag;
	short	event;		/* last keyboard event */
	long	time;		/* time of the last pressed key */
} KEYBOARD;

typedef struct 
{
	long	cmd;
	long	retval;
} KBD_DRIVER;

