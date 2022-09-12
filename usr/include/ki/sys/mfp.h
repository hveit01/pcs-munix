
/*	SccsId	@(#)mfp.h	1.2	7/27/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/15/88  HEAD/SYS:mfp";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     Versionsanpassung
*/

/*
 *
 *	Definitions/declarations for MC68901 Multi-Function Peripheral
 *
 */


/* MC68901 interrupt vector register (VA) definitions */
#define SOFTEOI  0x08     /* end-of-interrupt mode bit	 */
#define UVECTOR  0xf0     /* user-supplied vector      	 */



/* MC68901 interrupt control definitions (A registers) */
#define ITIMERB  0x01            /* timer B                       */
#define ITXERR   0x02            /* transmit error                */
#define ITXRDY   0x04            /* transmitter buffer empty      */
#define IRXERR   0x08            /* receive error                 */
#define IRXRDY   0x10            /* receiver buffer full          */
#define ITIMERA  0x20            /* timer A                       */
#define IGPIP6   0x40            /* gpip6			 */
#define IGPIP7   0x80            /* gpip7			 */


/* MC68901 interrupt control definitions (B registers) */
#define	IGPIP0		0x01		/* gpip0			 */
#define IGPIP1		0x02		/* gpip1			 */
#define IGPIP2		0x04		/* gpip2			 */
#define IGPIP3		0x08		/* gpip3			 */
#define ITIMERD		0x10		/* timer D			 */
#define ITIMERC		0x20		/* timer C			 */
#define IGPIP4		0x40		/* gpip4			 */
#define IGPIP5		0x80		/* gpip5			 */

/* MC68901 definitions for GPIP, AER, DDR */
#define GPIP0		0x01		/* gpip0			 */
#define GPIP1		0x02		/* gpip1			 */
#define GPIP2		0x04		/* gpip2			 */
#define GPIP3		0x08		/* gpip3			 */
#define GPIP4		0x10		/* gpip4			 */
#define GPIP5		0x20		/* gpip5			 */
#define GPIP6		0x40		/* gpip6			 */
#define GPIP7		0x80		/* gpip7			 */

/* MC68901 timers A and B mode definitions */
#define ABDIV4             0x01    /* delay mode, divide by 04  prescaler  */ 
#define ABDIV10            0x02    /* delay mode, divide by 10  prescaler  */ 
#define ABDIV16            0x03    /* delay mode, divide by 16  prescaler  */ 
#define ABDIV50            0x04    /* delay mode, divide by 50  prescaler  */ 
#define ABDIV64            0x05    /* delay mode, divide by 64  prescaler  */ 
#define ABDIV100           0x06    /* delay mode, divide by 100 prescaler  */ 
#define ABDIV200           0x07    /* delay mode, divide by 200 prescaler  */ 
#define EVNTCNT		   0x08	   /* event count mode			   */
#define PWDIV4		   0x09	   /* pulse width mode, divide by 04 ps    */
#define PWDIV10		   0x0a	   /* pulse width mode, divide by 10 ps    */
#define PWDIV16		   0x0b	   /* pulse width mode, divide by 16 ps    */
#define PWDIV50		   0x0c	   /* pulse width mode, divide by 50 ps    */
#define PWDIV64		   0x0d	   /* pulse width mode, divide by 64 ps    */
#define PWDIV100	   0x0e	   /* pulse width mode, divide by 100 ps   */
#define PWDIV200	   0x0f	   /* pulse width mode, divide by 200 ps   */
#define RESETABO	   0x10	   /* reset A or B output lines		   */
#define TMABMODE	   0x0f    /* timers A and B mode bits             */

/* MC68901 timer C mode definitions */
#define CDIV4              0x10    /* delay mode, divide by 04  prescaler  */ 
#define CDIV10             0x20    /* delay mode, divide by 10  prescaler  */ 
#define CDIV16             0x30    /* delay mode, divide by 16  prescaler  */ 
#define CDIV50             0x40    /* delay mode, divide by 50  prescaler  */ 
#define CDIV64             0x50    /* delay mode, divide by 64  prescaler  */ 
#define CDIV100            0x60    /* delay mode, divide by 100 prescaler  */ 
#define CDIV200            0x70    /* delay mode, divide by 200 prescaler  */ 
#define TMCMODE            0x70    /* timer C mode bits                    */

/* MC68901 timer D mode definitions */
#define DDIV4              0x01    /* delay mode, divide by 04  prescaler  */ 
#define DDIV10             0x02    /* delay mode, divide by 10  prescaler  */ 
#define DDIV16             0x03    /* delay mode, divide by 16  prescaler  */ 
#define DDIV50             0x04    /* delay mode, divide by 50  prescaler  */ 
#define DDIV64             0x05    /* delay mode, divide by 64  prescaler  */ 
#define DDIV100            0x06    /* delay mode, divide by 100 prescaler  */ 
#define DDIV200            0x07    /* delay mode, divide by 200 prescaler  */ 
#define TMDMODE            0x07    /* timer D mode bits                    */

/* MC68901 USART control register (UCR) definitions */
#define NULLREG         0x01	/* not used 				   */
#define PAREVEN         0x02	/* even parity 				   */
#define PARENAB         0x04	/* parity enable 			   */
#define SYNCH           0x00    /* synchronous mode (bits 3 and 4 = 0) 	   */
#define ASYNST1         0x08    /* asynchronous - 1   stop bit  	   */
#define ASYNST15        0x10    /* asynchronous - 1.5 stop bits 	   */
#define ASYNST2         0x18    /* asynchronous - 2   stop bits 	   */
#define WORDLEN8        0x00    /* 8 bits per word 			   */
#define WORDLEN7        0x20    /* 7 bits per word 			   */
#define WORDLEN6        0x40    /* 6 bits per word 			   */
#define WORDLEN5        0x60    /* 5 bits per word 			   */
#define CLOCK16         0x80    /* clock divided by 16 			   */

/* MC68901 receiver status register (RSR) definitions */
#define RXENAB          0x01    /* receiver enable 			   */
#define SYNCSTRP        0x02    /* synchronous strip enable 		   */
#define WORDASM         0x04    /* match/character in progress 		   */
#define RXBREAK         0x08    /* break detected 			   */
#define FRAMERR         0x10    /* frame error 				   */
#define PARERR          0x20    /* parity error 			   */
#define OVERERR         0x40    /* overrun error 			   */
#define RXRDY           0x80    /* receive buffer full 			   */

/* MC68901 transmitter status register (TSR) definitions */
#define TXENAB          0x01    /* transmitter enable 			   */
#define TXOUTHZ         0x00    /* disabled tx output - high impedance 	   */
#define TXOUTLO         0x02    /* disabled tx output - low 		   */
#define TXOUTHI         0x04    /* disabled tx output - high 		   */
#define TXOUTLP         0x06    /* disabled tx output - loopback 	   */
#define TXBREAK         0x08    /* transmit break 			   */
#define TXEOT           0x10    /* end of transmission 			   */
#define TXAUTO          0x20    /* auto turnaround 			   */
#define UNDERR          0x40    /* underrun error 			   */
#define TXRDY           0x80    /* transmit buffer empty 		   */ 

/*
 *      MC68901 Register Allocation    
 */

struct mfp_io
       {
       unsigned char   gpip;        /* 00 general-purpose i/o port   */
       unsigned char   aer;         /* 01 active edge register       */
       unsigned char   ddr;         /* 02 data direction register    */
       unsigned char   iera;        /* 03 interrupt enable reg A     */
       unsigned char   ierb;        /* 04 interrupt enable reg B     */
       unsigned char   ipra;        /* 05 interrupt pending reg A    */
       unsigned char   iprb;        /* 06 interrupt pending reg B    */
       unsigned char   isra;        /* 07 interrupt in-service reg A */
       unsigned char   isrb;        /* 08 interrupt in-service reg B */
       unsigned char   imra;        /* 09 interrupt mask register A  */
       unsigned char   imrb;        /* 0A interrupt mask register B  */
       unsigned char   vr;          /* 0B interrupt vector register  */
       unsigned char   tacr;        /* 0C timer A control register   */
       unsigned char   tbcr;        /* 0D timer B control register   */
       unsigned char   tcdcr;       /* 0E timers C and D control reg */
       unsigned char   tadr;        /* 0F timer A data register      */ 
       unsigned char   tbdr;        /* 10 timer B data register      */ 
       unsigned char   tcdr;        /* 11 timer C data register      */ 
       unsigned char   tddr;        /* 12 timer D data register      */ 
       unsigned char   scr;         /* 13 synchronous character reg  */
       unsigned char   ucr;         /* 14 USART control register     */
       unsigned char   rsr;         /* 15 receiver status register   */
       unsigned char   tsr;         /* 16 transmitter status reg     */
       unsigned char   udr;         /* 17 USART data register        */
};


/************************************************************************/
/*	LAP DEFINITONS							*/
/************************************************************************/
/* MC68901 Timer D Data Register	*/
#define B200	0xc0		/*  150 Baud 				*/
#define B300	0x60		/*  300 Baud 				*/
#define B600	0x30		/*  600 Baud		 		*/
#define B1200	0x18		/* 1200 Baud 				*/
#define B2400	0x0c		/* 2400 Baud 				*/
#define B4800	0x06		/* 4800 Baud 				*/
#define B9600	0x03		/* 9600 Baud 				*/
