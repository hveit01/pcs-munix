
/*	SccsId	@(#)mkv.h	1.2	7/26/88	*/

/*
#ident "@(#) RELEASE:  4.0  07/25/88  HEAD/SYS:mkv";
*/
/*
Modifications
vers    when    who     what
1.1     010170  NN      changed xxx to yyy (Example)
4.0     072588  mar     mkvinit - merker
*/


/*
        MGT + AQU - Parameter    (statisch)
        -----------------------------------
*/


#define MINMA  0
#define MAXMA  255 
#define MAXUK  9
#define AQL    MAXMA * MAXUK    /* Auftragsqueue length             */






/*
           mkv  data - structure
           ---------------------
*/


struct  mkv  {
        ushort mkvno;           /* eigene mkv #                     */
        ushort mkverr;          /* mkv permanent error              */
        short  intf2c;          /* fio2 Interrupt Counter           */
        short  intf2i;          /* fio2 Int. initial Count          */
        short  mkvstat;         /* mkv - Status      (see below)    */
        short  maxuk;           /* max. UK pro mars adr.            */
        short  minma;           /* kleinste vorkommende MA          */
        short  maxma;           /* groesste vorkommende MA          */
        short  aktauf;          /* laufindex fuer akt. Auftrag      */
        short  letauf;          /* laufindex fuer let. Auftrag      */
        char   sa_rech;         /* Rechner Source Adresse           */
        char   frame[5];        /* aktueller Empfangsframe
                                    0    1    2    3    4
                                   ara, afc, ada, asa, ass          */
        char   squitt;          /* puffer fuer Sendequittung        */
        ushort  abc;            /* actual byte count                */
        ushort  quidev;               /* quittungs dev. #           */
        ushort  mgt[MAXMA][MAXUK];    /* mars geraete tabelle       */
        ushort  aqu[AQL];             /* auftragsqueue              */
        ushort  mkv_init;             /* mkvinit done               */
};






/*
        mkvstat   (mkv - status)
        ------------------------
*/

#define SAKTIV  0x1
#define EAKTIV  0x2





/*
        Frame Bezeichnungen
        -------------------
*/

#define ARA 0
#define AFC 1
#define ADA 2
#define ASA 3
#define ASS 4

#define RESET 0x40
#define CONTR 0x80






/*
           mkv - pointer
           -------------
*/


struct  mkvptr  {
        int  f1_addr;            /* fio1 base adress         */
        int  f2_addr;            /* fio2 base adress         */
        struct  mkv  *ma_mkv;    /* pointer to mkv structure */
        int  mgtmin;             /* min. mars Addr.          */
        int  mgtmax;             /* max. mars Addr.          */
        int  ukanz;              /* Anzahl Unterkanaele      */
};






/*
        FIO - Register Definitionen
        ---------------------------
*/

#define REG0   0x00       /* FIO Reg. 00   Control Register 0     */
#define REG1   0x01       /*  "   "   01    "   "   "    "  1     */
#define REG2   0x02       /*  "   "   02   Int. Status Reg. 0     */
#define REG3   0x03       /*  "   "   03    "    "  "   "   1     */
#define REG4   0x04       /*  "   "   04    "    "  "   "   2     */
#define REG5   0x05       /*  "   "   05    "    "  "   "   3     */
#define REG6   0x06       /*  "   "   06   Int. Vektor Reg.       */
#define REG7   0x07       /*  "   "   07   Byte Count Reg.        */
#define REG8   0x08       /*  "   "   08   Byte Count Comp. Reg.  */
#define REG9   0x09       /*  "   "   09   Control Register 2     */
#define REGA   0x0A       /*  "   "   10    "   "   "    "  3     */
#define REGB   0x0B       /*  "   "   11   Message Out Register   */
#define REGC   0x0C       /*  "   "   12   Message In Register    */
#define REGD   0x0D       /*  "   "   13   Pattern Match Register */
#define REGE   0x0E       /*  "   "   14   Pattern Mask Register  */
#define REGF   0x0F       /*  "   "   15   Data Buffer Register   */

#define FIOINT 0x0E       /* FIO Interrupt Indicator              */
