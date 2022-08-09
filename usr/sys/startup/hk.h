/*modified from 16 bit MUNIX! */

/* Header file for HK-drivers */

/*
 *
 *  The following  structure defines the order of the Control
 *  and Status registers for the RK06/07
 *
 */
struct device
{
    short   hkcs1;  /* Control and Status register 1 */
    short   hkwc;   /* Word Count Register */
    short   hkba;   /* Memory Address Register */
    short   hkda;   /* Track Sector Address Register */
    short   hkcs2;  /* Control And Status Register 2 */
    short   hkds;   /* Drive Status Register */
    short   hkerr;  /* Error Register */
    short   hkas;   /* Attention summary and Offset Register */
    short   hkdc;   /* Desired Cylinder Register */
    short   hkla;   /* Dummy for unused location; 22bit for SC02  */
    short   hkdb;   /* Data Buffer */
    short   hkmr1;  /* Maintenance Register 1 */
    short   hkec1;  /* ECC Position Register */
    short   hkec2;  /* ECC Pattern Register */
    short   hkmr2;  /* Maint. Register 2 */
    short   hkmr3;  /* Maint. Register 3 */
};

#define HKADDR  ((struct device *)0xFFFF20)      /* RK6/711 address */

#define RK06    0
#define RK07    02000

#define HKCYLSZ 33792           /* bytes per cylinder */
#define HKSECSZ 512             /* bytes per sector */
#define RK07BL  53790           /* number of sectors on RK07 disk pack */
#define RK06BL  27126           /* number of sectors on RK06 disk pack */
#define SPARE   154             /* number of sectors reserved for bad sector handling (last tracks) */
#define NHK     8       /* number of drives on system */
#define NSECT   22      /* number of sectors per track */
#define NTRAC   3   /* number of tracks per cylinder */
#define NCYL6   411     /* number of cylinders on RK06 */
#define NCYL7   815     /* number of cylinders on RK07 */
#define NHW     3       /* number of header words */
#define NUMTRY  28      /* max retry count */

#define CYLADDR(x)      x/(NSECT*NTRAC)         /* cylinder address of logical sector x */
#define TRKSEC(x)       x%(NSECT*NTRAC)         /* track and sector */
#define LOGSEC(x,y)     ((long)x*NSECT*NTRAC + ((y&0x700) >> 8)*NSECT + (y&0x1f))     /* disk address --> logical sector */
#define MIN(A,B)        ((A) < (B) ? (A) : (B))

/*
 *
 * The following definitions specify the offset values
 * used during error recovery
 *
 */
#define P25 01  /* +25 Rk06, +12.5 Rk07 */
#define M25 0201    /* -25 RK06, -12.5 RK07 */
#define P200    010 /* +200 RK06, +100 RK07 */
#define M200    0210    /* -200, RK06, -100 RK07 */
#define P400    020 /* +400  RK06 , +200  RK07 */
#define M400    0220    /* -400  RK06 , -200  RK07 */
#define P800    040 /* +800  RK06 , +400 RK07 */
#define M800    0240    /* -800  RK06 , -400  RK07 */
#define P1200   060 /* +1200  RK06 , +600  RK07 */
#define M1200   0260    /* -1200  RK06 , -600 Rk07 */

/*
*   Control and Status bit definitions for  hkcs1
*/
#define GO  01  /* GO bit */
#define SELECT  01  /* Select Function */
#define PAKACK  02  /* Pack Acknowledge Function */
#define DCLR    04  /* Drive Clear Function */
#define UNLOAD  06  /* Unload Heads Function */
#define STRSPN  010 /* Start Spindle Function */
#define RECAL   012 /* Recalibrate Function */
#define OFFSET  014 /* Offset Function */
#define SEEK    016 /* Seek Function */
#define RCOM    020 /* Read Command */
#define WCOM    022 /* Write Command */
#define RHDR    024 /* Read Header  */
#define WHDR    026 /* Write Header */
#define WCHK    030 /* Write Check Function */
#define IEI 0100    /* Interrupt Inable bit */
#define CRDY    0200    /* Controller Ready bit */
#define SEL7    02000   /* Select RK07 bit */
#define CTO 04000   /* Controller Time Out bit */
#define CFMT    010000  /* Controller Format bit */
#define DTCPAR  020000  /* Drive to Controller Parity Error */
#define DINTR   040000  /* Drive Interrupt bit */
#define CCLR    0100000 /* Controller Clear bit */
#define CERR    0100000 /* Controller Error bit */

/*
*   Control and Status bit definitions for  hkcs2
*/
#define DLT 0100000 /* Data Late error */
#define WCE 040000  /* Write Check Error */
#define UPE 020000  /* Unibus Parity Error */
#define NED 010000  /* Nonexistent Drive error */
#define NEM 04000   /* Nonexistent Memory error */
#define PGE 02000   /* Programming error */
#define MDS 01000   /* Multiple Drive Select error */
#define UFE 0400    /* Unit Field Error */
#define SCLR    040 /* Subsystem Clear bit */
#define BAI 020 /* Bus Address Increment Inhibit bit */
#define RLS 010 /* Release bit  */

/*
*   Control and Status bit definitions for  hkds
*/
#define SVAL    0100000 /* Status Valid bit */
#define CDA 040000  /* Current Drive Attention bit */
#define PIP 020000  /* Position In Progress bit */
#define WRL 04000   /* Write Lock bit */
#define DDT 0400    /* Disk Drive Type bit */
#define DRDY    0200    /* Drive Ready bit */
#define VV  0100    /* Volume Valid bit */
#define DROT    040 /* Drive Off Track Error */
#define SPLS    020 /* Speed Loss Error */
#define ACLO    010 /* Drive AC Low */
#define DOFST   04  /* Drive Offset bit */
#define DRA 01  /* Drive Available bit */

/*
*   Control and Status bit definitions for  hkerr
*/
#define DCK 0100000 /* Data Check error */
#define DUNS    040000  /* Drive Unsafe error */
#define OPI 020000  /* Operation Incomplete error */
#define DTE 010000  /* Drive Timing error */
#define DWLE    04000   /* Drive Write Lock error */
#define IDAE    02000   /* Invalid Disk Address Error */
#define COE 01000   /* Cylinder Overflow Error  */
#define HRVC    0400    /* Header Vertical Redundance Check Error */
#define BSE 0200    /* Bad Sector Error */
#define ECH 0100    /* Error Correction Hard error */
#define DTYE    040 /* Drive Type error */
#define FMTE    020 /* Format Error */
#define CDPAR   010 /* Controller To Driver Parity Error */
#define NXF 04  /* Nonexecutable Function Error */
#define SKI 02  /* Seek Incomplete error */
#define ILF 01  /* Illegal Function Error */

struct  hkstruct
{
    int  drvtyp;    /* This contains the drive type */
    char mntflg;    /* This contains a mounted flag it is set to indicate*/
    /* that a drive type is known to the system */
    char recal; /* recalibrate flag */
    int  ccyl;  /* stores drives current cylinder  */
    int  cs1;   /* Error reporting save locations */
    int  wc;
    int  ba;
    int  da;
    int  cs2;
    int  ds;
    int  err;
    int  as;
    int  dc;
    int  spare;
    int  mr1;
    int  mr2;
    int  mr3;
};
