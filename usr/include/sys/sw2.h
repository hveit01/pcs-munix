/* Header file for sw-disk driver */

/*
 *
 *	The following  structure defines the order of the Control
 *      and Status registers for the SPECTRALOGIC Controller.
 *
 */
struct	device
{
	short   sw2cs1;          /* Control and Status register 1 */
	short   sw2wc;           /* Word count register           */
	short   sw2ba;           /* UNIBUS address register       */
	short   sw2da;           /* Desired address register      */
	short   sw2cs2;          /* Control and Status register 2 */
	short   sw2ds;           /* Drive Status                  */
	short   sw2er1;          /* Error register 1              */
	short   sw2as;           /* Attention Summary             */
	short   sw2la;           /* Look ahead                    */
	short   sw2db;           /* Data buffer                   */
	short   sw2mr1;          /* Maintenance register          */
	short   sw2dt;           /* Drive type                    */
	short   sw2sn;           /* Serial number                 */
	short   sw2of;           /* Offset register               */
	short   sw2dc;           /* Desired Cylinder address register */
	short   sw2hr;           /* Current Cylinder              */
	short   sw2mr2;          /* Error register 2              */
	short   sw2er2;          /* Error register 3              */
	short   sw2ec1;          /* Burst error bit position      */
	short   sw2ec2;          /* Burst error bit pattern       */
	short   sw2bae;          /* 11/70 bus extension           */
	short   sw2cs3;          /* 11/70 control & status 3      */
};

#define SWADDR  ((struct device *)0x3FFFFCC0)
#define NSW     2               /* number of drives */

/*
 *      The following structure defines some characteristics of the
 *      available drives on the SPECTRALOGIC Controller.
 *      Some Status and Error registers for error messages are also
 *      defined.
 */
struct {
	unsigned short  ncyl;           /* number of cylinders          */
	unsigned short  nhead;          /* number of heads (tracks)     */
	unsigned short  nblks;          /* number of blocks per track   */
	unsigned short  nbtrans;        /* bytes to be transferred      */
	short           command;        /* controller command           */
	short           ntry;           /* number of allowed retries    */
	unsigned short  stat;           /* status of drive operation    */
	daddr_t         sn;             /* logical sector number        */
	unsigned short  rblks;          /* requested logical block in track */
	short           rcyl;           /* requested cylinder           */
	short           rtrk;           /* requested track              */
	long            addr;           /* address of memory transfer   */
	daddr_t         badsn;          /* bad sector number            */
	/* folling items are here for restarting IO after alternate     */
	/* sector read / write                                          */
	unsigned short  bleft;          /* remaining bytes to transfer  */
	daddr_t         sblk;           /* block for restart            */
	long            saddr;          /* memory address for restart   */
	unsigned short  cs1;            /* error reporting saved status */
	unsigned short  cs2;
	unsigned short  ds;
	unsigned short  er1;
	unsigned short  er2;
	unsigned short  wc;
	unsigned short  ba;
	unsigned short  da;
	unsigned short  dc;
	unsigned short  bae;
} sw2_r[NSW];

#define b_cylin b_resid

/*
 * SW-disk ioctl definitions
 */
#define SW_FORMAT    (('S'<<8) + 1)  /* Format off-line                 */
#define SW_REEPROM   (('S'<<8) + 2)  /* Read eeproms                    */
#define SW_WEEPROM   (('S'<<8) + 3)  /* Write eeproms                   */
#define SW_RSTPROM   (('S'<<8) + 4)  /* Reset eeproms                   */
#define SW_READBAD   (('S'<<8) + 5)  /* Read sw_badblockinfo            */
#define SW_WRITEBAD  (('S'<<8) + 6)  /* Write sw_badblockinfo           */
#define SW_CLEAR     (('S'<<8) + 7)  /* Drive clear                     */
#define SW_HDRVAL    (('S'<<8) + 8)  /* Set Header valid                */
#define SW_HDRINVAL  (('S'<<8) + 9)  /* Set Header invalid              */
#define SW_REINIT    (('S'<<8) +10)  /* init disk after formatting      */
#define SW_NORETRY_AND_ECC   (('S'<<8) + 11)
				     /* no retry and ECC for bad sector */
				     /* scan; allow access of alternate */
				     /* sectors                         */
#define SW_GETCONF   (('S'<<8) +12)  /* get physical drive data         */
#define SW_READHDR   (('S'<<8) +13)  /* read header                     */
#define SW_CONTRINIT (('S'<<8) +14)  /* reset controller after          */
				     /* programming eeprom              */

#define SWHDIO  0200000              /* read/write header               */

struct sw2ctl {
	short sw2_loc;                /* offset into eeproms             */
	short sw2_data;               /* data R from / W to eeproms      */
};

struct sw2_conf
{
	short cyls;                  /* number of actual cylinders      */
	short heads;                 /* number of actual R/W heads      */
	short sectors;               /* number of sectors per track     */
};

/*
 *       Control and Status bit definitions for  swcs1
 */
#define NOP     00              /* No operation                         */
#define GO      01              /* Start command                        */
#define RECAL   06              /* Recalibrate                          */
#define DCLR    010             /* Drive clear                          */
#define OFFSET  014             /* Offset                               */
#define RTC     016             /* Return to centerline                 */
#define PRESET  020             /* Read-in-preset                       */
#define PACK    022             /* Pack acknowledge                     */
#define SEARCH  030             /* Search                               */
#define RDPROM  040             /* Read EEPROMs                         */
#define WTPROM  042             /* Write EEPROMs                        */
#define RSPROM  044             /* Reset EEPROMs                        */
#define WCHK    050             /* Write check data                     */
#define WCOM    060             /* Write data                           */
#define WHDR    062             /* Write header and data format         */
#define FORMAT  066             /* Format drive offline                 */
#define RCOM    070             /* Read data                            */
#define RHDR    072             /* Read header and data                 */

#define IE      0100            /* interrupt enable                     */
#define CRDY    0200            /* controller ready bit                 */
#define TRE     040000          /* transfer error                       */
#define DVA     04000           /* drive available                      */

/*
 *       Control and Status bit definitions for  swcs2
 */
#define CCLR    040             /* controller clear                     */
#define NEM     04000           /* nonexistent memory error             */
#define NED     010000          /* nonexistent drive error              */

/*
 *       Control and Status bit definitions for  swds
 */
#define VV      0100            /* volume valid                         */
#define DRY     0200            /* drive ready                          */
#define DPR     0400            /* drive present                        */
#define MOL     010000          /* medium on-line                       */
#define PIP     020000          /* positioning in progress              */
#define ERR     040000          /* error (see rmer1 and/or rmer2)       */

/*
 *       Control and Status bit definitions for swer1
 */
#define ILF     01              /* illegal function code                */
#define FER     020             /* format error                         */
#define ECH     0100            /* ECC hard error                       */
#define HCE     0200            /* Header compare error                 */
#define HCRC    0400            /* Header CRC error                     */
#define IAE     02000           /* invalid address error                */
#define WLE     04000           /* write lock error                     */
#define DTE     010000          /* drive timing error                   */
#define DCK     0100000         /* ECC error                            */

/*
 *       Control and Status bit definitions for swof
 */
#define OFFDIR  0200            /* offset direction -                   */
				/* 0=away from spindle;1=towards spindle*/
#define ECI     04000           /* ECC error inhibited                  */
#define HCI     02000           /* HCI error inhibited                  */
#define SKPENB  01000           /* enable i/o for skip sector           */
#define FMT22   010000          /* 16 bit-word format                   */

/*
 *       Control and Status bit definitions for rmer2
 */
#define BSE     0100000         /* bad track error                      */
#define SKP     040             /* skip sector error                    */

#define SWPROMSIZ      2048     /* size of EEPROMS on Spectralogic      */
#define BOOTPROMSTART  1024     /* start of the boot prom;              */
				/* only used for magicnumber            */

#define NUMTRY         16       /* number of I/O retries                */
#define MAXBADSEC      127      /* max number of bad sectors            */
#define SWMAGIC         (('S'<<8) | 'W')
#define SWBADBLOCKOFFSET 2L     /* number of blocks for bad block list  */

struct sw2_badblockinfo {
     short         sw2_magic;                /* magic number             */
     short         sw2_dummy[3];             /* to get a 1024 byte block */
     daddr_t       sw2_log_bad[MAXBADSEC];   /* log. bad blocks. 0=free  */
     daddr_t       sw2_log_alter[MAXBADSEC]; /* log. alternate block for */
					    /* corresponding bad block  */
					    /* sw_log_bad. 0=free       */
};

#define SWSECSIZ        512     /* number of Bytes per sector            */
#define SWHDRSIZ        (SWSECSIZ + sizeof(struct sw2_sechdr))
				/* number of Bytes in sector with header */

struct  sw2_blkhdr {
		short sw2_flgcyl;             /* flags and cylinder number*/
		short sw2_trksec;             /* track and sector numbers */
};

struct  sw2_sechdr {                          /* sector header            */
	union {
		struct  sw2_blkhdr sw2_blkhdr; /* flags, cyl, track, sec   */
		daddr_t sw2_sec;              /* phys sector number       */
	} un_sw2;
};


/*
 *      fancy locations in EEPROM
 */
#define PARBLK_LOC         0110
#define LSPLH_LOC          0320
#define LHPLC_LOC          0520
#define LCPLDL_LOC         0720
#define LCPLDH_LOC         01120


#define CYLADDR(x,y)    ((x) / (sw2_r[y].nblks*sw2_r[y].nhead))
#define TRKSEC(x,y)     ((x) % (sw2_r[y].nblks*sw2_r[y].nhead))
#define SW_SECTOT(y)    ((long)sw2_r[y].nblks*(long)sw2_r[y].nhead*(long)sw2_r[y].ncyl)

#define SWSECTRK(y)     (sw2_r[y].nblks)
#define SWTRKCYL(y)     (sw2_r[y].nhead)
#define SWCYL(y)        (sw2_r[y].ncyl)


/*
 *      status of driver; see sw2_r[].stat
 */
#define SW_INIT         01      /* drive initialized                    */
#define SW_SYSIO        02      /* bad block IO; access to sector 0 & 1 */
#define SW_SPECIAL      04      /* bad block scan; (SW_NORETRY_AND_ECC) */
#define SW_ALTIO        010     /* alternate sector access              */


#define SW_SPARE 210    /* reserved alternate sectors:                  */
			/* should correspond with SW_SPARE in c.c       */


/*
 *  Standard RM02/03, RM05 drive parameters
 */
#define RM_NSEC         32
#define RM_NCYLS        823
#define RM_2TRK         5
#define RM_5TRK         19

#define RM03            024
#define RM02            025
#define RM05            027
