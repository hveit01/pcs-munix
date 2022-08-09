/* FILE gdi.h  VERSION 6.2  DATE 86/12/09  TIME 16:34:11 */

/*
* description:  Include File for GDI Application Programs               *
*/


/************************************************************************/
/*      constant definitions                                            */
/************************************************************************/

#define FALSE        0
#define TRUE         1

/*      Write Modes                                                     */

#define WHITE           0
#define AND             1
#define INV_D_AND       2
#define STORE           3
#define INV_S_AND       4
#define DEST            5
#define XOR             6
#define OR              7
#define NOR             8
#define IXOR            9
#define INV_D           10
#define INV_D_OR        11
#define INV_S           12
#define INV_S_OR        13
#define NAND            14
#define BLACK           15

/* some modifier bits of the function code for g_bitblt*()              */
#define S_ALIGNED_MASK  0x10            /* src aligned 3 op bitblt      */
#define D_ALIGNED_MASK  0x20            /* dest aligned " " (regions)   */
#define SCAN_REVERSE    0x40            /* fillByXorScanlines           */



/*      Line & Edge Styles                                              */

#define G_SOLID         0xffffffff      /* linepattern for solid lines  */
#define G_DASH          0xff00ff00      /* for dashed lines             */
#define G_DOT           0x88888888      /* for dotted lines             */
#define G_DASHDOT       0x83f083f0      /* for dash-dot   lines         */
#define G_DASHDOTDOT    0xf888f888      /* for dash-dot-dot  lines      */

/*      Interior Styles                                                 */

#define G_HOLLOW        0
#define G_COLORED       1
#define G_PATTERNED     2
#define G_HATCHED       3


/*      Hatch Indices                                                   */

#define G_HORIZONTAL    0               /* horizontal hatching          */
#define G_VERTICAL      1               /* vertical hatching            */
#define G_POSITIVE      2               /* 45 deg. positiv hatching     */
#define G_NEGATIVE      3               /* 45 deg. negativ hatching     */
#define G_CROSS         4               /* horizontal and vertical
					   hatching			*/
#define G_POSNEG        5               /* 45 deg. positiv and negativ
					   hatching			*/

/*      Font Types                                                      */

#define G_RASTER  0                     /* Raster font                  */
#define G_STICK   1                     /* Stick  font                  */

/*      Character Sets                                                  */

#define G_ANSI    0                     /* ANSI character set           */
#define G_GERMAN  1                     /* GERMAN character set         */

/*      Event Types                                                     */

#define NULLEVENT       0
#define LEFTDOWN        1
#define MIDDLEDOWN      2
#define RIGHTDOWN       3
#define LEFTUP          4
#define MIDDLEUP        5
#define RIGHTUP         6
#define KEYDOWN         7
#define KEYUP           8
#define ENTERRECT       9
#define LEAVERECT      10
#define ABORTEVENT     11
#define DELTAEVENT     12
#define PENDOWNEVENT   13
#define PENUPEVENT     14
#define TKEYDOWNEVENT  15
#define TKEYUPEVENT    16
#define KEYEVENT       17

/*      Event Masks                                                     */

#define NULLMASK               ((EventMask) (1L << NULLEVENT      ))
#define LDOWNMASK              ((EventMask) (1l << LEFTDOWN       ))
#define MDOWNMASK              ((EventMask) (1l << MIDDLEDOWN     ))
#define RDOWNMASK              ((EventMask) (1l << RIGHTDOWN      ))
#define LUPMASK                ((EventMask) (1l << LEFTUP         ))
#define MUPMASK                ((EventMask) (1l << MIDDLEUP       ))
#define RUPMASK                ((EventMask) (1l << RIGHTUP        ))
#define KEYDOWNMASK            ((EventMask) (1l << KEYDOWN        ))
#define KEYUPMASK              ((EventMask) (1l << KEYUP          ))
#define ENTERRECTMASK          ((EventMask) (1l << ENTERRECT      ))
#define LEAVERECTMASK          ((EventMask) (1l << LEAVERECT      ))
#define ABORTMASK              ((EventMask) (1l << ABORTEVENT     ))
#define DELTAMASK              ((EventMask) (1l << DELTAEVENT     ))
#define PENDOWNMASK            ((EventMask) (1l << PENDOWNEVENT   ))
#define PENUPMASK              ((EventMask) (1l << PENUPEVENT     ))
#define TKEYDOWNMASK           ((EventMask) (1l << TKEYDOWNEVENT  ))
#define TKEYUPMASK             ((EventMask) (1l << TKEYUPEVENT    ))
#define KEYMASK                ((EventMask) (1l << KEYEVENT       ))

#define ANYEVENT               ((EventMask) 0xffffffff)

/*      Cursor Attribut Flags                                           */

#define G_CURTYPE       0x0001          /* cursor type                  */
#define G_CURCLIP       0x0002          /* cursor clip rectangle        */
#define G_CURVIEW       0x0004          /* cursor clip viewport         */
#define G_CURCOLI       0x0008          /* cursor color index           */
#define G_CURWMASK      0x0010          /* cursor write mask            */
#define G_CURWMODE      0x0020          /* cursor write mode            */
#define G_CURPAT        0x0040          /* cursor pattern               */
#define G_CURHOTSP      0x0080          /* cursor hotspot               */
#define G_CURVIS        0x0100          /* cursor visibility            */

/*      Cursor Types                                                    */

#define G_ARROW         0               /* cursor type arrow            */
#define G_CROSSHAIR     1               /* cross hair cursor            */
#define G_USERCURSOR    2               /* user defined 16 x 16 bit
					   cursor			*/
#define G_RUBBAND       3               /* rubberband cursor            */
#define G_RUBMOVBOX     4               /* moving outline box           */
#define G_RUBFIXBOX     5               /* outline box with fix origin  */

/*      Event Time                                                      */

#define G_NOWAIT         0L              /* get_event does not wait      */
#define G_WAIT           -1L             /* get_event waits              */
#define G_TILL_NOW       -1L             /* get_event waits              */

/*      Cursor & Window Visibility                                      */

#define G_INVISIBLE     0               /* cursor is invisible          */
#define G_VISIBLE       1               /* cursor is visible            */

/*      Input Devices                                                   */

#define G_KEYBRD  0
#define G_MOUSE   1
#define G_TABLET  2

/* device types */
#define G_NO_KEYBOARD 0                 /* no keyboard available        */
#define G_PREH        1                 /* PREH keyboard                */
#define G_CHERRYE     2                 /* CHERRY english keyboard      */
#define G_CHERRYG     3                 /* CHERRY german  keyboard      */

#define G_NO_TABLET   0                 /* no tablet available          */
#define G_VIDEOGRAPH1 1                 /* VIDEOGRAPH 1 tablet          */
#define G_VIDEOGRAPH2 2                 /* VIDEOGRAPH 2 tablet          */

#define G_BMTC1       1                 /* BMTC 1 display               */
#define G_BMTC2       2                 /* BMTC 2 display               */
#define G_CWS1        3                 /* color display                */

/************************************************************************/
/*      data structure definitions                                      */
/************************************************************************/


typedef short Fid;                      /* workstation file identifier  */
typedef short Font;                     /* Font identifier              */
typedef short Key;                      /* Key code                     */
typedef short MenuId;                   /* Menu identifier              */
typedef short MenItem;                  /* Menu item number             */
typedef short DevType;                  /* Input device type            */
typedef short BitmapId;                 /* User Bitmap Identifier       */

typedef unsigned long	EventMask;
typedef unsigned long	EventMessage;


typedef struct                          /* left upper corner of bitmap  */
	{
	short x, y;                     /* is (0,0), highest possible   */
	} Point;                        /* value is (2**15,2**15)       */


typedef struct
	{
	Point origin;                   /* min x, y                     */
	Point corner;                   /* max x, y                     */
	} Rectangle;

typedef struct                          /* fillPattern & texture        */
	{
	unsigned short pattern[16];
	}
	Pattern;

typedef struct
	{
	char rtab[16];
	char gtab[16];
	char btab[16];
	}
	ColTab;

typedef struct
	{
	Point           align;          /* Pattern alignment            */
	Pattern         pattern;        /* Bitmap fill pattern          */
	short           fillColorIndex; /* interior color               */
	short           auxColorIndex;  /* auxilliary color             */
	short           writeMode;      /* write mode for text elements */
	unsigned short  writeMask;      /* not used on s/w bitmaps      */
	}
	BitbltAttr;

typedef struct
	{
	unsigned long   lineStyle;      /* 32 bit line pattern          */
	short           lineWidth;      /* width symmetric to thin line */
	short           colorIndex;     /* color of every drawn pixel   */
	short           writeMode;      /* boolean function code        */
	unsigned short  writeMask;      /* enable/disable bit planes    */
	}
	LineAttr;


typedef struct
	{
	unsigned long   edgeStyle;      /* border style of fill elements*/
	short		edgeWidth;	/* border width of fill elements*/
	short           edgeColorIndex; /* border color of fill elements*/
	short           writeMode;      /* write mode for fill elements */
	unsigned short  writeMask;      /* not used on s/w bitmaps      */
	short		interiorStyle;	/* hollow, colored, hatched or
					   patterned			*/
	short		hatchIndex;	/* hatch type index (only 1-6)	*/
	short           fillColorIndex; /* interior color               */
	short           auxColorIndex;  /* auxilliary color             */
	Point           align;          /* Fill pattern alignment       */
	Pattern         fillPattern;    /* 16 x 16 bit pattern          */
	}
	FillAttr;

typedef struct
	{
	short           fontType;       /* RASTER or STICK font         */
	Font            fontId;         /* downloaded fontid            */
	short           colorIndex;     /* text color                   */
	short           boxColorIndex;  /* character box color          */
	short           writeMode;      /* write mode for text elements */
	unsigned short  writeMask;      /* not used on s/w bitmaps      */
	}
	TextAttr;

typedef struct
	{
	Point           delta;          /* Delta Event delta            */
	Rectangle       cageRect;       /* Input device cage rectangle  */
	EventMask       mask;           /* Event mask                   */
	}
	EventAttr;

typedef struct
	{
	short       event;
	long        data;
	long        time;
	Point       position;
	}
	EventRec;

typedef struct
	{
	unsigned short  curFlag;        /* cursor change flags          */
	short           curType;        /* cross hairs or little hand   */
	Rectangle       clipRect;       /* cursor clip rect             */
	Rectangle       viewport;       /* cursor transform rectangle   */
	short           colorIndex;     /* cursor color                 */
	short           writeMode;      /* write mode for fill elements */
	unsigned short  writeMask;      /* not used on s/w bitmaps      */
	Pattern         pattern;        /* 16 x 16 bit structure for
					   user defined cursor		*/
	Point           hotspot;        /* cursor pattern hot spot      */
	unsigned short  visibility;     /* cursor visibility            */
	}
	CurAttr;

typedef struct
	{
	Point screenSize;               /* last pixel right down of scr.*/
	short bitsPerPixel;             /* number of bit-planes         */
	short bitsPerWord;              /* data bus size of device 16/32*/
	short keyboardType;             /* what keyboard attached       */
	short tabletType;               /* what tablet attached         */
	short displayType;              /* color or bw                  */
	short noColTab;                 /* number of color tables       */
	}
	DeviceAttr;

/************************************************************************/
/*      GDI function declarations                                       */
/************************************************************************/

Point g_string();

Fid g_create_window();

Point g_get_locator_pos();

Key g_get_locator_key();


/************************************************************************/
/*      extern variables declaration                                    */
/************************************************************************/

extern short gerrno;			/* gdi error variable		*/



