/*                                                               19.06.85  *
 * "icc.h" is the global include file for all icc programs.                *
 * The intention is that all constant definitions used by more than one    *
 * independent module should  be concentrated in one file to prevent a     *
 * confusing amount of include files.					   *
 * Definitions only concerning one module should be hidden for other users *
 * in local include files.						   *
 * 									   *
 * Since this file is included by assembler sources too, definitions in C  *
 * syntax must be guarded by "#ifndef ASSEMBLER".			   *
 * 									   *
 * Note that "rtk.h" is included here (except for assembler sources with   *
 * a "#define ASSEMBLER" directive).			  		   *
 *									   */


#include "sys/icc/icc_flags.h"		/* message flags */
#ifndef ASSEMBLER
#include "sys/icc/rtk.h"		/* for message passing primitives */
#endif


/**************************** interrupt vector addresses **********************/
/*				( = vector << 2 )			      */
#define QBI_INTVEC	0x3b4	/* int.vector address for QBI-Error */
#define HOST_INTVEC	0x3bc	/* int.vector address Host->ICC	*/
#define LAN_INTVEC	0x3cc	/* int.vector address of LANCE contr. */
#define SCSI_INTVEC	0x3e8	/* int.vector address of SCSI bus */
#define SCC_INTVEC	0x300	/* int.vector address of serial commun.
				   controller SCC (loadable) */

#define POWF_INTVEC	0x3fc	/* int.vector address of Powerfail on Q-Bus */

#define DIS_INT		0x2500	/* PS interrupt mask for 'disabled interrupts'.
				   Do not use 'DISABLE' in ICC programs !   */
#define EN_INT		0x2100	/* PS interrupt mask for 'enabled interrupts'.
				   Do not use 'ENABLE' in ICC programs !    */
/*#define EN_INT	0x2000	/* PS interrupt mask for 'enabled interrupts'.
				   Do not use 'ENABLE' in ICC programs !    */


/**************************** host interrupt **********************************/

#define ICC0_INTLEV	4	/* host vector level for ICC 0	*/
#define ICC1_INTLEV	4	/* host vector level for ICC 1	*/
#define ICC2_INTLEV	4	/* host vector level for ICC 2	*/
#define ICC3_INTLEV	4	/* host vector level for ICC 3	*/

#define ICC0_INTVEC	0x200	/* host vector address for ICC 0 */
#define ICC1_INTVEC	0x208	/* host vector address for ICC 1 */
#define ICC2_INTVEC	0x210	/* host vector address for ICC 2 */
#define ICC3_INTVEC	0x218	/* host vector address for ICC 3 */


/**************************** software priorities *****************************/

#define MPP_PRIO	10
#define SCSI_PRIO	20
#define LANCE_PRIO	30


/************************** Q-Bus Interface (QBI) *****************************/

#ifndef ASSEMBLER
extern long Read_mem(), Write_mem(), Read_reg(), Write_reg();
extern long Read_mem_noi(), Write_mem_noi();
#endif

#define QBRES	((int *)0xe40000)  /* Q-Bus init. Write to this address causes
				      a reset on the host and the ICC !! */
#define QBICLR	((int *)0xe40002)  /* clear QB-Interface */
#define QBSTAT	0xe40002	   /* address of QBI status register */

#define WRVEC	0xe41000	   /* Q-Bus int.vector reg	*/

#define RDCR0	((int *)0xe42000)	/* hostregister 0 (read only) */
#define RDCR1	((int *)0xe42002)	/* hostregister 1	"     */
#define RDCR2	((int *)0xe42004)	/* hostregister 2	"     */
#define RDSINT	((int *)0xe42006)	/* ICC int.vector reg	"     */

#define DMAREG	0xe43000    /* reg for Q-bus DMA-address */

	/*----- control bits for DMAREG ------*/
	/*    CQ_xxxx                         */
	/*    ||  |______ name of Bit mask (according to hardware description */
	/*    ||_________ Q bus interface (QBI)				      */
	/*    |__________ C ontroll				      	      */

#define CQ_DMAIN	0x04	/* read-DMA bit	*/
#define CQ_IOPAGE	0x08	/* iopage-DMA bit */
#define CQ_WCSHIFT	4	/* shift count for wcnt	*/
#define CQ_BYTETR  	0x200	/* byte-DMA bit (command) */
#define CQ_QSIEN	0x400   /* QB-Interf. Int.Enable : end DMA cycle */

	/*----- status register bits (hardware) -----*/
	/*    SQ_xxxx                         */
	/*    ||  |______ name of Bit (according to hardware description */
	/*    ||_________ Q bus interface (QBI) 			 */
	/*    |__________ S tatus				         */

#define SQ_FEMPTY	1	/* FIFO empty */
#define SQ_FFULL	2	/* FIFO full */
#define SQ_QHALT	0x4	/* Q-Bus halt active */
#define SQ_BYTETR 	0x8	/* Byte-Transfer (status) */
#define SQ_DMABSY	0x40	/* Qbus-Interface busy with DMA transfer */
#define CQ_DMABSY_BIT 	6       /* bit postion in status word of DMABSY bit */
#define SQ_IP		0x80	/* Interrupt Pending */
#define CQ_IP_BIT 	7       /* bit postion in status word of IP bit */
#define SQ_QBIBSY	0xc0	/* Qbus-Interface busy with DMA or Interrupt
			           (SQ_DMABSY|SQ_IP) */

	/*------- ERROR MESSAGES (return value) from QBI routines -------*/
	/*------------------ error register bits (hardware) -------------*/
	/*    EQ_xxxx                                                    */
	/*    ||  |______ name of Bit (according to hardware description */
	/*    ||_________ Q bus interface (QBI) 			 */
	/*    |__________ E rror				         */

#define EQ_FUN	     0x100	/* FIFO underflow */
#define EQ_FOV	     0x200	/* FIFO overflow */
#define EQ_FIFCOL    0x400	/* FIFO I/O-collision */
#define EQ_NOBM	     0x800	/* No block mode mem */
#define CQ_NOBM_BIT  11         /* bit position in error register */
#define EQ_QBVIOL    0x1000	/* QBI violation */
#define CQ_QBVIOL_BIT  12       /* bit position in error register */
#define EQ_QTOUT     0x2000	/* Q-bus timeout */
#define CQ_QTOUT_BIT  13       /* bit position in error register */
#define EQ_QPERR     0x4000	/* Q-bus parity error */
#define EQ_QSERR     0x8000	/* Q-bus Interface sum-error */
#define CQ_QSERR_BIT 15         /* Q_bus sum-error bit position in error
				   register */


	/*------------------ error register bits (software) -------------*/
#define	EQ_QBI_ERR	0x80000000	/* error if sign bit set (negative
					   values)	*/
#define	EQ_ODD_ADDR	0x80010000	/* odd source or destination address */
#define EQ_SEQ_TOUT	0x80020000	/* QBI-sequencer timeout error */


/********************* Message passing primitives ****************************/

#ifndef ASSEMBLER
typedef struct _message
{	struct _message *link;
	SEMAPHORE reply;
	long sender;
	short flag ;
	char id[2];
	long data[ICC_NOFPAR];
} MESSAGE;

typedef struct rtk_header
{	struct _message *link;
	SEMAPHORE reply;
	long sender;
	short flag ;
	char id[2];
} RTK_HEADER;

typedef struct
{	MESSAGE *first, *last;
	SEMAPHORE notempty;
} PORT;

#define POOL_SIZE	32
#define NOWAIT	0L
#define WAIT	-1L

#define Init_message(msg)	(msg)->link = NULL
#define Reply(msg)		Signal(&(msg)->reply)

extern MESSAGE *Receive();
extern MESSAGE *Request_msg();
#endif


/********************* Processor control/status register *********************/

#define PCR	0xe84000	/* processor controll register */
#define ESR	0xe84008	/* error status register */

	/*-- bits for pcr register --*/
	/*-- pcr[0] --*/

#define NO_ANLAUF	1
#define PCRRES0		2
#define ANLAUF		0


	/*-- pcr[1] ---*/

#define SEG_SIZE_BIT 1	/* segment size: 1==>512 bytes; 0==>256 bytes */
#define WPEN	2	/* write protect mechanismus enable */

				/*-- pcr[2] --*/

#define FPE	1	/* force parity error.	*/
#define SPYEN	2	/* SASI,I/O-parity enable */

	/*---------- bits for esr register ------------*/
	/*-- esr[0] --*/

#define WPV	2	/* write protection violation */
#define TOUT	1	/* timeout error */

	/*-- esr[1] --*/

#define PERR	1	/* parity error */

/*--------------------	----------------*/

#define PROTECT_ADDR	0xf00000
#define PROTECT_BASE	(0xc00000>>1)
#define PROTECT_RAM_LENGTH (1024<<2)

#define CROM		0xe80000	/* configuration ROM address */

#define VECTOR_TABLE	((long *)0)	/* start of interrupt vector table */
#define SW_LOAD_ADDR	((char *)0)	/* load address of ICC-SW */
#define ENTRY_ADDR	((void (**)())4)	/* pointer to entry address */

#ifdef ASSEMBLER
#define RESTART_PIKI	0x10040e	/* cold start address of Pikitor */
#else
#define RESTART_PIKI	((void (*)())0x10040e)
#endif
