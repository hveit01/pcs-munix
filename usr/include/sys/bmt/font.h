/* FILE font.h  VERSION 8.1  DATE 87/05/22  TIME 13:39:55 */

/****************** new font header for berkeley fonts ******************/

#define BFONTCNT 256

typedef struct FONTINFO {
	struct header *phd;             /* berkeley font header         */
	struct dispatch *pch;           /* start of dispatch-table      */
	char   *pbits;                  /* start of bit array           */
	unsigned char firstChar;        /* start character              */
	unsigned char lastChar;         /* last char (for subsets       */
	short baseline;                 /* y offset from char box       */
	Layer font_lay;                 /* desription of bits           */
	short font_type;                /* fix or proportional spacing  */
	short colorIndex;               /* foreground color             */
	short boxColorIndex;            /* background color             */
	struct FONTINFO *bw_font;       /* fetch here the unexp. bits   */
} FontInfo;

#define FIXFONT  0
#define PROPFONT 1
#define CURSORID  31


typedef struct {
	unsigned short size;
	short maxx;
	short maxy;
	short xtend;
	unsigned char firstChar;
	unsigned char lastChar;
} fixfont;


