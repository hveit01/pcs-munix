/*
 * bpdata.h -- describes communication between bootserver process on
 *             file server and unix kernel on diskless node
 */

#define BFC     30              /* number of characters for bootfile name */

struct bpdata {
	enetaddr bp_masterid;	/* master node's ethernet address */
	ipnetaddr bp_masterip;	/* master node's ip address */
	enetaddr bp_etname;	/* my ethernet address */
	ipnetaddr bp_ipname;	/* my ip address */
	char	bp_nodename[10];/* my node name */
	dev_t	bp_rootdev;	/* my root device */
	dev_t	bp_swapdev;	/* my swap device or NODEV to use master's */
	daddr_t	bp_nswap;	/* number of 1/2 k blocks in my swap area */
	daddr_t	bp_swplo;	/* starting block offset in my swap area */
	char    bootfile[BFC];  /* name of Unix kernel for autoboot */
};

extern struct bpdata bpdata[];

#define MAXDLNODES      64      /* max number of dlnodes */
