/* FILE Window.h  VERSION 6.2  DATE 87/02/09  TIME 15:31:32 */

/***************************************************************************
 *	Window	structure 
 *
 *      who     when            why
 *      JD      ancient times   vt100 emulation
 *      BL      less ancient t. graphic attributes
 *      BL/KTA  23. 4. 86       tek emulation
 *************************************************************************/

/** general purpose structures **/

typedef	struct	{ short	x, y, z; }		ZPoint;
typedef	struct	{ short	a,b,c, d,e,f, g,h,i; }	Matrix;

/* parameter typedefs only for local procedures */

typedef struct {
	linePar lpar;
	long dummy;
	Rectangle dummyRect;
} wlinePar;

typedef struct {
	pointPar ppar;
	long dummy;
	Rectangle dummyRect;
} wpointPar;

/**					 **/
/** TekWin  & VecFont  structures follow **/
/**					 **/

typedef	struct	{
	/**				**/
	/**	STICK-FONTS		**/
	/**	    VECTOR-FONTS	**/
	/**		HERSHEY-FONTS	**/
	/**				**/
	/**	vector-font control	**/
	/**	block (VCB)		**/
	/**				**/
	ZPoint	scale;				/** char scaling **/
	ZPoint	spacing;			/** str  char  spacing **/

	ZPoint	fall;				/** char fall  angle in deg **/
	ZPoint	rotate;				/** str  rot   angle in deg **/
	ZPoint	slant;				/** str  slant angle in deg **/
	ZPoint	twirl;				/** char twirl angle in deg **/

	ZPoint	charadv;			/** character advance vector **/
	ZPoint	rowadv;				/** row advance vector **/
	ZPoint	pos;				/** present char. position **/
	ZPoint	margin;				/** present margin position **/

	Matrix	trans;		/**  3-D transformation matrix values **/

	ZPoint	perspective;	/** what factor of perspective present **/

	short	font;		/** font pointer definer **/
	short	size;		/** char size **/
	short	column;		/** column in line buffer **/

	wlinePar text;		/** text line attributes **/

	char	linebuf[160];	/** line buffer for characters **/
	short	*vectors[2], *seek[2];		/** pointer to vec.font data **/

}       VecFont;

typedef	struct	{
	/**			**/
	/**     TekWin          **/
	/**			**/
	/** the tektronics/4014 **/
	/** control block (TCB) **/
	/**			**/
	VecFont         stick;
	short		x, y, z;	/* Current Crosshair Cursor Position 	*/
	short		old_x, old_y, old_z;	/* Previous prosition		*/

	short		tek_state;	/* Tek4014 current state 	*/
	short		cur_state;	/* Alpha Cursor's state - on/off*/
	short		num_state;
	short		count;
	char		num_str[11];	/* Temporary buffer for ESC string */
	char		pendown;	/* Tablet Pen position		*/
	char		tab_mode;	/* Current tablet mode 		*/
	char		tab_header;	/* Next position header 	*/
	char		tab_char;	/* Last tablet character 	*/

	wlinePar line;		/** line attributes **/

}       TekWin;



#define MAX_ESCSEQ	16

typedef struct WINDOW    {
	/* linked list  */
	ListElement     winList;
	/* common part of Window structure                               */
	short   termMagic;      /* magic number for checks               */
	Layer   layer;          /* output layer                          */
	Bitmap  *pMap;          /* if invisible, for deleteBitmap()      */

	/* vt100 terminal part of window structure                       */
	short	flag;		/* Window flag                           */
	short	mode;		/* Window mode                           */
	short	savedflag;  	/* Saved flag                            */
	short   cursor_flag;    /* cursor off or in window               */
	Point	curpos;		/* Current cursor position in a window   */
	Point	savedpos;  	/* Saved cursor position                 */
	short	fontid; 	/* font id                               */
	FontInfo *font;         /* Pointer to current Font               */
	short	lineheight;	/* line height                           */
	short	charwidth;	/* character offset in window            */
	short	maxline;	/* max lines in a window                 */
	short	maxcol;		/* max. column in a window               */
	char	cntrlbuf[MAX_ESCSEQ]; 
				/* buffer for control esc. sequence 	 */
	short	cntrlin;	/* producer index into cntrlbuf          */
	short	cntrlout;	/* consumer index into cntrlbuf          */

	Code    bltfun;         /* Bitblt Function in Window             */
	Rectangle termRect;     /* Working Window                        */
	Rectangle scrollRect;   /* Current scrolling region              */

	/* for graphics in Windows      */
	BoundRect       boundrect;      /* transformation & windows     */
	InTransRect     intrans;
	OutTransRect    outtrans;
			/* Cursor context switch */
	curAttr		curattr;	/* Currsor Attribute Block */
	Pattern 	curPattern;
	Point		curPosition;	/* Cursor position */
	Point           curOpposite;    /* for rubbercursor*/
	Rectangle	dev1;		/* For the future use */
	Rectangle	dev2;		/* For the future use */
	short           devtransflag;
			/* Mouse context switch */
	short		mouseFlag;
	short		mouseMode;
	EventAttr	eventattr;	/* Event Attribute Block */
			/* Keyboard context switch */
	short		kbdFlag;
	short		kbdMode;
			/* Tablet context switch */
	short		tabFlag;
	short		tabMode;
	Pattern bckgrndPattern;
	Pattern bitbltPattern;  /* for filling rectangles and so on      */
	fillAttr fattr;         /* fill Attributes for closed curves     */
	Pattern  fillPattern;   /* for filling of closed curves          */
	LineAttr lattr;         /* needed for arc                        */
	TextAttr tattr;         /* needed for raster & stick fonts       */
	/* for the 4014 emulation                                        */
	TekWin  win4014;        /* Tektronics control Block (TCB)        */

	ListHeader resourceList;/* List header of resource list          */
	XAttr           xattr;  /* extensions for X Windows              */
} Window;


/* Window flags */
#define W_BOLD		 00001
#define W_UNDERSCORE	 00002
#define W_GSET		 00004
#define W_ASET		 00010
#define W_AUTOREPEAT     00020
#define W_WRAPAROUND     00040
#define W_REVERSE   	 01000
#define W_LINEFEED  	 02000
#define W_KEYPAD	 04000
#define W_ANSI		010000
#define W_BLINK		020000
#define W_DSET          040000          /* german font set      */

#define ESC		033


#define TERM_MAGIC      0x1234
#define GRAF_MAGIC      0x5678
/* Window mode */
#define W_VT100		1
#define W_TEK4014	2
#define W_TAB10		3


/* resource id's        */
#define MENU_RESRC      1
#define FONT_RESRC      2
#define MEM_RESRC       3

/* visibility           */
#define W_VISIBLE       0001
#define W_INVISIBLE     0002

Window *activeWin;


