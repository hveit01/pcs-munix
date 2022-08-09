/* FILE gdisys.h  VERSION 6.7  DATE 87/01/22  TIME 17:24:32 */

/************************************************************************
 * This header file is to be included for grafic primitives             *
 *                                                                      *
 * included by 'layer.h'                                                *
 *                                                                      *
 * patterned after 'Graphics in bitmap layers' Rob Pike, Bell Labs      *
 *                                                                      *
 *      who                     when            why                     *
 *                                                                      *
 *      Bernd Lind, PCS GmbH    April 84                                *
 ************************************************************************/

#ifdef CWS1
typedef unsigned long  Word;            /* Unit of storage              */
#else
typedef unsigned short Word;            /* Unit of storage              */
#endif

/******* tag defines for remote procedure call ***************************/
/* windows and control  */
#define SETBACKPAT      1
#define GETBOUNDRECT    2
#define CREATEWINDOW    3
#define DELETEWINDOW    4
#define UPFRONTWINDOW   5
#define MOVEWINDOW      6
#define SIZEWINDOW      7
#define GETWINDOW       8
#define SETWINDOW       9
#define WHICHWINDOW     10
#define WHICHCHANNEL    11
#define SETWINDOWID     12
#define DUMMY_MSG	14	/* Dummy Meassge        */

/* transformations      */
#define GETOUTTRANS     15
#define SETOUTTRANS     16
#define GETINTRANS      17
#define SETINTRANS      18

#define TEST_MSG	19	/* J.D Test only */
/* raster functions     */
#define SETBITBLTPAT    20
#define WINDOWTOBITMAP  21
#define BITMAPTOWINDOW  22
#define CREATEBITMAP    23
#define DELETEBITMAP    24
#define WTEXTURE        25      /* for g_texture        */
#define WBITBLT         26      /* for g_bitblt()               */
#define WBLT            27      /* for bzemul()                 */
#define WTEXT           28      /* for bzemul()                 */
#define LONG_REQ	29	/*J.D Test only */


/* line functions       */
#define GETLINEATTR     30
#define SETLINEATTR     31
#define WPOINT          32
#define WLINE           33
#define WARC            34
#define WELLARC         35
#define LONG_MSG	36	/* J.D */
#define WPOLY_LINE	37	/* J.d 	*/
#define USER_REQ	38	/* J.D */

#define EXIT_MSG        39

/* fill functions       */
#define SETFILLPAT      40
#define GETFILLATTR     41
#define SETFILLATTR     42
#define WSEED           43
#define WCIRCLE         44
#define WELLIPSE        45


#define GLOB_EVENT_ON	46
#define GLOB_EVENT_OFF	47
#define ACTIVATE_WINDOW	48
#define WPOLYGON        49

/* text functions       */
#define SETVT100SET     50              /* set vt100 character set      */
#define WSTRING         51
#define WCHARSIZE       52
#define FONTALLOC       53
#define FONTFREE        54
#define SETTERMFONT     55
#define LOADFONT        56

/* event functions      */
#define GETEVENTATTR    60
#define SETEVENTATTR    61

/* cursor functions     */
#define SETCURPAT       65
#define GETCURPAT       66
#define LINKCURSOR      67              /* link cursor to input device  */
#define UNLINKCURSOR    68              /* unlink cursor form idevice   */
#define GETCURATTR      69              /* set cursor attributes        */
#define SETCURATTR      70              /* set cursor attributes        */
#define SET_CUR_POS     71
#define GETLOC          72              /* returns locator pos & keys   */
#define CUR_HIDE        73
#define CUR_RESTORE     74
#define CUR_SELECT      75

#define TAB_SET         76
#define TAB_GET         77
#define MOUSE_SET       78
#define MOUSE_GET       79

/* menus                */
#define CREATEMENU      80      /* array of strings             */
#define GETITEMMENU     81      /* menu pointer, switches tracking off  */
#define TRACKMENU       82      /* menu pointer                 */
#define SHOWMENU        83      /* menu pointer and Point       */
#define HIDEMENU        84      /* menu pointer                 */
#define DELETEMENU      85      /* menu pointer                 */


/* miscellaneous        */
#define GETDEVICEATTR   90
#define FREEWINRES      91
#define DRAWBOX         92
#define GRAFBLOCK       93      /* blocks output of channel     */
#define GRAFUNBLOCK     94      /* flushes output of channel    */
#define SYNCHRONIZE     95      /* stop call ahead              */
#define LOCLMALLOC      96      /* get bytes locally            */
#define LOCMEMCPY       97      /* copy bytes down              */
#define GETWINDOWPTR    98
#define LOCFREE         99
#define MEM_SWAP        100
#define PUT_STREAM      101

/* for color device     */
#define KBD_MSG         105
#define MOUSE_MSG       106
#define TAB_MSG         107
#define TTY_MSG         108
#define MASK_MSG        109
#define COPYIN          110
#define COPYOUT         111
#define SETCOLTAB       112
#define GETCOLTAB       113
#define SWITCHCOLTAB    114
/*******end of tags for RPC ***********************************************/
/* additional tags start here   */
#define TOBACKWINDOW    120
#define HIDEWINDOW      121
#define SHOWWINDOW      122

#define WSEEDFILL       123
#define SETXATTR        124
#define SETTEXTATTR     125
#define GETTEXTATTR     126



/** bitblt's **/
/**     standard patterns       **/
#define NO_PATTERN      -1
#define FILL_PATTERN    0
#define BACK_PATTERN    1
#define WHITE_PATTERN   2
#define LIGHT_GREY      3
#define MIDDLE_GREY     4
#define DARK_GREY       5
#define BLACK_PATTERN   6
#define BITBLT_PATTERN  7


#define ARC     1
#define PIE     2
#define CHORD   3

#define local   0
#define host    1
#define dirty   2



/* Cursor States */
#define C_ONSCREEN 	0x1	/* Local State */
#define C_INHIBIT       0x2
#define C_VISIBLE       0x4	/* Local State */
#define C_WANTED        0x8     /* Local State */

/* COPYWINDOW flags     */
#define IN              0
#define OUT             1

/* Transformation Input Flags */
#define MOUSE_CUR	1
#define MOUSE_NCS	2
#define TAB_CUR		4
#define TAB_NCS		8
/************************************************************************/
/*                                                                      */
/*               Structures used by GDI Library                         */
/*                                                                      */
/************************************************************************/


typedef short location;                 /* virtual Bitmaps for B&W      */
					/* Rectangles are half-open,    */
					/* they contain the horiz. and  */
					/* vertical lines through       */
					/* 'origin', but not through    */
					/* corner                       */
typedef struct {
	Word *base;                     /* start of data                */
	unsigned short width;           /* width in words               */
	Rectangle rect;                 /* image rectangle              */
	location place;                 /* is it local or on the host   */
	short sizeofWord;               /* how many bytes in a Word 2/4 */
	short bitsPerPixel;             /* 1, 4, 8, ..                  */
} Bitmap, *p_Bitmap;
					/* A Bitmap extends an rectangle*/
					/* to the next word boundary    */


#define WORDS_IN_TEXT   16              /* this value is hardwired      */

typedef short Texture[WORDS_IN_TEXT];   /* is a 16x16 bit pattern       */

typedef struct  {
	Texture text;
} TextStruct;

typedef short   Code;   /* enumeration type removed, cause enum == int  */

/* type of 'func' parameter     */
/* the codes are so defined that a '1' is black.        */
/* there is a utility that switches the definition appropriately to     */
/* the screen definition (software inverse video)                       */

#define hollow          0
#define coloured        1
#define patterned       2
typedef short  InterStyle;

/** part of parameter list for point,line,..                            */
typedef struct {
	Code    func;                   /* boolean function code        */
	short   pixelSize;              /* how fat are pixels           */
	short   pixColour;              /* colour for pixels            */
	short   writeMask;              /* mask out pixels              */
} pointAttr;

typedef struct {
	LineAttr        lattr;          /* SAME ALIGNEMENT as FillAttr  */
	short		interiorStyle;	/* hollow, colored, hatched or
					   patterned			*/
	short		hatchIndex;	/* hatch type index (only 1-6)	*/
	short           fillColorIndex; /* interior color               */
	short           auxColorIndex;  /* auxilliary color             */
	Point           align;          /* Fill pattern alignment       */
	Pattern         *fillPat;       /* space for reference only     */
} fillAttr;

typedef struct
	{
	unsigned short  curFlag;        /* cursor change flags          */
	short           curType;        /* cross hairs or little hand   */
	Rectangle       clipRect;       /* cursor clip rect             */
	Rectangle       viewport;       /* cursor transform rectangle   */
	short           colorIndex;     /* cursor color                 */
	short           writeMode;      /* write mode for fill elements */
	unsigned short  writeMask;      /* not used on s/w bitmaps      */
	Point           hotspot;        /* cursor pattern hot spot      */
	unsigned short  visibility;     /* cursor visibility            */
	}
	curAttr;

typedef struct {                        /* window bounding rectangles   */
	Rectangle       orect;          /* outer bounding rectangle     */
	Rectangle       irect;          /* inner bounding rectangle     */
	short           bckgrndColIndex;/* background color of Window   */
	short           visibility;     /* window visibility            */
} BoundRect;

typedef struct {
					/* output transform rectangles  */
	Rectangle       extent;         /* NCS extent rectangle         */
	Rectangle       viewport;       /* LCS viewport rectangle       */
	Rectangle       cliprect;       /* output clipping rectangle    */
	short           scaleTransFlag; /* skip mapping extend->viewport*/
} OutTransRect;

typedef struct {
					/* input transform rectangles   */
	Rectangle       idcsext;        /* input device extent          */
	Rectangle       ncsvp;          /* input NCS viewport rectangle */
	short           scaleTransFlag; /* skip mapping extend->viewport*/
} InTransRect;

typedef struct {
	short multiplier;               /* how thick is a pat pixel     */
	short styleLength;              /* what's used from lineStyle   */
} XAttr;

/************************************************************************/
/*                                                                      */
/*               parameter list definitions                             */
/*                                                                      */
/************************************************************************/

typedef struct
{	unsigned short cnt;
	long *proc;
} Semaphore;

#define MSG_SIZE 	8
/* MESSAGE FLAGS	*/
#define REPLIED		0x01
#define TIME_OUT	0x02
#define NORELEASE       0x04
#define IN_QUEUE	0x08
#define LONG_SEND	0x10
#define LONG_REQUEST	0x20
#define LONG_ERROR	0x40


/* Long Message Buffer Flags	*/
#define BUFFER_FULL	0x01
#define PROD_WAITING	0x02
#define CONS_WAITING	0x04
#define BUFFER_FREE	0x10
#define BUFFER_WANTED	0x20

#define PPORT		PZERO

typedef struct msg {
	struct	msg	*link;
	Semaphore	reply;
	struct	msg	*sender;
	short		flag;
	char		id[2];
	long		data[MSG_SIZE];
	} BMT_MESSAGE;
	
typedef struct msg_header
{
	struct	msg_header *link;
	Semaphore reply;
	struct msg_header 	*sender;
	short	flag;
	char	id[2];
	} MSG_HEADER;


#define BUFF_SIZE	1024	/* Long Message Size */
#define SRC_IND		0	/* Index to the source pointer 		*/
#define DST_IND		1	/* Index to the destination pointer 	*/
#define SIZE_IND	2	/* Index to the size of long message 	*/
#define DATA_IND	3	/* Index to the first data 		*/
typedef struct {
	short	flag;
	short	size;
	long	cnt;
	short	time;		/* Client  Priority	*/
#ifndef CWS1
	char	*src;
	char	*dst;
	Semaphore s_sem;
	Semaphore r_sem;
#else
	long 	data[BUFF_SIZE];
#endif
} LONG_BMT_MESSAGE;

#define POLY_IND	DST_IND
#define MAX_POLY_BUF	(BUFF_SIZE / sizeof(Point))
#define MSG_IND	DATA_IND
#define MAX_MSG_BUFF  ((4 * BUFF_SIZE) / sizeof(BMT_MESSAGE))

typedef struct {
	MSG_HEADER	head;
	long		data[3];
	LineAttr	lAttr;
}polylinePar;

typedef struct {
	MSG_HEADER head;
	DeviceAttr dAttr;
} devPar;

typedef struct {
	MSG_HEADER      head;
	Bitmap          *lbitmap;
	char            *lbase;
	Rectangle       rect;
	short           bitsPerPixel;
} createBitmapPar;

typedef struct {
	MSG_HEADER head;
	Point p;                /* pixel to draw                        */
	pointAttr ptAttr;       /* attribute structure                  */
} pointPar;


/** lines       **/
typedef struct {
	MSG_HEADER head;
	Point pt1;              /* first pixel of the line              */
	Point pt2;              /* second pixel of the line             */
	LineAttr lAttr;         /* attribute structure                  */
} linePar;


typedef struct  {
	MSG_HEADER head;
	Point  middlePt;        /* middle point of the circle           */
	short  radius;          /* radius of the circle                 */
} circlePar;

typedef struct  {
	MSG_HEADER head;
	Point  middlePt;        /* middle point of the ellipse          */
	Point  halfAxes;        /* in x/y direction                     */
} ellPar;

typedef struct {
	MSG_HEADER head;
	Point middlePt;         /* middle point of full circle          */
	short  radius;          /* radius of the circle                 */
	Point startPt;          /* start point of arc                   */
	Point endPt;            /* end point of arc                     */
	short arc_type;         /* what function (arc, pie,chord)       */
} arcPar;

typedef struct {
	MSG_HEADER head;
	Point middlePt;         /* middle point of full circle          */
	Point halfAxes;         /* in x/y direction                     */
	Point startPt;          /* start point of arc                   */
	Point endPt;            /* end point of arc                     */
	short arc_type;         /* what function (arc, pie,chord)       */
				/* 18 bytes     */
} ellarcPar;

typedef struct {
	MSG_HEADER head;
	long   srcId;           /* can be all : index, (Layer *), ..    */
	Rectangle sRect;        /* source Rectangle                     */
	Point  dOrig;           /* origin of destination Rectangle      */
	Code   func;            /* boolean function code (16 possible)  */
	short  fillColIndex;    /* foreground of pattern                */
	short  auxColIndex;     /* background of pattern                */
	long   patId;           /* can be all: index, (TextStruct *)..  */
	short  writeMask;       /* which planes of dMap can be touched  */
	Point  mOrig;           /* for mask bitmap                      */
} bitbltPar;

typedef struct  {
	MSG_HEADER head;
	Rectangle dRect;        /* destination Rectangle                */
	Point bitpos;           /* alignement of pattern with dRect     */
	Code func;              /* boolean function code                */
	short  fillColIndex;    /* foreground of pattern                */
	short  auxColIndex;     /* background of pattern                */
	long   patId;           /* can be all: index, (TextStruct *)..  */
	short  writeMask;       /* which planes in dMap can be touched  */
} texturePar;

typedef struct  {
	MSG_HEADER head;
	long    winId;          /* return value (-1) if failed          */
	Point   delta;          /* new offset from Window origin        */
} sizeWindowPar;

typedef struct  {
	MSG_HEADER head;
	long    winId;          /* return value (-1) if failed          */
	Point   newOrig;        /* new origin of Window origin          */
} moveWindowPar;

typedef struct  {
	MSG_HEADER head;
	long    winId;          /* return value (-1) if failed          */
	Rectangle outRect;      /* outer Rectangle on the screen        */
	Rectangle inRect;       /* for e.g the terminal emulation       */
	long  bitmapId;         /* in which bitmap is that window       */
	short visibility;       /* Window on screen or not              */
} modifyWindowPar;

typedef struct {
	MSG_HEADER head;
	Point retPt;            /* return value of wstring      */
	short setDot;           /* set current dot or not (FALSE/TRUE)  */
	Point pt;               /* starting point       */
	Code  func;             /* function code        */
	short font;             /* fontid               */
	short nchar;            /* number of characters */
	char  chars[16];        /* rest are chars       */
} wstringPar;

typedef struct {
	MSG_HEADER	head;	/* Message header */
	Point retPt;            /* returns baseline and left offset     */
	short fontid;           /* which font                           */
	char  c;                /* the sizes of this character interest */
} wcharsizePar;

typedef struct {
	MSG_HEADER      head;   /* Message header       */
	Point   *vertexTab;     /* table of vertices    */
	short   number;         /* number of vertices   */
	Code    func;           /* XOR,..               */
	short   patId;          /* fillPattern          */
	short   writeMask;      /* bitplanes            */
	Point   p1;
	Point   p2;             /* if little points     */
	Point   p3;
	Point   p4;
	Point   p5;
} polygonPar;

typedef struct {
	MSG_HEADER      head;   /* Message header       */
	XAttr           xattr;  /* extensions for X     */
} xPar;

typedef struct {                /* only local/server use*/
	Point   left;           /* begin of line        */
	Point   right;          /* end point            */
	short   pat;            /* the fill pattern     */
	Code    func;           /* function code        */
	short   fillColorIndex; /* fore/background color*/
	short   auxColorIndex;
	short   writeMask;      /* which planes to touch*/
} fillALinePar;

/************************** menus ***************************************/
typedef struct  {
	MSG_HEADER head;
	char *retval;           /* returned Menu                        */
	char *pItems;           /* the strings to show                  */
	short nchar;            /* number of char's                     */
	short fontid;           /* in what font                         */
} createMenuPar;

typedef struct  {
	MSG_HEADER head;
	short menuId;           /* which menu to present                */
	Point screenPt;         /* .. and where on the screen           */
} showMenuPar;

typedef struct  {
	MSG_HEADER head;
	short retval;           /* which Item selected                  */
	short menuId;           /* what menu please                     */
	Point screenPt;         /* is point inside menu and where there */
} getItemMenuPar;               /* or outside                           */

/*****************************************************************************/
typedef struct {
	MSG_HEADER      head;           /* Message header       */
	short           type;           /* Cursor Id.           */
	short           flag;           /* Cursor mode          */
	Point           pos;            /* Cursor Position      */
	Rectangle       clipRect;       /* Clipping Rect.       */
	} Cursor_msg;

/* Tablet Flags */
#define M_LINK          00002
#define M_DISCRETE      00001
typedef struct {
	MSG_HEADER      head;   /* Message header */
	short           flag;   /* M_DISCRETE/M_LINK */
	Point           devsize;/* Table Size (max x & y)  */
	Point           position;/* Tablet Position (only read )  */
	short           buttons;/* Tablet Position (only read )  */
	} Mouse_msg;

/*****************************************************************************/
/* Mouse Flags */
#define T_IDLE          00001
#define T_LINK          00002
typedef struct {
	MSG_HEADER      head;   /* Message header */
	short           flag;   /* T_IDLE/T_LINK */
	Point           devsize;/* Table Size (max x & y)  */
	Point           position;/* Tablet Position (only read )  */
	} Tablet_msg;
/* ************************************************************************/
typedef struct {
	MSG_HEADER	head;	/* Message header */
	short		mode;	/* Keyboard  mode.     */
	short 		flag;	/* Keyboard flag  */
	short		dev;	/* Keyboard type */
	short		code;	/* Last key pressed. */
	short		event;	/* Last event        */
	unsigned long   time;   /* Time of the last key. */
	unsigned long   repTime;/* Software autorepeat */

	} Keyboard_msg;


typedef struct  {
	MSG_HEADER head;
	long   retval;         /* returned value                       */
} fontAllocPar;

typedef struct  {
	MSG_HEADER head;
	short   retval;         /* returned value                       */
	Point   screenPt;       /* which point to intersect with layers */
} whichWindowPar;

typedef struct {
	MSG_HEADER head;
	Rectangle drawRect;
} drawBoxPar;

/** standard Bitmap identifier  **/
#define NO_MAP          -1
#define DISPLAY_MAP     0
#define SCRATCH_MAP     1
/*********************** grafic calls for WINDOWS **************************/

#define NO_WINDOW -1

typedef struct {
	MSG_HEADER head;
	Point pt;
	short borderColor;
} wseedPar;

typedef struct          {
	MSG_HEADER head;
	char *retval;
	short cursorId;
} getCursadrPar;

/*************************** miscellaneous ******************************/
typedef struct  {
	MSG_HEADER head;
	char *retval;
	long size;
} loclmallocPar;

typedef struct  {       /* not local    */
	char *pSrc;
	char *pDest;
	long number;
} locmemcpyPar;

typedef struct
	{
	MSG_HEADER      head;           /* Message Header               */
	short           device;         /* input device                 */
	}
	linkPar;

typedef struct
	{
	MSG_HEADER      head;           /* Message Header               */
	short           charset;        /* characterset 0 ASCII, 1 GERMA*/
	}
	charsetPar;

typedef struct
	{
	MSG_HEADER      head;           /* Message Header               */
	short           device;         /* input device                 */
	Point           position;       /* device position              */
	unsigned short  keys;           /* device keys                  */
	}
	locPar;

typedef struct
	{
	short      what;
	long       message;
	long       when;
	Point      where;
	}
	EventRecord;

typedef struct
	{
	MSG_HEADER      head;           /* Message Header               */
	short   retval;
	unsigned short fgrndcol;
	unsigned short bckgrndcol;
	char *fontfile;                 /* in server adress space       */
	}
	loadFontPar;

typedef struct
	{
	MSG_HEADER      head;           /* Message Header               */
	char    *p;                     /* src/destination Pointer      */
	short   cnt;                    /* number of bytes in message   */
	char    data[26];               /* maximum are 26 bytes         */
	}
	copyPar;

/************************************************************************/
#define swap_in         0
#define swap_out        1
#define delete_mem      2
typedef short SWAP_CMD;

typedef struct {
	MSG_HEADER	head;	/* Message header */
	SWAP_CMD 	cmd;	/* swap_in or swap_out */
	short		bid;	/* memory block id     */
	short           ackn;   /* acknowledge flag    */
	unsigned long   size;   /* size of memory block */
	char		*addr;	/* source address of memory block */
	} swapmessage;
/* ********************* IOCTL MIOCOPYIN - MIOCCOPYOUT *********************/
typedef struct {
	unsigned long   size;   /* size of memory block */
	char		*saddr;	/* source address of memory block */
	char		*daddr;	/* destination address of memory block */
	} swapPar;

#define MAXMAPS         128             /* maximum number of structs    */



/* ======================================================================*/
#define MAX_KBD_BUF	((MSG_SIZE<<2)-6)
typedef struct {
	MSG_HEADER	head;
	long		time;
	short		cnt;
	char		buf[MAX_KBD_BUF];
	} Kbd_msg;
		

typedef struct {
	MSG_HEADER	head;
	Point		pos;
	long		time;
	short		buttons;
	short		event;
	short		window;
	} LocatorMsg;


#define RED     0
#define GREEN   1
#define BLUE    2

typedef struct {
	MSG_HEADER      head;
	short           entryNo;
	short           color;
	unsigned char   ctab[16];
} CltMsg;

typedef struct {
	short	cmd;	/* tab_ioctl commands */
	short	arg;
} Tab_cmd;

/* FILE mouse.h  VERSION 1.1  DATE 86/06/30  TIME 16:10:31 */

/*  "/usr/include/sys/mouse.h  bz  Fri 09 Nov 84  09:56";
 *
 * Cadmus Bitmap Controller ioctl() calls
 */
#define mIOC		('m'<<8)	/* mouse IOCTL call		*/
#define MIOCMOUSE	(mIOC|1)	/* enable  mouse I/O		*/
#define MIOCGETPOSC	(mIOC|2)	/* get clipped mouse position	*/
#define MIOCSETPOSC	(mIOC|3)	/* set clipped mouse position	*/
#define MIOCGETPOSW	(mIOC|4)	/* get wrapped mouse position	*/
#define MIOCSETPOSW	(mIOC|5)	/* set wrapped mouse position	*/
#define MIOCTIMEOUT	(mIOC|6)	/* set timeout interval		*/
#define  MIOC_MAP1      (mIOC|7)        /*  see dev/bip.c  */


/* J.Duplinsky  - Event Commands */
#define  MIOCSTART      (mIOC|10)         /* for mxt()       */
#define  SERVER_ON	(mIOC|10)
#define  GET_EVENT      (mIOC|11)
#define  FLUSH_GET      (mIOC|12)
#define  EVENT_AVAIL    (mIOC|13)
#define  ACTIVE_WINDOW  (mIOC|14)
#define  SERVER_BASE	(mIOC|15)
#define  POST_EVENT     (mIOC|17)
#define  FLUSH_EVENT    (mIOC|18)
#define  COLTAB_CMD     (mIOC|19)
#define  MOUSE_CMD    	(mIOC|20) 
#define  TAB_CMD    	(mIOC|21)
#define  TTY_CMD        (mIOC|22)
#define  KBD_CMD    	(mIOC|23)

/* J. Duplinsky */
#define  MIOCLRECEIVE	(mIOC|8)        /*  Long Receive      */
#define  MIOCLSEND	(mIOC|24)       /*  Long Send      */
#define  MIOCSEND       (mIOC|25)       /*  see new bip.c  */
#define  MIOCRECEIVE    (mIOC|26)       /*  see new bip.c  */
#define  MIOCREPLY      (mIOC|27)        /*  see new bip.c  */
#define  MIOCWAIT       (mIOC|28)        /*  see new bip.c  */
#define  MIOCRPC        (mIOC|29)        /*  see new bip.c  */

#define  MIOC_MAP1DEF   (mIOC|30)        /*  see new bip.c  */
#define  MIOCOPYIN     (mIOC|31)         /*  see new bip.c  */
#define  MIOCOPYOUT    (mIOC|32)         /*  see new bip.c  */
#define  MIOCRESET     (mIOC|33)         /* bip.c, BL       */
#define  MIOCRMWON     (mIOC|34)         /* bip.c, BL       */
#define  MIOCRMWOUT    (mIOC|35)         /* bip.c, BL       */
#define  MIOCCHANON    (mIOC|36)         /* bip.c, BL       */
#define  MIOCCHANOFF   (mIOC|37)         /* bip.c, BL       */
#define  MIOCCNTREG    (mIOC|38)         /* write creg of bmtc  */
#define  MIOCEND       (mIOC|40)         /* for mxt()       */

/*
 * Structure sent to set/get position calls (clipped and wrapped):
 *	short pos[M_POS_SIZE];
 *
 * Constants for constructing/dissecting the structure.
 */
#define M_POS_SIZE	3		/* # short ints	*/
#define M_X		0		/* x coordinate	*/
#define M_Y		1		/* y coordinate	*/
#define M_K		2		/* key state	*/
/*
 * pos[M_K] & M_1	means key (1 = leftmost) is depressed. 
 */
#define M_3  01
#define M_2  02
#define M_1  04

/* ******************* Button Identifiers ********************************/
#define R_BUTTON 01
#define M_BUTTON 02
#define L_BUTTON 04
/*
 * various maxima
 */
#define M_QMAX		  07		/* maximum M_QUANTUM		*/
#define M_QDFL		  01		/* default M_QUANTUM		*/
#define M_TMAX		 256		/* maximum timeout (1/10ths)	*/

/*
 * flags passed to MIOCMOUSE call:
 */
#define	M_CURSOR	0001		/* display local cursor		*/
#define M_THREE		0002		/* enable right  mouse key	*/
#define M_TWO		0004		/* enable middle mouse key	*/
#define	M_ONE		0010		/* enable left   mouse key	*/
#define M_ALL_3		(M_ONE|M_TWO|M_THREE)
#define M_MOTION	0020		/* enable motion tracking	*/
#define M_WRAPPED	0100		/* wrapped (x,y) else clipped	*/
#define M_QUANTUM(x)  ((x&M_QMAX)<<8)	/* settable motion delta	*/
#define M_NONE		0000		/* no mouse I/O			*/


/*
 * Codes returned for mouse key hits; excepting M_timeout
 * each is immediately followed by 4 byte (x,y).
 * The 4 bytes are written "raw", and may be decoded by:
 *
#define  Mgetxy( X, Y )                 \
	{ struct { short x, y; } xy;    \
	read( 0, &xy, sizeof xy );      \
	X = xy.x;                       \
	Y = xy.y;                       \
	}
 */
#define M_XY_SIZE	  06		/* # bytes of position info	*/
#define M_1_up		0x82		/* left   key went up		*/
#define M_2_up		0x81		/* middle key went up		*/
#define M_3_up		0x80		/* right  key went up		*/
#define M_1_down	0x86		/* left   key went down		*/
#define M_2_down	0x85		/* middle key went down		*/
#define M_3_down	0x84		/* right  key went down		*/
#define M_motion	0x83		/* mouse was moved		*/
#define M_timeout	0x87		/* timeout, no I/O in 0.1 sec.	*/
#define M_lowchar	0x80		/* lowest mouse char		*/
#define M_hichar	0x87		/* highest mouse char		*/







