
/* command values */
#define READBUFREADY    0               /* announce read buf facility      */
#define TRANSMIT        1
#define INIT_LANCE      2
#define CLOSE           3
#define STATISTIC       4
#define RECEIVE_LANCE   5
#define QUICKXMIT       6
#define PRINT_LANCE     7

#define HOSTBUFNO       64              /* max. number of buffers on host  */
#define NUM_TRANS_BUFS  16              /* # of transmit buffers           */
#define FREE_RECV_BUFS  4               /* # of receiver buffers not used for read aheads */

#define ICC_LANCE_HBSZ  1520            /* buffer size                     */

extern  char    hostbuf[][ICC_LANCE_HBSZ]; /* buffers for ethernet packets */
extern  short   icc_lance_hbno;         /* # of buffers                    */

struct freebuf
{  char  *rbufphysadr;
   short bufno;
};



struct lance_stat
{  long missed_packet;                  /* occurences of missed packets,   */
					/* due to lack of receive buffers  */
   long transmitretryfailed;            /* occurences of transmitter jams  */
};                                      /* after 16 retries                */



struct icc_lance_request
{  struct rtk_header rtkhdr;
   short cmdtype;                       /* type of message                 */
   union
   {  /* case READBUFREADY of */
      struct freebuf rec;               /* address and id of host buffer   */

      /* case QUICKXMIT of */
      /* case TRANSMIT of  */
      struct
      {  char           *tbufphysadr;   /* address of host buffer (source) */
	 short          tbufno;         /* id of the host buffer           */
	 unsigned short tsize;          /* size of packet                  */
	 struct icc_lance_request *tmsg;/* id of request at the host site  */
	 int            (*xdone)();     /* routine to be called after      */
					/* termination at the host site    */
	 short          tseq;           /* sequence number                 */
	 short          terr;           /* error return                    */
      } trans;

      /* case INIT_LANCE of */
      struct
      { enetaddr etheraddress;          /* physical ethernet addr. for init*/
      } init;

      /* case RECEIVE_LANCE of */
      /* case CLOSE         of */
      struct
      {  short  err;                    /* error return for receive funct. */
	 short  rbno;                   /* host buffer id                  */
	 short  rlength;                /* length of received packet       */
      } reply;

      /* case STATISTIC of */
      struct lance_stat reply_stat;     /* statistic data                  */

      /* case PRINT_LANCE of */
      long bitmap;

      short dummy [(ICC_NOFPAR*2) - 1]; /* to allign the size of the msg   */

   } lance_un;
}; /* icc_lance_request */



