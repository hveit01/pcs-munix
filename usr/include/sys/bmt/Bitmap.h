/* FILE Bitmap.h  VERSION 6.8  DATE 87/04/21  TIME 13:28:19 */

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
 *      BL                      Dec 85          constants for 68020     *
 *      BL                      21. 4. 86       graphic attributes      *
 *      BL                      10. 6. 86       enum's removed          *
 ************************************************************************/

#ifdef CWS1
#define BITS_PER_WORD   32              /* need this for screen layout  */
#define WORD_SHIFT      5
#define WORD_MASK       0xffffffe0
#define PIX_MASK        0x0000001f
#define BITS_PER_PIXEL  4               /* == colour !!!!!!!            */
#define NO_OF_COLOR     16
#define COLOR_MASK      0xf

#else   /* 16 bits wide words   */
#define BITS_PER_WORD   16              /* need this for screen layout  */
#define WORD_SHIFT      4
#define WORD_MASK       0xfff0
#define PIX_MASK        0x000f
#define BITS_PER_PIXEL  1               /* B&W                          */
#define NO_OF_COLOR     2
#define COLOR_MASK      0x1
#endif

/** additional information for line drawing on the stack        */
/*  (line drawing must be reentrant because of the cursor)      */

typedef struct {
	short dx;
	short dy;
	Point beginPt;
	Point shiftDelta;
	short number;
	Point updatePt;         /* determines quadrant                  */
	short inc1;
	short inc2;
	short error;
	Point endPt;            /* for fill algorithms (polygon ..)     */
	Point curPt;            /* current point                        */
	short multiplier;       /* multiplier for lineStyle     */
	short styleLength;      /* how much bits are used       */
} lineGlobals;


typedef struct {
	pointPar ptpar;         /* common for all point routines        */
	Bitmap *dMap;           /* destination Bitmap                   */
	Rectangle clipRect;     /* so it's an attribute free call       */
} bpointPar;

typedef struct {
	linePar lpar;           /* common for all line routines         */
	Rectangle clipRect;     /* so it's an attribute free call       */
} blinePar;

typedef struct {
	circlePar cpar;         /* 'normal' parameters                  */
	fillAttr *pAttr;        /* attribute structure                  */
	Bitmap *dMap;           /* destination Bitmap                   */
	Rectangle clipRect;     /* so it's an attribute free call       */
} bcirclePar;

typedef struct {
	arcPar apar;            /* 'normal' parameters                  */
	LineAttr *pAttr;        /* attribute structure                  */
	Bitmap *dMap;           /* destination Bitmap                   */
	Rectangle clipRect;
} barcPar;

typedef struct {
	fillALinePar    fpar;
	Bitmap  *dMap;
} bfillALinePar;


/* aliases      */
typedef barcPar bpiePar;
typedef barcPar bchordPar;

typedef struct  {
	texturePar tpar;
	Bitmap *dMap;
} btexturePar;

#define NO_OF_CODES  4          /* how many function codes are generic  */

/* ERRORS */
#define NO_ERR          0
#define NO_RECT         -1
#define WRONG_CODE      -2
#define TEXT_INVAL      -3

#define NIL             0L              /* set that in for pattern if   */
					/* no third operand supplied    */

#define BLT_M68000      0
#define BLT_M68010      1

/* TOGGLE TEXTURING OPCODE */
#define AND_TEXT        0
#define OR_TEXT         1

/* write masks  */
#define ONE_MASK 0xffffffff

/*----------------------------------------------------------------------*/
/* SOME NOTES:                                                          */
/* ADRESS MAPPING:                                                      */
/*      From the host:                       (executing == ON_THE_HOST) */
/*      The user must supply the base adress of the installed boards.   */
/*      This adress should stem from a 'ioctl' of the bitmap-driver.    */
/*      The base adress is one of the parameters of blt_conf().         */
/*      blt_conf() sets map-register 0 to the program-ram at 0x40000    */
/*      and map register 1 to the video-memory with control register 0, */
/*      that is adress 0x80000. So from the host there seem to be a     */
/*      contiguous adress space from (base_adress = 0) to (base_adress  */
/*      + 0x40000).                                                     */
/*                                                                      */
/*      Local:                                     (executing == LOCAL) */
/*      There are two configurations, whether the EPROM is installed or */
/*      not. But this doesn't affect bitblt(), cause the adresses of    */
/*      bitmaps are user-supplied (hidden in the bitmap-structure) and  */
/*      under the user's responsibility, and the I/O-adresses are con-  */
/*      stant in both configurations.                                   */
/*                                                                      */
/*      Warning:                                                        */
/*      Both the map-registers and the pixel-proc control registers are */
/*      write-only registers, so there's no chance to save the old con- */
/*      tent. bitblt() assumes that creg0 contains 3f00 (RD_NOOP), so   */
/*      that it can read from the video-memory without destroying       */
/*      data.                                                           */
/*      blt_conf() puts that value into the creg0. This should be no    */
/*      problem, because no other program may use the pixel-hardware    */
/*      any more, so that there's no need to change the control-regs.   */
/*      Harder to solve is the problem with the map-registers. The      */
/*      only clear solution with bitblt() is, as i can see, to down-    */
/*      load the routine and use it locally (with much performance im-  */
/*      provement).                                                     */
/*      But: Bitmaps are then reqired to reside on the board, cause     */
/*      for the local processor exists no possibility to reach data     */
/*      outside the local memory.                                       */
/*                                                                      */
/* PROCESSOR-TYPE:                                                      */
/*      A member of the 68k-family is recommended, because bitblt()     */
/*      is implemented as a code-generator. This code resides on the    */
/*      stack. If you execute it on the host, you must therefore exe-   */
/*      cute an egress(2) system-call before the first call to bitblt().*/
/*      (only with MUNIX 1.5 and less)                                  */
/*      The difference between the 68000 and the 68010 is made, because */
/*      the 68010 has a two instruction deep program-cache, which per-  */
/*      mits it to execute a two opcode tight loop without code-fetches,*/
/*      so that the whole memory bandwith is used to move data, whereas */
/*      the loop for the 68000 is enrolled twice (less branches).       */
/*                                                                      */
/* PIXEL-PROCESSOR:                                                     */
/*      Currently only the PCS-BMTC is supported.                       */
/*----------------------------------------------------------------------*/

