/* @(#)psl.h	6.1 */
/*
 *  	processor status
 */

#define	PS_C	0x1		/* carry bit */
#define	PS_V	0x2		/* overflow bit */
#define	PS_Z	0x4		/* zero bit */
#define	PS_N	0x8		/* negative bit */
#define PS_T1   0x8000          /* trace enable bit */
#define PS_T0   0x4000          /* trace enable bit */
#define PS_S    0x2000          /* system mode bit */
#define PS_IPL  0x0700          /* interrupt priority level */

/* esr bits */

#define ESRMASK       7
#define MEGAPAGEFAULT 1

#define PARITYFAULT   0x0100
#define TIMOFAULT     0x0008

/* pcr bits */
#define PCR_RUN         0x0001  /* normal run state if 1, boot state if 0 */
#define PCR_CLK         0x0004  /* start clock */
#define PCR_IENAB       0x0008  /* interrupt system enable */
#define PCR_LOGPHYS     0x0010  /* mmu enable */
#define PCR_PCONF       0x0020  /* generate P_BUS CONF signal */
#define PCR_HALT        0x0100  /* BHALT active */
