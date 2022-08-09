/*
 ****************************************************************
 *                                                              *
 *                                                              *
 *      HEADER FILE FOR MUNIX/NET BUFFER MANAGEMENT             *
 *                                                              *
 *                                                              *
 ****************************************************************
 */

/* Structure Declarations       */

struct  bufque                  /* buffer descriptor                    */
{
	ushort  bq_flag;
	ushort  bq_bufid;       /* buffer identifier ( e.g. hostbuf )   */
	ushort  bq_frsiz;       /* packet fragment size  (with headers) */
	caddr_t bq_pkptr;       /* address of packet header             */
	struct  rbq * bq_rbq;   /* receive buffer queue                 */
	struct  bufque *bq_nextbuf; /* pointer to next packet fragment  */
	struct  bufque *bq_nextque; /* pointer to next packet que       */
};

typedef struct bufque bufque;

/* codes for bq_flag            */

#define         BQ_ALLOC        0x01
#define         BQ_FREE         0x00
#define         BQ_SEND         0x10
#define         BQ_RCVE         0x20
#define         BQ_SENDOK       0x40

/* buffer management and reassembly constants   */

#define         NLANDEV         2       /* max. number of LAN devices   */
#define         MAXHOSTBUFNO    128     /* max. number of buffers per
					 * driver -> icc_lance.h !!!
					 */
#define         MAXHOSTBUFNOS   16      /* max. number of send buffers
					 * must equal with NUM_TRANS_BUFS
					 * -> icc_lance.h !!!
					 */
#define         FRTIME          (hz/2)  /* 1/2 sec ; should be chosen as
					 * TTL in the network
					 */
/* packet header sizes          */

#define         EIP_HEADER_SIZE    (E_HEADER_SIZE + IP_HEADER_SIZE)
#define         RT_HEADER_SIZE     (sizeof( struct dlpacket ) - \
				   EIP_HEADER_SIZE - UIMAXDATA_NEW_FRAG)
#define         MN_HEADER_SIZE     (sizeof( struct mnpacket ) - \
				   EIP_HEADER_SIZE - UIMAXDATA_NEW_FRAG)
#define         FR_HEADER_SIZE     UIMINPACKET

/* codes for u.u_segflg         */

#define         USERSEG         0
#define         KERNELSEG       1

/* codes for fragmentation - IP */

#define         IP_FMASK        0xf000  /* mask for more flag field     */
#define         IP_OMASK        0x0fff  /* mask for the offset field    */
#define         IP_FLAST        0x0000  /* last fragment flag           */
#define         IP_FMORE        0x2000  /* more fragments               */

				/* list of buffer queues holding fragments
				 * of incomplete packets for reception  */
struct  fragmentlist
{
	bufque  *fq_first ;     /* pointer to first packet que          */
	bufque  *fq_last ;      /* pointer to last packet que           */
	ushort  fq_count ;      /* number of queues in the list         */
};

typedef struct  fragmentlist    frque;

/* code  for fq_count           */

#define         FR_MAX          ((MAXHOSTBUFNO - MAXHOSTBUFNOS) / 4)

/* macros                       */

#define         INC_SSEQN(n)    if( ++n == 32767 ) n = 1
				/* increment ushort modulo 15           */
#define         min(a, b)       ( a > b ? b : a )


extern  int      buf_copy ();
extern  caddr_t  buf_get ();
extern  bufque  *buf_pointer ();
