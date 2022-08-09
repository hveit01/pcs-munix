/*	@(#)mon.h	1.6	*/
struct hdr {
	char	*lpc;
	char	*hpc;
	short   nfns;
	short   scale;
};

struct cnt {
	char	*fnpc;
	long	mcnt;
};

typedef unsigned short WORD;

#define MON_OUT	"mon.out"
#define MPROGS0 (600 * sizeof(WORD))    /* 300 for pdp11, 600 for 32-bits */
#ifndef NULL
#define NULL    ((char *)0)
#endif
