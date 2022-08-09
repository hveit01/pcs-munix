/************************************************************************
*                             L A N C E                                 *
*                                                                       *
* Some constants required for the lance processes                       *
* change history:                                                       *
*       28/04/86 jh     original version                                *
************************************************************************/

#define MAX_PACKETS     90      /* must be > NRRING + "something" */
#define NTRING          32      /* number of transmit ring descriptors */
#define NRRING          64      /* same for receive */
#define NTRINGLD        5       /* ld(NTRING) */
#define NRRINGLD        6       /* ld(NRRING) */
