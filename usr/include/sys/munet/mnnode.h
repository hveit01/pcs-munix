/* @(#) mnnode.h        1.1     Nov. 06 1986    */


/*
 * Include file for MUNIX/NET node infos
 *
 * Transmit access to the network is via a fixed number of node infos,
 * which are dynamically mapped to major/minor device pairs.
 *
 */

struct uinode {
	short nd_dev;                   /* dev of the node */
	enetaddr nd_etaddr;             /* Ethernet address */
	ipnetaddr nd_ipaddr;            /* IP address for the node */
	time_t nd_time;                 /* last time was there */
	ushort nd_flag;                 /* internal flag */
	short nd_vers;                  /* MUNIX/NET version for this node */
	time_t nd_timediff;             /* time difference to other systems in secs */
};


extern struct uinode uinode[];          /* the port table itself */

/*
 * Close and deallocate a MUNIX/NET node: this is simply a macro
 */
#define uiclosenode(nn)         uinode[nn].nd_dev = NODEV


/* supported MUNIX/NET versions */

#define WHAT_VERSION(nn)        uinode[nn].nd_vers
#define NOT_KNOWN       0       /* no information about node-version    */
#define OLD_VERSION     1       /* non-optimized MUNIX/NET              */
#define OPTIMIZED_NOFR  3       /* optimized MUNIX/NET                  */
#define OPTIMIZED_FRAG  4       /* optimized MUNIX/NET with fragmentation*/

#define MY_VERSION      OPTIMIZED_FRAG  /* my actual MUNIX/NET Version  */
