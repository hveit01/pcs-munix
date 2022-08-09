/*	Command 	*/

#define SCC_OPEN	1
#define SCC_CLOSE	2
#define SCC_IOCTL	3
#define SCC_DATA	4
#define SCC_TCOM	5
#define SCC_PROC	6

/*     Interrupt flags 	*/

#define R_INTR	0	/* Receive interrupt */
#define S_INTR	1	/* Special interrupt */
#define E_INTR	2	/* Error interrupt */

#define SCC_ACK	6
#define SCC_POOL_SIZE 16
