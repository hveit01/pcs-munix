/*
 * The structures header and dispatch define the format of a font file.
 *
 * See vfont(5)  and below  for more details.
 */
struct header {
	short magic;
	unsigned short size;
	short maxx;
	short maxy;
	short xtend;
}; 
#define  Fontmagic      0x11e /*  orig Berkeley: rows 8 16 24 ... bits,  */
#define  Fontmagic_even 0x11f /*  fonteven for bip:     16  32 48 ...
			      /*  also has 128 letter headers,  orig 256  */
#define  is_Font_even( hd )  ((hd).magic == Fontmagic_even)

struct dispatch {
	unsigned short addr;
	short nbytes;
	char up,down,left,right;
	short width;
};


/****************************************************************************
	for example:
vfontinfo -v -z0 /usr/lib/vfont/R.6  '='
Font /usr/lib/vfont/R.6, raster size 1941, max width 64, max height 40, xtend 24

 ASCII     offset    size  left    right   up     down    width 
  3d  =      2ab     12     -2     12      8     -2      13
[][][][][][][][][][]
[][][][][][][][][][]
                    
                    
                    
[][][][][][][][][][]

shows the 6 x 10 pixel block containing '='
	  up+down = 6,  left+right = 10  -- see vfont(5)
To save space in the font files, 0 bits around these 6 x 10 are not stored;
instead the block is shifted up and right
relative to a 'baseline' and letter 'virtual origin'.
Thus the '=' pixels are shifted so the top left pixel is up 8, right 2:

8     | [][][][][][][][][][]
7     | [][][][][][][][][][]
6     |
5     |
4     |
3     | [][][][][][][][][][]
2     |
1 ____1_2_________________111213_
-1                      width 13 is where the next letter should start.

The bits themselves are stored at offset 2ab + a0a in the font file:
	ffc0
	0000
	0000
	ffc0
	ffc0
i.e.  xd R.6 +0xcb5 is:
     cb0             ffc0ff  c0000000 000000ff  I                I
     cc0  c0c00060 0038000e  00030001 c003800e  I   ` 8          I

If the font file is 'evenword' (PCS standard),
each letter row is a multiple of 16 bits long.
This wastes a little space but make Dot_putc
(unified font putc for bitmap, lbp ...) faster.
The font file then starts with Fontmagic_even.

( Fontread( "R", 9, Fontread_word );
	makes the font even if need be: see <bip/font.h>

	denis bzowy     Fri Jul  6 12:25 1984
**/

typedef struct
      {
      struct header   fontHdr;        /* font information block       */
      struct dispatch charHdr[256];   /* character information blocks */
      }
      VFontInfo;

