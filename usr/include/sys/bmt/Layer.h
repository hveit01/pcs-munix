/* FILE Layer.h  VERSION 6.2  DATE 87/01/22  TIME 17:22:58 */

/**************************************************************************
 *
 *
 * These are type definitions and data declarations for Blit's window-system
 *
 *
 *      who     when            why
 *
 *      BL      apr 84          nothing interesting to do
 *      BL      sometimes       updates
 *      BL      14 mar 86       reference count and magic number for LAYER
 ************************************************************************/

#define LAY_MAGIC       0x2468

typedef int boolean;                    /* looks better                 */


typedef long misc;                      /* pointer to unknown arguments */
					/* destroy type-checking        */

/* The Layers are chained together in order from 'front' to 'back' on the  */
/* screen                                                                  */

typedef struct  LAYER   {
	Bitmap screenMap;               /* so always compatible         */
					/* with Bitmap struct           */
	ListHeader  obsHeader;          /* linked list of obscured rect */
	struct LAYER *next;             /* layer in front/behind        */
	struct LAYER *prev;             /* layer in front/behind        */
	short  ref_cnt;                 /* how many references to it    */
	short  layMagic;                /* test, if it is a layer       */
} Layer;

/* The obscured-list is doubly linked (no order), each element contains a  */
/* bitmap for storing the off-screen  image and a pointer to the frontmost */
/* layer that obscures it .The screen coordinates of the ob-               */
/* scured rectangle are held in the   'rect'-field of the 'Bitmap-struc-   */
/* ture pointed to by the 'Obscured- list entry                            */

typedef struct  OBSCURED        {
	ListElement obsList;            /* chaining of obscured elements*/
	Layer *lobs;                    /* frontmost obscuring Layer    */
	Bitmap *bmap;                   /* where the obscured data resid*/
} Obscured;


typedef struct {
	pointPar ptpar;         /* common for all point routines        */
	Layer *dLay;            /* destination Layer                    */
	Rectangle clipRect;     /* so it's an attribute free call       */
} lpointPar;

/* circle parameters    */
typedef struct {
	circlePar cpar;         /* 'normal' parameters                  */
	LineAttr *pAttr;        /* attribute structure                  */
	Layer *dLay;            /* destination Layer                    */
	Rectangle clipRect;     /* so it's an attribute free call       */
} lcirclePar;

typedef struct {
	arcPar apar;            /* 'normal' parameters                  */
	LineAttr *pAttr;        /* attribute structure                  */
	Layer *dLay;            /* destination Layer                    */
	Rectangle clipRect;     /* so it's an attribute free call       */
} larcPar;

typedef struct {
	ellarcPar apar;         /* 'normal' parameters                  */
	fillAttr *pAttr;        /* attribute structure                  */
	Layer *dLay;            /* destination Layer                    */
	Rectangle *pclipRect;
	TextStruct *fillPat;    /* filled figures                       */
} lellarcPar;

typedef struct {
	fillALinePar    fpar;
	Layer  *dLay;
} lfillALinePar;

typedef struct  {
	texturePar tpar;
	Layer *dLay;
} ltexturePar;

typedef struct  {
	polygonPar fpar;
	Layer *dLay;
} lpolygonPar;

