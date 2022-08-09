/* FILE global.h  VERSION 8.1  DATE 87/05/22  TIME 13:42:02 */

/*--------------------------------------------------------------------
   global.h
	
	Global definitions used by several modules.

----------------------------------------------------------------------


/*** Macro definitions ********************************************************/

#define MAX(a,b)	(((a) > (b))? (a): (b))
#define MIN(a,b)	(((a) < (b))? (a): (b))
#define ABS(a)		((a) >= 0 ? (a) : -(a))

/*** Global Constants *********************************************************/
#undef NIL
#define	NIL		0L
#define LONG_ONES	0xffffffff
#define SHORT_ONES	0xffff
#define BYTE_ONES	0xff
#define MACFILE		3	/* file descriptor qsh sends to program */

/*** Type definitions *********************************************************/


typedef unsigned char	Byte;
typedef char		SignedByte;
typedef short		*address;
typedef long		(*ProcPtr)();	/* pointer to func returning long */
typedef char		*Ptr;

