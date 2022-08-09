/***************************************************************************/
/*                                                                         */
/*                         DEFINE - BLOCK                                  */
/*                                                                         */
/***************************************************************************/

#define LANCE_ADDR      0xe00000

#define CSR0            0
#define CSR1            1
#define CSR2            2
#define CSR3            3


/*  Bits for CSR0       */

#define INIT    0x1             /* initialize bit                          */
#define STRT    0x2             /* start bit                               */
#define STOP    0x4             /* stop bit                                */
#define TDMD    0x8             /* transmit demand                         */
#define TXON    0x10            /* transmitter ON                          */
#define RXON    0x20            /* receiver ON                             */
#define INEA    0x40            /* interrupt enable                        */
#define INTR    0x80            /* interrupt flag                          */
#define IDON    0x100           /* initialization done                     */
#define TINT    0x200           /* transmitter interrupt                   */
#define RINT    0x400           /* receiver interrupt                      */
#define MERR    0x800           /* memory error                            */
#define MISS    0x1000          /* missed packet                           */
#define CERR    0x2000          /* collision error                         */
#define BABL    0x4000          /* BABBLE: transmitter timeout error       */
#define ERR     0x8000          /* error summary : BABL or CERR or MISS or */
				/*                 MERR                    */


/*  Bits for CSR3       */

#define BCON    0x1             /* byte control                            */
#define ACON    0x2             /* ALE control                             */
#define BSWP    0x4             /* byte swap: for buffer (silo) only       */


/* Initialization Block    */
/* Bits for operationsmode */

#define DRX     0x1             /* disable the receiver                    */
#define DTX     0x2             /* disable the transmitter                 */
#define LOOP    0x4             /* loopback                                */
#define DTCR    0x8             /* disable the transmit CRC                */
#define COLL    0x10            /* force collision                         */
#define DRTY    0x20            /* disable retry                           */
#define INTL    0x40            /* internal loopback                       */
#define PROM    0x8000          /* promiscuous mode: all incoming addresses*/
				/* are accepted                            */

/* Buffer Management                                                       */
/* Bits for receive message descriptor 1   ( RMD1 )                        */

#define ENP     0x100           /* end of packet                           */
#define STP     0x200           /* start of Packet                         */
#define BUFFR   0x400           /* buffer error                            */
#define CRC     0x800           /* CRC error                               */
#define OFLO    0x1000          /* overflow error                          */
#define FRAM    0x2000          /* framing error                           */
#define ERRR    0x4000          /* error sumary: FRAM | OFLO | CRC | BUFFR */
#define OWN     0x8000          /* descriptor entry is owned by lance      */


/* Bits for transmit message descriptor 1   ( TMD1 )                       */

/*#define ENP     0x100          * end of packet                           */
/*#define STP     0x200          * start of Packet                         */
#define DEF     0x400           /* deferred                                */
#define ONE     0x800           /* one retry                               */
#define MORE    0x1000          /* more retry                              */
#define ERRT    0x4000          /* error sumary: LCOL | LCAR | UFLO | RTRY */
/*#define OWN     0x8000         * descriptor entry is owned by lance      */


/* Bits for receive/transmit message descriptor 2   ( RMD2 or TMD2 )       */

#define ONES    0xf000


/* Bits for transmit message descriptor 3   ( TMD3 )                       */

#define TDR     0x200           /* time domain reflectometry               */
#define RTRY    0x400           /* retry error                             */
#define LCAR    0x800           /* loss of carrier                         */
#define LCOL    0x1000          /* late collision                          */
#define UFLO    0x4000          /* underflow error                         */
#define BUFFT   0x8000          /* buffer error                            */


#ifdef PIKITOR
#define NRBUF   2               /* # of receive buffers, must be power of 2*/
#define NRBUFLD 1               /* ld (NRBUF)                              */
#define NTBUF   1               /* # of transmit buffers, must be power of 2*/
#define NTBUFLD 0               /* ld (NTBUF)                              */
#else
#define NRBUF   64              /* # of receive buffers, must be power of 2*/
#define NRBUFLD 6               /* ld (NRBUF)                              */
#define NTBUF   8               /* # of transmit buffers, must be power of 2*/
				/* should be updated in icc_lance.c too !! */
#define NTBUFLD 3               /* ld (NTBUF)                              */
#endif

#define STK_LEN 1000L



/***************************************************************************/
/*                                                                         */
/*                         STRUCT - BLOCK                                  */
/*                                                                         */
/***************************************************************************/

struct lance_init_block
{  unsigned short mode;                 /* mode of operation               */
   unsigned short padr_hi,              /* physical address: 0..47, 0      */
		  padr_mi,
		  padr_lo;
   unsigned short ladrf[4];             /* logical address filter: 0..63   */
   unsigned short rdra[2];              /* receive descriptor ring address */
   unsigned short tdra[2];              /* transmit    "       "      "    */
};



struct ring_des
{  unsigned short ladr;                 /* low address (0..15) ot buffer   */
   unsigned short hadr;                 /* high address (16..23)      "    */
   short bcnt;                          /* buffer length                   */
   short mcnt;                          /* message in bytes                */
};                                      /* tdr for transmit message        */

#define tdr     mcnt


struct lance_regs
{  unsigned short rdp;                  /* register data port              */
   unsigned short rap;                  /* register address port           */
};
